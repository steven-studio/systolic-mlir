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
references, Python-oracle checksum).  Nothing limits the total: the sequence
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
             "batched", "scalar_acc"]

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
                arrays={"a": A, "b": B, "c": C}, stmt=stmt, seed=(hash(seed_tag) & 0xff) + 1)
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
        c.math_note = "The product's value for (i, j) lands at a skewed row: not a rectangular view of C."
    elif mutation == "coupled_A_and_C_row":
        j = first("J")
        A.subs = ("%s + %s" % (A.subs[0], j), A.subs[1])
        A.dims = (A.dims[0] + max(lp.ub for lp in c.loops if lp.var == j), A.dims[1])
        C.subs = ("%s + %s" % (C.subs[0], j), C.subs[1])
        C.dims = (C.dims[0] + max(lp.ub for lp in c.loops if lp.var == j), C.dims[1])
        c.math_class = "gemm_nonrectangular_region"
        c.math_note = "I = i + j couples two loops: GEMM elements over a parallelogram, not a rectangle."
    elif mutation == "singular":
        k = first("K")
        A.subs = ("%s + %s" % (A.subs[0], k), A.subs[1])
        A.dims = (A.dims[0] + max(lp.ub for lp in c.loops if lp.var == k), A.dims[1])
        c.math_class = "singular_mapping"
        c.math_note = "A's row index couples the reduction loop: the mapping is singular, not a GEMM."
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
        c.math_note = "The output is indexed by the reduction index; the column loop is summed away."
    elif mutation == "no_accumulation":
        c.stmt = "=AB"
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
    else:
        raise ValueError(mutation)
    c.arrays = {k: v for k, v in c.arrays.items()}
    return c


# --------------------------------------------------------------------------
# signatures, registry, coverage
# --------------------------------------------------------------------------


def signature(case: Case) -> str:
    parts = []
    for lp in case.loops:
        parts.append("%s:%s:%d:%d:%d" % (lp.var, lp.role, lp.lb, lp.exclusive_ub(), lp.step))
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
    return sorted(f)


# --------------------------------------------------------------------------
# the driver
# --------------------------------------------------------------------------


def build_one(mode, family, seed, index, max_extent, max_loops):
    """The case of (mode, seed, index) -- deterministic."""
    tag = "%s:%s:%d" % (mode, seed, index)
    rng = random.Random("%s/%d" % (seed, index)) if mode == "random" else None
    if mode == "random":
        p = Params.from_random(rng)
        M = rng.choice([1, 2, 3, 4, 5, 7, 8, 9, 12, 16, 17, 20, 24, 32])
        N = rng.choice([1, 2, 3, 4, 5, 7, 8, 9, 12, 16, 17, 20, 24, 32])
        K = rng.choice([1, 2, 3, 4, 5, 8, 15, 16, 17, 24, 32, 33, 40, 64])
        fam = family
        if family == "mixed":
            fam = rng.choices(["positive", "views", "mutation"], [60, 15, 25])[0]
        mut = rng.choice(MUTATIONS)
    else:
        F = Params.space_size()
        p = Params.from_index(index % F)
        M, N, K = shape_from_index(index // F, mode)
        fam = family
        if family == "mixed":
            fam = ["positive", "positive", "positive", "views", "mutation", "mutation"][index % 6]
        mut = MUTATIONS[(index // 7) % len(MUTATIONS)]
        rng = random.Random(tag)
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
        casespec.evaluate(case)
    except IndexError as e:
        raise Rejected("out of bounds: %s" % e)
    if cls in ("FALSE_POSITIVE", "UNEXPECTED_ACCEPT"):
        # the prediction itself says the implementation would offload a nest
        # the specification refuses: keep it (the run decides), but flag it
        case.notes = (case.notes + "\nPREDICTED FALSE POSITIVE: the current implementation is expected "
                      "to offload a nest the specification does not accept.").strip()
    if cls == "FALSE_NEGATIVE":
        reason = case.expect.get("reject_reason", "")
        if "same affine expression" in reason:
            case.issue = "ISSUE-001"
    return cls


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--mode", default="random", choices=["random", "enumerate", "boundary"])
    ap.add_argument("--seed", default="1")
    ap.add_argument("--count", type=int, default=20)
    ap.add_argument("--start", type=int, default=0, help="first index of the walk")
    ap.add_argument("--family", default="mixed", choices=["positive", "views", "mutation", "mixed"])
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
        print(casespec.c_file_text(case))
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
        with open(path, "w") as f:
            f.write(text)
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
