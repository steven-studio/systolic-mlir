#!/usr/bin/env python3
"""Prints linear systems with their truth over the rationals. The truth comes
from Fourier-Motzkin elimination on Python integers: a different algorithm, in a
different language, sharing no arithmetic with the solver it is a test for.

One line per system:

    <F|I> n nge ngt neq  then the rows in that order, each as
    a_1 .. a_n c  for  a . x + c >= 0, > 0 or == 0.
"""
import random
from math import gcd


def reduced(row):
    """The row divided by the gcd of all its numbers, which keeps its meaning."""
    a, c, strict = row
    divisor = abs(c)
    for value in a:
        divisor = gcd(divisor, value)
    if divisor <= 1:
        return row
    return (tuple(value // divisor for value in a), c // divisor, strict)


def eliminate(rows, j):
    """Fourier's step: each lower bound on x_j meets each upper bound on it. A
    pair is strict when either row is. A positive sum of true rows is true."""
    lower = [r for r in rows if r[0][j] > 0]
    upper = [r for r in rows if r[0][j] < 0]
    result = {r for r in rows if r[0][j] == 0}
    for p in lower:
        for q in upper:
            s, t = -q[0][j], p[0][j]
            a = tuple(s * x + t * y for x, y in zip(p[0], q[0]))
            result.add(reduced((a, s * p[1] + t * q[1], p[2] or q[2])))
    return result


def feasible(n, ge, gt, eq):
    rows = set()
    for a, c in ge:
        rows.add(reduced((tuple(a), c, False)))
    for a, c in gt:
        rows.add(reduced((tuple(a), c, True)))
    for a, c in eq:
        rows.add(reduced((tuple(a), c, False)))
        rows.add(reduced((tuple(-x for x in a), -c, False)))
    for j in range(n):
        rows = eliminate(rows, j)
    return all(c > 0 if strict else c >= 0 for _, c, strict in rows)


def emit(n, ge, gt, eq):
    words = ["F" if feasible(n, ge, gt, eq) else "I", n, len(ge), len(gt), len(eq)]
    for a, c in ge + gt + eq:
        words.extend(a)
        words.append(c)
    print(" ".join(str(w) for w in words))


def random_row(rng, n, span, constants):
    return [rng.randint(-span, span) for _ in range(n)], rng.randint(-constants, constants)


def mixed(rng, n, count, span, constants, weights=(5, 3, 2)):
    ge, gt, eq = [], [], []
    for _ in range(count):
        row = random_row(rng, n, span, constants)
        rng.choices((ge, gt, eq), weights)[0].append(row)
    return ge, gt, eq


def degenerate(rng, n, count):
    """Rows through one point, so that many bounds are tight at once."""
    point = [rng.randint(-2, 2) for _ in range(n)]
    ge, gt, eq = [], [], []
    for _ in range(count):
        a = [rng.randint(-3, 3) for _ in range(n)]
        c = -sum(x * y for x, y in zip(a, point))
        rng.choices((ge, gt, eq), (7, 2, 1))[0].append((a, c))
    return ge, gt, eq


def huge(rng, n, count):
    """Values far outside 64 bits, of both signs, with a scale for each row."""
    ge, gt, eq = [], [], []
    for _ in range(count):
        scale = rng.choice((1, 1 << 64, (1 << 89) - 1, -1 + (1 << 127)))
        a = [scale * rng.randint(-5, 5) for _ in range(n)]
        c = rng.choice((scale, 1)) * rng.randint(-9, 9)
        rng.choices((ge, gt, eq), (6, 3, 1))[0].append((a, c))
    return ge, gt, eq


def tight(rng, n, count):
    """Pairs of opposite rows that just meet, just miss, or just touch."""
    ge, gt, eq = [], [], []
    for _ in range(count):
        a = [rng.randint(-2, 2) for _ in range(n)]
        c = rng.randint(-3, 3)
        ge.append((a, c))
        opposite = ([-x for x in a], -c + rng.randint(-1, 1))
        rng.choice((ge, gt)).append(opposite)
    return ge, gt, eq


def main():
    rng = random.Random(20261102)
    for _ in range(9000):
        n = rng.randint(1, 3)
        emit(n, *mixed(rng, n, rng.randint(1, 8), 4, 6))
    for _ in range(600):
        emit(4, *mixed(rng, 4, rng.randint(2, 6), 3, 5))
    for _ in range(2000):
        n = rng.randint(2, 4)
        emit(n, *mixed(rng, n, rng.randint(2, 5), 3, 4, (1, 1, 4)))
    for _ in range(2000):
        n = rng.randint(1, 3)
        emit(n, *mixed(rng, n, rng.randint(1, 7), 3, 2, (1, 6, 1)))
    for _ in range(3000):
        n = rng.randint(2, 3)
        emit(n, *degenerate(rng, n, rng.randint(2, 8)))
    for _ in range(300):
        emit(4, *degenerate(rng, 4, rng.randint(2, 6)))
    for _ in range(2000):
        n = rng.randint(1, 3)
        emit(n, *huge(rng, n, rng.randint(1, 6)))
    for _ in range(2000):
        n = rng.randint(1, 3)
        emit(n, *tight(rng, n, rng.randint(1, 3)))
    # Rows with no variables, and systems with no rows.
    for _ in range(500):
        n = rng.randint(0, 3)
        zero = [0] * n
        ge = [(zero, rng.randint(-1, 1)) for _ in range(rng.randint(0, 2))]
        gt = [(zero, rng.randint(-1, 1)) for _ in range(rng.randint(0, 2))]
        eq = [(zero, rng.randint(-1, 1)) for _ in range(rng.randint(0, 2))]
        emit(n, ge, gt, eq)


main()
