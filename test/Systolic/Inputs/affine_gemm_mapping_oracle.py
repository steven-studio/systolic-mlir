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

Batched nests (ISSUE-006, ISSUE-007): the batch loops are, by definition,
the loops of two or more iterations that all three operands read. C must
read them alone -- no dimension of C mixes a batch loop with another loop,
no batch loop in two dimensions of C (the coupled, skewed and diagonal
outputs the specification rejects); a batch coordinate may fill several
dimensions of C; the factors may read them anywhere,
each through its own expression, as a dimension of their own or as an
offset of a role's dimension (A[4i - j][5k]). Everything else is decided by
enumeration, per batch iteration: the points with one value of the batch
loops must form a GEMM of strided views exactly as above, every batch
iteration the same shape (extents and roles). For a lowered batched nest
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
                       "batch_offset_c", "role_in_k", "batch_once"])
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
        # a batch loop added to a role's dimension: of a factor -- valid, the
        # factor's view moves with the batch (ISSUE-007) -- or of C -- the
        # output position moves with the batch: rejected
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

    n = len(nest.loops)
    many = [trips(*loop) >= 2 for loop in nest.loops]
    reads = {name: {s for f in nest.ops[name][1] for s in range(n) if f[0][s] != 0 and many[s]}
             for name in "ABC"}
    batch_loops = sorted(reads["A"] & reads["B"] & reads["C"])
    if not batch_loops:
        verdict = gemm_on_points(nest, pts)
        return verdict if verdict[0] != "ACCEPT" else ("ACCEPT", verdict[1], 0)
    # C reads every batch loop alone: no dimension of C mixes a batch loop
    # with another loop (a batch coordinate may fill several dimensions).
    for d, f in enumerate(nest.ops["C"][1]):
        if any(f[0][s] != 0 for s in batch_loops) and \
                any(f[0][s] != 0 and many[s] and s not in batch_loops for s in range(n)):
            return ("REJECT", "C's dimension %d mixes a batch coordinate with another coordinate" % d)
    # Per batch iteration: a GEMM of strided views, every iteration the same.
    groups = {}
    for x in pts:
        groups.setdefault(tuple(x[s] for s in batch_loops), []).append(x)
    shape = None
    for sub in groups.values():
        verdict = gemm_on_points(nest, sub)
        if verdict[0] != "ACCEPT":
            return (verdict[0], verdict[1] + " (within a batch iteration)")
        if not verdict[2]:
            return ("REJECT", "an element-wise product (every coordinate a batch one)")
        if shape is None:
            shape = verdict[1:]
        elif verdict[1:] != shape:
            return ("REJECT", "the batch iterations are not one GEMM shape")
    ext = shape[0]
    # a batched vector-times-scalar (two of M, N, K are 1): valid, not
    # lowered pending spec question SQ-5
    if sum(1 for e in ext if e == 1) >= 2:
        return ("UNSUPPORTED_BATCH", "SQ-5: two of M, N, K are 1 per batch")
    return ("ACCEPT", ext, len(batch_loops))


def gemm_on_points(nest, pts):
    """The enumeration on a set of points (the whole domain, or one batch
    iteration): ("ACCEPT", (M, N, K), roles) or ("REJECT", why). `roles`
    names each role's (operand, dimension, step) readers -- empty when no
    subscript varies on the points."""
    # Rank functions; coordinates = classes of rank functions up to mirroring.
    readers = []        # (operand, dim, ranks, extent, values, step)
    for name in "ABC":
        for d, f in enumerate(nest.ops[name][1]):
            vals = [value(f, x) for x in pts]
            distinct = sorted(set(vals))
            if len(distinct) == 1:
                continue                                   # a constant dimension
            step = distinct[1] - distinct[0]
            if any(b - a != step for a, b in zip(distinct, distinct[1:])):
                return ("REJECT", "%s's dimension %d is not an arithmetic progression (no strided view)" % (name, d))
            readers.append((name, d, tuple((v - distinct[0]) // step for v in vals), len(distinct),
                            tuple(vals), step))
    classes = []
    for rd in readers:
        for cl in classes:
            r0 = cl[0]
            if r0[3] == rd[3] and (r0[2] == rd[2] or r0[2] == tuple(r0[3] - 1 - v for v in rd[2])):
                cl.append(rd)
                break
        else:
            classes.append([rd])
    for first in (nest.order, nest.order[::-1]):
        a, b = first
        role = {}
        ok = True
        for cl in classes:
            names = sorted(rd[0] for rd in cl)
            if len(set(names)) != len(names):
                ok = False          # one operand reads one coordinate twice
                break
            ops_in = set(names)
            if ops_in == {a, "C"}:
                key = "I"
            elif ops_in == {b, "C"}:
                key = "J"
            elif ops_in == {a, b}:
                key = "K"
            else:
                key = None          # read by one operand (a broadcast) or all three
            if key is None or key in role:
                ok = False
                break
            role[key] = cl
        if not ok:
            continue
        ext = {r: (role[r][0][3] if r in role else 1) for r in "IJK"}
        ordinal = {r: (role[r][0][2] if r in role else tuple(0 for _ in pts)) for r in "IJK"}
        box = ext["I"] * ext["J"] * ext["K"]
        if len({(ordinal["I"][i], ordinal["J"][i], ordinal["K"][i]) for i in range(len(pts))}) == len(pts) == box:
            roles = tuple((r, tuple(sorted((rd[0], rd[1], rd[5]) for rd in role[r])))
                          for r in "IJK" if r in role)
            return ("ACCEPT", (ext["I"], ext["J"], ext["K"]), roles)
    return ("REJECT", "no mapping onto a GEMM of strided views")


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

DEF = re.compile(r"^\s*(%[\w.]+) = (?:memref\.subview|memref\.transpose|memref\.expand_shape) (%[\w.]+)")
TO_TENSOR = re.compile(r"^\s*(%[\w.]+) = bufferization\.to_tensor (%[\w.]+) .*: (memref<[^>]*(?:<[^>]*>[^>]*)?>) to tensor")
TILE = re.compile(r"systolic\.matmul_tile (%[\w.]+), (%[\w.]+), (%[\w.]+) m = (\d+) n = (\d+) k = (\d+)")
MEMREF = re.compile(r"memref<([\dx]+)xf32(?:, strided<\[([-\d, ]+)\](?:, offset: (-?\d+|\?))?>)?>")
MAPDEF = re.compile(r"^(#[\w$.]+) = affine_map<\(([^)]*)\) -> \((.*)\)>\s*$")
APPLY = re.compile(r"^\s*(%[\w.]+) = affine\.apply (#[\w$.]+|affine_map<\(([^)]*)\) -> \((.*?)\)>)\(([^)]*)\)")
FORLOOP = re.compile(r"^\s*affine\.for (%[\w.]+) = (-?\d+) to (-?\d+)(?: step (\d+))? \{")
SUBVIEW = re.compile(r"^\s*(%[\w.]+) = memref\.subview (%[\w.]+)\[([^\]]*)\] \[([^\]]*)\] \[([^\]]*)\] : memref<([\dx]+)xf32>")


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
                subviews[m.group(1)] = ([v.strip() for v in m.group(3).split(",")],
                                        [int(v) for v in m.group(6).split("x")])
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
    its subview's offsets and the base memref's row-major strides."""
    out = []
    for name, view, subview in views:
        shape, strides, offset = view
        if offset is None:
            offsets, base = subview
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

    def locate(name, shape, strides, offset, rows, cols):
        table = {}
        for r in range(rows):
            for c in range(cols):
                table.setdefault(offset + r * strides[0] + c * strides[1], []).append((r, c))
        base = nest.ops[name][0]
        out = []
        for x in pts:
            hits = table.get(linear(base, [value(f, x) for f in nest.ops[name][1]]), [])
            if len(hits) != 1:
                return None
            out.append(hits[0])
        return out

    la = locate(an, ash, ast, aof, M, K)
    lb = locate(bn, bsh, bst, bof, K, N)
    lc = locate("C", csh, cst, cof, M, N)
    if la is None or lb is None or lc is None:
        return "an access is not exactly one element of its operand's view"
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
               "batch_folded": 0, "role_in_k": 0, "batch_once": 0}

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
        counts[verdict[0] if verdict[0] != "UNSUPPORTED_BATCH" else "REJECT"] += 1
        if verdict[0] == "SKIP":
            continue
        if a.fp_default:
            if got is not None:
                mismatches.append((i, nest, "lowered without the reassociation permission"))
            continue
        if verdict[0] == "UNSUPPORTED_BATCH":
            if got is not None:
                mismatches.append((i, nest, "%s: not lowered by the compiler's rules, compiler lowered %s"
                                            % (verdict[1], got[0])))
            continue
        if verdict[0] == "ACCEPT":
            mnk = verdict[1]
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
    print("mismatches: %d" % len(mismatches))


if __name__ == "__main__":
    main()
