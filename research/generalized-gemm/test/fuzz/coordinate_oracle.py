#!/usr/bin/env python3
"""G3a oracle over programs. A spec is interpreted here, by plain Python loops,
and printed as MLIR for the driver. The event sequence must agree exactly, in
order, at every point of the scope; the status and every fact are recomputed.

The catalogue states what each named case must be, by hand. The random part draws
hidden coordinates, and states what recovery must find when the class applies.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
from pathlib import Path
import random
import subprocess
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).parent))
from coordinate_facts import compare, components, facts, kernel

WIDTH = 16
MASK = (1 << WIDTH) - 1
TIMEOUT = 150

# Expressions: an int, a name, or (op, a, b) with op in + * - sel<, or a remainder.


def evaluate(e, env):
    if isinstance(e, int):
        return e
    if isinstance(e, str):
        return env[e]
    op = e[0]
    a = evaluate(e[1], env)
    b = evaluate(e[2], env)
    if op == "+":
        return (a + b) & MASK
    if op == "-":
        return (a - b) & MASK
    if op == "*":
        return (a * b) & MASK
    if op == "%":
        return a % b
    if op == "sel<":  # (sel<, x, y, then, else)
        return evaluate(e[3] if a < b else e[4], env)
    raise ValueError(op)


class Emitter:
    def __init__(self, lines, names):
        self.lines, self.names, self.count = lines, names, 0

    def fresh(self):
        self.count += 1
        return f"%t{self.count}"

    def value(self, e, indent):
        if isinstance(e, int):
            name = self.fresh()
            self.lines.append(f"{indent}{name} = arith.constant {e} : index")
            return name
        if isinstance(e, str):
            return self.names[e]
        op = e[0]
        a = self.value(e[1], indent)
        b = self.value(e[2], indent)
        name = self.fresh()
        if op == "sel<":
            then, other = self.value(e[3], indent), self.value(e[4], indent)
            test = self.fresh()
            self.lines.append(f"{indent}{test} = arith.cmpi ult, {a}, {b} : index")
            self.lines.append(f"{indent}{name} = arith.select {test}, {then}, {other} : index")
            return name
        mnemonic = {"+": "addi", "-": "subi", "*": "muli", "%": "remui"}[op]
        self.lines.append(f"{indent}{name} = arith.{mnemonic} {a}, {b} : index")
        return name


def spec(name, loops, h, f, g, **extra):
    """`loops`: (name, low, high). `guard`: (a, b) keeps events with a < b. `branch`:
    (a, b, f2) uses f2 for the second operand view when not a < b, as a second site."""
    return dict(name=name, loops=loops, h=h, f=f, g=g, **extra)


def interpret(s, n):
    events = []

    def go(depth, env):
        if depth == len(s["loops"]):
            guard = s.get("guard")
            if guard and not evaluate(guard[0], env) < evaluate(guard[1], env):
                return
            view = s["f"]
            site = 0
            branch = s.get("branch")
            if branch and not evaluate(branch[0], env) < evaluate(branch[1], env):
                view = branch[2]
                site = 1
            h = [evaluate(e, env) for e in s["h"]]
            read = [evaluate(e, env) for e in s.get("h_read", s["h"])]
            events.append(dict(site=site, h=h, h_read=read, f=[evaluate(e, env) for e in view],
                               g=[evaluate(e, env) for e in s["g"]], env=dict(env)))
            return
        name, low, high = s["loops"][depth]
        for v in range(evaluate(low, env), evaluate(high, env)):
            env[name] = v
            go(depth + 1, env)
        env.pop(name, None)

    go(0, {"n": n})
    return events


def extents(events, view):
    if not events:
        return []
    return [max(values) + 1 for values in zip(*[e[view] for e in events])]


def print_function(s, scope):
    every = [e for n in scope for e in interpret(s, n)]
    rank = {v: len(s[v]) for v in ("f", "g", "h")}

    def shape(views, count):
        sizes = [1] * count
        for view in views:
            for position, size in enumerate(extents(every, view)):
                sizes[position] = max(sizes[position], size)
        return sizes
    A = shape(["f"], rank["f"])
    branch = s.get("branch")
    B = shape(["g"], rank["g"])
    C = shape(["h", "h_read"], rank["h"])
    if branch:
        branched = [dict(f2=[evaluate(x, e["env"]) for x in branch[2]]) for e in every]
        for position, size in enumerate(extents(branched, "f2") if branched else []):
            A[position] = max(A[position], size)

    def memref(sizes):
        return "memref<" + "x".join(str(v) for v in sizes) + "xi32>"
    tA, tB, tC = memref(A), memref(B), memref(C)
    lines = [f"func.func @{s['name']}(%A: {tA}, %B: {tB}, %C: {tC}, %n: index) {{"]
    names = {"n": "%n"}
    emit = Emitter(lines, names)
    one = "%one"
    lines.append(f"  {one} = arith.constant 1 : index")
    indent = "  "

    def access(root, view, type_):
        items = ", ".join(emit.value(e, indent) for e in view)
        value = emit.fresh()
        lines.append(f"{indent}{value} = memref.load {root}[{items}] : {type_}")
        return value

    closing = []
    for name, low, high in s["loops"]:
        lo, hi = emit.value(low, indent), emit.value(high, indent)
        names[name] = "%" + name
        lines.append(f"{indent}scf.for %{name} = {lo} to {hi} step {one} {{")
        closing.append(indent + "}")
        indent += "  "

    def mac(view):
        x = access("%A", view, tA)
        y = access("%B", s["g"], tB)
        z = access("%C", s.get("h_read", s["h"]), tC)
        m, u = emit.fresh(), emit.fresh()
        lines.append(f"{indent}{m} = arith.muli {x}, {y} : i32")
        lines.append(f"{indent}{u} = arith.addi {z}, {m} : i32")
        items = ", ".join(emit.value(e, indent) for e in s["h"])
        lines.append(f"{indent}memref.store {u}, %C[{items}] : {tC}")

    guard = s.get("guard")
    if guard:
        a, b = emit.value(guard[0], indent), emit.value(guard[1], indent)
        test = emit.fresh()
        lines.append(f"{indent}{test} = arith.cmpi ult, {a}, {b} : index")
        lines.append(f"{indent}scf.if {test} {{")
        closing.append(indent + "}")
        indent += "  "
    if branch:
        a, b = emit.value(branch[0], indent), emit.value(branch[1], indent)
        test = emit.fresh()
        lines.append(f"{indent}{test} = arith.cmpi ult, {a}, {b} : index")
        lines.append(f"{indent}scf.if {test} {{")
        saved = indent
        indent += "  "
        mac(s["f"])
        indent = saved
        lines.append(f"{indent}}} else {{")
        indent += "  "
        mac(branch[2])
        indent = saved
        lines.append(f"{indent}}}")
    else:
        mac(s["f"])
    for line in reversed(closing):
        lines.append(line)
    lines.append("  return")
    lines.append("}")
    return "\n".join(lines) + "\n"


def expectation(events):
    """What the status must be, from the definitions, with no library code."""
    if not events:
        return "UNKNOWN", None
    if any(e["h"] != e["h_read"] for e in events):
        return "CANDIDATE_REFUTED", None
    triples = list(dict.fromkeys((tuple(e["h"]), tuple(e["f"]), tuple(e["g"])) for e in events))
    h, f, g = [kernel([t[v] for t in triples]) for v in range(3)]
    I, J, K = components([h, f]), components([h, g]), components([f, g])
    P = components([h, f, g])
    fine = (kernel(list(zip(I, J))) == h and kernel(list(zip(I, K))) == f
            and kernel(list(zip(K, J))) == g)
    index = {t: n for n, t in enumerate(triples)}
    lift = [index[(tuple(e["h"]), tuple(e["f"]), tuple(e["g"]))] for e in events]
    labels = {name: [part[t] for t in lift] for name, part in zip("pijk", (P, I, J, K))}
    return ("CANDIDATE_FOUND" if fine else "UNKNOWN"), labels


def run_one(driver, s, scope, fill=None):
    """One process per program, so a slow proof holds up nothing else. Timing out
    is no result of any kind, and the caller counts it as such."""
    with tempfile.NamedTemporaryFile("w", suffix=".mlir") as handle:
        handle.write(print_function(s, range(scope[0], scope[1] + 1)))
        handle.flush()
        command = [driver, handle.name, "--lower-structured", f"--index-bit-width={WIDTH}",
                   f"--scope=0:{scope[0]}:{scope[1]}", "--print-events"]
        if fill is not None:
            command.append(f"--reference-fill={fill}")
        try:
            done = subprocess.run(command, capture_output=True, text=True, timeout=TIMEOUT)
        except subprocess.TimeoutExpired:
            return None
    if done.returncode:
        raise SystemExit(f"driver failed on {s['name']}:\n{done.stderr[-1200:]}")
    return json.loads(done.stdout)[0]


def check_result(s, result, want, tally):
    if result["function"] != s["name"]:
        raise SystemExit("functions out of order")
    if result["authorizes_lowering"] is not False:
        raise SystemExit("a result authorized lowering")
    tally["programs"] += 1
    if result["gate"] != "OPEN":
        # A limit of G2a. No candidate may appear, and the catalogue must open.
        tally["gate closed"] += 1
        if result["points"] or want is not None:
            raise SystemExit(f"{s['name']}: gate {result['gate']}: {result['reason']}")
        return
    for point in result["points"]:
        n = point["parameters"][-1]
        events = interpret(s, n)
        group_results = point["groups"]
        if len(group_results) != 1:
            raise SystemExit(f"{s['name']}: {len(group_results)} groups")
        group = group_results[0]
        status, labels = expectation(events)
        if group["status"] != status:
            raise SystemExit(f"{s['name']} n={n}: {group['status']} != {status}: {group['reason']}")
        if group["equivalence"] != "UNKNOWN":
            raise SystemExit("a candidate claimed equivalence")
        tally[status] += 1
        tally["points"] += 1
        tally["events"] += len(events)
        if not events:
            continue
        if point["run"] != "RETURNED" and "events" not in group:
            raise SystemExit(f"{s['name']} n={n}: the run did not return: {point['reason']}")
        got = group["events"]
        if len(got) != len(events):
            raise SystemExit(f"{s['name']} n={n}: {len(got)} events != {len(events)}")
        last = -1
        for mine, theirs in zip(events, got):
            for view in ("h", "h_read", "f", "g"):
                if mine[view] != theirs[view]:
                    raise SystemExit(f"{s['name']} n={n}: {view} {mine[view]} != {theirs[view]}")
            if theirs["step"] <= last:
                raise SystemExit(f"{s['name']}: event order")
            last = theirs["step"]
        if s.get("branch"):
            # The first event of each interpreted site maps to one driver site.
            sites = {}
            for mine, theirs in zip(events, got):
                if sites.setdefault(mine["site"], theirs["site"]) != theirs["site"]:
                    raise SystemExit(f"{s['name']}: sites merged")
            if len(set(sites.values())) != len(sites):
                raise SystemExit(f"{s['name']}: sites merged")
        if status == "CANDIDATE_FOUND":
            candidate = group["candidate"]
            for axis in "pijk":
                if kernel(candidate[axis]) != kernel(labels[axis]):
                    raise SystemExit(f"{s['name']} n={n}: {axis} differs from the search")
            report = group["report"]
        elif status == "CANDIDATE_REFUTED":
            candidate, report = group["candidate"], group["report"]
        else:
            if "hints" not in group or "candidate" in group:
                raise SystemExit(f"{s['name']} n={n}: Unknown needs hints and no candidate")
            candidate = None
        if candidate:
            bad = compare(report, facts(got, candidate))
            if bad:
                raise SystemExit(f"{s['name']} n={n}: {bad}")
            tally["genuine"] += report["genuine"]
        expected = want.get(n) if want else None
        if expected:
            actual = dict(status=group["status"])
            if candidate:
                actual.update(genuine=report["genuine"], shape=report["shape"]["class"],
                              cartesian=report["cartesian"]["form"],
                              multiplicity=report["multiplicity"]["form"],
                              eligible=report["eligibility"]["form"] == "for_later_verification",
                              axes=[(b["i"], b["j"], b["k"]) for b in report["shape"]["batches"]])
            for key, value in expected.items():
                if actual.get(key) != value:
                    raise SystemExit(f"{s['name']} n={n}: {key} {actual.get(key)} != {value}")
            tally["hand expectations"] += 1


def add(a, b): return ("+", a, b)
def mul(a, b): return ("*", a, b)


I3, J4, K2 = ("i", 0, 3), ("j", 0, 4), ("k", 0, 2)
STD = dict(h=["i", "j"], f=["i", "k"], g=["k", "j"])
FOUND = "CANDIDATE_FOUND"


def expect(status=FOUND, **more):
    return dict(status=status, **more)


GENUINE = dict(genuine=True, shape="gemm", cartesian="box", multiplicity="unit", eligible=True)
CATALOGUE = [
    (spec("standard", [I3, J4, K2], **STD), {0: expect(genuine=True, axes=[(3, 4, 2)], **{k: v for k, v in GENUINE.items() if k != "genuine"})}),
    (spec("standard_ikj", [I3, K2, J4], **STD), {0: expect(axes=[(3, 4, 2)], **GENUINE)}),
    (spec("parametric", [("i", 0, "n"), ("j", 0, 3), ("k", 0, 2)], **STD),
     {0: dict(status="UNKNOWN"), 1: expect(genuine=False, shape="gemv", eligible=True), 2: expect(axes=[(2, 3, 2)], **GENUINE)}),
    (spec("batched", [("p", 0, 2), I3, J4, K2], h=["p", "i", "j"], f=["p", "i", "k"], g=["p", "k", "j"]),
     {0: expect(axes=[(3, 4, 2), (3, 4, 2)], **GENUINE)}),
    (spec("batched_sizes", [("p", 0, 2), ("i", 0, add("p", 2)), ("j", 0, 3), ("k", 0, 2)],
          h=["p", "i", "j"], f=["p", "i", "k"], g=["p", "k", "j"]),
     {0: expect(axes=[(2, 3, 2), (3, 3, 2)], **GENUINE)}),
    (spec("transposed", [I3, J4, K2], h=["j", "i"], f=["k", "i"], g=["j", "k"]), {0: expect(axes=[(3, 4, 2)], **GENUINE)}),
    (spec("affine_permuted", [I3, J4, K2], h=[add("j", 2), add("i", 1)], f=[add(mul("k", 3), 1), "i"], g=[add("k", 2), mul("j", 2)]),
     {0: expect(**GENUINE)}),
    (spec("multidimensional", [("a", 0, 2), ("b", 0, 3), ("j", 0, 2), ("c", 0, 2), ("d", 0, 2)],
          h=["a", "b", "j"], f=["a", "b", "c", "d"], g=["c", "d", "j"]),
     {0: expect(axes=[(6, 2, 4)], **GENUINE)}),
    (spec("linearized", [I3, J4, K2], h=[add(mul("i", 4), "j")], f=[add(mul("i", 2), "k")], g=[add(mul("k", 4), "j")]),
     {0: expect(axes=[(3, 4, 2)], **GENUINE)}),
    (spec("nonlinear", [I3, J4, K2], h=[mul("i", "i"), mul(add("j", 1), add("j", 1))], f=[mul(mul("i", "i"), "i"), "k"], g=["k", mul("j", "j")]),
     {0: expect(**GENUINE)}),
    (spec("select_piecewise", [I3, J4, K2], h=["i", "j"], f=[("sel<", "i", 2, "i", add("k", 5)), ("sel<", "i", 2, "k", "i")], g=["k", "j"]),
     {0: expect(**GENUINE)}),
    (spec("branch_piecewise", [I3, J4, K2], h=["i", "j"], f=["i", "k"], g=["k", "j"], branch=("i", 2, [add("k", 5), "i"])),
     {0: expect(**GENUINE)}),
    (spec("colliding_piecewise", [("i", 0, 2), ("j", 0, 2), K2], h=["i", "j"], f=["i", "k"], g=["k", "j"], branch=("i", 1, ["k", "i"])),
     {0: dict(status="UNKNOWN")}),
    (spec("a_without_i", [I3, J4, K2], h=["i", "j"], f=["k"], g=["k", "j"]), {0: dict(status="UNKNOWN")}),
    (spec("b_without_j", [I3, J4, K2], h=["i", "j"], f=["i", "k"], g=["k"]), {0: dict(status="UNKNOWN")}),
    (spec("shared_both", [("p", 0, 2), I3, J4, K2], h=["p", "i", "j"], f=["i", "k"], g=["k", "j"]), {0: dict(status="UNKNOWN")}),
    (spec("shared_a", [("p", 0, 2), I3, J4, K2], h=["p", "i", "j"], f=["i", "k"], g=["p", "k", "j"]), {0: expect(axes=[(3, 8, 2)], **GENUINE)}),
    (spec("convolution", [("i", 0, 4), ("j", 0, 4)], h=[add("i", "j")], f=["i"], g=["j"]), {0: dict(status="UNKNOWN")}),
    (spec("triangular", [("i", 0, 4), ("j", 0, add("i", 1)), K2], **STD),
     {0: expect(genuine=False, cartesian="partial", multiplicity="unit", eligible=True, axes=[(4, 4, 2)])}),
    (spec("triangular_k", [("i", 0, 4), ("j", 0, 3), ("k", 0, add("i", 1))], **STD),
     {0: expect(genuine=False, cartesian="partial", axes=[(4, 3, 4)])}),
    (spec("missing_point", [I3, J4, K2], guard=(add(add(mul("i", 16), mul("j", 4)), "k"), 22), **STD),
     {0: expect(genuine=False, cartesian="partial", eligible=True)}),
    (spec("duplicate_points", [("r", 0, 2), I3, J4, K2], **STD),
     {0: expect(genuine=False, cartesian="box", multiplicity="uniform", eligible=True, axes=[(3, 4, 2)])}),
    (spec("multiplicity_differs", [I3, J4, K2, ("r", 0, add("i", 1))], **STD),
     {0: expect(genuine=False, cartesian="box", multiplicity="nonuniform", eligible=True)}),
    (spec("hidden_k", [I3, J4, K2], h=["i", "j"], f=["i"], g=["j"]),
     {0: expect(genuine=False, shape="outer", multiplicity="uniform", eligible=True, axes=[(3, 4, 1)])}),
    (spec("outer", [I3, J4], h=["i", "j"], f=["i"], g=["j"]), {0: expect(genuine=False, shape="outer", multiplicity="unit", cartesian="box", eligible=True)}),
    (spec("gemv", [I3, K2], h=["i"], f=["i", "k"], g=["k"]), {0: expect(genuine=False, shape="gemv", cartesian="box", eligible=True)}),
    (spec("dot", [("k", 0, 5)], h=[0], f=["k"], g=["k"]), {0: expect(genuine=False, shape="dot", cartesian="box", eligible=True, axes=[(1, 1, 5)])}),
    (spec("hadamard", [("i", 0, 5)], h=["i"], f=["i"], g=["i"]), {0: expect(genuine=False, shape="element_wise", cartesian="box", eligible=True)}),
    (spec("single", [("i", 0, 1)], h=["i"], f=["i"], g=["i"]), {0: expect(genuine=False, shape="scalar", eligible=True)}),
    (spec("band", [("i", 0, 3), ("j", 0, 3), ("k", 0, 3)], h=["i", "j"], f=["i", add("k", "j")], g=["k", "j"]),
     {0: expect(genuine=False, cartesian="partial", eligible=True, axes=[(3, 3, 5)])}),
    (spec("wrong_read", [I3, J4], h=["i", "j"], h_read=["i", add("j", 1)], f=["i"], g=["j"]),
     {0: dict(status="CANDIDATE_REFUTED")}),
]


def encode(rng, names):
    """Index expressions that are injective in `names` on the loop ranges."""
    if not names:
        return []
    style = rng.randrange(4)
    if style == 0 or len(names) == 1 and style == 1:
        return [(mul(n, n) if rng.random() < .25 else add(mul(n, rng.randint(1, 3)), rng.randint(0, 2))) for n in rng.sample(names, len(names))]
    if style == 1:
        order = rng.sample(names, len(names))
        total = order[0]
        for n in order[1:]:
            total = add(mul(total, 8), n)
        return [total]
    if style == 2:
        return [("sel<", names[0], 2, n, add(n, 0)) for n in names]
    return list(names)


def random_spec(rng, name):
    axes = {}
    loops = []
    # One axis may have two dimensions: a deeper nest costs G2a, and adds nothing.
    double = rng.choice("ijk") if rng.random() < .5 else None
    for axis in "ijk":
        for d in range(2 if axis == double else 1):
            label = f"{axis}{d}"
            axes.setdefault(axis, []).append(label)
            loops.append((label, 0, rng.randint(1, 3)))
    batch = []
    if rng.random() < .4:
        batch = ["p"]
        loops.append(("p", 0, rng.randint(1, 3)))
    rng.shuffle(loops)
    if rng.random() < .3:  # The bound sits on the parameter.
        position = rng.randrange(len(loops))
        loops[position] = (loops[position][0], 0, "n")
    I, J, K = axes["i"], axes["j"], axes["k"]
    h, f, g = encode(rng, batch + I + J), encode(rng, batch + I + K), encode(rng, batch + K + J)
    extra = {}
    damage = rng.randrange(8)
    if damage == 0:
        f = encode(rng, batch + K)  # A view loses a coordinate.
    elif damage == 1:
        g = encode(rng, batch + K)
    elif damage == 2:
        h = [add(I[0], J[0])] + encode(rng, batch)  # Not injective.
    elif damage == 3:
        names = [q[0] for q in loops]
        a, b = sorted(rng.sample(range(len(names)), 2))
        loops[b] = (names[b], 0, add(names[a], 1))  # Triangular.
    elif damage == 4:
        extra["guard"] = (add(mul(loops[0][0], 7), loops[-1][0]), rng.randint(4, 20))
    elif damage == 5:
        loops.insert(rng.randrange(len(loops) + 1), ("r", 0, rng.randint(2, 3)))
    return spec(name, loops, h, f, g, **extra)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--driver", required=True)
    parser.add_argument("--programs", type=int, default=120)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--jobs", type=int, default=4)
    args = parser.parse_args()

    tally = {key: 0 for key in ("programs", "points", "events", "gate closed", "genuine", "hand expectations",
                                "CANDIDATE_FOUND", "CANDIDATE_REFUTED", "UNKNOWN")}
    tally["timed out"] = 0
    rng = random.Random(args.seed)
    work = [(s, want, (0, 2)) for s, want in CATALOGUE]
    work += [(random_spec(rng, f"r{n}"), None, (1, 3)) for n in range(args.programs)]

    with ThreadPoolExecutor(args.jobs) as pool:
        outputs = list(pool.map(lambda w: run_one(args.driver, w[0], w[2]), work))
    for (s, want, scope), result in zip(work, outputs):
        if result is None:
            tally["timed out"] += 1
            if want is not None:
                raise SystemExit(f"{s['name']}: a named case timed out")
            continue
        check_result(s, result, want, tally)

    # Two fills, one table. The fill is an input the accesses must ignore.
    subset = [w for w, r in zip(work, outputs) if r is not None and r["gate"] == "OPEN"][:40]
    with ThreadPoolExecutor(args.jobs) as pool:
        filled = list(pool.map(lambda w: run_one(args.driver, w[0], w[2], 3), subset))
    tally["fill comparisons"] = 0
    first = {r["function"]: r for r in outputs if r is not None}
    for result in filled:
        if result is None:
            continue
        for p, q in zip(first[result["function"]]["points"], result["points"]):
            for x, y in zip(p["groups"], q["groups"]):
                if x.get("events") != y.get("events"):
                    raise SystemExit(f"{result['function']}: the fill changed the accesses")
                tally["fill comparisons"] += 1
    for key in sorted(tally):
        print(f"{key}: {tally[key]}")
    if (tally["gate closed"] + tally["timed out"]) * 4 > tally["programs"] + tally["timed out"]:
        raise SystemExit("too many gates closed for this run to mean much")
    print("0 disagreements")


if __name__ == "__main__":
    main()
