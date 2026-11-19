#!/usr/bin/env python3
"""G5b-M4-0 oracle. Random programs of sibling loop nests, each statement one MAC site
C[h] := R[h'] + A[f] * B[g] over argument roots and, sometimes, a fresh allocation, are
printed as MLIR for generalized-gemm-identity-report and interpreted here by plain Python
loops. Every answer is recomputed with no library code:

- G1: the sites, in program order, with their roots, and the groups the enumerator makes.
- The group gate, on groups made by hand: the first failure, in the contract's order
  (unsupported, no output, no sites, an id past the program, an id twice, not exactly the
  direct sites of the root triple), and the same words from the symbolic routes of G3b,
  G4a and G5a whenever the gate refuses. Every program asks at site id 0, at the last id
  and at the first id past them. The finite routes at the first group index past the
  groups say "no such group", all three.
- Cells: one root and one index are one cell, one root and two indices are two, roots of
  which one is fresh are apart, two argument roots are apart only under the name
  "roots i and j are separate" with i < j, and anything else may alias, including a root
  the program does not have and two ranks of one root. C[0] against D[0] is asked with
  and without that name.
- G3b on the empty family, at each point the driver ran, from the events interpreted
  here: refuted at the first point where the output's read and written cells differ
  (every family is refuted, the reason names output_read_differs), or where the written,
  first or second cells are not one cell across the group's events (this family only,
  with the counterexample kind and events named). Otherwise verified, at the tier of the
  whole scope only when no point was removed, and at the point tier when one was.
- G4a's finite claim is never at the tier of the whole scope once a point is removed.
  With every point run and the claim verified, its tier is the whole scope exactly when
  G4a reports no point left open. Whether a point stays open (a replay neither equal nor
  refuted) is G4a's own finding, which this oracle does not predict: that half is a
  check of consistency, not an independent one. A flat family refuted on sibling loops
  refutes that family only, never the program.

Every driver run has a time limit and an address-space limit, and a timeout fails the
run. A sanitizer build reserves its shadow memory as address space: give it
--memory-limit-gb 0, which keeps the time limit and drops only the address-space limit.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import random
import resource
import subprocess
import sys
import tempfile

TIMEOUT = 300
MEMORY = 4 << 30
WIDTH = 16
SIZE = 64
SOLVER = ["--timeout-ms=3000", "--rlimit=2000000"]
ARGUMENTS = 4  # %a %b %c %d are roots 0..3; a fresh allocation, when present, is root 4.
NAMES = ["a", "b", "c", "d", "f"]


class Failure(Exception):
    pass


def limit():
    if MEMORY:
        resource.setrlimit(resource.RLIMIT_AS, (MEMORY, MEMORY))


# ---------------------------------------------------------------------------------------
# Programs.
#
# A spec: {"fresh": bool, "nests": [nest]}, a nest: {"loops": [(var, bound)], "body":
# [stmt]} with a bound ("n",), ("const", k) or ("outer+1",) (the enclosing variable plus
# one), and a statement: {"out", "read", "a", "b": roots, "h", "hr", "f", "g": indices}
# with an index ("var", v), ("var+1", v), ("const", k) or ("sum", v, w). A nest with no
# loops holds statements with constant indices, run once.


def gen_index(rng, loops, allow_sum=True):
    vars_ = [v for v, _ in loops]
    choices = [("const", rng.randrange(3))]
    if vars_:
        v = rng.choice(vars_)
        choices += [("var", v), ("var", v), ("var+1", v)]
        if allow_sum and len(vars_) > 1:
            choices.append(("sum", vars_[0], vars_[1]))
    return rng.choice(choices)


def gen_stmt(rng, loops, roots, shape):
    out = rng.choice([2, 2, 3] + ([4] if 4 in roots else []))
    stmt = {"out": out, "read": out, "a": rng.choice([0, 0, 1]), "b": rng.choice([1, 1, 0])}
    if rng.random() < 0.1:
        stmt["a"] = rng.choice(roots)
    if shape == "cross" or rng.random() < 0.08:
        stmt["read"] = rng.choice([r for r in roots if r != out])
    stmt["h"] = gen_index(rng, loops)
    stmt["hr"] = stmt["h"]
    if shape == "shift" or rng.random() < 0.1:
        stmt["hr"] = ("var+1", loops[-1][0]) if loops else ("const", (stmt["h"][1] + 1) % 3)
        if stmt["hr"] == stmt["h"]:
            stmt["hr"] = ("const", 5)
    stmt["f"] = gen_index(rng, loops)
    stmt["g"] = gen_index(rng, loops)
    if shape == "constant":
        for key in ("h", "f", "g"):
            stmt[key] = ("const", rng.randrange(2))
        stmt["hr"] = stmt["h"]
    return stmt


def gen_spec(rng, index):
    # Every few programs is shaped, so that each outcome the oracle counts appears.
    shape = ["siblings", "shift", "constant", "cross", "fresh", "free"][index % 6]
    # Every second cross program reads another root in every update: no group is supported
    # and the enumeration's gate closes, which a run must see whatever its seed.
    every_cross = shape == "cross" and index % 12 == 3
    fresh = shape == "fresh" or rng.random() < 0.3
    roots = list(range(ARGUMENTS)) + ([4] if fresh else [])
    nests = []
    count = rng.randint(1, 3) if shape != "siblings" else rng.randint(2, 3)
    for number in range(count):
        depth = 0 if (shape == "constant" and number == 0) else rng.choice([1, 1, 2])
        loops = []
        for level in range(depth):
            var = f"v{number}_{level}"
            if level == 1 and rng.random() < 0.3:
                bound = ("outer+1",)
            else:
                bound = ("n",) if rng.random() < 0.75 else ("const", rng.randint(1, 2))
            loops.append((var, bound))
        kind = "cross" if every_cross else (shape if number == 0 else "free")
        body = [gen_stmt(rng, loops, roots, kind) for _ in range(rng.choice([1, 1, 2]))]
        nests.append({"loops": loops, "body": body})
    if shape == "siblings":
        # Two sibling nests with one root triple: one group across them.
        twin = dict(nests[0]["body"][0])
        nests[1]["body"][0] = dict(twin)
        for key in ("h", "hr", "f", "g"):
            nests[1]["body"][0][key] = rebind(twin[key], nests[0]["loops"], nests[1]["loops"])
        nests[1]["body"][0]["hr"] = nests[1]["body"][0]["h"]
    if shape == "constant":
        for nest in nests:
            for stmt in nest["body"]:
                for key in ("h", "f", "g"):
                    stmt[key] = ("const", 0)
                stmt["hr"] = stmt["h"]
                stmt["read"] = stmt["out"]
    lo = rng.choice([0, 1, 1])
    hi = lo + rng.choice([1, 2])
    return {"name": f"p{index}", "fresh": fresh, "nests": nests, "scope": (lo, hi), "shape": shape}


def rebind(idx, old, new):
    """The same index over another nest's variables, position by position."""
    names = {v: (new[i][0] if i < len(new) else None) for i, (v, _) in enumerate(old)}
    if idx[0] == "const":
        return idx
    if idx[0] == "sum":
        a, b = names.get(idx[1]), names.get(idx[2])
        return ("sum", a, b) if a and b else ("const", 0)
    v = names.get(idx[1])
    return (idx[0], v) if v else ("const", 0)


def statements(spec):
    out = []
    for nest in spec["nests"]:
        for stmt in nest["body"]:
            out.append((nest, stmt))
    return out


def print_function(spec):
    lines = [f"func.func @{spec['name']}(%a: memref<{SIZE}xi32>, %b: memref<{SIZE}xi32>, "
             f"%c: memref<{SIZE}xi32>, %d: memref<{SIZE}xi32>, %n: index) {{"]
    lines += ["  %c0 = arith.constant 0 : index", "  %c1 = arith.constant 1 : index",
              "  %c2 = arith.constant 2 : index", "  %c5 = arith.constant 5 : index"]
    if spec["fresh"]:
        # A fresh allocation is uninitialized, and a run that reads it stops: every cell is
        # written first, by a store that is no MAC site.
        lines += [f"  %f = memref.alloc() : memref<{SIZE}xi32>",
                  "  %zero = arith.constant 0 : i32", f"  %size = arith.constant {SIZE} : index",
                  "  scf.for %w = %c0 to %size step %c1 {",
                  f"    memref.store %zero, %f[%w] : memref<{SIZE}xi32>", "  }"]
    counter = [0]

    def fresh_name(base):
        counter[0] += 1
        return f"%{base}{counter[0]}"

    def index_value(idx, body, indent):
        kind = idx[0]
        if kind == "const":
            return {0: "%c0", 1: "%c1", 2: "%c2", 5: "%c5"}[idx[1]]
        if kind == "var":
            return f"%{idx[1]}"
        name = fresh_name("x")
        if kind == "var+1":
            body.append(f"{indent}{name} = arith.addi %{idx[1]}, %c1 : index")
        else:
            body.append(f"{indent}{name} = arith.addi %{idx[1]}, %{idx[2]} : index")
        return name

    for nest in spec["nests"]:
        indent = "  "
        closing = 0
        outer = None
        for var, bound in nest["loops"]:
            if bound[0] == "n":
                upper = "%n"
            elif bound[0] == "const":
                upper = {1: "%c1", 2: "%c2"}[bound[1]]
            else:
                upper = fresh_name("u")
                lines.append(f"{indent}{upper} = arith.addi %{outer}, %c1 : index")
            lines.append(f"{indent}scf.for %{var} = %c0 to {upper} step %c1 {{")
            indent += "  "
            closing += 1
            outer = var
        for stmt in nest["body"]:
            body = []
            f = index_value(stmt["f"], body, indent)
            g = index_value(stmt["g"], body, indent)
            hr = index_value(stmt["hr"], body, indent)
            h = index_value(stmt["h"], body, indent)
            x, y, z, m, s = (fresh_name(t) for t in "xyzms")
            ty = f"memref<{SIZE}xi32>"
            body += [f"{indent}{x} = memref.load %{NAMES[stmt['a']]}[{f}] : {ty}",
                     f"{indent}{y} = memref.load %{NAMES[stmt['b']]}[{g}] : {ty}",
                     f"{indent}{z} = memref.load %{NAMES[stmt['read']]}[{hr}] : {ty}",
                     f"{indent}{m} = arith.muli {x}, {y} : i32",
                     f"{indent}{s} = arith.addi {z}, {m} : i32",
                     f"{indent}memref.store {s}, %{NAMES[stmt['out']]}[{h}] : {ty}"]
            lines += body
        for level in range(closing):
            indent = indent[:-2]
            lines.append(f"{indent}}}")
    lines += ["  return", "}"]
    return "\n".join(lines) + "\n"


def interpret(spec, n):
    """The events of one run: (site, h, h', f, g), in execution order."""
    events = []
    site_of = {}
    for number, (_, stmt) in enumerate(statements(spec)):
        site_of[id(stmt)] = number

    def value(idx, env):
        if idx[0] == "const":
            return idx[1]
        if idx[0] == "var":
            return env[idx[1]]
        if idx[0] == "var+1":
            return env[idx[1]] + 1
        return env[idx[1]] + env[idx[2]]

    def run(nest, level, env):
        if level == len(nest["loops"]):
            for stmt in nest["body"]:
                events.append((site_of[id(stmt)], value(stmt["h"], env), value(stmt["hr"], env),
                               value(stmt["f"], env), value(stmt["g"], env)))
            return
        var, bound = nest["loops"][level]
        if bound[0] == "n":
            upper = n
        elif bound[0] == "const":
            upper = bound[1]
        else:
            upper = env[nest["loops"][level - 1][0]] + 1
        for v in range(upper):
            env[var] = v
            run(nest, level + 1, env)
        env.pop(var, None)

    for nest in spec["nests"]:
        run(nest, 0, {})
    return events


# ---------------------------------------------------------------------------------------
# The contract, written again here.


def model_sites(spec):
    return [{"id": i, "out": s["out"], "read": s["read"], "a": s["a"], "b": s["b"],
             "direct": s["read"] == s["out"]} for i, (_, s) in enumerate(statements(spec))]


def model_groups(sites):
    groups, known = [], {}
    for site in sites:
        if not site["direct"]:
            groups.append({"output": site["out"], "first": site["a"], "second": site["b"],
                           "sites": [site["id"]],
                           "unsupported": "the update reads one root and writes another"})
            continue
        key = (site["out"], site["a"], site["b"])
        if key not in known:
            known[key] = len(groups)
            groups.append({"output": key[0], "first": key[1], "second": key[2], "sites": [],
                           "unsupported": ""})
        groups[known[key]]["sites"].append(site["id"])
    return groups


def gate(sites, group):
    if group["unsupported"]:
        return group["unsupported"]
    if group["output"] is None:
        return "the group has no output in memory"
    ids = group["sites"]
    if not ids:
        return "the group has no sites"
    if any(i >= len(sites) for i in ids):
        return "the group names a site the program does not have"
    if len(set(ids)) != len(ids):
        return "a site is listed twice"
    exact = {s["id"] for s in sites
             if s["direct"] and (s["out"], s["a"], s["b"]) == (group["output"], group["first"], group["second"])}
    if set(ids) != exact:
        return "the group is not the sites of its roots"
    return None


def relation(roots, fresh, names, first, second):
    (ra, ia), (rb, ib) = first, second
    if ra >= roots or rb >= roots:
        return "MAY_ALIAS"
    if ra == rb:
        if len(ia) != len(ib):
            return "MAY_ALIAS"
        return "SAME" if ia == ib else "DISTINCT"
    if fresh[ra] or fresh[rb]:
        return "DISTINCT"
    low, high = min(ra, rb), max(ra, rb)
    return "DISTINCT" if f"roots {low} and {high} are separate" in names else "MAY_ALIAS"


def empty_family(spec, group, points, dropped):
    """G3b's verdict on the empty family over the points listed, in their order."""
    members = set(group["sites"])
    seen = False
    for point in points:
        events = [e for e in interpret(spec, point[0]) if e[0] in members]
        if not events:
            continue
        seen = True
        for number, (_, h, hr, _, _) in enumerate(events):
            if h != hr:
                return {"status": "REFUTED", "refutes": "EVERY_FAMILY", "point": point,
                        "reason": f"output_read_differs at events {number} and {number}"}
        for kind, column in (("dependence_h", 1), ("dependence_f", 3), ("dependence_g", 4)):
            for number, event in enumerate(events):
                if event[column] != events[0][column]:
                    return {"status": "REFUTED", "refutes": "THIS_FAMILY", "point": point,
                            "reason": f"{kind} at events 0 and {number}"}
    if not seen:
        return {"status": "UNKNOWN", "refutes": "NOTHING", "reason": "no point has events"}
    return {"status": "VERIFIED", "refutes": "NOTHING",
            "tier": "SCOPE_ENUMERATED" if dropped == 0 else "POINT"}


# ---------------------------------------------------------------------------------------
# Requests.


def site_lists(rng, sites, groups, roots):
    count = len(sites)
    last, past = count - 1, count
    triple = lambda s: (s["out"], s["a"], s["b"])
    lists = []

    def add(output, first, second, ids, unsupported=False):
        lists.append({"output": output, "first": first, "second": second, "sites": list(ids),
                      "unsupported": "marked unsupported by the driver" if unsupported else ""})

    t0, tl = triple(sites[0]), triple(sites[last])
    add(*t0, [0])
    add(*tl, [last])
    add(*t0, [past])
    add(*tl, [last, past])
    add(*t0, [0, 0])
    add(*t0, [])
    add(None, t0[1], t0[2], [0])
    add(*t0, [0], unsupported=True)
    for group in groups:
        key = (group["output"], group["first"], group["second"])
        ids = group["sites"]
        add(*key, ids)
        add(*key, list(reversed(ids)))
        add(*key, ids + [past])
        add(*key, ids + [ids[0]])
        if len(ids) > 1:
            add(*key, ids[1:])
        others = [s["id"] for s in sites if s["id"] not in ids]
        if others:
            add(*key, ids + [rng.choice(others)])
        if last not in ids:
            add(*key, ids + [last])
    for _ in range(6):
        ids = [rng.randrange(count + 2) for _ in range(rng.randint(0, 3))]
        output = rng.choice(list(range(roots + 1)) + [None])
        add(output, rng.randrange(roots + 1), rng.randrange(roots + 1), ids,
            unsupported=rng.random() < 0.1)
    return lists


def spec_text(group):
    out = "-" if group["output"] is None else str(group["output"])
    mark = "!" if group["unsupported"] else ""
    return f"{mark}{out},{group['first']},{group['second']}:" + ",".join(map(str, group["sites"]))


def cell_pairs(rng, roots):
    pairs = [((2, (0,)), (3, (0,))), ((2, (0,)), (2, (0,))), ((2, (0,)), (2, (1,))),
             ((3, (1,)), (2, (1,))), ((2, (0,)), (2, (0, 0))), ((2, (0,)), (roots, (0,)))]
    if roots > ARGUMENTS:
        pairs.append(((2, (0,)), (4, (0,))))
    for _ in range(10):
        a = (rng.randrange(roots + 2), tuple(rng.randrange(3) for _ in range(rng.choice([1, 1, 2]))))
        b = (rng.randrange(roots + 2), tuple(rng.randrange(3) for _ in range(rng.choice([1, 1, 2]))))
        if rng.random() < 0.3:
            b = (b[0], a[1])
        pairs.append((a, b))
    return pairs


def cell_text(cell):
    return f"{cell[0]}:" + ",".join(map(str, cell[1]))


def names_for(rng, mode):
    every = [f"roots {i} and {j} are separate" for i in range(ARGUMENTS) for j in range(i + 1, ARGUMENTS)]
    if mode == "all":
        names = list(every)
    else:
        names = [name for name in every if rng.random() < 0.4]
    # Names that must count for nothing: reversed, and of roots the program does not have.
    names += [f"roots {j} and {i} are separate" for i in range(ARGUMENTS) for j in range(i + 1, ARGUMENTS)
              if rng.random() < 0.3]
    names.append("roots 7 and 9 are separate")
    return names


def run_driver(driver, spec, lists, pairs, names, dropped):
    with tempfile.NamedTemporaryFile("w", suffix=".mlir") as handle:
        handle.write(print_function(spec))
        handle.flush()
        low, high = spec["scope"]
        command = [driver, handle.name, "--lower-structured", f"--index-bit-width={WIDTH}",
                   f"--scope=0:{low}:{high}", f"--drop-points={dropped}",
                   "--site-lists=" + ";".join(spec_text(g) for g in lists),
                   "--cells=" + ";".join(cell_text(a) + "/" + cell_text(b) for a, b in pairs),
                   "--authorize=" + ",".join(names)] + SOLVER
        try:
            done = subprocess.run(command, capture_output=True, text=True, timeout=TIMEOUT,
                                  preexec_fn=limit)
        except subprocess.TimeoutExpired:
            raise Failure(f"{spec['name']}: the driver timed out, which is no pass")
    if done.returncode:
        raise Failure(f"{spec['name']}: driver exit {done.returncode}:\n{done.stderr[-1500:]}")
    out = json.loads(done.stdout)
    if len(out) != 1:
        raise Failure(f"{spec['name']}: {len(out)} functions reported")
    return out[0]


# ---------------------------------------------------------------------------------------
# Checks.


def check(spec, seed, driver, counts):
    rng = random.Random(seed)
    sites = model_sites(spec)
    groups = model_groups(sites)
    nest_of = [number for number, nest in enumerate(spec["nests"]) for _ in nest["body"]]
    roots = ARGUMENTS + (1 if spec["fresh"] else 0)
    fresh = [False] * ARGUMENTS + ([True] if spec["fresh"] else [])
    low, high = spec["scope"]
    scope = [[n] for n in range(low, high + 1)]
    runs = [("all", 0), ("all", 1 if len(scope) > 1 else 0), ("some", 0)]
    full_points = None
    for mode, dropped in runs:
        names = names_for(rng, mode)
        lists = site_lists(rng, sites, groups, roots)
        pairs = cell_pairs(rng, roots)
        out = run_driver(driver, spec, lists, pairs, names, dropped)
        where = f"{spec['name']} ({mode}, dropped {dropped})"

        def need(condition, what):
            if not condition:
                raise Failure(f"{where}: {what}\n{print_function(spec)}")

        # The enumerator opens only for a program with a supported group, and closes with the
        # first group's words otherwise.
        supported = [g for g, group in enumerate(groups) if not group["unsupported"]]
        need(out["supported"] and out["certificate"], "unsupported, or no certificate")
        if supported:
            need(out["gate"] == "OPEN" and out["gate_reason"] == "", f"gate {out['gate']}: {out['gate_reason']}")
            closed = None
        else:
            need(out["gate"] == "UNSUPPORTED" and out["gate_reason"] == groups[0]["unsupported"],
                 f"gate {out['gate']}: {out['gate_reason']}")
            closed = "the domain gate is closed: " + groups[0]["unsupported"]
            counts["gate closed, no supported group"] += 1
        need(out["authorizes_lowering"] is False, "authorizes lowering")
        need(out["outside_domain"] == 0, "points outside the domain")
        # G1 and the enumerator's groups.
        theirs = [(s["id"], s["output"], s["read"], s["a"], s["b"], s["direct_form"]) for s in out["sites"]]
        ours = [(s["id"], s["out"], s["read"], s["a"], s["b"], True) for s in sites]
        need(theirs == ours, f"sites differ: {theirs} against {ours}")
        need([r["fresh"] for r in out["roots"]] == fresh, "roots differ")
        need(out["groups"] == groups, f"groups differ: {out['groups']} against {groups}")
        # The gate, and the symbolic routes' words when it refuses.
        need(len(out["site_lists"]) == len(lists), "site lists lost")
        for asked, answer in zip(lists, out["site_lists"]):
            refusal = gate(sites, asked)
            need(answer["spec"] == spec_text(asked), "site list order")
            need(answer["whole"] == (refusal is None), f"{answer['spec']}: whole {answer['whole']}")
            if refusal is None:
                need(answer["reason"] == "" and "g3b" not in answer, f"{answer['spec']}: reason")
                counts["whole"] += 1
                continue
            need(answer["reason"] == refusal, f"{answer['spec']}: '{answer['reason']}', not '{refusal}'")
            for route in ("g3b", "g4a", "g5a"):
                need(answer[route] == refusal, f"{answer['spec']}: {route} says '{answer[route]}'")
            counts["refused: " + refusal] += 1
        if 0 in [i for g in lists for i in g["sites"]]:
            counts["asked at id 0"] += 1
        for route in ("g3b", "g4a", "g5a"):
            need(out["past_groups"][route] == (closed or "no such group"), f"{route} past the groups")
        if closed:
            need(out["points"] == [] and out["finite"] == [] and not out["ran_its_scope"], "a closed gate ran")
            continue
        # Cells.
        need(len(out["cells"]) == len(pairs), "cells lost")
        for (a, b), answer in zip(pairs, out["cells"]):
            expect = relation(roots, fresh, set(names), a, b)
            need(answer["relation"] == expect, f"{answer['spec']}: {answer['relation']}, not {expect}")
            kind = expect
            if a[0] != b[0] and expect == "DISTINCT":
                kind += " by fresh" if (a[0] < roots and fresh[a[0]]) or (b[0] < roots and fresh[b[0]]) else " by name"
            if (a, b) == ((2, (0,)), (3, (0,))):
                kind = "C[0] D[0] " + expect
            counts["cell " + kind] += 1
        # The points: the scope, then the same list with its last points removed.
        points = out["points"]
        need(len(points) == len(scope) - dropped and len({tuple(p) for p in points}) == len(points),
             f"points {points}")
        need(all(p in scope for p in points), f"points {points} outside the scope")
        need(all(reason == "" for reason in out["point_reasons"]), f"a run stopped: {out['point_reasons']}")
        if dropped == 0:
            need(sorted(points) == scope, "not every point ran")
            full_points = points
        elif full_points is not None:
            need(points == full_points[:len(points)], "the removed points are not the last")
        need(out["ran_its_scope"] == (dropped == 0), f"ran its scope: {out['ran_its_scope']}")
        # G3b on the empty family, and G4a's tier.
        need([f["group"] for f in out["finite"]] == supported, "finite groups")
        for entry in out["finite"]:
            group = groups[entry["group"]]
            expect = empty_family(spec, group, points, dropped)
            got = entry["empty_family"]
            need(got["status"] == expect["status"] and got["refutes"] == expect["refutes"],
                 f"group {entry['group']}: {got}, not {expect}")
            if expect["status"] != "VERIFIED":
                need(got["reason"] == expect["reason"],
                     f"group {entry['group']}: '{got['reason']}', not '{expect['reason']}'")
            if expect["status"] == "REFUTED":
                need(got.get("failing_point") == [expect["point"]], f"group {entry['group']}: failing point")
            else:
                need("failing_point" not in got, f"group {entry['group']}: a failing point")
            if expect["status"] == "VERIFIED":
                need(got["tier"] == expect["tier"], f"group {entry['group']}: tier {got['tier']}")
            counts[f"empty family {expect['status']} {expect.get('tier', expect['refutes'])}"] += 1
            g4 = entry["g4a"]
            if dropped:
                need(g4["tier"] != "SCOPE_ENUMERATED", f"group {entry['group']}: G4a promoted")
            elif g4["status"] == "VERIFIED":
                # G4a's own finding: a point whose replay is neither equal nor refuted.
                left_open = "another point is open" in g4["reason"]
                need(g4["tier"] == ("POINT" if left_open else "SCOPE_ENUMERATED"),
                     f"group {entry['group']}: G4a tier {g4['tier']}: {g4['reason']}")
                if left_open:
                    counts["g4a verified with a point left open"] += 1
            counts[f"g4a {g4['status']} {g4['tier']}" + (" dropped" if dropped else "")] += 1
            # One flat family over sibling loops: refuting it refutes that family only.
            if len({nest_of[site] for site in group["sites"]}) > 1 and expect["refutes"] == "THIS_FAMILY":
                counts["sibling flat family refuted"] += 1
                if g4["status"] == "VERIFIED":
                    counts["sibling flat family refuted, G4a verified"] += 1


MINIMUM = {
    "whole": 20,
    "refused: marked unsupported by the driver": 10,
    "refused: the group has no output in memory": 10,
    "refused: the group has no sites": 10,
    "refused: the group names a site the program does not have": 20,
    "refused: a site is listed twice": 20,
    "refused: the group is not the sites of its roots": 20,
    "asked at id 0": 10,
    "cell SAME": 20,
    "cell DISTINCT": 20,
    "cell DISTINCT by fresh": 3,
    "cell DISTINCT by name": 10,
    "cell MAY_ALIAS": 20,
    "cell C[0] D[0] DISTINCT": 5,
    "cell C[0] D[0] MAY_ALIAS": 5,
    "empty family VERIFIED SCOPE_ENUMERATED": 3,
    "empty family VERIFIED POINT": 2,
    "empty family REFUTED EVERY_FAMILY": 3,
    "empty family REFUTED THIS_FAMILY": 10,
    "gate closed, no supported group": 2,
    "sibling flat family refuted": 5,
    "sibling flat family refuted, G4a verified": 2,
}


def main():
    global MEMORY
    parser = argparse.ArgumentParser()
    parser.add_argument("--driver", required=True)
    parser.add_argument("--programs", type=int, default=60)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--jobs", type=int, default=2)
    parser.add_argument("--no-minimum", action="store_true")
    parser.add_argument("--memory-limit-gb", type=int, default=MEMORY >> 30,
                        help="address-space limit per driver run; 0 for none (sanitizer builds)")
    args = parser.parse_args()
    MEMORY = args.memory_limit_gb << 30
    rng = random.Random(args.seed)
    specs = [gen_spec(rng, i) for i in range(args.programs)]
    seeds = [rng.randrange(1 << 30) for _ in specs]
    from collections import Counter
    counts = Counter()
    failures = []

    def one(item):
        spec, seed = item
        local = Counter()
        try:
            check(spec, seed, args.driver, local)
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
    # A disagreement goes to stderr, where the mutation harness reads it as this oracle's own.
    for failure in failures[:10]:
        print("FAIL:", failure, file=sys.stderr)
    print(f"{len(specs)} programs, {len(failures)} failures")
    sys.exit(1 if failures else 0)


if __name__ == "__main__":
    main()
