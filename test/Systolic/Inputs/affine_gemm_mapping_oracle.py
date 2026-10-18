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

With --fp-default the nests go through the pass WITHOUT the reassociation
permission, and nothing may be lowered (section 6).

Prints every mismatch and a summary line.
"""

import argparse
import itertools
import random
import re
import subprocess
import sys
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
    def __init__(self, loops, ops, cload, order, mode):
        self.loops = loops          # [(lb, ub, step)]
        self.ops = ops              # {"A"|"B"|"C": (shape, [f, ...])}
        self.cload = cload          # [f, ...] the accumulator load's subscripts
        self.order = order          # ("A", "B") or ("B", "A"): the product's factor order
        self.mode = mode
        self.bounds = None          # per loop ([lowers], [uppers]) as emitted, or None: lb, ub

    def points(self):
        return list(itertools.product(*[[lb + st * z for z in range(trips(lb, ub, st))]
                                        for lb, ub, st in self.loops]))


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


def generate(rng):
    loops, trip, ops = generate_valid(rng)
    r = rng.random()
    if r < 0.45:
        return finalize(rng, loops, ops, "valid")
    if r < 0.55:
        return finalize(rng, loops, ops, "oob", oob=True)
    kind, cload = mutate(rng, loops, trip, ops)
    return finalize(rng, loops, ops, "mutated:" + kind, cload=cload)


# ---------------------------------------------------------------------------
# The decision, by enumeration.

def decide(nest):
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

    # The roles by readers (docs/AFFINE_GEMM_MAPPING.md section 19.2, the
    # definition): a loop of two or more iterations read by all three
    # operands is a batch loop; by A and C, I; by B and C, J; by A and B, K;
    # by one operand, a broadcast. C may read batch loops anywhere (no C
    # purity since 2026-10-16); what C must be is injective per batch (G2).
    n = len(nest.loops)
    many = [trips(*loop) >= 2 for loop in nest.loops]
    reads = {name: {s for f in nest.ops[name][1] for s in range(n) if f[0][s] != 0 and many[s]}
             for name in "ABC"}
    batch_loops = sorted(reads["A"] & reads["B"] & reads["C"])
    role_loops = {"I": sorted((reads["A"] & reads["C"]) - set(batch_loops)),
                  "J": sorted((reads["B"] & reads["C"]) - set(batch_loops)),
                  "K": sorted((reads["A"] & reads["B"]) - set(batch_loops))}
    for s in range(n):
        if many[s] and s not in batch_loops and not any(s in l for l in role_loops.values()):
            return ("REJECT", "loop %d is read by one operand only (a broadcast)" % s)
    if batch_loops and not any(role_loops.values()):
        return ("REJECT", "an element-wise product (every coordinate a batch one)")
    # Every subscript's restriction to a role's loops: proportional to one
    # direction (one coordinate per role; SQ-1 and separate coordinates fail).
    directions = {}
    for role, loops in role_loops.items():
        if not loops:
            continue
        forms = [[f[0][s] for s in loops] for name in "ABC" for f in nest.ops[name][1]]
        nonzero = [f for f in forms if any(f)]
        if any(a[s] * b[t] != a[t] * b[s] for a in nonzero for b in nonzero
               for s in range(len(loops)) for t in range(len(loops))):
            return ("REJECT", "role %s is read through two different combinations of its loops "
                    "(SQ-1 or separate coordinates)" % role)
        directions[role] = nonzero[0]
    # Per batch iteration: a GEMM of affine views, every iteration the same.
    groups = {}
    for x in pts:
        groups.setdefault(tuple(x[s] for s in batch_loops), []).append(x)
    shape, general = None, False
    for sub in groups.values():
        verdict = gemm_on_points(nest, sub, role_loops, directions)
        if verdict[0] != "ACCEPT":
            return (verdict[0], verdict[1] + (" (within a batch iteration)" if batch_loops else ""))
        if shape is None:
            shape = verdict[1]
        elif verdict[1] != shape:
            return ("REJECT", "the batch iterations are not one GEMM shape")
        general = general or verdict[2]
    return ("ACCEPT", shape, len(batch_loops), general)


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
    ivs = ["%%x%d" % s for s in range(len(nest.loops))]
    ty = {k: mtype(nest.ops[k][0]) for k in "ABC"}
    out = ["func.func @%s(%%A: %s {llvm.noalias}, %%B: %s {llvm.noalias}, %%C: %s {llvm.noalias}) {"
           % (name, ty["A"], ty["B"], ty["C"])]
    ind = "  "
    def bound(kind, values):
        if len(values) == 1:
            return str(values[0])
        return "%s affine_map<() -> (%s)>()" % (kind, ", ".join(str(v) for v in values))
    for s, ((lb, ub, st), iv) in enumerate(zip(nest.loops, ivs)):
        lowers, uppers = nest.bounds[s] if nest.bounds else ([lb], [ub])
        assert max(lowers) == lb and min(uppers) == ub
        out.append("%saffine.for %s = %s to %s%s {" % (ind, iv, bound("max", lowers),
                                                       bound("min", uppers),
                                                       "" if st == 1 else " step %d" % st))
        ind += "  "
    idx = lambda fs: ", ".join(expr(f, ivs) for f in fs)
    for k in "AB":
        out.append("%s%%%s = affine.load %%%s[%s] : %s" % (ind, k.lower(), k, idx(nest.ops[k][1]), ty[k]))
    out.append("%s%%c = affine.load %%C[%s] : %s" % (ind, idx(nest.cload), ty["C"]))
    out.append("%s%%p = arith.mulf %%%s, %%%s : f32" % (ind, nest.order[0].lower(), nest.order[1].lower()))
    out.append("%s%%s = arith.addf %%c, %%p : f32" % ind)
    out.append("%saffine.store %%s, %%C[%s] : %s" % (ind, idx(nest.ops["C"][1]), ty["C"]))
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


def compiler_results(opt, text, permit):
    option = "--affine-matmul-to-systolic" + ("=allow-fp-reassociation=true" if permit else "")
    run = subprocess.run([opt, option, "-split-input-file", "-"], input=text,
                         capture_output=True, text=True)
    if run.returncode != 0:
        sys.stdout.write("systolic-opt failed (rc %d):\n%s\n" % (run.returncode, run.stderr[-4000:]))
        sys.exit(1)
    results = []
    for chunk in run.stdout.split("// -----"):
        if "func.func" not in chunk:
            continue
        tile = TILE.search(chunk)
        if not tile:
            results.append(None)
            continue
        source, tensors, subviews, maps, applies, batch_loops = {}, {}, {}, {}, {}, []
        for line in chunk.splitlines():
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
        views = []
        for t in tile.groups()[:3]:
            root, view, subview = tensors[t]
            views.append(("ABC"[int(root[len("%arg"):])], view, subview))
        results.append((tuple(int(v) for v in tile.groups()[3:]), views,
                        len(batch_loops), {"loops": batch_loops, "applies": applies}))
    return results


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


def batch_loops_of(nest):
    n = len(nest.loops)
    many = [trips(*loop) >= 2 for loop in nest.loops]
    reads = [{s for f in nest.ops[name][1] for s in range(n) if f[0][s] != 0 and many[s]}
             for name in "ABC"]
    return sorted(reads[0] & reads[1] & reads[2])


def batch_offset_in_role(nest):
    """Whether a factor's dimension mixes a batch loop with another loop."""
    batch = set(batch_loops_of(nest))
    many = [trips(*loop) >= 2 for loop in nest.loops]
    return any(any(f[0][s] for s in batch) and
               any(f[0][s] and many[s] and s not in batch for s in range(len(nest.loops)))
               for name in "AB" for f in nest.ops[name][1])


def c_has_batch_terms(nest):
    """Whether a dimension of C mixes a batch loop with another loop (the C
    'purity' the specification dropped on 2026-10-16)."""
    batch = set(batch_loops_of(nest))
    many = [trips(*loop) >= 2 for loop in nest.loops]
    return any(any(f[0][s] for s in batch) and
               any(f[0][s] and many[s] and s not in batch for s in range(len(nest.loops)))
               for f in nest.ops["C"][1])


def check_batched(nest, got):
    """The batched lowering: the emitted batch loops are the nest's batch
    loops (the same iterations -- origin, step, trip count --, same order),
    and for every batch value the views -- dynamic offsets evaluated -- pass
    check_views on that batch iteration's points."""
    mnk, views, _, info = got
    batch = batch_loops_of(nest)
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


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--opt", default="systolic-opt")
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--count", type=int, default=300)
    ap.add_argument("--fp-default", action="store_true",
                    help="run the pass without the reassociation permission: nothing may be lowered")
    ap.add_argument("--keep", help="write the generated MLIR here")
    a = ap.parse_args()

    rng = random.Random(a.seed)
    nests = [generate(rng) for _ in range(a.count)]
    for i, nest in enumerate(nests):
        decorate_bounds(random.Random("bounds:%d:%d" % (a.seed, i)), nest)
    text = "\n// -----\n".join(emit(nest, "case_%d" % i) for i, nest in enumerate(nests))
    if a.keep:
        with open(a.keep, "w") as f:
            f.write(text + "\n")
    results = compiler_results(a.opt, text, permit=not a.fp_default)
    if len(results) != len(nests):
        print("systolic-opt returned %d functions for %d nests" % (len(results), len(nests)))
        sys.exit(1)

    counts = {"ACCEPT": 0, "REJECT": 0, "SKIP": 0}
    feature = {"transposed": 0, "reversed": 0, "extent_one": 0, "rank_not_2": 0, "once": 0,
               "batched": 0, "batch_offset": 0, "both_transposed": 0, "folded": 0,
               "batch_folded": 0, "role_in_k": 0, "batch_once": 0, "c_batch_terms": 0,
               "two_unit": 0, "general": 0, "c_not_injective": 0}

    def is_transposed(view):
        shape, strides = view[0], view[1]
        return abs(strides[0]) < abs(strides[1]) and shape[0] > 1 and shape[1] > 1

    def both_factors_transposed(views):
        """A's and B's views both store their roles in the other order."""
        named = dict((nm, v) for nm, v, _ in views)
        return all(nm in named and named[nm] is not None and is_transposed(named[nm])
                   for nm in "AB")
    mismatches = []
    for i, (nest, got) in enumerate(zip(nests, results)):
        verdict = decide(nest)
        counts[verdict[0] if verdict[0] != "UNSUPPORTED" else "REJECT"] += 1
        if verdict[0] == "SKIP":
            continue
        if a.fp_default:
            if got is not None:
                mismatches.append((i, nest, "lowered without the reassociation permission"))
            continue
        if verdict[0] == "UNSUPPORTED":
            if got is not None:
                mismatches.append((i, nest, "%s: not lowered by the compiler's rules, compiler lowered %s"
                                            % (verdict[1], got[0])))
            feature["c_not_injective"] += 1
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
            if verdict[2]:
                # batched: the batch loops re-emitted around the tiles, each
                # operand's view at offsets evaluated per batch value
                if got[2] != verdict[2]:
                    mismatches.append((i, nest, "batched over %d loop(s), compiler emitted %d affine.for"
                                                % (verdict[2], got[2])))
                    continue
                bad = check_batched(nest, got)
                if bad:
                    mismatches.append((i, nest, "batched views: " + bad))
                    continue
                feature["batched"] += 1
                feature["c_batch_terms"] += c_has_batch_terms(nest)
                feature["two_unit"] += sum(e == 1 for e in mnk) >= 2
                feature["batch_offset"] += batch_offset_in_role(nest)
                feature["both_transposed"] += both_factors_transposed(got[1])
                feature["folded"] += has_folded_bound(nest)
                feature["batch_folded"] += has_folded_bound(nest, set(batch_loops_of(nest)))
                feature["role_in_k"] += nest.mode == "mutated:role_in_k"
                continue
            if any(v[1] is None or v[1][2] is None for v in got[1]):
                mismatches.append((i, nest, "a view of a plain GEMM has a dynamic offset"))
                continue
            bad = check_views(nest, got[0], [(nm, v) for nm, v, _ in got[1]])
            if bad:
                mismatches.append((i, nest, "views: " + bad))
                continue
            views = dict((nm, v) for nm, v, _ in got[1])
            feature["transposed"] += any(abs(v[1][0]) < abs(v[1][1]) and v[0][0] > 1 and v[0][1] > 1 for v in views.values())
            feature["both_transposed"] += both_factors_transposed(got[1])
            feature["reversed"] += any(s < 0 for v in views.values() for s, n in zip(v[1], v[0]) if n > 1)
            feature["extent_one"] += 1 in mnk
            feature["rank_not_2"] += any(len(nest.ops[k][0]) != 2 for k in "ABC")
            feature["once"] += any(trips(*l) == 1 for l in nest.loops)
            feature["folded"] += has_folded_bound(nest)
            feature["role_in_k"] += nest.mode == "mutated:role_in_k"
            feature["batch_once"] += nest.mode == "mutated:batch_once"
        elif got is not None:
            mismatches.append((i, nest, "oracle REJECT (%s), compiler lowered %s" % (verdict[1], got[0])))

    for i, nest, what in mismatches:
        print("MISMATCH case_%d [%s]: %s" % (i, nest.mode, what))
        print(emit(nest, "case_%d" % i))
    print("oracle: %d nests, accept %d, reject %d, skipped %d%s"
          % (len(nests), counts["ACCEPT"], counts["REJECT"], counts["SKIP"],
             " (pass run without the reassociation permission)" if a.fp_default else ""))
    if not a.fp_default:
        print("accepted with: transposed view %(transposed)d, negative stride %(reversed)d, "
              "M/N/K = 1 %(extent_one)d, an operand of rank != 2 %(rank_not_2)d, "
              "a loop that runs once %(once)d, batched %(batched)d "
              "(a factor's view moving with the batch %(batch_offset)d), "
              "A and B both transposed %(both_transposed)d, "
              "a max/min loop bound %(folded)d (of a batch loop %(batch_folded)d), "
              "mutations role_in_k %(role_in_k)d and batch_once %(batch_once)d" % feature)
        print("generalized mapping: batched with C reading a batch loop in a role dimension "
              "%(c_batch_terms)d, batched with two unit extents %(two_unit)d, skewed or merged "
              "views (%(general_state)s) %(general)d, C not injective (refused) "
              "%(c_not_injective)d" % dict(feature, general_state="lowered" if GENERAL_VIEWS_LOWERED
                                                   else "recognized, not lowered"))
    print("mismatches: %d" % len(mismatches))


if __name__ == "__main__":
    main()
