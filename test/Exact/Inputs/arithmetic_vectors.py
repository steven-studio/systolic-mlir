#!/usr/bin/env python3
"""Arithmetic expectations for the exact solver, computed by Python's own big
integers and fractions, which share no code with the solver.

Output, one case per line:
  int  a b sum difference product floor ceil mod gcd lcm bits_of_a
  rat  n1 d1 n2 d2 p1 q1 sum_p sum_q product_p product_q less floor1 ceil1
b, or d, is never zero. Division by zero is tested in the unit test.
"""
import math
import random
from fractions import Fraction

BOUNDARIES = [
    0, 1, 2, 3, 7, 10, 2**31 - 1, 2**31, 2**31 + 1, 2**32, 2**62, 2**63 - 2,
    2**63 - 1, 2**63, 2**63 + 1, 2**64 - 1, 2**64, 2**64 + 1, 10**18, 10**19,
    2**127 - 1, 2**127, 2**128, 2**200 + 3, 3**100, 6**70,
]
SIGNED = sorted({v for b in BOUNDARIES for v in (b, -b)})


def int_case(a, b):
    print("int", a, b, a + b, a - b, a * b, a // b, -((-a) // b), a % b,
          math.gcd(a, b), math.lcm(a, b), a.bit_length())


def rat_case(n1, d1, n2, d2):
    x, y = Fraction(n1, d1), Fraction(n2, d2)
    s, p = x + y, x * y
    print("rat", n1, d1, n2, d2, x.numerator, x.denominator, s.numerator,
          s.denominator, p.numerator, p.denominator, int(x < y),
          math.floor(x), math.ceil(x))


def main():
    for a in SIGNED:
        for b in SIGNED:
            if b != 0:
                int_case(a, b)
    rng = random.Random(20261102)
    pool = [v for v in SIGNED if v != 0]
    for _ in range(3000):
        rat_case(rng.choice(SIGNED), rng.choice(pool), rng.choice(SIGNED),
                 rng.choice(pool))
    for _ in range(3000):
        bits = rng.randint(1, 300)
        int_case(rng.getrandbits(bits) - rng.getrandbits(bits),
                 (rng.getrandbits(rng.randint(1, 150)) + 1) * rng.choice((1, -1)))


main()
