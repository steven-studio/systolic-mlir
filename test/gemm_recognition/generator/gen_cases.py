#!/usr/bin/env python3
"""
gen_cases.py -- the unbounded case generator.

    python3 generator/gen_cases.py --mode random    --seed S --count N [--start I]
    python3 generator/gen_cases.py --mode enumerate --count N [--start I]
    python3 generator/gen_cases.py --mode boundary  --count N [--start I]
        [--family positive|views|mutation|mixed] [--out DIR] [--registry FILE]
        [--max-extent E] [--max-loops L]

Every call writes N *new* cases (never seen before, by canonical signature) to
DIR/gen_<signature>.c with DIR/manifest.json, in exactly the format of the
showcase set (generator/casespec.py: header, kernel, two or three independent
references, Python-oracle checksum).  Each case file is made read-only (mode
0555) as soon as it is written; the batch's JSON files stay writable.  Nothing limits the total: the sequence
is infinite -- `--start` continues where a previous run stopped, the seed and
index of every case are recorded, and `gen_cases.py --reproduce SEED:INDEX`
(or MODE:INDEX) re-creates one case bit for bit.

  random      a pseudo-random walk over the whole index family (seeded)
  enumerate   a deterministic, exhaustive walk: feature tuple = index mod F,
              shape = Cantor-unpaired index div F -- every combination of loop
              order, origins, steps, offsets, strides, splits and repetitions
              with every shape, in a fixed order, forever
  boundary    the same walk over boundary values: extents 1, k_max-1, k_max,
              k_max+1, 2 k_max, 3 k_max+1; offsets and strides that reach the
              last element of the array; origins that cancel negative constants

Families (what the nests are):
  positive  dense GEMMs inside the current implementation's envelope: all
            operands spell a shared coordinate the same way (the showcase
            positives' family, expected offloaded)
  views     dense GEMMs with per-operand offsets / strides on a shared
            coordinate -- valid GEMMs by the specification, refused by the
            recognizer at 6e25528 (ISSUE-001); generated to track the issue
  mutation  one deliberate mutation of a positive nest: a transposed operand,
            a coupled or singular mapping, element-wise, the reduction index
            in the output, no accumulation, subtraction, alpha, init inside,
            double, no restrict, reversed rows, a repetition loop inside the
            reduction, a third factor, a sum -- to check that the compiler
            keeps refusing what the specification refuses (never a false
            positive) and that every program still computes the right thing
  mixed     all of the above (the default: 60% positive, 15% views, 25% mutation)

Generator rejections: a candidate that is not a valid program (a subscript
out of its array, a zero-trip loop, an extent beyond --max-extent, a nest
deeper than --max-loops, a duplicate) is recorded in DIR/rejected.json with
the reason and is never counted as a compiler result.

Coverage: DIR/../coverage.json accumulates, over every batch, how many cases
exercised each feature (loop order, non-zero origin, step > 1, offsets,
strides, splits, repetitions, statement forms, shape classes, families,
classifications), so that the exploration can be steered.
"""

import argparse
import hashlib
import itertools
import json
import os
import random
import sys
import time
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import casespec  # noqa: E402
from casespec import Array, Case, Loop  # noqa: E402

K_MAX = 16  # the fleet's k_max (K_MAX_8X8 = K_MAX_4X4 = 16)

ORDERS = ["ijk", "ikj", "jik", "jki", "kij", "kji"]
STMTS = ["+=", "=C+AB", "=AB+C", "+=BA"]
MUTATIONS = ["transpose_A", "transpose_B", "transpose_C", "coupled_C_row", "coupled_A_and_C_row",
             "singular", "elementwise", "output_in_reduction", "no_accumulation", "subtract", "alpha",
             "init_inside", "double", "no_restrict", "reversed_rows", "repeat_inside_K", "triple", "sum",
             "batched", "scalar_acc", "batched_shared_B", "batched_shared_A", "batched_c_only"]

# --------------------------------------------------------------------------
# building blocks
# --------------------------------------------------------------------------


class Rejected(Exception):
    pass


def cantor_unpair3(n):
    """n -> (a, b, c) in N^3, a bijection (two Cantor unpairings)."""
    def unpair(z):
        w = int(((8 * z + 1) ** 0.5 - 1) // 2)
        while (w + 1) * (w + 2) // 2 <= z:
            w += 1
        while w * (w + 1) // 2 > z:
            w -= 1
        t = w * (w + 1) // 2
        y = z - t
        x = w - y
        return x, y
    a, rest = unpair(n)
    b, c = unpair(rest)
    return a, b, c


def shape_from_index(n, mode):
    """The (M, N, K) of enumeration index n: small shapes first."""
    a, b, c = cantor_unpair3(n)
    if mode == "boundary":
        bvals = [1, 2, K_MAX - 1, K_MAX, K_MAX + 1, 2 * K_MAX, 3 * K_MAX + 1, 7, 9, 12]
        return bvals[a % len(bvals)], bvals[b % len(bvals)], bvals[c % len(bvals)]
    return a + 1, b + 1, c + 1


class Params:
    """One point of the feature space; built either from a Random or from an
    enumeration index.  Every field is a small choice."""

    FIELDS = [
        ("order", 6), ("origin_pattern", 4), ("step_pattern", 4), ("offset_pattern", 5),
        ("stride_pattern", 4), ("split_pattern", 5), ("rep_pattern", 4), ("stmt", 4),
        ("syntax", 3), ("slack", 2),
    ]

    @classmethod
    def space_size(cls):
        n = 1
        for _, k in cls.FIELDS:
            n *= k
        return n

    @classmethod
    def from_index(cls, idx):
        p = cls()
        for name, k in cls.FIELDS:
            setattr(p, name, idx % k)
            idx //= k
        return p

    @classmethod
    def from_random(cls, rng):
        p = cls()
        for name, k in cls.FIELDS:
            setattr(p, name, rng.randrange(k))
        return p


def positive_case(cid, M, N, K, p: Params, family, rng, max_extent, max_loops, seed_tag):
    """A dense GEMM from parameters.  family 'positive': shared views;
    'views': per-operand offsets/strides on one or more coordinates."""
    if min(M, N, K) < 1:
        raise Rejected("empty extent")
    if max(M, N, K) > max_extent:
        raise Rejected("extent %d beyond --max-extent %d" % (max(M, N, K), max_extent))
    # --- loops per coordinate: 1 or 2 (or 3 for K), splitting the extent
    groups = {}          # g -> list of (var, lb, trip, step, coef)
    ext = {"I": M, "J": N, "K": K}
    split = {"I": 0, "J": 0, "K": 0}
    sp = p.split_pattern
    if sp == 1:
        split["K"] = 1
    elif sp == 2:
        split["I"], split["J"] = 1, 1
    elif sp == 3:
        split["I"], split["J"], split["K"] = 1, 1, 1
    elif sp == 4:
        split["K"] = 2
    origins = [(0, 0, 0), (2, 0, 0), (0, 3, 1), (4, 1, 2)][p.origin_pattern]
    steps = [(1, 1, 1), (2, 1, 1), (1, 1, 3), (2, 2, 1)][p.step_pattern]
    base_names = {"I": "i", "J": "j", "K": "k"}
    for gi, g in enumerate("IJK"):
        e = ext[g]
        parts = []
        if split[g] == 0 or e < 4:
            parts = [e]
        else:
            # a radix r that divides e: prefer 2..8
            cands = [r for r in range(2, min(e, 9)) if e % r == 0]
            if not cands:
                parts = [e]
            else:
                r = cands[(p.order + gi) % len(cands)] if family != "random" else rng.choice(cands)
                parts = [e // r, r] if split[g] == 1 else [e // r, r]
                if split[g] == 2 and (e // r) >= 4:
                    inner = [q for q in range(2, e // r) if (e // r) % q == 0]
                    if inner:
                        q = inner[0]
                        parts = [e // r // q, q, r]
        lps = []
        if len(parts) == 1:
            lb = origins[gi]
            st = steps[gi]
            lps.append([base_names[g], lb, e, st, 1])   # var, lb, trip, step, coef (filled later)
        else:
            # mixed radix, steps 1: coordinate = sum coef_l * var_l
            mult = 1
            for li, trip in reversed(list(enumerate(parts))):
                lps.append([base_names[g] + str(li), 0, trip, 1, mult])
                mult *= trip
            lps.reverse()   # coarse first in the list (order randomised below)
        groups[g] = lps

    # --- per-operand views: offsets and strides on a coordinate
    # offset_pattern: 0 none, 1 I, 2 J, 3 K, 4 all; stride_pattern: 0 none, 1 I, 2 J, 3 K
    offs = {"I": 0, "J": 0, "K": 0}
    strd = {"I": 1, "J": 1, "K": 1}
    if p.offset_pattern in (1, 4):
        offs["I"] = 2
    if p.offset_pattern in (2, 4):
        offs["J"] = 3
    if p.offset_pattern in (3, 4):
        offs["K"] = 1
    if p.stride_pattern == 1:
        strd["I"] = 2
    elif p.stride_pattern == 2:
        strd["J"] = 3
    elif p.stride_pattern == 3:
        strd["K"] = 2
    # per operand (family 'views'): a second set that differs on one coordinate
    per_op = {"a": dict(I=(strd["I"], offs["I"]), K=(strd["K"], offs["K"])),
              "b": dict(K=(strd["K"], offs["K"]), J=(strd["J"], offs["J"])),
              "c": dict(I=(strd["I"], offs["I"]), J=(strd["J"], offs["J"]))}
    if family == "views":
        which = ["c_I_offset", "c_J_offset", "a_I_offset", "a_K_offset", "c_I_stride", "b_J_stride"][
            rng.randrange(6) if rng else p.offset_pattern % 6]
        op, coord, kind = which.split("_")
        s0, o0 = per_op[op][coord]
        per_op[op][coord] = (s0 * 2, o0) if kind == "stride" else (s0, o0 + 5)

    def expr(op, g):
        """The subscript of operand `op` for coordinate g: coef*var terms + offset."""
        scale, off = per_op[op][g]
        terms = []
        for var, lb, trip, st, coef in groups[g]:
            c = coef * scale
            terms.append(var if c == 1 else "%d * %s" % (c, var))
        e = " + ".join(terms)
        if off:
            e = "%s + %d" % (e, off) if p.syntax != 2 else "%d + %s" % (off, e)
        return e

    def span(op, g):
        scale, off = per_op[op][g]
        lo = hi = off
        for var, lb, trip, st, coef in groups[g]:
            c = coef * scale
            lo += c * lb
            hi += c * (lb + st * (trip - 1))
        return lo, hi

    # --- the loop nest: all loops in a chosen order, repetition loops inserted
    all_loops = []
    for g in "IJK":
        for var, lb, trip, st, coef in groups[g]:
            all_loops.append(Loop(var=var, lb=lb, ub=lb + st * trip, step=st, role=g))
    order = ORDERS[p.order]
    # order the groups by `order`, then interleave split loops deterministically
    by_group = {g: [lp for lp in all_loops if lp.role == g] for g in "IJK"}
    seq = []
    for ch in order:
        seq += by_group[ch.upper()]
    if p.split_pattern in (2, 3) and len(seq) >= 5:
        # tiled nests: the coarse loops first, then the fine ones (I0 J0 [K0] I1 J1 [K1])
        coarse = [lp for lp in seq if lp.var.endswith("0")]
        fine = [lp for lp in seq if not lp.var.endswith("0")]
        seq = coarse + fine
    reps = [(0, 0), (1, 0), (2, 1), (3, 2)][p.rep_pattern]   # (count, placement)
    nrep, placement = reps
    if nrep:
        k_positions = [i for i, lp in enumerate(seq) if lp.role == "K"]
        first_k = min(k_positions)
        for r in range(nrep):
            trip = 2 + r
            # placement 0: outside; 1: between loops but before the first K loop; 2: just before the first K loop
            if placement == 0:
                pos = 0
            elif placement == 1:
                pos = min(1, first_k)
            else:
                pos = first_k
            seq.insert(pos, Loop(var="r%d" % r, lb=0, ub=trip, step=1, role="R"))
            k_positions = [i for i, lp in enumerate(seq) if lp.role == "K"]
            first_k = min(k_positions)
    if len(seq) > max_loops:
        raise Rejected("nest of %d loops exceeds --max-loops %d" % (len(seq), max_loops))
    # syntax variations
    if p.syntax == 1:
        for lp in seq:
            if lp.step == 1:
                lp.cmp = "<="
                lp.ub = lp.ub - 1
                lp.inc = "post++"
    # --- arrays sized to the spans (+ slack on the leading dimension)
    def dims(op, g0, g1):
        lo0, hi0 = span(op, g0)
        lo1, hi1 = span(op, g1)
        if lo0 < 0 or lo1 < 0:
            raise Rejected("negative subscript on %s" % op)
        slack = 3 if p.slack else 0
        return (hi0 + 1 + slack, hi1 + 1)

    A = Array("A", dims("a", "I", "K"), (expr("a", "I"), expr("a", "K")))
    B = Array("B", dims("b", "K", "J"), (expr("b", "K"), expr("b", "J")))
    C = Array("C", dims("c", "I", "J"), (expr("c", "I"), expr("c", "J")))
    for arr in (A, B, C):
        if arr.dims[0] * arr.dims[1] > 65536:
            raise Rejected("array %s of %dx%d elements is too large" % (arr.name, *arr.dims))
    stmt = STMTS[p.stmt]
    name = "gen_%s_%s_%dx%dx%d" % (family, order, M, N, K)
    case = Case(id=cid, name=name, set="generated", category="generated_" + family, loops=seq,
                arrays={"a": A, "b": B, "c": C}, stmt=stmt,
                # the data seed must be a deterministic function of the tag (Python's
                # str hash is randomised per process, which made --reproduce inexact)
                seed=(zlib.crc32(seed_tag.encode()) & 0xff) + 1)
    return case


def mutate(case: Case, mutation: str, rng) -> Case:
    """One deliberate deviation from a dense GEMM (or a documented limitation)."""
    import copy
    c = copy.deepcopy(case)
    c.name = c.name + "_" + mutation
    c.category = "mutation_" + mutation
    A, B, C = c.arrays["a"], c.arrays["b"], c.arrays["c"]
    roles = {lp.var: lp.role for lp in c.loops}

    def vars_of(g):
        return [lp.var for lp in c.loops if lp.role == g]

    def first(g):
        return vars_of(g)[0]

    if mutation == "transpose_A":
        A.subs = (A.subs[1], A.subs[0])
        A.dims = (A.dims[1], A.dims[0])
    elif mutation == "transpose_B":
        B.subs = (B.subs[1], B.subs[0])
        B.dims = (B.dims[1], B.dims[0])
    elif mutation == "transpose_C":
        C.subs = (C.subs[1], C.subs[0])
        C.dims = (C.dims[1], C.dims[0])
    elif mutation == "coupled_C_row":
        j = first("J")
        C.subs = ("%s + %s" % (C.subs[0], j), C.subs[1])
        C.dims = (C.dims[0] + max(lp.ub for lp in c.loops if lp.var == j), C.dims[1])
        c.math_class = "gemm_values_skewed_placement"
        c.math_note = ("The product's value for (i, j) lands at a skewed row C[I + j][J]: with J one loop "
                       "an affine, injective (skewed) view of C, a GEMM by docs/AFFINE_GEMM_MAPPING.md "
                       "section 19 (recognized since 2026-10-16, lowered as one memref.reinterpret_cast "
                       "since 2026-10-18, section 19.11; ISSUE-008); with J over several loops C reads J "
                       "through two combinations (valid only by demotion, not constructed).")
    elif mutation == "coupled_A_and_C_row":
        j = first("J")
        A.subs = ("%s + %s" % (A.subs[0], j), A.subs[1])
        A.dims = (A.dims[0] + max(lp.ub for lp in c.loops if lp.var == j), A.dims[1])
        C.subs = ("%s + %s" % (C.subs[0], j), C.subs[1])
        C.dims = (C.dims[0] + max(lp.ub for lp in c.loops if lp.var == j), C.dims[1])
        c.math_class = "gemm_nonrectangular_region"
        c.math_note = ("A[I + j][K], C[I + j][J]: with J one loop, j is read by A, B and C -- a batch "
                       "loop -- and per j the GEMV C[I + j][j] += A[I + j][K] B[K][j], a batched GEMM of "
                       "affine views (docs/AFFINE_GEMM_MAPPING.md section 19.3, ISSUE-008); one "
                       "rectangular GEMM it is not (its outputs form a parallelogram).")
    elif mutation == "singular":
        k = first("K")
        A.subs = ("%s + %s" % (A.subs[0], k), A.subs[1])
        A.dims = (A.dims[0] + max(lp.ub for lp in c.loops if lp.var == k), A.dims[1])
        c.math_class = "skewed_a_row"
        c.math_note = ("A's row index adds the reduction loop: with K one loop, A's view (I, K) -> "
                       "(I + K, K) is affine, a GEMM by docs/AFFINE_GEMM_MAPPING.md section 19 "
                       "(recognized since 2026-10-16, lowered as one memref.reinterpret_cast since "
                       "2026-10-18, section 19.11; ISSUE-008) -- the old rank test ('a singular mapping') "
                       "does not decide it; with K over several loops A reads K through two combinations "
                       "(valid only by demotion, not constructed).")
    elif mutation == "elementwise":
        A.subs = C.subs
        A.dims = C.dims
        B.subs = C.subs
        B.dims = C.dims
        for lp in c.loops:
            if lp.role == "K":
                lp.role = "R"
        c.math_class = "elementwise_product"
        c.math_note = "C += (reduction trip) * (A o B): the Hadamard product, no reduction coordinate."
    elif mutation == "output_in_reduction":
        C.subs = (C.subs[0], A.subs[1])
        C.dims = (C.dims[0], A.dims[1])
        c.math_class = "not_gemm_scaled_rows"
        c.math_note = ("The output is indexed by the reduction index; the column loop is summed away: "
                       "the reduction loop is read by all three operands (a batch loop), the column loop "
                       "by B only -- a batched GEMM of scaled vectors once the column loop is demoted to "
                       "a batch loop (docs/AFFINE_GEMM_MAPPING.md section 19.2; not constructed: a "
                       "documented limitation), lowered directly when it runs once (SQ-5, decided "
                       "2026-10-16).")
    elif mutation == "no_accumulation":
        c.stmt = "=AB"
        if casespec.c_written_once(c):
            # every element of C written once: the K = 1 GEMM into a zeroed C
            # (docs/AFFINE_GEMM_MAPPING.md section 20, 2026-10-19), a GEMM
            c.math_class = ""
            c.math_note = ("C = A * B writes every element of C exactly once: the K = 1 GEMM into a "
                           "zeroed C, C = 0 + A B (docs/AFFINE_GEMM_MAPPING.md section 20).")
        else:
            c.math_class = "not_gemm_overwrite"
            c.math_note = "The last reduction iteration overwrites C: an outer product of the last column/row."
    elif mutation == "subtract":
        c.stmt = "-="
    elif mutation == "alpha":
        c.stmt = "+=alpha"
        c.alpha = 2.0
    elif mutation == "init_inside":
        if c.loops[-1].role != "K" or len(vars_of("K")) != 1 or any(lp.role == "R" for lp in c.loops):
            raise Rejected("init_inside needs a single reduction loop innermost and no repetition")
        c.kernel_style = "init_inside"
    elif mutation == "scalar_acc":
        if c.loops[-1].role != "K" or len(vars_of("K")) != 1 or any(lp.role == "R" for lp in c.loops):
            raise Rejected("scalar_acc needs a single reduction loop innermost and no repetition")
        c.kernel_style = "scalar_acc"
    elif mutation == "double":
        c.elem = "double"
        for arr in (A, B, C):
            arr.elem = "double"
    elif mutation == "no_restrict":
        for arr in (A, B, C):
            arr.restrict = False
    elif mutation == "reversed_rows":
        i = first("I")
        lp = [l for l in c.loops if l.var == i][0]
        top = lp.lb + lp.step * (lp.trip() - 1)
        if len(vars_of("I")) != 1:
            raise Rejected("reversed_rows needs a single row loop")
        A.subs = (A.subs[0].replace(i, "(%d - %s)" % (top + lp.lb, i)), A.subs[1])
        C.subs = (C.subs[0].replace(i, "(%d - %s)" % (top + lp.lb, i)), C.subs[1])
    elif mutation == "repeat_inside_K":
        c.loops.append(Loop(var="rr", lb=0, ub=2, step=1, role="R"))
    elif mutation == "triple":
        c.stmt = "+=triple"
        c.arrays["d"] = Array("D", B.dims, B.subs)
        c.math_class = "gemm_of_hadamard_operand"
        c.math_note = "A times (B o D): a GEMM of A and a Hadamard product, not of the three arrays."
    elif mutation == "sum":
        c.stmt = "+=sum"
        c.math_class = "not_gemm_sum"
        c.math_note = "Row sums of A plus column sums of B; no products."
    elif mutation == "batched":
        nb = 2
        c.loops.insert(0, Loop(var="b", lb=0, ub=nb, step=1, role="P"))
        for arr in (A, B, C):
            arr.subs = ("b",) + tuple(arr.subs)
            arr.dims = (nb,) + tuple(arr.dims)
    elif mutation in ("batched_shared_B", "batched_shared_A", "batched_c_only"):
        # The dimension-intersection partitioning (docs/AFFINE_GEMM_MAPPING.md
        # section 21, 2026-10-19): a batch coordinate one factor (or both) does
        # not read -- C[b] += A[b] B, C[b] += A B[b], C[b] += A B -- is a
        # partition of the GEMM into the slabs C[b], the factor(s) without it
        # shared by the partitions.
        nb = 2
        carried = {"batched_shared_B": (A, C), "batched_shared_A": (B, C), "batched_c_only": (C,)}[mutation]
        # A factor is shared only when the role b sits next to (I when B does
        # not read b, J when A does not) has a loop of two or more iterations:
        # when that role runs once, b would simply BE the role (a loop of one
        # iteration is a constant on the domain -- the readers' canonical
        # decomposition, a plain GEMM of M or N = nb, not a partition), so
        # every operand carries b instead (the batched mutation's shape).
        joined = {"batched_shared_B": "I", "batched_shared_A": "J"}.get(mutation)
        role_runs_once = joined is not None and all(lp.trip() == 1 for lp in c.loops if lp.role == joined)
        if role_runs_once:
            carried = (A, B, C)
        c.loops.insert(0, Loop(var="b", lb=0, ub=nb, step=1, role="P"))
        for arr in carried:
            arr.subs = ("b",) + tuple(arr.subs)
            arr.dims = (nb,) + tuple(arr.dims)
        c.math_class = ""
        if role_runs_once:
            c.math_note = ("A batched GEMM, every operand reading the batch coordinate b (ISSUE-006): "
                           "the %s role runs once, so a factor not reading b would make b that role "
                           "(a plain GEMM), not a partition." % joined)
        else:
            c.math_note = ("A batched GEMM with %s not reading the batch coordinate b: a partition on b "
                           "(docs/AFFINE_GEMM_MAPPING.md section 21), the shared factor(s) read by every "
                           "partition." % {"batched_shared_B": "B", "batched_shared_A": "A",
                                           "batched_c_only": "A and B"}[mutation])
    else:
        raise ValueError(mutation)
    c.arrays = {k: v for k, v in c.arrays.items()}
    return c


# --------------------------------------------------------------------------
# the 'domains' family: iteration domains that are not boxes, one array as
# both factors, K = 1 without a reduction loop (docs/AFFINE_GEMM_MAPPING.md
# section 19.12, 2026-10-18) -- variants of a positive case
# --------------------------------------------------------------------------

DOMAIN_VARIANTS = ["shift", "shear", "chain", "step_shift", "shift_same_base", "same_base",
                   "same_view", "k1_const", "triangle", "alias_c", "alias_c_b", "shift_batched",
                   "disguise", "rep_triangle", "shift_cond"]
DOMAIN_WEIGHTS = [14, 12, 10, 7, 7, 10, 5, 8, 7, 5, 3, 6, 10, 8, 6]
DOMAIN_NOTES = {
    "shift": "a loop's two bounds shifted by an enclosing counter (k in [i + c, i + c + K)): a "
             "constant-extent parallelotope, the same GEMM on the box x = U x' (docs section 19.12)",
    "shear": "a loop's two bounds shifted by a multiple of an enclosing counter, or by two of them: "
             "a sheared parallelotope, the same GEMM on its box (docs section 19.12)",
    "chain": "two shifted loops, the inner one shifted by the outer shifted one (chained shifts: U "
             "composes them), the same GEMM on the box (docs section 19.12)",
    "step_shift": "a loop of step > 1 shifted by an enclosing counter (docs section 19.12)",
    "shift_same_base": "a shifted loop and one array as both factors through two views (docs "
                       "sections 7, 19.12)",
    "same_base": "one array as both factors, each through its own affine view (A[i][k] * A[k][j] "
                 "with different offsets or strides, A . A^T): two read-only views of the shared "
                 "base, lowered since 2026-10-18 (docs sections 7, 19.12)",
    "same_view": "one array as both factors through the SAME element map (A[f(i)][g(k)] * "
                 "A[f(k)][g(j)]): one tensor serves both (docs section 7)",
    "k1_const": "K = 1 with no reduction loop: the reduction index a constant (C[i][j] += A[i][c] * "
                "B[c][j]), a GEMM of reduction extent one (SQ-4, decided 2026-10-15; the K = 1 "
                "requirement of 2026-10-18)",
    "triangle": "the lower bound rises with an enclosing counter, the upper bound does not: the trip "
                "count varies, the iteration set is a triangle inside the box -- not centrally "
                "symmetric, no affine image of a box (docs section 19.7); translated by the C front end "
                "since 2026-10-18 (exact subscript ranges by running the loops, docs section 19.13). "
                "The specification refuses it when its updates (F_A, F_B, F_C over the points) are not "
                "centrally symmetric either: no GEMM in any decomposition (a split coordinate whose "
                "loops are read only together can make them symmetric, even a GEMM: k0 - 4 k1 with k0 in "
                "[k1, 4)). The compiler proves it no GEMM (PROVEN_INVALID) when the accesses tell the "
                "iterations apart (the stacked access map has rank n, the number of loops of its "
                "candidate band: the nest below a leading run of loops no access reads whose counters "
                "no loop below the run reads, their bounds may read each other), the factors and C are "
                "three "
                "different arrays, no stepped loop's lower bound reads a counter and every written "
                "bound stays within 64 bits (docs section 19.13.4); UNKNOWN otherwise",
    "alias_c": "the output is the same array as the first factor (A[i][j] += A[i][k] * B[k][j]): the "
               "factor may alias C, refused ('A may overlap C'); the specification decides by the "
               "addresses whether a product reads an element written earlier (not a GEMM of the "
               "operands: REJECT) or not (a GEMM refused conservatively: OUT_OF_SCOPE)",
    "alias_c_b": "the output is the same array as the second factor (B[i][j] += A[i][k] * B[k][j]): "
                 "as alias_c, with B",
    "shift_batched": "a batched GEMM (a batch loop around the nest, every operand carrying its "
                     "coordinate) with a loop shifted by an enclosing counter -- the batch loop or "
                     "another: a sheared batched nest through the front end (docs section 19.12)",
    "disguise": "a box in disguise (docs section 19.13): an inner loop's lower bound rises with an "
                "enclosing counter so steeply that its range is empty except at that counter's first "
                "value -- a triangle as written (the bounds do not shift together), whose integer "
                "points are the box of the positive case with that counter fixed: normalized exactly "
                "and offloaded",
    "rep_triangle": "two repetition loops in a triangle (r1 in [0, 3), r0 in [r1, 2): three points) "
                    "outside or between the GEMM loops: independent of them, so every point of the "
                    "GEMM runs three times; hoisted around the GEMM (docs section 19.13) when that is "
                    "a legal interchange",
    "shift_cond": "a shifted loop written with its condition over the enclosing counters (for (int k "
                  "= i + c; k - i < c + n; ++k)): the front end canonicalizes it to k < i + c + n, the "
                  "constant-extent family (docs section 19.12)",
}


def _rename(sub: str, old: str, new: str) -> str:
    f = casespec.parse_affine(sub)
    coef = dict(f.coef)
    if old in coef:
        coef[new] = coef.get(new, 0) + coef.pop(old)
    return casespec.LinearForm(coef, f.const).c_text()


def _substitute(sub: str, var: str, value: int) -> str:
    f = casespec.parse_affine(sub)
    coef = dict(f.coef)
    const = f.const + coef.pop(var, 0) * value
    return casespec.LinearForm(coef, const).c_text()


def _apply_shear(case: Case, shears):
    """shears: {target var: {enclosing var: coefficient}} -- the targets' two
    bounds shifted alike (family F), every subscript composed so that the
    nest touches the same elements at corresponding points: f(x) with
    x'_s = x_s - l_s . x_<s, the inverse of casespec.box_form, which gives
    the case back."""
    order = [lp.var for lp in case.loops]
    for target, terms in shears.items():
        lp = next(l for l in case.loops if l.var == target)
        assert all(order.index(src) < order.index(target) for src in terms), (target, terms)
        lp.lb_terms = dict(terms)
        lp.ub_terms = dict(terms)
    for arr in case.arrays.values():
        new = []
        for sub in arr.subs:
            f = casespec.parse_affine(sub)
            coef = dict(f.coef)
            # the simultaneous substitution: every target's ORIGINAL
            # coefficient (f.coef), whatever the order of `shears`
            for target, terms in shears.items():
                c = f.coef.get(target, 0)
                if c:
                    for src, l in terms.items():
                        coef[src] = coef.get(src, 0) - c * l
            new.append(casespec.LinearForm(coef, f.const).c_text())
        arr.subs = tuple(new)


def _shear_targets(case: Case, need_step=False):
    """The loops (not the outermost) that count up and have an enclosing loop
    of two or more iterations to read, with those loops."""
    out = []
    for s, lp in enumerate(case.loops):
        if s == 0 or lp.step < 0 or lp.cond or not lp.is_box():
            continue
        if need_step and lp.step == 1:
            continue
        sources = [l for l in case.loops[:s] if l.trip() >= 2]
        if sources:
            out.append((lp, sources))
    return out


def _share_base(c: Case):
    """B reads A's array: one parameter, two views."""
    A, B = c.arrays["a"], c.arrays["b"]
    if len(A.dims) != len(B.dims):
        raise Rejected("same base needs factors of one rank")
    dims = tuple(max(x, y) for x, y in zip(A.dims, B.dims))
    A.dims = dims
    c.arrays["b"] = Array(A.name, dims, B.subs, restrict=A.restrict, elem=B.elem)


def _same_view(c: Case):
    """B'(K, J) = A[f_I(K)][f_K(J)]: the same element map as A'(I, K) =
    A[f_I(I)][f_K(K)] -- which needs the I, J and K loops to run over the same
    values (M = N = K, one origin, one step: the compiler's sameView compares
    the roles' extents too), so J's and K's loops are made I's."""
    A, B, C = c.arrays["a"], c.arrays["b"], c.arrays["c"]
    fa = [casespec.parse_affine(sub) for sub in A.subs]
    if len(fa) != 2 or any(len(f.coef) != 1 for f in fa):
        raise Rejected("same_view needs A's two subscripts over one counter each")
    i_var, k_var = (list(f.coef)[0] for f in fa)
    i_loops = [lp for lp in c.loops if lp.role == "I"]
    j_loops = [lp for lp in c.loops if lp.role == "J"]
    k_loops = [lp for lp in c.loops if lp.role == "K"]
    if len(i_loops) != 1 or len(j_loops) != 1 or len(k_loops) != 1 or k_loops[0].var != k_var \
            or i_loops[0].var != i_var:
        raise Rejected("same_view needs one loop per role")
    il = i_loops[0]
    for lp in (j_loops[0], k_loops[0]):
        lp.lb, lp.ub, lp.step, lp.cmp, lp.inc = il.lb, il.ub, il.step, il.cmp, il.inc
    subs = (_rename(A.subs[0], i_var, k_var), _rename(A.subs[1], k_var, j_loops[0].var))
    c.arrays["b"] = Array(A.name, A.dims, subs, restrict=A.restrict, elem=B.elem)
    # the arrays sized to the new ranges (the loops of J and K changed)
    for arr in (A, c.arrays["b"], C):
        needed = []
        for sub in arr.subs:
            lo, hi = casespec._form_range(casespec.parse_affine(sub), c.loops)
            if lo < 0:
                raise Rejected("same_view: a negative subscript")
            needed.append(hi + 1)
        arr.dims = tuple(max(x, y) for x, y in zip(arr.dims, needed))
    A.dims = c.arrays["b"].dims = tuple(max(x, y) for x, y in zip(A.dims, c.arrays["b"].dims))


def domain_variant(case: Case, variant: str, rng) -> Case:
    import copy
    c = copy.deepcopy(case)
    c.name = c.name + "_" + variant
    c.category = "domain_" + variant
    c.math_note = DOMAIN_NOTES[variant]
    A, C = c.arrays["a"], c.arrays["c"]
    if variant in ("shift", "shear", "chain", "step_shift", "shift_same_base", "shift_batched"):
        if variant == "shift_batched":
            c = mutate(c, "batched", rng)
            c.name = c.name.replace("_batched", "_shift_batched")
            c.category = "domain_shift_batched"
            c.math_note = DOMAIN_NOTES[variant]
        targets = _shear_targets(c, need_step=(variant == "step_shift"))
        if not targets:
            raise Rejected("%s: no loop%s to shift" % (variant, " of step > 1" if variant == "step_shift" else ""))
        lp, sources = rng.choice(targets)
        if variant in ("shift", "step_shift", "shift_same_base", "shift_batched"):
            shears = {lp.var: {rng.choice(sources).var: rng.choice([1, -1])}}
        elif variant == "shear":
            srcs = rng.sample(sources, min(len(sources), rng.choice([1, 2])))
            shears = {lp.var: {src.var: rng.choice([2, -2, 1, -1]) for src in srcs}}
            if len(srcs) == 1 and abs(shears[lp.var][srcs[0].var]) == 1:
                shears[lp.var][srcs[0].var] = rng.choice([2, -2])
        else:   # chain: a loop inside the shifted one, shifted by it (and maybe by another)
            inner = [l for l in c.loops[c.loops.index(lp) + 1:] if l.step > 0 and not l.cond and l.is_box()]
            if not inner:
                raise Rejected("chain: no loop inside the shifted one")
            second = rng.choice(inner)
            shears = {lp.var: {rng.choice(sources).var: rng.choice([1, -1, 2])},
                      second.var: {lp.var: rng.choice([1, -1])}}
            if rng.random() < 0.5:
                shears[second.var][rng.choice(sources).var] = rng.choice([1, -1, 2])
        _apply_shear(c, shears)
        if variant == "shift_same_base":
            _share_base(c)
    elif variant == "same_base":
        _share_base(c)
    elif variant == "same_view":
        _same_view(c)
    elif variant == "k1_const":
        ks = [lp for lp in c.loops if lp.role == "K"]
        if len(ks) != 1 or ks[0].trip() != 1:
            raise Rejected("k1_const needs one reduction loop of one iteration")
        k = ks[0]
        c.loops = [lp for lp in c.loops if lp is not k]
        for arr in c.arrays.values():
            arr.subs = tuple(_substitute(sub, k.var, k.lb) for sub in arr.subs)
    elif variant == "triangle":
        # a role loop (a repetition loop's varying count would repeat every
        # point alike when it sums to one number, and the empty iterations of
        # a collapsed triangle leave a box: those are GEMMs -- the families R
        # and X of docs section 19.13 --, not the negative this variant is
        # for)
        targets = [(lp, src) for lp, src in _shear_targets(c) if lp.role in "IJK"]
        if not targets:
            raise Rejected("triangle: no role loop to shift")
        lp, sources = rng.choice(targets)
        src = rng.choice(sources)
        # the lower bound rises by the loop's step per unit of the enclosing
        # counter (staying on the loop's lattice); the upper bound stays: the
        # trip count varies
        lp.lb_terms = {src.var: lp.step}
        trips = {len(lp.values({src.var: v})) for v in src.values()}
        if len(trips - {0}) < 2:
            raise Rejected("triangle: the shifted loop's trip count does not vary over the "
                           "enclosing counter's values (a box in disguise)")
        c.math_class = "non_box_domain"
    elif variant == "disguise":
        # an inner loop s counting up, an enclosing loop t of two or more
        # values: s's lower bound gains c (x_t - first_t) with |c| step_t >=
        # s's extent, so only x_t = first_t leaves s non-empty
        pairs = []
        for si, lp in enumerate(c.loops):
            if lp.step <= 0 or lp.cond or not lp.is_box() or lp.trip() < 1:
                continue
            for t in c.loops[:si]:
                if t.trip() >= 2 and not t.cond and not _gap_if_fixed(c, t):
                    pairs.append((lp, t))
        if not pairs:
            raise Rejected("disguise: no loop pair")
        lp, t = rng.choice(pairs)
        width = lp.exclusive_ub() - lp.lb
        first = t.values()[0]
        mag = (width + abs(t.step) - 1) // abs(t.step) + rng.choice([0, 1])
        coef = mag if t.step > 0 else -mag
        lp.lb_terms = {t.var: coef}
        lp.lb = lp.lb - coef * first
        lp.ub_terms = {}
        c.math_class = ""
    elif variant == "rep_triangle":
        if any(l.var in ("r0t", "r1t") for l in c.loops):
            raise Rejected("rep_triangle: names taken")
        pos = rng.randrange(len(c.loops) + 1)
        r1 = Loop(var="r1t", lb=0, ub=3, step=1, role="R")
        r0 = Loop(var="r0t", lb=0, ub=2, step=1, role="R", lb_terms={"r1t": 1})
        c.loops[pos:pos] = [r1, r0]
    elif variant == "shift_cond":
        targets = _shear_targets(c)
        if not targets:
            raise Rejected("shift_cond: no loop to shift")
        lp, sources = rng.choice(targets)
        src = rng.choice(sources)
        _apply_shear(c, {lp.var: {src.var: 1}})
        # the condition written over the enclosing counter: k - src OP ub
        bound = lp.ub
        lp.cond = "%s - %s %s %d" % (lp.var, src.var, lp.cmp, bound)
    elif variant in ("alias_c", "alias_c_b"):
        F = A if variant == "alias_c" else c.arrays["b"]
        if len(F.dims) != len(C.dims):
            raise Rejected("%s needs the factor and C of one rank" % variant)
        # no repetition loop: the feedback through the aliased array grows
        # with every repetition and leaves float's exact integers
        c.loops = [lp for lp in c.loops if lp.role != "R"]
        dims = tuple(max(x, y) for x, y in zip(F.dims, C.dims))
        F.dims = dims
        c.arrays["c"] = Array(F.name, dims, C.subs, restrict=F.restrict, elem=C.elem)
    else:
        raise ValueError(variant)
    return c


def _gap_if_fixed(case: Case, t: Loop) -> bool:
    """Whether fixing loop t to one value (what a box in disguise does: its
    other values leave an inner loop empty) leaves t's logical coordinate with
    a gap: t is a middle loop of a split coordinate, a finer and a coarser
    loop of its role around it in scale (16 * k0 + 8 * k1 + k2 with k1 fixed
    reads K at 0..7, 16..23, ...). Such a GEMM is not enumerated densely and
    is refused by the dense partition -- a limitation of its own, not the box
    in disguise this variant is for."""
    if t.role not in ("I", "J", "K"):
        return False
    for arr in case.arrays.values():
        for f in arr.forms():
            if t.var not in f.coef:
                continue
            scales = sorted(abs(f.coef[lp.var] * lp.step) for lp in case.loops
                            if lp.var in f.coef and lp.role == t.role and lp.trip() >= 2)
            mine = abs(f.coef[t.var] * t.step)
            if scales and scales[0] < mine < scales[-1]:
                return True
    return False


def _same_element_map(case: Case) -> bool:
    """One array as both factors through one view: A's (I, K) and B's (K, J)
    subscripts the same functions of their single counters AND those counters
    running over the same values (the compiler's sameView compares the
    element maps with the roles' extents), judged on the box form."""
    if case.a.name != case.b.name or len(case.a.subs) != 2 or len(case.b.subs) != 2:
        return False
    box = casespec.box_form(case) if casespec.shifts_together(case) else case
    loops = {lp.var: (lp.lb, lp.exclusive_ub(), lp.step) for lp in box.loops}

    def shape(sub):
        f = casespec.parse_affine(sub)
        if len(f.coef) != 1:
            return None
        (var, coef), = f.coef.items()
        return (coef, f.const, loops.get(var))
    return all(shape(x) is not None and shape(x) == shape(y) for x, y in zip(box.a.subs, box.b.subs))


# --------------------------------------------------------------------------
# signatures, registry, coverage
# --------------------------------------------------------------------------


def signature(case: Case) -> str:
    parts = []
    for lp in case.loops:
        parts.append("%s:%s:%d:%d:%d" % (lp.var, lp.role, lp.lb, lp.exclusive_ub(), lp.step)
                     + ("" if lp.is_box() else ":%s:%s" % (sorted(lp.lb_terms.items()),
                                                           sorted(lp.ub_terms.items()))))
    for role in sorted(case.arrays):
        arr = case.arrays[role]
        parts.append("%s=%s%s:%s:%s:%s" % (role, arr.name, list(arr.dims),
                                           [f.c_text() for f in arr.forms()], arr.restrict, arr.elem))
    parts.append(case.stmt)
    parts.append(case.kernel_style)
    parts.append(case.elem)
    parts.append(str(case.alpha))
    return hashlib.sha1("|".join(parts).encode()).hexdigest()[:12]


def load_json(path, default):
    try:
        with open(path) as f:
            return json.load(f)
    except (OSError, ValueError):
        return default


def save_json(path, obj):
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    with open(path, "w") as f:
        json.dump(obj, f, indent=1)
        f.write("\n")


def features(case: Case, cls):
    an = case.analysis
    f = set()
    f.add("family:" + case.category.split("_", 1)[1] if "_" in case.category else case.category)
    f.add("classification:" + cls)
    f.add("stmt:" + case.stmt)
    f.add("style:" + case.kernel_style)
    f.add("loops:%d" % len(case.loops))
    if any(lp.lb for lp in case.loops):
        f.add("origin:nonzero")
    if any(lp.step > 1 for lp in case.loops):
        f.add("step:gt1")
    if any(lp.cmp == "<=" for lp in case.loops):
        f.add("cmp:le")
    if any(lp.role == "R" for lp in case.loops):
        f.add("repetition")
    if any(lp.role == "P" for lp in case.loops):
        f.add("batched")
    for role, arr in case.arrays.items():
        for form in arr.forms():
            if form.const:
                f.add("offset:" + role)
            if any(abs(v) > 1 for v in form.coef.values()):
                f.add("stride:" + role)
            if len(form.coef) > 1:
                f.add("multi_loop_subscript:" + role)
    order = "".join(lp.role for lp in case.loops if lp.role in "IJK")
    f.add("order:" + order)
    if an.get("extents"):
        e = an["extents"]
        f.add("K:%s" % ("lt_kmax" if e["K"] < K_MAX else "eq_kmax" if e["K"] == K_MAX else "gt_kmax"))
        f.add("MN:%s" % ("tiny" if max(e["I"], e["J"]) <= 4 else "small" if max(e["I"], e["J"]) <= 16 else "large"))
    for t in an.get("gemm_tags", []):
        f.add("tag:" + t)
    # the families of 2026-10-18 (docs/AFFINE_GEMM_MAPPING.md section 19.12)
    if an.get("domain"):
        f.add("domain:" + an["domain"])
    if any(not lp.is_box() and lp.var not in {v for arr in case.arrays.values()
                                               for g in arr.forms() for v in g.vars()}
           for lp in case.loops):
        f.add("bounds:repetition_triangle")
    shifted = {lp.var for lp in case.loops if not lp.is_box()}
    for lp in case.loops:
        if lp.is_box():
            continue
        terms = lp.lb_terms or lp.ub_terms
        f.add("bounds:shifted")
        if any(abs(v) > 1 for v in terms.values()) or len(terms) > 1:
            f.add("bounds:sheared")
        if any(src in shifted for src in terms):
            f.add("bounds:chained")
        if lp.step > 1:
            f.add("bounds:shifted_step_gt1")
        if not lp.shifts_together():
            f.add("bounds:not_together")
        if lp.cond:
            f.add("bounds:written_condition")
    if case.a.name == case.b.name:
        f.add("factors:same_base")
        f.add("factors:" + ("same_view" if _same_element_map(case) else "two_views"))
    if case.c.name in (case.a.name, case.b.name):
        f.add("output:aliases_factor")
    if an.get("is_gemm"):
        if not any(lp.role == "K" for lp in case.loops):
            f.add("K1:no_loop")
        elif (an.get("extents") or {}).get("K") == 1:
            f.add("K1:loop")
    return sorted(f)


# --------------------------------------------------------------------------
# the driver
# --------------------------------------------------------------------------


def build_one(mode, family, seed, index, max_extent, max_loops):
    """The case of (mode, seed, index) -- deterministic."""
    tag = "%s:%s:%d" % (mode, seed, index)
    rng = random.Random("%s/%d" % (seed, index)) if mode == "random" else None
    variant = None
    if mode == "random":
        p = Params.from_random(rng)
        M = rng.choice([1, 2, 3, 4, 5, 7, 8, 9, 12, 16, 17, 20, 24, 32])
        N = rng.choice([1, 2, 3, 4, 5, 7, 8, 9, 12, 16, 17, 20, 24, 32])
        K = rng.choice([1, 2, 3, 4, 5, 8, 15, 16, 17, 24, 32, 33, 40, 64])
        fam = family
        if family == "mixed":
            fam = rng.choices(["positive", "views", "mutation"], [60, 15, 25])[0]
        mut = rng.choice(MUTATIONS)
        if family == "domains":
            variant = rng.choices(DOMAIN_VARIANTS, DOMAIN_WEIGHTS)[0]
    else:
        F = Params.space_size()
        p = Params.from_index(index % F)
        M, N, K = shape_from_index(index // F, mode)
        fam = family
        if family == "mixed":
            fam = ["positive", "positive", "positive", "views", "mutation", "mutation"][index % 6]
        mut = MUTATIONS[(index // 7) % len(MUTATIONS)]
        rng = random.Random(tag)
        if family == "domains":
            variant = DOMAIN_VARIANTS[(index // 5) % len(DOMAIN_VARIANTS)]
            # the shapes walked with the index itself (the generic walk stays
            # at 1 x 1 x 1 for the first 921,600 indices, where no loop can be
            # shifted): the all-ones shape skipped, a new shape every 7 indices
            M, N, K = shape_from_index(index // 7 + 1, mode)
    if fam == "domains":
        # the variants of docs section 19.12 over a positive case (a quarter
        # of them with per-operand views); K = 1 for the constant reduction
        # index, M, N, K otherwise as drawn
        if variant == "k1_const":
            K = 1
        base = positive_case("gen_tmp", M, N, K, p, "views" if rng.random() < 0.25 else "positive",
                             rng, max_extent, max_loops, tag)
        case = domain_variant(base, variant, rng)
        case.set = "generated"
        return case, fam, variant, tag
    base = positive_case("gen_tmp", M, N, K, p, "positive" if fam != "views" else "views", rng,
                         max_extent, max_loops, tag)
    if fam == "mutation":
        case = mutate(base, mut, rng)
    else:
        case = base
    case.set = "generated"
    return case, fam, (mut if fam == "mutation" else None), tag


def finalize(case: Case):
    """analyse, predict, spec, classify, evaluate -- raise Rejected on an invalid case."""
    try:
        casespec.analyse(case)
    except ValueError as e:
        raise Rejected("analysis: %s" % e)
    casespec.predict(case)
    casespec.spec_expect(case)
    cls = casespec.classify_case(case)
    try:
        result = casespec.evaluate(case)
    except IndexError as e:
        raise Rejected("out of bounds: %s" % e)
    # The program checks itself in float: every value must be an integer float
    # represents exactly (|v| < 2^24). Products of the data in [-3, 3] over a
    # GEMM stay far below that; an output that is also a factor (the domains
    # family's alias_c) can feed back and grow without bound.
    peak = max(result.get("peak", 0), max((abs(v) for v in result["c"]), default=0))
    if peak >= 2 ** 24:
        raise Rejected("a product, partial sum or result of magnitude %d exceeds float's exact "
                       "integers" % peak)
    if not casespec.is_box_case(case) and casespec.shifts_together(case):
        # the box form (what the compiler analyses) computes the same C
        if casespec.evaluate(casespec.box_form(case))["c"] != result["c"]:
            raise RuntimeError("%s: the box form computes a different C than the nest as written"
                               % case.id)
    elif not casespec.is_box_case(case):
        # family X: the box of the parallelotope its points form (what the
        # compiler analyses after its exact normalization) computes the same
        # C -- exact_form must describe exactly the nest's points (review
        # 2026-10-18 #24: a loop counting down keeps only the values it takes
        # on the set). (Not for family R: its analysis case is the
        # two-iteration surrogate, another set by design.)
        form = casespec.exact_form(case)
        if form is not None and casespec.evaluate(casespec.box_form(form))["c"] != result["c"]:
            raise RuntimeError("%s: the box of its exact parallelotope computes a different C than the "
                               "nest as written" % case.id)
    if cls in ("FALSE_POSITIVE", "UNEXPECTED_ACCEPT"):
        # the prediction itself says the implementation would offload a nest
        # the specification refuses: keep it (the run decides), but flag it
        case.notes = (case.notes + "\nPREDICTED FALSE POSITIVE: the current implementation is expected "
                      "to offload a nest the specification does not accept.").strip()
    if cls == "FALSE_NEGATIVE":
        reason = case.expect.get("reject_reason", "")
        if "same affine expression" in reason:
            case.issue = "ISSUE-001"
        # (the skewed or merged views of ISSUE-008 are lowered since
        # 2026-10-18, docs/AFFINE_GEMM_MAPPING.md section 19.11: no predicted
        # false negative carries that tag any more)
    return cls


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--mode", default="random", choices=["random", "enumerate", "boundary"])
    ap.add_argument("--seed", default="1")
    ap.add_argument("--count", type=int, default=20)
    ap.add_argument("--start", type=int, default=0, help="first index of the walk")
    ap.add_argument("--family", default="mixed",
                    choices=["positive", "views", "mutation", "mixed", "domains"])
    ap.add_argument("--out", default=None, help="output directory (default generated/batch_<mode>_<seed>_<start>)")
    ap.add_argument("--registry", default=os.path.join(HERE, "..", "generated", "registry.json"))
    ap.add_argument("--coverage", default=os.path.join(HERE, "..", "generated", "coverage.json"))
    ap.add_argument("--max-extent", type=int, default=64)
    ap.add_argument("--max-loops", type=int, default=8)
    ap.add_argument("--max-tries", type=int, default=0, help="stop after this many indices (0: until --count found)")
    ap.add_argument("--reproduce", help="MODE:SEED:INDEX -- regenerate exactly that case and print it "
                                         "(give the same --family the batch was generated with)")
    ap.add_argument("--no-registry", action="store_true", help="do not consult or update the registry")
    a = ap.parse_args()

    if a.reproduce:
        mode, seed, index = a.reproduce.split(":")
        case, fam, mut, tag = build_one(mode, a.family, seed, int(index), a.max_extent, a.max_loops)
        case.id = "gen_" + signature(case)
        cls = finalize(case)
        sys.stdout.write(casespec.c_file_text(case))   # byte for byte the file's content
        print("// family %s mutation %s classification %s signature %s" % (fam, mut, cls, case.id), file=sys.stderr)
        return 0

    out = a.out or os.path.join(HERE, "..", "generated", "batch_%s_%s_%d" % (a.mode, a.seed, a.start))
    os.makedirs(out, exist_ok=True)
    registry = {} if a.no_registry else load_json(a.registry, {})
    coverage = load_json(a.coverage, {"features": {}, "cases": 0, "rejected": 0})
    manifest = {"kind": "generated_batch", "mode": a.mode, "seed": a.seed, "start": a.start,
                "family": a.family, "generator": "generator/gen_cases.py", "cases": []}
    rejected = []
    found = 0
    index = a.start
    tries = 0
    t0 = time.time()
    while found < a.count:
        if a.max_tries and tries >= a.max_tries:
            break
        tries += 1
        tag = "%s:%s:%d" % (a.mode, a.seed, index)
        try:
            case, fam, mut, _ = build_one(a.mode, a.family, a.seed, index, a.max_extent, a.max_loops)
            sig = signature(case)
            if sig in registry:
                raise Rejected("duplicate of %s (first generated as %s)" % (sig, registry[sig]))
            case.id = "gen_" + sig
            cls = finalize(case)
            text = casespec.c_file_text(case)
        except Rejected as e:
            rejected.append({"index": index, "tag": tag, "reason": str(e)})
            index += 1
            continue
        path = os.path.join(out, case.id + ".c")
        # written once, then read-only (0o555): see casespec.write_test_source
        casespec.write_test_source(path, text)
        entry = casespec.manifest_entry(case, os.path.basename(out))
        entry["file"] = case.id + ".c"
        entry["generated_from"] = {"mode": a.mode, "seed": a.seed, "index": index, "family": fam,
                                   "mutation": mut,
                                   "reproduce": "python3 generator/gen_cases.py --family %s --reproduce %s" % (a.family, tag)}
        entry["features"] = features(case, cls)
        manifest["cases"].append(entry)
        registry[sig] = tag
        coverage["cases"] += 1
        for feat in entry["features"]:
            coverage["features"][feat] = coverage["features"].get(feat, 0) + 1
        found += 1
        index += 1
    manifest["next_index"] = index
    manifest["rejected_count"] = len(rejected)
    manifest["seconds"] = round(time.time() - t0, 2)
    coverage["rejected"] += len(rejected)
    save_json(os.path.join(out, "manifest.json"), manifest)
    save_json(os.path.join(out, "rejected.json"), rejected)
    if not a.no_registry:
        save_json(a.registry, registry)
    save_json(a.coverage, coverage)
    from collections import Counter
    cc = Counter(e["classification"] for e in manifest["cases"])
    print("wrote %d case(s) to %s (next index %d; %d generator rejection(s)); classifications: %s"
          % (found, out, index, len(rejected), dict(cc)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
