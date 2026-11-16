#!/usr/bin/env python3
"""Stage 1 oracle (docs/GENERALIZED_GEMM_SCOPE.md): one record per loop nest, and composition
in place.

Random functions of two to four loop nests over five argument arrays and sometimes one
allocated array, in 32-bit integers, binary32 or binary64: products C[h] += A[f] * B[g] over
a box in any loop order, with offsets, scales and transposed operands; nests that share an
output; nests that read as an operand what an earlier nest wrote; a nest whose operand is its
own output; a nest whose operand is not known apart from its output; a nest that writes a
second array beside its product; a nest that zeroes its output before the reduction loop; a
nest that sets an array to a constant; a nest that copies one; and single statements between
the nests that move a value from one array to another. Each function is printed as MLIR for the nest driver
and interpreted here by plain Python loops. Nothing of the library is used: the records are
read as JSON, and what they say is recomputed from the program this file printed.

- The function: its roots, the order of its statements and nests, and how the nests meet on
  roots (same root, or two roots not known apart, with what each does to it) are recomputed
  from the printed program and must be equal, entry for entry.
- A nest that is a product over roots apart from its output, by an authorized name or by a
  fresh allocation, each of its loops with two iterations or more, must have a VERIFIED
  record whose content is the program's: M, N and K;
  which loop is I, J and K, with its bounds; for each operand its root and, per dimension,
  the level, the scale and the constant, and the offset over coordinates from zero; the
  operation, the operand order of the addition and its flags; each pair of roots with its
  basis, a fresh allocation a proof and a name an assumption; and whether the source
  permits another order: by a proof for integers, by the flag for floats, never otherwise.
  A product with a loop of one iteration need not have a complete record (the search makes
  no coordinate of a variable with one value); when it has one, the same holds of it.
- Any record with a verified ordered fold, whatever the nest: on random memories, the nest
  run here from that memory ends with the same bits as the fold the record describes, from
  that memory: operands read from the memory at the nest's entry, the contributions of each
  cell folded in the order of the record's levels with the record's own operation.
- A nest whose operand is on its output's root with cells in common, a nest whose operand is
  not known apart from its output, a nest that also writes another array, and a nest that
  sets its output to zero before its reduction loop must not be verified: for each, a run is
  shown here (for the second, a call with the two arguments on one array) at which the fold
  from the entry memory and the nest differ.
- Composition (case E). On random memories, the function is run statement by statement, and
  again with every nest that has a complete record replaced by that record's fold from the
  memory at the nest's entry, everything else left as it is and the order kept. The two final
  memories must be equal, bit for bit. No nest is taken to be independent of another.
- Where the record says a proof lets the order change, every order of a cell's contributions
  tried here gives the same bits. Where it is the source's flag, the flag is in the program,
  and on some function another order changes the bits: a permission, and no proof.
- With --domain-driver, the whole-function claims of the existing driver are read for the
  functions of two products and nothing else, and held to runs made here: where the function
  and the fold of both products' contributions from the function's entry memory differ on a
  run (a nest reads cells the other wrote), the claim over both groups is not VERIFIED; with
  a shared output and no such read it is VERIFIED, and no group's own claim is, a run
  showing that one group's contributions alone are not the function. In both, each nest's
  record is verified by itself: the shapes a whole-function claim cannot state.

Every driver run has a time limit and an address-space limit, and a timeout fails the run.
"""
import argparse
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
import itertools
import json
import random
import resource
import struct
import subprocess
import sys
import tempfile

import numpy as np  # binary32 arithmetic, exactly: Python's own floats are binary64.

TIMEOUT = 1200
MEMORY = 4 << 30
SIZE = 6
SOLVER = ["--timeout-ms=20000", "--rlimit=2000000", "--max-level=1", "--candidates-per-group=1"]
ARGUMENTS = "abcde"
FRESH = "t"  # Root 5 when the function has it.
ROLES = {"c": "ij", "a": "ik", "b": "kj"}


class Failure(Exception):
    pass


def limit():
    if MEMORY:
        resource.setrlimit(resource.RLIMIT_AS, (MEMORY, MEMORY))


def is_float(element):
    return element in ("f32", "f64")


def mul(element, a, b):
    if element == "f32":
        return float(np.float32(a) * np.float32(b))
    if element == "f64":
        return a * b
    return (a * b) & 0xFFFFFFFF


def add(element, a, b):
    if element == "f32":
        return float(np.float32(a) + np.float32(b))
    if element == "f64":
        return a + b
    return (a + b) & 0xFFFFFFFF


def bits(element, value):
    if element == "f32":
        return struct.unpack("<I", struct.pack("<f", value))[0]
    if element == "f64":
        return struct.unpack("<Q", struct.pack("<d", value))[0]
    return value & 0xFFFFFFFF


# ---------------------------------------------------------------------------------------
# Programs.


def root_of(spec, name):
    return 5 if name == FRESH else ARGUMENTS.index(name)


def gen_view(rng, operand, bounds):
    """Per dimension (role, scale, constant), with every index inside the array."""
    roles = list(ROLES[operand])
    if operand != "c" and rng.random() < 0.3:
        roles.reverse()  # A transposed operand. C keeps (I, J): its cells must not collide.
    view = []
    for role in roles:
        low, high = bounds[role]
        scale = 2 if operand != "c" and rng.random() < 0.2 and 2 * (high - 1) < SIZE else 1
        room = SIZE - 1 - scale * (high - 1)
        view.append((role, scale, rng.randint(0, min(room, 2))))
    return view


def gen_product(rng, c, a, b):
    bounds = {}
    # One product in seven has a loop of one iteration. Its variable takes one value, and the
    # search makes no coordinate of it: such a nest need not have a complete record, and
    # whatever its record says is held to the same checks.
    thin = rng.choice("ijk") if rng.random() < 0.15 else None
    for role in "ijk":
        low = rng.choice([0, 0, 1])
        bounds[role] = (low, low + (1 if role == thin else rng.randint(2, 3)))
    order = list("ijk")
    rng.shuffle(order)
    return {"kind": "product", "c": c, "a": a, "b": b, "order": order, "bounds": bounds, "thin": thin is not None,
            "views": {operand: gen_view(rng, operand, bounds) for operand in "cab"},
            "first": rng.random() < 0.8, "extra": None, "zeroed": False}


SHAPES = ["shared", "chain", "three", "self", "unnamed", "fresh", "mixed", "chain", "extra", "zeroed"]
ELEMENTS = ["i32", "f32", "f64"]


def written(statement):
    return {cell(statement["views"]["c"], point) for point in points(statement)}


def read_through(statement, name):
    """The cells the product reads as operands through the array `name`."""
    return {cell(statement["views"][operand], point) for operand in "ab" if statement[operand] == name
            for point in points(statement)}


def until(rng, make, holds):
    """A generated statement that meets a condition this file can check itself."""
    for _ in range(400):
        statement = make()
        if holds(statement):
            return statement
    raise Failure("the generator found no statement of the shape it wanted")


def gen_spec(rng, index):
    # Every shape with every element, in turn: the floors below then say something.
    shape = SHAPES[index % len(SHAPES)]
    element = ELEMENTS[(index // len(SHAPES) + index) % len(ELEMENTS)]
    spec = {"name": f"p{index}", "element": element, "shape": shape, "width": rng.choice([16, 32]),
            "flag": is_float(element) and rng.random() < 0.5, "fresh": shape == "fresh", "unnamed": set()}
    statements = []
    if shape == "shared":
        # Two products on one output, with cells in common, and operands nobody writes.
        first = gen_product(rng, "c", "a", "b")
        statements = [first, until(rng, lambda: gen_product(rng, "c", "a", "d"),
                                   lambda second: written(first) & written(second))]
    elif shape == "chain":
        # The second reads, as an operand, cells the first wrote; in floats a value of C is
        # sometimes moved into D between them.
        first = gen_product(rng, "c", "a", "b")
        statements = [first]
        if is_float(element) and rng.random() < 0.5:
            statements.append({"kind": "move", "from": ("c", rng.randrange(3), rng.randrange(3)),
                               "to": ("d", rng.randrange(3), rng.randrange(3))})
        operands = rng.choice([("c", "b"), ("a", "c"), ("c", "c")])
        statements.append(until(rng, lambda: gen_product(rng, "d", *operands),
                                lambda second: written(first) & read_through(second, "c")))
    elif shape == "three":
        statements = [gen_product(rng, "c", "a", "b"), gen_product(rng, "d", "c", "b"), gen_product(rng, "c", "d", "a")]
    elif shape == "self":
        # An operand on the output's own root, reading a cell after the nest wrote it: a
        # fold from the entry memory it is not, as a run made here shows.
        operands = rng.choice([("c", "b"), ("a", "c")])
        probe = random.Random(rng.randrange(1 << 30))
        statements = [gen_product(rng, "c", "a", "b"),
                      until(rng, lambda: gen_product(rng, "c", *operands),
                            lambda second: fold_differs(spec, second, probe, tries=6))]
    elif shape == "unnamed":
        # The second product's A is not known apart from its C: nobody says so. On one
        # array, A reads a cell after the nest wrote it, as a run made here shows.
        probe = random.Random(rng.randrange(1 << 30))
        statements = [gen_product(rng, "c", "a", "b"),
                      until(rng, lambda: gen_product(rng, "d", "e", "b"),
                            lambda second: fold_differs(spec, second, probe, {"e": "d"}, tries=6))]
        spec["unnamed"] = {(3, 4)}
    elif shape == "extra":
        # The second nest also stores a constant into E, at C's index: more than a fold.
        second = gen_product(rng, "d", "a", "b")
        second["extra"] = ("e", rng.choice([5, 7]))
        statements = [gen_product(rng, "c", "a", "b"), second]
    elif shape == "zeroed":
        # The second nest sets each cell of its output to zero before its reduction loop:
        # an assignment, and no fold from the value at the nest's entry.
        second = gen_product(rng, "d", "c", "b")
        second["order"] = [role for role in second["order"] if role != "k"] + ["k"]
        second["zeroed"] = True
        statements = [gen_product(rng, "c", "a", "b"), second]
    elif shape == "fresh":
        # t is allocated, set to a constant, accumulated into twice, and copied out.
        statements = [{"kind": "fill", "to": FRESH, "value": rng.choice([0, 0, 1])},
                      gen_product(rng, FRESH, "a", "b"), gen_product(rng, FRESH, "c", "a"),
                      {"kind": "copy", "from": FRESH, "to": "e"}]
    else:
        statements = [{"kind": "fill", "to": "c", "value": 0}, gen_product(rng, "c", "a", "b"),
                      {"kind": "move", "from": ("c", 1, 1), "to": ("a", 0, 0)}, gen_product(rng, "c", "a", "d"),
                      {"kind": "copy", "from": "c", "to": "e"}]
    spec["statements"] = statements
    return spec


def roots_of(spec):
    return list(ARGUMENTS) + ([FRESH] if spec["fresh"] else [])


def authorized(spec):
    names = []
    for x in range(5):
        for y in range(x + 1, 5):
            if (x, y) not in spec["unnamed"]:
                names.append(f"roots {x} and {y} are separate")
    return names


def index_text(view, number):
    parts = []
    for role, scale, constant in view:
        text = f"%{role}{number}"
        if scale != 1:
            text += f" * {scale}"
        if constant:
            text += f" + {constant}"
        parts.append(text)
    return ", ".join(parts)


def print_function(spec):
    e = spec["element"]
    memref = f"memref<{SIZE}x{SIZE}x{e}>"
    arguments = ", ".join(f"%{name}: {memref}" for name in ARGUMENTS)
    lines = [f"func.func @{spec['name']}({arguments}) {{"]
    if spec["fresh"]:
        lines.append(f"  %{FRESH} = memref.alloc() : {memref}")
    flag = " fastmath<reassoc>" if spec["flag"] else ""
    for number, statement in enumerate(spec["statements"]):
        kind = statement["kind"]
        if kind == "product":
            constant = (lambda value: f"{value}.0" if is_float(e) else str(value))
            if statement["zeroed"] or statement["extra"]:
                value = 0 if statement["zeroed"] else statement["extra"][1]
                lines.append(f"  %o{number} = arith.constant {constant(value)} : {e}")
            for depth, role in enumerate(statement["order"]):
                if depth == 2 and statement["zeroed"]:
                    lines.append(f"      affine.store %o{number}, %{statement['c']}"
                                 f"[{index_text(statement['views']['c'], number)}] : {memref}")
                low, high = statement["bounds"][role]
                lines.append("  " * (depth + 1) + f"affine.for %{role}{number} = {low} to {high} {{")
            pad = "        "
            if statement["extra"]:
                lines.append(f"{pad}affine.store %o{number}, %{statement['extra'][0]}"
                             f"[{index_text(statement['views']['c'], number)}] : {memref}")
            for name, operand in (("x", "a"), ("y", "b"), ("z", "c")):
                lines.append(f"{pad}%{name}{number} = affine.load %{statement[operand]}"
                             f"[{index_text(statement['views'][operand], number)}] : {memref}")
            if is_float(e):
                lines.append(f"{pad}%m{number} = arith.mulf %x{number}, %y{number} : {e}")
                operands = f"%z{number}, %m{number}" if statement["first"] else f"%m{number}, %z{number}"
                lines.append(f"{pad}%s{number} = arith.addf {operands}{flag} : {e}")
            else:
                lines.append(f"{pad}%m{number} = arith.muli %x{number}, %y{number} : {e}")
                operands = f"%z{number}, %m{number}" if statement["first"] else f"%m{number}, %z{number}"
                lines.append(f"{pad}%s{number} = arith.addi {operands} : {e}")
            lines.append(f"{pad}affine.store %s{number}, %{statement['c']}"
                         f"[{index_text(statement['views']['c'], number)}] : {memref}")
            for depth in reversed(range(3)):
                lines.append("  " * (depth + 1) + "}")
        elif kind == "fill":
            constant = f"{statement['value']}.0" if is_float(e) else str(statement["value"])
            lines += [f"  %v{number} = arith.constant {constant} : {e}",
                      f"  affine.for %i{number} = 0 to {SIZE} {{", f"    affine.for %j{number} = 0 to {SIZE} {{",
                      f"      affine.store %v{number}, %{statement['to']}[%i{number}, %j{number}] : {memref}",
                      "    }", "  }"]
        elif kind == "copy":
            lines += [f"  affine.for %i{number} = 0 to {SIZE} {{", f"    affine.for %j{number} = 0 to {SIZE} {{",
                      f"      %v{number} = affine.load %{statement['from']}[%i{number}, %j{number}] : {memref}",
                      f"      affine.store %v{number}, %{statement['to']}[%i{number}, %j{number}] : {memref}",
                      "    }", "  }"]
        else:
            (source, sr, sc), (target, tr, tc) = statement["from"], statement["to"]
            lines += [f"  %r{number} = arith.constant {sr} : index", f"  %q{number} = arith.constant {sc} : index",
                      f"  %u{number} = arith.constant {tr} : index", f"  %w{number} = arith.constant {tc} : index",
                      f"  %v{number} = memref.load %{source}[%r{number}, %q{number}] : {memref}",
                      f"  memref.store %v{number}, %{target}[%u{number}, %w{number}] : {memref}"]
    lines += ["  return", "}"]
    return "\n".join(lines) + "\n"


# ---------------------------------------------------------------------------------------
# This file's own semantics of what it printed.


def cell(view, point):
    return tuple(scale * point[role] + constant for role, scale, constant in view)


def points(statement):
    """The nest's iterations, in its own loop order."""
    ranges = [range(*statement["bounds"][role]) for role in statement["order"]]
    for values in itertools.product(*ranges):
        yield dict(zip(statement["order"], values))


def run_product(element, statement, memory, alias=None):
    def array(name):
        return memory[alias.get(name, name) if alias else name]

    low = statement["bounds"]["k"][0]
    for point in points(statement):
        at = cell(statement["views"]["c"], point)
        if statement["zeroed"] and point["k"] == low:
            # k is the innermost loop of such a nest: this is the store before it.
            array(statement["c"])[at] = 0.0 if is_float(element) else 0
        if statement["extra"]:
            name, value = statement["extra"]
            array(name)[at] = float(value) if is_float(element) else value
        product = mul(element, array(statement["a"])[cell(statement["views"]["a"], point)],
                      array(statement["b"])[cell(statement["views"]["b"], point)])
        old = array(statement["c"])[at]
        array(statement["c"])[at] = add(element, old, product) if statement["first"] else add(element, product, old)


def run_statement(spec, statement, memory):
    e = spec["element"]
    kind = statement["kind"]
    if kind == "product":
        run_product(e, statement, memory)
    elif kind == "fill":
        value = float(statement["value"]) if is_float(e) else statement["value"]
        for at in itertools.product(range(SIZE), repeat=2):
            memory[statement["to"]][at] = value
    elif kind == "copy":
        for at in itertools.product(range(SIZE), repeat=2):
            memory[statement["to"]][at] = memory[statement["from"]][at]
    else:
        (source, sr, sc), (target, tr, tc) = statement["from"], statement["to"]
        memory[target][(tr, tc)] = memory[source][(sr, sc)]


def random_memory(spec, rng):
    e = spec["element"]
    memory = {}
    for name in roots_of(spec):
        cells = {}
        for at in itertools.product(range(SIZE), repeat=2):
            if is_float(e):
                # Mostly small, sometimes far apart in magnitude, so that an order can matter.
                value = rng.choice([rng.randint(-8, 8) * 0.5, rng.randint(-8, 8) * 0.5, rng.randint(-8, 8) * 0.5,
                                    rng.choice([-1, 1]) * (2.0 ** rng.choice([20, 24, 30])),
                                    rng.choice([-1, 1]) * (2.0 ** rng.choice([-20, -24]))])
                cells[at] = float(np.float32(value)) if e == "f32" else value
            else:
                cells[at] = rng.choice([rng.randrange(1 << 32), rng.randint(0, 9), (1 << 31) - rng.randint(0, 3)])
        memory[name] = cells
    return memory


def copy_memory(memory):
    return {name: dict(cells) for name, cells in memory.items()}


def same_memory(element, a, b):
    return all(bits(element, a[name][at]) == bits(element, b[name][at]) for name in a for at in a[name])


def accesses(statement):
    """Per array name: 1 read, 2 write."""
    out = Counter()
    kind = statement["kind"]
    if kind == "product":
        for operand in "ab":
            out[statement[operand]] |= 1
        out[statement["c"]] |= 3
        if statement["extra"]:
            out[statement["extra"][0]] |= 2
    elif kind == "fill":
        out[statement["to"]] |= 2
    elif kind == "copy":
        out[statement["from"]] |= 1
        out[statement["to"]] |= 2
    return out


def apart(spec, x, y):
    """How two roots stand: 'proof' with the allocation, 'assumption' by a name, else None."""
    if x == y:
        return None
    if 5 in (x, y):
        return "proof"
    return None if (min(x, y), max(x, y)) in spec["unnamed"] else "assumption"


def access_names(mask):
    return [name for bit, name in ((1, "read"), (2, "write")) if mask & bit]


# ---------------------------------------------------------------------------------------
# The record, read with nothing but what it says.


def element_of(record):
    name = record["gemm"]["accumulation"]["element"]
    return {"bv32": "i32", "f32": "f32", "f64": "f64"}.get(name)


def record_fold(spec, record, memory, permute=None):
    """The fold the record describes, from `memory`: operands read from the entry memory, the
    contributions of each cell folded in the order of the levels. `permute` reorders each
    cell's contributions, to ask whether an order matters."""
    gemm = record["gemm"]
    e = element_of(record)
    if e is None:
        raise Failure(f"{spec['name']}: an element this oracle does not know: {gemm['accumulation']['element']}")
    names = roots_of(spec)
    arrays = {operand: names[gemm["views"][operand]["root"]["root"]] for operand in "CAB"}
    ranges = []
    for level in gemm["levels"]:
        values = range(level["lower"], level["upper"])
        ranges.append(reversed(values) if level["descending"] else values)
    first = gemm["accumulation"]["accumulator_first"]

    def index(operand, values):
        out = []
        for dimension in gemm["views"][operand]["dimensions"]:
            value = dimension["constant"]
            for term in dimension["levels"]:
                value += term["coefficient"] * values[term["level"]]
            out.append(value)
        return tuple(out)

    contributions = {}
    for values in itertools.product(*ranges):
        product = mul(e, memory[arrays["A"]][index("A", values)], memory[arrays["B"]][index("B", values)])
        contributions.setdefault(index("C", values), []).append(product)
    out = copy_memory(memory)
    for at, products in contributions.items():
        if permute:
            products = permute(products)
        value = memory[arrays["C"]][at]
        for product in products:
            value = add(e, value, product) if first else add(e, product, value)
        out[arrays["C"]][at] = value
    return out


def run_driver(command):
    try:
        done = subprocess.run(command, capture_output=True, text=True, timeout=TIMEOUT, preexec_fn=limit)
    except subprocess.TimeoutExpired:
        raise Failure("timeout: " + " ".join(command))
    if done.returncode != 0:
        raise Failure(f"exit {done.returncode}: {' '.join(command)}\n{done.stderr[-2000:]}")
    return json.loads(done.stdout)


def check_function(spec, out, counts):
    name = spec["name"]
    if out["function"] != name or out["status"] != "OUTLINED" or out["authorizes_lowering"] is not False:
        raise Failure(f"{name}: the function's header: {out['status']} {out['reason']}")
    roots = [(root["root"], root["origin"], root.get("argument")) for root in out["roots"]]
    expected = [(n, "argument", n) for n in range(5)] + ([(5, "allocation", None)] if spec["fresh"] else [])
    if roots != expected:
        raise Failure(f"{name}: roots {roots}, expected {expected}")
    # The order: the allocation, then each statement where it is.
    order = [("allocation", 5)] if spec["fresh"] else []
    nests = []
    for statement in spec["statements"]:
        if statement["kind"] == "move":
            order += [("access", root_of(spec, statement["from"][0]), ("read",)),
                      ("access", root_of(spec, statement["to"][0]), ("write",))]
        else:
            order.append(("nest", len(nests)))
            nests.append(statement)
    seen = []
    for entry in out["order"]:
        if entry["kind"] == "nest":
            seen.append(("nest", entry["nest"]))
        elif entry["kind"] == "allocation":
            seen.append(("allocation", entry["root"]))
        elif entry["kind"] == "access":
            seen.append(("access", entry["root"], tuple(entry["access"])))
        else:
            seen.append((entry["kind"],))
    if seen != order:
        raise Failure(f"{name}: order {seen}, expected {order}")
    counts["orders recomputed"] += 1
    # How the nests meet, by root.
    expected = set()
    for p in range(len(nests)):
        for q in range(p + 1, len(nests)):
            for x, first in sorted(accesses(nests[p]).items()):
                for y, second in sorted(accesses(nests[q]).items()):
                    if not (first | second) & 2:
                        continue
                    rx, ry = root_of(spec, x), root_of(spec, y)
                    if rx == ry:
                        relation = "same root"
                    elif apart(spec, rx, ry) is None:
                        relation = "roots not known apart"
                    else:
                        continue
                    expected.add((p, q, relation, rx, ry, tuple(access_names(first)), tuple(access_names(second))))
    found = {(d["earlier"], d["later"], d["relation"], d.get("earlier_root"), d.get("later_root"),
              tuple(d.get("earlier_access", ())), tuple(d.get("later_access", ()))) for d in out["dependences"]}
    if found != expected or len(out["dependences"]) != len(expected):
        raise Failure(f"{name}: dependences {sorted(found)}, expected {sorted(expected)}")
    counts["dependence lists recomputed"] += 1
    counts["dependences"] += len(expected)
    if len(out["nests"]) != len(nests):
        raise Failure(f"{name}: {len(out['nests'])} records for {len(nests)} nests")
    return nests


def check_separation(spec, statement, record, counts):
    names = sorted({root_of(spec, name) for name in accesses(statement)})
    expected = {}
    for x, y in itertools.combinations(names, 2):
        expected[(x, y)] = apart(spec, x, y) or "unknown"
    found = {tuple(pair["roots"]): pair["kind"] for pair in record["separation"]}
    if found != expected:
        raise Failure(f"{spec['name']} nest {record['nest']}: separation {found}, expected {expected}")
    for pair in record["separation"]:
        if (pair["kind"] == "proof") != (pair["basis"] == "fresh allocation") or \
                pair["apart"] != (pair["kind"] != "unknown") or \
                pair["name"] != f"roots {pair['roots'][0]} and {pair['roots'][1]} are separate":
            raise Failure(f"{spec['name']} nest {record['nest']}: a separation entry: {pair}")
    counts["separation lists recomputed"] += 1
    counts["separations by a proof"] += sum(kind == "proof" for kind in found.values())


def check_product_fields(spec, statement, record, counts):
    where = f"{spec['name']} nest {record['nest']}"
    gemm = record["gemm"]
    e = spec["element"]
    if record["status"] != "VERIFIED" or record["refusals"] or not record["ordered_fold_verified"]:
        raise Failure(f"{where}: a product over roots apart from its output is {record['status']}: "
                      f"{record['refusals']}")
    bounds, order = statement["bounds"], statement["order"]
    extents = {"M": bounds["i"][1] - bounds["i"][0], "N": bounds["j"][1] - bounds["j"][0],
               "K": bounds["k"][1] - bounds["k"][0]}
    if gemm["extents"] != extents or not gemm["box"] or gemm["guard"] != "true":
        raise Failure(f"{where}: extents {gemm['extents']}, expected {extents}")
    levels = [(level["axis"], level["lower"], level["upper"], level["extent"], level["descending"])
              for level in gemm["levels"]]
    expected = [(role.upper(), bounds[role][0], bounds[role][1], bounds[role][1] - bounds[role][0], False)
                for role in order]
    if levels != expected:
        raise Failure(f"{where}: levels {levels}, expected {expected}")
    if gemm["axes"] != {"P": [], **{role.upper(): [order.index(role)] for role in "ijk"}}:
        raise Failure(f"{where}: axes {gemm['axes']}")
    for operand in "cab":
        view = gemm["views"][operand.upper()]
        if view["root"]["root"] != root_of(spec, statement[operand]) or not (view["proposed"] and view["affine"]
                                                                            and view["proved"]):
            raise Failure(f"{where}: the view of {operand.upper()}: {view['root']} {view['fact']}")
        found = []
        for dimension in view["dimensions"]:
            found.append((dimension["affine"], dimension["constant"],
                          [(term["level"], term["coefficient"]) for term in dimension["levels"]],
                          [(term["role"], term["scale"]) for term in dimension["roles"]], dimension["offset"],
                          dimension["parameters"]))
        expected = [(True, constant, [(order.index(role), scale)], [(role.upper(), scale)],
                     constant + scale * bounds[role][0], []) for role, scale, constant in statement["views"][operand]]
        if found != expected:
            raise Failure(f"{where}: the view of {operand.upper()} is {found}, expected {expected}")
    accumulation = gemm["accumulation"]
    expected = {"reduction": "fadd" if is_float(e) else "add", "multiply": "fmul" if is_float(e) else "mul",
                "element": "bv32" if e == "i32" else e, "accumulator_first": statement["first"], "sites": 1,
                "sites_agree": True, "fold": "ORDERED_FLOAT" if is_float(e) else "MODULAR_INTEGER"}
    for key, value in expected.items():
        if accumulation[key] != value:
            raise Failure(f"{where}: accumulation.{key} is {accumulation[key]}, expected {value}")
    flags = ["reassoc"] if spec["flag"] else []
    if gemm["reduction_flags"] != flags or gemm["multiply_flags"] or gemm["frame"] != {"other_reads": 0,
                                                                                      "other_writes": 0}:
        raise Failure(f"{where}: flags {gemm['reduction_flags']} {gemm['multiply_flags']}, frame {gemm['frame']}")
    theorem = gemm["tiers"]["theorem13"]
    if (theorem["status"], theorem["tier"], theorem["logical_tier"]) != ("VERIFIED", "SYMBOLIC", "SYMBOLIC"):
        raise Failure(f"{where}: Theorem 13 is {theorem}")
    counts["products with every field equal to the program's"] += 1


def check_reassociation(spec, statement, record, rng, fills, counts):
    """Whether the order may change, and on what basis. `statement` is None for a nest this
    file did not construct as a product apart from its output."""
    where = f"{spec['name']} nest {record['nest']}"
    info = record["reassociation"]
    e = spec["element"]
    permitted, basis = info["permitted"], info["basis"]
    if record["reassociation_permitted"] != permitted or (basis == "none") == permitted:
        raise Failure(f"{where}: reassociation {info}")
    if is_float(e):
        if info["source_flag_reassoc"] != spec["flag"]:
            raise Failure(f"{where}: the source's flag is {spec['flag']}, the record says {info['source_flag_reassoc']}")
        if basis == "source flag" and not spec["flag"]:
            raise Failure(f"{where}: a permission by a flag the program does not have")
    elif info["source_flag_reassoc"] is not None or basis == "source flag":
        raise Failure(f"{where}: an integer fold with a float's flag: {info}")
    if statement is not None:
        k = statement["bounds"]["k"][1] - statement["bounds"]["k"][0]
        if not is_float(e) and (not permitted or basis != "proof"):
            raise Failure(f"{where}: a wrapping integer fold is not permitted to reorder: {info}")
        if is_float(e) and k > 1 and (permitted != spec["flag"] or (permitted and basis != "source flag")):
            raise Failure(f"{where}: a float fold of {k} contributions, flag {spec['flag']}: {info}")
        if is_float(e) and k == 1 and not spec["flag"] and permitted and basis != "proof":
            raise Failure(f"{where}: one contribution per cell and no flag: {info}")
        counts["reassociation by the rule, " + (basis if permitted else "refused")] += 1
    if not record["ordered_fold_verified"] or record["status"] != "VERIFIED":
        return
    # A proof: no order of a cell's contributions gives other bits. A flag: one may.
    changed = False
    for _ in range(fills):
        memory = random_memory(spec, rng)
        straight = record_fold(spec, record, memory)
        for permute in (lambda products: list(reversed(products)),
                        lambda products: rng.sample(products, len(products))):
            other = record_fold(spec, record, memory, permute)
            if not same_memory(e, straight, other):
                changed = True
                if permitted and basis == "proof":
                    raise Failure(f"{where}: the record says a proof lets the order change, and another order "
                                  "gives other bits")
        counts["folds in another order"] += 2
    if permitted and basis == "proof":
        counts["orders that a proof permits, tried"] += 1
    if permitted and basis == "source flag":
        counts["orders that a flag permits, tried"] += 1
        counts["of them, some order changes the bits"] += changed
    if not permitted:
        counts["orders not permitted, tried"] += 1
        counts["of them, some order changes the bits (unflagged)"] += changed


def fold_differs(spec, statement, rng, alias=None, tries=40):
    """Whether, on some memory, the nest and the fold of its contributions from the memory at
    its entry end with other bits. With `alias`, in a call where an operand's argument and
    the output's are one array."""
    e = spec["element"]
    for _ in range(tries):
        memory = random_memory(spec, rng)
        for operand_name, output_name in (alias or {}).items():
            memory[operand_name] = dict(memory[output_name])  # One array, under two names.
        after = copy_memory(memory)
        for operand_name, output_name in (alias or {}).items():
            after[operand_name] = after[output_name]
        run_product(e, statement, after)
        folded = copy_memory(memory)
        contributions = {}
        for point in points(statement):
            product = mul(e, memory[statement["a"]][cell(statement["views"]["a"], point)],
                          memory[statement["b"]][cell(statement["views"]["b"], point)])
            contributions.setdefault(cell(statement["views"]["c"], point), []).append(product)
        for at, products in contributions.items():
            value = memory[statement["c"]][at]
            for product in products:
                value = add(e, value, product) if statement["first"] else add(e, product, value)
            folded[statement["c"]][at] = value
        if not same_memory(e, folded, after):
            return True
    return False


def check_negative(spec, statement, record, rng, counts):
    """A nest that must not be verified, with a run that shows why."""
    where = f"{spec['name']} nest {record['nest']}"
    e = spec["element"]
    c = root_of(spec, statement["c"])
    alias, reason = None, None
    for operand in "ab":
        r = root_of(spec, statement[operand])
        if r == c:
            reason = "an operand on the output's root"
        elif apart(spec, r, c) is None:
            alias = {statement[operand]: statement["c"]}
            reason = "an operand not known apart from the output"
    if statement["extra"]:
        reason = "another write in the nest"
    if statement["zeroed"]:
        reason = "the output set to a constant in the nest"
    if reason is None:
        return False
    # The fold from the entry memory, against the nest: for the second kind, in a call with
    # the two arguments on one array.
    if fold_differs(spec, statement, rng, alias):
        if record["ordered_fold_verified"] or record["status"] in ("VERIFIED", "VERIFIED_NOT_MAPPED"):
            raise Failure(f"{where}: {reason}, a run where the fold from the entry memory and the nest "
                          f"differ, and the record says {record['status']}")
        counts["not verified, with a run that differs: " + reason] += 1
        return True
    counts["negative shapes with no differing run found"] += 1
    return True


def check(spec, seed, driver, domain_driver, fills, counts):
    rng = random.Random(seed)
    name = spec["name"]
    e = spec["element"]
    with tempfile.NamedTemporaryFile("w", suffix=".mlir") as source:
        source.write(print_function(spec))
        source.flush()
        common = [source.name, f"--function={name}", f"--index-bit-width={spec['width']}",
                  "--authorize=" + ",".join(authorized(spec))]
        out = run_driver([driver] + common + SOLVER)[0]
        whole = None
        products = [s for s in spec["statements"] if s["kind"] == "product"]
        if domain_driver and spec["shape"] in ("shared", "chain") and len(products) == 2 and \
                len(spec["statements"]) == 2:
            whole = run_driver([domain_driver] + common + SOLVER + ["--lower-structured", "--segments",
                                                                    "--groups=all"])[0]
    nests = check_function(spec, out, counts)
    counts["programs " + e] += 1
    counts["programs of shape " + spec["shape"]] += 1
    complete = {}
    for statement, record in zip(nests, out["nests"]):
        where = f"{name} nest {record['nest']}"
        if record["function"] != name or record["authorizes_lowering"] is not False:
            raise Failure(f"{where}: the record's header")
        counts["records"] += 1
        counts["records " + record["status"]] += 1
        check_separation(spec, statement, record, counts)
        if statement["kind"] != "product":
            if record["status"] != "NO_GEMM" or record["gemm"] is not None or record["ordered_fold_verified"]:
                raise Failure(f"{where}: a nest with no product is {record['status']}")
            counts["nests with no product, said so"] += 1
            continue
        negative = check_negative(spec, statement, record, rng, counts)
        ruled = not negative and (not statement["thin"] or record["status"] == "VERIFIED")
        if ruled:
            check_product_fields(spec, statement, record, counts)
        elif not negative:
            counts["products with a loop of one iteration and no complete record: " + record["status"]] += 1
        if record["gemm"] is not None:
            check_reassociation(spec, statement if ruled else None, record, rng, fills, counts)
        # Any verified fold, on its own memories: the nest against what the record describes.
        if record["ordered_fold_verified"] and record["status"] == "VERIFIED":
            for _ in range(fills):
                memory = random_memory(spec, rng)
                after = copy_memory(memory)
                run_product(e, statement, after)
                if not same_memory(e, after, record_fold(spec, record, memory)):
                    raise Failure(f"{where}: the nest and the record's fold differ on a memory")
                counts["nests against their record's fold"] += 1
            complete[id(statement)] = record
        elif record["ordered_fold_verified"]:
            counts["verified folds with no box"] += 1

    # Composition in place: every complete record's fold from the memory at its nest's entry.
    for _ in range(fills):
        memory = random_memory(spec, rng)
        if spec["fresh"]:
            memory[FRESH] = {at: 0.0 if is_float(e) else 0 for at in memory[FRESH]}  # Set before any read.
        straight = copy_memory(memory)
        for statement in spec["statements"]:
            run_statement(spec, statement, straight)
        composed = copy_memory(memory)
        for statement in spec["statements"]:
            record = complete.get(id(statement))
            if record is None:
                run_statement(spec, statement, composed)
            else:
                composed = record_fold(spec, record, composed)
        if not same_memory(e, straight, composed):
            raise Failure(f"{name}: with {len(complete)} nests replaced by their records' folds, the final "
                          "memory differs from the function's")
        counts["compositions equal to the function"] += 1
    counts["nests replaced in compositions"] += len(complete)
    if len(complete) >= 2:
        counts["programs with two or more nests replaced"] += 1
    if len(complete) == len(products):
        counts["programs with every product replaced"] += 1

    if whole is not None:
        # The existing whole-function claims, as they are today, for two products and
        # nothing else. A claim over both groups says the function's memory is the fold of
        # every contribution from the function's entry memory; a group's own claim says it
        # of that group's contributions alone. Each is held to a run made here.
        sets = whole.get("group_sets", [])
        joint = sets[0]["reduction"]["status"] if sets else None
        alone = [candidate["theorem13"]["status"] for group in whole["groups"] for candidate in group["candidates"]]

        def differs(chosen):
            """Whether, on some memory, the function and the fold of the chosen products'
            contributions from the entry memory end with other bits."""
            for _ in range(20):
                memory = random_memory(spec, rng)
                after = copy_memory(memory)
                for statement in products:
                    run_product(e, statement, after)
                folded = copy_memory(memory)
                for statement in chosen:
                    for point in points(statement):
                        product = mul(e, memory[statement["a"]][cell(statement["views"]["a"], point)],
                                      memory[statement["b"]][cell(statement["views"]["b"], point)])
                        at = cell(statement["views"]["c"], point)
                        old = folded[statement["c"]][at]
                        folded[statement["c"]][at] = add(e, old, product) if statement["first"] else add(e, product, old)
                if not same_memory(e, after, folded):
                    return True
            return False

        if differs(products):
            if joint == "VERIFIED":
                raise Failure(f"{name}: the function and the fold from its entry memory differ on a run, and the "
                              "claim over both groups is VERIFIED")
            counts["cross read: the joint claim is not verified, each record is"] += len(complete) == 2
        elif spec["shape"] == "shared":
            if joint != "VERIFIED":
                raise Failure(f"{name}: two nests on one output and no cross read, and the claim over both groups "
                              f"is {joint}")
            if len(whole["groups"]) == 2 and all(differs([one]) for one in products):
                if any(status == "VERIFIED" for status in alone):
                    raise Failure(f"{name}: a group's own whole-function claim is VERIFIED, and a run differs")
                counts["shared output: the joint claim is verified, no group's own is, each record is"] += \
                    len(complete) == 2
        counts["whole-function claims read"] += 1


MINIMUM = {
    "programs i32": 6, "programs f32": 4, "programs f64": 2,
    "orders recomputed": 24, "dependence lists recomputed": 24, "dependences": 30, "separation lists recomputed": 50,
    "separations by a proof": 4,
    "products with every field equal to the program's": 30,
    "nests against their record's fold": 100,
    "nests with no product, said so": 4,
    "compositions equal to the function": 80,
    "programs with two or more nests replaced": 12,
    "nests replaced in compositions": 30,
    "not verified, with a run that differs: an operand on the output's root": 2,
    "not verified, with a run that differs: an operand not known apart from the output": 2,
    "not verified, with a run that differs: another write in the nest": 2,
    "not verified, with a run that differs: the output set to a constant in the nest": 2,
    "reassociation by the rule, proof": 8, "reassociation by the rule, source flag": 3,
    "reassociation by the rule, refused": 3,
    "orders that a proof permits, tried": 8, "orders that a flag permits, tried": 3,
    "of them, some order changes the bits": 1,
}
WHOLE_MINIMUM = {
    "whole-function claims read": 3,
    "cross read: the joint claim is not verified, each record is": 1,
    "shared output: the joint claim is verified, no group's own is, each record is": 1,
}


def main():
    global MEMORY
    parser = argparse.ArgumentParser()
    parser.add_argument("--driver", required=True)
    parser.add_argument("--domain-driver", default="")
    parser.add_argument("--programs", type=int, default=30)
    parser.add_argument("--fills", type=int, default=4)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--jobs", type=int, default=2)
    parser.add_argument("--no-minimum", action="store_true")
    parser.add_argument("--memory-limit-gb", type=int, default=MEMORY >> 30)
    parser.add_argument("--print", type=int, default=-1, help="print program N and stop")
    args = parser.parse_args()
    MEMORY = args.memory_limit_gb << 30
    rng = random.Random(args.seed)
    specs = [gen_spec(rng, i) for i in range(args.programs)]
    seeds = [rng.randrange(1 << 30) for _ in specs]
    if args.print >= 0:
        print(print_function(specs[args.print]))
        return
    counts, failures = Counter(), []

    def one(item):
        spec, seed = item
        local = Counter()
        try:
            check(spec, seed, args.driver, args.domain_driver, args.fills, local)
        except Failure as failure:
            return str(failure), local
        return None, local

    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        for failure, local in pool.map(one, zip(specs, seeds)):
            counts.update(local)
            if failure:
                failures.append(failure)
    for key in sorted(counts):
        print(f"{counts[key]:6d}  {key}")
    if not args.no_minimum:
        minimum = dict(MINIMUM)
        if args.domain_driver:
            minimum.update(WHOLE_MINIMUM)
        for key, least in minimum.items():
            if counts[key] < least:
                failures.append(f"only {counts[key]} of '{key}', fewer than {least}: the check is vacuous")
    for failure in failures[:10]:
        print("FAIL:", failure, file=sys.stderr)
    print(f"{len(specs)} programs, {len(failures)} failures")
    sys.exit(1 if failures else 0)


if __name__ == "__main__":
    main()
