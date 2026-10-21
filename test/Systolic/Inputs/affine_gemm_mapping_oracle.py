#!/usr/bin/env python3
"""Brute-force oracle for the affine GEMM mapping (docs/AFFINE_GEMM_MAPPING.md,
proof obligation PO5: soundness and completeness of recognition, dense
partition and lowering within the supported class of section 10).

Generates random affine loop nests

    for x_1 .. x_n (constant bounds, steps >= 1, n = 1..5):
      C[c(x)] = C[..] + A[a(x)] * B[b(x)]      (factors in either order)

whose operands have any rank (1..3), store their two GEMM roles in either
order (transposes), read a coordinate forwards or backwards (negative-stride
views), have constant (extent-one) dimensions anywhere, and may lack a role
(M, N or K = 1). Mostly valid GEMMs built from random mixed-radix mappings
(steps, origins, reversed loops, split coordinates, loops that run once adding
arbitrary terms), then randomly perturbed.

Each nest is DECIDED by enumerating its iteration points -- not with the
structure theorem the compiler uses:

  * the C load and the C store address the same element at every point;
  * every non-constant subscript's values form an arithmetic progression; its
    rank (value - min) / step is an ordinal; two subscripts address the same
    coordinate iff their ranks agree or are mirror images (E - 1 - rank);
  * a coordinate is read by exactly two operands (A and C: I; B and C: J; A
    and B: K), each in one dimension; at most one coordinate per role; a role
    no coordinate plays has extent 1;
  * (I, J, K) is a bijection of the points onto [0, M) x [0, N) x [0, K);
  * every subscript stays inside its dimension (the program is defined).

and the COMPILER'S OUTPUT is CHECKED, not compared with a predicted form:
from the lowered IR the final view of each operand (the memref type that
reaches bufferization.to_tensor: base buffer, offset and signed strides, after
subview / transpose / expand_shape) gives the address of each view element.
For every iteration point the access's address must be exactly one view
element of each operand, the three elements must agree on (I, K), (K, J),
(I, J), and those triples must be a bijection onto the box; the extents must
equal the oracle's. That is conditions (V) and (B) of section 3 established
directly on the emitted views, whatever ordinal direction or factor assignment
the compiler chose.

Batched nests (ISSUE-006, ISSUE-007, docs/AFFINE_GEMM_MAPPING.md section 19):
the batch loops are, by definition, the loops of two or more iterations that
all three operands read; the other loops' roles are their readers (A and C:
I, B and C: J, A and B: K, one operand: a broadcast, refused). Every operand
may read the batch loops anywhere, C included (no C purity since 2026-10-16),
and a dimension may combine both of its operand's roles (a skewed or merged
view). Per batch iteration, by enumeration: every subscript's restriction to
a role's loops is proportional to one direction, that coordinate's values are
an arithmetic progression taken once per point of the role's loops, (I, J, K)
is a bijection onto the box, and C's element is injective in (I, J) (else the
nest is valid only with more batch loops, which the compiler does not
construct: not lowered); every batch iteration the same shape. For a lowered batched nest
the emitted batch loops are enumerated, the affine.apply offsets of each
operand's subview evaluated at every batch value, and the views checked per
batch iteration on that iteration's points -- the same address-exact check.

Some loops are emitted with redundant constant bounds, max(lb, lb - d) to
min(ub, ub + d) with the effective bound in any position (P0.6): the compiler
must fold them -- a bound map's first result alone runs other iterations. The
decoration has its own random stream per nest, so a seed's nests are the
same with or without it.

Sheared iteration domains (docs/AFFINE_GEMM_MAPPING.md section 19.12, since
2026-10-18): some nests are emitted on a constant-extent parallelotope instead
of their box -- loop s runs from a_s + l_s . x_<s to b_s + l_s . x_<s (the
same integer form l_s shifts both bounds; max/min decorations keep that linear
part), the subscripts rewritten so that the emitted nest reads and writes
exactly the elements of the box nest at corresponding points, in the same
order (x' = L x with L lower unitriangular; an access c . x' is c . L . x).
The DECISION stays the box nest's (the roles are taken after the bijection,
section 19.2 / 19.12), and the compiler's views are checked on the box
points: a lowering of the sheared nest is right iff it is the box nest's. A
'triangle' (the two bounds shifted by different forms) is decided by
enumerating its points as emitted (section 19.13): when they are a
constant-extent parallelotope whose shift rows are those of the bounds
(find_parallelotope -- a degenerate triangle), the nest is the GEMM of that
parallelotope's box (derived_box_nest), which the compiler's exact path must
lower and whose views are checked on its points; any other triangle is
outside the family the compiler normalizes: it must not be lowered. Sheared
nests are emitted in every mode, so an out-of-bounds access or a
non-injective C under a shear is a rejection / refusal like any other.

Boxes in disguise (section 19.13): some box or sheared nests are emitted with
the same points written as a non-parallelotope -- an extra max / min bound
result with another linear part that never wins on the domain, or an
unsheared outer loop whose range is extended by values for which an extra
upper result of an inner loop empties it (disguise; check_shear asserts the
points and accesses unchanged, and only disguises inside the documented
family are kept). The compiler's exact path must find the parallelotope
again and lower it; the nest is decided, and the lowering checked, on the box
of the parallelotope its points form (find_parallelotope, the shift rows tried
in the documented order -- zero, then the lower and the upper results' rows --,
and derived_box_nest): the original box, or, when a shear's source loop runs
once and the zero row already fits, the same points in the written
coordinates.

One memref as both factors (section 7, 19.12.3): some nests read A's memref
for B too -- through two different views (A[i][k] * A[k][j], transposed,
skewed, strided or reversed variants: two read-only tensors of one buffer),
or through ONE view (A'(I, K) and B'(K, J) the same element map, which needs
M = N = K: one tensor serves both). Some nests make C a view of A's memref:
the compiler must refuse them ("A may overlap C").

K = 1 (a reduction of extent one: a loop that runs once read by both
factors, or no reduction loop at all) is counted explicitly: such a GEMM
must stay eligible and lowered.

The pass runs with classify=true: every refusal remark ends in its class (docs
section 9; the hoist's two refusals, UNKNOWN, since 2026-10-19), and the IR is
the same. A nest's remark is the first one located in its lines. A refused
triangle's class is checked against its enumerated points (section 19.13.4):
PROVEN_INVALID -- the claim that no decomposition makes the nest a GEMM --
only on a point set that is not centrally symmetric, no parallelotope of the
family, whose stacked access map x -> (F_A(x), F_B(x), F_C(x)) has full
column rank (the accesses tell the iterations apart; with the C store's
subscripts or the accumulator load's, which agree on the points) and whose
two factors and C are three different memrefs, and with the witness its
remark states -- a point of the set whose mirror about the midpoint of the
lexicographic extremes is outside it; conversely, a recognized triangle
meeting those conditions with the load's map, the compiler's (and with no
stepped loop whose lower bound reads a counter), must be PROVEN_INVALID. The
classes are counted in the summary.

Reduction-batch partitioning (docs section 23; on by default since
2026-10-21, --no-reduction-batches runs the pass with reduction-batches=false
and decides without it): every generated nest accumulates, C = C + A * B (an
assignment never gets reduction batch loops, section 20.4: none is generated
here). On the box the nest is decided on, the loops of two or more iterations
C does not read are, by their readers, read by one factor only or by both.
The decompositions tried, in order (reduction_batch_choices): the one-factor
loops as reduction batch (RB) loops; then, with two or more loops read by
both factors, each of them as K -- the longest first, ties the innermost --
and the others RB too. The first one that is a GEMM by enumeration (the
batch iterations, RB values included, each a GEMM of its points with C
injective per iteration; decide_with) is the verdict; with none, the
recognition of sections 19-21 alone decides. Repeated products are not a
reason to refuse: every iteration point stays one update. A lowered RB nest
is checked like every batched one -- the emitted batch loops run the
decision's batch loops (RB loops included), so every iteration runs exactly
once, and every batch value's views hold its points' accesses address by
address (each RB value's factor views at their offsets, C's view the slab all
its RB values share) -- and the IR must read C, run the tile and write C back
inside every batch loop (check_serial_accumulation): the RB iterations
accumulate into one slab in order, from C's initial value.

Ordered output batches (docs section 25, since 2026-10-21; independent of
--no-reduction-batches): an accumulation decided on its counters as written
(the nests reduction batches are made on: no normalization with U != I) whose
C is not injective on a batch iteration's (I, J) box keeps every
contribution when an output role becomes batch loops -- the role of the
product's first factor first, then the other's -- provided those loops and
the other batch loops are an OUTERMOST PREFIX of the nest's loops of two or
more iterations (ordered_prefix): the batch iterations then run in the order
written, each reading C as the previous one left it. The decomposition is
decided again by enumeration (each tile a GEMM, C injective on it). On such
a prefix a demoted loop may also share a dimension of C with a row or column
coordinate (the partitions' outputs overlap or interleave), except where it
moves the role's coordinate in the role's factor too through a different
linear form (SQ-1, not lowered). Assignments are not generated; sheared and
disguised nests with U != I keep the refusal. Every lowered nest -- batched
or not, ordered or not -- is then checked contribution by contribution
(check_ordered_contributions): from the compiler's views, per emitted batch
iteration in the emitted order, no tile writes an element of C twice, every
element of C receives exactly the program's products (as {A element, B
element} pairs), and the products of different output batch values arrive in
the program's order (only one tile's products, and one slab's reduction
batch iterations, are reassociated).

With --fp-default the nests go through the pass WITHOUT the reassociation
permission, and nothing may be lowered (section 6), with reduction batch loops
or ordered output batches or without.

Prints every mismatch and a summary line.
"""

import argparse
import itertools
import random
import re
import subprocess
import sys
from fractions import Fraction
from math import gcd


# Whether the compiler lowers skewed and merged views (a dimension combining two of an
# operand's roles, or a role in several dimensions: docs/AFFINE_GEMM_MAPPING.md section
# 19). Recognized since the generalized mapping (Phase C, 2026-10-16) and lowered since
# Phase D (2026-10-18, section 19.11: an explicit memref.reinterpret_cast view, checked
# here address by address like every other view). False makes every such GEMM a required
# refusal (the state between the two phases).
GENERAL_VIEWS_LOWERED = True


def trips(lb, ub, step):
    return 0 if ub <= lb else (ub - lb - 1) // step + 1


def value(f, x):
    coefs, const = f
    return sum(c * v for c, v in zip(coefs, x)) + const


class Nest:
    """The BOX nest: constant loops, the subscripts over its counters x'. It
    is what the oracle decides and checks. How it is EMITTED may differ:
    sheared loops (the counters x of the emitted nest, x_s = x'_s + l_s . x_<s,
    `shear[s]` = l_s over the outer loops), a 'triangle' (`shear_ub` differs
    from `shear`: not the same computation, not in the compiler's family),
    operands sharing one memref (`arg_of`: the emitted argument of each)."""

    def __init__(self, loops, ops, cload, order, mode):
        self.loops = loops          # [(lb, ub, step)]
        self.ops = ops              # {"A"|"B"|"C": (shape, [f, ...])}
        self.cload = cload          # [f, ...] the accumulator load's subscripts
        self.order = order          # ("A", "B") or ("B", "A"): the product's factor order
        self.mode = mode
        self.bounds = None          # per loop ([lowers], [uppers]) as emitted, or None: lb, ub
        self.shear = None           # per loop s: [l_s0, .., l_s(s-1)] over the outer loops, or None
        self.shear_ub = None        # the upper bounds' rows when they differ (a triangle), else None
        self.domain_mode = "box"    # "box" | "shear" | "triangle"
        self.arg_of = {"A": 0, "B": 1, "C": 2}   # the emitted memref argument of each operand
        self.shared = None          # None | "two_views" | "same_view" | "alias_c"
        # A box (or sheared box) in disguise (docs section 19.13): per loop,
        # extra lower / upper bound results with other linear parts that are
        # inactive on the domain ([(row over the outer emitted counters,
        # constant)]), and an extension of a loop's upper bound whose new
        # values leave an inner loop empty -- the same points, written as a
        # non-parallelotope.
        self.extra_lower = None     # per loop [(row, constant)], or None
        self.extra_upper = None
        self.ub_extend = None       # per loop: added to every upper constant, or None
        self.disguised = False

    def points(self):
        return list(itertools.product(*[[lb + st * z for z in range(trips(lb, ub, st))]
                                        for lb, ub, st in self.loops]))

    def sheared_loops(self):
        return [s for s, row in enumerate(self.shear or []) if any(row)]

    def chained(self):
        """A sheared loop whose shift reads another sheared loop."""
        sheared = set(self.sheared_loops())
        return any(t in sheared for s in sheared for t, c in enumerate(self.shear[s]) if c)


def shear_matrix(nest):
    """L with x' = L x: the box counters from the emitted ones (docs section
    19.12.1: x_s = x'_s + l_s . x_<s, so x'_s = x_s - l_s . x_<s). Lower
    unitriangular, unimodular."""
    n = len(nest.loops)
    L = [[1 if s == t else 0 for t in range(n)] for s in range(n)]
    for s, row in enumerate(nest.shear or []):
        for t, c in enumerate(row):
            L[s][t] -= c
    return L


def emitted_row(f, L):
    """A subscript c . x' + k over the emitted counters: (c . L) . x + k."""
    coefs, const = f
    n = len(coefs)
    return ([sum(coefs[s] * L[s][t] for s in range(n)) for t in range(n)], const)


def emitted_results(nest, s):
    """Loop s's bound results as emitted: ([(row, constant)] lower, [(row,
    constant)] upper), rows over the outer emitted counters (the max of the
    lower results and the min of the upper ones is the bound)."""
    lb, ub, _ = nest.loops[s]
    lowers, uppers = nest.bounds[s] if nest.bounds else ([lb], [ub])
    lo_row = list(nest.shear[s]) if nest.shear else [0] * s
    up_row = list(nest.shear_ub[s]) if nest.shear_ub else lo_row
    extend = nest.ub_extend[s] if nest.ub_extend else 0
    lo = [(lo_row, c) for c in lowers] + (list(nest.extra_lower[s]) if nest.extra_lower else [])
    up = [(up_row, c + extend) for c in uppers] + (list(nest.extra_upper[s]) if nest.extra_upper else [])
    return lo, up


def emitted_points(nest):
    """The emitted nest's iteration points (bounds depending on the outer
    counters), in iteration order."""
    n = len(nest.loops)
    out = []

    def rec(s, x):
        if s == n:
            out.append(tuple(x))
            return
        st = nest.loops[s][2]
        lo_res, up_res = emitted_results(nest, s)
        lo = max(c + sum(a * x[t] for t, a in enumerate(row)) for row, c in lo_res)
        up = min(c + sum(a * x[t] for t, a in enumerate(row)) for row, c in up_res)
        for z in range(trips(lo, up, st)):
            rec(s + 1, x + [lo + st * z])
    rec(0, [])
    return out


def emitted_prefixes(nest, depth):
    """The points of the emitted nest truncated to its first `depth` loops:
    the outer points at which loop `depth` is entered."""
    out = []

    def rec(s, x):
        if s == depth:
            out.append(tuple(x))
            return
        st = nest.loops[s][2]
        lo_res, up_res = emitted_results(nest, s)
        lo = max(c + sum(a * x[t] for t, a in enumerate(row)) for row, c in lo_res)
        up = min(c + sum(a * x[t] for t, a in enumerate(row)) for row, c in up_res)
        for z in range(trips(lo, up, st)):
            rec(s + 1, x + [lo + st * z])
    rec(0, [])
    return out


def find_parallelotope(nest, pts):
    """The documented exact family (docs section 19.13), decided by
    enumeration, independently of the compiler's Presburger search: the
    emitted point set `pts` is a constant-extent parallelotope whose shift row
    for each loop is zero or the linear part of one of the loop's emitted
    bound results -- per loop s, outermost first, the projection of `pts` onto
    x_0..x_s equals {x_<s in the previous projection, a <= x_s - l . x_<s <=
    e, x_s - l . x_<s = a (mod step)} with a, e the extremes on `pts`. A
    stepped loop whose lower results have different rows is in the family
    only when one of them is the max at every outer point entering the loop
    (its start, and so its congruence, is then that result's). [(l, a, e,
    step)] per loop, or None."""
    if not pts:
        return None
    n = len(nest.loops)
    prev = {()}
    found = []
    for s in range(n):
        step = nest.loops[s][2]
        lo_res, up_res = emitted_results(nest, s)
        if step > 1 and len({tuple(row) for row, _ in lo_res}) > 1:
            # one result must be the max at every outer point entering loop s
            entered = emitted_prefixes(nest, s)
            def at(res, x):
                row, c = res
                return c + sum(a * v for a, v in zip(row, x))
            if not any(all(at(r, x) >= at(j, x) for x in entered for j in lo_res) for r in lo_res):
                return None
        cands = [[0] * s]
        for row, _ in lo_res + up_res:
            r = (list(row) + [0] * s)[:s]
            if r not in cands:
                cands.append(r)
        proj = {x[:s + 1] for x in pts}
        hit = None
        for l in cands:
            vals = [x[s] - sum(c * v for c, v in zip(l, x[:s])) for x in proj]
            a, e = min(vals), max(vals)
            cand = set()
            for p in prev:
                base = sum(c * v for c, v in zip(l, p))
                for t in range((e - a) // step + 1):
                    cand.add(p + (a + base + step * t,))
            if cand == proj:
                hit = (l, a, e, step)
                break
        if hit is None:
            return None
        found.append(hit)
        prev = proj
    return found


def derived_box_nest(nest, rows):
    """The box nest of an emitted parallelotope (find_parallelotope): loops
    [a_s, e_s] with the step, every subscript of the emitted nest composed
    with x = U x' (x_s = x'_s + l_s . x_<s). The same computation on the same
    elements, point for point."""
    n = len(nest.loops)
    L = shear_matrix(nest)
    # U as rows over x': x_s = x'_s + sum_t l_s[t] x_t, x_t already over x'.
    U = []
    for s, (l, _, _, _) in enumerate(rows):
        row = [1 if u == s else 0 for u in range(n)]
        for t, c in enumerate(l):
            if c:
                row = [a + c * b for a, b in zip(row, U[t])]
        U.append(row)

    def compose(f):
        g = emitted_row(f, L)            # over the emitted counters x
        coefs, const = g
        return [[sum(coefs[s] * U[s][u] for s in range(n)) for u in range(n)], const]
    ops = {k: (shape, [compose(f) for f in fs]) for k, (shape, fs) in nest.ops.items()}
    box = Nest([(a, e + 1, st) for _, a, e, st in rows], ops, [compose(f) for f in nest.cload],
               nest.order, nest.mode + "/as its box")
    box.arg_of, box.shared = nest.arg_of, nest.shared
    return box


def disguise(rng, nest):
    """Write a box or sheared nest as a non-parallelotope with the same points
    (docs section 19.13): an inactive extra lower or upper bound result with
    another linear part (on the domain it never wins the max / min), or an
    unsheared outer loop's range extended by values for which an inner loop's
    extra upper result empties it. The compiler's exact path must find the
    parallelotope again; check_shear asserts the points are unchanged."""
    n = len(nest.loops)
    if n < 2 or nest.domain_mode not in ("box", "shear"):
        return
    pts = emitted_points(nest)
    if not pts:
        return
    nest.extra_lower = [[] for _ in range(n)]
    nest.extra_upper = [[] for _ in range(n)]
    nest.ub_extend = [0] * n
    s = rng.randrange(1, n)
    t = rng.randrange(s)
    lo_res, up_res = emitted_results(nest, s)
    hi_t = max(x[t] for x in pts)
    lo_t = min(x[t] for x in pts)
    kind = rng.choice(["lower", "upper", "extend"])
    sheared_t = nest.shear and any(nest.shear[t])
    if kind == "extend" and (sheared_t or nest.shear_ub):
        kind = "upper"
    c = rng.choice([1, 2])
    if kind == "lower":
        # lb_s(x) + c (x_t - hi_t) <= lb_s(x) on the domain
        row, const = lo_res[0]
        row = list(row) + [0] * (s - len(row))
        row[t] += c
        best = max(cc for _, cc in lo_res)
        nest.extra_lower[s].append((row, best - c * hi_t))
    elif kind == "upper":
        # ub_s(x) + c (x_t - lo_t) >= ub_s(x) on the domain
        row, const = up_res[0]
        row = list(row) + [0] * (s - len(row))
        row[t] += c
        least = min(cc for _, cc in up_res)
        nest.extra_upper[s].append((row, least - c * lo_t))
    else:
        # loop t runs k more values; for them loop s is empty: ub_s(x) - W (x_t
        # - hi_t) with W step_t >= the widest range of loop s
        st_t = nest.loops[t][2]
        k = rng.choice([1, 2])
        nest.ub_extend[t] = k * st_t
        row, _ = up_res[0]
        row = list(row) + [0] * (s - len(row))
        lo_c = max(cc for _, cc in lo_res)
        up_c = min(cc for _, cc in up_res)
        W = (max(up_c - lo_c, 0) + st_t) // st_t + 1
        row[t] -= W
        nest.extra_upper[s].append((row, up_c + W * hi_t))
    nest.disguised = True
    check_shear(nest)
    # Only disguises inside the documented family (find_parallelotope, by
    # enumeration): an extra result that wins the max / min at an outer point
    # with no full point below it (a stepped loop's start then depends on
    # it) is reverted.
    if find_parallelotope(nest, emitted_points(nest)) is None:
        nest.extra_lower = nest.extra_upper = nest.ub_extend = None
        nest.disguised = False


def check_shear(nest):
    """The emitted sheared nest performs the box nest's computation: the same
    number of points, in the same order, x' = L x at each, and every access
    (A, B, the C load, the C store) at the same element. Raises on failure
    (an oracle bug, never a compiler one)."""
    L = shear_matrix(nest)
    box, emitted = nest.points(), emitted_points(nest)
    assert len(box) == len(emitted), (nest.mode, len(box), len(emitted))
    n = len(nest.loops)
    rows = {name: [emitted_row(f, L) for f in fs] for name, (_, fs) in nest.ops.items()}
    rows["cload"] = [emitted_row(f, L) for f in nest.cload]
    for xb, xe in zip(box, emitted):
        assert tuple(sum(L[s][t] * xe[t] for t in range(n)) for s in range(n)) == xb
        for name, fs in nest.ops.items():
            for f, g in zip(fs[1], rows[name]):
                assert value(f, xb) == value(g, xe), (name, f, g, xb, xe)
        for f, g in zip(nest.cload, rows["cload"]):
            assert value(f, xb) == value(g, xe)


# ---------------------------------------------------------------------------
# Generation.

def lcm(a, b):
    return a * b // gcd(a, b)


def make_loop(rng, n):
    step = rng.choice([1, 1, 1, 2, 3])
    lb = rng.randint(-4, 6)
    return (lb, lb + step * (n - 1) + rng.randint(1, step), step)


ROLES = {"A": ("I", "K"), "B": ("K", "J"), "C": ("I", "J")}


def generate_valid(rng):
    present = [r for r in "IJK" if rng.random() < 0.85]
    if not present:
        present = [rng.choice("IJK")]
    # A batch coordinate (one in six nests): its own loop, read by every
    # operand through one shared subscript (the same list object, so that
    # finalize gives the three the same constant).
    batch = rng.random() < 0.17
    coord = list(present) + (["P"] if batch else [])
    while len(coord) < rng.randint(len(present), 5):
        coord.append(rng.choice(present + [None]))
    rng.shuffle(coord)
    n = len(coord)
    trip = [1 if c is None or rng.random() < 0.12 else rng.randint(2, 4) for c in coord]
    for r in present + (["P"] if batch else []):
        if not any(coord[s] == r and trip[s] >= 2 for s in range(n)):
            trip[next(s for s in range(n) if coord[s] == r)] = rng.randint(2, 4)
    loops = [make_loop(rng, t) for t in trip]
    g = {}
    for r in present:
        group = [s for s in range(n) if coord[s] == r and trip[s] >= 2]
        rng.shuffle(group)
        base = 1
        for s in group:
            base = lcm(base, loops[s][2])
        coefs, mag = [0] * n, 1
        for s in group:
            coefs[s] = rng.choice([1, 1, -1]) * base * mag // loops[s][2]
            mag *= trip[s]
        g[r] = coefs
    once = [s for s in range(n) if trip[s] == 1]

    ops = {}
    batch_sub = None
    if batch:
        s = next(s for s in range(n) if coord[s] == "P")
        coefs = [0] * n
        coefs[s] = rng.choice([1, 1, 2, -1])
        batch_sub = [coefs, 0]
    for name in "ABC":
        dims = []
        if batch_sub is not None:
            dims.append(batch_sub)       # shared: one object for the three
        for r in ROLES[name]:
            if r in g:
                p = rng.choice([1, 1, 1, 2, 3]) * rng.choice([1, 1, -1])
                dims.append([[p * c for c in g[r]], 0])
            elif rng.random() < 0.6:
                dims.append([[0] * n, None])          # a constant dimension for the absent role
        if rng.random() < 0.3:
            dims.insert(rng.randint(0, len(dims)), [[0] * n, None])
        if not dims:
            dims.append([[0] * n, None])
        if rng.random() < 0.35:
            rng.shuffle(dims)
        for f in dims:
            if f is batch_sub:
                continue
            for s in once:
                if rng.random() < 0.4:
                    f[0][s] += rng.randint(-3, 3)
        ops[name] = dims
    for s in once:
        if all(f[0][s] == 0 for fs in ops.values() for f in fs):
            rng.choice([f for fs in ops.values() for f in fs])[0][s] = rng.choice([-2, -1, 1, 2])
    return loops, trip, ops


def finalize(rng, loops, ops, mode, cload=None, oob=False):
    pts = list(itertools.product(*[[lb + st * z for z in range(trips(lb, ub, st))]
                                   for lb, ub, st in loops]))
    done = set()
    for name in "ABC":
        for f in ops[name]:
            if id(f) in done:
                continue
            done.add(id(f))
            if f[1] is None:
                f[1] = rng.randint(0, 3)              # a constant index
                f[1] -= min(value((f[0], 0), x) for x in pts) if any(f[0]) else 0
            else:
                lo = min(value((f[0], 0), x) for x in pts)
                f[1] = -lo + rng.choice([0, 0, 1, 2, 5])
    if cload is not None:
        # A mutated accumulator: C's constants, one coefficient changed.
        for d, f in enumerate(cload):
            f[1] = ops["C"][d][1]
    if cload is None:
        cload = [[list(f[0]), f[1]] for f in ops["C"]]
        for f in cload:
            for s, (lb, ub, st) in enumerate(loops):
                if trips(lb, ub, st) == 1 and rng.random() < 0.3:
                    d = rng.randint(-2, 2)
                    f[0][s] += d
                    f[1] -= d * lb
    shapes = {}
    for name in "ABC":
        fs = ops[name]
        shape = []
        for d, f in enumerate(fs):
            hi = max(value(f, x) for x in pts)
            if name == "C":
                hi = max(hi, max(value(cload[d], x) for x in pts))
            shape.append(max(hi, 0) + 1 + rng.choice([0, 0, 1, 3]))
        shapes[name] = shape
    if oob:
        name = rng.choice("ABC")
        d = rng.randrange(len(ops[name]))
        fs = [ops[name][d]] + ([cload[d]] if name == "C" else [])
        if rng.random() < 0.5:
            shapes[name][d] = max(max(value(f, x) for f in fs for x in pts), 1)
        else:
            shift = min(value(f, x) for f in fs for x in pts) + 1
            for f in fs:
                f[1] -= shift
    order = ("A", "B") if rng.random() < 0.6 else ("B", "A")
    return Nest(loops, {k: (tuple(shapes[k]), ops[k]) for k in "ABC"}, cload, order, mode)


def mutate(rng, loops, trip, ops):
    kind = rng.choice(["radix", "couple", "acc", "broadcast", "double_read",
                       "batch", "batch_shift", "scale_coef", "swap_roles",
                       "add_const_loop", "drop_dim", "batch_offset",
                       "batch_offset_c", "role_in_k", "batch_once", "skew", "merge_c"])
    cload = None
    many = [s for s in range(len(loops)) if trip[s] >= 2]
    every = [f for fs in ops.values() for f in fs]
    varying = [f for f in every if f[1] is not None]
    if kind == "radix" and varying and many:
        f = rng.choice(varying)
        nz = [s for s in many if f[0][s] != 0]
        if nz:
            f[0][rng.choice(nz)] *= rng.choice([2, 3])
    elif kind == "couple" and varying and many:
        rng.choice(varying)[0][rng.choice(many)] += rng.choice([-1, 1, 2])
    elif kind == "acc" and many:
        cload = [[list(f[0]), f[1]] for f in ops["C"]]
        cload[rng.randrange(len(cload))][0][rng.choice(many)] += 1
    elif kind == "broadcast" and varying:
        f = rng.choice(varying)
        f[0] = [c if trip[s] == 1 else 0 for s, c in enumerate(f[0])]
        f[1] = None
    elif kind == "double_read" and varying:
        name = rng.choice("ABC")
        src = rng.choice(varying)
        ops[name].insert(rng.randint(0, len(ops[name])), [list(src[0]), 0])
    elif kind == "batch" and varying:
        # a role's subscript duplicated as a batch subscript of every operand:
        # that role's loops then drive a batch coordinate too (coupled)
        src = rng.choice(varying)
        shared = [list(src[0]), 0]
        for name in "ABC":
            ops[name].insert(rng.randint(0, len(ops[name])), shared)
    elif kind == "batch_shift":
        # a batch subscript read with another constant by one operand: valid,
        # lowered since ISSUE-007 (each operand at its own batch index)
        shared = [f for f in ops["C"] if f in ops["A"] and f in ops["B"] and
                  any(f is g for g in ops["A"]) and any(f is g for g in ops["B"])]
        if shared:
            f = shared[0]
            name = rng.choice("AB")
            ops[name][[id(g) for g in ops[name]].index(id(f))] = [list(f[0]), None]
    elif kind in ("batch_offset", "batch_offset_c"):
        # a batch loop added to a role's dimension: of a factor -- the
        # factor's view moves with the batch (ISSUE-007) -- or of C -- the
        # output view moves with the batch, valid when C stays injective per
        # batch (section 19.3, since 2026-10-16)
        shared = [f for f in ops["C"] if any(f is g for g in ops["A"]) and
                  any(f is g for g in ops["B"])]
        names = "AB" if kind == "batch_offset" else "C"
        roles = [f for nm in names for f in ops[nm]
                 if f[1] is not None and not any(f is g for g in shared) and
                 any(f[0][s] != 0 for s in many)]
        if shared and roles:
            f, b = rng.choice(roles), shared[0]
            c = rng.choice([1, -1, 2])
            f[0] = [x + c * y for x, y in zip(f[0], b[0])]
    elif kind == "role_in_k":
        # a loop of I or J added, in proportion, to the K subscripts of both
        # factors: read by all three operands it is a batch loop, and each
        # factor reads K at an offset that moves with it (A[i][k + j] with
        # B[k + j][j]: valid, one dense GEMM per j); the positional reading
        # K = k + j, J = j has that loop drive two coordinates
        def reads(name, s):
            return any(f[0][s] != 0 for f in ops[name])
        k_loops = [s for s in many if reads("A", s) and reads("B", s) and not reads("C", s)]
        ij_loops = [s for s in many if reads("C", s) and reads("A", s) != reads("B", s)]
        ka = [f for f in ops["A"] if k_loops and f[0][k_loops[0]] != 0]
        kb = [f for f in ops["B"] if k_loops and f[0][k_loops[0]] != 0]
        if ij_loops and len(ka) == 1 and len(kb) == 1 and ka[0] is not kb[0]:
            s, k, c = rng.choice(ij_loops), k_loops[0], rng.choice([1, -1, 2])
            for f in (ka[0], kb[0]):
                f[0][s] += c * f[0][k]
    elif kind == "batch_once":
        # the batch loop runs once: the shared batch subscript is a constant
        # on the domain -- a fixed index of a plain GEMM, no batch
        # coordinate; in half of them the loops of two roles run once as
        # well (M x 1 x 1 and the like: a plain GEMM, not the batched
        # vector-times-scalar SQ-5 leaves open)
        shared = [f for f in ops["C"] if any(f is g for g in ops["A"]) and
                  any(f is g for g in ops["B"])]
        ss = [s for s in many if shared and shared[0][0][s] != 0]
        if ss:
            once = [ss[0]]
            if rng.random() < 0.5:
                role_of = {frozenset("AC"): "I", frozenset("BC"): "J", frozenset("AB"): "K"}
                role = {s: role_of.get(frozenset(nm for nm in "ABC"
                                                 if any(f[0][s] != 0 for f in ops[nm])))
                        for s in many}
                roles = sorted(set(role.values()) - {None})
                if len(roles) >= 2:
                    drop = rng.sample(roles, 2)
                    once += [s for s in many if role[s] in drop]
            for s in once:
                lb, ub, st = loops[s]
                loops[s] = (lb, lb + rng.randint(1, st), st)
                trip[s] = 1
    elif kind in ("skew", "merge_c"):
        # skew: one dimension of an operand gets a multiple of another of its
        # dimensions that reads a different role (A[i][i + k], C[i + j][j]):
        # a skewed view, a GEMM (docs section 19). merge_c: C's two role
        # dimensions become one, C[a * f_I + b * f_J]: injective or not (G2),
        # depending on a, b and the extents.
        name = rng.choice("ABC") if kind == "skew" else "C"
        role_dims = [d for d, f in enumerate(ops[name]) if f[1] is not None and
                     any(f[0][s] != 0 for s in many)]
        if len(role_dims) >= 2:
            d1, d2 = rng.sample(role_dims, 2)
            f1, f2 = ops[name][d1], ops[name][d2]
            if not any(f1 is g or f2 is g for nm in "ABC" if nm != name for g in ops[nm]):
                if kind == "skew":
                    c = rng.choice([1, 1, -1, 2])
                    f1[0] = [x + c * y for x, y in zip(f1[0], f2[0])]
                else:
                    a, b = rng.choice([1, 2, 3, 4, 8]), rng.choice([1, 1, 2, 4, -1])
                    f1[0] = [a * x + b * y for x, y in zip(f1[0], f2[0])]
                    ops[name].pop(d2)
    elif kind == "scale_coef" and varying:
        f = rng.choice(varying)
        f[0] = [-2 * c for c in f[0]]            # still valid: a reversed, strided view
    elif kind == "swap_roles":
        ops["A"], ops["B"] = ops["B"], ops["A"]
    elif kind == "add_const_loop":
        once = [s for s in range(len(loops)) if trip[s] == 1]
        if once:
            rng.choice(every)[0][rng.choice(once)] += rng.choice([-5, 7])
    elif kind == "drop_dim":
        name = rng.choice("ABC")
        if len(ops[name]) > 1:
            ops[name].pop(rng.randrange(len(ops[name])))
    return kind, cload


def decorate_bounds(rng, nest):
    """Redundant constant bounds for some loops: the lower bound a max, the
    upper a min, of the effective bound and looser ones, in random order."""
    bounds = []
    for lb, ub, _ in nest.loops:
        lowers, uppers = [lb], [ub]
        if rng.random() < 0.3:
            lowers += [lb - rng.randint(1, 3) for _ in range(rng.choice([1, 1, 2]))]
            rng.shuffle(lowers)
        if rng.random() < 0.3:
            uppers += [ub + rng.randint(1, 3) for _ in range(rng.choice([1, 1, 2]))]
            rng.shuffle(uppers)
        bounds.append((lowers, uppers))
    nest.bounds = bounds


def has_folded_bound(nest, loops=None):
    """Whether one of the loops (all, or the given indices) is emitted with a
    max or min bound."""
    if not nest.bounds:
        return False
    return any(len(lo) > 1 or len(up) > 1
               for s, (lo, up) in enumerate(nest.bounds) if loops is None or s in loops)


def generate_same_view(rng):
    """One memref as both factors through ONE view (docs section 7): A'(I, K)
    and B'(K, J) the same element map -- the same offsets and strides on a
    square M = N = K --, which the compiler serves with one tensor. The three
    role loops are one and the same (lb, ub, step); B's subscripts are A's
    with the role columns moved (I's loop -> K's loop, K's loop -> J's loop),
    the terms of a loop that runs once the same in both; the constants are
    made equal after finalize (unify_same_view)."""
    trip = rng.randint(2, 4)
    role_loop = make_loop(rng, trip)
    coord = ["I", "J", "K"]
    rng.shuffle(coord)
    if rng.random() < 0.3:
        coord.insert(rng.randint(0, 3), None)      # a loop that runs once
    n = len(coord)
    loops = [role_loop if c is not None else make_loop(rng, 1) for c in coord]
    pos = {r: coord.index(r) for r in "IJK"}

    def row(role, p):
        coefs = [0] * n
        coefs[pos[role]] = p
        return coefs
    p_first = rng.choice([1, 1, 1, 2, 3]) * rng.choice([1, 1, -1])
    p_second = rng.choice([1, 1, 1, 2, 3]) * rng.choice([1, 1, -1])
    a_dims = [[row("I", p_first), 0], [row("K", p_second), 0]]
    b_dims = [[row("K", p_first), 0], [row("J", p_second), 0]]
    if rng.random() < 0.35:                        # both stored transposed
        a_dims.reverse()
        b_dims.reverse()
    if rng.random() < 0.3:                         # a constant dimension, same place in both
        d = rng.randint(0, 2)
        a_dims.insert(d, [[0] * n, None])
        b_dims.insert(d, [[0] * n, None])
    c_dims = [[row("I", rng.choice([1, 1, 2, -1])), 0], [row("J", rng.choice([1, 1, 2, -1])), 0]]
    if rng.random() < 0.3:
        c_dims.reverse()
    once = [s for s in range(n) if coord[s] is None]
    for d in range(len(a_dims)):
        for s in once:
            if rng.random() < 0.4:
                delta = rng.randint(-3, 3)
                a_dims[d][0][s] += delta
                b_dims[d][0][s] += delta
    for f in c_dims:
        for s in once:
            if rng.random() < 0.4:
                f[0][s] += rng.randint(-3, 3)
    ops = {"A": a_dims, "B": b_dims, "C": c_dims}
    for s in once:
        if all(f[0][s] == 0 for fs in ops.values() for f in fs):
            c_dims[0][0][s] = rng.choice([-2, -1, 1, 2])
    return loops, [1 if c is None else trip for c in coord], ops


def unify_same_view(nest):
    """After finalize: B's constants are A's (the same element map), and the
    two factors share A's memref."""
    a_dims, b_dims = nest.ops["A"][1], nest.ops["B"][1]
    for fa, fb in zip(a_dims, b_dims):
        fb[1] = fa[1]
    pts = nest.points()
    shape = tuple(max(nest.ops["A"][0][d], nest.ops["B"][0][d],
                      max(value(f, x) for f in (fa, fb) for x in pts) + 1)
                  for d, (fa, fb) in enumerate(zip(a_dims, b_dims)))
    nest.ops["A"] = (shape, a_dims)
    nest.ops["B"] = (shape, b_dims)
    nest.arg_of = {"A": 0, "B": 0, "C": 1}
    nest.shared = "same_view"


def share_base(nest):
    """B reads A's memref through its own view (docs section 7, 19.12.3): one
    buffer of the two shapes' elementwise maximum, two read-only views. False
    when the ranks differ (one memref has one rank)."""
    if len(nest.ops["A"][0]) != len(nest.ops["B"][0]):
        return False
    shape = tuple(max(a, b) for a, b in zip(nest.ops["A"][0], nest.ops["B"][0]))
    nest.ops["A"] = (shape, nest.ops["A"][1])
    nest.ops["B"] = (shape, nest.ops["B"][1])
    nest.arg_of = {"A": 0, "B": 0, "C": 1}
    nest.shared = "two_views"
    return True


def alias_output(nest):
    """C is a view of A's memref: the output may overlap a factor, which the
    compiler refuses (section 7: 'A may overlap C'); not lowered whatever the
    mathematics says."""
    if len(nest.ops["A"][0]) != len(nest.ops["C"][0]):
        return False
    shape = tuple(max(a, c) for a, c in zip(nest.ops["A"][0], nest.ops["C"][0]))
    nest.ops["A"] = (shape, nest.ops["A"][1])
    nest.ops["C"] = (shape, nest.ops["C"][1])
    nest.arg_of = {"A": 0, "B": 1, "C": 0}
    nest.shared = "alias_c"
    return True


def shear_domain(rng, nest):
    """Emit the nest on a constant-extent parallelotope (docs section 19.12.1,
    the family the compiler normalizes): one or two loops s >= 1 get their
    bounds shifted by an integer form of outer counters -- coefficients in
    {-2, -1, 1, 2}, one or two outer loops, preferring an outer loop that is
    itself sheared (a chained shift). With a small probability the nest is a
    'triangle' instead: the upper bound shifted by a different form (or not
    at all), outside the family -- the compiler must not lower it."""
    n = len(nest.loops)
    if n < 2:
        return
    nest.shear = [[0] * s for s in range(n)]
    chosen = sorted(rng.sample(range(1, n), min(n - 1, rng.choice([1, 1, 2]))))
    for s in chosen:
        picks = set()
        sheared_outer = [t for t in range(s) if any(nest.shear[t])]
        if sheared_outer and rng.random() < 0.7:
            picks.add(rng.choice(sheared_outer))
        want = min(s, rng.choice([1, 1, 2]))
        while len(picks) < want:
            picks.add(rng.randrange(s))
        for t in picks:
            nest.shear[s][t] = rng.choice([-2, -1, 1, 1, 2])
    nest.domain_mode = "shear"
    if rng.random() < 0.18:
        s = rng.choice(chosen)
        nest.shear_ub = [list(row) for row in nest.shear]
        if rng.random() < 0.6:
            nest.shear_ub[s] = [0] * s                 # a constant upper bound
        else:
            t = next(t for t in range(s) if nest.shear[s][t])
            nest.shear_ub[s][t] += rng.choice([-1, 1, 2])   # another slope
        nest.domain_mode = "triangle"


def generate(rng):
    if rng.random() < 0.04:
        loops, trip, ops = generate_same_view(rng)
        nest = finalize(rng, loops, ops, "valid:same_view")
        unify_same_view(nest)
    else:
        loops, trip, ops = generate_valid(rng)
        r = rng.random()
        if r < 0.45:
            nest = finalize(rng, loops, ops, "valid")
        elif r < 0.55:
            nest = finalize(rng, loops, ops, "oob", oob=True)
        else:
            kind, cload = mutate(rng, loops, trip, ops)
            nest = finalize(rng, loops, ops, "mutated:" + kind, cload=cload)
        r = rng.random()
        if r < 0.12:
            share_base(nest)
        elif r < 0.16:
            alias_output(nest)
    if rng.random() < 0.3:
        shear_domain(rng, nest)
        if nest.domain_mode == "shear":
            check_shear(nest)
    return nest


# ---------------------------------------------------------------------------
# The decision, by enumeration.

def ordered_prefix(nest, batch):
    """Whether the loops `batch` are an outermost prefix of the nest's loops
    of two or more iterations (docs section 25): every loop of two or more
    iterations that is not in `batch` comes after all of them. The batch
    loops are then emitted around the tiles in the order written, and every
    element of C receives the contributions of different batch iterations in
    the program's order (only the order inside one tile changes)."""
    batch, inner = set(batch), False
    for s, loop in enumerate(nest.loops):
        if trips(*loop) < 2:
            continue
        if s not in batch:
            inner = True
        elif inner:
            return False
    return True


def classify_loops(nest, rb=(), whole=(), ordered=False):
    """The compiler's classification of the loops of two or more iterations,
    with the dimension-intersection partitioning of docs/AFFINE_GEMM_MAPPING.md
    section 21 (2026-10-19): a loop read by all three operands is a batch loop;
    one read by C only is a batch loop too (both factors shared by its
    partitions); A and C: I, B and C: J, A and B: K; one factor only: a
    broadcast (refused).  A row or column role that is not one dense
    coordinate -- its subscripts' restrictions not proportional to one
    direction, or the coordinate not a dense mixed radix of its loops -- keeps
    one sub-coordinate and its other loops become batch loops (the compiler's
    chooseDemotion: the loops joined by a subscript form components; the
    component of the largest extent stays, ties to the one read in C's
    innermost dimension, then the first loop; a single component keeps its
    loop of the largest extent); the analyses run again.  A demoted loop must
    be read by C alone in a dimension of its own (else UNSUPPORTED: the
    partitions' outputs would overlap).  A reduction role that is not one
    coordinate is refused (a partial-C reduction).
    `rb`: the reduction batch loops of the decomposition tried (docs section
    23, decide): loops C does not read, made batch loops -- never a role, no
    broadcast refusal for them --, each value a GEMM accumulating into the
    same slab of C. Empty: the recognition of sections 19-21 alone.
    `whole`: the loops of output roles demoted all together (section 25:
    C not injective on the (I, J) box of an accumulation; decide_with), batch
    loops and demoted loops like the others. `ordered`: the nest accumulates
    on a box with its counters as written (section 25) -- then, when the
    batch loops are an outermost prefix (ordered_prefix), a demoted loop may
    share a dimension of C with a role coordinate: the partitions' outputs
    overlap, and each batch iteration reads C as the previous one wrote it.
    Returns (batch_loops, role_loops, directions, demoted, None) or
    (None, None, None, None, (verdict, why))."""
    n = len(nest.loops)
    many = [trips(*loop) >= 2 for loop in nest.loops]
    reads = {name: {s for f in nest.ops[name][1] for s in range(n) if f[0][s] != 0 and many[s]}
             for name in "ABC"}
    rb = set(rb)
    assert not rb & reads["C"], "a reduction batch loop is read by C"
    triple = reads["A"] & reads["B"] & reads["C"]
    demoted = {s for s in range(n) if many[s] and s in reads["C"]
               and s not in reads["A"] and s not in reads["B"]} | set(whole)
    for s in range(n):
        if many[s] and s not in rb and s not in reads["C"] and \
                len([nm for nm in "AB" if s in reads[nm]]) == 1:
            return None, None, None, None, ("REJECT", "loop %d is read by one operand only (a broadcast)" % s)

    def choose(cand):
        cand = sorted(cand)
        parent = {s: s for s in cand}

        def find(x):
            while parent[x] != x:
                parent[x] = parent[parent[x]]
                x = parent[x]
            return x
        for name in "ABC":
            for f in nest.ops[name][1]:
                here = [s for s in cand if f[0][s] != 0]
                for s in here[1:]:
                    parent[find(s)] = find(here[0])
        c_dim = {s: max([d for d, f in enumerate(nest.ops["C"][1]) if f[0][s] != 0] or [-1]) for s in cand}
        comps = {}
        for s in cand:
            comps.setdefault(find(s), []).append(s)
        comps = list(comps.values())

        def key(ss):
            ext = 1
            for s in ss:
                ext *= trips(*nest.loops[s])
            return (-ext, -max(c_dim[s] for s in ss), min(ss))
        keep = min(comps, key=key)
        if len(comps) > 1:
            return {s for ss in comps if ss is not keep for s in ss}
        best = min(keep, key=lambda s: key([s]))
        return {s for s in keep if s != best}

    for _ in range(n + 1):
        batch_loops = sorted(triple | demoted | rb)
        role_loops = {"I": sorted((reads["A"] & reads["C"]) - set(batch_loops)),
                      "J": sorted((reads["B"] & reads["C"]) - set(batch_loops)),
                      "K": sorted((reads["A"] & reads["B"]) - set(batch_loops))}
        # a demoted loop is read by C alone in a dimension of its own -- unless
        # the batches run in the program's order around the tiles (section 25)
        if ordered and ordered_prefix(nest, batch_loops):
            # Then a demoted loop may share a dimension of C with a row or
            # column coordinate. Not lowered (UNSUPPORTED, as before section
            # 25): when it shares one with the role's coordinate in the role's
            # factor too, the two subscripts -- the role's loops and the
            # demoted ones -- must be multiples of one linear form (else an
            # operand-specific decomposition of the role's coordinate, the
            # batch moving it differently in the factor and in C: SQ-1).
            for role, factor in (("I", "A"), ("J", "B")):
                loops = role_loops[role]
                span = sorted(set(loops) | demoted)

                def mixed(f):
                    return any(f[0][s] for s in loops) and any(f[0][s] for s in demoted)
                for ff in [f for f in nest.ops[factor][1] if mixed(f)]:
                    for fc in [f for f in nest.ops["C"][1] if mixed(f)]:
                        u, v = [ff[0][s] for s in span], [fc[0][s] for s in span]
                        if any(u[a] * v[b] != u[b] * v[a] for a in range(len(span)) for b in range(len(span))):
                            return None, None, None, None, (
                                "UNSUPPORTED", "role %s's coordinate is moved by a demoted loop in %s and in C "
                                "through subscripts that are not multiples of one linear form (an operand-specific "
                                "decomposition, SQ-1)" % (role, factor))
        for d, f in enumerate(nest.ops["C"][1]):
            if ordered and ordered_prefix(nest, batch_loops):
                break
            dem = [s for s in range(n) if f[0][s] != 0 and s in demoted]
            if dem and any(f[0][s] != 0 and many[s] and s not in batch_loops for s in range(n)):
                return None, None, None, None, ("UNSUPPORTED", "loop %d would partition the GEMM but C reads it in dimension %d together with a role coordinate (the partitions' outputs would not be independent slabs)" % (dem[0], d))
        if batch_loops and not any(role_loops.values()):
            return None, None, None, None, ("REJECT", "an element-wise product (every coordinate a batch one)")
        directions, candidate = {}, None
        for role, loops in role_loops.items():
            if not loops:
                continue
            forms = [[f[0][s] for s in loops] for name in "ABC" for f in nest.ops[name][1]]
            nonzero = [f for f in forms if any(f)]
            if any(a[s] * b[t] != a[t] * b[s] for a in nonzero for b in nonzero
                   for s in range(len(loops)) for t in range(len(loops))):
                if role in "IJ":
                    candidate = loops
                    break
                return None, None, None, None, ("REJECT", "role K is read through two different combinations of its loops (a partial-C reduction / separate reduction coordinates)")
            g = nonzero[0]
            # the coordinate's values over the role's box, taken once and consecutively?
            values = {}
            pts_role = itertools.product(*[[lb + st * z for z in range(trips(lb, ub, st))]
                                           for lb, ub, st in (nest.loops[s] for s in loops)])
            vals = sorted({sum(c * x for c, x in zip(g, pt)) for pt in pts_role})
            count = 1
            for s in loops:
                count *= trips(*nest.loops[s])
            step = vals[1] - vals[0] if len(vals) > 1 else 1
            if len(vals) != count or any(b - a != step for a, b in zip(vals, vals[1:])):
                if role in "IJ" and len(loops) > 1:
                    candidate = loops
                    break
                return None, None, None, None, ("REJECT", "role %s is not one dense coordinate of its loops" % role)
            directions[role] = g
        if candidate is None:
            return batch_loops, role_loops, directions, sorted(demoted), None
        chosen = choose(candidate)
        if not chosen:
            return None, None, None, None, ("REJECT", "role over several coordinates, no demotion")
        demoted |= chosen
    return None, None, None, None, ("REJECT", "the demotion did not converge")


def reduction_loops(nest):
    """The loops of two or more iterations C does not read, by their readers
    (docs sections 22-23): (one_factor, shared) -- read by one factor only (a
    'broadcast' before section 23), and read by both factors (the
    contraction candidates S)."""
    n = len(nest.loops)
    many = [trips(*loop) >= 2 for loop in nest.loops]
    reads = {name: {s for f in nest.ops[name][1] for s in range(n) if f[0][s] != 0 and many[s]}
             for name in "ABC"}
    one = sorted(s for s in range(n) if many[s] and s not in reads["C"]
                 and (s in reads["A"]) != (s in reads["B"]))
    shared = sorted((reads["A"] & reads["B"]) - reads["C"])
    return one, shared


def reduction_batch_choices(nest):
    """The decompositions with reduction batch loops (docs section 23.2), in
    the documented order: first the loops read by one factor only as RB (the
    contraction loops all K); then, when two or more loops are read by both
    factors and not by C, each of them in turn as K -- the longest first (the
    largest tiles), ties the innermost -- and the others RB as well."""
    one, shared = reduction_loops(nest)
    choices = [frozenset(one)]
    if len(shared) >= 2:
        for k in sorted(shared, key=lambda s: (-trips(*nest.loops[s]), -s)):
            choices.append(frozenset(one) | (frozenset(shared) - {k}))
    return choices


def decide(nest, reduction_batches=True, ordered=True):
    """The verdict on the box nest: ("ACCEPT", (M, N, K), number of batch
    loops, general, batch loops, reduction batch loops) or (REJECT |
    UNSUPPORTED | SKIP, why).

    With `reduction_batches` (the default since 2026-10-21; the nest is an
    accumulation on a box: every generated nest accumulates, C = C + A * B,
    so the assignment rule -- an assignment never gets reduction batch loops,
    section 20.4 -- has nothing to exclude here) the decompositions of
    reduction_batch_choices are tried in order and the first that is a dense
    GEMM by enumeration (decide_with) is the verdict; when none is, the
    verdict is the recognition of sections 19-21 alone (no reduction batch
    loop), its reason included. Not a copy of the compiler's search: each
    choice is decided by enumerating its points, every reduction batch value
    a GEMM of its own on its points (gemm_on_points) into the slab of C its
    points address.

    `ordered` (section 25, independent of `reduction_batches`): the nest is
    decided on its counters as written (no domain normalization with U != I)
    -- every generated nest accumulates and is decided on a box -- so a C
    that is not injective on a tile's (I, J) box may be resolved by ordered
    output batches (decide_with)."""
    pts = nest.points()
    if not pts:
        return ("SKIP", "empty domain (SQ-2)")
    n = len(nest.loops)
    used = [any(f[0][s] != 0 for _, fs in nest.ops.values() for f in fs) or
            any(f[0][s] != 0 for f in nest.cload) for s in range(n)]
    if not all(used):
        return ("SKIP", "a loop no access uses (a repetition loop)")
    for d, f in enumerate(nest.ops["C"][1]):
        if any(value(f, x) != value(nest.cload[d], x) for x in pts):
            return ("REJECT", "the accumulator is loaded and stored at different elements")
    # Every access inside its memref (else the program is undefined).
    for name in "ABC":
        shape, fs = nest.ops[name]
        for d, f in enumerate(fs):
            vals = [value(f, x) for x in pts]
            if min(vals) < 0 or max(vals) >= shape[d]:
                return ("REJECT", "%s is accessed out of bounds along dimension %d" % (name, d))
    if reduction_batches:
        for rb in reduction_batch_choices(nest):
            verdict = decide_with(nest, pts, rb, ordered)
            if verdict[0] == "ACCEPT":
                return verdict
    return decide_with(nest, pts, frozenset(), ordered)


def decide_with(nest, pts, rb, ordered=False):
    """One decomposition: the roles by readers with the dimension-intersection
    partitioning (classify_loops; docs sections 19.2 and 21), the loops `rb`
    reduction batch loops (section 23). C may read batch loops anywhere (no C
    purity since 2026-10-16) except that a demoted loop is read alone; what C
    must be is injective per batch iteration (G2). A reduction batch loop is
    not read by C, so its iterations with the other batch values fixed all
    write one slab of C: the lowering accumulates them into it in order
    (check_serial_accumulation checks that on the IR).

    Ordered output batches (docs section 25, `ordered`: an accumulation on a
    box, counters as written): when a batch iteration's C is not injective on
    its (I, J) box -- two outputs of one tile would write one element, one
    update lost -- the contributions to that element are a reduction the
    program performs in its order. One output role is made batch loops all
    together -- the role of the product's first factor first, then the
    other's (a role with loops) -- when its loops with the batch loops are an
    outermost prefix of the nest (ordered_prefix): the batch iterations then
    run in the order written, each accumulating into C as the previous one
    left it, so every contribution is kept, in order across batch iterations
    and reassociated only inside one tile. The decomposition is decided again
    by enumeration (every tile a GEMM with C injective); with no such role the
    refusal stands. The lowering is checked address by address per batch
    iteration and by check_ordered_contributions."""
    whole = set()
    for _ in range(len(nest.loops) + 1):
        verdict = decide_parts(nest, pts, rb, whole, ordered)
        if verdict[0] != "UNSUPPORTED" or not ordered or len(verdict) < 3:
            return verdict[:2] if verdict[0] != "ACCEPT" else verdict
        # C not injective on a tile's (I, J) box: an output role demoted
        role_loops, batch_loops = verdict[2], verdict[3]
        first = "I" if nest.order[0] == "A" else "J"
        chosen = None
        for role in (first, "J" if first == "I" else "I"):
            loops = role_loops[role]
            if loops and ordered_prefix(nest, set(batch_loops) | set(loops)):
                chosen = loops
                break
        if chosen is None:
            return verdict[:2]
        whole |= set(chosen)
    return ("REJECT", "the ordered output batches did not converge")


def decide_parts(nest, pts, rb, whole, ordered):
    """decide_with for one set of whole-role demotions `whole`; a refusal for
    C not injective on a tile's box carries the role and batch loops (the
    retry's input)."""
    batch_loops, role_loops, directions, demoted, reject = classify_loops(nest, rb, whole, ordered)
    if reject is not None:
        return reject
    # Per batch iteration: a GEMM of affine views, every iteration the same.
    # The batch iterations are the batch loops' values: with the points of a
    # box grouped by them, every point is in exactly one group.
    groups = {}
    for x in pts:
        groups.setdefault(tuple(x[s] for s in batch_loops), []).append(x)
    shape, general = None, False
    for sub in groups.values():
        verdict = gemm_on_points(nest, sub, role_loops, directions)
        if verdict[0] == "UNSUPPORTED":
            return (verdict[0], verdict[1] + (" (within a batch iteration)" if batch_loops else ""),
                    role_loops, batch_loops)
        if verdict[0] != "ACCEPT":
            return (verdict[0], verdict[1] + (" (within a batch iteration)" if batch_loops else ""))
        if shape is None:
            shape = verdict[1]
        elif verdict[1] != shape:
            return ("REJECT", "the batch iterations are not one GEMM shape")
        general = general or verdict[2]
    if rb:
        # Each slab of C: the reduction batch iterations that share the
        # other batch values write the same elements (C reads no RB loop), so
        # every one of them accumulates into the slab the first one does.
        slabs = {}
        for key, sub in groups.items():
            outer = tuple(v for s, v in zip(batch_loops, key) if s not in rb)
            elements = frozenset(tuple(value(f, x) for f in nest.ops["C"][1]) for x in sub)
            if slabs.setdefault(outer, elements) != elements:
                return ("REJECT", "the reduction batch iterations of one slab write different elements of C")
    # Section 25 on the decision: the batch iterations of an ordered
    # decomposition may write overlapping elements of C (the whole-role
    # demotions, a demoted loop sharing C's dimension with a role); `overlap`
    # records it, and whether the batch loops are an outermost prefix.
    overlap = False
    seen = {}
    for key, sub in groups.items():
        outer = tuple(v for s, v in zip(batch_loops, key) if s not in rb)
        for x in sub:
            element = tuple(value(f, x) for f in nest.ops["C"][1])
            if seen.setdefault(element, outer) != outer:
                overlap = True
    # A demoted loop sharing a dimension of C with a row or column coordinate
    # (refused before section 25: the partitions' outputs are not
    # independent slabs).
    shared_dim = any(any(f[0][s] for s in demoted) and
                     any(f[0][s] and trips(*nest.loops[s]) >= 2 and s not in batch_loops
                         for s in range(len(nest.loops)))
                     for f in nest.ops["C"][1])
    overlap = (overlap, ordered_prefix(nest, batch_loops), shared_dim)
    return ("ACCEPT", shape, len(batch_loops), general, list(batch_loops), sorted(rb), sorted(whole), overlap)


def gemm_on_points(nest, pts, role_loops, directions):
    """The enumeration on one batch iteration's points (or all of them):
    ("ACCEPT", (M, N, K), general) or ("REJECT" | "UNSUPPORTED", why).
    `general`: some operand dimension combines two roles or a role spans
    several dimensions (a skewed or merged view)."""
    ext, ordinal = {}, {}
    for role in "IJK":
        if role not in directions:
            ext[role], ordinal[role] = 1, (lambda x: 0)
            continue
        loops, g = role_loops[role], directions[role]
        coordinate = (lambda g, loops: lambda x: sum(c * x[s] for c, s in zip(g, loops)))(g, loops)
        values = sorted({coordinate(x) for x in pts})
        step = values[1] - values[0] if len(values) > 1 else 1
        if any(b - a != step for a, b in zip(values, values[1:])):
            return ("REJECT", "role %s's values are not an arithmetic progression (no strided view)" % role)
        count = 1
        for s in loops:
            count *= len({x[s] for x in pts})
        if len(values) != count:
            return ("REJECT", "role %s takes a value more than once (a weighted sum)" % role)
        ext[role] = len(values)
        ordinal[role] = (lambda c, lo, st: lambda x: (c(x) - lo) // st)(coordinate, values[0], step)
    slots = {(ordinal["I"](x), ordinal["J"](x), ordinal["K"](x)) for x in pts}
    if len(slots) != len(pts) or len(pts) != ext["I"] * ext["J"] * ext["K"]:
        return ("REJECT", "(I, J, K) is not a bijection onto the box")
    # G2: C's element is one per (I, J), and different (I, J) write different elements.
    written = {}
    for x in pts:
        element = tuple(value(f, x) for f in nest.ops["C"][1])
        if written.setdefault((ordinal["I"](x), ordinal["J"](x)), element) != element:
            return ("REJECT", "C's element changes along the reduction")
    if len(set(written.values())) != len(written):
        return ("UNSUPPORTED", "C's view is not injective on the (I, J) box (a decomposition with more "
                               "batch loops would be valid and is not constructed)")
    general = False
    for name, roles in (("A", "IK"), ("B", "KJ"), ("C", "IJ")):
        dims = {role: [] for role in roles}
        for d, f in enumerate(nest.ops[name][1]):
            used = [role for role in roles if role in directions and any(f[0][s] for s in role_loops[role])]
            general = general or len(used) > 1
            for role in used:
                dims[role].append(d)
        general = general or any(len(v) > 1 for v in dims.values())
    return ("ACCEPT", (ext["I"], ext["J"], ext["K"]), general)


# ---------------------------------------------------------------------------
# MLIR emission.

def expr(f, ivs):
    coefs, const = f
    terms = [iv if c == 1 else "%s * %d" % (iv, c) for c, iv in zip(coefs, ivs) if c != 0]
    text = " + ".join(terms)
    if const or not terms:
        text = (text + " + " if text else "") + str(const)
    return text.replace("+ -", "- ")


def mtype(shape):
    return "memref<%sxf32>" % "x".join(str(d) for d in shape)


def emit(nest, name):
    n = len(nest.loops)
    ivs = ["%%x%d" % s for s in range(n)]
    # One memref argument per distinct buffer, named after the first operand
    # that uses it (A and B, or A and C, may share one: nest.arg_of).
    arg_name = {}
    for k in "ABC":
        arg_name.setdefault(nest.arg_of[k], "%" + k)
    args = []
    for a in sorted(arg_name):
        shapes = {nest.ops[k][0] for k in "ABC" if nest.arg_of[k] == a}
        assert len(shapes) == 1, "operands sharing a memref must share its shape"
        args.append("%s: %s {llvm.noalias}" % (arg_name[a], mtype(shapes.pop())))
    ty = {k: mtype(nest.ops[k][0]) for k in "ABC"}
    ref = {k: arg_name[nest.arg_of[k]] for k in "ABC"}
    out = ["func.func @%s(%s) {" % (name, ", ".join(args))]
    ind = "  "
    L = shear_matrix(nest)

    def bound(kind, results):
        """The results [(row, constant)] of a bound (one, or several folded by
        max/min), each row . x_<s + constant: affine_map<(d..) -> (..)>(outer
        counters)."""
        dims = sorted({t for row, _ in results for t, c in enumerate(row) if c})
        if not dims:
            if len(results) == 1:
                return str(results[0][1])
            return "%s affine_map<() -> (%s)>()" % (kind, ", ".join(str(v) for _, v in results))
        names = {t: "d%d" % i for i, t in enumerate(dims)}
        texts = []
        for row, v in results:
            terms = [names[t] if c == 1 else "%s * %d" % (names[t], c) for t, c in enumerate(row) if c]
            text = " + ".join(terms)
            if v or not terms:
                text = (text + " + " if text else "") + str(v)
            texts.append(text.replace("+ -", "- "))
        return "%saffine_map<(%s) -> (%s)>(%s)" % (kind + " " if len(results) > 1 else "",
                                                   ", ".join(names[t] for t in dims), ", ".join(texts),
                                                   ", ".join(ivs[t] for t in dims))
    for s, ((lb, ub, st), iv) in enumerate(zip(nest.loops, ivs)):
        lowers, uppers = nest.bounds[s] if nest.bounds else ([lb], [ub])
        assert max(lowers) == lb and min(uppers) == ub
        lo_res, up_res = emitted_results(nest, s)
        out.append("%saffine.for %s = %s to %s%s {" % (ind, iv, bound("max", lo_res),
                                                       bound("min", up_res),
                                                       "" if st == 1 else " step %d" % st))
        ind += "  "
    idx = lambda fs: ", ".join(expr(emitted_row(f, L), ivs) for f in fs)
    for k in "AB":
        out.append("%s%%%s = affine.load %s[%s] : %s" % (ind, k.lower(), ref[k], idx(nest.ops[k][1]), ty[k]))
    out.append("%s%%c = affine.load %s[%s] : %s" % (ind, ref["C"], idx(nest.cload), ty["C"]))
    out.append("%s%%p = arith.mulf %%%s, %%%s : f32" % (ind, nest.order[0].lower(), nest.order[1].lower()))
    out.append("%s%%s = arith.addf %%c, %%p : f32" % ind)
    out.append("%saffine.store %%s, %s[%s] : %s" % (ind, ref["C"], idx(nest.ops["C"][1]), ty["C"]))
    for _ in nest.loops:
        ind = ind[:-2]
        out.append(ind + "}")
    out.append("  return\n}")
    return "\n".join(out)


# ---------------------------------------------------------------------------
# The compiler's views.

DEF = re.compile(r"^\s*(%[\w.]+) = (?:memref\.subview|memref\.transpose|memref\.expand_shape|memref\.reinterpret_cast) (%[\w.]+)")
TO_TENSOR = re.compile(r"^\s*(%[\w.]+) = bufferization\.to_tensor (%[\w.]+) .*: (memref<[^>]*(?:<[^>]*>[^>]*)?>) to tensor")
TILE = re.compile(r"systolic\.matmul_tile (%[\w.]+), (%[\w.]+), (%[\w.]+) m = (\d+) n = (\d+) k = (\d+)")
TILE_DEF = re.compile(r"^\s*(%[\w.]+) = systolic\.matmul_tile ")
MATERIALIZE = re.compile(r"^\s*bufferization\.materialize_in_destination (%[\w.]+) in (?:restrict )?(?:writable )?(%[\w.]+)")
MEMREF = re.compile(r"memref<([\dx]+)xf32(?:, strided<\[([-\d, ]+)\](?:, offset: (-?\d+|\?))?>)?>")
MAPDEF = re.compile(r"^(#[\w$.]+) = affine_map<\(([^)]*)\) -> \((.*)\)>\s*$")
APPLY = re.compile(r"^\s*(%[\w.]+) = affine\.apply (#[\w$.]+|affine_map<\(([^)]*)\) -> \((.*?)\)>)\(([^)]*)\)")
FORLOOP = re.compile(r"^\s*affine\.for (%[\w.]+) = (-?\d+) to (-?\d+)(?: step (\d+))? \{")
SUBVIEW = re.compile(r"^\s*(%[\w.]+) = memref\.subview (%[\w.]+)\[([^\]]*)\] \[([^\]]*)\] \[([^\]]*)\] : memref<([\dx]+)xf32>")
# An explicit view (docs/AFFINE_GEMM_MAPPING.md section 19.11): one reinterpret_cast of the
# base memref to rows x columns with an element offset (a constant, or an affine.apply of
# the batch counters), sizes and element strides.
REINTERPRET = re.compile(r"^\s*(%[\w.]+) = memref\.reinterpret_cast (%[\w.]+) to offset: \[([^\]]*)\], sizes: \[([^\]]*)\], strides: \[([^\]]*)\] : memref<([\dx]+)xf32>")


def parse_view(type_text):
    """(shape, strides, offset) of a view type; offset None when dynamic (a
    batched nest: computed from the subview's offsets per batch value)."""
    m = MEMREF.match(type_text)
    if m is None:
        return None
    shape = [int(v) for v in m.group(1).split("x")]
    if m.group(2):
        strides = [int(v) for v in m.group(2).split(",")]
        offset = None if m.group(3) == "?" else int(m.group(3) or 0)
    else:
        strides = [1] * len(shape)
        for d in range(len(shape) - 2, -1, -1):
            strides[d] = strides[d + 1] * shape[d + 1]
        offset = 0
    return shape, strides, offset


# A refusal remark with its class (classify=true), located at the nest's anchor loop:
# with -split-input-file the line numbers are those of the whole input.
REMARK = re.compile(r"^<stdin>:(\d+):\d+: remark: affine GEMM not lowered to systolic\.matmul_tile: (.*) "
                    r"\[(PROVEN_VALID|VALID_BUT_LOWERING_UNSUPPORTED|PROVEN_INVALID|UNKNOWN)\]$", re.M)


def compiler_results(opt, text, permit, reduction_batches=True):
    """The lowered views of every nest (None when not lowered), and the refusal
    remarks [(line, class, reason)]. classify=true only appends each refused
    nest's class to its remark; the IR is the same. Reduction-batch
    partitioning is the pass's default (docs section 23, since 2026-10-21):
    the option is passed only to turn it off."""
    option = "--affine-matmul-to-systolic=classify=true" + (" allow-fp-reassociation=true" if permit else "") + \
        ("" if reduction_batches else " reduction-batches=false")
    run = subprocess.run([opt, option, "-split-input-file", "-"], input=text,
                         capture_output=True, text=True)
    if run.returncode != 0:
        sys.stdout.write("systolic-opt failed (rc %d):\n%s\n" % (run.returncode, run.stderr[-4000:]))
        sys.exit(1)
    remarks = [(int(m.group(1)), m.group(3), m.group(2)) for m in REMARK.finditer(run.stderr)]
    results = []
    for chunk in run.stdout.split("// -----"):
        if "func.func" not in chunk:
            continue
        tile = TILE.search(chunk)
        if not tile:
            results.append(None)
            continue
        source, tensors, subviews, maps, applies, batch_loops = {}, {}, {}, {}, {}, []
        # Where the C tile is read, computed and written back: per line, its
        # position and the counters of the affine.for loops around it.
        stack, placed, materialized = [], {}, []
        for number, line in enumerate(chunk.splitlines()):
            around = tuple(label for label in stack if label)
            loop = FORLOOP.match(line)
            for ch in line:
                if ch == "{":
                    stack.append(loop.group(1) if loop else None)
                elif ch == "}" and stack:
                    stack.pop()
            m = TILE_DEF.match(line)
            if m:
                placed["tile"] = (m.group(1), number, around)
            m = MATERIALIZE.match(line)
            if m:
                materialized.append((m.group(1), m.group(2), number, around))
            m = MAPDEF.match(line)
            if m:
                maps[m.group(1)] = (m.group(2), m.group(3))
            m = APPLY.match(line)
            if m:
                dims, body = (maps[m.group(2)] if m.group(2).startswith("#")
                              else (m.group(3), m.group(4)))
                applies[m.group(1)] = (dims, body, [v.strip() for v in m.group(5).split(",")])
            m = FORLOOP.match(line)
            if m:
                batch_loops.append((m.group(1), int(m.group(2)), int(m.group(3)), int(m.group(4) or 1)))
            m = SUBVIEW.match(line)
            if m:
                subviews[m.group(1)] = ("subview", [v.strip() for v in m.group(3).split(",")],
                                        [int(v) for v in m.group(6).split("x")])
            m = REINTERPRET.match(line)
            if m:
                subviews[m.group(1)] = ("reinterpret", m.group(3).strip())
            m = DEF.match(line)
            if m:
                source[m.group(1)] = m.group(2)
            m = TO_TENSOR.match(line)
            if m:
                root, hop = m.group(2), None
                while root in source:
                    if root in subviews:
                        hop = root
                    root = source[root]
                tensors[m.group(1)] = (root, parse_view(m.group(3)), subviews.get(hop))
                placed[m.group(1)] = (m.group(2), number, around)
        views = []
        for t in tile.groups()[:3]:
            root, view, subview = tensors[t]
            # the memref ARGUMENT the view roots at; the nest names it (arg_of)
            views.append((int(root[len("%arg"):]) if root.startswith("%arg") else None,
                          view, subview))
        results.append((tuple(int(v) for v in tile.groups()[3:]), views,
                        len(batch_loops), {"loops": batch_loops, "applies": applies,
                                           "operands": list(tile.groups()[:3]),
                                           "placed": placed, "materialized": materialized}))
    return results, remarks


def column_rank(rows, n):
    """The exact rank of integer rows over n columns (Gaussian elimination in
    fractions)."""
    m = [[Fraction(v) for v in row] for row in rows]
    rank = 0
    for col in range(n):
        pivot = next((r for r in range(rank, len(m)) if m[r][col] != 0), None)
        if pivot is None:
            continue
        m[rank], m[pivot] = m[pivot], m[rank]
        for r in range(len(m)):
            if r != rank and m[r][col] != 0:
                f = m[r][col] / m[rank][col]
                m[r] = [a - f * b for a, b in zip(m[r], m[rank])]
        rank += 1
    return rank


def centrally_symmetric(pts):
    """Whether a nonempty point set equals its reflection about the midpoint
    of its lexicographic extremes (the only centre a symmetric set can have:
    the reflection reverses the order)."""
    s = set(pts)
    c = tuple(a + b for a, b in zip(min(s), max(s)))
    return all(tuple(ci - x for ci, x in zip(c, p)) in s for p in s)


def band_points(nest, first):
    """The points of the band of loops first.. alone, the leading counters at
    their box lower bounds: no band bound reads them, so any value gives the
    same set -- also when the leading run has no point at all (review of
    2026-10-19, round 4: the projection of the nest's points was then empty,
    and a correct PROVEN_INVALID on the band a mismatch)."""
    n = len(nest.loops)
    out = set()

    def rec(s, x):
        if s == n:
            out.add(tuple(x[first:]))
            return
        st = nest.loops[s][2]
        lo_res, up_res = emitted_results(nest, s)
        lo = max(c + sum(a * x[t] for t, a in enumerate(row)) for row, c in lo_res)
        up = min(c + sum(a * x[t] for t, a in enumerate(row)) for row, c in up_res)
        for z in range(trips(lo, up, st)):
            rec(s + 1, x + [lo + st * z])
    rec(first, [nest.loops[t][0] for t in range(first)])
    return sorted(out)


def sheared_box(points):
    """Whether the tuples `points` are a box sheared along their coordinate
    order -- at every value of the earlier coordinates the next one runs over
    an arithmetic progression of one length and step whose start is an
    affine function of the earlier ones (exact, fractions): the points of a
    constant-extent parallelotope nest. Sufficient, not necessary."""
    from fractions import Fraction
    pts = set(points)
    if not pts:
        return False
    for m in range(len(next(iter(pts)))):
        runs = {}
        for p in pts:
            runs.setdefault(p[:m], set()).add(p[m])
        shapes, rows = set(), []
        for prefix, vals in runs.items():
            vals = sorted(vals)
            steps = {b - a for a, b in zip(vals, vals[1:])}
            if len(steps) > 1:
                return False
            shapes.add((len(vals), steps.pop() if steps else None))
            rows.append([Fraction(1)] + [Fraction(c) for c in prefix] + [Fraction(vals[0])])
        if len({k for k, _ in shapes}) != 1 or len({st for _, st in shapes if st is not None}) > 1:
            return False
        rank = 0
        for col in range(len(rows[0]) - 1):
            pivot = next((r for r in range(rank, len(rows)) if rows[r][col] != 0), None)
            if pivot is None:
                continue
            rows[rank], rows[pivot] = rows[pivot], rows[rank]
            for r in range(len(rows)):
                if r != rank and rows[r][col] != 0:
                    f = rows[r][col] / rows[rank][col]
                    rows[r] = [a - f * b for a, b in zip(rows[r], rows[rank])]
            rank += 1
        if any(row[-1] != 0 for row in rows[rank:]):
            return False
    return True


def repetition_family_nest(nest):
    """Whether the nest is a GEMM over a parallelotope repeated by independent
    repetition loops (family R, docs section 19.13.3), which the compiler
    lowers: loops no access reads (as emitted), whose bounds read only each
    other and whose counters no other loop's bounds read, around GEMM loops
    whose points form a box sheared along the loop order (review of
    2026-10-19, round 4: such a nest under a repetition triangle was called
    a lowered triangle, a mismatch)."""
    n = len(nest.loops)
    L = shear_matrix(nest)
    rows = [emitted_row(f, L)[0] for k in "ABC" for f in nest.ops[k][1]] + \
           [emitted_row(f, L)[0] for f in nest.cload]
    unread = {t for t in range(n) if not any(r[t] for r in rows)}
    if not unread:
        return False
    for s in range(n):
        lo, up = emitted_results(nest, s)
        reads = {t for row, _ in lo + up for t, c in enumerate(row) if c}
        if (s in unread and not reads <= unread) or (s not in unread and reads & unread):
            return False
    gemm = [t for t in range(n) if t not in unread]
    return sheared_box([tuple(p[t] for t in gemm) for p in emitted_points(nest)])


# The disproof's witness in its remark (disproveAffineImageOfBox).
WITNESS = re.compile(r"the point \(([-0-9, ]+)\) is in it, its mirror \(([-0-9, ]+)\) about the "
                     r"midpoint of its lexicographic extremes is not")


def check_triangle_class(nest, remark):
    """A refused triangle (no parallelotope of the family, as emitted) and its
    remark (class, reason), or None when the nest was not recognized: the
    class, and a mismatch or None. PROVEN_INVALID (docs section 19.13.4)
    claims that no decomposition makes the nest a GEMM; the oracle confirms it
    from the enumerated points: not centrally symmetric (every affine image
    of a box is), the stacked access map of full column rank (the updates are
    then an affine copy of the points: with a lower rank an asymmetric set can
    map one to one onto a box -- strip-mined loops with a remainder), and
    three different memrefs (two views of one may exchange the factors). The
    converse, for a recognized nest: those conditions with no stepped loop
    whose lower bound reads a counter (the compiler does not attempt that
    one) must give PROVEN_INVALID. The compiler ranks the accumulator load's
    subscripts (accessC); the recognizer accepts a load and a store that
    differ off the domain (terms of loops that run once, finalize), so a
    full rank of either map proves the accesses tell the points apart, and
    the converse asks for the load's (review of 2026-10-19). The remark's
    witness is checked too: a point of the set whose reflection about the
    midpoint of the lexicographic extremes -- the mirror it states -- is not
    in the set (the generated triangles are never symmetric, so this is what
    exercises the symmetry test on them).

    All of it on the compiler's candidate band: the loops from the outermost
    one an access reads inward (collectAffineMatmulCandidates tries the inner
    anchors first, and the first that covers every counter the accesses read
    is the one recognized). Leading loops no access reads run the band zero
    or more times; the points (band_points: the band's own, enumerated
    alone), the ranks and the witness are the band's (review of 2026-10-19:
    a leading loop no access reads made a correct PROVEN_INVALID a
    mismatch). A band whose bounds read such a loop's counter has a domain
    with parameters, which the disproof never claims."""
    n = len(nest.loops)
    L = shear_matrix(nest)
    rows_ab = [emitted_row(f, L)[0] for name in "AB" for f in nest.ops[name][1]]
    rows_load = [emitted_row(f, L)[0] for f in nest.cload]
    rows_store = [emitted_row(f, L)[0] for f in nest.ops["C"][1]]
    read = [t for t in range(n) if any(row[t] for row in rows_ab + rows_load + rows_store)]
    first = min(read) if read else 0
    nb = n - first
    parametric = any(row[t] for s in range(first, n) for side in emitted_results(nest, s)
                     for row, _ in side for t in range(min(first, len(row))))
    pts = sorted({p[first:] for p in emitted_points(nest)}) if parametric else band_points(nest, first)
    full_load = column_rank([row[first:] for row in rows_ab + rows_load], nb) == nb
    full_store = column_rank([row[first:] for row in rows_ab + rows_store], nb) == nb
    three = len({nest.arg_of[name] for name in "ABC"}) == 3
    asymmetric = bool(pts) and not centrally_symmetric(pts)
    stepped = any(nest.loops[s][2] > 1 and any(any(row) for row, _ in emitted_results(nest, s)[0])
                  for s in range(first, n))
    if remark is None:
        return "not recognized", None
    cls, reason = remark
    if cls == "PROVEN_INVALID":
        failed = [what for what, ok in (("the bounds of its candidate band read the counter of a loop "
                                         "outside it", not parametric),
                                        ("its point set is centrally symmetric or empty", asymmetric),
                                        ("its stacked access map has rank below %d" % nb,
                                         full_load or full_store),
                                        ("two of its operands share a memref", three)) if not ok]
        witness = WITNESS.search(reason)
        if not witness:
            failed.append("its remark states no witness point")
        elif pts:
            p, q = (tuple(int(v) for v in witness.group(g).split(",")) for g in (1, 2))
            s = set(pts)
            if len(p) != nb or len(q) != nb:
                failed.append("its witness %s / %s does not have the %d coordinates of the candidate "
                              "band" % (p, q, nb))
            else:
                mirror = tuple(a + b - x for a, b, x in zip(min(s), max(s), p))
                if p not in s or q != mirror or q in s:
                    failed.append("its witness is wrong (the point %s must be in it, and %s must be its "
                                  "mirror %s and outside it)" % (p, q, mirror))
        if failed:
            return cls, "a triangle classified PROVEN_INVALID although %s (%s)" % ("; ".join(failed), reason)
    elif asymmetric and full_load and three and not stepped and not parametric:
        return cls, ("a triangle that is not centrally symmetric, with a stacked access map of rank %d "
                     "and three memrefs, classified %s, not PROVEN_INVALID (%s)" % (nb, cls, reason))
    return cls, None


def name_views(nest, raw, swap=False):
    """The tile's operands as A, B, C: each view roots at a memref argument,
    which nest.arg_of maps back to the operand(s) using it. Two operands on
    one shared argument are told apart by position -- the tile's first factor
    is A, the second B --, or the other way round with `swap` (the compiler
    may have solved the transposed problem B^T A^T, which is accepted)."""
    named = []
    for p, (arg, view, subview) in enumerate(raw):
        candidates = [k for k in "ABC" if nest.arg_of[k] == arg]
        positional = "ABC"[1 - p if swap and p < 2 else p]
        name = positional if positional in candidates else (candidates[0] if candidates else "?")
        named.append((name, view, subview))
    return named


def evaluate(token, env, applies):
    """A subview offset: a constant, or an affine.apply of the batch loops'
    counters (linear forms only)."""
    if re.fullmatch(r"-?\d+", token):
        return int(token)
    dims, body, operands = applies[token]
    names = [d.strip() for d in dims.split(",") if d.strip()]
    if re.search(r"floordiv|ceildiv|mod", body):
        raise ValueError("non-linear batch offset " + body)
    expr = body
    for name, operand in sorted(zip(names, operands), key=lambda p: -len(p[0])):
        expr = re.sub(r"\b%s\b" % name, "(%d)" % env[operand], expr)
    return eval(expr, {"__builtins__": {}}, {})


def batch_views(views, env, applies):
    """The views of one batch iteration: each dynamic offset computed from
    its subview's offsets and the base memref's row-major strides, or from
    its reinterpret_cast's element offset."""
    out = []
    for name, view, subview in views:
        shape, strides, offset = view
        if offset is None:
            if subview[0] == "reinterpret":
                offset = evaluate(subview[1], env, applies)
            else:
                _, offsets, base = subview
                rm = [1] * len(base)
                for d in range(len(base) - 2, -1, -1):
                    rm[d] = rm[d + 1] * base[d + 1]
                offset = sum(evaluate(t, env, applies) * rm[d] for d, t in enumerate(offsets))
        out.append((name, (shape, strides, offset)))
    return out


def linear(shape, index):
    address = 0
    for d, i in zip(shape, index):
        address = address * d + i
    return address


def check_views(nest, mnk, views, pts=None):
    """(V) and (B) on the compiler's own views, on the points `pts` (all of
    them, or one batch iteration's); None when they hold."""
    if pts is None:
        pts = nest.points()
    M, N, K = mnk
    (an, (ash, ast, aof)), (bn, (bsh, bst, bof)), (cn, (csh, cst, cof)) = views
    if cn != "C" or {an, bn} != {"A", "B"}:
        return "the tile's operands are %s, %s, %s" % (an, bn, cn)
    if ash != [M, K] or bsh != [K, N] or csh != [M, N]:
        return "view shapes %s %s %s for %dx%dx%d" % (ash, bsh, csh, M, N, K)

    def hits_of(name, shape, strides, offset, rows, cols):
        """Per point, the view elements (r, c) at the access's address."""
        table = {}
        for r in range(rows):
            for c in range(cols):
                table.setdefault(offset + r * strides[0] + c * strides[1], []).append((r, c))
        base = nest.ops[name][0]
        return [table.get(linear(base, [value(f, x) for f in nest.ops[name][1]]), []) for x in pts]

    # C's view is injective (G2): every access is exactly one element, which
    # fixes (I, J) at the point. A factor's view may repeat elements (a Hankel
    # A[i + k], docs section 19): of its elements at the access's address, the
    # one in row I (A) or column J (B) is the point's; it must be unique.
    lc = hits_of("C", csh, cst, cof, M, N)
    if any(len(h) != 1 for h in lc):
        return "an access of C is not exactly one element of its view"
    lc = [h[0] for h in lc]
    la, lb = [], []
    for ha, hb, (i, j) in zip(hits_of(an, ash, ast, aof, M, K), hits_of(bn, bsh, bst, bof, K, N), lc):
        ra = [(r, c) for r, c in ha if r == i]
        rb = [(r, c) for r, c in hb if c == j]
        if len(ra) != 1 or len(rb) != 1:
            return "an access of a factor is not exactly one element of its view in the output's row or column"
        la.append(ra[0])
        lb.append(rb[0])
    triples = set()
    for (i, k), (k2, j), (i2, j2) in zip(la, lb, lc):
        if i != i2 or k != k2 or j != j2:
            return "A, B and C disagree on (I, J, K) at a point"
        triples.add((i, j, k))
    if len(triples) != len(pts) or len(pts) != M * N * K:
        return "(I, J, K) is not a bijection onto the box"
    # Every view element is inside its allocation (it is some access, which
    # is in bounds -- checked by the decision; here: the view's extreme
    # addresses).
    for name, (shape, strides, offset), rows, cols in ((an, (ash, ast, aof), M, K), (bn, (bsh, bst, bof), K, N), ("C", (csh, cst, cof), M, N)):
        size = 1
        for d in nest.ops[name][0]:
            size *= d
        corners = [offset + r * strides[0] + c * strides[1] for r in (0, rows - 1) for c in (0, cols - 1)]
        if min(corners) < 0 or max(corners) >= size:
            return "%s's view leaves its allocation" % name
    return None


def batch_offset_in_role(nest, batch):
    """Whether a factor's dimension mixes a batch loop (of the decision's
    `batch` loops) with another loop."""
    batch = set(batch)
    many = [trips(*loop) >= 2 for loop in nest.loops]
    return any(any(f[0][s] for s in batch) and
               any(f[0][s] and many[s] and s not in batch for s in range(len(nest.loops)))
               for name in "AB" for f in nest.ops[name][1])


def c_has_batch_terms(nest, batch):
    """Whether a dimension of C mixes a batch loop with another loop (the C
    'purity' the specification dropped on 2026-10-16)."""
    batch = set(batch)
    many = [trips(*loop) >= 2 for loop in nest.loops]
    return any(any(f[0][s] for s in batch) and
               any(f[0][s] and many[s] and s not in batch for s in range(len(nest.loops)))
               for f in nest.ops["C"][1])


def check_batched(nest, got, batch):
    """The batched lowering: the emitted batch loops are the decision's batch
    loops `batch` (those read by all three operands, the demoted ones of
    section 21 and the reduction batch loops of section 23: the same
    iterations -- origin, step, trip count --, same order), and for every
    batch value the views -- dynamic offsets evaluated -- pass check_views on
    that batch iteration's points. The batch values' product is then the
    nest's box, so every iteration runs exactly once, at its own addresses.
    The accumulation of the batch iterations into C is checked on the IR
    by check_serial_accumulation."""
    mnk, views, _, info = got
    loops = info["loops"]
    if len(loops) != len(batch):
        return "%d batch loop(s) emitted for %d" % (len(loops), len(batch))
    for (iv, lb, ub, st), s in zip(loops, batch):
        nlb, nub, nst = nest.loops[s]
        if (lb, st, trips(lb, ub, st)) != (nlb, nst, trips(nlb, nub, nst)):
            return ("emitted batch loop %s = %d to %d step %d does not run the iterations of "
                    "the nest's loop %d (%d to %d step %d)" % (iv, lb, ub, st, s, nlb, nub, nst))
    pts = nest.points()
    for values in itertools.product(*[[lb + st * z for z in range(trips(lb, ub, st))]
                                      for _, lb, ub, st in loops]):
        env = {iv: v for (iv, _, _, _), v in zip(loops, values)}
        sub = [x for x in pts if all(x[s] == v for s, v in zip(batch, values))]
        try:
            concrete = batch_views(views, env, info["applies"])
        except (KeyError, ValueError, TypeError) as e:
            return "batch %s: the views' offsets cannot be evaluated (%s)" % (values, e)
        bad = check_views(nest, mnk, concrete, sub)
        if bad:
            return "batch %s: %s" % (values, bad)
    return None


def check_serial_accumulation(got):
    """The tile's C is read (bufferization.to_tensor of C's view), computed
    and written back (materialize_in_destination of the tile's result into
    that same view) inside every emitted batch loop, in that order: each
    batch iteration starts from C as the previous one left it -- for the
    reduction batch iterations of section 23, which all accumulate into one
    slab, the accumulation in order from C's initial value; a C read before
    the batch loops, or a write-back after them, would lose all but one
    iteration's products. None when it holds."""
    info = got[3]
    loops = tuple(iv for iv, _, _, _ in info["loops"])
    placed, materialized = info["placed"], info["materialized"]
    c_tensor = info["operands"][2]
    if "tile" not in placed or c_tensor not in placed:
        return "the tile or its C operand is not defined in the function body"
    result, tile_at, tile_around = placed["tile"]
    c_view, c_at, c_around = placed[c_tensor]
    if tile_around != loops or c_around != loops:
        return ("the tile (in loops %s) or its C read (in loops %s) is not inside every batch loop %s"
                % (tile_around, c_around, loops))
    if c_at > tile_at:
        return "C is read after the tile"
    back = [(at, around) for src, dst, at, around in materialized if src == result and dst == c_view]
    if len(back) != 1:
        return "the tile's result is written back into C's view %d time(s), not once" % len(back)
    at, around = back[0]
    if around != loops or at < tile_at:
        return "the tile's result is written back into C outside the batch loops %s or before the tile" % (loops,)
    return None


def check_ordered_contributions(nest, got, batch, rb):
    """The lowered program's dataflow into C, simulated from the compiler's
    own views (docs section 25): the emitted batch loops run in their
    emitted order (outermost first); every batch iteration reads its C tile,
    adds its products and writes the tile back before the next one reads C
    (check_serial_accumulation establishes that structure on the IR), so the
    contributions an element of C receives are the blocks of the batch
    iterations that write it, in the emitted order, each block the K
    products of one output of one tile (their order inside a tile is the
    reassociation the permission allows). Checked against the program -- the
    nest's points in lexicographic order, each point one contribution
    {A element, B element} to its C element (an unordered pair: the factors
    are named by memref argument and address, the product commutes):

      * no tile writes an element of C twice (a tile's outputs are distinct
        elements: else both read the same old value and one update is lost);
      * every element of C receives exactly the program's contributions,
        each once (nothing lost, nothing added);
      * across batch iterations of different output batch values (the batch
        loops other than the reduction batch loops `rb`), in the program's
        order: the element's blocks, merged per run of one output batch
        value, are consecutive segments of the program's sequence of its
        contributions, in order. The reduction batch iterations of one slab
        (section 23) and the products inside one tile are reassociated.

    `batch`: the decision's batch loops (the emitted loops run them in that
    order, check_batched). None when it holds."""
    from collections import Counter
    mnk, raw, _, info = got
    M, N, K = mnk
    loops = info["loops"]
    if len(loops) != len(batch):
        return "%d batch loop(s) emitted for %d" % (len(loops), len(batch))

    def address(name, x):
        return (nest.arg_of[name], linear(nest.ops[name][0], [value(f, x) for f in nest.ops[name][1]]))
    program = {}
    for x in nest.points():
        program.setdefault(address("C", x), []).append(tuple(sorted((address("A", x), address("B", x)))))
    lowered = {}
    rb = set(rb)
    for values in itertools.product(*[[lb + st * z for z in range(trips(lb, ub, st))]
                                      for _, lb, ub, st in loops]):
        env = {iv: v for (iv, _, _, _), v in zip(loops, values)}
        outer = tuple(v for s, v in zip(batch, values) if s not in rb)
        try:
            concrete = batch_views([(None, v, sv) for _, v, sv in raw], env, info["applies"])
        except (KeyError, ValueError, TypeError) as e:
            return "batch %s: the views' offsets cannot be evaluated (%s)" % (values, e)
        (_, (_, ast, aof)), (_, (_, bst, bof)), (_, (_, cst, cof)) = concrete
        aarg, barg, carg = raw[0][0], raw[1][0], raw[2][0]
        written = {}
        for r in range(M):
            for c in range(N):
                element = (carg, cof + r * cst[0] + c * cst[1])
                if element in written:
                    return ("batch %s: the tile's outputs %s and %s are one element of C (one update lost)"
                            % (values, written[element], (r, c)))
                written[element] = (r, c)
                block = Counter(tuple(sorted(((aarg, aof + r * ast[0] + k * ast[1]),
                                              (barg, bof + k * bst[0] + c * bst[1])))) for k in range(K))
                runs = lowered.setdefault(element, [])
                if runs and runs[-1][0] == outer:
                    runs[-1][1].update(block)
                else:
                    runs.append((outer, block))
    for element in set(program) | set(lowered):
        sequence, at = program.get(element, []), 0
        for outer, block in lowered.get(element, []):
            n = sum(block.values())
            if Counter(sequence[at:at + n]) != block:
                return ("element %s of C: the contributions of output batch %s are not the next %d of the "
                        "program's, in its order (a lost, extra or reordered update)" % (element, outer, n))
            at += n
        if at != len(sequence):
            return ("element %s of C receives %d of the program's %d contributions"
                    % (element, at, len(sequence)))
    return None


def reduction_once_loop(nest):
    """Whether a loop that runs once is read by both factors (a reduction of
    extent one driven by a loop, as against no reduction loop at all)."""
    n = len(nest.loops)
    return any(trips(*nest.loops[s]) == 1 and
               all(any(f[0][s] for f in nest.ops[name][1]) for name in "AB")
               for s in range(n))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--opt", default="systolic-opt")
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--count", type=int, default=300)
    ap.add_argument("--fp-default", action="store_true",
                    help="run the pass without the reassociation permission: nothing may be lowered")
    ap.add_argument("--no-reduction-batches", action="store_true",
                    help="run the pass with reduction-batches=false and decide without reduction batch "
                         "loops (the recognition of docs sections 19-21 alone)")
    ap.add_argument("--keep", help="write the generated MLIR here")
    a = ap.parse_args()

    rng = random.Random(a.seed)
    nests = [generate(rng) for _ in range(a.count)]
    for i, nest in enumerate(nests):
        decorate_bounds(random.Random("bounds:%d:%d" % (a.seed, i)), nest)
    for i, nest in enumerate(nests):
        dr = random.Random("disguise:%d:%d" % (a.seed, i))
        if dr.random() < 0.15:
            disguise(dr, nest)
    text = "\n// -----\n".join(emit(nest, "case_%d" % i) for i, nest in enumerate(nests))
    if a.keep:
        with open(a.keep, "w") as f:
            f.write(text + "\n")
    rb_on = not a.no_reduction_batches
    results, remarks = compiler_results(a.opt, text, permit=not a.fp_default, reduction_batches=rb_on)
    if len(results) != len(nests):
        print("systolic-opt returned %d functions for %d nests" % (len(results), len(nests)))
        sys.exit(1)
    # each nest's first line in the joined input; a remark belongs to the nest
    # whose lines hold its location (its anchor loop)
    starts, line = [], 1
    for i, nest in enumerate(nests):
        starts.append(line)
        line += emit(nest, "case_%d" % i).count("\n") + 2
    remark_of = {}
    for at, cls, reason in remarks:
        i = max(j for j, first in enumerate(starts) if first <= at)
        remark_of.setdefault(i, (cls, reason))
    triangle_class = {c: 0 for c in ("PROVEN_INVALID", "UNKNOWN", "VALID_BUT_LOWERING_UNSUPPORTED",
                                     "PROVEN_VALID", "not recognized")}

    counts = {"ACCEPT": 0, "REJECT": 0, "SKIP": 0}
    feature = {"transposed": 0, "reversed": 0, "extent_one": 0, "rank_not_2": 0, "once": 0,
               "batched": 0, "batch_offset": 0, "both_transposed": 0, "folded": 0,
               "batch_folded": 0, "role_in_k": 0, "batch_once": 0, "c_batch_terms": 0,
               "two_unit": 0, "general": 0, "c_not_injective": 0,
               # since 2026-10-18 (section 19.12)
               "sheared": 0, "chained": 0, "shear_step": 0, "shear_folded": 0,
               "shared": 0, "two_views": 0, "same_view": 0, "same_view_one_tensor": 0,
               "k1": 0, "k1_noloop": 0,
               "triangle": 0, "alias_c": 0, "shear_oob": 0, "shear_c_not_injective": 0,
               # since the exact domains (section 19.13)
               "disguised": 0, "degenerate_triangle": 0,
               # reduction-batch partitioning (section 23, since 2026-10-21)
               "rb": 0, "rb_one_factor": 0, "rb_shared": 0, "rb_virtual_k1": 0,
               "rb_with_output_batch": 0, "rb_not_box": 0, "rb_strict": 0,
               # ordered output batches (section 25)
               "ob": 0, "ob_whole": 0, "ob_shared_dim": 0, "ob_with_rb": 0, "ob_not_box": 0,
               "overlap": 0, "overlap_prefix": 0, "ob_strict": 0}

    def is_transposed(view):
        shape, strides = view[0], view[1]
        return abs(strides[0]) < abs(strides[1]) and shape[0] > 1 and shape[1] > 1

    def both_factors_transposed(views):
        """A's and B's views both store their roles in the other order."""
        named = dict((nm, v) for nm, v, _ in views)
        return all(nm in named and named[nm] is not None and is_transposed(named[nm])
                   for nm in "AB")

    def count_new_families(emitted, nest, mnk, got):
        """The features of section 19.12 on a lowered, verified nest: the
        domain's as emitted, the GEMM's on the nest it was decided on (its
        box, for a triangle or a box in disguise)."""
        if emitted.domain_mode == "shear":
            feature["sheared"] += 1
            feature["chained"] += emitted.chained()
            feature["shear_step"] += any(emitted.loops[s][2] > 1 for s in emitted.sheared_loops())
            feature["shear_folded"] += has_folded_bound(emitted, set(emitted.sheared_loops()))
        if nest.shared in ("two_views", "same_view"):
            feature["shared"] += 1
            feature[nest.shared] += 1
            if nest.shared == "same_view":
                feature["same_view_one_tensor"] += got[3]["operands"][0] == got[3]["operands"][1]
        if mnk[2] == 1:
            feature["k1"] += 1
            feature["k1_noloop"] += not reduction_once_loop(nest)

    mismatches = []
    shown = {}       # case index -> the nest as emitted, when decided on its box
    for i, (nest, got) in enumerate(zip(nests, results)):
        # A triangle's points are enumerated as emitted: a parallelotope with
        # the documented shift rows (find_parallelotope) is a GEMM decided on
        # its box (derived_box_nest) -- the compiler's exact path must lower
        # it; any other triangle is outside the compiler's domain family and
        # must not be lowered (a conservative refusal, not a mathematical
        # verdict: counted apart, never a pass) -- but a GEMM over a
        # parallelotope repeated by independent repetition loops of any shape
        # (family R) may be, and is SKIPped as decide() SKIPs a repetition
        # loop on a box.
        # A box in disguise is decided the same way, on the parallelotope its
        # points form: when a shear's source runs once, the zero row already
        # fits and the exact path keeps the written coordinates -- the same
        # points and accesses as the original box, another parametrization.
        degenerate = False
        emitted = nest
        shift_rows = None
        if nest.domain_mode == "triangle" or nest.disguised:
            rows = find_parallelotope(nest, emitted_points(nest))
            if rows is None and nest.disguised:
                raise RuntimeError("case_%d: a disguise outside the family was kept" % i)
            if rows is None:
                feature["triangle"] += 1
                if got is not None and repetition_family_nest(nest):
                    counts["SKIP"] += 1
                    continue
                if got is not None:
                    mismatches.append((i, nest, "a triangle (bounds not shifting together) was lowered %s"
                                                % (got[0],)))
                    continue
                cls, bad = check_triangle_class(nest, remark_of.get(i))
                triangle_class[cls] += 1
                if bad:
                    mismatches.append((i, nest, bad))
                continue
            nest = derived_box_nest(nest, rows)
            shift_rows = rows
            degenerate = emitted.domain_mode == "triangle"
            shown[i] = emitted
        # Reduction batches (docs section 23) are made only on the counters as
        # written: not on a nest whose normalization has U != I -- a sheared
        # nest, or a disguise whose parallelotope has a nonzero shift row (no
        # integer basis transformation widens the recognition in Phase 1).
        rewritten = (emitted.domain_mode == "shear" and
                     any(any(r) for r in (emitted.shear or []) if r)) or \
                    (shift_rows is not None and any(any(l) for l, _, _, _ in shift_rows))
        verdict = decide(nest, reduction_batches=rb_on and not rewritten, ordered=not rewritten)
        if nest.shared == "alias_c":
            feature["alias_c"] += 1
            if got is not None:
                mismatches.append((i, nest, "C is a view of a factor's memref; lowered %s" % (got[0],)))
            continue
        counts[verdict[0] if verdict[0] != "UNSUPPORTED" else "REJECT"] += 1
        if verdict[0] == "SKIP":
            continue
        if a.fp_default:
            if got is not None:
                mismatches.append((i, nest, "lowered without the reassociation permission"))
            # a decomposition with reduction batch loops reorders every
            # element's sum: strict floating point refuses it as any other
            feature["rb_strict"] += verdict[0] == "ACCEPT" and bool(verdict[5])
            feature["ob_strict"] += verdict[0] == "ACCEPT" and (bool(verdict[6]) or verdict[7][2])
            continue
        if verdict[0] == "UNSUPPORTED":
            if got is not None:
                mismatches.append((i, nest, "%s: not lowered by the compiler's rules, compiler lowered %s"
                                            % (verdict[1], got[0])))
            feature["c_not_injective"] += 1
            feature["shear_c_not_injective"] += emitted.domain_mode == "shear"
            continue
        if verdict[0] == "ACCEPT":
            mnk = verdict[1]
            if verdict[3] and not GENERAL_VIEWS_LOWERED:
                # a skewed or merged view: a GEMM, recognized, not lowered yet
                if got is not None:
                    mismatches.append((i, nest, "a skewed or merged view was lowered %s (its lowering "
                                                "is not implemented: it must be refused)" % (got[0],)))
                feature["general"] += 1
                continue
            feature["general"] += bool(verdict[3])
            if got is None:
                mismatches.append((i, nest, "oracle ACCEPT %dx%dx%d, compiler did not lower" % mnk))
                continue
            # The factors may be assigned either way: A x B into C, or the
            # transposed problem B^T x A^T into C^T (M and N exchanged). Both
            # are valid; the views of the one the compiler chose are checked.
            if got[0] not in (mnk, (mnk[1], mnk[0], mnk[2])):
                mismatches.append((i, nest, "oracle %s, compiler %s" % (mnk, got[0])))
                continue
            # Two factors on one memref are named by position, and once more
            # the other way round when that fails (name_views).
            bad, named = None, None
            for swap in ([False, True] if nest.shared in ("two_views", "same_view") else [False]):
                named = name_views(nest, got[1], swap)
                if verdict[2]:
                    # batched: the batch loops re-emitted around the tiles, each
                    # operand's view at offsets evaluated per batch value
                    if got[2] != verdict[2]:
                        bad = "batched over %d loop(s), compiler emitted %d affine.for" % (verdict[2], got[2])
                        break
                    bad = check_batched(nest, (got[0], named, got[2], got[3]), verdict[4])
                    if bad:
                        bad = "batched views: " + bad
                    else:
                        bad = check_serial_accumulation(got)
                        if bad:
                            bad = "batched accumulation into C: " + bad
                        else:
                            bad = check_ordered_contributions(nest, got, verdict[4], verdict[5])
                            if bad:
                                bad = "dataflow into C: " + bad
                else:
                    if any(v[1] is None or v[1][2] is None for v in named):
                        bad = "a view of a plain GEMM has a dynamic offset"
                        break
                    bad = check_views(nest, got[0], [(nm, v) for nm, v, _ in named])
                    if bad:
                        bad = "views: " + bad
                    else:
                        bad = check_ordered_contributions(nest, got, [], [])
                        if bad:
                            bad = "dataflow into C: " + bad
                if bad is None:
                    break
            if bad:
                mismatches.append((i, nest, bad))
                continue
            count_new_families(emitted, nest, mnk, got)
            feature["degenerate_triangle"] += degenerate
            feature["disguised"] += emitted.disguised
            if verdict[5]:
                one, shared = reduction_loops(nest)
                feature["rb"] += 1
                feature["rb_one_factor"] += bool(set(one) & set(verdict[5]))
                feature["rb_shared"] += bool(set(shared) & set(verdict[5]))
                feature["rb_virtual_k1"] += not shared
                feature["rb_with_output_batch"] += len(verdict[4]) > len(verdict[5])
                feature["rb_not_box"] += emitted.domain_mode != "box" or emitted.disguised
            if verdict[6] or verdict[7][2]:
                # section 25: lowered only with the ordered output batches
                feature["ob"] += 1
                feature["ob_whole"] += bool(verdict[6])
                feature["ob_shared_dim"] += not verdict[6]
                feature["ob_with_rb"] += bool(verdict[5])
                feature["ob_not_box"] += emitted.domain_mode != "box" or emitted.disguised
            feature["overlap"] += verdict[7][0]
            feature["overlap_prefix"] += verdict[7][0] and verdict[7][1]
            if verdict[2]:
                feature["batched"] += 1
                feature["c_batch_terms"] += c_has_batch_terms(nest, verdict[4])
                feature["two_unit"] += sum(e == 1 for e in mnk) >= 2
                feature["batch_offset"] += batch_offset_in_role(nest, verdict[4])
                feature["both_transposed"] += both_factors_transposed(named)
                feature["folded"] += has_folded_bound(emitted)
                feature["batch_folded"] += has_folded_bound(emitted, set(verdict[4]))
                feature["role_in_k"] += emitted.mode == "mutated:role_in_k"
                continue
            views = dict((nm, v) for nm, v, _ in named)
            feature["transposed"] += any(abs(v[1][0]) < abs(v[1][1]) and v[0][0] > 1 and v[0][1] > 1 for v in views.values())
            feature["both_transposed"] += both_factors_transposed(named)
            feature["reversed"] += any(s < 0 for v in views.values() for s, n in zip(v[1], v[0]) if n > 1)
            feature["extent_one"] += 1 in mnk
            feature["rank_not_2"] += any(len(nest.ops[k][0]) != 2 for k in "ABC")
            feature["once"] += any(trips(*l) == 1 for l in nest.loops)
            feature["folded"] += has_folded_bound(emitted)
            feature["role_in_k"] += emitted.mode == "mutated:role_in_k"
            feature["batch_once"] += emitted.mode == "mutated:batch_once"
        else:
            if emitted.domain_mode == "shear" and "out of bounds" in verdict[1]:
                feature["shear_oob"] += 1
            if got is not None:
                mismatches.append((i, nest, "oracle REJECT (%s), compiler lowered %s" % (verdict[1], got[0])))

    for i, nest, what in mismatches:
        nest = shown.get(i, nest)
        print("MISMATCH case_%d [%s%s%s%s]: %s" % (i, nest.mode,
                                                  ", " + nest.domain_mode if nest.domain_mode != "box" else "",
                                                  ", " + nest.shared if nest.shared else "",
                                                  ", disguised" if nest.disguised else "", what))
        print(emit(nest, "case_%d" % i))
    print("oracle: %d nests, accept %d, reject %d, skipped %d, refused by rule %d (triangles %d, "
          "C aliasing a factor %d)%s"
          % (len(nests), counts["ACCEPT"], counts["REJECT"], counts["SKIP"],
             feature["triangle"] + feature["alias_c"], feature["triangle"], feature["alias_c"],
             " (pass run without the reassociation permission)" if a.fp_default else ""))
    if not a.fp_default:
        print("accepted with: transposed view %(transposed)d, negative stride %(reversed)d, "
              "M/N/K = 1 %(extent_one)d, an operand of rank != 2 %(rank_not_2)d, "
              "a loop that runs once %(once)d, batched %(batched)d "
              "(a factor's view moving with the batch %(batch_offset)d), "
              "A and B both transposed %(both_transposed)d, "
              "a max/min loop bound %(folded)d (of a batch loop %(batch_folded)d), "
              "mutations role_in_k %(role_in_k)d and batch_once %(batch_once)d, "
              "a sheared domain %(sheared)d (chained %(chained)d, with a step > 1 %(shear_step)d, "
              "with a max/min bound %(shear_folded)d), one array as both factors %(shared)d "
              "(through two different views %(two_views)d, through one view %(same_view)d, "
              "served by one tensor %(same_view_one_tensor)d), "
              "K = 1 %(k1)d (without a reduction loop %(k1_noloop)d), "
              "a box in disguise %(disguised)d, a degenerate triangle %(degenerate_triangle)d" % feature)
        print("generalized mapping: batched with C reading a batch loop in a role dimension "
              "%(c_batch_terms)d, batched with two unit extents %(two_unit)d, skewed or merged "
              "views (%(general_state)s) %(general)d, C not injective (refused) "
              "%(c_not_injective)d, domain outside the normalized family (refused) %(triangle)d, "
              "C the base of a factor (refused) %(alias_c)d, out of bounds after a shear "
              "(rejected) %(shear_oob)d, C not injective after a shear (refused) "
              "%(shear_c_not_injective)d" % dict(feature, general_state="lowered" if GENERAL_VIEWS_LOWERED
                                                   else "recognized, not lowered"))
        print("reduction-batch partitioning%s: lowered %d (with a loop read by one factor only %d, "
              "K chosen among the loops both factors read %d, virtual K = 1 %d, with an output batch "
              "too %d, on a sheared or disguised domain %d)"
              % ("" if rb_on else " (off)", feature["rb"], feature["rb_one_factor"], feature["rb_shared"],
                 feature["rb_virtual_k1"], feature["rb_with_output_batch"], feature["rb_not_box"]))
        print("ordered output batches: lowered %d (an output role made batch loops %d, a demoted loop sharing "
              "a dimension of C with a row or column coordinate %d, with reduction batch loops too %d, on a "
              "triangle or disguised domain %d); lowered with batch iterations writing overlapping elements of C "
              "%d (an outermost prefix %d), each checked contribution by contribution"
              % (feature["ob"], feature["ob_whole"], feature["ob_shared_dim"], feature["ob_with_rb"],
                 feature["ob_not_box"], feature["overlap"], feature["overlap_prefix"]))
    else:
        print("not lowered (strict mode): %d outside the normalized family, %d with C the base of a factor, "
              "%d with reduction batch loops, %d with ordered output batches"
              % (feature["triangle"], feature["alias_c"], feature["rb_strict"], feature["ob_strict"]))
    print("refused triangles by class: PROVEN_INVALID %(PROVEN_INVALID)d (each checked: not centrally "
          "symmetric with the stated witness, the stacked access map of full rank, three memrefs), "
          "UNKNOWN %(UNKNOWN)d, "
          "VALID_BUT_LOWERING_UNSUPPORTED %(VALID_BUT_LOWERING_UNSUPPORTED)d, PROVEN_VALID "
          "%(PROVEN_VALID)d, not recognized %(not recognized)d" % triangle_class)
    print("mismatches: %d" % len(mismatches))


if __name__ == "__main__":
    main()
