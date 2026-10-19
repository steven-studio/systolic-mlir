#!/usr/bin/env python3
"""Randomized property test of the integer lattice tools (IntegerLattice.h,
docs/AFFINE_GEMM_MAPPING.md section 19), against an independent Python
implementation: exact fractions for ranks and determinants, brute-force
enumeration for kernels, mixed radixes and injectivity.

Generates random cases, runs `systolic-opt --test-integer-lattice` on them
once, and checks every printed answer:

  matrix  -- the rank equals the rank over Q (fraction elimination); the
             kernel has cols - rank rows, each in the kernel; the rows are in
             Hermite normal form (echelon, positive pivots, the entries above
             a pivot in [0, pivot)); every integer kernel vector in a box is an
             integer combination of them; |det| equals the fraction determinant;
  forms   -- a common direction exists iff all nonzero forms are pairwise
             proportional; then it is the primitive first form, multipliers exact;
  radix   -- 'mixed radix, extent E' iff the map is a bijection onto E
             consecutive integers (enumeration);
  inject  -- 'injective' iff no two box points have the same image
             (enumeration); a reported collision (dI, dJ) is a real one.

Prints one line per failure and a summary line.
"""

import argparse
import itertools
import random
import re
import subprocess
import sys
from fractions import Fraction
from math import gcd


def rank_q(m, cols):
    rows = [[Fraction(v) for v in r] for r in m]
    rank = 0
    for c in range(cols):
        pivot = next((r for r in range(rank, len(rows)) if rows[r][c] != 0), None)
        if pivot is None:
            continue
        rows[rank], rows[pivot] = rows[pivot], rows[rank]
        for r in range(len(rows)):
            if r != rank and rows[r][c] != 0:
                f = rows[r][c] / rows[rank][c]
                rows[r] = [a - f * b for a, b in zip(rows[r], rows[rank])]
        rank += 1
    return rank


def det_q(m):
    n = len(m)
    rows = [[Fraction(v) for v in r] for r in m]
    det = Fraction(1)
    for c in range(n):
        pivot = next((r for r in range(c, n) if rows[r][c] != 0), None)
        if pivot is None:
            return 0
        if pivot != c:
            rows[c], rows[pivot] = rows[pivot], rows[c]
            det = -det
        det *= rows[c][c]
        for r in range(c + 1, n):
            f = rows[r][c] / rows[c][c]
            rows[r] = [a - f * b for a, b in zip(rows[r], rows[c])]
    return abs(int(det))


def in_lattice(w, basis):
    """Whether w is an integer combination of the rows of an echelon basis."""
    w = list(w)
    for row in basis:
        p = next(i for i, v in enumerate(row) if v != 0)
        if w[p] % row[p] != 0:
            return False
        q = w[p] // row[p]
        w = [a - q * b for a, b in zip(w, row)]
    return all(v == 0 for v in w)


def check_hnf(basis):
    last = -1
    for i, row in enumerate(basis):
        p = next((c for c, v in enumerate(row) if v != 0), None)
        if p is None or p <= last or row[p] <= 0:
            return "not in echelon form with positive pivots"
        for j in range(i):
            if not (0 <= basis[j][p] < row[p]):
                return "entry above a pivot not reduced"
        last = p
    return None


def primitive(v):
    g = 0
    for x in v:
        g = gcd(g, abs(x))
    if g == 0:
        return list(v)
    sign = next((1 if x > 0 else -1 for x in v if x != 0), 1)
    return [x // g * sign for x in v]


def gen_matrix(rng):
    rows, cols = rng.randint(1, 4), rng.randint(1, 5)
    hi = rng.choice([1, 2, 3, 6, 6, 12])
    m = [[rng.randint(-hi, hi) for _ in range(cols)] for _ in range(rows)]
    if rng.random() < 0.2 and rows > 1:   # a dependent row
        a, b = rng.randint(-2, 2), rng.randint(-2, 2)
        m[-1] = [a * x + b * y for x, y in zip(m[0], m[1 % rows])]
    if rng.random() < 0.15:               # square
        n = rng.randint(1, 4)
        m = [[rng.randint(-hi, hi) for _ in range(n)] for _ in range(n)]
    return m


def gen_forms(rng):
    n = rng.randint(1, 4)
    base = [rng.randint(-5, 5) for _ in range(n)]
    forms = []
    for _ in range(rng.randint(1, 4)):
        if rng.random() < 0.7:
            forms.append([rng.choice([-3, -2, -1, 0, 1, 2, 3]) * x for x in base])
        else:
            forms.append([rng.randint(-5, 5) for _ in range(n)])
    return forms


def gen_radix(rng):
    q = rng.randint(1, 3)
    extents = [rng.randint(1, 4) for _ in range(q)]
    if rng.random() < 0.5:                # a genuine mixed radix, shuffled
        order = list(range(q))
        rng.shuffle(order)
        coefs, scale = [0] * q, 1
        for s in order:
            coefs[s] = scale * rng.choice([1, -1])
            scale *= extents[s]
    else:
        coefs = [rng.choice([-12, -8, -4, -3, -2, -1, 1, 2, 3, 4, 6, 8, 12]) for _ in range(q)]
    return coefs, extents


def gen_inject(rng):
    r = rng.randint(1, 3)
    base = [rng.randint(-4, 4) for _ in range(r)]
    if rng.random() < 0.5:
        lam_i = [rng.randint(-2, 2) * x for x in base]
        lam_j = [rng.randint(-6, 6) * x for x in base]
    else:
        lam_i = [rng.randint(-6, 6) for _ in range(r)]
        lam_j = [rng.randint(-6, 6) for _ in range(r)]
    return lam_i, lam_j, rng.randint(1, 7), rng.randint(1, 7)


def attr(m):
    return "[" + ", ".join("[" + ", ".join(str(v) for v in r) + "]" for r in m) + "]"


def vec(v):
    return "[" + ", ".join(str(x) for x in v) + "]"


LINE = re.compile(r"^@(\w+): (.*)$")


def parse_matrix(text):
    rows = re.findall(r"\[([-\d, ]*)\]", text)
    return [[int(x) for x in r.split(",") if x.strip()] for r in rows]


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--opt", default="systolic-opt")
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--count", type=int, default=200)
    a = ap.parse_args()
    rng = random.Random(a.seed)

    cases, funcs = {}, []
    for i in range(a.count):
        m = gen_matrix(rng)
        cases["m%d" % i] = ("matrix", m)
        funcs.append("func.func @m%d() attributes {lattice.matrix = %s} { return }" % (i, attr(m)))
        f = gen_forms(rng)
        cases["f%d" % i] = ("forms", f)
        funcs.append("func.func @f%d() attributes {lattice.forms = %s} { return }" % (i, attr(f)))
        c, n = gen_radix(rng)
        cases["r%d" % i] = ("radix", (c, n))
        funcs.append("func.func @r%d() attributes {lattice.radix = %s, lattice.extents = %s} { return }"
                     % (i, vec(c), vec(n)))
        li, lj, M, N = gen_inject(rng)
        cases["j%d" % i] = ("inject", (li, lj, M, N))
        funcs.append("func.func @j%d() attributes {lattice.inject = %s, lattice.box = %s} { return }"
                     % (i, attr([li, lj]), vec([M, N])))
    run = subprocess.run([a.opt, "--test-integer-lattice", "-", "-o", "/dev/null"],
                         input="\n".join(funcs) + "\n", capture_output=True, text=True)
    if run.returncode != 0:
        print("systolic-opt failed (rc %d):\n%s" % (run.returncode, run.stderr[-3000:]))
        sys.exit(1)
    answers = {}
    for line in run.stderr.splitlines():
        m = LINE.match(line)
        if m:
            answers[m.group(1)] = m.group(2)

    failures = []
    counts = {"matrix": 0, "forms": 0, "radix": 0, "inject": 0}

    def fail(name, what):
        failures.append("FAIL @%s (%s): %s" % (name, cases[name], what))

    for name, (kind, data) in cases.items():
        counts[kind] += 1
        got = answers.get(name)
        if got is None:
            fail(name, "no answer")
            continue
        if kind == "matrix":
            m = data
            cols = len(m[0])
            mm = re.match(r"rank (\d+), kernel (\[.*\])(?:, \|det\| (\d+))?$", got)
            if not mm:
                fail(name, "unparsed answer " + got)
                continue
            rank, basis = int(mm.group(1)), parse_matrix(mm.group(2)[1:-1]) if mm.group(2) != "[]" else []
            if rank != rank_q(m, cols):
                fail(name, "rank %d, expected %d" % (rank, rank_q(m, cols)))
            if len(basis) != cols - rank_q(m, cols):
                fail(name, "kernel of %d vectors for nullity %d" % (len(basis), cols - rank_q(m, cols)))
            for v in basis:
                if any(sum(a * b for a, b in zip(r, v)) != 0 for r in m):
                    fail(name, "kernel vector %s not in the kernel" % v)
            bad = check_hnf(basis)
            if bad:
                fail(name, bad)
            bound = 4 if cols <= 4 else 2
            for w in itertools.product(range(-bound, bound + 1), repeat=cols):
                if any(w) and all(sum(a * b for a, b in zip(r, w)) == 0 for r in m) and not in_lattice(w, basis):
                    fail(name, "kernel vector %s is not generated by %s" % (list(w), basis))
                    break
            if len(m) == cols:
                want = det_q(m)
                if mm.group(3) is None or int(mm.group(3)) != want:
                    fail(name, "|det| %s, expected %d" % (mm.group(3), want))
        elif kind == "forms":
            forms = data
            nonzero = [f for f in forms if any(f)]
            proportional = bool(nonzero) and all(
                f[s] * g[t] == f[t] * g[s] for f in nonzero for g in nonzero
                for s in range(len(f)) for t in range(len(f)))
            if not proportional:
                if got != "no common direction":
                    fail(name, "expected no common direction, got " + got)
                continue
            d = primitive(nonzero[0])
            lead = next(i for i, x in enumerate(d) if x != 0)
            mult = [f[lead] // d[lead] if any(f) else 0 for f in forms]
            want = "direction %s, multipliers %s" % (vec(d), vec(mult))
            if got != want:
                fail(name, "expected %s, got %s" % (want, got))
        elif kind == "radix":
            coefs, extents = data
            if any(n < 1 for n in extents):
                ok = False
            else:
                vals = [sum(c * z for c, z in zip(coefs, p))
                        for p in itertools.product(*[range(n) for n in extents])]
                ok = len(set(vals)) == len(vals) and max(vals) - min(vals) + 1 == len(vals)
            total = 1
            for n in extents:
                total *= n
            want = "mixed radix, extent %d" % total if ok else "not a mixed radix"
            if got != want:
                fail(name, "expected %s, got %s" % (want, got))
        else:
            li, lj, M, N = data
            seen, collides = {}, False
            for I in range(M):
                for J in range(N):
                    key = tuple(a * I + b * J for a, b in zip(li, lj))
                    if key in seen:
                        collides = True
                    seen[key] = (I, J)
            if not collides:
                if got != "injective":
                    fail(name, "expected injective, got " + got)
                continue
            mm = re.match(r"collision \((-?\d+), (-?\d+)\)$", got)
            if not mm:
                fail(name, "expected a collision, got " + got)
                continue
            dI, dJ = int(mm.group(1)), int(mm.group(2))
            if (dI, dJ) == (0, 0) or abs(dI) >= M or abs(dJ) >= N or \
                    any(a * dI + b * dJ != 0 for a, b in zip(li, lj)):
                fail(name, "reported collision (%d, %d) is not one" % (dI, dJ))

    for f in failures:
        print(f)
    print("lattice check: %d matrices, %d forms, %d radixes, %d injectivity cases, failures %d"
          % (counts["matrix"], counts["forms"], counts["radix"], counts["inject"], len(failures)))


if __name__ == "__main__":
    main()
