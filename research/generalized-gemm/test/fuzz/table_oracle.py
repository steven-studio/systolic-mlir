#!/usr/bin/env python3
"""Phase 5 oracle for the table programs (Appendix G.5, bit-vector adaptation).

Independent of the library: a family of reductions whose column index is a linear
recurrence with history, f(m+1) = a*f(m) + b*f(m-1) + c (with f(0), f(1) given), is
written as MLIR here, the driver is run with --slices, and every claim about the
synthesized index program of the subscript is recomputed in plain Python:

- The expected subscript sequence f(0..n-1) at every point of the scope, by this
  script's own arithmetic at the width, from the recurrence it generated. The
  driver never sees that recurrence as a formula, only the MLIR.
- The reported program text, parsed by this script's own reader of the statement
  grammar (arrays included), is run by this script's own interpreter of the G.5
  rules (partial maps, bounds, uninitialized reads) at every m of every point.

The rules of the record are then checked, never taken from the driver:
  PROVED and SAMPLE_CONSISTENT  the program's value equals the expected f(m) at every
                                m of every point of the scope, sampled or not, and
                                the program has a rule there (no NoRule).
  PROVED                        additionally at every point of the scope, including
                                points the driver did not run.
  REFUTED                       the program really differs from (or has no rule at)
                                the expected sequence at the model's m.
  UNKNOWN                       nothing is required.
A table program must match the template text (array T0[(m + d)] ; ...), since that is
what the driver claims to have searched. The hand-written reference programs of the
unit tests are not used here: the oracle only sees what the driver synthesized.
"""
import argparse
import json
import random
import re
import subprocess
import sys
import tempfile
from pathlib import Path

WIDTH = 8
MASK = (1 << WIDTH) - 1


def spec_sequence(a, b, c, f0, f1, n):
    """f(0..n-1) at the width, from the recurrence."""
    seq = []
    for m in range(n):
        if m == 0:
            seq.append(f0 & MASK)
        elif m == 1:
            seq.append(f1 & MASK)
        else:
            seq.append((a * seq[m - 1] + b * seq[m - 2] + c) & MASK)
    return seq


def mlir(name, a, b, c, f0, f1, cols):
    """A reduction C[0][j] += A[0][f] * B[f][j] over k < n with f, g carried."""
    lines = [f"func.func @{name}(%a: memref<1x{cols}xi32>, %b: memref<{cols}x2xi32>, %c: memref<1x2xi32>, %n: index) {{",
             "  %c0 = arith.constant 0 : index", "  %c1 = arith.constant 1 : index", "  %c2 = arith.constant 2 : index",
             f"  %ca = arith.constant {a} : index", f"  %cb = arith.constant {b} : index", f"  %cc = arith.constant {c} : index",
             f"  %f0 = arith.constant {f0} : index", f"  %f1 = arith.constant {f1} : index",
             "  scf.for %j = %c0 to %c2 step %c1 {",
             "    %r:2 = scf.for %k = %c0 to %n step %c1 iter_args(%f = %f0, %g = %f1) -> (index, index) {",
             # g holds f(m+1) one step ahead: f' = g, g' = a*g + b*f + c.
             f"      %x = memref.load %a[%c0, %f] : memref<1x{cols}xi32>",
             f"      %y = memref.load %b[%f, %j] : memref<{cols}x2xi32>",
             "      %z = memref.load %c[%c0, %j] : memref<1x2xi32>",
             "      %m = arith.muli %x, %y : i32", "      %s = arith.addi %z, %m : i32",
             "      memref.store %s, %c[%c0, %j] : memref<1x2xi32>",
             "      %ag = arith.muli %ca, %g : index", "      %bf = arith.muli %cb, %f : index",
             "      %t = arith.addi %ag, %bf : index", "      %ng = arith.addi %t, %cc : index",
             "      scf.yield %g, %ng : index, index", "    }", "  }", "  return", "}"]
    return "\n".join(lines) + "\n"


# --- the statement grammar, read back from the driver's text ---------------------

TOKEN = re.compile(r"\s*(:=|<=|[A-Za-z_][A-Za-z_0-9]*|\d+|[()\[\];<=+*-])")


def tokenize(text):
    out, pos = [], 0
    while pos < len(text):
        m = TOKEN.match(text, pos)
        if not m:
            raise ValueError("bad text at %r" % text[pos:pos + 20])
        out.append(m.group(1))
        pos = m.end()
    return out


class Program:
    def __init__(self):
        self.arrays = []   # (name, [extent expr], is_input)
        self.body = None
        self.result = None


class Parser:
    """Parses the formatIndexProgram text: `array T0[e] ; S ; return E`."""

    def __init__(self, text):
        self.t = tokenize(text)
        self.i = 0

    def peek(self, k=0):
        return self.t[self.i + k] if self.i + k < len(self.t) else ""

    def take(self, want=None):
        if self.i >= len(self.t):
            raise ValueError("unexpected end")
        tok = self.t[self.i]
        if want is not None and tok != want:
            raise ValueError("expected %r got %r" % (want, tok))
        self.i += 1
        return tok

    def program(self):
        p = Program()
        while self.peek() in ("array", "input"):
            is_input = self.peek() == "input"
            if is_input:
                self.take("input")
            self.take("array")
            name = self.take()
            extents = []
            while self.peek() == "[":
                self.take("[")
                extents.append(self.expr())
                self.take("]")
            p.arrays.append((name, extents, is_input))
            self.take(";")
        stmts = []
        while True:
            if self.peek() == "return":
                break
            stmts.append(self.stmt())
            if self.peek() == ";":
                self.take(";")
        self.take("return")
        p.result = self.expr()
        p.body = ("seq", stmts)
        if self.i != len(self.t):
            raise ValueError("trailing tokens")
        return p

    def stmt(self):
        tok = self.peek()
        if tok == "skip":
            self.take()
            return ("skip",)
        if tok == "if":
            self.take("if")
            cond = self.cond()
            self.take("then")
            self.take("(")
            a = self.block()
            self.take(")")
            self.take("else")
            self.take("(")
            b = self.block()
            self.take(")")
            return ("if", cond, a, b)
        if tok == "while":
            self.take("while")
            cond = self.cond()
            self.take("do")
            self.take("(")
            a = self.block()
            self.take(")")
            return ("while", cond, a)
        if tok.startswith("T") and self.peek(1) == "[":
            name = self.take()
            idx = []
            while self.peek() == "[":
                self.take("[")
                idx.append(self.expr())
                self.take("]")
            self.take(":=")
            return ("write", name, idx, self.expr())
        name = self.take()
        self.take(":=")
        return ("assign", name, self.expr())

    def block(self):
        stmts = [self.stmt()]
        while self.peek() == ";":
            self.take(";")
            stmts.append(self.stmt())
        return ("seq", stmts)

    def cond(self):
        if self.peek() == "not":
            self.take("not")
            self.take("(")
            c = self.cond()
            self.take(")")
            return ("not", c)
        if self.peek() == "(" and self._paren_is_cond():
            self.take("(")
            left = self.cond()
            self.take(")")
            op = self.take()
            self.take("(")
            right = self.cond()
            self.take(")")
            return (op, left, right)
        a = self.expr()
        op = self.take()
        if op not in ("=", "<", "<="):
            raise ValueError("bad comparison %r" % op)
        b = self.expr()
        return (op, a, b)

    def _paren_is_cond(self):
        # A parenthesized condition is followed by `) and (` or `) or (`.
        depth, k = 0, self.i
        while k < len(self.t):
            if self.t[k] == "(":
                depth += 1
            elif self.t[k] == ")":
                depth -= 1
                if depth == 0:
                    return k + 1 < len(self.t) and self.t[k + 1] in ("and", "or")
            k += 1
        return False

    def expr(self):
        tok = self.peek()
        if tok == "(":
            self.take("(")
            if self.peek() == "-" and self.peek(1) != "(" and not self.peek(1).isdigit() and self.i + 1 < len(self.t):
                pass
            if self.peek() == "-":
                # (-E) negation.
                self.take("-")
                a = self.expr()
                self.take(")")
                return ("neg", a)
            a = self.expr()
            op = self.take()
            if op in ("div", "mod"):
                k = int(self.take())
                self.take(")")
                return (op, a, k)
            b = self.expr()
            self.take(")")
            return (op, a, b)
        if tok.isdigit():
            self.take()
            return ("const", int(tok))
        name = self.take()
        if self.peek() == "[":
            idx = []
            while self.peek() == "[":
                self.take("[")
                idx.append(self.expr())
                self.take("]")
            return ("read", name, idx)
        return ("var", name)


class NoRule(Exception):
    pass


class Interpreter:
    """The rules of G.2 and G.5 at WIDTH bits: partial maps, bounds, uninitialized reads."""

    def __init__(self, program, inputs, fuel=1 << 16):
        self.p = program
        self.inputs = inputs  # name -> value
        self.locals = {}
        self.fuel = fuel
        self.arrays = {}
        for name, extents, is_input in program.arrays:
            if is_input:
                raise NoRule("no input tables here")
            self.arrays[name] = ([self.expr(e) for e in extents], {})

    def run(self):
        self.stmt(self.p.body)
        return self.expr(self.p.result)

    def tick(self):
        if self.fuel == 0:
            raise TimeoutError("fuel")
        self.fuel -= 1

    def expr(self, e):
        kind = e[0]
        if kind == "const":
            return e[1] & MASK
        if kind == "var":
            name = e[1]
            if name in self.inputs:
                return self.inputs[name] & MASK
            return self.locals.get(name, 0)
        if kind == "neg":
            return (-self.expr(e[1])) & MASK
        if kind == "read":
            extents, elems = self.arrays[e[1]]
            k = tuple(self.expr(i) for i in e[2])
            if len(k) != len(extents):
                raise NoRule("rank")
            for d, kd in enumerate(k):
                if kd >= extents[d]:
                    raise NoRule("read out of the extent")
            if k not in elems:
                raise NoRule("read of an element without a value")
            return elems[k]
        a = self.expr(e[1])
        if kind == "div":
            return a // e[2]
        if kind == "mod":
            return a % e[2]
        b = self.expr(e[2])
        if kind == "+":
            return (a + b) & MASK
        if kind == "-":
            return (a - b) & MASK
        if kind == "*":
            return (a * b) & MASK
        raise ValueError("bad expression %r" % (kind,))

    def cond(self, c):
        op = c[0]
        if op == "not":
            return not self.cond(c[1])
        if op == "and":
            return self.cond(c[1]) and self.cond(c[2])
        if op == "or":
            return self.cond(c[1]) or self.cond(c[2])
        a, b = self.expr(c[1]), self.expr(c[2])
        return {"=": a == b, "<": a < b, "<=": a <= b}[op]

    def stmt(self, s):
        kind = s[0]
        if kind == "seq":
            for x in s[1]:
                self.stmt(x)
        elif kind == "skip":
            self.tick()
        elif kind == "assign":
            self.tick()
            self.locals[s[1]] = self.expr(s[2])
        elif kind == "write":
            self.tick()
            value = self.expr(s[3])
            extents, elems = self.arrays[s[1]]
            k = tuple(self.expr(i) for i in s[2])
            for d, kd in enumerate(k):
                if kd >= extents[d]:
                    raise NoRule("write out of the extent")
            elems[k] = value
        elif kind == "if":
            self.tick()
            self.stmt(s[2] if self.cond(s[1]) else s[3])
        elif kind == "while":
            while True:
                self.tick()
                if not self.cond(s[1]):
                    return
                self.stmt(s[2])
        else:
            raise ValueError("bad statement %r" % (kind,))


def run_program(text, m, n):
    """The program's value at iteration m (inputs m and the parameter p0 = n), or
    "NoRule"/"OutOfFuel"."""
    program = Parser(text).program()
    try:
        return Interpreter(program, {"m": m, "p0": n}).run()
    except NoRule:
        return "NoRule"
    except TimeoutError:
        return "OutOfFuel"


TEMPLATE = re.compile(r"^array T0\[\(m \+ (\d+)\)\] ; (T0\[\d+\] := [^;]+ ; )+v0 := (\d+) ; while v0 < \(m \+ 1\) do \(T0\[v0\] := .* ; v0 := \(v0 \+ 1\)\) ; return T0\[m\]$")


def check_case(driver, case, low, high, verbose):
    name, a, b, c, f0, f1 = case
    cols = 64
    text = mlir(name, a, b, c, f0, f1, cols)
    with tempfile.NamedTemporaryFile("w", suffix=".mlir", delete=False) as f:
        f.write(text)
        path = f.name
    cmd = [driver, path, f"--function={name}", "--lower-structured", f"--index-bit-width={WIDTH}",
           f"--scope=0:{low}:{high}", "--max-level=0", "--slices"]
    out = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
    if out.returncode != 0:
        return [f"{name}: driver failed: {out.stderr[:200]}"]
    report = json.loads(out.stdout)[0]
    problems = []
    forms = []
    for sl in report["closed_forms"]["slices"]:
        for loop in sl["loops"]:
            if loop["reason"]:
                continue
            for form in loop["closed_forms"]:
                forms.append((sl, loop, form))
    # The subscript's variable is the one whose slice has the A column use; here it is
    # the carried variable with a table or while program, or a loop-free form.
    judged = 0
    for sl, loop, form in forms:
        if form["kind"] == "loop-free":
            continue
        status = form["status"]
        program = form["program"]
        if not program:
            # No candidate stands (the search found nothing of the shape): nothing to judge,
            # and the status must be UNKNOWN.
            if status != "UNKNOWN":
                problems.append(f"{name}: a {status} form without a program")
            continue
        judged += 1
        if form["kind"] == "table" and not TEMPLATE.match(program):
            problems.append(f"{name}: a table program outside the claimed template: {program}")
            continue
        # Which variable? f is the subscript (the one the sampled subscript matches).
        # Compute expected sequences for both carried variables: f and g = f(m+1).
        expected_by_var = {}
        # The driver names variables by state id; the oracle decides which the program
        # matches by the sampled values: compare to f and to g sequences.
        for n in range(low, high + 1):
            seq = spec_sequence(a, b, c, f0, f1, n + 1)
            expected_by_var.setdefault("f", {})[n] = seq[:n]
            expected_by_var.setdefault("g", {})[n] = seq[1:n + 1]
        # Decide the variable by agreement at the smallest point (sampled by the driver).
        which = None
        n0 = low
        for cand in ("f", "g"):
            ok = True
            for m in range(n0):
                v = run_program(program, m, n0)
                if v != expected_by_var[cand][n0][m]:
                    ok = False
                    break
            if ok:
                which = cand
                break
        if status in ("PROVED", "SAMPLE_CONSISTENT"):
            if which is None:
                problems.append(f"{name}: {status} program matches neither f nor g at n = {n0}: {program}")
                continue
            for n in range(low, high + 1):
                for m in range(n):
                    v = run_program(program, m, n)
                    if v != expected_by_var[which][n][m]:
                        problems.append(f"{name}: {status} program differs at n = {n}, m = {m}: {v} vs "
                                        f"{expected_by_var[which][n][m]} ({program})")
                        break
        elif status == "REFUTED":
            model = form.get("model") or {}
            if "m" not in model or "p0" not in model:
                problems.append(f"{name}: REFUTED without m and p0 in the model")
                continue
            m, n = model["m"], model["p0"]
            if not (low <= n <= high) or m >= n:
                continue  # A model outside the scope or past the loop: not judged here.
            v = run_program(program, m, n)
            if which is not None and v == expected_by_var[which][n][m]:
                problems.append(f"{name}: REFUTED program agrees with {which} at the model (n = {n}, m = {m})")
        if verbose:
            print(f"  {name}: variable {which}: {form['kind']} {status} rounds {form['rounds']}: {program}")
    if judged == 0 and verbose:
        print(f"  {name}: no while or table program stood (UNKNOWN), nothing judged")
    return problems


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--driver", required=True)
    ap.add_argument("--cases", type=int, default=12)
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--low", type=int, default=2)
    ap.add_argument("--high", type=int, default=7)
    ap.add_argument("--verbose", action="store_true")
    args = ap.parse_args()
    rng = random.Random(args.seed)
    cases = [("fib_like", 1, 1, 1, 0, 1), ("fib", 1, 1, 0, 0, 1), ("double_prev", 2, 0, 1, 0, 1), ("skip", 0, 1, 2, 0, 1)]
    for k in range(args.cases):
        a = rng.choice([0, 1, 1, 2, 3])
        b = rng.choice([0, 1, 1, 2])
        c = rng.choice([0, 1, 2, 3])
        f0 = rng.choice([0, 1, 2])
        f1 = rng.choice([1, 2, 3])
        if a == 0 and b == 0:
            a = 1
        cases.append((f"rec{k}", a, b, c, f0, f1))
    problems = []
    for case in cases:
        # Keep every subscript below 64 over the scope, or the MLIR has no run to speak of:
        # shrink the scope for fast-growing recurrences.
        name, a, b, c, f0, f1 = case
        high = args.high
        while high > args.low and max(spec_sequence(a, b, c, f0, f1, high)) >= 64:
            high -= 1
        if max(spec_sequence(a, b, c, f0, f1, high)) >= 64:
            if args.verbose:
                print(f"  {name}: skipped, the subscript leaves the array within the smallest scope")
            continue
        problems += check_case(args.driver, case, args.low, high, args.verbose)
    for p in problems:
        print("PROBLEM", p)
    print(f"{len(cases)} cases, {len(problems)} problems")
    sys.exit(1 if problems else 0)


if __name__ == "__main__":
    main()
