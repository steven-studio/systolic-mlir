#!/usr/bin/env python3
"""G3b oracle. A spec is interpreted by plain Python loops, printed as MLIR for the
driver, and every claim the driver makes is recomputed with no library code:

- The event table, in order, at every point the driver ran.
- Each term, by a Python evaluator with explicit widths, then every fact of the
  report from its definition, and the definedness of each division.
- ScopeEnumerated: every point of the scope really ran.
- SymbolicExact: at EVERY point of the scope, including those the driver never ran,
  the events are the orbit of the declared order, each once, in that order.
- SymbolicValid: at every point of the scope, the dependences hold and the
  coordinates identify the events.

So a symbolic claim is tested beyond its samples. The catalogue states by hand what
each named case must reach. Tier expectations are lower bounds on evidence, except
where the case exists to show that a tier must not be reached.
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
import coordinate_oracle as co
from coordinate_facts import compare, facts, kernel

TIMEOUT = 280
PREDICATES = {0: lambda a, b, w: a == b, 1: lambda a, b, w: a != b,
              2: lambda a, b, w: signed(a, w) < signed(b, w), 3: lambda a, b, w: signed(a, w) <= signed(b, w),
              4: lambda a, b, w: signed(a, w) > signed(b, w), 5: lambda a, b, w: signed(a, w) >= signed(b, w),
              6: lambda a, b, w: a < b, 7: lambda a, b, w: a <= b, 8: lambda a, b, w: a > b, 9: lambda a, b, w: a >= b}


def signed(value, width):
    return value - (1 << width) if value >> (width - 1) else value


class Undefined(Exception):
    pass


def term_value(t, state, parameters):
    """Exact, at the term's own width. Raises Undefined on a zero divisor."""
    width = t["width"]
    mask = (1 << width) - 1
    if "const" in t:
        return t["const"] & mask
    if "state" in t:
        return state[t["state"]] & mask
    if "param" in t:
        return parameters[t["param"]] & mask
    op = t["op"]
    args = [term_value(a, state, parameters) for a in t["args"]]
    if op == "add":
        return (args[0] + args[1]) & mask
    if op == "sub":
        return (args[0] - args[1]) & mask
    if op == "mul":
        return (args[0] * args[1]) & mask
    if op in ("udiv", "urem"):
        if args[1] == 0:
            raise Undefined()
        return args[0] // args[1] if op == "udiv" else args[0] % args[1]
    if op in ("shl", "lshr"):
        if args[1] >= width:
            raise Undefined()
        return ((args[0] << args[1]) & mask) if op == "shl" else args[0] >> args[1]
    if op == "and":
        return args[0] & args[1]
    if op == "or":
        return args[0] | args[1]
    if op == "xor":
        return args[0] ^ args[1]
    if op == "icmp":
        inner = t["args"][0]["width"]
        return int(PREDICATES[t["predicate"]](args[0], args[1], inner))
    if op == "ite":
        return args[1] if args[0] else args[2]
    raise SystemExit(f"the oracle does not know {op}")


def run_one(driver, s, scope, extra):
    with tempfile.NamedTemporaryFile("w", suffix=".mlir") as handle:
        handle.write(co.print_function(s, range(scope[0], scope[1] + 1)))
        handle.flush()
        command = [driver, handle.name, "--lower-structured", f"--index-bit-width={co.WIDTH}",
                   f"--scope=0:{scope[0]}:{scope[1]}", "--print-events"] + extra
        try:
            done = subprocess.run(command, capture_output=True, text=True, timeout=TIMEOUT)
        except subprocess.TimeoutExpired:
            return None
    if done.returncode:
        raise SystemExit(f"driver failed on {s['name']}:\n{done.stderr[-1200:]}")
    return json.loads(done.stdout)[0]


def state_of(names, event):
    return dict(zip(names, event["x"]))


def labels_of(candidate, names, events, parameters):
    out = {}
    for axis in "pijk":
        tuples = [tuple(term_value(t, state_of(names, e), parameters) for t in candidate["axes"][axis])
                  for e in events]
        out[axis] = list(kernel(tuples))
    return out


def variable_map(s, tables):
    """Which state variable holds which loop variable, from the driver's tables.
    Lets a term be evaluated at points the driver never ran."""
    mapping = {}
    for table in tables:
        if "events" not in table:
            continue
        n = table["parameters"][-1]
        mine = co.interpret(s, n)
        names = table["coordinate_names"]
        for position, name in enumerate(names):
            column = [e["x"][position] for e in table["events"]]
            matches = {v for v in {loop[0] for loop in s["loops"]}
                       if [m["env"][v] for m in mine] == column}
            previous = mapping.get(name)
            mapping[name] = matches if previous is None else previous & matches
    return mapping


def replay_at(s, candidate, n, mapping):
    """Coordinates, level values and views at a point, by interpretation alone."""
    rows = []
    for event in co.interpret(s, n):
        state = {}
        for name, names in mapping.items():
            if names:
                state[name] = event["env"][sorted(names)[0]]
        rows.append((event, state))
    return rows


def check_symbolic(s, candidate, scope, mapping, tally):
    tier = candidate["symbolic"]["tier"]
    if tier == "PROPOSED":
        return
    for n in range(scope[0], scope[1] + 1):
        rows = replay_at(s, candidate, n, mapping)
        if not rows:
            continue
        parameters = [n]
        try:
            coords = [tuple(tuple(term_value(t, state, parameters) for t in candidate["axes"][axis])
                            for axis in "pijk") for _, state in rows]
        except KeyError:
            continue  # A term over state with no loop variable behind it.
        for key, view in ((lambda c: (c[0], c[1], c[2]), "h"), (lambda c: (c[0], c[1], c[3]), "f"),
                          (lambda c: (c[0], c[3], c[2]), "g")):
            seen = {}
            for coordinate, (event, _) in zip(coords, rows):
                value = tuple(event[view])
                if seen.setdefault(key(coordinate), value) != value:
                    raise SystemExit(f"{s['name']} n={n}: {tier} claims a dependence for {view}, and it fails")
        if len(set(coords)) != len(coords):
            raise SystemExit(f"{s['name']} n={n}: {tier}, and two events share coordinates")
        if any(event["h"] != event["h_read"] for event, _ in rows):
            raise SystemExit(f"{s['name']} n={n}: {tier}, and an event reads another cell")
        tally["symbolic points replayed"] += 1
        if tier == "SYMBOLIC_EXACT":
            check_orbit(s, candidate, rows, n)


def check_orbit(s, candidate, rows, n):
    """The events are first, succ(first), ... exactly, by the declared order."""
    levels = candidate["order"]
    parameters = [n]

    def upper(m, values):
        level = levels[m]
        top = term_value(level["upper"], {}, parameters)
        if level["upper_outer"] >= 0:
            top += values[level["upper_outer"]]
        return top & ((1 << level["value"]["width"]) - 1)

    def inside(values):
        return all(term_value(levels[m]["lower"], {}, parameters) <= values[m] < upper(m, values)
                   for m in range(len(levels)))

    def successor(values):
        for m in reversed(range(len(levels))):
            if values[m] + 1 < upper(m, values):
                return values[:m] + [values[m] + 1] + [
                    term_value(levels[r]["lower"], {}, parameters) for r in range(m + 1, len(levels))]
        return None

    current = [term_value(level["lower"], {}, parameters) for level in levels]
    if not inside(current):
        raise SystemExit(f"{s['name']} n={n}: events exist, and the first element is outside")
    for position, (_, state) in enumerate(rows):
        actual = [term_value(level["value"], state, parameters) for level in levels]
        if actual != current:
            raise SystemExit(f"{s['name']} n={n}: event {position} is {actual}, the order says {current}")
        following = successor(current)
        if position + 1 == len(rows):
            if following is not None:
                raise SystemExit(f"{s['name']} n={n}: the order continues past the last event")
        else:
            if following is None or not inside(following):
                raise SystemExit(f"{s['name']} n={n}: the order ends before the events do")
            current = following


def check_result(s, result, want, scope, tally):
    for group in result["groups"]:
        for candidate in group["candidates"]:
            expected = "every run" if result["termination"] == "PROVEN_TERMINATING" else "runs that complete"
            if candidate["symbolic"]["tier"] in ("SYMBOLIC_VALID", "SYMBOLIC_EXACT") and \
                    candidate["symbolic"]["holds_for"] != expected:
                raise SystemExit(f"{s['name']}: the tier misstates what it covers")
    tally["programs"] += 1
    if result["authorizes_lowering"] is not False:
        raise SystemExit("a result authorized lowering")
    if result["gate"] != "OPEN":
        tally["gate closed"] += 1
        if result["groups"] or want is not None:
            raise SystemExit(f"{s['name']}: gate {result['gate']}: {result['reason']}")
        return
    group = result["groups"][0]
    tables = group["tables"]
    ran = 0
    for table in tables:
        n = table["parameters"][-1]
        mine = co.interpret(s, n)
        if not mine:
            continue
        got = table.get("events")
        if got is None:
            raise SystemExit(f"{s['name']} n={n}: no table: {table['reason']}")
        ran += 1
        if len(got) != len(mine):
            raise SystemExit(f"{s['name']} n={n}: {len(got)} events, the program has {len(mine)}")
        for a, b in zip(mine, got):
            for view in ("h", "h_read", "f", "g"):
                if a[view] != b[view]:
                    raise SystemExit(f"{s['name']} n={n}: {view}: {a[view]} != {b[view]}")
        tally["events"] += len(got)
    mapping = variable_map(s, tables)

    by_point = {tuple(t["parameters"]): t for t in tables}
    structures = set()
    for candidate in group["candidates"]:
        tally["candidates"] += 1
        tally[candidate["finite"]["tier"]] += 1
        tally[candidate["symbolic"]["tier"]] += 1
        if candidate["finite"]["status"] != "VERIFIED":
            raise SystemExit(f"{s['name']}: an unverified candidate was kept")
        for family in ("finite", "symbolic"):
            if candidate[family]["equivalence"] != "UNKNOWN":
                raise SystemExit("a candidate claimed equivalence")
        if candidate["finite"]["tier"] == "SCOPE_ENUMERATED" and result["sampled"]:
            raise SystemExit(f"{s['name']}: a sample was called the scope")
        full = bool(candidate["points"])
        structure = []
        for point in candidate["points"]:
            table = by_point[tuple(point["parameters"])]
            names = table["coordinate_names"]
            labels = labels_of(candidate, names, table["events"], point["parameters"])
            structure.append(tuple(kernel(labels[a]) for a in "pijk"))
            report = point["report"]
            bad = compare(report, facts(table["events"], labels))
            if bad:
                raise SystemExit(f"{s['name']} n={point['parameters']}: {bad}")
            expected = (report["validity"]["decomposition_valid"] and report["cartesian"]["form"] == "box"
                        and report["multiplicity"]["form"] == "unit" and report["strict_non_degenerate"])
            if point["structurally_full"] != expected:
                raise SystemExit(f"{s['name']}: structurally_full is wrong")
            full = full and expected
            tally["facts compared"] += 1
        if candidate["structurally_full"] != full:
            raise SystemExit(f"{s['name']}: the summary of structurally_full is wrong")
        structure = tuple(structure)
        if structure in structures:
            raise SystemExit(f"{s['name']}: two alternatives have one structure")
        structures.add(structure)
        check_symbolic(s, candidate, scope, mapping, tally)
    if want:
        check_want(s, group, want, tally)


def check_want(s, group, want, tally):
    if want.get("status", "CANDIDATE_FOUND") and group["status"] != want.get("status", "CANDIDATE_FOUND"):
        raise SystemExit(f"{s['name']}: {group['status']}: {group['reason']}")
    names = want.get("axes")
    if names is None:
        tally["hand expectations"] += 1
        return
    # Some candidate has exactly these axes, as variables.
    table = next(t for t in reversed(group["tables"]) if "events" in t)
    n = table["parameters"][-1]
    mine = co.interpret(s, n)
    wanted = {axis: kernel([tuple(co.evaluate(v, e["env"]) for v in names.get(axis, [])) for e in mine])
              for axis in "pijk"}
    found = None
    for candidate in group["candidates"]:
        labels = labels_of(candidate, table["coordinate_names"], table["events"], [n])
        if all(kernel(labels[a]) == wanted[a] for a in "pijk"):
            found = candidate
    if not found:
        raise SystemExit(f"{s['name']}: no candidate has the axes {names}")
    rank = ["PROPOSED", "POINT", "SCOPE_ENUMERATED", "SYMBOLIC_VALID", "SYMBOLIC_EXACT"]
    if rank.index(found["symbolic"]["tier"]) < rank.index(want.get("symbolic", "PROPOSED")):
        raise SystemExit(f"{s['name']}: {found['symbolic']['tier']}: {found['symbolic']['reason']}")
    if "exact_forbidden" in want and found["symbolic"]["tier"] == "SYMBOLIC_EXACT":
        raise SystemExit(f"{s['name']}: exact, which this case must not reach")
    if "full" in want:
        report = [p for p in found["points"] if p["parameters"] == [n]][0]
        if report["structurally_full"] != want["full"]:
            raise SystemExit(f"{s['name']}: structurally_full is {report['structurally_full']}")
        if "genuine" in want and report["report"]["genuine"] != want["genuine"]:
            raise SystemExit(f"{s['name']}: genuine is {report['report']['genuine']}")
    tally["hand expectations"] += 1


def add(a, b): return ("+", a, b)
def mul(a, b): return ("*", a, b)
def rem(a, b): return ("%", a, b)


I3, J4, K2 = ("i", 0, 3), ("j", 0, 4), ("k", 0, 2)
N = ("i", 0, "n")
STD = dict(h=["i", "j"], f=["i", "k"], g=["k", "j"])
EXACT = "SYMBOLIC_EXACT"
AXES = dict(i=["i"], j=["j"], k=["k"])
spec = co.spec
CATALOGUE = [
    (spec("standard", [I3, J4, K2], **STD), dict(axes=AXES, symbolic=EXACT, full=True, genuine=True), 0),
    (spec("symbolic_n", [N, J4, K2], **STD), dict(axes=AXES, symbolic=EXACT, full=True, genuine=True), 0),
    (spec("symbolic_all", [N, ("j", 0, "n"), ("k", 0, "n")], **STD), dict(axes=AXES, symbolic=EXACT, full=True), 0),
    (spec("batched", [("p", 0, 2), I3, J4, K2], h=["p", "i", "j"], f=["p", "i", "k"], g=["p", "k", "j"]),
     dict(axes=dict(p=["p"], **AXES), symbolic=EXACT, full=True, genuine=True), 0),
    (spec("batched_sizes", [("p", 0, 2), ("i", 0, add("p", 2)), ("j", 0, 3), ("k", 0, 2)],
          h=["p", "i", "j"], f=["p", "i", "k"], g=["p", "k", "j"]),
     dict(axes=dict(p=["p"], **AXES), symbolic=EXACT, full=True), 0),
    (spec("transposed", [I3, J4, K2], h=["j", "i"], f=["k", "i"], g=["j", "k"]),
     dict(axes=AXES, symbolic=EXACT, full=True, genuine=True), 0),
    (spec("multidimensional", [("a", 0, 2), ("b", 0, 3), ("j", 0, 2), ("c", 0, 2), ("d", 0, 2)],
          h=["a", "b", "j"], f=["a", "b", "c", "d"], g=["c", "d", "j"]),
     dict(axes=dict(i=["a", "b"], j=["j"], k=["c", "d"]), symbolic=EXACT, full=True), 0),
    (spec("a_without_i", [N, J4, K2], h=["i", "j"], f=["k"], g=["k", "j"]),
     dict(axes=AXES, symbolic=EXACT, full=True, genuine=False), 0),
    (spec("b_without_j", [N, J4, K2], h=["i", "j"], f=["i", "k"], g=["k"]),
     dict(axes=AXES, symbolic=EXACT, full=True, genuine=False), 0),
    (spec("shared_both", [("p", 0, 2), I3, J4, K2], h=["p", "i", "j"], f=["i", "k"], g=["k", "j"]),
     dict(axes=dict(p=["p"], **AXES), symbolic=EXACT, full=True, genuine=False), 0),
    (spec("hidden_k", [N, J4, K2], h=["i", "j"], f=["i"], g=["j"]),
     dict(axes=AXES, symbolic=EXACT, full=True, genuine=False), 0),
    (spec("duplicates", [("r", 0, 2), I3, J4, K2], **STD), dict(), 0),
    (spec("multiplicity_differs", [I3, J4, K2, ("r", 0, add("i", 1))], **STD), dict(), 0),
    (spec("nonlinear_views", [N, J4, K2], h=[mul("i", "i"), "j"], f=[mul("i", "i"), "k"], g=["k", mul("j", "j")]),
     dict(axes=AXES, symbolic=EXACT, full=True), 0),
    (spec("polynomial_output", [("x", 0, 9), K2], h=[mul("x", "x")], f=[("/", "x", 3), "k"], g=["k", rem("x", 3)]),
     dict(axes=dict(i=[("/", "x", 3)], j=[rem("x", 3)], k=["k"]), full=True), 2),
    (spec("piecewise", [N, J4, K2], h=["i", "j"], f=[("sel<", "i", 2, "i", add("k", 5)), ("sel<", "i", 2, "k", "i")], g=["k", "j"]),
     dict(axes=AXES, symbolic=EXACT, full=True), 0),
    (spec("branching", [N, J4, K2], h=["i", "j"], f=["i", "k"], g=["k", "j"], branch=("i", 2, [add("k", 5), "i"])),
     dict(axes=AXES, symbolic=EXACT, full=True), 0),
    (spec("modular", [I3, J4, ("k", 0, 4)], h=["i", "j"], f=["i", "k"], g=[rem(add("i", "k"), 4), "j"]),
     dict(axes=dict(i=["i"], j=["j"], k=[rem(add("i", "k"), 4)]), full=True), 2),
    (spec("linearized", [("x", 0, 12), K2], h=["x"], f=[("/", "x", 4), "k"], g=["k", rem("x", 4)]),
     dict(axes=dict(i=[("/", "x", 4)], j=[rem("x", 4)], k=["k"]), symbolic=EXACT, full=True), 2),
    (spec("triangular", [("i", 0, "n"), ("j", 0, add("i", 1)), K2], **STD), dict(axes=AXES, symbolic=EXACT, full=False), 0),
    (spec("hole", [I3, J4, K2], guard=(add(add(mul("i", 16), mul("j", 4)), "k"), 22), **STD),
     dict(axes=AXES, full=False, exact_forbidden=True), 0),
    (spec("step_two", [("t", 0, "n")], h=[mul("t", 2)], f=[mul("t", 2)], g=[mul("t", 2)]), dict(), 0),
    (spec("late_read", [N, J4, K2], h=["i", "j"], h_read=[("sel<", "i", 4, "i", 0), "j"], f=["i", "k"], g=["k", "j"]), dict(status=None), 0),
    (spec("gemv", [N, K2], h=["i"], f=["i", "k"], g=["k"]), dict(axes=dict(i=["i"], k=["k"]), symbolic=EXACT, full=False), 0),
    (spec("dot", [("k", 0, "n")], h=[0], f=["k"], g=["k"]), dict(axes=dict(k=["k"]), symbolic=EXACT, full=False), 0),
    (spec("outer", [N, J4], h=["i", "j"], f=["i"], g=["j"]), dict(axes=dict(i=["i"], j=["j"]), symbolic=EXACT, full=False), 0),
    (spec("hadamard", [("i", 0, "n")], h=["i"], f=["i"], g=["i"]), dict(axes=dict(p=["i"]), symbolic=EXACT, full=False), 0),
    (spec("wrong_read", [I3, J4], h=["i", "j"], h_read=["i", add("j", 1)], f=["i"], g=["j"]), dict(status="CANDIDATE_REFUTED"), 0),
]


def encode(rng, names):
    names = list(names)
    if not names:
        return []
    rng.shuffle(names)
    style = rng.randrange(3)
    if style == 0:
        return [add(mul(n, rng.randint(1, 3)), rng.randint(0, 2)) for n in names]
    if style == 1:
        total = names[0]
        for n in names[1:]:
            total = add(mul(total, 8), n)
        return [total]
    return names


def random_spec(rng, name):
    extents = {v: rng.randint(1, 3) for v in "ijk"}
    loops = [(v, 0, e) for v, e in extents.items()]
    batch = []
    if rng.random() < .35:
        batch = ["p"]
        loops.append(("p", 0, rng.randint(2, 3)))
    rng.shuffle(loops)
    if rng.random() < .5:
        position = rng.randrange(len(loops))
        loops[position] = (loops[position][0], 0, "n")
    h, f, g = batch + ["i", "j"], batch + ["i", "k"], batch + ["k", "j"]
    damage = rng.randrange(7)
    extra = {}
    if damage == 0:
        f = batch + ["k"]
    elif damage == 1:
        g = batch + ["k"]
    elif damage == 2:
        f = ["i", "k"]
        g = ["k", "j"]
    elif damage == 3:
        f, g = batch + ["i"], batch + ["j"]
    elif damage == 4:
        names = [q[0] for q in loops]
        a, b = sorted(rng.sample(range(len(names)), 2))
        loops[b] = (names[b], 0, add(names[a], 1))
    elif damage == 5:
        loops.insert(rng.randrange(len(loops) + 1), ("r", 0, 2))
    return spec(name, loops, encode(rng, h), encode(rng, f), encode(rng, g), **extra)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--driver", required=True)
    parser.add_argument("--programs", type=int, default=60)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--jobs", type=int, default=6)
    parser.add_argument("--no-catalogue", action="store_true")
    args = parser.parse_args()

    tally = {key: 0 for key in (
        "programs", "candidates", "events", "facts compared", "symbolic points replayed", "gate closed",
        "timed out", "hand expectations", "POINT", "SCOPE_ENUMERATED", "SYMBOLIC_VALID", "SYMBOLIC_EXACT",
        "PROPOSED")}
    rng = random.Random(args.seed)
    work = []
    if not args.no_catalogue:
        # The scope is wider than the points run, so symbolic claims go beyond them.
        work += [(s, want, (2, 5), ["--max-level=" + str(level), "--max-points=2"]) for s, want, level in CATALOGUE]
    for n in range(args.programs):
        work.append((random_spec(rng, f"r{n}"), None, (1, 4), ["--max-level=1"]))

    with ThreadPoolExecutor(args.jobs) as pool:
        outputs = list(pool.map(lambda w: run_one(args.driver, w[0], w[2], w[3]), work))
    for (s, want, scope, _), result in zip(work, outputs):
        if result is None:
            tally["timed out"] += 1
            if want:
                raise SystemExit(f"{s['name']}: a named case timed out")
            continue
        check_result(s, result, want, scope, tally)
    for key in sorted(tally):
        print(f"{key}: {tally[key]}")
    if tally["timed out"] * 4 > tally["programs"] + tally["timed out"]:
        raise SystemExit("too many timeouts for this run to mean much")
    print("0 disagreements")


if __name__ == "__main__":
    main()
