#!/usr/bin/env python3
"""G5b-M4a oracle: domain claims over the union of several groups.

Random programs of nests run one after the other, each nest one MAC site C[h] += A[f] * B[g]
over its own root triple, from four argument roots and sometimes a fresh allocation. Some
nests share a triple (one group over two nests), some write the same output root with other
operands (two groups writing the same cells), some write another root, and some put two
sites of two groups in one loop body. They are printed as MLIR for the domain driver and
interpreted here by plain Python loops. The driver is asked, with every name authorized and
with none, for the union of every group, in both listing orders, and for each group alone.
Every answer is recomputed with no library code:

- XS and XP: the union's groups are the oracle's; the segments partition the union's sites,
  each segment inside one group, which segment_groups names.
- Theorem 15 over the union: a finite Verified claim holds at each listed point, a symbolic
  one at every point of the scope. The events of the union's sites never return to an
  earlier segment, and each segment's events in order are its set in order: the set is
  enumerated here from the printed order, and the events' level values come from each
  event's state. Each listed point's event count is the union's. A finite refutation
  fails Theorem 15 at its point.
- Groups whose sites interleave in one loop body are never Verified.
- CI: the root pairs are the oracle's: apart by a fresh allocation or by name, or may
  alias. No two segments share a cell, and no cell has two contributions, are Proven only
  when that holds on every cell of one root at every point of the scope and every two
  output roots are apart; they are Refuted only with a point where it fails on one root.
- The listing changes nothing: `all` and `all-reversed` print the same claims. A set of one
  group is that group's own segmented claim, word for word.

Every driver run has a time limit and an address-space limit, and a timeout fails the run.
"""
import argparse
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
import json
import random
import resource
import subprocess
import sys
import tempfile

TIMEOUT = 900
MEMORY = 4 << 30
WIDTH = 16
SIZE = 16
SOLVER = ["--timeout-ms=3000", "--rlimit=2000000"]
ROOTS = {"a": 0, "b": 1, "c": 2, "d": 3, "f": 4}


class Failure(Exception):
    pass


def limit():
    if MEMORY:
        resource.setrlimit(resource.RLIMIT_AS, (MEMORY, MEMORY))


# ---------------------------------------------------------------------------------------
# Terms the driver prints, at their own width (as in domain_oracle.py, which this copies so
# that each oracle stands alone).


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
    """S(v): the order's points, in its order."""
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
# Programs. A spec: {"name", "fresh", "scope", "nests": [nest]}; a nest: {"loops": [(var,
# bound)], "when": None or k (the nest runs only when n > k), "sites": [site]}; a bound is
# ("n",), ("const", k) or ("outer",) (the enclosing variable, plus one); a site: {"out",
# "a", "b": root names, "h": index}, an index ("var", v, c) meaning v + c, or ("const", c).


def gen_site(rng, loops, roots, out=None):
    v = rng.choice(loops)[0] if loops else None
    h = ("var", v, rng.choice([0, 0, 1])) if v else ("const", rng.randrange(3))
    if loops and len(loops) > 1 and rng.random() < 0.3:
        h = ("sum", loops[0][0], loops[1][0])
    outs = ["c", "d"] + (["f"] if "f" in roots else [])
    return {"out": out or rng.choice(outs), "a": rng.choice(["a", "a", "b"]), "b": rng.choice(["b", "b", "a"]),
            "h": h}


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
        nest["sites"].append(gen_site(rng, loops, roots, out=other))
    return nest


def gen_spec(rng, index):
    shape = ["shared_out", "distinct_out", "same_triple", "fresh", "interleaved", "free"][index % 6]
    fresh = shape == "fresh" or rng.random() < 0.25
    roots = ["a", "b", "c", "d"] + (["f"] if fresh else [])
    count = rng.randint(2, 4)
    nests = [gen_nest(rng, k, roots, shape) for k in range(count)]
    first = nests[0]["sites"][0]
    second = nests[1]["sites"][0]
    if shape == "shared_out":      # Case A: one output root, other operands.
        second.update(out=first["out"], a=first["a"], b="d" if first["b"] != "d" else "c")
        if second["b"] == second["out"]:
            second["b"] = "a"
    elif shape == "distinct_out":  # Case B: another output root, the same operands.
        second.update(out="d" if first["out"] == "c" else "c", a=first["a"], b=first["b"])
    elif shape == "same_triple":   # One group over two nests.
        second.update(out=first["out"], a=first["a"], b=first["b"])
    elif shape == "fresh":
        second.update(out="f")
    lo = rng.choice([1, 1, 2])
    return {"name": f"s{index}", "fresh": fresh, "nests": nests, "scope": (lo, lo + rng.choice([1, 2])),
            "shape": shape}


def sites_of(spec):
    out = []
    for k, nest in enumerate(spec["nests"]):
        for site in nest["sites"]:
            out.append((k, site))
    return out


def print_function(spec):
    t = f"memref<{SIZE}xi32>"
    lines = [f"func.func @{spec['name']}(%a: {t}, %b: {t}, %c: {t}, %d: {t}, %n: index) {{",
             "  %c0 = arith.constant 0 : index", "  %c1 = arith.constant 1 : index",
             "  %c2 = arith.constant 2 : index", "  %c3 = arith.constant 3 : index"]
    if spec["fresh"]:
        lines += [f"  %f = memref.alloc() : {t}", "  %zero = arith.constant 0 : i32",
                  f"  %size = arith.constant {SIZE} : index",
                  "  scf.for %w = %c0 to %size step %c1 {", f"    memref.store %zero, %f[%w] : {t}", "  }"]
    count = [0]

    def name(base):
        count[0] += 1
        return f"%{base}{count[0]}"

    for k, nest in enumerate(spec["nests"]):
        indent, closing = "  ", 0
        if nest["when"] is not None:
            test = name("w")
            lines.append(f"{indent}{test} = arith.cmpi ult, %c{nest['when']}, %n : index")
            lines.append(f"{indent}scf.if {test} {{")
            indent += "  "
            closing += 1
        outer = None
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
                      f"{indent}{z} = memref.load %{site['out']}[{index}] : {t}",
                      f"{indent}{m} = arith.muli {x}, {y} : i32",
                      f"{indent}{s} = arith.addi {z}, {m} : i32",
                      f"{indent}memref.store {s}, %{site['out']}[{index}] : {t}"]
        for _ in range(closing):
            indent = indent[:-2]
            lines.append(f"{indent}}}")
    lines += ["  return", "}"]
    return "\n".join(lines) + "\n"


def interpret(spec, n):
    """The MAC events of one run, in order: site, nest, output root, index, environment."""
    events = []
    ids = {}
    for number, (k, site) in enumerate(sites_of(spec)):
        ids[id(site)] = number

    def index(h, env):
        if h[0] == "const":
            return h[1]
        if h[0] == "sum":
            return env[h[1]] + env[h[2]]
        return env[h[1]] + h[2]

    for k, nest in enumerate(spec["nests"]):
        if nest["when"] is not None and not nest["when"] < n:
            continue

        def go(level, env):
            if level == len(nest["loops"]):
                for site in nest["sites"]:
                    events.append({"site": ids[id(site)], "nest": k, "root": ROOTS[site["out"]],
                                   "h": index(site["h"], env), "env": dict(env)})
                return
            var, bound = nest["loops"][level]
            upper = n if bound[0] == "n" else bound[1] if bound[0] == "const" else env[nest["loops"][level - 1][0]] + 1
            for value in range(upper):
                env[var] = value
                go(level + 1, env)
            env.pop(var, None)

        go(0, {"n": n})
    return events


def model_groups(spec):
    """Groups by root triple, in the order of their first site."""
    groups, known = [], {}
    for number, (k, site) in enumerate(sites_of(spec)):
        key = (ROOTS[site["out"]], ROOTS[site["a"]], ROOTS[site["b"]])
        if key not in known:
            known[key] = len(groups)
            groups.append({"output": key[0], "first": key[1], "second": key[2], "sites": []})
        groups[known[key]]["sites"].append(number)
    return groups


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
    """Per segment: which spec variable, or constant, each state id holds at every one of its
    events, read from its group's table of states."""
    maps = []
    for seg in segments:
        sites = set(seg["sites"])
        g = group_of_site[seg["sites"][0]]
        candidates = {}
        for table in out["groups"][g].get("tables", []):
            if "events" not in table:
                continue
            n = table["parameters"][0]
            mine = [e for e in interpret(spec, n) if group_of_site.get(e["site"]) == g]
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


def theorem15(spec, n, segments, maps):
    """None when Theorem 15 holds over the union at n, or what fails."""
    where = {site: s for s, seg in enumerate(segments) for site in seg["sites"]}
    events = [e for e in interpret(spec, n) if e["site"] in where]
    order = [where[e["site"]] for e in events]
    for k in range(1, len(order)):
        if order[k] < order[k - 1]:
            return f"event {k} goes back from segment {order[k - 1]} to {order[k]}"
    for s, seg in enumerate(segments):
        mine = [e for e in events if where[e["site"]] == s]
        try:
            lam = [tuple(term(level["value"], state_values(maps[s], e["env"]), [n], []) for level in seg["levels"])
                   for e in mine]
        except KeyError:
            return f"segment {s}: a level reads a state the oracle cannot place"
        if lam != enumerate_set(seg, [n]):
            return f"segment {s}: its events are not its set in order"
    return None


def cells_meet(spec, n, segments):
    """Whether two segments write one cell of one root, and whether a cell of one root has
    two contributions, at n."""
    where = {site: s for s, seg in enumerate(segments) for site in seg["sites"]}
    writers, counts = {}, Counter()
    for e in interpret(spec, n):
        if e["site"] in where:
            cell = (e["root"], e["h"])
            writers.setdefault(cell, set()).add(where[e["site"]])
            counts[cell] += 1
    return any(len(w) > 1 for w in writers.values()), any(c > 1 for c in counts.values())


def expected_pairs(spec, segments, authorized):
    roots = sorted({ROOTS[sites_of(spec)[seg["sites"][0]][1]["out"]] for seg in segments})
    out = []
    for i, r in enumerate(roots):
        for s in roots[i + 1:]:
            if 4 in (r, s):
                out.append((r, s, "DISTINCT", "fresh allocation", False))
            elif authorized:
                out.append((r, s, "DISTINCT", "authorized", True))
            else:
                out.append((r, s, "MAY_ALIAS", "", True))
    return out


def check_claim(spec, out, claim, route, segments, union_sites, group_of_site, authorized, counts, label):
    where = f"{spec['name']} {label} {route}"
    status = claim["status"]
    if claim["authorizes_lowering"]:
        raise Failure(f"{where}: authorizes lowering")
    if status == "VERIFIED" and spec["shape"] == "interleaved" and label == "all":
        raise Failure(f"{where}: interleaved groups verified")
    if status == "UNKNOWN" or segments is None:
        counts[f"{route} UNKNOWN"] += 1
        return
    maps = segment_maps(spec, out, segments, group_of_site, where)
    low, high = spec["scope"]
    scope = list(range(low, high + 1))
    if status == "REFUTED":
        point = claim["refutation"].get("point")
        if not point:
            raise Failure(f"{where}: a refutation with no point")
        if theorem15(spec, point[0], segments, maps) is None:
            raise Failure(f"{where}: refuted at n={point[0]}, where Theorem 15 holds")
        counts[f"{route} REFUTED replayed"] += 1
        if spec["shape"] == "interleaved" and label == "all":
            counts["interleaved refuted"] += 1
        return
    # Verified: at each listed point (finite) or at every point (symbolic).
    listed = [p["parameters"][0] for p in claim["points"]] if route == "finite" else scope
    for n in listed:
        wrong = theorem15(spec, n, segments, maps)
        if wrong:
            raise Failure(f"{where} n={n}: Theorem 15 is false: {wrong}\n{print_function(spec)}")
        counts["points against Theorem 15"] += 1
    if route == "finite":
        if claim["tier"] == "SCOPE_ENUMERATED" and sorted(listed) != scope:
            raise Failure(f"{where}: the whole scope without every point")
        for p in claim["points"]:
            n = p["parameters"][0]
            expect = sum(1 for e in interpret(spec, n) if e["site"] in union_sites)
            if p["events"] != expect:
                raise Failure(f"{where} n={n}: {p['events']} events, the union has {expect}")
            empty = [s for s, seg in enumerate(segments)
                     if not any(e["site"] in seg["sites"] for e in interpret(spec, n))]
            if empty:
                counts["segments empty at a point"] += 1
    counts[f"{route} VERIFIED"] += 1
    # The cells, with their roots.
    pairs = [(p["first"], p["second"], p["relation"], p["basis"], p["assumption"]) for p in claim["root_pairs"]]
    expect_pairs = expected_pairs(spec, segments, authorized)
    if pairs != expect_pairs:
        raise Failure(f"{where}: root pairs {pairs}, the oracle's {expect_pairs}")
    apart = all(p[2] == "DISTINCT" for p in expect_pairs)
    shared_any = single_any = False
    for n in scope:
        shared, multiple = cells_meet(spec, n, segments)
        shared_any, single_any = shared_any or shared, single_any or multiple
    for name, fact, meets in (("shared", claim["shared_cells"], shared_any),
                              ("single", claim["single_contribution"], single_any)):
        if fact["status"] == "PROVEN":
            if meets or not apart:
                raise Failure(f"{where}: {name} proven, yet cells meet ({meets}) or roots may alias ({not apart})")
            counts[f"{name} proven checked"] += 1
            if expect_pairs:
                counts[f"{name} proven across roots apart"] += 1
        elif fact["status"] == "REFUTED":
            n = fact["point"][0]
            shared, multiple = cells_meet(spec, n, segments)
            if not (shared if name == "shared" else multiple):
                raise Failure(f"{where}: {name} refuted at n={n}, where no cell of one root shows it")
            counts[f"{name} refuted replayed"] += 1
        else:
            if not apart and not meets:
                counts[f"{name} unknown, roots may alias"] += 1


def check(spec, seed, driver, counts):
    groups = model_groups(spec)
    group_of = {site: g for g, group in enumerate(groups) for site in group["sites"]}
    sets = ["all", "all-reversed"] + [str(g) for g in range(len(groups))]
    every = [f"roots {i} and {j} are separate" for i in range(4) for j in range(i + 1, 4)]
    for authorized in (True, False):
        out = run_driver(driver, spec, every if authorized else [], sets)
        label = "authorized" if authorized else "unauthorized"
        if out.get("gate") != "OPEN":
            raise Failure(f"{spec['name']}: gate {out.get('gate')}: {out.get('reason')}")
        entries = out["group_sets"]
        if len(entries) != len(sets):
            raise Failure(f"{spec['name']}: {len(entries)} sets answered")
        # The listing changes nothing.
        if entries[0]["finite"] != entries[1]["finite"] or entries[0]["symbolic"] != entries[1]["symbolic"]:
            raise Failure(f"{spec['name']} {label}: the listing changes the claim")
        counts["listings compared"] += 1
        canonical = [{"output": g["output"], "first": g["first"], "second": g["second"], "sites": g["sites"]}
                     for g in groups]
        if entries[0]["finite"]["groups"] != canonical:
            raise Failure(f"{spec['name']}: groups {entries[0]['finite']['groups']}, the oracle's {canonical}")
        for k, entry in enumerate(entries):
            listed = list(range(len(groups))) if k == 0 else list(reversed(range(len(groups)))) if k == 1 else [k - 2]
            if entry["groups"] != listed:
                raise Failure(f"{spec['name']}: set {k} lists {entry['groups']}")
            union_sites = {s for g in listed for s in groups[g]["sites"]}
            segments = entry["request"]["segments"] if "request" in entry else None
            if segments is not None:
                covered = [s for seg in segments for s in seg["sites"]]
                if sorted(covered) != sorted(union_sites):
                    raise Failure(f"{spec['name']}: the segments are no partition of the union")
            for route in ("finite", "symbolic"):
                claim = entry[route]
                if segments is not None and claim["status"] != "UNKNOWN":
                    seg_groups = claim["segment_groups"]
                    canonical_listed = sorted(listed, key=lambda g: groups[g]["sites"][0])
                    for s, seg in enumerate(segments):
                        owners = {group_of[site] for site in seg["sites"]}
                        if len(owners) != 1 or canonical_listed[seg_groups[s]] not in owners:
                            raise Failure(f"{spec['name']}: segment {s} is not inside the group named")
                check_claim(spec, out, claim, route, segments, union_sites, group_of, authorized, counts,
                            "all" if k < 2 else f"group {k - 2}")
            # A set of one group is the group's own claim.
            if k >= 2:
                candidates = out["groups"][k - 2]["candidates"]
                if candidates and "request" in candidates[0]:
                    for route, own in (("finite", "domain_finite"), ("symbolic", "domain_symbolic")):
                        a, b = entry[route], candidates[0][own]
                        if (a["status"], a["tier"], a["reason"]) != (b["status"], b["tier"], b["reason"]):
                            raise Failure(f"{spec['name']} group {k - 2} {route}: the set says {a['status']} "
                                          f"({a['reason']}), the group {b['status']} ({b['reason']})")
                    counts["one-group sets matched"] += 1
        counts[f"programs run {label}"] += 1


MINIMUM = {
    "finite VERIFIED": 40, "symbolic VERIFIED": 40, "points against Theorem 15": 200,
    "listings compared": 20, "one-group sets matched": 40, "interleaved refuted": 3,
    "shared refuted replayed": 5, "shared proven checked": 10, "shared proven across roots apart": 5,
    "shared unknown, roots may alias": 5, "single refuted replayed": 5, "segments empty at a point": 3,
}


def main():
    global MEMORY
    parser = argparse.ArgumentParser()
    parser.add_argument("--driver", required=True)
    parser.add_argument("--programs", type=int, default=30)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--jobs", type=int, default=2)
    parser.add_argument("--no-minimum", action="store_true")
    parser.add_argument("--memory-limit-gb", type=int, default=MEMORY >> 30,
                        help="address-space limit per driver run; 0 for none (sanitizer builds)")
    args = parser.parse_args()
    MEMORY = args.memory_limit_gb << 30
    rng = random.Random(args.seed)
    specs = [gen_spec(rng, i) for i in range(args.programs)]
    counts, failures = Counter(), []

    def one(spec):
        local = Counter()
        try:
            check(spec, 0, args.driver, local)
        except Failure as failure:
            return str(failure), local
        return None, local

    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        for failure, local in pool.map(one, specs):
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
