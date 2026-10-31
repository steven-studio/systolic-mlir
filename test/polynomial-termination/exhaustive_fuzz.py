#!/usr/bin/env python3
"""Exhaustive-input soundness fuzz for polynomial-termination-report.

Each generated loop is described by one small specification. The specification
is printed as MLIR for the analyzer and executed here with Python integers for
every input in the requested domain. A repeated complete header state is an
exact nontermination observation, so no iteration limit is involved.

differential.py enumerates one canonical single-state loop at widths 3 and 4.
This test instead randomizes widths 1..4, two-state accumulators, block order,
self-loops, latches, prelude forwarding, branch polarity, swapped operands,
invariant guards and restricted domains.

A PROVEN verdict must hold for every enumerated input. UNKNOWN is always
acceptable: this test measures soundness, not completeness.
"""
import argparse
import collections
import json
from pathlib import Path
import random
import subprocess
import sys
import tempfile

COMPARISONS = {
    "slt": lambda a, b, w: signed(a, w) < signed(b, w),
    "sle": lambda a, b, w: signed(a, w) <= signed(b, w),
    "sgt": lambda a, b, w: signed(a, w) > signed(b, w),
    "sge": lambda a, b, w: signed(a, w) >= signed(b, w),
    "ult": lambda a, b, w: a < b,
    "ule": lambda a, b, w: a <= b,
    "ugt": lambda a, b, w: a > b,
    "uge": lambda a, b, w: a >= b,
    "eq": lambda a, b, w: a == b,
    "ne": lambda a, b, w: a != b,
}


def signed(value, width):
    return value - (1 << width) if value >> (width - 1) else value


class Loop:
    """One function: its MLIR text and an independent executable meaning."""

    def __init__(self, rng, name):
        self.rng = rng
        self.width = rng.choice([1, 2, 3, 3, 4, 4])
        self.mask = (1 << self.width) - 1
        self.type = f"i{self.width}"
        self.counter = 0
        self.prelude, self.header, self.body = [], [], []
        self.text = self.build(name)

    def fresh(self):
        self.counter += 1
        return f"%v{self.counter}"

    def constant(self, value, ops):
        name = self.fresh()
        ops.append(f"{name} = arith.constant {signed(value & self.mask, self.width)} : {self.type}")
        return name

    def binary(self, op, lhs, rhs, ops):
        name = self.fresh()
        ops.append(f"{name} = arith.{op} {lhs}, {rhs} : {self.type}")
        return name

    def invariant(self, ops):
        """An SSA invariant and its meaning over the parameters (p0, p1)."""
        m, value = self.mask, self.rng.randint(0, self.mask)
        kind = self.rng.choice(["c", "c", "c", "p0", "p1", "p0+c", "p1*c", "p0-p1", "p0*p1"])
        if kind == "c":
            return self.constant(value, ops), lambda p: value
        if kind == "p0":
            return "%p0", lambda p: p[0]
        if kind == "p1":
            return "%p1", lambda p: p[1]
        if kind == "p0+c":
            return (self.binary("addi", "%p0", self.constant(value, ops), ops),
                    lambda p: (p[0] + value) & m)
        if kind == "p1*c":
            return (self.binary("muli", "%p1", self.constant(value, ops), ops),
                    lambda p: (p[1] * value) & m)
        if kind == "p0-p1":
            return self.binary("subi", "%p0", "%p1", ops), lambda p: (p[0] - p[1]) & m
        return self.binary("muli", "%p0", "%p1", ops), lambda p: (p[0] * p[1]) & m

    def linear_update(self, index):
        """A linear update of state `index`; a step may be symbolic or zero."""
        rng, m, ops, state = self.rng, self.mask, self.body, f"%s{index}"
        kind = rng.choice(["add", "add", "add", "sub", "commuted", "chained", "symbolic", "same"])
        if kind == "same":
            return state, lambda s, p: s[index]
        if kind == "symbolic":
            return self.binary("addi", state, "%p1", ops), lambda s, p: (s[index] + p[1]) & m
        value = rng.randint(0, m)
        step = self.constant(value, ops)
        if kind == "add":
            return self.binary("addi", state, step, ops), lambda s, p: (s[index] + value) & m
        if kind == "commuted":
            return self.binary("addi", step, state, ops), lambda s, p: (s[index] + value) & m
        if kind == "sub":
            return self.binary("subi", state, step, ops), lambda s, p: (s[index] - value) & m
        other = rng.randint(0, m)
        middle = self.binary("addi", state, step, ops)
        return (self.binary("addi", middle, self.constant(other, ops), ops),
                lambda s, p: (s[index] + value + other) & m)

    def accumulator_update(self, index, source):
        rng, m, ops, state = self.rng, self.mask, self.body, f"%s{index}"
        kind = rng.choice(["linear", "linear", "square", "constant", "same"])
        if kind == "same":
            return state, lambda s, p: s[index]
        if kind == "constant":
            value = rng.randint(0, m)
            return (self.binary("addi", state, self.constant(value, ops), ops),
                    lambda s, p: (s[index] + value) & m)
        if kind == "linear":
            return (self.binary("addi", state, f"%s{source}", ops),
                    lambda s, p: (s[index] + s[source]) & m)
        square = self.binary("muli", f"%s{source}", f"%s{source}", ops)
        return (self.binary("addi", state, square, ops),
                lambda s, p: (s[index] + s[source] * s[source]) & m)

    def build(self, name):
        rng, ty = self.rng, self.type
        start0, self.init0 = self.invariant(self.prelude)
        start1, self.init1 = self.invariant(self.prelude)
        bound, self.bound = self.invariant(rng.choice([self.prelude, self.header]))

        self.guard_state = rng.choice([0, 0, 0, 1])
        self.predicate = rng.choice(list(COMPARISONS)[:8] * 3 + ["eq", "ne"])
        self.swapped = rng.random() < 0.3
        self.invariant_guard = rng.random() < 0.06
        self.constant_guard = rng.choice([True, False]) if rng.random() < 0.04 else None
        self.continue_on_true = rng.random() < 0.75

        guard = self.fresh()
        if self.constant_guard is not None:
            self.header.append(f"{guard} = arith.constant {'true' if self.constant_guard else 'false'}")
        else:
            tested = "%p0" if self.invariant_guard else f"%s{self.guard_state}"
            lhs, rhs = (bound, tested) if self.swapped else (tested, bound)
            self.header.append(f"{guard} = arith.cmpi {self.predicate}, {lhs}, {rhs} : {ty}")

        if rng.random() < 0.5:
            next0, self.update0 = self.linear_update(0)
            next1, self.update1 = self.accumulator_update(1, 0)
        else:
            next1, self.update1 = self.linear_update(1)
            next0, self.update0 = self.accumulator_update(0, 1)

        shape = rng.choice(["plain", "plain", "latch", "self", "forwarded"])
        states = f"%s0: {ty}, %s1: {ty}"
        back = f"^header({next0}, {next1} : {ty}, {ty})"
        blocks = []

        def block(label, arguments, operations, terminator):
            head = f"{label}({arguments}):" if arguments else f"{label}:"
            blocks.append("\n".join([head] + [f"  {op}" for op in operations + [terminator]]))

        def branch(continuation):
            if self.continue_on_true:
                return f"cf.cond_br {guard}, {continuation}, ^exit"
            return f"cf.cond_br {guard}, ^exit, {continuation}"

        if shape == "self":
            block("^header", states, self.header + self.body, branch(back))
        else:
            block("^header", states, self.header, branch("^body"))
            if shape == "latch":
                split = rng.randint(0, len(self.body))
                block("^body", "", self.body[:split], "cf.br ^latch")
                block("^latch", "", self.body[split:], f"cf.br {back}")
            else:
                block("^body", "", self.body, f"cf.br {back}")
        block("^exit", "", [], f"return %p0 : {ty}")

        entry = f"cf.br ^header({start0}, {start1} : {ty}, {ty})"
        if shape == "forwarded":
            block("^forward", f"%f: {ty}", [], f"cf.br ^header(%f, {start1} : {ty}, {ty})")
            entry = f"cf.br ^forward({start0} : {ty})"
        rng.shuffle(blocks)
        prelude = "\n".join(f"  {op}" for op in self.prelude + [entry])
        return (f"func.func @{name}(%p0: {ty}, %p1: {ty}) -> {ty} {{\n{prelude}\n"
                + "\n".join(blocks) + "\n}")

    def continues(self, state, parameters):
        if self.constant_guard is not None:
            return self.constant_guard == self.continue_on_true
        tested = parameters[0] if self.invariant_guard else state[self.guard_state]
        bound = self.bound(parameters)
        lhs, rhs = (bound, tested) if self.swapped else (tested, bound)
        return COMPARISONS[self.predicate](lhs, rhs, self.width) == self.continue_on_true

    def exits(self, parameters):
        state = (self.init0(parameters), self.init1(parameters))
        seen = set()
        while self.continues(state, parameters):
            if state in seen:
                return False  # exact: the update is a deterministic function
            seen.add(state)
            state = (self.update0(state, parameters), self.update1(state, parameters))
        return True


def check(loop, analysis, domain, stats):
    lower, upper = domain or (-(1 << (loop.width - 1)), (1 << (loop.width - 1)) - 1)
    outcomes = [loop.exits((first & loop.mask, second))
                for first in range(lower, upper + 1) for second in range(1 << loop.width)]
    status = analysis["status"]
    stats[status] += 1
    if status == "PROVEN_TERMINATING" and not all(outcomes):
        return "PROVEN_TERMINATING but an input cycles"
    if status == "PROVEN_NONTERMINATING" and any(outcomes):
        return "PROVEN_NONTERMINATING but an input exits"
    if status == "UNKNOWN" and analysis["certificate"] is not None:
        return "UNKNOWN carries a certificate"
    if status != "UNKNOWN":
        assert analysis["certificate"] is not None
    return None


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--driver", required=True)
    parser.add_argument("--mlir-opt", required=True)
    parser.add_argument("--seed", type=int, default=20261031)
    parser.add_argument("--modules", type=int, default=40)
    parser.add_argument("--functions", type=int, default=25)
    args = parser.parse_args()

    rng = random.Random(args.seed)
    stats = collections.Counter()
    failures = []
    with tempfile.TemporaryDirectory() as directory:
        for index in range(args.modules):
            loops = [Loop(rng, f"f{n}") for n in range(args.functions)]
            path = Path(directory) / f"m{index}.mlir"
            path.write_text("\n".join(loop.text for loop in loops) + "\n")
            verified = subprocess.run([args.mlir_opt, str(path), "-o", "/dev/null"],
                                      capture_output=True, text=True)
            assert verified.returncode == 0, verified.stderr[-2000:]
            command, domain = [args.driver, str(path)], None
            if rng.random() < 0.3:  # valid at every width, including i1
                domain = tuple(sorted((rng.randint(-1, 0), rng.randint(-1, 0))))
                command.append(f"--parameter-range=0:{domain[0]}:{domain[1]}")
            report = subprocess.run(command, capture_output=True, text=True, timeout=300)
            assert report.returncode == 0, (report.returncode, report.stdout[-2000:], report.stderr[-2000:])
            for loop, item in zip(loops, json.loads(report.stdout)["functions"]):
                problem = check(loop, item["analysis"], domain, stats)
                if problem:
                    failures.append((problem, item["analysis"]["reason"], loop.text))
    for problem, reason, text in failures[:5]:
        print(f"FAIL: {problem}\nreason: {reason}\n{text}\n")
    total = sum(stats.values())
    print(f"{total} functions: {stats['PROVEN_TERMINATING']} terminating, "
          f"{stats['PROVEN_NONTERMINATING']} nonterminating, {stats['UNKNOWN']} unknown; "
          f"{len(failures)} failures")
    # Coverage guard: a run with no positive verdict would prove nothing.
    if total >= 100:
        assert stats["PROVEN_TERMINATING"] and stats["PROVEN_NONTERMINATING"], stats
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
