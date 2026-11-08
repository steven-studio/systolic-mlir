#!/usr/bin/env python3
"""G5b-M3 oracle: claims over sets of groups (Theorem 16), recomputed from the source
trace with no library code.

A spec is a list of loop nests over six one-dimensional roots, each MAC statement one
site, interpreted here by plain Python and printed as MLIR for the driver. A group is a
triple of roots (output; first, second). The driver is asked for claims over sets of
groups: all of them, all of them listed backwards, each alone, each set that leaves one
out, one group twice, and an index that does not exist. For each claim:

- VERIFIED (finite at its points, symbolic at every point of the scope): the merged
  replay holds on every completing run of many fills. It is computed here from the run's
  own trace: the events of the listed groups in execution order, each operand the
  initial value of its cell, each event's value the cell's current value plus its
  product, each cell's final value the replay's, with a constant written before a cell's
  first event starting its fold. Nothing assumes the groups' cells apart.
- REFUTED: the witness completes here, and the relation fails on it.
- The facts: events per listed group, cells two listed groups write, whether the listed
  order holds, output cells and the longest fold, each at every point the claim lists;
  a proven order or shared-cells fact at every point it covers, a refuted one at its
  point.
- The listing changes no claim: all and all-backwards agree in status, tier and reason.
- A set of one group is that group's own claim.
- Non-vacuity: at a verified point where a cell has contributions from two groups, the
  fold with one contribution dropped, one duplicated, or two of different groups swapped
  is compared with the program's own final value. A difference shows that the check
  would have caught a claim that missed, duplicated or reordered an event. Integer swaps
  commute and are counted apart.

Fills are a test of the checker and never a proof. Every driver run has a time limit,
and a timeout fails this run. The catalogue states by hand what named cases must give,
the numeric example of the M3 brief among them.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
from collections import Counter
import json
import math
import random
import struct
import subprocess
import sys
import tempfile

TIMEOUT = 900
EXTENT = 8
WIDTH = 16
ROOTS = "abcdef"
SOLVER = ["--timeout-ms=3000", "--rlimit=2000000"]
AUTH = ",".join(f"roots {a} and {b} are separate" for a in range(len(ROOTS)) for b in range(a + 1, len(ROOTS)))


class Stop(Exception):
    pass


def bits(value, element):
    if element == "f64":
        return struct.unpack("<Q", struct.pack("<d", value))[0]
    return value & 0xFFFFFFFF


def from_hex(text, element):
    raw = int(text, 16)
    if element == "f64":
        return struct.unpack("<d", struct.pack("<Q", raw))[0]
    return raw


def mul(a, b, element):
    if element == "f64":
        r = a * b
        if math.isnan(r):
            raise Stop("NaN")
        return r
    return (a * b) & 0xFFFFFFFF


def add(a, b, element):
    if element == "f64":
        r = a + b
        if math.isnan(r):
            raise Stop("NaN")
        return r
    return (a + b) & 0xFFFFFFFF


def expression(e, env):
    if isinstance(e, int):
        return e
    if isinstance(e, str):
        return env[e]
    a, b = expression(e[1], env), expression(e[2], env)
    return a + b if e[0] == "+" else a * b


def interpret(spec, n, memory):
    """The ordered trace of one run, and its final memory. Raises Stop when the run does
    not complete. An event: ("event", triple, cell, a cell, b cell, a, b, update)."""
    element = spec["element"]
    mem = {root: list(cells) for root, cells in memory.items()}
    trace = []

    def at(index, env):
        i = expression(index, env)
        if not 0 <= i < EXTENT:
            raise Stop("out of bounds")
        return i

    def body(statements, env):
        for s in statements:
            kind = s[0]
            if kind == "loop":
                _, var, low, high, inner = s
                for v in range(expression(low, env), expression(high, env)):
                    body(inner, {**env, var: v})
            elif kind == "if":
                _, left, right, inner = s
                if expression(left, env) >= expression(right, env):
                    body(inner, env)
            elif kind == "mac":
                _, cr, ci, ar, ai, br, bi = s
                f, g, h = at(ai, env), at(bi, env), at(ci, env)
                x, y, z = mem[ar][f], mem[br][g], mem[cr][h]
                u = add(z, mul(x, y, element), element)
                mem[cr][h] = u
                trace.append(("event", (cr, ar, br), (cr, h), (ar, f), (br, g), x, y, u))
            elif kind == "set":
                _, root, index, value = s
                i = at(index, env)
                mem[root][i] = value
                trace.append(("write", (root, i), True, value))
            elif kind == "copy":
                _, root, index, source, from_index = s
                i, j = at(index, env), at(from_index, env)
                mem[root][i] = mem[source][j]
                trace.append(("write", (root, i), False, mem[root][i]))
            else:
                raise SystemExit(f"unknown statement {s}")

    body(spec["body"], {"n": n})
    return trace, mem


def contributions(spec, triples, memory, trace):
    """Per cell, the listed events' products in order, and the value each fold starts
    from: a constant written before the cell's first listed event, or its initial value."""
    element = spec["element"]
    start, lists = {}, {}
    for t in trace:
        if t[0] == "write" or t[1] not in triples:
            key = t[1] if t[0] == "write" else t[2]
            if key not in lists:
                if t[0] == "write" and t[2]:
                    start[key] = t[3]
                else:
                    start.pop(key, None)
            continue
        _, triple, cell, a, b, *_ = t
        product = mul(memory[a[0]][a[1]], memory[b[0]][b[1]], element)
        lists.setdefault(cell, []).append((triples.index(triple), product))
    return {cell: (start.get(cell, memory[cell[0]][cell[1]]), items) for cell, items in lists.items()}


def fold(spec, first, products):
    value = first
    for p in products:
        value = add(value, p, spec["element"])
    return value


def relation_fails(spec, triples, memory, trace, final):
    """Theorem 16's relation for the listed triples, from its definition. A reason when
    it does not hold."""
    element = spec["element"]
    current, touched = {}, set()
    for t in trace:
        if t[0] == "write" or t[1] not in triples:
            key = t[1] if t[0] == "write" else t[2]
            if key not in touched:
                if t[0] == "write" and t[2]:
                    current[key] = t[3]
                else:
                    current.pop(key, None)
            continue
        _, triple, cell, a, b, x, y, u = t
        a0, b0 = memory[a[0]][a[1]], memory[b[0]][b[1]]
        if bits(x, element) != bits(a0, element) or bits(y, element) != bits(b0, element):
            return "an operand read is not its initial value"
        begin = current.get(cell, memory[cell[0]][cell[1]])
        expected = add(begin, mul(a0, b0, element), element)
        if bits(expected, element) != bits(u, element):
            return "an event's value is not the replay's"
        current[cell] = expected
        touched.add(cell)
    for cell in touched:
        if bits(final[cell[0]][cell[1]], element) != bits(current[cell], element):
            return "a final cell is not the replay's"
    return None


def trace_facts(trace, triples):
    """What the claim's per-point facts must say, from the trace alone."""
    events = [0] * len(triples)
    writers, counts, sequence = {}, {}, []
    for t in trace:
        if t[0] != "event" or t[1] not in triples:
            continue
        g = triples.index(t[1])
        events[g] += 1
        writers.setdefault(t[2], set()).add(g)
        counts[t[2]] = counts.get(t[2], 0) + 1
        sequence.append(g)
    return {"events": events,
            "shared": sum(1 for w in writers.values() if len(w) > 1),
            "ordered": all(a <= b for a, b in zip(sequence, sequence[1:])),
            "output_cells": len(counts),
            "max_contributions": max(counts.values(), default=0),
            "total": sum(events)}


POOL = [0.0, -0.0, 1.0, -1.0, 0.5, 3.0, 1e16, -1e16, 1e-3, 9007199254740992.0, 1e308, -1e308]


def fill(spec, rng):
    memory = {}
    for root in ROOTS:
        if spec["element"] == "i32":
            memory[root] = [rng.choice([rng.getrandbits(32), rng.randrange(8), 0xFFFFFFFF]) for _ in range(EXTENT)]
        else:
            memory[root] = [rng.choice(POOL) if rng.random() < 0.6 else rng.randrange(-64, 64) / 8.0
                            for _ in range(EXTENT)]
    return memory


def structural_trace(spec, n):
    """The trace at a point, with values that never stop a run. None if it stops."""
    one = 1 if spec["element"] == "i32" else 1.0
    try:
        trace, _ = interpret(spec, n, {r: [one] * EXTENT for r in ROOTS})
    except Stop:
        return None
    return trace


def print_function(spec):
    element = spec["element"]
    t = f"memref<{EXTENT}x{element}>"
    lines, counter, consts = [], [0], set(range(EXTENT + 1))

    def value(e):
        if isinstance(e, int):
            consts.add(e)
            return f"%c{e}"
        if isinstance(e, str):
            return "%n" if e == "n" else "%" + e
        a, b = value(e[1]), value(e[2])
        counter[0] += 1
        name = f"%t{counter[0]}"
        lines.append(f"{name} = {'arith.addi' if e[0] == '+' else 'arith.muli'} {a}, {b} : index")
        return name

    def body(statements):
        for s in statements:
            kind = s[0]
            if kind == "loop":
                _, var, low, high, inner = s
                lo, hi = value(low), value(high)
                lines.append(f"scf.for %{var} = {lo} to {hi} step %c1 {{")
                body(inner)
                lines.append("}")
            elif kind == "if":
                _, left, right, inner = s
                a, b = value(left), value(right)
                counter[0] += 1
                lines.append(f"%t{counter[0]} = arith.cmpi uge, {a}, {b} : index")
                lines.append(f"scf.if %t{counter[0]} {{")
                body(inner)
                lines.append("}")
            elif kind == "mac":
                _, cr, ci, ar, ai, br, bi = s
                h, f, g = value(ci), value(ai), value(bi)
                counter[0] += 1
                k = counter[0]
                plus, times = ("arith.addf", "arith.mulf") if element == "f64" else ("arith.addi", "arith.muli")
                lines.extend([f"%x{k} = memref.load %{ar}[{f}] : {t}", f"%y{k} = memref.load %{br}[{g}] : {t}",
                          f"%z{k} = memref.load %{cr}[{h}] : {t}", f"%m{k} = {times} %x{k}, %y{k} : {element}",
                          f"%s{k} = {plus} %z{k}, %m{k} : {element}", f"memref.store %s{k}, %{cr}[{h}] : {t}"])
            elif kind == "set":
                _, root, index, v = s
                i = value(index)
                counter[0] += 1
                literal = str(v) if element == "i32" else "0x%016X" % bits(v, "f64")
                lines.extend([f"%v{counter[0]} = arith.constant {literal} : {element}",
                          f"memref.store %v{counter[0]}, %{root}[{i}] : {t}"])
            elif kind == "copy":
                _, root, index, source, from_index = s
                i, j = value(index), value(from_index)
                counter[0] += 1
                lines.extend([f"%w{counter[0]} = memref.load %{source}[{j}] : {t}",
                          f"memref.store %w{counter[0]}, %{root}[{i}] : {t}"])

    body(spec["body"])
    head = [f"%c{v} = arith.constant {v} : index" for v in sorted(consts)]
    arguments = ", ".join(f"%{r}: {t}" for r in ROOTS) + ", %n: index"
    return f"func.func @{spec['name']}({arguments}) {{\n" + "\n".join(head + lines) + "\nreturn\n}\n"


def macs(statements):
    for s in statements:
        if s[0] == "mac":
            yield s
        elif s[0] in ("loop", "if"):
            yield from macs(s[-1])


def predicted_groups(spec):
    """Triples in the order of their first site, as G4a numbers its groups."""
    out = []
    for s in macs(spec["body"]):
        triple = (s[1], s[3], s[5])
        if triple not in out:
            out.append(triple)
    return out


def set_texts(k):
    texts = ["all", "all-reversed"] + [str(i) for i in range(k)]
    if k >= 3:
        texts += [",".join(str(j) for j in range(k) if j != i) for i in range(k)]
    return texts + ["0,0", f"0,{k + 90}"]


def run_driver(driver, spec, sets):
    with tempfile.NamedTemporaryFile("w", suffix=".mlir") as handle:
        handle.write(print_function(spec))
        handle.flush()
        low, high = spec["scope"]
        command = [driver, handle.name, "--lower-structured", f"--index-bit-width={WIDTH}",
                   f"--scope=0:{low}:{high}", "--no-search", f"--authorize={AUTH}",
                   "--groups=" + ";".join(sets)] + SOLVER
        done = subprocess.run(command, capture_output=True, text=True, timeout=TIMEOUT)
    if done.returncode:
        raise SystemExit(f"driver failed on {spec['name']}:\n{done.stderr[-1500:]}")
    return json.loads(done.stdout)[0]


def fail(spec, text):
    raise SystemExit(f"{spec['name']}: {text}\n{print_function(spec)}")


def check_relation(spec, triples, points, rng, fills, tally, what):
    for n in points:
        for _ in range(fills):
            memory = fill(spec, rng)
            try:
                trace, final = interpret(spec, n, memory)
            except Stop:
                tally["stopped runs"] += 1
                continue
            tally["completing runs"] += 1
            try:
                why = relation_fails(spec, triples, memory, trace, final)
            except Stop:
                why = "the replay stops where the program does not"
            if why:
                fail(spec, f"n={n}: {what} is false: {why}")
            perturb(spec, triples, memory, trace, final, tally)


def perturb(spec, triples, memory, trace, final, tally):
    """A dropped, a duplicated and a swapped contribution, against the program's result."""
    element = spec["element"]
    try:
        cells = contributions(spec, triples, memory, trace)
    except Stop:
        return
    for cell, (first, items) in cells.items():
        if len({g for g, _ in items}) < 2:
            continue
        actual = bits(final[cell[0]][cell[1]], element)
        products = [p for _, p in items]
        try:
            dropped = bits(fold(spec, first, products[:-1]), element)
            doubled = bits(fold(spec, first, products[:1] + products), element)
        except Stop:
            continue
        tally["perturbed cells"] += 1
        tally["drop detected" if dropped != actual else "drop not seen"] += 1
        tally["duplicate detected" if doubled != actual else "duplicate not seen"] += 1
        # The last contribution swapped with the nearest earlier one of another group.
        last = len(items) - 1
        other = next((k for k in range(last - 1, -1, -1) if items[k][0] != items[last][0]), None)
        if other is not None:
            swapped = products[:]
            swapped[other], swapped[last] = swapped[last], swapped[other]
            try:
                value = bits(fold(spec, first, swapped), element)
            except Stop:
                continue
            if element == "i32":
                tally["integer swaps (commute)"] += 1
                if value != actual:
                    fail(spec, "an integer swap changed a modular sum")
            else:
                tally["float swap detected" if value != actual else "float swap not seen"] += 1
        return


def check_witness(spec, triples, entry, tally):
    element = spec["element"]
    memory = {ROOTS[r]: [from_hex(text, element) for text in cells] for r, cells in enumerate(entry["memory"])}
    n = entry["parameters"][0]
    try:
        trace, final = interpret(spec, n, memory)
    except Stop as stop:
        fail(spec, f"the witness does not complete here: {stop}")
    if not relation_fails(spec, triples, memory, trace, final):
        fail(spec, "the witness shows no difference")
    tally["witnesses replayed"] += 1


def check_claim_common(spec, claim):
    if claim["domain_and_order"] != "UNKNOWN" or claim["transformation"] != "UNKNOWN":
        fail(spec, "a later phase's claim was made")
    if claim["status"] == "REFUTED" and claim["tier"] != "PROPOSED":
        fail(spec, "a refutation has a tier")
    for name in ("group_order", "shared_cells"):
        fact = claim[name]
        if (fact["status"] == "REFUTED") != ("point" in fact):
            fail(spec, f"a {name} fact without its point")
        if fact["status"] == "PROVEN" and (claim["status"] != "VERIFIED" or fact["tier"] != claim["tier"]):
            fail(spec, f"a proven {name} fact on a claim that does not stand at its tier")


def check_set(spec, result, entry, rng, fills, tally, scope_points, traces):
    finite, symbolic = entry["finite"], entry["symbolic"]
    indices = entry["groups"]
    if entry["authorizes_lowering"]:
        fail(spec, "a set authorized lowering")
    if len(set(indices)) != len(indices):
        for claim in (finite, symbolic):
            if claim["status"] != "UNKNOWN" or "listed twice" not in claim["reason"]:
                fail(spec, f"a group listed twice gave {claim['status']}: {claim['reason']}")
        tally["duplicates refused"] += 1
        return
    if any(i >= len(result["groups"]) for i in indices):
        for claim in (finite, symbolic):
            if claim["status"] != "UNKNOWN":
                fail(spec, "a set with no such group was judged")
        tally["missing indices refused"] += 1
        return
    triples = [tuple(ROOTS[r] for r in roots) for roots in entry["roots"]]
    for claim in (finite, symbolic):
        check_claim_common(spec, claim)
        tally[f"set {claim['status']}"] += 1
    if symbolic["status"] == "REFUTED":
        fail(spec, "a symbolic set claim was refuted")
    expected_fold = "MODULAR_INTEGER" if spec["element"] == "i32" else "ORDERED_FLOAT"
    for claim in (finite, symbolic):
        if claim["status"] == "VERIFIED" and claim["fold"] != expected_fold:
            fail(spec, f"fold {claim['fold']} for {spec['element']}")

    # The finite claim at its points, and its facts there.
    covered = []
    if finite["status"] == "VERIFIED":
        listed = [p["parameters"][0] for p in finite["points"]]
        if len(finite["group_points"]) != len(finite["points"]):
            fail(spec, "group points are not parallel to the points")
        if finite["tier"] == "SCOPE_ENUMERATED":
            for n in scope_points:
                if traces.get(n) is not None and trace_facts(traces[n], triples)["total"] and n not in listed:
                    fail(spec, f"an enumerated set claim misses n={n}")
        for k, n in enumerate(listed):
            trace = traces.get(n)
            if trace is None:
                fail(spec, f"the claim lists n={n}, where no run completes")
            facts = trace_facts(trace, triples)
            point, group = finite["points"][k], finite["group_points"][k]
            got = {"events": group["events"], "shared": group["shared_cells"], "ordered": group["listed_order"],
                   "output_cells": point["output_cells"], "max_contributions": point["max_contributions"],
                   "total": point["events"]}
            if got != facts:
                fail(spec, f"n={n}: the claim's facts {got} are not the trace's {facts}")
            tally["set points matched"] += 1
            if facts["shared"]:
                tally["verified points with shared cells"] += 1
            if 0 in facts["events"]:
                tally["verified points with an empty group"] += 1
            if facts["max_contributions"] >= 3:
                tally["verified points with three contributions"] += 1
        covered = listed
        check_relation(spec, triples, listed, rng, fills, tally, "the finite set claim")
        tally["finite set VERIFIED " + finite["tier"]] += 1
    if finite["status"] == "REFUTED":
        check_witness(spec, triples, finite["witness"], tally)
        reads = [t for n in scope_points if traces.get(n) for t in traces[n]]
        tally["set refuted with a witness"] += 1
        written = {t[2] for t in reads if t[0] == "event" and t[1] in triples}
        if any(t[0] == "event" and t[1] in triples and (t[3] in written or t[4] in written) for t in reads):
            tally["set refuted with a cross-group read"] += 1
    if symbolic["status"] == "VERIFIED":
        check_relation(spec, triples, scope_points, rng, max(1, fills // 2), tally, "the symbolic set claim")
        tally["symbolic set VERIFIED"] += 1
        if any(traces.get(n) and trace_facts(traces[n], triples)["shared"] for n in scope_points):
            tally["symbolic sets with shared cells"] += 1
            if len({t[1:] for t in triples}) == len(triples) and len(triples) > 1:
                tally["symbolic sets of different operand roots sharing cells"] += 1

    # The facts over what they cover.
    for claim, points in ((finite, covered), (symbolic, scope_points)):
        for name, key, good in (("group_order", "ordered", True), ("shared_cells", "shared", 0)):
            fact = claim[name]
            tally[f"{name} {fact['status']}"] += 1
            if fact["status"] == "PROVEN":
                for n in points:
                    if traces.get(n) is not None and trace_facts(traces[n], triples)[key] != good:
                        fail(spec, f"{name} proven, and false at n={n}")
            if fact["status"] == "REFUTED":
                n = fact["point"][0]
                if traces.get(n) is None or trace_facts(traces[n], triples)[key] == good:
                    fail(spec, f"{name} refuted at n={n}, where it holds")


def check_spec(spec, driver, rng, fills, tally):
    predicted = predicted_groups(spec)
    sets = set_texts(len(predicted))
    result = run_driver(driver, spec, sets)
    if result["authorizes_lowering"]:
        fail(spec, "a result authorized lowering")
    found = [tuple(ROOTS[r] for r in (g["output"], g["first"], g["second"])) for g in result["groups"]]
    if found != predicted:
        fail(spec, f"the oracle's grouping {predicted} is not the driver's {found}")
    low, high = spec["scope"]
    scope_points = list(range(low, high + 1))
    traces = {n: structural_trace(spec, n) for n in scope_points}
    by_text = dict(zip(sets, result["group_sets"]))
    if len(result["group_sets"]) != len(sets):
        fail(spec, "a set without a result")
    tally["programs"] += 1
    for text, entry in by_text.items():
        check_set(spec, result, entry, rng, fills, tally, scope_points, traces)
    # The listing changes no claim: not its verdict or reason, nor its obligations,
    # contracts or points.
    forward, backward = by_text["all"], by_text["all-reversed"]
    for route in ("finite", "symbolic"):
        a, b = forward[route], backward[route]
        if (a["status"], a["tier"], a["reason"]) != (b["status"], b["tier"], b["reason"]):
            fail(spec, f"the listing changed the {route} claim: {a['reason']} / {b['reason']}")
        for key in ("obligations", "contracts", "points"):
            if a[key] != b[key]:
                fail(spec, f"the listing changed the {route} claim's {key}")
        tally["listings compared"] += 1
    # A set of one group is that group's claim.
    for g in range(len(predicted)):
        mine, theirs = by_text[str(g)], result["groups"][g]
        for route in ("finite", "symbolic"):
            a, b = mine[route], theirs[route]
            if (a["status"], a["tier"], a["reason"]) != (b["status"], b["tier"], b["reason"]):
                fail(spec, f"group {g} as a set differs from its own {route} claim")
        tally["one-group sets matched"] += 1
    # Each group alone verified, and the union not: an incompleteness, never an error.
    if all(result["groups"][g]["symbolic"]["status"] == "VERIFIED" for g in range(len(predicted))):
        if forward["symbolic"]["status"] != "VERIFIED":
            tally["union unknown although each group holds"] += 1
    # Leaving out a group that writes a listed cell must not verify where it does.
    if len(predicted) >= 3:
        for left in range(len(predicted)):
            subset = [p for j, p in enumerate(predicted) if j != left]
            entry = by_text[",".join(str(j) for j in range(len(predicted)) if j != left)]
            hits = [n for n in scope_points
                    if traces.get(n) and any(t[0] == "event" and t[1] == predicted[left] and
                                             any(u[0] == "event" and u[1] in subset and u[2] == t[2]
                                                 for u in traces[n]) for t in traces[n])]
            if not hits:
                continue
            tally["missing-group sets that share a cell"] += 1
            finite = entry["finite"]
            listed = [p["parameters"][0] for p in finite["points"]] if finite["status"] == "VERIFIED" else []
            if set(listed) & set(hits) or entry["symbolic"]["status"] == "VERIFIED":
                fail(spec, f"a set that leaves out a writer of its cells was verified at {hits}")
    want = spec.get("want", {})
    for text, (fs, ss) in want.items():
        got = (by_text[text]["finite"]["status"], by_text[text]["symbolic"]["status"])
        if got != (fs, ss):
            fail(spec, f"set {text}: wanted {(fs, ss)}, got {got}: {by_text[text]['finite']['reason']}")
        tally["hand expectations"] += 1


def L(var, low, high, *inner):
    return ("loop", var, low, high, list(inner))


def M(out, h, x, f, y, g):
    return ("mac", out, h, x, f, y, g)


V, R, U = "VERIFIED", "REFUTED", "UNKNOWN"


def catalogue():
    sib = [L("i", 0, "n", M("c", "i", "a", "i", "b", "i")), L("j", 0, "n", M("c", "j", "d", "j", "e", "j"))]
    raw = [L("i", 0, "n", M("c", "i", "a", "i", "b", "i")), L("j", 0, "n", M("d", "j", "c", "j", "e", "j"))]

    def S(name, body, element="i32", scope=(1, 3), want=None):
        return dict(name=name, body=body, element=element, scope=scope, want=want or {})

    return [
        # The numeric example: A in a, B in b, C in c and a unit factor in d.
        S("numeric", [L("i", 0, "n", M("c", "i", "a", "i", "d", "i")), L("j", 0, "n", M("c", "j", "b", "j", "d", "j"))],
          scope=(1, 2), want={"all": (V, V), "0": (R, U), "1": (R, U)}),
        S("sib_roots", sib, want={"all": (V, V), "all-reversed": (V, V), "0": (R, U), "1": (R, U)}),
        S("sib_float", sib, "f64", want={"all": (V, V)}),
        S("sib_float_backwards", sib[::-1], "f64", want={"all": (V, V)}),
        S("different_cells", [sib[0], L("j", 0, "n", M("c", ("+", "j", 4), "d", "j", "e", "j"))],
          want={"all": (V, V), "0": (V, V), "1": (V, V)}),
        S("reads_written", raw, want={"all": (R, U), "0": (V, V), "1": (R, U)}),
        S("reads_first", raw[::-1], want={"all": (V, U)}),
        S("interleaved", [L("i", 0, "n", M("c", "i", "a", "i", "b", "i"), M("c", "i", "d", "i", "e", "i"))],
          want={"all": (V, V)}),
        S("empty_second", [sib[0], L("j", "n", 2, M("c", "j", "d", "j", "e", "j"))], want={"all": (V, V)}),
        S("repeated", [L("t", 0, 2, sib[0]), sib[1]], want={"all": (V, V)}),
        S("three_groups", sib + [L("k", 0, "n", M("c", "k", "a", "k", "e", "k"))],
          want={"all": (V, V), "0,1": (R, U), "0,2": (R, U), "1,2": (R, U)}),
        S("guarded", [sib[0], ("if", "n", 2, [sib[1]])], want={"all": (V, V)}),
        S("framed", sib + [("set", "c", 7, 5), ("set", "d", 7, 5)], want={"all": (V, V)}),
        S("overwritten", sib + [("set", "c", 0, 5)], want={"all": (R, U)}),
    ]


def random_spec(rng, name):
    element = rng.choice(["i32", "i32", "f64"])
    nests = []
    for s in range(rng.choice([2, 2, 3, 3, 4])):
        var = "ijkl"[s]
        out = rng.choice(["c", "c", "d"])
        first = rng.choice(["a", "b", "e", "f", "a", "c", "d"])
        second = rng.choice(["a", "b", "e", "f"])
        index = lambda: rng.choice([var, var, ("+", var, 1), ("+", var, 4), 0])
        low, high = rng.choice([(0, "n"), (0, "n"), (0, 2), ("n", 3), (1, "n")])
        nest = L(var, low, high, M(out, rng.choice([var, var, var, ("+", var, 1), 0]), first, index(), second, index()))
        if rng.random() < 0.2:
            nest = L("t" + var, 0, 2, nest)
        if rng.random() < 0.15:
            nest = ("if", "n", rng.choice([1, 2, 3]), [nest])
        nests.append(nest)
    if rng.random() < 0.25:
        value = 5 if element == "i32" else 0.5
        foreign = rng.choice([("set", rng.choice("cda"), rng.choice([0, 5, 7]), value),
                              ("copy", rng.choice("cd"), rng.choice([6, 7, 0]), rng.choice("ab"), 0)])
        nests.insert(rng.randrange(len(nests) + 1), foreign)
    return dict(name=name, element=element, body=nests, scope=(1, 3), want={})


def demonstrate_numeric(tally):
    """The brief's example, run here: C = [0, 0], A = [1, 2], B = [10, 20]."""
    spec = catalogue()[0]
    memory = {r: [0] * EXTENT for r in ROOTS}
    memory["a"][:2], memory["b"][:2], memory["d"][:2] = [1, 2], [10, 20], [1, 1]
    trace, final = interpret(spec, 2, memory)
    steps = [(t[2][1], t[7]) for t in trace if t[0] == "event"]
    if steps != [(0, 1), (1, 2), (0, 11), (1, 22)] or final["c"][:2] != [11, 22]:
        raise SystemExit(f"the numeric example: {steps}, {final['c'][:2]}")
    triples = [("c", "a", "d"), ("c", "b", "d")]
    if relation_fails(spec, triples, memory, trace, final) is not None:
        raise SystemExit("the numeric example breaks the merged relation")
    if relation_fails(spec, triples[:1], memory, trace, final) is None:
        raise SystemExit("the numeric example satisfies one group's relation alone")
    tally["numeric example"] += 1


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--driver", required=True)
    parser.add_argument("--programs", type=int, default=40)
    parser.add_argument("--fills", type=int, default=16)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--jobs", type=int, default=2)
    args = parser.parse_args()
    tally = Counter()
    demonstrate_numeric(tally)
    rng = random.Random(args.seed)
    specs = catalogue() + [random_spec(rng, f"m{i}") for i in range(args.programs)]

    def one(item):
        n, spec = item
        local = Counter()
        try:
            check_spec(spec, args.driver, random.Random(args.seed * 7919 + n), args.fills, local)
        except subprocess.TimeoutExpired:
            local["timed out"] += 1
        return local

    with ThreadPoolExecutor(args.jobs) as pool:
        for local in pool.map(one, enumerate(specs)):
            tally.update(local)
    for key in sorted(tally):
        print(f"{key}: {tally[key]}")
    if tally["timed out"]:
        raise SystemExit("a driver run timed out")
    # The run says something about Theorem 16 only if each case was met.
    needs = {"verified points with shared cells": 10, "symbolic sets with shared cells": 3,
             "symbolic sets of different operand roots sharing cells": 2, "set refuted with a witness": 4,
             "set refuted with a cross-group read": 1, "group_order PROVEN": 5, "group_order REFUTED": 3,
             "shared_cells PROVEN": 3, "shared_cells REFUTED": 5, "verified points with an empty group": 2,
             "verified points with three contributions": 1, "missing-group sets that share a cell": 2,
             "drop detected": 10, "duplicate detected": 10, "float swap detected": 1, "duplicates refused": 1,
             "one-group sets matched": 10, "hand expectations": 20, "numeric example": 1}
    short = {k: tally[k] for k, v in needs.items() if tally[k] < v}
    if short:
        raise SystemExit(f"this run did not exercise: {short}")
    print("0 disagreements")


if __name__ == "__main__":
    main()
