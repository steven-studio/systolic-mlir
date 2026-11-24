#!/usr/bin/env python3
"""Prints integer systems with their truth, found by trying every integer point
in a box that the system itself imposes. Nothing here shares code or arithmetic
with the solver it is a test for.

One line per system:

    <F|I> n nge ngt neq ncong  then the rows in that order, each as
    a_1 .. a_n c      for a . x + c >= 0, > 0 or == 0, and for a congruence
    a_1 .. a_n c m    for a . x + c == 0 (mod m).
"""
import itertools
import random

BOX = 4


def value(a, c, x):
    return c + sum(p * q for p, q in zip(a, x))


def has_point(n, ge, gt, eq, cong):
    box = range(-BOX, BOX + 1)
    for x in itertools.product(box, repeat=n):
        if any(value(a, c, x) < 0 for a, c in ge):
            continue
        if any(value(a, c, x) <= 0 for a, c in gt):
            continue
        if any(value(a, c, x) != 0 for a, c in eq):
            continue
        if any(value(a, c, x) % m != 0 for a, c, m in cong):
            continue
        return True
    return False


def with_box(n, ge):
    rows = list(ge)
    for j in range(n):
        up = [0] * n
        down = [0] * n
        up[j] = 1
        down[j] = -1
        rows.append((up, BOX))
        rows.append((down, BOX))
    return rows


def emit(n, ge, gt, eq, cong):
    ge = with_box(n, ge)
    words = ["F" if has_point(n, ge, gt, eq, cong) else "I", n, len(ge), len(gt),
             len(eq), len(cong)]
    for a, c in ge + gt + eq:
        words.extend(a)
        words.append(c)
    for a, c, m in cong:
        words.extend(a)
        words.append(c)
        words.append(m)
    print(" ".join(str(w) for w in words))


def row(rng, n, span, constants):
    return [rng.randint(-span, span) for _ in range(n)], rng.randint(-constants, constants)


def mixed(rng, n, count, span, constants, weights):
    ge, gt, eq, cong = [], [], [], []
    for _ in range(count):
        a, c = row(rng, n, span, constants)
        pick = rng.choices((ge, gt, eq, cong), weights)[0]
        if pick is cong:
            cong.append((a, c, rng.randint(1, 6)))
        else:
            pick.append((a, c))
    return ge, gt, eq, cong


def holds_at_point(rng, n, count):
    """Rows that keep one integer point, so that a large share are feasible."""
    point = [rng.randint(-BOX, BOX) for _ in range(n)]
    ge, gt, eq, cong = [], [], [], []
    for _ in range(count):
        a = [rng.randint(-5, 5) for _ in range(n)]
        at = sum(p * q for p, q in zip(a, point))
        kind = rng.randint(0, 9)
        if kind < 5:
            ge.append((a, -at + rng.randint(0, 2)))
        elif kind < 7:
            gt.append((a, -at + rng.randint(1, 3)))
        elif kind < 8:
            eq.append((a, -at))
        else:
            m = rng.randint(2, 7)
            cong.append((a, -at + m * rng.randint(-2, 2), m))
    return ge, gt, eq, cong


def rational_only(rng, n, count):
    """Thin rows around a fraction, with no room for an integer point."""
    ge, gt, eq, cong = [], [], [], []
    for _ in range(count):
        a = [rng.randint(-3, 3) * 2 for _ in range(n)]
        c = 2 * rng.randint(-3, 3) + 1
        ge.append((a, c))
        ge.append(([-x for x in a], -c + rng.randint(0, 1)))
    return ge, gt, eq, cong


def main():
    rng = random.Random(20261103)
    for _ in range(2500):
        n = rng.randint(1, 3)
        emit(n, *mixed(rng, n, rng.randint(1, 6), 4, 7, (5, 2, 2, 2)))
    for _ in range(1500):
        n = rng.randint(1, 3)
        emit(n, *holds_at_point(rng, n, rng.randint(1, 8)))
    for _ in range(800):
        n = rng.randint(1, 3)
        emit(n, *rational_only(rng, n, rng.randint(1, 3)))
    for _ in range(800):
        n = rng.randint(1, 3)
        emit(n, *mixed(rng, n, rng.randint(1, 5), 3, 5, (1, 1, 1, 6)))
    # Four variables, fewer of them, since this is 9 ** 4 points each.
    for _ in range(120):
        emit(4, *holds_at_point(rng, 4, rng.randint(2, 7)))
    # No variables at all.
    for _ in range(100):
        emit(0, [([], rng.randint(-1, 1))], [([], rng.randint(-1, 1))],
             [([], rng.randint(-1, 1))], [([], rng.randint(-3, 3), rng.randint(1, 3))])


main()
