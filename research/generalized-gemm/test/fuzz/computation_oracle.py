#!/usr/bin/env python3
"""G4a oracle. A spec is interpreted by plain Python loops and printed as MLIR for the
driver, and every claim is recomputed here with no library code.

For each claim of a group, at the points it covers, the program runs on many initial
memories and the replay relation is computed from its definition: operands from the
initial memory, each event's value in order, each final cell. A VERIFIED claim that
fails at any completing run is an error. A REFUTED claim must reproduce at its witness.

Fills here are a test of the checker, never a proof. The checker's own proof is for
all initial memories, and this file only tries to break it, with signed zeros,
rounding ties, infinities and NaN in the fills.

Every driver run has a time limit, and a timeout fails this run: it never counts as a
pass. The catalogue states by hand what each named case must give.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import math
from pathlib import Path
import random
import struct
import subprocess
import sys
import tempfile

TIMEOUT = 240
EXTENT = 6
WIDTH = 16
SOLVER = ["--timeout-ms=2000", "--rlimit=400000"]


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


def signed32(value):
    return value - (1 << 32) if value >> 31 else value


def fmul(a, b, element, flags):
    if element == "f64":
        r = a * b
        if math.isnan(r):
            raise Stop("NaN")
        return r
    r = (a * b) & 0xFFFFFFFF
    if "nsw" in flags and signed32(a) * signed32(b) != signed32(r):
        raise Stop("poison")
    return r


def fadd(a, b, element, flags):
    if element == "f64":
        r = a + b
        if math.isnan(r):
            raise Stop("NaN")
        return r
    r = (a + b) & 0xFFFFFFFF
    if "nsw" in flags and signed32(a) + signed32(b) != signed32(r):
        raise Stop("poison")
    return r


def expression(e, env):
    if isinstance(e, int):
        return e
    if isinstance(e, str):
        return env[e]
    a, b = expression(e[1], env), expression(e[2], env)
    if e[0] == "+":
        return a + b
    if e[0] == "*":
        return a * b
    if e[0] == "%":
        return a % b
    raise SystemExit(f"unknown expression {e}")


def cell(index, env):
    r, c = expression(index[0], env), expression(index[1], env)
    if not (0 <= r < EXTENT and 0 <= c < EXTENT):
        raise Stop("out of bounds")
    return (r, c)


def interpret(spec, n, memory):
    """The ordered trace of one run. Raises Stop when the run does not complete."""
    element = spec["element"]
    trace = []
    mem = {root: dict(cells) for root, cells in memory.items()}

    def body(statements, env):
        for s in statements:
            kind = s[0]
            if kind == "loop":
                _, var, bound, inner = s
                for v in range(expression(bound, env) if bound != "n" else n):
                    body(inner, {**env, var: v})
            elif kind == "if":
                _, left, right, inner = s
                if expression(left, env) >= expression(right, env):
                    body(inner, env)
            elif kind == "mac":
                _, cr, ci, ar, ai, br, bi, *rest = s
                addf, mulf, order = rest if rest else ("", "", "zm")
                f, g, h = cell(ai, env), cell(bi, env), cell(ci, env)
                x, y, z = mem[ar][f], mem[br][g], mem[cr][h]
                m = fmul(x, y, element, mulf)
                u = fadd(z, m, element, addf) if order == "zm" else fadd(m, z, element, addf)
                mem[cr][h] = u
                trace.append(("event", (cr, ar, br), cr, h, ar, f, br, g, x, y, u, s))
            elif kind == "lost":
                _, cr, ci, ar, ai, br, bi = s
                f, g, h = cell(ai, env), cell(bi, env), cell(ci, env)
                x, y, z = mem[ar][f], mem[br][g], mem[cr][h]
                m = fmul(x, y, element, "")
                for _ in range(2):
                    u = fadd(z, m, element, "")
                    mem[cr][h] = u
                    trace.append(("event", (cr, ar, br), cr, h, ar, f, br, g, x, y, u, s))
            elif kind == "set":
                _, root, index, value = s
                at = cell(index, env)
                mem[root][at] = value
                trace.append(("write", root, at, True, value))
            elif kind == "copy":
                _, root, index, source, from_index = s
                at, read = cell(index, env), cell(from_index, env)
                mem[root][at] = mem[source][read]
                trace.append(("write", root, at, False, mem[root][at]))
            elif kind == "extra":
                # A second store of the last update, to another cell.
                _, root, index = s
                last = next(t for t in reversed(trace) if t[0] == "event")
                at = cell(index, env)
                mem[root][at] = last[10]
                trace.append(("write", root, at, False, last[10]))
            else:
                raise SystemExit(f"unknown statement {s}")

    body(spec["body"], {"n": n})
    return trace, mem


def relation_fails(spec, group, memory, trace, final):
    """The replay of G4a, from its definition. A reason when it does not hold."""
    element = spec["element"]
    current, touched = {}, set()
    for t in trace:
        if t[0] == "write" or t[1] != group:
            key = (t[1], t[2]) if t[0] == "write" else (t[2], t[3])
            is_const = t[0] == "write" and t[3]
            if key not in touched:
                if is_const:
                    current[key] = t[4]
                else:
                    current.pop(key, None)
            continue
        _, _, cr, h, ar, f, br, g, x, y, u, site = t
        key = (cr, h)
        a0, b0 = memory[ar][f], memory[br][g]
        if bits(x, element) != bits(a0, element) or bits(y, element) != bits(b0, element):
            return "an operand read is not its initial value"
        start = current.get(key, memory[cr][h])
        addf, mulf, order = (site[7], site[8], site[9]) if len(site) > 7 else ("", "", "zm")
        # Flags change definedness, and not the value of a run that completed.
        m = fmul(a0, b0, element, "")
        expected = fadd(start, m, element, "") if order == "zm" else fadd(m, start, element, "")
        if bits(expected, element) != bits(u, element):
            return "an event's value is not the replay's"
        current[key] = expected
        touched.add(key)
    for key in touched:
        if bits(final[key[0]][key[1]], element) != bits(current[key], element):
            return "a final cell is not the replay's"
    return None


POOL = [0.0, -0.0, 1.0, -1.0, 0.5, 3.0, 1e16, -1e16, 1e-3, 9007199254740992.0, 1e308, -1e308,
        5e-324, math.inf, -math.inf]


def fill(spec, rng, nan):
    memory = {}
    for root in "abcd":
        cells = {}
        for r in range(EXTENT):
            for c in range(EXTENT):
                if spec["element"] == "i32":
                    cells[(r, c)] = rng.choice([rng.getrandbits(32), rng.randrange(8), 0xFFFFFFFF, 0x7FFFFFFF])
                elif nan and rng.random() < 0.02:
                    cells[(r, c)] = math.nan
                else:
                    cells[(r, c)] = rng.choice(POOL) if rng.random() < 0.7 else rng.randrange(-64, 64) / 8.0
        memory[root] = cells
    return memory


def print_expression(e, lines, names):
    if isinstance(e, int):
        return f"%c{e}"
    if isinstance(e, str):
        return "%n" if e == "n" else "%" + e
    a, b = print_expression(e[1], lines, names), print_expression(e[2], lines, names)
    names[0] += 1
    name = f"%t{names[0]}"
    op = {"+": "arith.addi", "*": "arith.muli", "%": "arith.remui"}[e[0]]
    lines.append(f"{name} = {op} {a}, {b} : index")
    return name


def constants_in(spec):
    found = set(range(EXTENT + 1))

    def walk(item):
        if isinstance(item, int):
            found.add(item)
        elif isinstance(item, (list, tuple)):
            for i in item:
                walk(i)
    walk(spec["body"])
    return sorted(v for v in found if v >= 0 and not isinstance(v, bool))


def print_function(spec, authorized=True):
    element = spec["element"]
    t = f"memref<{EXTENT}x{EXTENT}x{element}>"
    names = [0]
    out = []
    add, mul = ("arith.addf", "arith.mulf") if element == "f64" else ("arith.addi", "arith.muli")

    def fm(flags):
        if not flags:
            return ""
        return f" overflow<{flags}>" if element == "i32" else f" fastmath<{flags}>"

    def index(i, lines):
        return ", ".join(print_expression(e, lines, names) for e in i)

    def literal(value):
        if element == "i32":
            return str(value)
        return "0x%016X" % bits(value, "f64")

    def body(statements):
        for s in statements:
            kind = s[0]
            lines = []
            if kind == "loop":
                _, var, bound, inner = s
                b = print_expression(bound, lines, names)
                out.extend(lines)
                out.append(f"scf.for %{var} = %c0 to {b} step %c1 {{")
                body(inner)
                out.append("}")
            elif kind == "if":
                _, left, right, inner = s
                a, b = print_expression(left, lines, names), print_expression(right, lines, names)
                names[0] += 1
                out.extend(lines)
                out.append(f"%t{names[0]} = arith.cmpi uge, {a}, {b} : index")
                out.append(f"scf.if %t{names[0]} {{")
                body(inner)
                out.append("}")
            elif kind in ("mac", "lost"):
                _, cr, ci, ar, ai, br, bi, *rest = s
                addf, mulf, order = rest if rest else ("", "", "zm")
                ii, fi, gi = index(ci, lines), index(ai, lines), index(bi, lines)
                out.extend(lines)
                names[0] += 1
                k = names[0]
                out.append(f"%x{k} = memref.load %{ar}[{fi}] : {t}")
                out.append(f"%y{k} = memref.load %{br}[{gi}] : {t}")
                out.append(f"%z{k} = memref.load %{cr}[{ii}] : {t}")
                out.append(f"%m{k} = {mul} %x{k}, %y{k}{fm(mulf)} : {element}")
                pair = f"%z{k}, %m{k}" if order == "zm" else f"%m{k}, %z{k}"
                out.append(f"%s{k} = {add} {pair}{fm(addf)} : {element}")
                out.append(f"memref.store %s{k}, %{cr}[{ii}] : {t}")
                if kind == "lost":
                    out.append(f"%u{k} = {add} {pair} : {element}")
                    out.append(f"memref.store %u{k}, %{cr}[{ii}] : {t}")
                spec.setdefault("_last", []).append(f"%s{k}")
            elif kind == "set":
                _, root, i, value = s
                text = index(i, lines)
                out.extend(lines)
                names[0] += 1
                out.append(f"%v{names[0]} = arith.constant {literal(value)} : {element}")
                out.append(f"memref.store %v{names[0]}, %{root}[{text}] : {t}")
            elif kind == "copy":
                _, root, i, source, from_i = s
                to, read = index(i, lines), index(from_i, lines)
                out.extend(lines)
                names[0] += 1
                out.append(f"%w{names[0]} = memref.load %{source}[{read}] : {t}")
                out.append(f"memref.store %w{names[0]}, %{root}[{to}] : {t}")
            elif kind == "extra":
                _, root, i = s
                text = index(i, lines)
                # The store goes first, so the site keeps its store to C.
                last = out.pop()
                out.extend(lines)
                out.append(f"memref.store {spec['_last'][-1]}, %{root}[{text}] : {t}")
                out.append(last)
    spec.pop("_last", None)
    body(spec["body"])
    head = [f"%c{v} = arith.constant {v} : index" for v in constants_in(spec)]
    arguments = ", ".join(f"%{r}: {t}" for r in "abcd") + ", %n: index"
    return (f"func.func @{spec['name']}({arguments}) {{\n" + "\n".join(head + out) + "\nreturn\n}\n")


PAIRS = ",".join(f"roots {a} and {b} are separate" for a in range(4) for b in range(a + 1, 4))
ROOTS = {0: "a", 1: "b", 2: "c", 3: "d"}


def run_driver(driver, spec, extra):
    with tempfile.NamedTemporaryFile("w", suffix=".mlir") as handle:
        handle.write(print_function(spec))
        handle.flush()
        low, high = spec["scope"]
        command = [driver, handle.name, "--lower-structured", f"--index-bit-width={WIDTH}",
                   f"--scope=0:{low}:{high}"] + SOLVER + extra
        done = subprocess.run(command, capture_output=True, text=True, timeout=TIMEOUT)
    if done.returncode:
        raise SystemExit(f"driver failed on {spec['name']}:\n{done.stderr[-1200:]}")
    return json.loads(done.stdout)[0]


def check_claim(spec, group, claim, points, rng, fills, tally):
    for n in points:
        for f in range(fills):
            memory = fill(spec, rng, nan=f % 5 == 0)
            try:
                trace, final = interpret(spec, n, memory)
            except Stop:
                tally["stopped runs"] += 1
                continue
            tally["completing runs"] += 1
            try:
                why = relation_fails(spec, group, memory, trace, final)
            except Stop:
                why = "the replay stops where the program does not"
            if why:
                raise SystemExit(f"{spec['name']} n={n}: {claim} is false: {why}\n{print_function(spec)}")


def check_witness(spec, group, entry, tally):
    element = spec["element"]
    memory = {}
    for r, cells in enumerate(entry["memory"]):
        values = {}
        for c, text in enumerate(cells):
            values[(c // EXTENT, c % EXTENT)] = from_hex(text, element)
        memory[ROOTS[r]] = values
    n = entry["parameters"][0]
    try:
        trace, final = interpret(spec, n, memory)
    except Stop as stop:
        raise SystemExit(f"{spec['name']}: the witness does not complete here: {stop}")
    if not relation_fails(spec, group, memory, trace, final):
        raise SystemExit(f"{spec['name']}: the witness shows no difference")
    tally["witnesses replayed"] += 1


def check_result(spec, result, rng, fills, tally):
    if result["authorizes_lowering"]:
        raise SystemExit(f"{spec['name']}: a result authorized lowering")
    low, high = spec["scope"]
    for g in result["groups"]:
        if g["unsupported"]:
            continue
        key = (ROOTS[g["output"]], ROOTS[g["first"]], ROOTS[g["second"]])
        finite, symbolic = g["finite"], g["symbolic"]
        for claim in (finite, symbolic):
            tally[f"{claim['status']}"] += 1
            if claim["domain_and_order"] != "UNKNOWN" or claim["transformation"] != "UNKNOWN":
                raise SystemExit(f"{spec['name']}: a later phase's claim was made")
        if finite["status"] == "REFUTED" and finite["tier"] != "PROPOSED":
            raise SystemExit(f"{spec['name']}: a refutation has a tier")
        if symbolic["status"] == "REFUTED":
            raise SystemExit(f"{spec['name']}: refuted on an over-approximation")
        if finite["status"] == "VERIFIED":
            tally["finite " + finite["tier"]] += 1
            points = [p["parameters"][0] for p in finite["points"]]
            if finite["tier"] == "SCOPE_ENUMERATED":
                missing = [n for n in range(low, high + 1) if n not in points and n > 0]
                if missing and any(spec_has_events(spec, key, n) for n in missing):
                    raise SystemExit(f"{spec['name']}: enumerated claim misses {missing}")
            check_claim(spec, key, "the finite claim", points, rng, fills, tally)
        if symbolic["status"] == "VERIFIED":
            tally["symbolic SYMBOLIC"] += 1
            # Over the whole scope, past anything the driver ran.
            check_claim(spec, key, "the symbolic claim", range(low, high + 1), rng, fills, tally)
        if finite["status"] == "REFUTED":
            check_witness(spec, key, finite["witness"], tally)
        if finite["status"] == "VERIFIED" and symbolic["status"] == "VERIFIED":
            tally["both routes"] += 1
        for claim in (finite, symbolic):
            for use in claim["separations"]:
                if use["assumption"] != (use["basis"] != "fresh allocation"):
                    raise SystemExit(f"{spec['name']}: a separation hides its basis")


def spec_has_events(spec, key, n):
    memory = {r: {(i, j): 0 if spec["element"] == "i32" else 1.0 for i in range(EXTENT) for j in range(EXTENT)}
              for r in "abcd"}
    try:
        trace, _ = interpret(spec, n, memory)
    except Stop:
        return True
    return any(t[0] == "event" and t[1] == key for t in trace)


def L(var, bound, *inner):
    return ("loop", var, bound, list(inner))


GEMM = ("mac", "c", ("i", "j"), "a", ("i", "k"), "b", ("k", "j"))
IGEMM = GEMM
NEST = lambda *inner: L("i", "n", L("j", 2, L("k", 2, *inner)))


def named():
    """(name, element, body, finite status, symbolic status, for group (c, a, b)...)"""
    V, R, U = "VERIFIED", "REFUTED", "UNKNOWN"
    cases = [
        ("clean", "f64", [NEST(GEMM)], V, V),
        ("clean_int", "i32", [NEST(GEMM)], V, V),
        ("nsw", "i32", [NEST(GEMM + ("nsw", "", "zm"))], V, V),
        ("commuted", "f64", [NEST(GEMM + ("", "", "mz"))], V, V),
        ("contract", "f64", [NEST(GEMM + ("contract", "contract", "zm"))], V, V),
        ("reassoc", "f64", [NEST(GEMM + ("reassoc", "reassoc", "zm"))], V, V),
        ("lost", "f64", [NEST(("lost",) + GEMM[1:])], R, U),
        ("after", "f64", [NEST(GEMM), ("set", "c", (0, 0), 0.0)], R, U),
        ("reset", "f64", [L("i", "n", L("j", 2, L("k", 2, GEMM), ("set", "c", ("i", "j"), 0.0)))], R, U),
        ("prefix", "f64", [L("i", "n", L("j", 2, ("set", "c", ("i", "j"), 0.0), L("k", 2, GEMM)))], V, U),
        ("overlap", "f64", [NEST(("mac", "c", ("i", "j"), "c", ("k", "i"), "b", ("k", "j")))], R, U),
        ("repeated", "f64", [L("t", 2, NEST(GEMM))], V, V),
        ("repeated_int", "i32", [L("t", 3, NEST(GEMM))], V, V),
        ("collapsed", "f64", [NEST(("mac", "c", (0, 0), "a", ("i", "k"), "b", ("k", "j")))], V, V),
        ("modular", "i32", [NEST(("mac", "c", (("%", ("+", "i", "j"), 2), 0), "a", ("i", "k"), "b", ("k", "j")))], V, V),
        ("extra_elsewhere", "f64", [NEST(GEMM, ("extra", "d", ("i", "j")))], V, V),
        ("extra_in_output", "f64", [NEST(GEMM, ("extra", "c", ("j", "i")))], R, U),
        ("disjoint_rows", "f64", [NEST(("mac", "c", ("i", "j"), "c", (5, "k"), "b", ("k", "j")))], V, V),
        ("late_write", "f64", [NEST(GEMM), ("if", "n", 3, [("set", "c", (0, 0), 0.0)])], V, V),
        ("late_elsewhere", "f64", [NEST(GEMM), ("set", "d", (0, 0), 1.0)], V, V),
    ]
    return [{"name": n, "element": e, "body": b, "scope": (1, 2), "want": (f, s)} for n, e, b, f, s in cases]


def random_spec(rng, name):
    element = rng.choice(["f64", "i32"])
    constant = 0.0 if element == "f64" else 0

    def index_expression(variables):
        kind = rng.randrange(6)
        v = rng.choice(variables)
        if kind == 0:
            return ("+", v, rng.randrange(1, 3))
        if kind == 1:
            return ("%", ("+", v, rng.choice(variables)), 2)
        if kind == 2:
            return rng.randrange(EXTENT)
        return v

    def access(variables):
        return (index_expression(variables), index_expression(variables))

    variables = ["i", "j", "k"]
    flags = ("", "", rng.choice(["zm", "mz"]))
    b_root = rng.choice("bbc" if rng.random() < 0.15 else "bb")
    site = ("mac", "c", access(variables), rng.choice("aac"), access(variables), b_root, access(variables)) + flags
    inner = [site]
    roll = rng.randrange(10)
    if roll == 0:
        inner.append(("set", rng.choice("cd"), access(variables), constant))
    elif roll == 1:
        inner.append(("extra", rng.choice("dc"), access(variables)))
    elif roll == 2:
        inner.insert(0, ("mac", "c", access(variables), "a", access(variables), "d", access(variables)))
    elif roll == 3:
        inner.append(("copy", "d", access(variables), "c", access(variables)))
    bounds = [rng.choice(["n", 2, 3]) for _ in range(3)]
    nest = [L("i", bounds[0], L("j", bounds[1], L("k", bounds[2], *inner)))]
    roll = rng.randrange(6)
    if roll == 0:
        nest.append(("set", rng.choice("cd"), (rng.randrange(EXTENT), rng.randrange(EXTENT)), constant))
    elif roll == 1:
        nest.insert(0, ("set", "c", (rng.randrange(EXTENT), rng.randrange(EXTENT)), constant))
    elif roll == 2:
        nest = [L("t", 2, *nest)]
    return {"name": name, "element": element, "body": nest, "scope": (1, 3), "want": None}


def demonstrations(tally):
    """Why the rules exist: each of these is a false statement a checker could make."""
    # 1e16 + 1 + 1 in order stays 1e16, and the sum of the contributions first does not.
    fold = 1e16
    for _ in range(2):
        fold = fold + 1.0 * 1.0
    if fold != 1e16 or 1e16 + (1.0 + 1.0) == fold:
        raise SystemExit("rounding demonstration failed")
    # A start from +0 is not a start from the cell: -0 + -0 stays -0, +0 + -0 does not.
    if math.copysign(1, -0.0 + -0.0) != -1 or math.copysign(1, 0.0 + -0.0) != 1:
        raise SystemExit("signed zero demonstration failed")
    # Fused and separate rounding differ, so `contract` allows another result.
    x = 1.0 + 2.0 ** -30
    product = x * x
    exact = (1 << 60) + (1 << 31) + 1
    fused = float(exact - (1 << 60)) * 2.0 ** -60
    if abs((product - 1.0) - fused) == 0:
        raise SystemExit("FMA demonstration failed")
    # Integers fold in any order, in value.
    rng = random.Random(11)
    values = [rng.getrandbits(32) for _ in range(9)]
    total = 5
    for v in values:
        total = (total + v) & 0xFFFFFFFF
    rng.shuffle(values)
    other = 5
    for v in values:
        other = (other + v) & 0xFFFFFFFF
    if total != other:
        raise SystemExit("Theorem 10 demonstration failed")
    # Aliased arguments: with A and C one buffer, the separate-array replay is false.
    spec = {"name": "alias", "element": "i32", "body": [L("i", 2, ("mac", "c", (0, 0), "a", (0, 0), "b", (0, 0)))]}
    shared = {(r, c): 3 for r in range(EXTENT) for c in range(EXTENT)}
    other_root = {(r, c): 2 for r in range(EXTENT) for c in range(EXTENT)}
    memory = {"a": dict(shared), "b": dict(other_root), "c": dict(shared), "d": dict(shared)}
    trace, final = interpret(spec, 0, memory)
    aliased = 3
    for _ in range(2):
        aliased = aliased + aliased * 2  # A reads what C just wrote.
    if final["c"][(0, 0)] == aliased:
        raise SystemExit("aliasing demonstration failed")
    tally["demonstrations"] += 6


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--driver", required=True)
    parser.add_argument("--programs", type=int, default=60)
    parser.add_argument("--fills", type=int, default=40)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--jobs", type=int, default=6)
    parser.add_argument("--no-catalogue", action="store_true")
    args = parser.parse_args()

    tally = {k: 0 for k in ("programs", "completing runs", "stopped runs", "witnesses replayed",
                            "hand expectations", "demonstrations", "both routes", "VERIFIED",
                            "REFUTED", "UNKNOWN", "finite POINT", "finite SCOPE_ENUMERATED",
                            "symbolic SYMBOLIC", "timed out", "unauthorized refused")}
    demonstrations(tally)
    specs = [] if args.no_catalogue else named()
    rng = random.Random(args.seed)
    specs += [random_spec(rng, f"p{n}") for n in range(args.programs)]

    def one(spec):
        try:
            return spec, run_driver(args.driver, spec, ["--no-search", f"--authorize={PAIRS}"])
        except subprocess.TimeoutExpired:
            return spec, None

    with ThreadPoolExecutor(args.jobs) as pool:
        results = list(pool.map(one, specs))
    for n, (spec, result) in enumerate(results):
        if result is None:
            tally["timed out"] += 1
            continue
        tally["programs"] += 1
        check_result(spec, result, random.Random(args.seed * 1000 + n), args.fills, tally)
        if spec["want"]:
            group = result["groups"][0]
            got = (group["finite"]["status"], group["symbolic"]["status"])
            if got != spec["want"]:
                raise SystemExit(f"{spec['name']}: wanted {spec['want']}, got {got}: "
                                 f"{group['finite']['reason']}; {group['symbolic']['reason']}")
            tally["hand expectations"] += 1

    if not args.no_catalogue:
        # No authorization: no claim on either route, whatever the program.
        spec = named()[0]
        result = run_driver(args.driver, spec, ["--no-search"])
        group = result["groups"][0]
        if group["finite"]["status"] != "UNKNOWN" or group["symbolic"]["status"] != "UNKNOWN":
            raise SystemExit("a claim without the separation authorized")
        tally["unauthorized refused"] += 1
        # The search adds candidates, which join the group's claim at the least tier.
        result = run_driver(args.driver, dict(spec), ["--max-level=0", f"--authorize={PAIRS}"])
        candidates = result["groups"][0]["candidates"]
        if not candidates or any(c["tier"] != "SYMBOLIC" or c["authorizes_lowering"] or
                                 c["transformation"] != "UNKNOWN" for c in candidates):
            raise SystemExit(f"candidates: {candidates}")

    for key in sorted(tally):
        print(f"{key}: {tally[key]}")
    # A timeout is no evidence, so it can never be a pass.
    if tally["timed out"]:
        raise SystemExit("a driver run timed out")
    if not tally["VERIFIED"] or not tally["completing runs"]:
        raise SystemExit("this run tested nothing")
    print("0 disagreements")


if __name__ == "__main__":
    main()
