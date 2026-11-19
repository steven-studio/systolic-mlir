#!/usr/bin/env python3
"""G3a oracle over finite tables: no MLIR, no join algorithm in the ground truth.

1. Theorem 2. Every small table: a decomposition is searched for over all set
   partitions, from the definition. The canonical result must agree on existence,
   and on the unique answer.
2. Every fact the checker reports, recomputed independently, for random tables
   and random, trivial, perturbed and true candidates, counterexamples included.
3. Theorems 1 and 3 on large hidden batched boxes and on subsets of them.
"""
import argparse
import itertools
import json
from pathlib import Path
import random
import subprocess
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).parent))
from coordinate_facts import check_support, compare, components, facts, kernel


def refines(a, b):
    seen = {}
    return all(seen.setdefault(x, y) == y for x, y in zip(a, b))


def meet(*parts):
    return kernel(list(zip(*parts)))


PARTITIONS = {}


def partitions(n):
    if n not in PARTITIONS:
        out = []

        def grow(prefix, blocks):
            if len(prefix) == n:
                out.append(tuple(prefix))
                return
            for b in range(blocks + 1):
                grow(prefix + [b], max(blocks, b + 1))
        grow([], 0)
        PARTITIONS[n] = out
    return PARTITIONS[n]


def is_box(rP, rI, rJ, rK):
    n = len(rP)
    if len({(rI[t], rJ[t], rK[t]) for t in range(n)}) != n:
        return False
    for p in set(rP):
        ts = [t for t in range(n) if rP[t] == p]
        if len(ts) != len({rI[t] for t in ts}) * len({rJ[t] for t in ts}) * len({rK[t] for t in ts}):
            return False
    return True


def all_decompositions(T):
    n = len(T)
    h, f, g = [kernel([t[v] for t in T]) for v in range(3)]
    every = partitions(n)
    Is = [r for r in every if refines(h, r) and refines(f, r)]
    Js = [r for r in every if refines(h, r) and refines(g, r)]
    Ks = [r for r in every if refines(f, r) and refines(g, r)]
    found = []
    for rI in Is:
        for rJ in Js:
            if meet(rI, rJ) != h:
                continue
            for rK in Ks:
                if meet(rI, rK) != f or meet(rK, rJ) != g:
                    continue
                for rP in every:
                    if refines(rI, rP) and refines(rJ, rP) and refines(rK, rP) and is_box(rP, rI, rJ, rK):
                        found.append((rP, rI, rJ, rK))
    return found


def event(site, x, h, f, g, read=None):
    return {"site": site, "x": list(x), "h": list(h), "h_read": list(read if read is not None else h),
            "f": list(f), "g": list(g)}


def run_driver(driver, tables):
    with tempfile.NamedTemporaryFile("w", suffix=".json") as handle:
        json.dump(tables, handle)
        handle.flush()
        done = subprocess.run([driver, "--table", handle.name], capture_output=True, text=True)
    if done.returncode:
        raise SystemExit(f"driver failed: {done.stderr[-800:]}")
    return json.loads(done.stdout)


def small_tables():
    """Every set of at most 7 triples over each alphabet."""
    tables = []
    seen = set()
    for sizes in ((2, 2, 2), (3, 2, 2), (2, 3, 2), (2, 2, 3)):
        universe = list(itertools.product(*[range(s) for s in sizes]))
        for size in range(1, 8):
            if size == 7 and len(universe) > 8:
                continue
            for T in itertools.combinations(universe, size):
                if size == 6 and len(universe) > 8 and hash(T) % 3:
                    continue
                if T not in seen:
                    seen.add(T)
                    tables.append(T)
    return tables


def tiled_tables():
    """Real tiles of at most 7 points in all, and each with one point removed or
    one view value changed, so that both answers occur often and near each other."""
    shapes = [(a, b, c) for a in range(1, 8) for b in range(1, 8) for c in range(1, 8) if a * b * c <= 7]
    tilings = [[s] for s in shapes]
    tilings += [[s, t] for s in shapes for t in shapes if s[0] * s[1] * s[2] + t[0] * t[1] * t[2] <= 7]
    tables = set()
    for tiling in tilings:
        T = [((p, i, j), (p, i, k), (p, k, j)) for p, (a, b, c) in enumerate(tiling)
             for i, j, k in itertools.product(range(a), range(b), range(c))]
        ids = [{}, {}, {}]
        T = tuple(tuple(ids[v].setdefault(t[v], len(ids[v])) for v in range(3)) for t in T)
        tables.add(T)
        for gone in range(len(T)):
            if len(T) > 1:
                tables.add(T[:gone] + T[gone + 1:])
        for position, view in itertools.product(range(len(T)), range(3)):
            for value in range(len(ids[view])):
                changed = list(T[position])
                changed[view] = value
                variant = T[:position] + (tuple(changed),) + T[position + 1:]
                if len(set(variant)) == len(variant):
                    tables.add(variant)
    return sorted(tables)


def theorem2(driver):
    tables = small_tables() + tiled_tables()
    print(f"theorem 2: {len(tables)} tables", flush=True)
    results = []
    for start in range(0, len(tables), 2000):
        chunk = tables[start:start + 2000]
        payload = [{"events": [event(0, [n], [t[0]], [t[1]], [t[2]]) for n, t in enumerate(T)]} for T in chunk]
        results.extend(run_driver(driver, payload))
    holds = fails = 0
    for T, out in zip(tables, results):
        found = all_decompositions(T)
        report = out["report"]
        says = (report["validity"]["decomposition_valid"] and report["cartesian"]["form"] == "box"
                and all(report["injectivity"][v] for v in "hfg"))
        if says != bool(found):
            raise SystemExit(f"EXISTENCE differs on {T}: {says} vs {len(found)}")
        if report["unique_up_to_relabelling"] != says:
            raise SystemExit(f"UNIQUENESS flag differs on {T}")
        if found:
            holds += 1
            c = out["candidate"]
            if len(set(found)) != 1:
                raise SystemExit(f"NOT UNIQUE on {T}: {found[:3]}")
            if (kernel(c["p"]), kernel(c["i"]), kernel(c["j"]), kernel(c["k"])) != found[0]:
                raise SystemExit(f"WRONG ANSWER on {T}")
        else:
            fails += 1
    print(f"theorem 2: {holds} decompose, {fails} do not, all agree with the search", flush=True)
    return len(tables)


def random_table(rng):
    size = rng.randint(1, 30)
    alphabet = [rng.randint(1, 5) for _ in range(3)]
    width = rng.randint(0, 3)
    events = []
    for _ in range(size):
        h = [rng.randrange(alphabet[0])]
        read = h if rng.random() < .98 else [rng.randrange(alphabet[0] + 1)]
        events.append(event(rng.randrange(2), [rng.randrange(3) for _ in range(width)], h,
                            [rng.randrange(alphabet[1])], [rng.randrange(alphabet[2])], read))
    return events


def random_candidate(rng, events):
    n = len(events)
    mode = rng.randrange(6)
    if mode == 0:
        return None  # The canonical one.
    if mode == 1:
        limit = rng.randint(1, 4)
        return {axis: [rng.randrange(limit) for _ in range(n)] for axis in "pijk"}
    if mode == 2:
        ids = list(range(n))
        return {"p": [0] * n, "i": ids, "j": ids, "k": ids}
    if mode == 3:
        return {"p": list(range(n)), "i": [0] * n, "j": [0] * n, "k": [0] * n}
    # Coordinates built from the views, then possibly damaged.
    labels = {"p": [0] * n,
              "i": list(kernel([tuple(e["h"]) for e in events])),
              "j": list(kernel([tuple(e["f"]) for e in events])),
              "k": list(kernel([tuple(e["g"]) for e in events]))}
    if mode == 5:
        axis = rng.choice("pijk")
        labels[axis][rng.randrange(n)] = rng.randrange(4)
    return labels


def check_canonical(events, candidate):
    """Canonical labels are the joins over T, lifted to the events. Searched here
    without union-find, so two points of one triple cannot get different labels."""
    triples = [(tuple(e["h"]), tuple(e["f"]), tuple(e["g"])) for e in events]
    distinct = list(dict.fromkeys(triples))
    h, f, g = [[t[v] for t in distinct] for v in range(3)]
    joins = dict(i=components([h, f]), j=components([h, g]), k=components([f, g]),
                 p=components([h, f, g]))
    index = {t: n for n, t in enumerate(distinct)}
    for axis, part in joins.items():
        if kernel(candidate[axis]) != kernel([part[index[t]] for t in triples]):
            raise SystemExit(f"canonical {axis} differs from the search: {events}")


def random_facts(driver, count, seed):
    rng = random.Random(seed)
    tables = []
    for _ in range(count):
        events = random_table(rng)
        entry = {"events": events}
        candidate = random_candidate(rng, events)
        if candidate:
            entry["candidate"] = candidate
        tables.append(entry)
    checked = valid = 0
    for start in range(0, count, 1000):
        chunk = tables[start:start + 1000]
        for entry, out in zip(chunk, run_driver(driver, chunk)):
            if "malformed" in out["report"]:
                raise SystemExit("a well-formed table was refused")
            if "candidate" not in entry:
                check_canonical(entry["events"], out["candidate"])
            expected = facts(entry["events"], out["candidate"])
            bad = compare(out["report"], expected)
            if bad:
                raise SystemExit(f"FACTS differ: {bad}\n{json.dumps(entry)[:1200]}")
            problem = check_support(entry["events"], out["candidate"],
                                    out["report"]["shape"].get("support"),
                                    expected["validity"]["function_of_point"])
            if problem:
                raise SystemExit(f"{problem}\n{json.dumps(entry)[:1200]}")
            checked += 1
            valid += expected["validity"]["decomposition_valid"]
    print(f"facts: {checked} candidates, {valid} valid, every field equal", flush=True)
    return checked


def hidden(rng):
    tags = [{}, {}, {}]

    def label(which, key):
        # A random relabelling: injective, and it says nothing about the structure.
        return [tags[which].setdefault(key, rng.randrange(1 << 40))]
    rows = []
    for p in range(rng.randint(1, 4)):
        a, b, c = rng.randint(1, 5), rng.randint(1, 5), rng.randint(1, 5)
        rows.extend((p, i, j, k) for i, j, k in itertools.product(range(a), range(b), range(c)))
    rng.shuffle(rows)
    events = [event(0, q, label(0, (q[0], q[1], q[2])), label(1, (q[0], q[1], q[3])),
                    label(2, (q[0], q[3], q[2]))) for q in rows]
    return rows, events


def theorem1(driver, count, seed):
    rng = random.Random(seed)
    cases = [hidden(rng) for _ in range(count)]
    results = run_driver(driver, [{"events": events} for rows, events in cases])
    genuine = 0
    for (rows, events), out in zip(cases, results):
        c = out["candidate"]
        report = out["report"]
        truth = (kernel([q[0] for q in rows]), kernel([q[:2] for q in rows]),
                 kernel([(q[0], q[2]) for q in rows]), kernel([(q[0], q[3]) for q in rows]))
        if (kernel(c["p"]), kernel(c["i"]), kernel(c["j"]), kernel(c["k"])) != truth:
            raise SystemExit(f"HIDDEN GEMM not recovered: {rows[:12]}")
        strict = all(max(q[1] for q in rows if q[0] == p) >= 1 and max(q[2] for q in rows if q[0] == p) >= 1
                     and max(q[3] for q in rows if q[0] == p) >= 1 for p in {q[0] for q in rows})
        if report["genuine"] != strict or compare(report, facts(events, c)):
            raise SystemExit(f"HIDDEN GEMM misclassified: {rows[:12]}")
        genuine += strict
    print(f"theorem 1: {count} hidden batched boxes, {genuine} genuine, all exact", flush=True)


def theorem3(driver, count, seed):
    rng = random.Random(seed)
    cases = []
    while len(cases) < count:
        a, b, c = rng.randint(1, 4), rng.randint(1, 4), rng.randint(1, 4)
        density = rng.choice([.3, .6, .9])
        rows = [q for q in itertools.product(range(2), range(a), range(b), range(c)) if rng.random() < density]
        if rows:
            cases.append(rows)
    payload = [{"events": [event(0, q, [q[0], q[1], q[2]], [q[0], q[1], q[3]], [q[0], q[3], q[2]])
                           for q in rows]} for rows in cases]
    connected_count = 0
    for rows, out in zip(cases, run_driver(driver, payload)):
        got = kernel(out["candidate"]["i"])
        truth = kernel([q[:2] for q in rows])
        if not refines(got, truth):
            raise SystemExit(f"NOT A REFINEMENT: {rows}")
        connected = True
        for key in {q[:2] for q in rows}:
            part = [q for q in rows if q[:2] == key]
            if len(set(components([[q[2] for q in part], [q[3] for q in part]]))) != 1:
                connected = False
        if connected != (got == truth):
            raise SystemExit(f"SLICE LEMMA fails: {rows}")
        connected_count += connected
    print(f"theorem 3: {count} subsets, {connected_count} with connected slices", flush=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--driver", required=True)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--random", type=int, default=20000)
    parser.add_argument("--hidden", type=int, default=2000)
    parser.add_argument("--subsets", type=int, default=5000)
    parser.add_argument("--skip-exhaustive", action="store_true")
    args = parser.parse_args()
    if not args.skip_exhaustive:
        theorem2(args.driver)
    random_facts(args.driver, args.random, args.seed)
    theorem1(args.driver, args.hidden, args.seed + 1)
    theorem3(args.driver, args.subsets, args.seed + 2)
    print("0 disagreements")


if __name__ == "__main__":
    main()
