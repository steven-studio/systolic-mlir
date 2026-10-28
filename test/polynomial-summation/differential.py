#!/usr/bin/env python3
"""Independent Python int/Fraction oracles; no finite-difference derivation.

The C++ driver derives and checks exact coefficient identities for each case.
This process only executes conventional polynomial arithmetic and simultaneous
state updates, with per-operation modular reduction where applicable.
"""
import argparse
from fractions import Fraction
from math import comb
from pathlib import Path
import random
import subprocess


def horner(coefficients, value, modulus=None):
    result = 0
    for coefficient in reversed(coefficients):
        result *= value
        if modulus is not None:
            result %= modulus
        result += coefficient
        if modulus is not None:
            result %= modulus
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--driver", required=True, type=Path)
    args = parser.parse_args()
    rng = random.Random(0x5CE2026)
    commands, expected, categories = [], [], {}

    def add(category, command, expectation):
        commands.append(command)
        expected.append(expectation)
        categories[category] = categories.get(category, 0) + 1

    # Generalized binomials are checked via Python's independent math.comb,
    # including points below the degree, boundaries, negatives and huge n.
    for k in list(range(20)) + [31, 32, 47, 63, 64]:
        for n in [-10**50, -101, -k - 1, -1, 0, 1, k - 1, k, k + 1, 100, 10**50]:
            value = comb(n, k) if n >= 0 else (-1) ** k * comb(-n + k - 1, k)
            add("binomial", f"binom {n} {k}", ("binom", Fraction(value)))

    fixed = [([0] * k + [1], 1, 1, 0) for k in range(1, 11)]
    fixed += [([5, -3, 7], -17, 3, 29), ([-17, 12, -11, 2], -8, 5, -91),
              ([Fraction(-13, 7), Fraction(11, 3), Fraction(-7, 5), Fraction(9, 2)],
               -19, 7, Fraction(13, 17)),
              ([2**250 + 7, -2**180, 11, 2**200], -71, 13, 2**300),
              ([0], -10, 4, 75), ([], 2, 3, -25), ([17], -12, 0, 0)]
    for _ in range(90):
        degree = rng.randrange(0, 10)
        coefficients = [Fraction(rng.randrange(-500, 501), rng.randrange(1, 14))
                        for _ in range(degree + 1)]
        fixed.append((coefficients, rng.randrange(-30, 31),
                      rng.choice([-5, -1, 0, 1, 2, 3, 7, 11]),
                      Fraction(rng.randrange(-1000, 1001), rng.randrange(1, 10))))
    for coefficients, start, step, initial in fixed:
        value = Fraction(initial)
        for n in range(61):
            text = " ".join(map(str, coefficients))
            add("sum", f"sum {len(coefficients)} {text} {start} {step} {initial} {n}",
                ("sum", value))
            value += horner(coefficients, start + step * n)
    # Exercise the degree boundary without using a reference summation formula.
    for k in [16, 31, 63]:
        coefficients = [0] * k + [1]
        for start, step in [(-3, 2), (1, 1)]:
            for n in [0, 1, 2, 3, 9, 17]:
                value = sum(pow(start + step * i, k) for i in range(n))
                text = " ".join(map(str, coefficients))
                add("high_degree", f"sum {k+1} {text} {start} {step} 0 {n}", ("sum", value))

    for mode, widths in [("math", [0]), ("mod", [1, 2, 3, 8, 16, 32, 65, 127])]:
        for width in widths:
            for _ in range(8):
                coefficients = [rng.randrange(-11, 12) for _ in range(rng.randrange(1, 7))]
                start, step = rng.randrange(-300, 301), rng.choice([1, 2, 3, 7, 19])
                y0, z0 = rng.randrange(-200, 201), rng.randrange(-200, 201)
                modulus = 2**width if mode == "mod" else None
                x, y, z = start, y0, z0
                if modulus is not None:
                    x, y, z = x % modulus, y % modulus, z % modulus
                for n in range(91):
                    text = " ".join(map(str, coefficients))
                    add(f"tri_{mode}", f"tri {mode} {width} {n} {start} {step} {y0} {z0} "
                        f"{len(coefficients)} {text}", ("tri", (x, y, z)))
                    # Simultaneous old-state semantics; z uses old y.
                    next_z = z + y
                    next_y = y + horner(coefficients, x, modulus)
                    next_x = x + step
                    if modulus is not None:
                        next_x %= modulus
                        next_y %= modulus
                        next_z %= modulus
                    x, y, z = next_x, next_y, next_z

    # Safe signed examples with a proof, and exact direct execution as oracle.
    for width in [8, 16, 32, 65]:
        for n in range(11):
            x, y, z = 0, 0, 0
            for _ in range(n):
                x, y, z = x + 1, y + x, z + y
            if max(x, y, z) < 2**(width - 1):
                add("tri_signed", f"tri signed {width} {n} 0 1 0 0 2 0 1",
                    ("tri", (x, y, z)))
    # Iteration count must not be reduced modulo 2^w. Many wraps in the
    # induction AND accumulators; the oracle reduces each source operation.
    x, y, z = 1, 0, 0
    for n in range(1026):
        if n in [0, 16, 255, 256, 257, 511, 512, 1024, 1025]:
            add("multiple_wraps", f"tri mod 8 {n} 1 1 0 0 2 0 1", ("tri", (x, y, z)))
        x, y, z = (x + 1) % 256, (y + x) % 256, (z + y) % 256

    run = subprocess.run([str(args.driver.resolve())], input="\n".join(commands) + "\n",
                         text=True, capture_output=True)
    if run.returncode:
        raise RuntimeError(f"driver exited {run.returncode}:\n{run.stderr}")
    lines = run.stdout.splitlines()
    if len(lines) != len(expected):
        raise AssertionError(f"driver returned {len(lines)} lines for {len(expected)} cases")
    for i, (line, (kind, value)) in enumerate(zip(lines, expected)):
        fields = line.split("\t")
        if fields[0] != kind:
            raise AssertionError(f"{commands[i]} -> {line}")
        actual = tuple(map(int, fields[1:])) if kind == "tri" else Fraction(fields[1])
        if actual != value:
            raise AssertionError(f"{commands[i]} -> {actual}, expected {value}")
        if kind == "sum":
            # Independently evaluate the exported conventional coefficients.
            # The main oracle above never uses these exported coefficients.
            n = int(commands[i].split()[-1])
            monomial = list(map(Fraction, fields[3].split(',')))
            if horner(monomial, n) != value:
                raise AssertionError(f"exported monomial mismatch: {commands[i]}")
    print(f"PASS: {len(expected)} independent Python int/Fraction comparisons; seed=0x5ce2026")
    print(", ".join(f"{name}={count}" for name, count in sorted(categories.items())))


if __name__ == "__main__":
    main()
