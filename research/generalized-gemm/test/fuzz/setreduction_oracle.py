#!/usr/bin/env python3
"""G5b-M4b-1 oracle: Theorem 17 over a set of groups.

Random programs of nests run one after the other, each nest one MAC site C[h] += A[f]*B[g]
over its own root triple, integer or double, from four argument roots and sometimes a fresh
allocation: some nests share a triple, some write the same output root with other operands,
some write another root, some read another nest's output as an operand, some put two
groups' sites in one loop body. They are printed as MLIR for the domain driver, which is
asked with --segments --groups for the union of every group in both listing orders and for
each group alone, and interpreted here by plain Python loops. The driver's `reduction`
entry is recomputed with no library code:

- The verdict's tier and status follow its components: Symbolic only when both symbolic
  claims are VERIFIED with an exact order; a finite verdict lists only points both finite
  claims list; REFUTED names the component that refuted it; a Verified verdict has every
  pair of output roots DISTINCT.
- Theorem 17, where VERIFIED, at each listed point (finite) or every point of the scope
  (symbolic), with random memories: the program's final memory equals, cell by cell, the
  fold of the cell's contributions in the request's order, segment by segment and within a
  segment in the order's lambda order (each segment's set enumerated here from the printed
  order, each event placed by its state), each contribution the product of the initial
  operands at the event's indices, each step the program's exact operation: wrapping 32-bit
  integers, or IEEE doubles in that exact order. For doubles the fold with two segments'
  contributions to a shared cell swapped must differ on some program, or the run is
  vacuous; for integers it never differs.
- REFUTED by the computation: the printed witness memory is run here, and the program's
  final memory differs from the fold with initial operands. REFUTED by the domain:
  Theorem 15 fails over the union at the printed point.
- The listing changes nothing, and a set of one group has the status and tier of that
  group's own Theorem 13 on the same request.
- The reorder facts: `trivial` PROVEN only where no cell has two contributions at any point
  of the scope and all output roots are apart; `float` PROVEN only with it.
- G5b-M4b-2a, the integer fact: PROVEN by `trivial`, or by the rule, which this file reads
  off the program it printed: a 32-bit fold, no overflow flag, no load of an output root
  between the nests (the observer knob), no stop on an accumulate load (the stop knob); then
  the fact's tier is the verdict's, the union's program with its nests in the other order
  (where no nest of the union reads a root another writes: the fact fixes the products and
  licenses no move of a read) and with a repeat loop reversed ends with the same bits on
  every written cell, and every permutation of a cell's contributions folds to the same
  value. REFUTED only with a flag,
  and the witness is run here: the program completes, and the reversed fold of some cell
  overflows a signed 32-bit addition. A plain 32-bit union with two contributions to a cell
  is never left Unknown; a knob never lets the rule prove.

Every driver run has a time limit and an address-space limit, and a timeout fails the run.
"""
import argparse
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
from functools import reduce
import itertools
import json
import random
import resource
import struct
import subprocess
import sys
import tempfile

TIMEOUT = 1200
MEMORY = 4 << 30
WIDTH = 16
SIZE = 16
SOLVER = ["--timeout-ms=3000", "--rlimit=2000000"]
ROOTS = {"a": 0, "b": 1, "c": 2, "d": 3, "e": 4, "f": 4}  # e, an observer's root, and f, a fresh allocation, never meet.
NAMES = "abcdf"


class Failure(Exception):
    pass


class Stopped(Exception):
    """The interpreted run stopped: a flagged addition overflowed. Outside every claim."""


def limit():
    if MEMORY:
        resource.setrlimit(resource.RLIMIT_AS, (MEMORY, MEMORY))


# ---------------------------------------------------------------------------------------
# Terms the driver prints (the domain oracle's evaluator, copied so this file stands alone).


def wrap(value, width):
    return value & ((1 << width) - 1)


def signed(value, width):
    value = wrap(value, width)
    return value - (1 << width) if value >> (width - 1) else value


PREDICATES = {0: lambda a, b, w: a == b, 1: lambda a, b, w: a != b,
              2: lambda a, b, w: signed(a, w) < signed(b, w), 3: lambda a, b, w: signed(a, w) <= signed(b, w),
              4: lambda a, b, w: signed(a, w) > signed(b, w), 5: lambda a, b, w: signed(a, w) >= signed(b, w),
              6: lambda a, b, w: a < b, 7: lambda a, b, w: a <= b, 8: lambda a, b, w: a > b, 9: lambda a, b, w: a >= b}


def term(t, state, params, levels):
    w = t["width"]
    if "const" in t:
        return wrap(int(t["const"]), w)
    if "state" in t:
        return wrap(state[t["state"]], w)
    if "param" in t:
        return wrap(params[t["param"]], w)
    if "level" in t:
        return wrap(levels[t["level"]], w)
    op, args = t["op"], t["args"]
    if op == "ite":
        return term(args[1] if term(args[0], state, params, levels) else args[2], state, params, levels)
    v = [term(a, state, params, levels) for a in args]
    inner = args[0]["width"]
    if op == "add": return wrap(v[0] + v[1], w)
    if op == "sub": return wrap(v[0] - v[1], w)
    if op == "mul": return wrap(v[0] * v[1], w)
    if op == "udiv": return wrap((1 << w) - 1, w) if v[1] == 0 else v[0] // v[1]
    if op == "urem": return v[0] if v[1] == 0 else v[0] % v[1]
    if op == "shl": return 0 if v[1] >= w else wrap(v[0] << v[1], w)
    if op == "lshr": return 0 if v[1] >= w else v[0] >> v[1]
    if op == "ashr": return wrap(signed(v[0], w) >> min(v[1], w - 1), w)
    if op == "and": return v[0] & v[1]
    if op == "or": return v[0] | v[1]
    if op == "xor": return v[0] ^ v[1]
    if op == "not": return wrap(~v[0], w)
    if op == "icmp": return int(PREDICATES[t["predicate"]](v[0], v[1], inner))
    if op == "zext": return v[0]
    if op == "sext": return wrap(signed(v[0], inner), w)
    if op == "trunc": return wrap(v[0], w)
    raise Failure(f"the oracle does not know {op}")


def enumerate_set(order, params):
    levels, out = order["levels"], []

    def less(level, a, b):
        w = level["value"]["width"]
        return (signed(a, w) < signed(b, w)) if level["signed"] else a < b

    def go(m, current):
        if len(out) > 1 << 16:
            raise Failure("the set is too large for the oracle")
        if m == len(levels):
            if order["guard"] is None or term(order["guard"], {}, params, current):
                out.append(tuple(current))
            return
        level = levels[m]
        w = level["value"]["width"]
        lo, hi = term(level["lower"], {}, params, current), term(level["upper"], {}, params, current)
        if not less(level, lo, hi):
            return
        span = wrap(hi - lo, w)
        for k in (range(span - 1, -1, -1) if level["descending"] else range(span)):
            go(m + 1, current + [wrap(lo + k, w)])

    go(0, [])
    return out


# ---------------------------------------------------------------------------------------
# Programs.


def gen_site(rng, loops, roots, out=None, operand_roots=("a", "b")):
    v = rng.choice(loops)[0] if loops else None
    h = ("var", v, rng.choice([0, 0, 1])) if v else ("const", rng.randrange(3))
    if loops and len(loops) > 1 and rng.random() < 0.3:
        h = ("sum", loops[0][0], loops[1][0])
    outs = ["c", "d"] + (["f"] if "f" in roots else [])
    a, b = operand_roots
    return {"out": out or rng.choice(outs), "a": a, "b": b, "h": h}


def gen_nest(rng, number, roots, shape):
    loops = []
    for level in range(rng.choice([1, 1, 2])):
        var = f"v{number}_{level}"
        bound = ("outer",) if level == 1 and rng.random() < 0.3 else (
            ("n",) if rng.random() < 0.8 else ("const", rng.randint(1, 3)))
        loops.append((var, bound))
    nest = {"loops": loops, "when": 1 if rng.random() < 0.2 else None, "sites": [gen_site(rng, loops, roots)]}
    if shape == "interleaved" and number == 0:
        first = nest["sites"][0]
        other = "d" if first["out"] == "c" else "c"
        nest["sites"].append(gen_site(rng, loops, roots, out=other, operand_roots=("b", "a")))
    return nest


def gen_spec(rng, index):
    shape = ["shared_out", "distinct_out", "same_triple", "reads_output", "interleaved", "repeat", "fresh_out"][index % 7]
    element = "f64" if index % 2 else "i32"
    # G5b-M4b-2a: one thing the integer rule must see, on integer programs: an overflow
    # flag on every add, a load of an output root between the nests stored to a root e of
    # its own, or a stop on the second nest's accumulate load. An observer's root is root 4,
    # as a fresh allocation is, so the two never meet.
    # 3i mod 4 walks the knobs so that every knob meets the shared-cell shapes within 30
    # programs (index 14: shared_out with nsw; 26: repeat with the stop; 28: shared_out observed).
    knob = ["none", "nsw", "observer", "stop"][(3 * (index // 2)) % 4] if element == "i32" else "none"
    # A flagged program keeps its outputs in argument roots: a fresh root starts every cell at
    # zero, and with two contributions a reversal from zero overflows exactly when the program
    # does, so no witness could exist there.
    fresh = shape == "fresh_out" or (knob not in ("observer", "nsw") and rng.random() < 0.25)
    if knob == "observer" and fresh:
        knob = "none"
    roots = ["a", "b", "c", "d"] + (["f"] if fresh else [])
    nests = [gen_nest(rng, k, roots, shape) for k in range(rng.randint(2, 3))]
    first, second = nests[0]["sites"][0], nests[1]["sites"][0]
    same_index = ("var", nests[1]["loops"][0][0], first["h"][2] if first["h"][0] == "var" else 0)
    if shape == "shared_out":       # Two groups, one output root, other operands, overlapping cells.
        second.update(out=first["out"], a="b", b="d" if first["out"] != "d" else "c", h=same_index)
    elif shape == "distinct_out":   # Another output root, the same operands.
        second.update(out="d" if first["out"] == "c" else "c", a=first["a"], b=first["b"])
    elif shape == "same_triple":    # One group over two nests.
        second.update(out=first["out"], a=first["a"], b=first["b"])
    elif shape == "reads_output":   # The second reads what the first writes: refuted by M3.
        second.update(out="d" if first["out"] == "c" else "c", a=first["out"], b="b")
    elif shape == "repeat":         # The second nest's inner loop is a repeat: several contributions.
        inner = ("v1_r", ("const", 2))
        nests[1]["loops"].append(inner)
        second.update(out=first["out"], a="b", b="d" if first["out"] != "d" else "c", h=same_index)
    elif shape == "fresh_out":      # The second writes the fresh root, whose initializing stores
        second.update(out="f", a=first["a"], b=first["b"])  # keep M3's symbolic claim Unknown.
    lo = rng.choice([1, 1, 2])
    return {"name": f"r{index}", "element": element, "fresh": fresh, "nests": nests,
            "scope": (lo, lo + rng.choice([1, 2])), "shape": shape,
            "flags": "nsw" if knob == "nsw" else None, "observer": knob == "observer", "stop": knob == "stop"}


def knobs(spec):
    """The knobs set on this program: the flag, the observer, the stop."""
    return [k for k in (spec.get("flags"), "observer" if spec.get("observer") else None,
                        "stop" if spec.get("stop") else None) if k]


def blockers(spec, union_sites):
    """What keeps the integer rule from proving a set whose sites are `union_sites`, read off
    the program: a flag is on every add; a load of a root the set writes by a site outside
    the set (an operand, or that site's own accumulate load) is an effect outside the union
    that reads an output root, whatever its place in the order; the observer reads the first
    nest's output root; the stop reads the second nest's accumulate load, so it blocks the
    sets that write the second nest's output root. A set none of them touches has partial
    sums that decide nothing."""
    sites = sites_of(spec)
    outs = {ROOTS[site["out"]] for number, (nest, site) in enumerate(sites) if number in union_sites}
    out = []
    if spec.get("flags"):
        out.append(spec["flags"])
    if any(ROOTS[site[k]] in outs for number, (nest, site) in enumerate(sites) if number not in union_sites
           for k in ("a", "b", "out")):
        out.append("another nest reads a root the set writes")
    if spec.get("observer") and ROOTS[spec["nests"][0]["sites"][0]["out"]] in outs:
        out.append("observer")
    if spec.get("stop") and any(ROOTS[site["out"]] in outs for site in spec["nests"][1]["sites"]):
        out.append("stop")
    return out


def sites_of(spec):
    return [(k, site) for k, nest in enumerate(spec["nests"]) for site in nest["sites"]]


def print_function(spec):
    e = spec["element"]
    t = f"memref<{SIZE}x{e}>"
    mul, add = ("arith.mulf", "arith.addf") if e == "f64" else ("arith.muli", "arith.addi")
    observer = f", %e: {t}" if spec.get("observer") else ""
    lines = [f"func.func @{spec['name']}(%a: {t}, %b: {t}, %c: {t}, %d: {t}{observer}, %n: index) {{",
             "  %c0 = arith.constant 0 : index", "  %c1 = arith.constant 1 : index",
             "  %c2 = arith.constant 2 : index", "  %c3 = arith.constant 3 : index"]
    if spec.get("stop"):
        lines.append("  %one = arith.constant 1 : i32")
    flag = " overflow<nsw>" if spec.get("flags") == "nsw" else ""
    if spec["fresh"]:
        zero = "arith.constant 0.0 : f64" if e == "f64" else "arith.constant 0 : i32"
        lines += [f"  %f = memref.alloc() : {t}", f"  %zero = {zero}", f"  %size = arith.constant {SIZE} : index",
                  "  scf.for %w = %c0 to %size step %c1 {", f"    memref.store %zero, %f[%w] : {t}", "  }"]
    count = [0]

    def name(base):
        count[0] += 1
        return f"%{base}{count[0]}"

    for k, nest in enumerate(spec["nests"]):
        indent, closing, outer = "  ", 0, None
        if nest["when"] is not None:
            test = name("w")
            lines += [f"{indent}{test} = arith.cmpi ult, %c{nest['when']}, %n : index", f"{indent}scf.if {test} {{"]
            indent += "  "
            closing += 1
        for var, bound in nest["loops"]:
            if bound[0] == "n":
                upper = "%n"
            elif bound[0] == "const":
                upper = f"%c{bound[1]}"
            else:
                upper = name("u")
                lines.append(f"{indent}{upper} = arith.addi %{outer}, %c1 : index")
            lines.append(f"{indent}scf.for %{var} = %c0 to {upper} step %c1 {{")
            indent += "  "
            closing += 1
            outer = var
        for site in nest["sites"]:
            h = site["h"]
            if h[0] == "const":
                index = f"%c{h[1]}"
            elif h[0] == "sum":
                index = name("x")
                lines.append(f"{indent}{index} = arith.addi %{h[1]}, %{h[2]} : index")
            elif h[2] == 0:
                index = f"%{h[1]}"
            else:
                index = name("x")
                lines.append(f"{indent}{index} = arith.addi %{h[1]}, %c{h[2]} : index")
            x, y, z, m, s = (name(b) for b in "xyzms")
            lines += [f"{indent}{x} = memref.load %{site['a']}[{index}] : {t}",
                      f"{indent}{y} = memref.load %{site['b']}[{index}] : {t}",
                      f"{indent}{z} = memref.load %{site['out']}[{index}] : {t}"]
            if spec.get("stop") and k == 1:
                # A stop that reads the accumulate load: never taken (the divisor is odd), so
                # every run completes, and the integer rule must still refuse.
                odd, q = name("o"), name("q")
                lines += [f"{indent}{odd} = arith.ori {z}, %one : i32", f"{indent}{q} = arith.divui %one, {odd} : i32"]
            lines += [f"{indent}{m} = {mul} {x}, {y} : {e}",
                      f"{indent}{s} = {add} {z}, {m}{flag} : {e}",
                      f"{indent}memref.store {s}, %{site['out']}[{index}] : {t}"]
        for _ in range(closing):
            indent = indent[:-2]
            lines.append(f"{indent}}}")
        if k == 0 and spec.get("observer"):
            # A load of the first nest's output root between the nests, stored to e: an
            # effect outside the groups that sees a partial sum.
            first = spec["nests"][0]["sites"][0]["out"]
            lines += [f"  %o = memref.load %{first}[%c0] : {t}", f"  memref.store %o, %e[%c0] : {t}"]
    lines += ["  return", "}"]
    return "\n".join(lines) + "\n"


def interpret(spec, n, memory=None):
    """Events in order: site, nest, output root, index, operand roots and indices, env; with
    a memory, the run too: products and the final memory. Operands are read as they are at
    the event, so a run that reads what an earlier nest wrote is what the program does."""
    events = []
    ids = {id(site): number for number, (k, site) in enumerate(sites_of(spec))}
    mem = None if memory is None else {r: list(cells) for r, cells in memory.items()}
    e = spec["element"]

    def index(h, env):
        if h[0] == "const":
            return h[1]
        if h[0] == "sum":
            return env[h[1]] + env[h[2]]
        return env[h[1]] + h[2]

    def combine(z, a, b):
        if e == "f64":
            return z + a * b
        product = wrap(a * b, 32)
        total = wrap(z + product, 32)
        if spec.get("flags") == "nsw" and signed(z, 32) + signed(product, 32) != signed(total, 32):
            raise Stopped()
        return total

    for k, nest in enumerate(spec["nests"]):
        if k == 1 and spec.get("observer") and mem is not None:
            mem[ROOTS["e"]][0] = mem[ROOTS[spec["nests"][0]["sites"][0]["out"]]][0]
        if nest["when"] is not None and not nest["when"] < n:
            continue

        def go(level, env):
            if level == len(nest["loops"]):
                for site in nest["sites"]:
                    x = index(site["h"], env)
                    if not 0 <= x < SIZE:
                        raise Failure("index out of bounds in the oracle's program")
                    event = {"site": ids[id(site)], "nest": k, "root": ROOTS[site["out"]], "h": x,
                             "a": (ROOTS[site["a"]], x), "b": (ROOTS[site["b"]], x), "env": dict(env)}
                    if mem is not None:
                        ra, rb, ro = ROOTS[site["a"]], ROOTS[site["b"]], ROOTS[site["out"]]
                        mem[ro][x] = combine(mem[ro][x], mem[ra][x], mem[rb][x])
                    events.append(event)
                return
            var, bound = nest["loops"][level]
            upper = n if bound[0] == "n" else bound[1] if bound[0] == "const" else env[nest["loops"][level - 1][0]] + 1
            values = reversed(range(upper)) if var in spec.get("reverse", ()) else range(upper)
            for value in values:
                env[var] = value
                go(level + 1, env)
            env.pop(var, None)

        go(0, {"n": n})
    return events, mem


def model_groups(spec):
    groups, known = [], {}
    for number, (k, site) in enumerate(sites_of(spec)):
        key = (ROOTS[site["out"]], ROOTS[site["a"]], ROOTS[site["b"]])
        if key not in known:
            known[key] = len(groups)
            groups.append({"output": key[0], "first": key[1], "second": key[2], "sites": []})
        groups[known[key]]["sites"].append(number)
    return groups


def designed_memory(spec):
    """A memory on which the order of two contributions shows. Output roots hold 2^53; an
    operand only the second nest reads holds -2^53; every other operand holds 1. A cell at
    2^53 taking +1 then -2^53 ends at 0, and taking -2^53 then +1 ends at 1. For integers the
    same roles get 2^31, 2^32 - 1 and 1, wrapped."""
    big = 9007199254740992.0
    sites = sites_of(spec)
    outs = {ROOTS[site["out"]] for _, site in sites}
    first_ops = {ROOTS[sites[0][1][k]] for k in "ab"}
    second_ops = {ROOTS[site[k]] for nest, site in sites if nest == 1 for k in "ab"}
    memory = {}
    for r in range(4):
        if spec["element"] == "f64":
            value = big if r in outs else -big if r in second_ops - first_ops else 1.0
        elif spec.get("flags") == "nsw":
            # Outputs at INT_MAX, an operand only the first nest reads at -1, the rest 1:
            # contributions -1 then +1 complete in the program's order and overflow reversed.
            value = (1 << 31) - 1 if r in outs else ((1 << 32) - 1) if r in first_ops - second_ops else 1
        else:
            value = (1 << 31) if r in outs else ((1 << 32) - 1) if r in second_ops - first_ops else 1
        memory[r] = [value] * SIZE
    if spec["fresh"] or spec.get("observer"):
        memory[4] = [0.0 if spec["element"] == "f64" else 0] * SIZE
    return memory


def random_memory(spec, rng):
    """Every argument root filled at random; the fresh root as the program leaves it, zero."""
    if spec["element"] == "f64":
        # Operands near one and at 2^53, outputs at 0 and 2^53: a sum that rounds away a
        # small contribution in one order and keeps it in the other.
        big = 9007199254740992.0
        operands = [1.0, 1.0, -1.0, big, -big, 0.5, 3.0]
        outputs = [0.0, 1.0, big, -big, 0.75, 2.0]
        memory = {ROOTS[r]: [rng.choice(operands) for _ in range(SIZE)] for r in "ab"}
        memory.update({ROOTS[r]: [rng.choice(outputs) for _ in range(SIZE)] for r in "cd"})
    else:
        memory = {ROOTS[r]: [rng.randrange(1 << 32) for _ in range(SIZE)] for r in "abcd"}
    if spec["fresh"] or spec.get("observer"):
        memory[4] = [0.0 if spec["element"] == "f64" else 0] * SIZE
    return memory


def bits(value, element):
    if element == "f64":
        return struct.pack("<d", value)
    return struct.pack("<I", wrap(value, 32))


def hex_to_value(text, element):
    raw = int(text, 16)
    if element == "f64":
        return struct.unpack("<d", struct.pack("<Q", raw))[0]
    return wrap(raw, 32)


def run_driver(driver, spec, names, sets):
    with tempfile.NamedTemporaryFile("w", suffix=".mlir") as handle:
        handle.write(print_function(spec))
        handle.flush()
        low, high = spec["scope"]
        command = [driver, handle.name, "--lower-structured", f"--index-bit-width={WIDTH}", f"--scope=0:{low}:{high}",
                   "--segments", "--max-level=1", "--candidates-per-group=1", "--print-events",
                   "--authorize=" + ",".join(names), "--groups=" + ";".join(sets)] + SOLVER
        try:
            done = subprocess.run(command, capture_output=True, text=True, timeout=TIMEOUT, preexec_fn=limit)
        except subprocess.TimeoutExpired:
            raise Failure(f"{spec['name']}: the driver timed out, which is no pass")
    if done.returncode:
        raise Failure(f"{spec['name']}: driver exit {done.returncode}:\n{done.stderr[-1200:]}")
    return json.loads(done.stdout)[0]


# ---------------------------------------------------------------------------------------
# Checks.


def segment_maps(spec, out, segments, group_of_site, where):
    maps = []
    for seg in segments:
        sites = set(seg["sites"])
        g = group_of_site[seg["sites"][0]]
        candidates = {}
        for table in out["groups"][g].get("tables", []):
            if "events" not in table:
                continue
            n = table["parameters"][0]
            mine = [e for e in interpret(spec, n)[0] if group_of_site.get(e["site"]) == g]
            theirs = table["events"]
            if [e["site"] for e in mine] != [e["site"] for e in theirs]:
                raise Failure(f"{where} n={n}: group {g}'s events differ from the driver's table")
            for ours, driver in zip(mine, theirs):
                if ours["site"] not in sites:
                    continue
                for id_, value in enumerate(driver["state"]):
                    options = set() if value is None else (
                        {k for k, v in ours["env"].items() if wrap(v, WIDTH) == wrap(value, WIDTH)} | {("const", value)})
                    candidates[id_] = candidates.get(id_, options) & options
        maps.append({id_: sorted(opts, key=str)[0] for id_, opts in candidates.items() if opts})
    return maps


def state_values(mapping, env):
    return {id_: (src[1] if isinstance(src, tuple) else env[src]) for id_, src in mapping.items()}


def ordered_events(spec, n, segments, maps):
    """U's events in the request's order: segment by segment, each in its set's order. None
    with the reason when Theorem 15 fails here."""
    where = {site: s for s, seg in enumerate(segments) for site in seg["sites"]}
    events = [e for e in interpret(spec, n)[0] if e["site"] in where]
    order = [where[e["site"]] for e in events]
    for k in range(1, len(order)):
        if order[k] < order[k - 1]:
            return None, f"event {k} goes back from segment {order[k - 1]} to {order[k]}"
    out = []
    for s, seg in enumerate(segments):
        mine = [e for e in events if where[e["site"]] == s]
        try:
            lam = [tuple(term(level["value"], state_values(maps[s], e["env"]), [n], [])
                         for level in seg["levels"]) for e in mine]
        except KeyError:
            return None, f"segment {s}: a level reads a state the oracle cannot place"
        expected = enumerate_set(seg, [n])
        if lam != expected:
            return None, f"segment {s}: its events are not its set in order"
        out += mine
    return out, None


def theorem17_fold(spec, memory, ordered):
    """The final memory Theorem 17 states: each cell folded from its initial value with the
    products of the initial operands, in the given order of events."""
    e = spec["element"]
    final = {r: list(cells) for r, cells in memory.items()}
    for event in ordered:
        (ra, ia), (rb, ib) = event["a"], event["b"]
        product = memory[ra][ia] * memory[rb][ib]
        root, x = event["root"], event["h"]
        final[root][x] = final[root][x] + product if e == "f64" else wrap(final[root][x] + wrap(product, 32), 32)
    return final


def same_cells(a, b, element, cells):
    """The given cells agree, bit for bit. A set claim speaks for the cells the union's events
    write: Theorem 16 is stated for them, and M3's F1 keeps other writes off them. A cell of
    the same root that only another group writes is that group's, and outside the claim."""
    return all(bits(a[r][i], element) == bits(b[r][i], element) for r, i in cells)


def check_verified(spec, out, entry, verdict, segments, group_of_site, union_sites, rng, fills, counts, label):
    where = f"{spec['name']} {label}"
    maps = segment_maps(spec, out, segments, group_of_site, where)
    low, high = spec["scope"]
    scope = list(range(low, high + 1))
    points = [p[0] for p in verdict["points"]] if verdict["tier"] in ("POINT", "SCOPE_ENUMERATED") else scope
    if any(n not in scope for n in points):
        raise Failure(f"{where}: a point outside the scope")
    if verdict["tier"] == "SCOPE_ENUMERATED":
        # The whole scope: every point was examined, and every point at which the union has
        # an event is listed; a point with no event has nothing to fold, as in Theorem 13.
        eventful = [n for n in scope if any(e["site"] in union_sites for e in interpret(spec, n)[0])]
        if sorted(points) != eventful:
            raise Failure(f"{where}: the whole scope lists {sorted(points)}, the union has events at {eventful}")
    swaps_differ = 0
    for n in points:
        ordered, wrong = ordered_events(spec, n, segments, maps)
        if wrong:
            raise Failure(f"{where} n={n}: Theorem 15 is false under a verified verdict: {wrong}")
        counts["points against Theorem 17"] += 1
        written = {(e["root"], e["h"]) for e in ordered}
        for fill in range(fills):
            memory = designed_memory(spec) if fill == 0 else random_memory(spec, rng)
            try:
                _, actual = interpret(spec, n, memory)
            except Stopped:
                counts["fills whose run stops"] += 1  # A flagged add overflowed: outside the claim.
                continue
            stated = theorem17_fold(spec, memory, ordered)
            if not same_cells(actual, stated, spec["element"], written):
                raise Failure(f"{where} n={n}: Theorem 17 is false: the fold in the request's order is not the "
                              f"program's final memory\n{print_function(spec)}")
            counts["Theorem 17 folds"] += 1
            # Swap the first two contributions of different segments to one cell, if any.
            cells = {}
            for k, event in enumerate(ordered):
                cells.setdefault((event["root"], event["h"]), []).append(k)
            for members in cells.values():
                segs = [[s for s, seg in enumerate(segments) if ordered[k]["site"] in seg["sites"]][0] for k in members]
                pair = next(((members[i], members[j]) for i in range(len(members)) for j in range(i + 1, len(members))
                             if segs[i] != segs[j]), None)
                if pair is None:
                    continue
                swapped = list(ordered)
                swapped[pair[0]], swapped[pair[1]] = swapped[pair[1]], swapped[pair[0]]
                other = theorem17_fold(spec, memory, swapped)
                differs = not same_cells(stated, other, spec["element"], written)
                if spec["element"] == "i32":
                    counts["integer swaps checked"] += 1
                    if differs:
                        raise Failure(f"{where}: an integer swap changed a wrapping sum")
                else:
                    counts["float swaps checked"] += 1
                    if differs:
                        swaps_differ += 1
                break
    if swaps_differ:
        counts["float swaps that change the result"] += 1
    counts[f"{verdict['tier']} verified"] += 1
    # The facts.
    trivial = verdict["reorder"]["trivial"]
    if trivial["status"] == "PROVEN":
        for n in scope:
            seen = Counter((e["root"], e["h"]) for e in interpret(spec, n)[0]
                           if any(e["site"] in seg["sites"] for seg in segments))
            if any(c > 1 for c in seen.values()):
                raise Failure(f"{where} n={n}: trivial proven with a cell of two contributions")
        counts["trivial proven checked"] += 1
    fact = verdict["reorder"]["float"]
    if fact["status"] == "PROVEN" and trivial["status"] != "PROVEN":
        raise Failure(f"{where}: float reorder proven without trivial")
    if fact["status"] == "REFUTED":
        raise Failure(f"{where}: a float reorder refutation, which M4b-2a does not make")
    check_integer_fact(spec, verdict, trivial, points, segments, maps, rng, fills, counts, where, label, union_sites)


def overflows(start, products):
    """Whether folding `products` onto `start` overflows a signed 32-bit addition at some step."""
    acc = signed(start, 32)
    for product in products:
        acc += signed(product, 32)
        if not -(1 << 31) <= acc < (1 << 31):
            return True
    return False


def contributions_of(ordered, memory):
    """Per written cell, its products in the given order, from the initial memory."""
    cells = {}
    for event in ordered:
        (ra, ia), (rb, ib) = event["a"], event["b"]
        cells.setdefault((event["root"], event["h"]), []).append(wrap(memory[ra][ia] * memory[rb][ib], 32))
    return cells


def check_integer_fact(spec, verdict, trivial, points, segments, maps, rng, fills, counts, where, label, union_sites):
    """G5b-M4b-2a: the integer fact against the program this file printed."""
    fact = verdict["reorder"]["integer"]
    blocked = blockers(spec, union_sites)
    plain = spec["element"] == "i32" and not blocked
    if fact["status"] == "PROVEN":
        if trivial["status"] == "PROVEN":
            counts["integer proven by trivial"] += 1
            return
        if not plain:
            raise Failure(f"{where}: integer reorder proven by the rule on {spec['element']} with {blocked}")
        if verdict["fold"] != "MODULAR_INTEGER":
            raise Failure(f"{where}: integer reorder proven on an ordered fold")
        if fact["tier"] != verdict["tier"]:
            raise Failure(f"{where}: the integer fact's tier {fact['tier']} is not the verdict's {verdict['tier']}")
        counts["integer proven by the rule, structure checked"] += 1
        # The union's program in another order is another program: its nests reversed, and a
        # repeat loop run backwards. On every written cell both end as the program does. The
        # fact speaks of the fold's order with the products fixed, and licenses no move of a
        # read: where a nest of the union reads a root another nest of it writes (allowed when
        # every such read precedes the writes), reversing the nests changes that nest's
        # operands, which is not a reordering of any cell's contributions, so the nests are
        # not reversed there; a repeat loop reads nothing another nest writes within itself.
        others = []
        sites = sites_of(spec)
        outs = {ROOTS[site["out"]] for number, (nest, site) in enumerate(sites) if number in union_sites}
        cross = any(ROOTS[site[k]] in outs for number, (nest, site) in enumerate(sites) if number in union_sites
                    for k in ("a", "b"))
        if label == "all" and not cross:
            others.append(dict(spec, nests=list(reversed(spec["nests"]))))
        repeats = {var for nest in spec["nests"] for var, bound in nest["loops"] if bound[0] == "const" and bound[1] > 1}
        if label == "all" and repeats:
            others.append(dict(spec, reverse=repeats))
        for n in points:
            ordered, wrong = ordered_events(spec, n, segments, maps)
            if wrong:
                raise Failure(f"{where} n={n}: {wrong}")
            written = {(e["root"], e["h"]) for e in ordered}
            for fill in range(fills):
                memory = designed_memory(spec) if fill == 0 else random_memory(spec, rng)
                _, mine = interpret(spec, n, memory)
                for other in others:
                    _, theirs = interpret(other, n, memory)
                    if not same_cells(mine, theirs, "i32", written):
                        raise Failure(f"{where} n={n}: the program in another order ends differently under a proven "
                                      f"integer reorder fact\n{print_function(spec)}")
                    counts["programs compared in another order"] += 1
                # Every permutation of a cell's contributions, folded with wrapping addition.
                for (root, x), products in contributions_of(ordered, memory).items():
                    if len(products) < 2 or len(products) > 5:
                        continue
                    folds = {reduce(lambda acc, p: wrap(acc + p, 32), perm, memory[root][x])
                             for perm in itertools.permutations(products)}
                    if len(folds) != 1:
                        raise Failure(f"{where} n={n}: two permutations of wrapping additions differ")
                    counts["permutations folded"] += 1
        return
    if fact["status"] == "REFUTED":
        if spec["element"] != "i32" or not spec.get("flags"):
            raise Failure(f"{where}: an integer reorder refutation with no overflow flag")
        if not fact.get("memory") or not fact.get("point"):
            raise Failure(f"{where}: an integer refutation without its witness")
        n = fact["point"][0]
        memory = {r: [hex_to_value(text, "i32") if text is not None else 0 for text in cells]
                  for r, cells in enumerate(fact["memory"])}
        try:
            interpret(spec, n, memory)
        except Stopped:
            raise Failure(f"{where}: the witness run stops, so it is no witness")
        ordered, wrong = ordered_events(spec, n, segments, maps)
        if wrong:
            raise Failure(f"{where} n={n}: {wrong}")
        reversed_overflows = False
        for (root, x), products in contributions_of(ordered, memory).items():
            if len(products) < 2:
                continue
            if overflows(memory[root][x], products):
                raise Failure(f"{where}: the forward fold overflows, yet the run completed")
            reversed_overflows = reversed_overflows or overflows(memory[root][x], list(reversed(products)))
        if not reversed_overflows:
            raise Failure(f"{where}: no cell's reversed fold overflows: the witness shows nothing")
        counts["integer refutations replayed"] += 1
        return
    if plain and trivial["status"] != "PROVEN":
        raise Failure(f"{where}: integer reorder Unknown on a plain 32-bit union: {fact['detail']}")
    if blocked and trivial["status"] != "PROVEN":
        counts["negative shapes checked"] += 1


def check_refuted(spec, out, entry, verdict, segments, group_of_site, union_sites, counts, label):
    where = f"{spec['name']} {label}"
    by = verdict["refuted_by"]
    if by == "COMPUTATION":
        if verdict["computation_finite"]["status"] != "REFUTED" and verdict["computation_symbolic"]["status"] != "REFUTED":
            raise Failure(f"{where}: refuted by the computation, which is not refuted")
        witness = verdict.get("witness")
        if not witness:
            raise Failure(f"{where}: a computation refutation without its witness")
        n = witness["parameters"][0]
        memory = {r: [hex_to_value(text, spec["element"]) if text is not None else 0 for text in cells]
                  for r, cells in enumerate(witness["memory"])}
        try:
            events, actual = interpret(spec, n, memory)
        except Stopped:
            raise Failure(f"{where}: the computation's witness run stops, so it is no witness")
        # Theorem 16's relation for the union: its events in execution order, from the initial
        # operands, against what the whole program made of the union's output roots.
        mine = [e for e in events if e["site"] in union_sites]
        stated = theorem17_fold(spec, memory, mine)
        if same_cells(actual, stated, spec["element"], {(e["root"], e["h"]) for e in mine}):
            raise Failure(f"{where}: the witness shows no difference between the program and the replay")
        counts["computation refutations replayed"] += 1
    elif by == "DOMAIN":
        if verdict["domain_finite"]["status"] != "REFUTED" and verdict["domain_symbolic"]["status"] != "REFUTED":
            raise Failure(f"{where}: refuted by the domain, which is not refuted")
        point = verdict.get("failing_point")
        if not point or segments is None:
            raise Failure(f"{where}: a domain refutation without its point")
        maps = segment_maps(spec, out, segments, group_of_site, where)
        ordered, wrong = ordered_events(spec, point[0], segments, maps)
        if not wrong:
            raise Failure(f"{where}: refuted at n={point[0]}, where Theorem 15 holds")
        counts["domain refutations replayed"] += 1
    else:
        raise Failure(f"{where}: REFUTED by nothing")


def check(spec, seed, driver, fills, counts):
    rng = random.Random(seed)
    groups = model_groups(spec)
    group_of = {site: g for g, group in enumerate(groups) for site in group["sites"]}
    sets = ["all", "all-reversed"] + [str(g) for g in range(len(groups))]
    roots = 5 if spec["fresh"] or spec.get("observer") else 4
    names = [f"roots {i} and {j} are separate" for i in range(roots) for j in range(i + 1, roots)]
    out = run_driver(driver, spec, names, sets)
    if out.get("gate") != "OPEN":
        raise Failure(f"{spec['name']}: gate {out.get('gate')}: {out.get('reason')}")
    entries = out["group_sets"]
    if len(entries) != len(sets):
        raise Failure(f"{spec['name']}: {len(entries)} sets answered")
    if entries[0]["reduction"] != entries[1]["reduction"]:
        raise Failure(f"{spec['name']}: the listing changed the verdict")
    counts["listings compared"] += 1
    for k, entry in enumerate(entries):
        label = "all" if k < 2 else f"group {k - 2}"
        listed = list(range(len(groups))) if k == 0 else list(reversed(range(len(groups)))) if k == 1 else [k - 2]
        if entry["groups"] != listed:
            raise Failure(f"{spec['name']}: set {k} lists {entry['groups']}")
        union_sites = {site for g in listed for site in groups[g]["sites"]}
        verdict = entry["reduction"]
        segments = entry["request"]["segments"] if "request" in entry else None
        if verdict["authorizes_lowering"] or verdict["transformation"] != "UNKNOWN" or verdict["logical_tier"] != "PROPOSED":
            raise Failure(f"{spec['name']} {label}: a later phase's claim")
        comp = (verdict["computation_finite"], verdict["computation_symbolic"])
        dom = (verdict["domain_finite"], verdict["domain_symbolic"])
        status, tier = verdict["status"], verdict["tier"]
        if status == "VERIFIED":
            if tier == "SYMBOLIC":
                if comp[1]["status"] != "VERIFIED" or dom[1]["status"] != "VERIFIED" or not entry["symbolic"]["exact_order"]:
                    raise Failure(f"{spec['name']} {label}: symbolic without both symbolic claims and an exact order")
            else:
                if comp[0]["status"] != "VERIFIED" or dom[0]["status"] != "VERIFIED":
                    raise Failure(f"{spec['name']} {label}: finite without both finite claims")
                listed = {tuple(p["parameters"]) for p in entry["finite"]["points"]}
                if any(tuple(p) not in listed for p in verdict["points"]):
                    raise Failure(f"{spec['name']} {label}: a point the domain did not list")
                if tier == "SCOPE_ENUMERATED" and (comp[0]["tier"] != "SCOPE_ENUMERATED" or dom[0]["tier"] != "SCOPE_ENUMERATED"):
                    raise Failure(f"{spec['name']} {label}: the whole scope from a partial claim")
            if any(p["relation"] != "DISTINCT" for p in entry["finite"]["root_pairs"] + entry["symbolic"]["root_pairs"]):
                raise Failure(f"{spec['name']} {label}: verified with roots that may alias")
            # A cross-group read: at no verified point may an event of the union read a cell an
            # earlier event of the union wrote (Theorem 16 reads the initial operands). The
            # interleaved and reads_output shapes are refuted where their nests run together;
            # at a point where the guarded nest does not run, a verdict at that point alone is
            # right, so the check is at the verified points, not by shape.
            verified_points = ([p[0] for p in verdict["points"]] if verdict["tier"] in ("POINT", "SCOPE_ENUMERATED")
                               else list(range(spec["scope"][0], spec["scope"][1] + 1)))
            for n in verified_points:
                written = set()
                for e in interpret(spec, n)[0]:
                    if e["site"] not in union_sites:
                        continue
                    if e["a"] in written or e["b"] in written:
                        raise Failure(f"{spec['name']} {label} n={n}: a cross-group read verified")
                    written.add((e["root"], e["h"]))
            if spec["shape"] in ("interleaved", "reads_output") and label == "all":
                counts[f"{spec['shape']} union verified only where its nests do not meet"] += 1
            check_verified(spec, out, entry, verdict, segments, group_of, union_sites, rng, fills, counts, label)
        elif status == "REFUTED":
            check_refuted(spec, out, entry, verdict, segments, group_of, union_sites, counts, label)
            if label == "all":
                counts[f"all refuted by {verdict['refuted_by'].lower()}, {spec['shape']}"] += 1
        else:
            counts["UNKNOWN verdicts"] += 1
            if any(c["status"] == "REFUTED" for c in comp + dom):
                raise Failure(f"{spec['name']} {label}: a refuted component under an Unknown verdict")
        # A set of one group against the group's own Theorem 13 on the same request.
        if k >= 2:
            candidates = out["groups"][k - 2]["candidates"]
            if candidates and "request" in candidates[0] and candidates[0]["request"] == entry.get("request"):
                t13 = candidates[0]["theorem13"]
                if (t13["status"], t13["tier"]) != (status, tier):
                    raise Failure(f"{spec['name']} group {k - 2}: Theorem 17 says {status} {tier}, Theorem 13 "
                                  f"{t13['status']} {t13['tier']} on the same request")
                counts["one-group verdicts equal to Theorem 13"] += 1
                # G5b-M4b-2a: the facts too, status and tier, where the verdict is verified. A
                # set verdict refused or refuted before its facts are computed carries default
                # facts (M4b-1), while Theorem 13 still reports the domain's trivial fact; there
                # the set's integer and float facts must be Unknown.
                theirs, mine = candidates[0].get("reorder", {}), verdict["reorder"]
                if status == "VERIFIED":
                    for name in ("trivial", "integer", "float"):
                        if (theirs.get(name, {}).get("status"), theirs.get(name, {}).get("tier")) != (mine[name]["status"], mine[name]["tier"]):
                            raise Failure(f"{spec['name']} group {k - 2}: the set's {name} fact {mine[name]['status']} "
                                          f"{mine[name]['tier']} differs from Theorem 13's {theirs.get(name)}")
                    counts["one-group facts equal to Theorem 13"] += 1
                elif any(mine[name]["status"] != "UNKNOWN" for name in ("integer", "float")):
                    raise Failure(f"{spec['name']} group {k - 2}: a reorder fact on a verdict that is not verified")
    counts[f"programs {spec['element']}"] += 1


MINIMUM = {
    "SYMBOLIC verified": 20, "SCOPE_ENUMERATED verified": 3, "points against Theorem 17": 100,
    "Theorem 17 folds": 400, "float swaps checked": 20, "float swaps that change the result": 3,
    "integer swaps checked": 20, "computation refutations replayed": 3, "domain refutations replayed": 3,
    "trivial proven checked": 5, "listings compared": 20, "one-group verdicts equal to Theorem 13": 20,
    "programs f64": 10, "programs i32": 10,
    # G5b-M4b-2a. Plain integer unions with a shared cell are one program in eight, so the
    # floors are low; a run that meets none of them proves nothing about the fact.
    "integer proven by the rule, structure checked": 5, "programs compared in another order": 5,
    "permutations folded": 50, "negative shapes checked": 5, "one-group facts equal to Theorem 13": 20,
    "integer refutations replayed": 1,
}


def main():
    global MEMORY
    parser = argparse.ArgumentParser()
    parser.add_argument("--driver", required=True)
    parser.add_argument("--programs", type=int, default=30)
    parser.add_argument("--fills", type=int, default=6)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--jobs", type=int, default=2)
    parser.add_argument("--no-minimum", action="store_true")
    parser.add_argument("--memory-limit-gb", type=int, default=MEMORY >> 30)
    args = parser.parse_args()
    MEMORY = args.memory_limit_gb << 30
    rng = random.Random(args.seed)
    specs = [gen_spec(rng, i) for i in range(args.programs)]
    seeds = [rng.randrange(1 << 30) for _ in specs]
    counts, failures = Counter(), []

    def one(item):
        spec, seed = item
        local = Counter()
        try:
            check(spec, seed, args.driver, args.fills, local)
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
        for key, least in MINIMUM.items():
            if counts[key] < least:
                failures.append(f"only {counts[key]} of '{key}', fewer than {least}: the check is vacuous")
    for failure in failures[:10]:
        print("FAIL:", failure, file=sys.stderr)
    print(f"{len(specs)} programs, {len(failures)} failures")
    sys.exit(1 if failures else 0)


if __name__ == "__main__":
    main()
