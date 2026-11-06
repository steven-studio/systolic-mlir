#!/usr/bin/env python3
"""G5a oracle. A spec is interpreted by plain Python loops and printed as MLIR for the
driver, and every claim is recomputed here with no library code:

- Theorem 12, at EVERY point of the scope, including points the driver never ran: the
  levels' values at the events, in order, equal S(v) enumerated from the bounds and the
  guard, in order, with each level's direction and signedness.
- Multiplicity: a constant box μ against the counts of each logical point.
- The cell key, collisions and single contributions, from the events.
- Theorem 13, with random fills: each cell's final value is the fold of its
  contributions, from the initial operands, in the order of the key.
- Reorder facts: a proven integer fact against random permutations; every witness
  replayed.
- A refuted domain claim: the events and the set differ at its point.
- Multiplicity (Theorem 14): a spec may name repeat loops. It then runs again with the
  family of its other loops given to the driver, and the emitted μ term is evaluated here
  at every logical point of the nest, including the points with no event, against the
  count of events. A perturbed term must disagree, or this check is vacuous.

Every driver run has a time limit, and a timeout fails the run.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import math
import random
import struct
import subprocess
import sys
import tempfile

TIMEOUT = 600
WIDTH = 16
EXTENT = 8
SOLVER = ["--timeout-ms=3000", "--rlimit=2000000"]


class Stop(Exception):
    pass


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
    """A term of the driver at its own width, with SMT-LIB total division."""
    w = t["width"]
    if "const" in t:
        # A constant past 64 bits comes as decimal text.
        return wrap(int(t["const"]), w)
    if "state" in t:
        return wrap(state[t["state"]], w)
    if "param" in t:
        return wrap(params[t["param"]], w)
    if "level" in t:
        return wrap(levels[t["level"]], w)
    op = t["op"]
    args = t["args"]
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
    if op == "and": return v[0] & v[1]
    if op == "or": return v[0] | v[1]
    if op == "xor": return v[0] ^ v[1]
    if op == "not": return wrap(~v[0], w)
    if op == "icmp": return int(PREDICATES[t["predicate"]](v[0], v[1], inner))
    if op == "zext": return v[0]
    if op == "sext": return wrap(signed(v[0], inner), w)
    if op == "trunc": return wrap(v[0], w)
    raise SystemExit(f"the oracle does not know {op}")


# Specs. Index expressions: int, a name, or (op, a, b) with op in + - * %.
def expr(e, env):
    if isinstance(e, int):
        return e
    if isinstance(e, str):
        return env[e]
    a, b = expr(e[1], env), expr(e[2], env)
    return {"+": a + b, "-": a - b, "*": a * b, "%": a % b if b else 0}[e[0]]


def condition(c, env):
    if c is None:
        return True
    kind = c[0]
    if kind == "<":
        return expr(c[1], env) < expr(c[2], env)
    if kind == "==":
        return expr(c[1], env) == expr(c[2], env)
    if kind == "bit":
        return (c[1] >> expr(c[2], env)) & 1 == 1
    raise SystemExit(kind)


def interpret(spec, n, memory=None):
    """Events in order. Each: env, cell, operand cells, and with memory, values."""
    events = []
    mem = None if memory is None else {r: dict(cells) for r, cells in memory.items()}
    element = spec["element"]

    def site(env, which):
        h = tuple(expr(e, env) for e in spec["h"])
        f = tuple(expr(e, env) for e in spec["f" if which == 0 else "f2"])
        g = tuple(expr(e, env) for e in spec["g"])
        for idx in (h, f, g):
            if not all(0 <= x < EXTENT for x in idx):
                raise Stop("out of bounds")
        event = dict(env=dict(env), h=h, f=f, g=g, site=which)
        if mem is not None:
            a, b, z = mem["a"][f], mem["b"][g], mem["c"][h]
            if element == "f64":
                m = a * b
                u = z + m
                if math.isnan(m) or math.isnan(u):
                    raise Stop("NaN")
            else:
                m = wrap(a * b, 32)
                u = wrap(z + m, 32)
                if spec.get("nsw") and signed(z, 32) + signed(m, 32) != signed(u, 32):
                    raise Stop("poison")
            mem["c"][h] = u
            event.update(a=a, b=b, update=u)
        events.append(event)

    def go(depth, env):
        if depth == len(spec["loops"]):
            for name, e in spec.get("derived", []):
                env[name] = expr(e, env)
            if spec.get("piecewise"):
                site(env, 0 if condition(spec["guard"], env) else 1)
            elif condition(spec.get("guard"), env):
                site(env, 0)
            return
        name, low, high, step = spec["loops"][depth]
        for v in range(expr(low, env), expr(high, env), step):
            env[name] = v
            go(depth + 1, env)
        env.pop(name, None)

    go(0, {"n": n})
    return events, mem


class Emitter:
    def __init__(self):
        self.lines, self.count, self.consts = [], 0, set()

    def value(self, e, names):
        if isinstance(e, int):
            self.consts.add(e)
            return f"%c{e}" if e >= 0 else f"%cm{-e}"
        if isinstance(e, str):
            return names[e]
        a, b = self.value(e[1], names), self.value(e[2], names)
        self.count += 1
        name = f"%t{self.count}"
        op = {"+": "addi", "-": "subi", "*": "muli", "%": "remui"}[e[0]]
        self.lines.append(f"{name} = arith.{op} {a}, {b} : index")
        return name


def print_function(spec):
    element = spec["element"]
    t = f"memref<{EXTENT}x{EXTENT}x{element}>"
    em = Emitter()
    names = {"n": "%n"}
    body = []
    closing = 0
    for name, low, high, step in spec["loops"]:
        lo, hi = em.value(low, names), em.value(high, names)
        em.consts.add(step)
        body.extend(em.lines)
        em.lines = []
        body.append(f"scf.for %{name} = {lo} to {hi} step %c{step} {{")
        names[name] = "%" + name
        closing += 1
    for name, e in spec.get("derived", []):
        v = em.value(e, names)
        body.extend(em.lines)
        em.lines = []
        em.count += 1
        body.append(f"%d{em.count} = arith.addi {v}, %c0 : index")
        em.consts.add(0)
        names[name] = f"%d{em.count}"

    def mac(view, suffix):
        idx = lambda es: ", ".join(em.value(e, names) for e in es)
        h, f, g = idx(spec["h"]), idx(spec[view]), idx(spec["g"])
        body.extend(em.lines)
        em.lines = []
        add, mul = ("arith.addf", "arith.mulf") if element == "f64" else ("arith.addi", "arith.muli")
        flags = " overflow<nsw>" if spec.get("nsw") else ""
        s = suffix
        body.extend([f"%x{s} = memref.load %a[{f}] : {t}", f"%y{s} = memref.load %b[{g}] : {t}",
                     f"%z{s} = memref.load %c[{h}] : {t}", f"%m{s} = {mul} %x{s}, %y{s} : {element}",
                     f"%s{s} = {add} %z{s}, %m{s}{flags} : {element}", f"memref.store %s{s}, %c[{h}] : {t}"])

    guard = spec.get("guard")
    if guard:
        if guard[0] == "bit":
            mask = em.value(guard[1], names)
            pos = em.value(guard[2], names)
            em.consts.add(1)
            em.count += 1
            sh, one, test = f"%t{em.count}a", f"%t{em.count}b", f"%t{em.count}c"
            em.lines += [f"{sh} = arith.shrui {mask}, {pos} : index", f"{one} = arith.andi {sh}, %c1 : index",
                         f"{test} = arith.cmpi eq, {one}, %c1 : index"]
        else:
            a, b = em.value(guard[1], names), em.value(guard[2], names)
            em.count += 1
            test = f"%t{em.count}c"
            em.lines.append(f"{test} = arith.cmpi {'ult' if guard[0] == '<' else 'eq'}, {a}, {b} : index")
        body.extend(em.lines)
        em.lines = []
        body.append(f"scf.if {test} {{")
        mac("f", "1")
        if spec.get("piecewise"):
            body.append("} else {")
            mac("f2", "2")
        body.append("}")
    else:
        mac("f", "1")
    body += ["}"] * closing
    em.consts |= {0, 1}
    head = [f"%c{v} = arith.constant {v} : index" if v >= 0 else f"%cm{-v} = arith.constant {v} : index"
            for v in sorted(em.consts)]
    args = ", ".join(f"%{r}: {t}" for r in "abc") + ", %n: index"
    return f"func.func @{spec['name']}({args}) {{\n" + "\n".join(head + body) + "\nreturn\n}\n"


AUTH = "roots 0 and 1 are separate,roots 0 and 2 are separate,roots 1 and 2 are separate"


def run_driver(driver, spec, family=None):
    with tempfile.NamedTemporaryFile("w", suffix=".mlir") as handle:
        handle.write(print_function(spec))
        handle.flush()
        low, high = spec["scope"]
        command = [driver, handle.name, "--lower-structured", f"--index-bit-width={spec.get('width', WIDTH)}",
                   f"--scope=0:{low}:{high}", "--max-level=1", "--candidates-per-group=1",
                   "--print-events", f"--authorize={AUTH}"] + SOLVER
        if family is not None:
            command.append("--family=" + family)
        done = subprocess.run(command, capture_output=True, text=True, timeout=TIMEOUT)
    if done.returncode:
        raise SystemExit(f"driver failed on {spec['name']}:\n{done.stderr[-1500:]}")
    return json.loads(done.stdout)[0]


def state_map(spec, tables):
    """Which spec value each state variable holds, read from the driver's tables."""
    candidates = None
    for table in tables:
        if "events" not in table:
            continue
        n = table["parameters"][0]
        ours, _ = interpret(spec, n)
        if len(ours) != len(table["events"]):
            raise SystemExit(f"{spec['name']} n={n}: {len(ours)} events here, {len(table['events'])} there")
        for mine, theirs in zip(ours, table["events"]):
            values = theirs["x"][1:]  # The first entry is the site.
            env = dict(mine["env"])
            for id_, value in zip(table["names"], values):
                options = {k for k, v in env.items() if wrap(v, spec.get("width", WIDTH)) == value} | (
                    {("const", value)})
                if candidates is None:
                    candidates = {}
                candidates[id_] = candidates.get(id_, options) & options
    return {id_: sorted(opts, key=str)[0] for id_, opts in (candidates or {}).items() if opts}


def state_values(mapping, env):
    out = {}
    for id_, source in mapping.items():
        out[id_] = source[1] if isinstance(source, tuple) else env[source]
    return out


def enumerate_set(request, params, limit=1 << 18):
    levels = request["levels"]
    out = []

    def less(level, a, b):
        w = level["value"]["width"]
        x, y = (signed(a, w), signed(b, w)) if level["signed"] else (a, b)
        return x < y

    def go(m, current):
        if len(out) > limit:
            raise SystemExit("the box is too large for the oracle")
        if m == len(levels):
            if request["guard"] is None or term(request["guard"], {}, params, current):
                out.append(tuple(current))
            return
        level = levels[m]
        w = level["value"]["width"]
        lo, hi = term(level["lower"], {}, params, current), term(level["upper"], {}, params, current)
        if not less(level, lo, hi):
            return
        span = wrap(hi - lo, w)
        order = range(span - 1, -1, -1) if level["descending"] else range(span)
        for k in order:
            go(m + 1, current + [wrap(lo + k, w)])

    go(0, [])
    return out


def lambdas(spec, request, mapping, n):
    events, _ = interpret(spec, n)
    out = []
    for e in events:
        st = state_values(mapping, e["env"])
        out.append(tuple(term(level["value"], st, [n], []) for level in request["levels"]))
    return events, out


def check_domain(spec, entry, n, tally, mapping):
    request = entry["request"]
    events, lam = lambdas(spec, request, mapping, n)
    expected = enumerate_set(request, [n])
    if lam != expected:
        raise SystemExit(f"{spec['name']} n={n}: Theorem 12 is false: events {lam[:6]} set {expected[:6]}\n"
                         f"{print_function(spec)}")
    tally["points checked"] += 1
    return events, lam


def check_facts(spec, entry, domain, n, events, lam, mapping, tally):
    request = entry["request"]
    key = request["cell_key"]
    cells = {}
    for e, l in zip(events, lam):
        cells.setdefault(e["h"], []).append((l, e))
    if domain["cell_key"]["status"] == "PROVEN":
        for members in cells.values():
            for (a, _), (b, _) in zip(members, members[1:]):
                ka, kb = [a[k] for k in key], [b[k] for k in key]
                levels = [request["levels"][k] for k in key]
                if not less_key(levels, ka, kb):
                    raise SystemExit(f"{spec['name']} n={n}: the cell key is false")
        tally["keys checked"] += 1
    logical = lambda e: tuple(term(t, state_values(mapping, e["env"]), [n], [])
                              for axis in "pij" for t in entry["axes"][axis])
    if domain["output_injective"]["status"] == "PROVEN":
        for members in cells.values():
            if len({logical(e) for _, e in members}) > 1:
                raise SystemExit(f"{spec['name']} n={n}: C1 is false")
    if domain["single_contribution"]["status"] == "PROVEN":
        if any(len(m) > 1 for m in cells.values()):
            raise SystemExit(f"{spec['name']} n={n}: T1 is false")
    if domain["multiplicity"] in ("CONSTANT_BOX", "LOGICAL_BOX") and domain["multiplicity_term"]:
        check_multiplicity(spec, entry, domain, n, events, lam, mapping, tally)


def mentions(e, names):
    if isinstance(e, str):
        return e in names
    if isinstance(e, (tuple, list)):
        return any(mentions(x, names) for x in e)
    return False


def logical_visits(spec, n):
    """The logical points L(v): the nest with every repeat loop run once, at its first
    value, under the spec's own logical guard. Theorem 14 says nothing of a point outside
    L(v), so a guard that excludes one keeps it out here too. None when the guard reads a
    repeat variable: no box exists then, and nothing is checked."""
    repeats = spec.get("repeats", [])
    guard = spec.get("guard")
    if guard is not None and mentions(guard, repeats):
        return None
    once = dict(spec)
    once["loops"] = [(name, low, (("+", low, 1) if name in repeats else high), step)
                     for name, low, high, step in spec["loops"]]
    # A piecewise nest fires on both sides of its guard, so every point is visited.
    if spec.get("piecewise"):
        once["guard"] = None
        once["piecewise"] = False
    try:
        events, _ = interpret(once, n)
    except Stop:
        return []
    return [e["env"] for e in events]


def check_multiplicity(spec, entry, domain, n, events, lam, mapping, tally):
    """Theorem 14: the emitted μ term, evaluated here at the levels of each logical point,
    equals the number of events of that point, and is 0 where the point has none."""
    request = entry["request"]
    mu_term = domain["multiplicity_term"]
    logical = lambda env: tuple(term(t, state_values(mapping, env), [n], [])
                                for axis in "pijk" for t in entry["axes"][axis])
    levels_of = lambda env: [term(level["value"], state_values(mapping, env), [n], [])
                             for level in request["levels"]]
    counts, mus = {}, {}
    for e, l in zip(events, lam):
        k = logical(e["env"])
        counts[k] = counts.get(k, 0) + 1
        mu = term(mu_term, {}, [n], list(l))
        if k in mus and mus[k] != mu:
            raise SystemExit(f"{spec['name']} n={n}: μ differs within a logical point")
        mus[k] = mu
    for k, c in counts.items():
        if c != mus[k]:
            raise SystemExit(f"{spec['name']} n={n}: μ = {mus[k]} is false at {k}: {c} events")
    # Points of the nest with no event at all: μ must be 0 there, which the driver's
    # finite route never looked at.
    visits = logical_visits(spec, n) if spec.get("repeats") else None
    if visits is not None:
        zero = 0
        for env in visits:
            k = logical(env)
            if k in counts:
                continue
            try:
                mu = term(mu_term, {}, [n], levels_of(env))
            except KeyError:
                continue
            if mu != 0:
                raise SystemExit(f"{spec['name']} n={n}: μ = {mu} at {k}, which has no event")
            zero += 1
        tally["zero multiplicity points"] += zero
    # The check is no tautology: a term off by one must disagree somewhere.
    perturbed = {"width": mu_term["width"], "op": "add",
                 "args": [mu_term, {"width": mu_term["width"], "const": 1}]}
    if events and all(term(perturbed, {}, [n], list(l)) == counts[logical(e["env"])]
                      for e, l in zip(events, lam)):
        raise SystemExit(f"{spec['name']}: the multiplicity check cannot fail")
    tally["multiplicities checked"] += 1
    tally["multiplicity " + domain["multiplicity"]] += 1


def less_key(levels, a, b):
    for level, x, y in zip(levels, a, b):
        if x == y:
            continue
        w = level["value"]["width"]
        before = signed(x, w) < signed(y, w) if level["signed"] else x < y
        return not before if level["descending"] else before
    return False


def bits(value, element):
    return struct.unpack("<Q", struct.pack("<d", value))[0] if element == "f64" else value


POOL = [0.0, -0.0, 1.0, -1.0, 0.5, 3.0, 1e16, -1e16, 1e-3, 9007199254740992.0]


def fill(spec, rng):
    memory = {}
    for root in "abc":
        cells = {}
        for r in range(EXTENT):
            for c in range(EXTENT):
                if spec["element"] == "f64":
                    cells[(r, c)] = rng.choice(POOL) if rng.random() < 0.7 else rng.randrange(-64, 64) / 8.0
                else:
                    cells[(r, c)] = rng.getrandbits(32) if rng.random() < 0.3 else rng.randrange(8)
        memory[root] = cells
    return memory


def fold(spec, start, products, reverse=False):
    value = start
    seq = list(reversed(products)) if reverse else products
    for p in seq:
        if spec["element"] == "f64":
            value = value + p
        else:
            nxt = wrap(value + p, 32)
            if spec.get("nsw") and signed(value, 32) + signed(p, 32) != signed(nxt, 32):
                raise Stop("poison")
            value = nxt
    return value


def check_theorem13(spec, entry, n, mapping, rng, fills, tally):
    request = entry["request"]
    key = request["cell_key"]
    levels = [request["levels"][k] for k in key]
    for _ in range(fills):
        memory = fill(spec, rng)
        try:
            events, final = interpret(spec, n, memory)
        except Stop:
            tally["stopped runs"] += 1
            continue
        _, lam = lambdas(spec, request, mapping, n)
        cells = {}
        for e, l in zip(events, lam):
            cells.setdefault(e["h"], []).append(([l[k] for k in key], e))
        for cell, members in cells.items():
            import functools
            ordered = sorted(members, key=functools.cmp_to_key(
                lambda x, y: -1 if less_key(levels, x[0], y[0]) else (1 if less_key(levels, y[0], x[0]) else 0)))
            products = []
            for _, e in ordered:
                a, b = memory["a"][e["f"]], memory["b"][e["g"]]
                products.append(a * b if spec["element"] == "f64" else wrap(a * b, 32))
            expected = fold(spec, memory["c"][cell], products)
            if bits(expected, spec["element"]) != bits(final["c"][cell], spec["element"]):
                raise SystemExit(f"{spec['name']} n={n}: Theorem 13 is false at cell {cell}")
        tally["Theorem 13 runs"] += 1
        if entry["reorder"]["integer"]["status"] == "PROVEN":
            for cell, members in cells.items():
                products = [wrap(memory["a"][e["f"]] * memory["b"][e["g"]], 32) for _, e in members]
                rng.shuffle(products)
                if fold(spec, memory["c"][cell], products) != final["c"][cell]:
                    raise SystemExit(f"{spec['name']} n={n}: the integer reorder fact is false")
            tally["permutations checked"] += 1


def check_witness(spec, fact, tally):
    n = fact["point"][0]
    memory = {}
    for r, cells in zip("abc", fact["memory"]):
        values = {}
        for c, text in enumerate(cells):
            raw = int(text, 16)
            values[(c // EXTENT, c % EXTENT)] = (struct.unpack("<d", struct.pack("<Q", raw))[0]
                                                 if spec["element"] == "f64" else raw)
        memory[r] = values
    try:
        events, final = interpret(spec, n, memory)
    except Stop as stop:
        raise SystemExit(f"{spec['name']}: a reorder witness does not complete here: {stop}")
    cells = {}
    for e in events:
        cells.setdefault(e["h"], []).append(e)
    for cell, members in cells.items():
        products = [(e["a"] * e["b"]) if spec["element"] == "f64" else wrap(e["a"] * e["b"], 32) for e in members]
        if len(products) < 2:
            continue
        try:
            backward = fold(spec, memory["c"][cell], products, reverse=True)
        except Stop:
            tally["witnesses replayed"] += 1
            return
        if bits(backward, spec["element"]) != bits(final["c"][cell], spec["element"]):
            tally["witnesses replayed"] += 1
            return
    raise SystemExit(f"{spec['name']}: a reorder witness does not replay")


def family_text(spec, mapping):
    """The spec's non-repeat loop variables as a driver family, by their state ids."""
    by_name = {}
    for id_, source in mapping.items():
        if isinstance(source, str) and source not in by_name:
            by_name[source] = id_
    axes = {"p": [], "i": [], "j": [], "k": []}
    for name, *_ in spec["loops"]:
        if name in spec.get("repeats", []):
            continue
        if name not in by_name:
            return None
        axes[name if name in axes else "p"].append(str(by_name[name]))
    return ";".join(f"{axis}:{','.join(ids)}" for axis, ids in axes.items())


def check_repeats(spec, driver, rng, fills, tally):
    """The second run of a spec with repeat loops, with the family given."""
    first = run_driver(driver, spec)
    tally["repeat candidates generated"] += 1
    mapping = None
    for group in first["groups"]:
        mapping = state_map(spec, group.get("tables", []))
    text = family_text(spec, mapping or {})
    if text is None:
        tally["repeat family unmapped"] += 1
        return
    result = run_driver(driver, spec, family=text)
    for group in result["groups"]:
        for entry in group["candidates"]:
            if "request" not in entry:
                tally["repeat no proposal"] += 1
                continue
            form = entry["domain_symbolic"]["multiplicity"]
            finite_form = entry["domain_finite"]["multiplicity"]
            if form == "LOGICAL_BOX":
                tally["repeat accepted LOGICAL_BOX"] += 1
            elif form == "CONSTANT_BOX":
                tally["repeat accepted CONSTANT_BOX"] += 1
            elif entry["domain_symbolic"]["status"] == "VERIFIED" or finite_form == "COUNTED":
                tally["repeat refused (COUNTED/UNKNOWN)"] += 1
            if spec.get("refuse") and form in ("LOGICAL_BOX", "CONSTANT_BOX"):
                raise SystemExit(f"{spec['name']}: a box where Theorem 14's conditions fail")
            if spec.get("box") and form != spec["box"] and entry["domain_symbolic"]["status"] == "VERIFIED":
                raise SystemExit(f"{spec['name']}: wanted {spec['box']}, got {form}")
    check(spec, result, rng, fills, tally)


def check(spec, result, rng, fills, tally):
    if result["authorizes_lowering"]:
        raise SystemExit("a result authorizes lowering")
    low, high = spec["scope"]
    for group in result["groups"]:
        tables = group.get("tables", [])
        mapping = state_map(spec, tables)
        if not group["candidates"]:
            tally["no candidate"] += 1
        for entry in group["candidates"]:
            if entry["authorizes_lowering"] or entry["transformation"] != "UNKNOWN":
                raise SystemExit("a later phase's claim was made")
            if "request" not in entry:
                tally["no proposal"] += 1
                continue
            needed = set()
            for level in entry["request"]["levels"]:
                stack = [level["value"]]
                while stack:
                    t = stack.pop()
                    if "state" in t:
                        needed.add(t["state"])
                    stack += t.get("args", [])
            if not needed <= set(mapping):
                tally["unmapped"] += 1
                continue
            ds, df = entry["domain_symbolic"], entry["domain_finite"]
            tally["symbolic " + ds["status"]] += 1
            tally["finite " + df["status"]] += 1
            if ds["status"] == "REFUTED":
                raise SystemExit("a symbolic domain claim was refuted")
            points = []
            if ds["status"] == "VERIFIED":
                points = list(range(low, high + 1))
            elif df["status"] == "VERIFIED":
                points = [p["parameters"][0] for p in df["points"]]
            for n in points:
                events, lam = check_domain(spec, entry, n, tally, mapping)
                check_facts(spec, entry, ds if ds["status"] == "VERIFIED" else df, n, events, lam, mapping, tally)
            if df["status"] == "REFUTED":
                n = df["refutation"]["point"][0]
                _, lam = lambdas(spec, entry["request"], mapping, n)
                if lam == enumerate_set(entry["request"], [n]):
                    raise SystemExit(f"{spec['name']}: a refutation does not replay")
                tally["refutations replayed"] += 1
            t13 = entry["theorem13"]
            if t13["status"] == "VERIFIED":
                tally["Theorem 13 " + t13["tier"]] += 1
                at = list(range(low, high + 1)) if t13["tier"] == "SYMBOLIC" else [p[0] for p in t13["points"]]
                for n in at:
                    check_theorem13(spec, entry, n, mapping, rng, fills, tally)
            for kind in ("integer", "float"):
                fact = entry["reorder"][kind]
                if fact["status"] == "REFUTED":
                    check_witness(spec, fact, tally)
                if kind == "float" and fact["status"] == "PROVEN":
                    if entry["reorder"]["trivial"]["status"] != "PROVEN":
                        raise SystemExit("a float reorder fact proven without one contribution per cell")


def S(name, loops, h, f, g, scope=(1, 3), element="i32", **more):
    return dict(name=name, loops=loops, h=h, f=f, g=g, scope=scope, element=element, **more)


IJK = [("i", 0, "n", 1), ("j", 0, 3, 1), ("k", 0, 3, 1)]


def catalogue():
    return [
        S("rect", IJK, ["i", "j"], ["i", "k"], ["k", "j"]),
        S("triangle", [("i", 0, "n", 1), ("j", 0, 4, 1), ("k", 0, "j", 1)], ["i", "j"], ["i", "k"], ["k", "j"]),
        S("holed", IJK, ["i", "j"], ["i", "k"], ["k", "j"], guard=("bit", 365, ("+", ("*", "j", 3), "k"))),
        S("modular", IJK, ["i", "j"], ["i", "k"], ["k", "j"], guard=("==", ("%", ("*", "j", "k"), 3), 0)),
        S("piecewise", IJK, ["i", "j"], ["i", "k"], ["k", "j"], guard=("<", "k", "j"), piecewise=True,
          f2=["k", "i"]),
        S("stepped", [("i", 0, "n", 1), ("j", 0, 3, 1), ("k", 0, 5, 2)], ["i", "j"], ["i", "k"], ["k", "j"]),
        S("repeated", [("t", 0, 2, 1)] + IJK, ["i", "j"], ["i", "k"], ["k", "j"]),
        S("growing", [("i", 0, "n", 1), ("k", 0, 3, 1), ("t", 0, ("+", "k", 1), 1)], ["i", 0], ["i", "k"], ["k", "i"],
          repeats=["t"], box="LOGICAL_BOX"),
        S("triangle_repeat", [("i", 0, "n", 1), ("k", 0, 3, 1), ("t", "k", 3, 1)], ["i", 0], ["i", "k"], ["k", "i"],
          repeats=["t"], box="LOGICAL_BOX"),
        S("empty_repeat", [("i", 0, "n", 1), ("k", 0, 3, 1), ("t", "k", 2, 1)], ["i", 0], ["i", "k"], ["k", "i"],
          repeats=["t"], box="LOGICAL_BOX"),
        S("signed_repeat", [("i", 0, "n", 1), ("k", 0, 3, 1), ("t", -1, ("+", "k", 1), 1)], ["i", 0], ["i", "k"], ["k", "i"],
          repeats=["t"], box="LOGICAL_BOX"),
        S("two_repeats", [("i", 0, "n", 1), ("k", 0, 3, 1), ("t", 0, ("+", "k", 1), 1), ("u", 0, 2, 1)], ["i", 0], ["i", "k"],
          ["k", "i"], repeats=["t", "u"], box="LOGICAL_BOX"),
        S("constant_repeat", [("t", 0, 2, 1), ("i", 0, "n", 1), ("k", 0, 3, 1)], ["i", 0], ["i", "k"], ["k", "i"],
          repeats=["t"], box="CONSTANT_BOX"),
        S("nested_repeats", [("i", 0, "n", 1), ("k", 0, 3, 1), ("t", 0, 2, 1), ("u", 0, ("+", "t", 1), 1)], ["i", 0],
          ["i", "k"], ["k", "i"], repeats=["t", "u"], refuse=True),
        # t < k + 1 is the box t in [0, k + 1), and a box it is. A parity guard is no
        # interval in t, so Theorem 14's C1 refuses a closed form.
        S("bounded_by_guard", [("i", 0, "n", 1), ("k", 0, 3, 1), ("t", 0, 3, 1)], ["i", 0], ["i", "k"], ["k", "i"],
          guard=("<", "t", ("+", "k", 1)), repeats=["t"], box="LOGICAL_BOX"),
        S("parity_repeat", [("i", 0, "n", 1), ("k", 0, 3, 1), ("t", 0, 3, 1)], ["i", 0], ["i", "k"], ["k", "i"],
          guard=("==", ("%", ("+", "t", "k"), 2), 0), repeats=["t"], refuse=True),
        S("wide_repeats", [("i", 0, "n", 1), ("k", 0, 3, 1), ("t", 0, 2, 1), ("u", 0, 2, 1)], ["i", 0], ["i", "k"],
          ["k", "i"], repeats=["t", "u"], refuse=True, width=64),
        S("float_repeat", [("i", 0, "n", 1), ("k", 0, 3, 1), ("t", 0, ("+", "k", 1), 1)], ["i", 0], ["i", "k"], ["k", "i"],
          repeats=["t"], box="LOGICAL_BOX", element="f64"),
        S("reversed", IJK, ["i", "j"], ["i", "kk"], ["kk", "j"], derived=[("kk", ("-", 2, "k"))]),
        S("collide", IJK, [("%", ("+", "i", "j"), 2), 0], ["i", "k"], ["k", "j"]),
        S("nsw", IJK, ["i", "j"], ["i", "k"], ["k", "j"], nsw=True),
        S("floats", IJK, ["i", "j"], ["i", "k"], ["k", "j"], element="f64"),
    ]


def random_spec(rng, name):
    loops = [("i", 0, "n", 1), ("j", 0, rng.choice([2, 3, 4]), 1)]
    k_high = rng.choice([3, 4, "j"])
    loops.append(("k", 0, k_high, rng.choice([1, 1, 2])))
    repeats = []
    if rng.random() < 0.2:
        loops = [("t", 0, 2, 1)] + loops
        repeats = ["t"]
    elif rng.random() < 0.25:
        loops = loops + [("t", 0, rng.choice([("+", "k", 1), 2, ("-", 3, "k")]), 1)]
        repeats = ["t"]
    guard = rng.choice([None, None, ("bit", rng.randrange(1, 512), ("+", ("*", "j", 3), "k")),
                        ("==", ("%", ("+", "j", "k"), 2), 0), ("<", "k", "j")])
    h = rng.choice([["i", "j"], ["i", "j"], [("%", ("+", "i", "j"), 2), 0], ["j", "i"]])
    spec = S(name, loops, h, ["i", "k"], ["k", "j"], element=rng.choice(["i32", "i32", "f64"]), guard=guard)
    if guard and rng.random() < 0.2:
        spec["piecewise"] = True
        spec["f2"] = ["k", "i"]
    if spec["element"] == "i32" and rng.random() < 0.2:
        spec["nsw"] = True
    if repeats:
        spec["repeats"] = repeats
    return spec


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--driver", required=True)
    parser.add_argument("--programs", type=int, default=40)
    parser.add_argument("--fills", type=int, default=16)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--jobs", type=int, default=2)
    args = parser.parse_args()
    from collections import Counter
    tally = Counter()
    rng = random.Random(args.seed)
    specs = catalogue() + [random_spec(rng, f"p{i}") for i in range(args.programs)]

    def one(spec):
        try:
            return spec, run_driver(args.driver, spec)
        except subprocess.TimeoutExpired:
            return spec, None

    with ThreadPoolExecutor(args.jobs) as pool:
        results = list(pool.map(one, specs))
    for n, (spec, result) in enumerate(results):
        if result is None:
            tally["timed out"] += 1
            continue
        tally["programs"] += 1
        check(spec, result, random.Random(args.seed * 7919 + n), args.fills, tally)
    for n, spec in enumerate(specs):
        if spec.get("repeats"):
            try:
                check_repeats(spec, args.driver, random.Random(args.seed * 104729 + n), args.fills, tally)
            except subprocess.TimeoutExpired:
                tally["timed out"] += 1
    for key in sorted(tally):
        print(f"{key}: {tally[key]}")
    if tally["timed out"]:
        raise SystemExit("a driver run timed out")
    if not tally["symbolic VERIFIED"] or not tally["points checked"] or not tally["Theorem 13 runs"]:
        raise SystemExit("this run tested nothing")
    # Theorem 14 must have been exercised, accepted and refused, or this run says nothing
    # about it.
    if tally["repeat accepted LOGICAL_BOX"] < 4 or tally["multiplicity LOGICAL_BOX"] < 4:
        raise SystemExit("fewer than 4 LOGICAL_BOX cases were checked")
    if tally["repeat accepted CONSTANT_BOX"] < 1 or tally["repeat refused (COUNTED/UNKNOWN)"] < 2:
        raise SystemExit("the constant box or the refusals were not exercised")
    if tally["zero multiplicity points"] < 1:
        raise SystemExit("no logical point with zero events was checked")
    print("0 disagreements")


if __name__ == "__main__":
    main()
