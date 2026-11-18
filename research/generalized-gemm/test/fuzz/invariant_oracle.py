#!/usr/bin/env python3
"""Independent oracle of the inductive invariant path (statement BV record, Section 13).

Generates scalar loops in the MLIR subset the path supports, at widths 3 and 4, with
correct and wrong function candidates, runs the synthesis report with
--loop-invariants, and checks every claim with its own parser and interpreter of
that subset (wrapping arithmetic, nsw poison, a branch on poison stops the run) and
its own evaluator of the report's term text. No SMT solver and no code of this
project is used to decide anything:

  PROVED_BY_INVARIANT  every input of the scope is run to the end; every run that
                       returns, returns the candidate's bits; when the claim says
                       every run returns, every run does; when poison is said to be
                       excluded, no returned value is poison. At width 3 the
                       certificate's INV-init, INV-step and INV-exit are also checked
                       by brute force over every header state and parameter.
  REFUTED              the counterexample's run returns another value than the
                       candidate, and the value the report says.
  NOT_INDUCTIVE        (a given certificate) brute force finds the violation.
  anything else        no claim; counted, and a correct candidate left unproved is
                       incompleteness, not an error.

Exit 1 with "FAIL ..." lines when a claim is wrong.
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


# --- The MLIR subset ------------------------------------------------------------

class Program:
    def __init__(self, text, width):
        self.width = width
        self.blocks = []  # (name, [args], [ops], terminator)
        lines = [l.strip() for l in text.splitlines() if l.strip() and not l.strip().startswith("//")]
        header = lines[0]
        self.params = re.findall(r"%(\w+): i\d+", header.split("->")[0])
        current = ("entry", [], [], None)
        for line in lines[1:]:
            if line == "}":
                break
            m = re.match(r"\^(\w+)(?:\((.*)\))?:$", line)
            if m:
                self.blocks.append(current)
                args = re.findall(r"%(\w+):", m.group(2) or "")
                current = (m.group(1), args, [], None)
                continue
            if line.startswith(("cf.br", "cf.cond_br", "return")):
                current = (current[0], current[1], current[2], line)
            else:
                current[2].append(line)
        self.blocks.append(current)
        self.index = {b[0]: k for k, b in enumerate(self.blocks)}

    def mask(self, v):
        return v & ((1 << self.width) - 1)

    def signed(self, v):
        v = self.mask(v)
        return v - (1 << self.width) if v >> (self.width - 1) else v

    def op(self, line, env, poison):
        m = re.match(r"%(\w+) = arith\.constant (-?\d+|true|false) : i\d+", line)
        if m:
            value = {"true": 1, "false": 0}.get(m.group(2))
            env[m.group(1)] = self.mask(int(m.group(2))) if value is None else value
            poison[m.group(1)] = False
            return
        m = re.match(r"%(\w+) = arith\.(addi|subi|muli) %(\w+), %(\w+)( overflow<nsw>)? : i\d+", line)
        if m:
            d, kind, a, b, nsw = m.groups()
            x, y = env[a], env[b]
            exact = {"addi": self.signed(x) + self.signed(y), "subi": self.signed(x) - self.signed(y),
                     "muli": self.signed(x) * self.signed(y)}[kind]
            env[d] = self.mask(exact)
            overflow = bool(nsw) and self.signed(exact) != exact
            poison[d] = poison[a] or poison[b] or overflow
            return
        m = re.match(r"%(\w+) = arith\.cmpi (\w+), %(\w+), %(\w+) : i\d+", line)
        if m:
            d, p, a, b = m.groups()
            x, y = env[a], env[b]
            sx, sy = self.signed(x), self.signed(y)
            env[d] = int({"eq": x == y, "ne": x != y, "slt": sx < sy, "sle": sx <= sy, "sgt": sx > sy,
                          "sge": sx >= sy, "ult": x < y, "ule": x <= y, "ugt": x > y, "uge": x >= y}[p])
            poison[d] = poison[a] or poison[b]
            return
        raise ValueError("unsupported op: " + line)

    def run_from(self, block, args, params, fuel, stop_at=None):
        """('return', value, poison) | ('arrive', args, poisons) | ('ub',) | ('fuel',)."""
        env = dict(zip(self.params, params))
        poison = {p: False for p in self.params}
        if block != 0:
            # Values the entry block defines (constants, pure arithmetic) are in scope
            # at every later block.
            for line in self.blocks[0][2]:
                self.op(line, env, poison)
        name, names, ops, term = self.blocks[block]
        for n, (v, q) in zip(names, args):
            env[n], poison[n] = v, q
        first = True
        while fuel > 0:
            fuel -= 1
            name, names, ops, term = self.blocks[block]
            if not first and stop_at is not None and block == stop_at:
                return ("arrive", [(env[n], poison[n]) for n in names])
            first = False
            for line in ops:
                self.op(line, env, poison)
            if term.startswith("return"):
                r = re.match(r"return %(\w+)", term).group(1)
                return ("return", env[r], poison[r])
            if term.startswith("cf.cond_br"):
                m = re.match(r"cf\.cond_br %(\w+), \^(\w+)(?:\((.*?)\))?, \^(\w+)(?:\((.*?)\))?", term)
                c = m.group(1)
                if poison[c]:
                    return ("ub",)
                target, operands = (m.group(2), m.group(3)) if env[c] else (m.group(4), m.group(5))
            else:
                m = re.match(r"cf\.br \^(\w+)(?:\((.*?)\))?", term)
                target, operands = m.group(1), m.group(2)
            values = re.findall(r"%(\w+)", (operands or "").split(":")[0])
            block = self.index[target]
            new = [(env[v], poison[v]) for v in values]
            for n, (v, q) in zip(self.blocks[block][1], new):
                env[n], poison[n] = v, q
            if stop_at is not None and block == stop_at:
                return ("arrive", new)
        return ("fuel",)

    def run(self, params, fuel=100000):
        return self.run_from(0, [], params, fuel)


# --- The report's term text -----------------------------------------------------

TOKEN = re.compile(r"\s*(<=s|<=u|>=s|>=u|>>u|>>s|==|!=|&&|\|\||/u|/s|%u|%s|<<|<s|<u|>s|>u|"
                   r"[-+*](?:nsw|nuw)*|[&|^!~(),]|[A-Za-z_]\w*|\d+)")


def parse_term(text):
    tokens, pos = [], 0
    while pos < len(text):
        m = TOKEN.match(text, pos)
        if not m or m.end() == pos:
            if text[pos:].strip() == "":
                break
            raise ValueError("cannot read " + text[pos:])
        tokens.append(m.group(1))
        pos = m.end()
    tokens = [re.sub(r"(nsw|nuw)+$", "", t) if t[0] in "+-*" else t for t in tokens]
    at = [0]

    def peek():
        return tokens[at[0]] if at[0] < len(tokens) else None

    def take():
        at[0] += 1
        return tokens[at[0] - 1]

    def binary(sub, ops):
        def parse():
            left = sub()
            while peek() in ops:
                op = take()
                left = (op, left, sub())
            return left
        return parse

    def unary():
        t = peek()
        if t in ("!", "~"):
            take()
            return (t, unary())
        if t == "-":
            take()
            if peek() and peek().isdigit():
                return ("const", -int(take()))
            return ("neg", unary())
        return primary()

    def primary():
        t = take()
        if t == "(":
            inner = expr()
            assert take() == ")"
            return inner
        if t.isdigit():
            return ("const", int(t))
        if t in ("true", "false"):
            return ("bool", int(t == "true"))
        if t == "ite":
            assert take() == "("
            c = expr()
            assert take() == ","
            a = expr()
            assert take() == ","
            b = expr()
            assert take() == ")"
            return ("ite", c, a, b)
        m = re.fullmatch(r"(sp|s|p)(\d+)", t)
        if m:
            return (m.group(1), int(m.group(2)))
        raise ValueError("unknown name " + t)

    product = binary(unary, ("*", "/u", "/s", "%u", "%s"))
    total = binary(product, ("+", "-"))
    shift = binary(total, ("<<", ">>u", ">>s"))
    bits = binary(shift, ("&", "|", "^"))

    def compare():
        left = bits()
        if peek() in ("==", "!=", "<s", "<=s", ">s", ">=s", "<u", "<=u", ">u", ">=u"):
            op = take()
            return (op, left, bits())
        return left

    conj = binary(compare, ("&&",))
    expr = binary(conj, ("||",))
    tree = expr()
    if at[0] != len(tokens):
        raise ValueError("trailing text in " + text)
    return tree


def evaluate(tree, width, state, shadow, params):
    mask = (1 << width) - 1

    def signed(v):
        v &= mask
        return v - (1 << width) if v >> (width - 1) else v

    def ev(t):
        k = t[0]
        if k == "const":
            return t[1] & mask
        if k == "bool":
            return t[1]
        if k == "s":
            return state[t[1]]
        if k == "sp":
            return int(shadow[t[1]])
        if k == "p":
            return params[t[1]]
        if k == "neg":
            return -ev(t[1]) & mask
        if k == "!":
            return 1 - ev(t[1])
        if k == "~":
            return ~ev(t[1]) & mask
        if k == "ite":
            return ev(t[2]) if ev(t[1]) else ev(t[3])
        a, b = ev(t[1]), ev(t[2])
        if k == "+": return (a + b) & mask
        if k == "-": return (a - b) & mask
        if k == "*": return (a * b) & mask
        if k == "/u": return (a // b) & mask if b else mask
        if k == "%u": return a % b if b else a
        if k == "&&": return a & b
        if k == "||": return a | b
        if k == "&": return a & b
        if k == "|": return a | b
        if k == "^": return a ^ b
        if k == "==": return int(a == b)
        if k == "!=": return int(a != b)
        if k == "<s": return int(signed(a) < signed(b))
        if k == "<=s": return int(signed(a) <= signed(b))
        if k == ">s": return int(signed(a) > signed(b))
        if k == ">=s": return int(signed(a) >= signed(b))
        if k == "<u": return int(a < b)
        if k == "<=u": return int(a <= b)
        if k == ">u": return int(a > b)
        if k == ">=u": return int(a >= b)
        raise ValueError("operator " + k)

    return ev(tree)


# --- The catalogue --------------------------------------------------------------

def program_text(width, a, b, c, pre, nsw, early):
    t = f"i{width}"
    f = " overflow<nsw>" if nsw else ""
    lines = [f"func.func @f(%n: {t}, %x0: {t}, %g0: {t}) -> {t} {{",
             f"  %c0 = arith.constant 0 : {t}"]
    if early:
        lines += [f"  %neg = arith.cmpi slt, %n, %c0 : {t}", "  cf.cond_br %neg, ^early, ^start",
                  "^early:", f"  return %g0 : {t}", "^start:"]
    lines += [f"  cf.br ^header(%c0, %x0, %g0 : {t}, {t}, {t})",
              f"^header(%i: {t}, %x: {t}, %g: {t}):",
              f"  %more = arith.cmpi slt, %i, %n : {t}",
              "  cf.cond_br %more, ^body, ^done", "^body:",
              f"  %ca = arith.constant {a} : {t}", f"  %cb = arith.constant {b} : {t}",
              f"  %cc = arith.constant {c} : {t}", f"  %one = arith.constant 1 : {t}"]
    if pre:
        lines += [f"  %x1 = arith.addi %x, %cc{f} : {t}", f"  %t = arith.muli %ca, %x1{f} : {t}"]
    else:
        lines += [f"  %t = arith.muli %ca, %x{f} : {t}", f"  %x1 = arith.addi %x, %cc{f} : {t}"]
    lines += [f"  %u = arith.addi %t, %cb{f} : {t}", f"  %g1 = arith.addi %g, %u{f} : {t}",
              f"  %i1 = arith.addi %i, %one{f} : {t}",
              f"  cf.br ^header(%i1, %x1, %g1 : {t}, {t}, {t})", "^done:", f"  return %g : {t}", "}"]
    return "\n".join(lines) + "\n"


def coupled_text(width, c, nsw):
    """x += c, y += x (old x): y = y0 + N*x0 + c*N*(N-1)/2."""
    t = f"i{width}"
    f = " overflow<nsw>" if nsw else ""
    return "\n".join([
        f"func.func @f(%n: {t}, %x0: {t}, %g0: {t}) -> {t} {{",
        f"  %c0 = arith.constant 0 : {t}", f"  %one = arith.constant 1 : {t}", f"  %cc = arith.constant {c} : {t}",
        f"  cf.br ^header(%c0, %x0, %g0 : {t}, {t}, {t})",
        f"^header(%i: {t}, %x: {t}, %g: {t}):",
        f"  %more = arith.cmpi slt, %i, %n : {t}", "  cf.cond_br %more, ^body, ^done", "^body:",
        f"  %x1 = arith.addi %x, %cc{f} : {t}", f"  %g1 = arith.addi %g, %x{f} : {t}",
        f"  %i1 = arith.addi %i, %one{f} : {t}",
        f"  cf.br ^header(%i1, %x1, %g1 : {t}, {t}, {t})", "^done:", f"  return %g : {t}", "}"]) + "\n"


def candidate(a, b, c, pre):
    """g0 + sum over k < N of a * x_k + b, with x_k = x0 + c*k (post) or x0 + c*(k+1) (pre)."""
    if (a, b, c) == (2, 1, 1):
        return "p2 + (p1 + p0 + 1) * (p1 + p0 + 1) - (p1 + 1) * (p1 + 1)" if pre else \
               "p2 + (p1 + p0) * (p1 + p0) - p1 * p1"
    h = c // 2
    shift = "(p0 + 1)" if pre else "(p0 - 1)"
    return f"p2 + {a} * p0 * p1 + {a * h} * p0 * {shift} + {b} * p0"


def cases(seed, count):
    rnd = random.Random(seed)
    out = []
    shapes = [(2, 1, 1)] + [(a, b, c) for a in (1, 2, 3) for b in (0, 1, 3) for c in (0, 2)]
    for k in range(count):
        width = 3 if k % 2 == 0 else 4
        a, b, c = shapes[0] if k < 4 else rnd.choice(shapes)
        pre, nsw, early = rnd.random() < 0.3, rnd.random() < 0.3, rnd.random() < 0.25
        right = candidate(a, b, c, pre)
        if early:
            right = f"ite(p0 <s 0, p2, {right})"
        kind = rnd.choice(["right", "right", "wrong-constant", "wrong-hidden", "wrong-order"])
        text = {"right": right, "wrong-constant": f"({right}) + 1",
                "wrong-hidden": f"({right}) + ite(p1 == {rnd.randrange(1, 1 << (width - 1))}, 1, 0)",
                "wrong-order": f"ite(p0 <s 0, p2, {candidate(a, b, c, not pre)})" if early
                               else candidate(a, b, c, not pre)}[kind]
        out.append(dict(width=width, a=a, b=b, c=c, pre=pre, nsw=nsw, early=early, kind=kind,
                        candidate=text, scope=None if early else f"0:0:{(1 << (width - 1)) - 1}"))
    # Two coupled variables (M1), and cases with no formula at all (mode B: the formula
    # is synthesized and then checked here like any other).
    for k in range(max(4, count // 3)):
        width = 3 if k % 2 == 0 else 4
        c = rnd.choice((0, 2))
        nsw = rnd.random() < 0.3
        right = f"p2 + p0 * p1 + {c // 2} * p0 * (p0 - 1)"
        kind = rnd.choice(["right", "wrong-constant", "auto", "auto"])
        text = {"right": right, "wrong-constant": f"({right}) + 1", "auto": None}[kind]
        out.append(dict(width=width, a=0, b=0, c=c, pre=False, nsw=nsw, early=False, kind="coupled " + kind,
                        candidate=text, scope=f"0:0:{(1 << (width - 1)) - 1}", coupled=True))
    for k in range(max(2, count // 6)):
        width = 3 if k % 2 == 0 else 4
        a, b, c = rnd.choice([(1, 0, 0), (2, 1, 0), (1, 1, 2), (3, 0, 0)])
        out.append(dict(width=width, a=a, b=b, c=c, pre=False, nsw=False, early=False, kind="auto",
                        candidate=None, scope=f"0:0:{(1 << (width - 1)) - 1}"))
    # Given certificates: the request's (right) and broken ones, at width 3.
    inv = "(0 <=s s0) && (s0 <=s p0) && (s1 == p1 + s0) && (s2 - p2 == s1 * s1 - p1 * p1)"
    for name, text in [("request", inv),
                       ("missing term", "(0 <=s s0) && (s0 <=s p0) && (s1 == p1 + s0) && (s2 - p2 == s1 * s1)"),
                       ("double step", "(0 <=s s0) && (s0 <=s p0) && (s1 == p1 + 2 * s0)"),
                       ("no bounds", "(s1 == p1 + s0) && (s2 - p2 == s1 * s1 - p1 * p1)")]:
        out.append(dict(width=3, a=2, b=1, c=1, pre=False, nsw=False, early=False, kind="given " + name,
                        candidate=candidate(2, 1, 1, False), scope="0:0:3", invariant=text))
    return out


# --- Checks ---------------------------------------------------------------------

def scope_points(width, scope, nparams):
    lo, hi = -(1 << (width - 1)), (1 << (width - 1)) - 1
    ranges = [range(lo, hi + 1)] * nparams
    if scope:
        p, l, h = (int(x) for x in scope.split(":"))
        ranges = list(ranges)
        ranges[p] = range(l, h + 1)
    mask = (1 << width) - 1
    for point in itertools.product(*ranges):
        yield [v & mask for v in point]


def header_map(spec, program):
    """s<id> -> ("arg", block, index) for a block argument, or ("const", value) for a
    constant defined by an arith.constant op, from the record's own description of the
    variable and this oracle's own reading of the program."""
    out = {}
    for var in spec["variables"]:
        sid = int(var["name"][1:])
        m = re.fullmatch(r"argument (\d+) of block (\d+)", var["is"])
        if m:
            out[sid] = ("arg", int(m.group(2)), int(m.group(1)))
            continue
        m = re.fullmatch(r"result of op (\d+) of block (\d+)", var["is"])
        if not m:
            return None
        ops = program.blocks[int(m.group(2))][2]
        k = int(m.group(1))
        c = re.match(r"%\w+ = arith\.constant (-?\d+) : i\d+", ops[k]) if k < len(ops) else None
        if not c:
            return None
        out[sid] = ("const", program.mask(int(c.group(1))))
    return out


def brute_force(program, spec, atoms, cand, scope, failures, label, expect):
    """INV-init, INV-step and INV-exit over every header state and parameter, at width 3."""
    width = program.width
    hm = header_map(spec, program)
    if hm is None:
        return "skipped: a variable is neither a block argument nor a constant"
    blocks = {v[1] for v in hm.values() if v[0] == "arg"}
    if len(blocks) != 1:
        return "skipped"
    header = blocks.pop()
    arity = len(program.blocks[header][1])
    args_ids = sorted(i for i, v in hm.items() if v[0] == "arg")
    if sorted(hm[i][2] for i in args_ids) != list(range(arity)):
        return "skipped: a header argument is not in the record"
    consts = {i: v[1] for i, v in hm.items() if v[0] == "const"}
    trees = [parse_term(a) for a in atoms]
    f = parse_term(cand)
    nvals = 1 << width

    def inv(state, shadow, params):
        full = {**consts, **state}
        fullshadow = {**{i: False for i in consts}, **shadow}
        return all(evaluate(t, width, full, fullshadow, params) for t in trees)

    broken = {"INV-init": 0, "INV-step": 0, "INV-exit": 0}
    points = list(scope_points(width, scope, len(program.params)))
    for params in points:
        # INV-init: the state at the first arrival.
        r = program.run_from(0, [], params, 1000, stop_at=header)
        if r[0] == "arrive":
            state = {i: r[1][hm[i][2]][0] for i in args_ids}
            shadow = {i: r[1][hm[i][2]][1] for i in args_ids}
            if not inv(state, shadow, params):
                broken["INV-init"] += 1
        for values in itertools.product(range(nvals), repeat=len(args_ids)):
            state = dict(zip(args_ids, values))
            shadow = {i: False for i in args_ids}
            if not inv(state, shadow, params):
                continue
            args = [None] * arity
            for i in args_ids:
                args[hm[i][2]] = (state[i], False)
            r = program.run_from(header, args, params, 1000, stop_at=header)
            if r[0] == "arrive":
                nstate = {i: r[1][hm[i][2]][0] for i in args_ids}
                nshadow = {i: r[1][hm[i][2]][1] for i in args_ids}
                if not inv(nstate, nshadow, params):
                    broken["INV-step"] += 1
            elif r[0] == "return":
                if r[1] != evaluate(f, width, {}, {}, params):
                    broken["INV-exit"] += 1
    if expect == "proved" and any(broken.values()):
        failures.append(f"FAIL {label}: a proved certificate fails by brute force: {broken}")
    return broken


def check(driver, case, failures, stats, keep):
    width = case["width"]
    if case.get("coupled"):
        text = coupled_text(width, case["c"], case["nsw"])
    else:
        text = program_text(width, case["a"], case["b"], case["c"], case["pre"], case["nsw"], case["early"])
    program = Program(text, width)
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "loop.mlir"
        path.write_text(text)
        command = [driver, str(path), "--loop-invariants", "--timeout-ms=20000"]
        if case["candidate"] is not None:
            command.append(f"--loop-function=0:{case['candidate']}")
        if case["scope"]:
            command.append(f"--scope={case['scope']}")
        if case.get("invariant"):
            command.append(f"--loop-invariant={case['invariant']}")
        done = subprocess.run(command, capture_output=True, text=True, timeout=1800)
    label = f"{case['kind']} w{width} a{case['a']} b{case['b']} c{case['c']}" + \
            (" coupled" if case.get("coupled") else "") + \
            (" pre" if case["pre"] else "") + (" nsw" if case["nsw"] else "") + (" early" if case["early"] else "")
    if done.returncode:
        failures.append(f"FAIL {label}: the report exited {done.returncode}: {done.stderr[-300:]}")
        return
    record = json.loads(done.stdout)[0]["loop_invariant"]
    status = record["status"]
    stats[status] = stats.get(status, 0) + 1
    formula = case["candidate"]
    if formula is None:
        # Mode B: the formula the report synthesized, when it has one.
        results = record.get("function", {}).get("results", [])
        if not results:
            keep.append(dict(label=label, status=status, truth="no formula"))
            if status in ("PROVED_BY_INVARIANT", "REFUTED"):
                failures.append(f"FAIL {label}: {status} without a formula")
            return
        formula = results[0]["candidate"]
        stats["synthesized formulas"] = stats.get("synthesized formulas", 0) + 1
    case = dict(case, candidate=formula)
    f = parse_term(formula)
    # The truth, by running every input of the scope.
    runs, mismatch, returns_all, poisoned = 0, None, True, False
    for params in scope_points(width, case["scope"], 3):
        r = program.run(params)
        runs += 1
        if r[0] != "return":
            returns_all = False
            continue
        poisoned = poisoned or r[2]
        if r[1] != evaluate(f, width, {}, {}, params) and mismatch is None:
            mismatch = params
    truth = "right" if mismatch is None else "wrong"
    keep.append(dict(label=label, status=status, truth=truth))
    if status == "PROVED_BY_INVARIANT":
        if mismatch is not None:
            failures.append(f"FAIL {label}: PROVED, but the run at {mismatch} returns another value")
        if "every run returns" in record["claim"] and not returns_all:
            failures.append(f"FAIL {label}: the claim says every run returns; one does not")
        if record["poison"] == "excluded" and poisoned:
            failures.append(f"FAIL {label}: poison is said to be excluded; a run returns poison")
        if width == 3 and record.get("certificate"):
            result = brute_force(program, record["specification"], record["certificate"]["atoms"],
                                 case["candidate"], case["scope"], failures, label, "proved")
            stats["certificates brute-forced"] = stats.get("certificates brute-forced", 0) + \
                (0 if isinstance(result, str) else 1)
    elif status == "REFUTED":
        cex = record.get("counterexample")
        if not cex:
            failures.append(f"FAIL {label}: REFUTED without a counterexample")
            return
        mask = (1 << width) - 1
        params = [int(v) & mask for v in cex["parameters"]]
        r = program.run(params)
        if r[0] != "return" or r[1] == evaluate(f, width, {}, {}, params):
            failures.append(f"FAIL {label}: the counterexample {params} does not refute: {r}")
        elif r[1] != int(cex["returned_by_the_run"][0]) & mask:
            failures.append(f"FAIL {label}: the counterexample's recorded value differs from the run")
    elif case.get("invariant") and record["invariant"]["verdict"] == "NOT_INDUCTIVE":
        broken = brute_force(program, record["specification"], record["certificate"]["atoms"],
                             case["candidate"], case["scope"], failures, label, "refused")
        if not isinstance(broken, str):
            stats["refusals brute-forced"] = stats.get("refusals brute-forced", 0) + 1
        if not isinstance(broken, str) and not broken[record["invariant"]["falsified_by"]]:
            failures.append(f"FAIL {label}: {record['invariant']['falsified_by']} was reported "
                            f"violated, and brute force finds no violation: {broken}")
    if status in ("PROVED_BY_INVARIANT",) and truth == "wrong":
        pass  # Already a failure above.
    if status == "REFUTED" and truth == "right":
        failures.append(f"FAIL {label}: a right candidate was refuted")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--driver", required=True)
    parser.add_argument("--cases", type=int, default=24)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--summary", default="")
    args = parser.parse_args()
    failures, stats, keep = [], {}, []
    for case in cases(args.seed, args.cases):
        check(args.driver, case, failures, stats, keep)
    right = [k for k in keep if k["truth"] == "right"]
    proved = [k for k in right if k["status"] == "PROVED_BY_INVARIANT"]
    wrong = [k for k in keep if k["truth"] == "wrong"]
    refuted = [k for k in wrong if k["status"] == "REFUTED"]
    print(f"cases {len(keep)}; right candidates {len(right)}, proved {len(proved)}; "
          f"wrong candidates {len(wrong)}, refuted {len(refuted)}; statuses {stats}")
    for k in keep:
        print(f"  {k['status']:20} {k['truth']:5} {k['label']}")
    if args.summary:
        Path(args.summary).write_text(json.dumps(dict(stats=stats, cases=keep), indent=1))
    for f in failures:
        print(f, file=sys.stderr)
    print(f"{len(failures)} problems")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
