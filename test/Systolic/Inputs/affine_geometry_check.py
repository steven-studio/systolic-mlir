#!/usr/bin/env python3
"""Randomized cross-check of the geometric screening (screenAffineMatmulGeometry,
docs/AFFINE_GEMM_MAPPING.md section 19, layer 1) against an independent Python
implementation.

The nests are those of the GEMM mapping oracle (affine_gemm_mapping_oracle.py:
operands of rank 1..3, transposes, negative coefficients, split coordinates,
loops that run once, batches, redundant max/min bounds, perturbations). For each
one `systolic-opt --test-affine-matmul-discovery=geometry=true` prints the
screening; this script recomputes every printed fact from the nest itself:

  - per loop: the trip count; for loops of >= 2 iterations the readers (the
    operands with a nonzero coefficient) and the class (I, J, K, batch, one
    reader, no reader);
  - the ordinal rows (coefficient x step) of A, B and C (C from the accumulator
    load), in the order of the product's factors;
  - the ranks of C and of all subscripts over Q (fractions), on the loops of
    >= 2 iterations and on those that are not batch loops;
  - C's kernel: as many vectors as the nullity, each in the kernel, in Hermite
    normal form, generating every kernel vector of a small box (brute force);
  - the 'joins two points and is not a reduction' lines, from the basis;
  - the six-vector conditions A-D (operands of two dimensions each): ranks, and
    for C the existence of d (rational kernel), and the printed d itself;
  - the task algorithm of 2026-10-18 (docs section 19.10) line by line: the five
    rank conditions, 'A varies along null(Ic, Jc)' (some rational kernel basis
    vector has a nonzero dot with Ia or Ka -- the whole basis), the same for B,
    the two swap conditions, and the verdict (every 'return error' line passes).

Prints one line per failure and a summary.
"""

import argparse
import itertools
import os
import random
import re
import subprocess
import sys
from fractions import Fraction

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import affine_gemm_mapping_oracle as oracle  # noqa: E402


def rank_q(rows, cols):
    m = [[Fraction(v) for v in r] for r in rows]
    rank = 0
    for c in range(cols):
        p = next((r for r in range(rank, len(m)) if m[r][c] != 0), None)
        if p is None:
            continue
        m[rank], m[p] = m[p], m[rank]
        for r in range(len(m)):
            if r != rank and m[r][c] != 0:
                f = m[r][c] / m[rank][c]
                m[r] = [a - f * b for a, b in zip(m[r], m[rank])]
        rank += 1
    return rank


def rational_kernel(rows, cols):
    """A basis of the rational kernel (reduced row echelon)."""
    m = [[Fraction(v) for v in r] for r in rows]
    pivots, rank = [], 0
    for c in range(cols):
        p = next((r for r in range(rank, len(m)) if m[r][c] != 0), None)
        if p is None:
            continue
        m[rank], m[p] = m[p], m[rank]
        m[rank] = [v / m[rank][c] for v in m[rank]]
        for r in range(len(m)):
            if r != rank and m[r][c] != 0:
                f = m[r][c]
                m[r] = [a - f * b for a, b in zip(m[r], m[rank])]
        pivots.append(c)
        rank += 1
    basis = []
    for free in [c for c in range(cols) if c not in pivots]:
        v = [Fraction(0)] * cols
        v[free] = Fraction(1)
        for i, c in enumerate(pivots):
            v[c] = -m[i][free]
        basis.append(v)
    return basis


def dot(u, v):
    return sum(a * b for a, b in zip(u, v))


def in_lattice(w, basis):
    w = list(w)
    for row in basis:
        p = next(i for i, v in enumerate(row) if v != 0)
        if w[p] % row[p] != 0:
            return False
        q = w[p] // row[p]
        w = [a - q * b for a, b in zip(w, row)]
    return all(v == 0 for v in w)


def hnf_problem(basis):
    last = -1
    for i, row in enumerate(basis):
        p = next((c for c, v in enumerate(row) if v != 0), None)
        if p is None or p <= last or row[p] <= 0:
            return "kernel not in echelon form with positive pivots"
        for j in range(i):
            if not (0 <= basis[j][p] < row[p]):
                return "kernel entry above a pivot not reduced"
        last = p
    return None


BLOCK = "geometry screening (necessary conditions only, not a decision):"


def parse_matrix(text):
    if text.strip() == "[]":
        return []
    return [[int(x) for x in r.split(",") if x.strip()]
            for r in re.findall(r"\[([-\d, ]+)\]", text)]


def check(nest, lines):
    """Failures (strings) of one printed screening block against the nest."""
    out = []
    n = len(nest.loops)
    tr = [oracle.trips(*l) for l in nest.loops]
    if any(t == 0 for t in tr):
        if not (lines and lines[0].startswith("  not screened:")):
            out.append("an empty loop was screened")
        return out
    if lines and lines[0].startswith("  not screened:"):
        return ["not screened: " + lines[0]]
    steps = [st for _, _, st in nest.loops]
    first, second = nest.order
    rows = {"A": [[f[0][s] * steps[s] for s in range(n)] for f in nest.ops[first][1]],
            "B": [[f[0][s] * steps[s] for s in range(n)] for f in nest.ops[second][1]],
            "C": [[f[0][s] * steps[s] for s in range(n)] for f in nest.cload]}
    full = [s for s in range(n) if tr[s] >= 2]
    role = {}
    for s in full:
        rd = "".join(x for x in "ABC" if any(r[s] != 0 for r in rows[x]))
        role[s] = {"AC": "I", "BC": "J", "AB": "K", "ABC": "batch", "": "no reader"}.get(
            rd, "one reader (a broadcast)")
    roles = [s for s in full if role[s] != "batch"]
    expect = []
    for s in range(n):
        if tr[s] < 2:
            expect.append("  loop %d: trip %d, one iteration (a constant)" % (s, tr[s]))
            continue
        rd = " ".join(x for x in "ABC" if any(r[s] != 0 for r in rows[x]))
        name = "K (reduction)" if role[s] == "K" else role[s]
        expect.append("  loop %d: trip %d, readers %s -> %s" % (s, tr[s], rd, name) if rd else
                      "  loop %d: trip %d, readers -> %s" % (s, tr[s], name))
    for x in "ABC":
        expect.append("  %s ordinal rows: [%s]" % (x, ", ".join("[" + ", ".join(str(v) for v in r) + "]" for r in rows[x])))
    sub = lambda rs, cols: [[r[s] for s in cols] for r in rs]
    allrows = rows["A"] + rows["B"] + rows["C"]
    expect.append("  rank of C: %d (role space %d); rank of all subscripts: %d (role space %d)"
                  % (rank_q(sub(rows["C"], full), len(full)), rank_q(sub(rows["C"], roles), len(roles)),
                     rank_q(sub(allrows, full), len(full)), rank_q(sub(allrows, roles), len(roles))))
    for i, (want, got) in enumerate(zip(expect, lines)):
        if want != got:
            out.append("line %d: expected %r, got %r" % (i, want, got))
    rest = lines[len(expect):]
    if len(lines) < len(expect):
        return out + ["block too short"]
    # C's kernel.
    if not rest or not rest[0].startswith("  C kernel: "):
        return out + ["no C kernel line"]
    basis = parse_matrix(rest[0][len("  C kernel: "):])
    nullity = len(full) - rank_q(sub(rows["C"], full), len(full))
    if len(basis) != nullity:
        out.append("kernel of %d vectors for nullity %d" % (len(basis), nullity))
    for v in basis:
        if len(v) != n or any(v[s] != 0 for s in range(n) if s not in full):
            out.append("kernel vector %s has entries on loops of one iteration" % v)
        if any(dot(r, v) != 0 for r in rows["C"]):
            out.append("kernel vector %s not in C's kernel" % v)
    restricted = [[v[s] for s in full] for v in basis]
    problem = hnf_problem(restricted)
    if problem:
        out.append(problem)
    if len(full) <= 4 and not problem:
        b = 2 if len(full) == 4 else 3
        for w in itertools.product(range(-b, b + 1), repeat=len(full)):
            if any(w) and all(dot([r[s] for s in full], w) == 0 for r in rows["C"]) and \
                    not in_lattice(w, restricted):
                out.append("kernel vector %s (on the loops %s) is not generated" % (list(w), full))
                break
    # Reduction loops and the 'joins two points' lines.
    red = [s for s in full if role[s] == "K"]
    want_red = "  reduction loops:" + ("".join(" %d" % s for s in red) if red else " none")
    if len(rest) < 2 or rest[1] != want_red:
        out.append("expected %r, got %r" % (want_red, rest[1] if len(rest) > 1 else None))
    joins = [v for v in basis if all(abs(v[s]) < tr[s] for s in full) and
             any(v[s] != 0 and role[s] != "K" for s in full)]
    got_joins = [parse_matrix("[" + m.group(1) + "]")[0] for m in
                 (re.match(r"  C kernel direction \[([-\d, ]+)\] joins", l) for l in rest[2:]) if m]
    if got_joins != joins:
        out.append("joining directions %s, expected %s" % (got_joins, joins))
    # Six-vector conditions.
    cond_lines = [l for l in rest if l.startswith("  six-vector conditions")]
    task_lines = [l for l in rest if l.startswith("  task algorithm")]
    two = all(len(rows[x]) == 2 for x in "ABC")
    if not two:
        if cond_lines or task_lines:
            out.append("six-vector conditions printed for operands not of two dimensions")
        return out
    if len(cond_lines) != 2 or len(task_lines) != 2:
        return out + ["expected two six-vector and two task-algorithm lines, got %d and %d"
                      % (len(cond_lines), len(task_lines))]
    TASK = re.compile(r"  task algorithm \((.*)\): rank\(Ic,Jc\)=2 (yes|no), rank\(Ia,Ka\)=2 (yes|no), "
                      r"rank\(Kb,Jb\)=2 (yes|no), rank\(six\)=3 (yes|no), rank\(Ia,Ka,Kb,Jb\)=3 (yes|no), "
                      r"A varies along null\(Ic,Jc\) (yes|no), B varies along null\(Ic,Jc\) (yes|no), "
                      r"swap\(Ia,Ka\) (yes|no), swap\(Kb,Jb\) (yes|no) -> (passes|error)$")
    for space, cols, line in (("full space", full, task_lines[0]), ("role space", roles, task_lines[1])):
        cI, cJ = [[r[s] for s in cols] for r in rows["C"]]
        aI, aK = [[r[s] for s in cols] for r in rows["A"]]
        bK, bJ = [[r[s] for s in cols] for r in rows["B"]]
        rk = lambda *vs: rank_q(list(vs), len(cols))
        yn = lambda b: "yes" if b else "no"
        ker = rational_kernel([cI, cJ], len(cols)) if cols else []
        varies = lambda v: any(dot(v, w) != 0 for w in ker)
        a_var, b_var = varies(aI) or varies(aK), varies(bK) or varies(bJ)
        r1, r2, r3, r4, r5 = (rk(cI, cJ) == 2, rk(aI, aK) == 2, rk(bK, bJ) == 2,
                              rk(cI, cJ, aI, aK, bK, bJ) == 3, rk(aI, aK, bK, bJ) == 3)
        want = (yn(r1), yn(r2), yn(r3), yn(r4), yn(r5), yn(a_var), yn(b_var),
                yn(varies(aI) and not varies(aK)), yn(not varies(bK) and varies(bJ)),
                "passes" if r1 and r2 and r3 and r4 and r5 and a_var and b_var else "error")
        m = TASK.match(line)
        if not m or m.group(1) != space:
            out.append("unparsed task-algorithm line %r" % line)
            continue
        if m.groups()[1:] != want:
            out.append("%s: task algorithm %s, expected %s" % (space, m.groups()[1:], want))
    for space, cols, line in (("full space", full, cond_lines[0]), ("role space", roles, cond_lines[1])):
        cI, cJ = [[r[s] for s in cols] for r in rows["C"]]
        aI, aK = [[r[s] for s in cols] for r in rows["A"]]
        bK, bJ = [[r[s] for s in cols] for r in rows["B"]]
        rk = lambda *vs: rank_q(list(vs), len(cols))
        yn = lambda b: "yes" if b else "no"
        ker = rational_kernel([cI, cJ], len(cols)) if cols else []
        c_ok = bool(cols) and any(dot(aK, v) != 0 for v in ker) and any(dot(bK, v) != 0 for v in ker)
        m = re.match(r"  six-vector conditions \((.*)\): A (yes|no), B (yes|no), C (yes|no)(?: \(d = \[([-\d, ]*)\]\))?, D (yes|no) (yes|no) (yes|no)$", line)
        if not m or m.group(1) != space:
            out.append("unparsed six-vector line %r" % line)
            continue
        want = (yn(rk(cI, cJ) == 2), yn(rk(cI, cJ, aI, aK, bK, bJ) == 3), yn(c_ok),
                yn(rk(aI, aK) == 2), yn(rk(bK, bJ) == 2), yn(rk(aI, bJ) == 2))
        got = (m.group(2), m.group(3), m.group(4), m.group(6), m.group(7), m.group(8))
        if want != got:
            out.append("%s: conditions %s, expected %s" % (space, got, want))
        if m.group(5) is not None:
            d = [int(x) for x in m.group(5).split(",")]
            dc = [d[s] for s in cols]
            if len(d) != n or any(d[s] != 0 for s in range(n) if s not in cols) or \
                    dot(cI, dc) != 0 or dot(cJ, dc) != 0 or dot(aK, dc) == 0 or dot(bK, dc) == 0:
                out.append("%s: printed d = %s is not a witness" % (space, d))
        elif c_ok:
            out.append("%s: condition C holds but no d printed" % space)
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--opt", default="systolic-opt")
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--count", type=int, default=300)
    a = ap.parse_args()
    rng = random.Random(a.seed)
    nests = [oracle.generate(rng) for _ in range(a.count)]
    for i, nest in enumerate(nests):
        # The screening is defined on a box (docs section 19.2, 19.12): the
        # compiler normalizes a constant-extent parallelotope onto its box
        # first, which this check then recomputes on the oracle's box nest. A
        # 'triangle' (bounds that do not shift together) is outside that
        # family: it must come back 'not screened' (checked below).
        oracle.decorate_bounds(random.Random("bounds:%d:%d" % (a.seed, i)), nest)
    text = "\n// -----\n".join(oracle.emit(nest, "case_%d" % i) for i, nest in enumerate(nests))
    run = subprocess.run([a.opt, "--test-affine-matmul-discovery=geometry=true", "-split-input-file",
                          "-", "-o", "/dev/null"], input=text, capture_output=True, text=True)
    if run.returncode != 0:
        print("systolic-opt failed (rc %d):\n%s" % (run.returncode, run.stderr[-3000:]))
        sys.exit(1)
    # One split per nest (each starts with 'candidate count:'); a nest may
    # yield several candidates (inner sub-chains too): its own block is the
    # one that screens all of its loops.
    splits, blocks, cur = [], None, None
    for line in run.stderr.splitlines():
        if line.startswith("candidate count:"):
            blocks = []
            splits.append(blocks)
            cur = None
        elif line == BLOCK and blocks is not None:
            cur = []
            blocks.append(cur)
        elif cur is not None and line.startswith("  ") and not line.startswith("  reason:") \
                and not line.startswith("  class:"):
            cur.append(line)
        else:
            cur = None
    failures = []
    if len(splits) != len(nests):
        print("FAIL: %d splits for %d nests" % (len(splits), len(nests)))
        sys.exit(1)

    def own_block(nest, candidates):
        n = len(nest.loops)
        for lines in candidates:
            if lines and lines[0].startswith("  not screened"):
                continue
            if sum(1 for l in lines if l.startswith("  loop ")) == n:
                return lines
        return candidates[-1] if candidates else []
    blocks = [own_block(nest, c) for nest, c in zip(nests, splits)]
    screened = 0
    triangles = 0
    for i, (nest, lines) in enumerate(zip(nests, blocks)):
        if nest.domain_mode == "triangle":
            # not in the normalized family: the discovery pass must leave the
            # nest as written and screen nothing
            triangles += 1
            if not (lines and lines[0].startswith("  not screened")):
                failures.append("FAIL case_%d [%s, triangle]: a domain outside the normalized family "
                                "was screened" % (i, nest.mode))
            continue
        screened += not (lines and lines[0].startswith("  not screened"))
        for f in check(nest, lines):
            failures.append("FAIL case_%d [%s]: %s" % (i, nest.mode, f))
    for f in failures:
        print(f)
    print("geometry check: %d nests, %d screened, %d triangles not screened, failures %d"
          % (len(nests), screened, triangles, len(failures)))


if __name__ == "__main__":
    main()
