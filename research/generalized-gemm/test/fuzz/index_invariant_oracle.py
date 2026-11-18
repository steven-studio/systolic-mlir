#!/usr/bin/env python3
"""Independent oracle of the index invariants (statement BV record, Section 14).

A spec is a GEMM-shaped nest whose K subscript is a scalar recurrence carried by
scf.for iter_args: started from a parameter or a constant, stepped up or down by a
constant before or after the MAC, at the index width or at a narrower one (where it
wraps, or with nsw becomes poison), reset when the k loop is entered or carried through
the j loop. The spec is interpreted here by plain Python loops and printed as MLIR for
the driver. Every claim the driver makes with --index-invariants is then recomputed at
EVERY point of the scope, with this file's own interpreter and its own reader of the
report's term text. No SMT solver and no code of this project decides anything:

  INDUCTIVE            every atom holds at every arrival at its cut, in every run.
  relation PROVED      at every event of the site, in every run, the recorded subscript
                       equals the relation's value at the entry of the event's block.
  relation REFUTED     the witness's run has that event, with that subscript, and the
                       relation's value there is the other value the report states.
  over_coordinates     recomputed from the relation's text and the family's axes.
  SYMBOLIC_VALID/EXACT at every point: equal (P,I,K) give one A address, equal (P,K,J)
                       one B address, equal (P,I,J) one C address, and no two events share
                       coordinates; for EXACT the events are the orbit of the declared order.
  PROVEN_TERMINATING   every run of the scope returns.
  a given invariant    is reported as given, never as found; one that is false at some
                       arrival of some run is never INDUCTIVE, and is then handed to no
                       checker (the certificate is not strengthened).

A claim the driver does not make is not an error: UNKNOWN and NOTHING_FOUND are counted.
Exit 1 with "FAIL ..." lines when a claim is wrong, or a hand-stated expectation of the
catalogue is missed.
"""
import argparse
import itertools
import json
import random
import re
import subprocess
import sys
import tempfile
from pathlib import Path

TIMEOUT = 600


def signed(value, width):
    value &= (1 << width) - 1
    return value - (1 << width) if value >> (width - 1) else value


# --- The spec: printed as MLIR, and interpreted -----------------------------------

def mlir(spec):
    """The nest as scf.for with iter_args. Entry constants in a fixed order: the report
    names them "result of op N of block 0"."""
    iw, xw = spec["iw"], spec["xw"]
    xt = "index" if xw == iw else f"i{xw}"
    da, db, dn, J = spec["da"], spec["db"], spec["dn"], spec["J"]
    ma, mb, mc = f"memref<{dn}x{da}xi32>", f"memref<{db}x{J}xi32>", f"memref<{dn}x{J}xi32>"
    kind, value = spec["init"]
    args = [f"%a: {ma}", f"%b: {mb}", f"%c: {mc}", "%n: index", "%kk: index"]
    if kind == "param":
        args.append(f"%x0: {xt}")
    lines = [f"func.func @f({', '.join(args)}) {{",
             "  %c0 = arith.constant 0 : index",
             "  %c1 = arith.constant 1 : index",
             f"  %cJ = arith.constant {J} : index",
             f"  %st = arith.constant {abs(spec['step'])} : {xt}"]
    start = "%x0"
    if kind == "const":
        lines.append(f"  %xs = arith.constant {value} : {xt}")
        start = "%xs"
    flag = " overflow<nsw>" if spec["nsw"] else ""
    op = "arith.addi" if spec["step"] >= 0 else "arith.subi"
    update = f"        %x1 = {op} %x, %st{flag} : {xt}"
    lines.append("  scf.for %i = %c0 to %n step %c1 {")
    if spec["carried"]:
        lines.append(f"    %xo = scf.for %j = %c0 to %cJ step %c1 iter_args(%xj = {start}) -> ({xt}) {{")
        inner = "%xj"
    else:
        lines.append("    scf.for %j = %c0 to %cJ step %c1 {")
        inner = start
    lines.append(f"      %xf = scf.for %k = %c0 to %kk step %c1 iter_args(%x = {inner}) -> ({xt}) {{")
    used = "%x"
    if spec["pre"]:
        lines.append(update)
        used = "%x1"
    if xw != iw:
        cast = "arith.index_castui" if spec["cast"] == "zext" else "arith.index_cast"
        lines.append(f"        %u = {cast} {used} : {xt} to index")
        used = "%u"
    lines += [f"        %p = memref.load %a[%i, {used}] : {ma}",
              f"        %q = memref.load %b[{used}, %j] : {mb}",
              f"        %z = memref.load %c[%i, %j] : {mc}",
              "        %m = arith.muli %p, %q : i32",
              "        %s = arith.addi %z, %m : i32",
              f"        memref.store %s, %c[%i, %j] : {mc}"]
    if not spec["pre"]:
        lines.append(update)
    lines += [f"        scf.yield %x1 : {xt}", "      }"]
    if spec["carried"]:
        lines.append(f"      scf.yield %xf : {xt}")
    lines += ["    }", "  }", "  return", "}"]
    return "\n".join(lines) + "\n"


def entry_constants(spec):
    """Value and width of each entry-block operation, in order."""
    out = [(0, spec["iw"]), (1, spec["iw"]), (spec["J"], spec["iw"]), (abs(spec["step"]), spec["xw"])]
    if spec["init"][0] == "const":
        out.append((spec["init"][1], spec["xw"]))
    return out


def header_arguments(spec):
    """Per loop level, the roles of the header block's arguments, in order."""
    return [["i"], ["j", "xj"] if spec["carried"] else ["j"], ["k", "x"]]


def interpret(spec, params):
    """The run from one point. Returns (status, arrivals, events): each arrival is
    (level, state) with state[role] = (value, poison) for every header argument assigned
    so far; each event has the state at the entry of its block and the three addresses."""
    iw, xw = spec["iw"], spec["xw"]
    mi, mx = (1 << iw) - 1, (1 << xw) - 1
    n, kk = params[0] & mi, params[1] & mi
    kind, value = spec["init"]
    start = ((params[2] if kind == "param" else value) & mx, False)
    c = spec["step"]
    state, arrivals, events = {}, [], []

    def arrive(level):
        arrivals.append((level, dict(state)))

    def step(x):
        raw = signed(x[0], xw) + c
        overflow = spec["nsw"] and not (-(1 << (xw - 1)) <= raw < (1 << (xw - 1)))
        return (raw & mx, x[1] or overflow)

    def subscript(x):
        if xw == iw:
            return x
        wide = x[0] if spec["cast"] == "zext" else signed(x[0], xw) & mi
        return (wide, x[1])

    i = 0
    while True:
        state["i"] = (i, False)
        arrive(0)
        if not signed(i, iw) < signed(n, iw):
            break
        j, xj = 0, start
        while True:
            state["j"] = (j, False)
            if spec["carried"]:
                state["xj"] = xj
            arrive(1)
            if not signed(j, iw) < spec["J"]:
                break
            k, x = 0, xj if spec["carried"] else start
            while True:
                state["k"], state["x"] = (k, False), x
                arrive(2)
                if not signed(k, iw) < signed(kk, iw):
                    break
                # The MAC block. Its side conditions come before its effects: a poison
                # subscript or one outside the extents stops the run with no event.
                after = step(x)
                u = subscript(after if spec["pre"] else x)
                if u[1]:
                    return "stopped: a poison subscript", arrivals, events
                cells = [(signed(i, iw), spec["dn"]), (signed(u[0], iw), spec["da"]),
                         (signed(u[0], iw), spec["db"]), (signed(j, iw), spec["J"])]
                if any(v < 0 or v >= extent for v, extent in cells):
                    return "stopped: a subscript outside its extent", arrivals, events
                events.append(dict(state=dict(state), a=(i, u[0]), b=(u[0], j), c=(i, j)))
                x = after
                k = (k + 1) & mi
            if spec["carried"]:
                xj = x
            j = (j + 1) & mi
        i = (i + 1) & mi
    return "returned", arrivals, events


# --- The report's term text ---------------------------------------------------------

TOKEN = re.compile(r"\s*(zext\d+|sext\d+|trunc\d+|ite|sp\d+|s\d+|p\d+|\d+|==|!=|<=s|>=s|<=u|>=u|<s|>s|<u|>u|"
                   r"&&|\|\||/u|%u|[-+*](?:nsw|nuw)*|[()!,])")


def parse(text):
    tokens, pos = [], 0
    while pos < len(text):
        if text[pos:].strip() == "":
            break
        m = TOKEN.match(text, pos)
        if not m:
            raise ValueError("cannot read " + text[pos:])
        tokens.append(m.group(1))
        pos = m.end()
    # A flag on an operator (+nsw) sets a poison bit and leaves the value as it is.
    tokens = [t[0] if t[0] in "+-*" else t for t in tokens]
    at = [0]

    def peek():
        return tokens[at[0]] if at[0] < len(tokens) else None

    def take(expected=None):
        token = tokens[at[0]]
        if expected is not None and token != expected:
            raise ValueError(f"expected {expected}, found {token} in {text}")
        at[0] += 1
        return token

    def binary(sub, ops):
        def go():
            left = sub()
            while peek() in ops:
                op = take()
                left = (op, left, sub())
            return left
        return go

    def unary():
        if peek() == "!":
            take()
            return ("!", unary())
        if peek() == "-":
            take()
            if peek().isdigit():
                return ("const", -int(take()))
            return ("neg", unary())
        return primary()

    def primary():
        token = take()
        if token == "(":
            inner = expr()
            take(")")
            return inner
        if token.isdigit():
            return ("const", int(token))
        m = re.fullmatch(r"(zext|sext|trunc)(\d+)", token)
        if m:
            take("(")
            inner = expr()
            take(")")
            return (m.group(1), int(m.group(2)), inner)
        m = re.fullmatch(r"(sp|s|p)(\d+)", token)
        if m:
            return (m.group(1), int(m.group(2)))
        raise ValueError(f"unknown token {token} in {text}")

    product = binary(unary, ("*", "/u", "%u"))
    total = binary(product, ("+", "-"))

    def compare():
        left = total()
        if peek() in ("==", "!=", "<s", "<=s", ">s", ">=s", "<u", "<=u", ">u", ">=u"):
            op = take()
            return (op, left, total())
        return left

    conj = binary(compare, ("&&",))
    expr = binary(conj, ("||",))
    tree = expr()
    if at[0] != len(tokens):
        raise ValueError("trailing text in " + text)
    return tree


class Undefined(Exception):
    """A term reads a variable that has no value at this state."""


def evaluate(tree, env, width=None):
    """(value, width) of a term at a state. `env` maps ("s", id), ("sp", id) and
    ("p", id) to (value, width). A constant takes the width of the other operand, or
    `width` when the whole term is one (a subscript the invariants fold to a constant)."""
    def ev(t, hint):
        kind = t[0]
        if kind == "const":
            if hint is None:
                raise ValueError("a constant with no width")
            return t[1] & ((1 << hint) - 1), hint
        if kind in ("s", "sp", "p"):
            if (kind, t[1]) not in env:
                raise Undefined()
            return env[(kind, t[1])]
        if kind in ("zext", "sext", "trunc"):
            value, width = ev(t[2], None)
            to = t[1]
            if kind == "sext":
                value = signed(value, width)
            return value & ((1 << to) - 1), to
        if kind == "neg":
            value, width = ev(t[1], hint)
            return -value & ((1 << width) - 1), width
        if kind == "!":
            value, width = ev(t[1], 1)
            return 1 - value, 1
        left, right = t[1], t[2]
        boolean = kind in ("&&", "||")
        inner = 1 if boolean else hint
        if left[0] == "const" and right[0] != "const":
            b, width = ev(right, inner)
            a, _ = ev(left, width)
        else:
            a, width = ev(left, inner)
            b, other = ev(right, width)
            if other != width:
                raise ValueError("operands of two widths")
        mask = (1 << width) - 1
        if kind == "+": return (a + b) & mask, width
        if kind == "-": return (a - b) & mask, width
        if kind == "*": return (a * b) & mask, width
        if kind == "/u": return (a // b if b else mask), width
        if kind == "%u": return (a % b if b else a), width
        if kind == "&&": return a & b, 1
        if kind == "||": return a | b, 1
        sa, sb = signed(a, width), signed(b, width)
        table = {"==": a == b, "!=": a != b, "<s": sa < sb, "<=s": sa <= sb, ">s": sa > sb,
                 ">=s": sa >= sb, "<u": a < b, "<=u": a <= b, ">u": a > b, ">=u": a >= b}
        return int(table[kind]), 1
    return ev(tree, width)


def reads(tree):
    """The ("s", id) leaves of a term."""
    if tree[0] == "s":
        return {tree}
    out = set()
    for part in tree[1:]:
        if isinstance(part, tuple):
            out |= reads(part)
    return out


# --- The report's term JSON (axes and orders) ------------------------------------------

PREDICATES = {0: lambda a, b, w: a == b, 1: lambda a, b, w: a != b,
              2: lambda a, b, w: signed(a, w) < signed(b, w), 3: lambda a, b, w: signed(a, w) <= signed(b, w),
              4: lambda a, b, w: signed(a, w) > signed(b, w), 5: lambda a, b, w: signed(a, w) >= signed(b, w),
              6: lambda a, b, w: a < b, 7: lambda a, b, w: a <= b, 8: lambda a, b, w: a > b, 9: lambda a, b, w: a >= b}


def term_value(t, env):
    width = t["width"]
    mask = (1 << width) - 1
    if "const" in t:
        return t["const"] & mask
    if "state" in t:
        if ("s", t["state"]) not in env:
            raise Undefined()
        return env[("s", t["state"])][0] & mask
    if "param" in t:
        return env[("p", t["param"])][0] & mask
    args = [term_value(a, env) for a in t["args"]]
    op = t["op"]
    if op == "add": return (args[0] + args[1]) & mask
    if op == "sub": return (args[0] - args[1]) & mask
    if op == "mul": return (args[0] * args[1]) & mask
    if op == "icmp": return int(PREDICATES[t["predicate"]](args[0], args[1], t["args"][0]["width"]))
    if op == "ite": return args[1] if args[0] else args[2]
    raise ValueError(f"the oracle does not read the operator {op}")


def term_reads(t):
    if "state" in t:
        return {("s", t["state"])}
    out = set()
    for a in t.get("args", []):
        out |= term_reads(a)
    return out


# --- Running the driver -------------------------------------------------------------

def scope_text(spec):
    return ",".join(f"{p}:{lo}:{hi}" for p, (lo, hi) in enumerate(spec["scope"]))


def scope_points(spec):
    return [list(point) for point in itertools.product(*[range(lo, hi + 1) for lo, hi in spec["scope"]])]


def run_driver(driver, spec, extra):
    with tempfile.NamedTemporaryFile("w", suffix=".mlir") as handle:
        handle.write(mlir(spec))
        handle.flush()
        command = [driver, handle.name, "--lower-structured", f"--index-bit-width={spec['iw']}",
                   f"--scope={scope_text(spec)}", "--max-level=0", "--max-candidates=2"] + extra
        done = subprocess.run(command, capture_output=True, text=True, timeout=TIMEOUT)
    if done.returncode:
        raise SystemExit(f"driver failed on {spec['name']}:\n{' '.join(command)}\n{done.stderr[-1500:]}")
    return json.loads(done.stdout)[0]


class Model:
    """What the report's names stand for in the spec, and the runs of the whole scope."""

    def __init__(self, spec, record):
        self.spec = spec
        cuts = sorted(c["cut"] for c in record["cuts"] if "note" not in c)
        if len(cuts) != 3:
            raise SystemExit(f"{spec['name']}: {len(cuts)} cuts, and the nest has three loops")
        self.level = {cut: level for level, cut in enumerate(cuts)}
        arguments = header_arguments(spec)
        constants = entry_constants(spec)
        self.role, self.constant, self.width = {}, {}, {}
        for variable in record["variables"]:
            sid = int(variable["name"][1:])
            self.width[sid] = int(variable["sort"][2:])
            m = re.fullmatch(r"argument (\d+) of block (\d+)", variable["is"])
            if m and int(m.group(2)) in self.level:
                names = arguments[self.level[int(m.group(2))]]
                if int(m.group(1)) < len(names):
                    self.role[sid] = names[int(m.group(1))]
                continue
            m = re.fullmatch(r"result of op (\d+) of block 0", variable["is"])
            if m and int(m.group(1)) < len(constants):
                self.constant[sid] = constants[int(m.group(1))]
        unknown = [v["name"] for v in record["variables"]
                   if int(v["name"][1:]) not in self.role and int(v["name"][1:]) not in self.constant]
        if unknown:
            raise SystemExit(f"{spec['name']}: the oracle cannot place {unknown}")
        self.pwidth = [int(p["sort"][2:]) for p in record["parameters"]]
        self.runs = [(point, *interpret(spec, point)) for point in scope_points(spec)]

    def env(self, state, point):
        out = {}
        for p, value in enumerate(point):
            out[("p", p)] = (value & ((1 << self.pwidth[p]) - 1), self.pwidth[p])
        for sid, (value, width) in self.constant.items():
            out[("s", sid)] = (value & ((1 << width) - 1), width)
            out[("sp", sid)] = (0, 1)
        for sid, role in self.role.items():
            if role in state:
                out[("s", sid)] = (state[role][0], self.width[sid])
                out[("sp", sid)] = (int(state[role][1]), 1)
        return out

    def name(self, role):
        for sid, r in self.role.items():
            if r == role:
                return f"s{sid}"
        raise SystemExit(f"{self.spec['name']}: no state variable is {role}")

    def cut(self, level):
        for cut, l in self.level.items():
            if l == level:
                return cut


def check_atoms(model, record, label, failures, stats):
    """Every atom beyond the termination certificate's holds at every arrival at its cut."""
    for entry in record["cuts"]:
        if "note" in entry:
            continue
        level = model.level[entry["cut"]]
        for text in entry["atoms"]:
            tree = parse(text)
            for point, _, arrivals, _ in model.runs:
                for at, state in arrivals:
                    if at != level:
                        continue
                    try:
                        value, _ = evaluate(tree, model.env(state, point))
                    except Undefined:
                        continue
                    stats["atom arrivals checked"] += 1
                    if not value:
                        failures.append(f"FAIL {label}: {text} at L{entry['cut']} is called inductive and is "
                                        f"false at an arrival of the run from {point}: {state}")
                        return


def false_somewhere(model, level, text):
    """Whether the condition is false at some arrival at the level of some run."""
    tree = parse(text)
    for point, _, arrivals, _ in model.runs:
        for at, state in arrivals:
            if at != level:
                continue
            try:
                if not evaluate(tree, model.env(state, point))[0]:
                    return True
            except Undefined:
                pass
    return False


def address(event, operand, dimension):
    return event["abc"[operand]][dimension]


def check_relations(model, candidate, axes, label, failures, stats):
    for relation in candidate["relations"]:
        operand = "ABC".index(relation["operand"])
        dimension = relation["dimension"]
        tree = parse(relation["relation"])
        what = f"{relation['operand']}[{dimension}] == {relation['relation']}"
        if relation["status"] == "PROVED":
            for point, _, _, events in model.runs:
                for position, event in enumerate(events):
                    value, width = evaluate(tree, model.env(event["state"], point), model.spec["iw"])
                    stats["relation events checked"] += 1
                    if value != address(event, operand, dimension) & ((1 << width) - 1):
                        failures.append(f"FAIL {label}: {what} is called proved; at event {position} of the "
                                        f"run from {point} the subscript is {address(event, operand, dimension)} "
                                        f"and the relation gives {value}")
                        return
        elif relation["status"] == "REFUTED":
            witness = relation["witness"]
            point = [int(v) for v in witness["parameters"]]
            status, _, events = interpret(model.spec, point)
            ok = status == "returned" and witness["event"] < len(events)
            if ok:
                event = events[witness["event"]]
                value, width = evaluate(tree, model.env(event["state"], point), model.spec["iw"])
                actual = address(event, operand, dimension)
                ok = signed(actual, width) == int(witness["subscript_in_the_run"]) and \
                    signed(value, width) == int(witness["relation_value"]) and value != actual
            stats["relation witnesses replayed"] += 1
            if not ok:
                failures.append(f"FAIL {label}: {what} is called refuted, and the witness does not replay: {witness}")
        if "over_coordinates" in relation:
            allowed = set()
            for axis in ("pik", "pkj", "pij")[operand]:
                for term in axes[axis]:
                    allowed |= term_reads(term)
            # Every coordinate of these reports is a plain state variable (level 0), so
            # "through the coordinates" is "reads only their variables".
            plain = all("state" in term for axis in "pijk" for term in axes[axis])
            if plain and relation["over_coordinates"] != (reads(tree) <= allowed):
                failures.append(f"FAIL {label}: {what} over_coordinates is {relation['over_coordinates']}, "
                                f"and it reads {sorted(reads(tree))} against {sorted(allowed)}")
            stats["over_coordinates recomputed"] += plain


def check_tier(model, found, label, failures, stats):
    """A symbolic tier is a claim about every point of the scope."""
    symbolic = found["symbolic"]
    tier = symbolic["tier"]
    if tier not in ("SYMBOLIC_VALID", "SYMBOLIC_EXACT"):
        return
    every = symbolic["holds_for"] == "every run"
    for point, status, _, events in model.runs:
        if status != "returned":
            if every:
                failures.append(f"FAIL {label}: {tier} for every run, and the run from {point} {status}")
                return
            continue
        try:
            coords = [tuple(tuple(term_value(t, model.env(e["state"], point)) for t in found["axes"][axis])
                            for axis in "pijk") for e in events]
        except Undefined:
            failures.append(f"FAIL {label}: {tier}, and a coordinate has no value at an event from {point}")
            return
        for key, view, name in ((lambda c: (c[0], c[1], c[3]), "a", "A (V1f)"),
                                (lambda c: (c[0], c[3], c[2]), "b", "B (V1g)"),
                                (lambda c: (c[0], c[1], c[2]), "c", "C (V1h)")):
            seen = {}
            for coordinate, event in zip(coords, events):
                if seen.setdefault(key(coordinate), event[view]) != event[view]:
                    failures.append(f"FAIL {label}: {tier}, and two events of the run from {point} with equal "
                                    f"coordinates have two addresses of {name}")
                    return
        if len(set(coords)) != len(coords):
            failures.append(f"FAIL {label}: {tier}, and two events of the run from {point} share coordinates")
            return
        stats["tier points checked"] += 1
        if tier == "SYMBOLIC_EXACT" and events and not orbit(model, found, point, events):
            failures.append(f"FAIL {label}: SYMBOLIC_EXACT, and the events of the run from {point} are not the "
                            f"orbit of the declared order")
            return


def orbit(model, found, point, events):
    """The events are first, succ(first), ... exactly, by the declared order."""
    levels = found["order"]
    base = model.env({}, point)

    def upper(m, values):
        top = term_value(levels[m]["upper"], base)
        if levels[m]["upper_outer"] >= 0:
            top += values[levels[m]["upper_outer"]]
        return top & ((1 << levels[m]["value"]["width"]) - 1)

    def inside(values):
        return all(term_value(levels[m]["lower"], base) <= values[m] < upper(m, values)
                   for m in range(len(levels)))

    def successor(values):
        for m in reversed(range(len(levels))):
            if values[m] + 1 < upper(m, values):
                return values[:m] + [values[m] + 1] + [term_value(levels[r]["lower"], base)
                                                       for r in range(m + 1, len(levels))]
        return None

    current = [term_value(level["lower"], base) for level in levels]
    if not inside(current):
        return False
    for position, event in enumerate(events):
        env = model.env(event["state"], point)
        if [term_value(level["value"], env) for level in levels] != current:
            return False
        following = successor(current)
        if position + 1 == len(events):
            return following is None
        if following is None or not inside(following):
            return False
        current = following
    return True


def check_report(model, report, label, given, failures, stats):
    """Everything one report claims, against the runs of the scope."""
    record = report["index_invariants"]
    status = record["status"]
    stats[f"status {status}"] += 1
    if record["automatic"] == given:
        failures.append(f"FAIL {label}: automatic is {record['automatic']} for a "
                        f"{'given' if given else 'searched'} certificate")
    strengthened = record["certificate"]["strengthened"]
    if status != "INDUCTIVE" and strengthened:
        failures.append(f"FAIL {label}: {status}, and a certificate was strengthened")
    if status == "INDUCTIVE":
        check_atoms(model, record, label, failures, stats)
    if report["termination"] == "PROVEN_TERMINATING":
        stats["proven terminating"] += 1
        for point, run, _, _ in model.runs:
            if run != "returned":
                failures.append(f"FAIL {label}: PROVEN_TERMINATING, and the run from {point} {run}")
                break
    groups = report.get("groups", [])
    for candidate in record["candidates"]:
        found = groups[candidate["group"]]["candidates"][candidate["candidate"]]
        check_relations(model, candidate, found["axes"], label, failures, stats)
    for group in groups:
        for found in group["candidates"]:
            stats[f"tier {found['symbolic']['tier']}"] += 1
            check_tier(model, found, label, failures, stats)


# --- The catalogue ---------------------------------------------------------------------

def base(name, **changes):
    spec = dict(name=name, iw=6, xw=6, J=2, init=("param", None), step=1, pre=False, nsw=False,
                carried=False, cast="zext", dn=2, da=16, db=16, scope=[(1, 2), (1, 3), (0, 3)])
    spec.update(changes)
    if spec["init"][0] == "const":
        spec["scope"] = spec["scope"][:2]
    return spec


# Hand-stated expectations: what each named case must reach. "atoms" is the least number
# of atoms beyond the termination certificate's that the search must keep; "exact" asks
# for SYMBOLIC_EXACT on the first candidate with them; "given" says the relation the spec
# implies, given by hand, must be proved.
#
# The search reads its samples over the integers, so it has candidates for a narrow
# recurrence only when the sampled runs do not wrap it. At 3 bits every sampled run
# wraps: those cases state no expectation of the search, only of the given relation.
# narrow5_wraps is the case where the samples (K up to 12) do not wrap a 5-bit x and
# the scope (K up to 70) does, many times over.
CATALOGUE = [
    (base("reset_post"), dict(atoms=1, exact=True, given=True)),
    (base("reset_pre", pre=True), dict(atoms=1, exact=True, given=True)),
    (base("reset_step3", step=3, da=20, db=20), dict(atoms=1, exact=True, given=True)),
    (base("reset_down", step=-1, scope=[(1, 2), (1, 3), (4, 7)]), dict(atoms=1, exact=True, given=True)),
    (base("constant_start", init=("const", 2)), dict(atoms=1, exact=True, given=True)),
    (base("carried", carried=True, da=24, db=24), dict(atoms=1)),
    (base("index_nsw", nsw=True), dict(atoms=1, exact=True, given=True)),
    (base("narrow5_wraps", iw=8, xw=5, J=1, dn=1, da=32, db=32, scope=[(1, 1), (1, 70), (0, 3)]),
     dict(atoms=1, exact=True, given=True)),
    (base("narrow3_wraps", xw=3, da=8, db=8, step=3, scope=[(1, 2), (1, 5), (0, 3)]), dict(given=True)),
    (base("narrow3_nsw", xw=3, da=8, db=8, nsw=True, scope=[(1, 2), (1, 5), (0, 3)]), dict(given=True)),
    (base("narrow3_sext", xw=3, cast="sext", da=8, db=8, scope=[(1, 2), (1, 3), (0, 3)]), dict(given=True)),
]


def random_spec(rnd, k):
    narrow = rnd.random() < 0.4
    xw = rnd.choice((3, 4)) if narrow else 6
    step = rnd.choice((1, 1, 2, 3, -1, -2))
    spec = base(f"random{k}", xw=xw, step=step, pre=rnd.random() < 0.4, nsw=rnd.random() < 0.3,
                carried=rnd.random() < 0.2, cast=rnd.choice(("zext", "zext", "sext")), J=rnd.choice((1, 2, 3)),
                da=rnd.choice((8, 12, 16)), db=rnd.choice((8, 12, 16)))
    # The start's range, inside the signed range of x's width: a scope is stated in
    # signed decimals, and the driver refuses a bound the width cannot hold.
    top = (1 << (xw - 1)) - 1
    low = 0 if step > 0 else rnd.choice((0, min(4, top)))
    spec["scope"] = [(1, 2), (1, rnd.choice((2, 3, 4))), (low, min(top, low + rnd.choice((2, 3, 4))))]
    if rnd.random() < 0.25:
        spec["init"] = ("const", rnd.choice((0, 1, 2)))
        spec["scope"] = spec["scope"][:2]
    return spec


def relation_text(model):
    """x at the k header in the report's names, for a recurrence that is reset: the start
    plus the step times k, at x's width."""
    spec = model.spec
    x, k = model.name("x"), model.name("k")
    counter = k if spec["xw"] == spec["iw"] else f"trunc{spec['xw']}({k})"
    start = "p2" if spec["init"][0] == "param" else str(spec["init"][1])
    c = spec["step"]
    return x, f"{start} {'+' if c >= 0 else '-'} {abs(c)} * {counter}"


def check_case(driver, spec, want, failures, stats):
    label = spec["name"]
    report = run_driver(driver, spec, ["--index-invariants"])
    record = report["index_invariants"]
    model = Model(spec, record)
    stats["programs"] += 1
    stats["scope points run"] += len(model.runs)
    stats["runs that stop"] += sum(run != "returned" for _, run, _, _ in model.runs)
    check_report(model, report, label + " (search)", False, failures, stats)
    if want is not None:
        if record["atoms_beyond_the_termination_certificate"] < want.get("atoms", 0):
            failures.append(f"FAIL {label}: expected at least {want['atoms']} atom(s), the search kept "
                            f"{record['atoms_beyond_the_termination_certificate']}")
        tier = report["groups"][0]["candidates"][0]["symbolic"]["tier"] if report["groups"] and \
            report["groups"][0]["candidates"] else "none"
        if want.get("exact") and tier != "SYMBOLIC_EXACT":
            failures.append(f"FAIL {label}: expected SYMBOLIC_EXACT with the invariants, found {tier}")
        stats["hand expectations"] += 1
    if spec["carried"]:
        return
    # Given certificates, in the report's own names: the relation the spec implies, and
    # three wrong ones. A wrong one that is false at some arrival must never be INDUCTIVE.
    x, right = relation_text(model)
    cut = model.cut(2)
    for kind, text in (("right", f"{x} == {right}"), ("offset", f"{x} == {right} + 1"),
                       ("stale", f"{x} == {right.split(' ')[0]}"), ("slope", f"{x} == {right} + {model.name('k') if spec['xw'] == spec['iw'] else 'trunc%d(%s)' % (spec['xw'], model.name('k'))}")):
        given = run_driver(driver, spec, [f"--index-invariant={cut}:{text}"])
        name = f"{label} (given {kind}: {text})"
        false = false_somewhere(model, 2, text)
        status = given["index_invariants"]["status"]
        stats[f"given {'false' if false else 'true'} {status}"] += 1
        if false and status == "INDUCTIVE":
            failures.append(f"FAIL {name}: false at an arrival of a run, and called inductive")
        if kind == "right" and false:
            raise SystemExit(f"{name}: the oracle's own relation is false at an arrival")
        if kind == "right" and want is not None and want.get("given") and status != "INDUCTIVE":
            failures.append(f"FAIL {name}: expected INDUCTIVE, found {status}")
        check_report(model, given, name, True, failures, stats)
    # A wrong index relation beside the proposed ones: A's K subscript plus one.
    proposed = [r for c in record["candidates"] for r in c["relations"]
                if r["operand"] == "A" and r["dimension"] == 1 and r["status"] == "PROVED"]
    if proposed:
        wrong = f"0:A:1:{proposed[0]['relation']} + 1"
        claimed = run_driver(driver, spec, ["--index-invariants", f"--index-relation={wrong}"])
        verdicts = [r["status"] for c in claimed["index_invariants"]["candidates"] for r in c["relations"]
                    if r["origin"] == "given"]
        has_event = any(events for _, _, _, events in model.runs)
        for verdict in verdicts:
            stats[f"wrong relation {verdict}"] += 1
            if verdict == "PROVED" and has_event:
                failures.append(f"FAIL {label}: the relation {wrong} is called proved")
        check_report(model, claimed, f"{label} (relation {wrong})", False, failures, stats)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--driver", required=True)
    parser.add_argument("--programs", type=int, default=12, help="random specs beside the catalogue")
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--only", default="", help="comma separated case names")
    parser.add_argument("--print", action="store_true", help="print each spec's MLIR and stop")
    parser.add_argument("--summary", default="")
    args = parser.parse_args()
    rnd = random.Random(args.seed)
    cases = list(CATALOGUE) + [(random_spec(rnd, k), None) for k in range(args.programs)]
    if args.only:
        cases = [case for case in cases if case[0]["name"] in args.only.split(",")]
    if args.print:
        for spec, _ in cases:
            print(f"// {spec}\n{mlir(spec)}")
        return 0
    failures = []

    class Tally(dict):
        def __missing__(self, key):
            return 0
    stats = Tally()
    lines = []
    for spec, want in cases:
        before = len(failures)
        check_case(args.driver, spec, want, failures, stats)
        lines.append(f"  {'FAIL' if len(failures) > before else 'ok  '} {spec['name']}")
    print("index invariant oracle: " + ", ".join(f"{k} {v}" for k, v in sorted(stats.items())))
    print("\n".join(lines))
    for failure in failures:
        print(failure)
    print(f"{len(failures)} problems")
    if args.summary:
        Path(args.summary).write_text(json.dumps(dict(stats=dict(stats), failures=failures), indent=1))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
