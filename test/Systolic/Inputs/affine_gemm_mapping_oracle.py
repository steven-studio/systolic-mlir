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

Batched nests (a coordinate read by all three operands, ISSUE-006): the
oracle decides them by enumeration too -- the three batch subscripts must take
the same value at every point, the batch loops (the loops those subscripts
use) must drive no I, J or K, and within every batch iteration (I, J, K) must
be a bijection onto the box. The compiler re-emits the batch loops around the
tiles with the operands' batch dimensions at a dynamic offset, which this
script's static view parser does not read: for a batched nest it checks the
lowering (the tile, its extents, one affine.for per batch loop) and leaves the
addresses to the end-to-end tests (test/SystolicCC/affine_batched.c, the
showcase and the generated corpus, which run the kernels). A batch coordinate
read through different expressions (A[b + 1] with C[b]) is valid but not
lowered; the oracle checks that it is left alone.

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
                       "add_const_loop", "drop_dim"])
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
        # not lowered
        shared = [f for f in ops["C"] if f in ops["A"] and f in ops["B"] and
                  any(f is g for g in ops["A"]) and any(f is g for g in ops["B"])]
        if shared:
            f = shared[0]
            name = rng.choice("AB")
            ops[name][[id(g) for g in ops[name]].index(id(f))] = [list(f[0]), None]
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

    # Rank functions; coordinates = classes of rank functions up to mirroring.
    readers = []        # (operand, dim, ranks, extent, values, subscript)
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
                            tuple(vals), f))
    classes = []
    for rd in readers:
        for cl in classes:
            r0 = cl[0]
            if r0[3] == rd[3] and (r0[2] == rd[2] or r0[2] == tuple(r0[3] - 1 - v for v in rd[2])):
                cl.append(rd)
                break
        else:
            classes.append([rd])
    # Batch coordinates: a class read by all three operands (ISSUE-006). The
    # three subscripts must have the same value at every point (else the
    # compiler leaves the nest alone: a per-operand view of a batch
    # coordinate); the batch loops are the loops those subscripts use.
    batch, rest, batch_loops = [], [], set()
    for cl in classes:
        names = [rd[0] for rd in cl]
        if sorted(names) == ["A", "B", "C"]:
            if len({rd[4] for rd in cl}) != 1:
                return ("UNSUPPORTED_BATCH", "a batch coordinate read through different expressions")
            batch.append(cl)
            # the batch loops: two or more iterations (a loop that runs once
            # is a constant on the domain, folded into the batch index)
            batch_loops |= {s for s in range(n)
                            if cl[0][5][0][s] != 0 and trips(*nest.loops[s]) >= 2}
        else:
            rest.append(cl)
    if batch and not rest:
        return ("REJECT", "an element-wise product (every coordinate a batch one)")
    batch_key = [tuple(x[s] for s in sorted(batch_loops)) for x in pts]
    for first in (nest.order, nest.order[::-1]):
        a, b = first
        role = {}
        ok = True
        for cl in rest:
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
                key = None
            if key is None or key in role:
                ok = False
                break
            role[key] = cl
        if not ok:
            continue
        ext = {r: (role[r][0][3] if r in role else 1) for r in "IJK"}
        ordinal = {r: (role[r][0][2] if r in role else tuple(0 for _ in pts)) for r in "IJK"}
        # within every batch iteration, (I, J, K) is a bijection onto the box
        groups = {}
        for i, key in enumerate(batch_key):
            groups.setdefault(key, []).append(i)
        box = ext["I"] * ext["J"] * ext["K"]
        if all(len({(ordinal["I"][i], ordinal["J"][i], ordinal["K"][i]) for i in g}) == len(g) == box
               for g in groups.values()):
            # a batched vector-times-scalar (two of M, N, K are 1): valid, not
            # lowered pending spec question SQ-5
            if batch and sum(1 for r in "IJK" if ext[r] == 1) >= 2:
                return ("UNSUPPORTED_BATCH", "SQ-5: two of M, N, K are 1 per batch")
            return ("ACCEPT", (ext["I"], ext["J"], ext["K"]), len(batch_loops))
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
    for (lb, ub, st), iv in zip(nest.loops, ivs):
        out.append("%saffine.for %s = %d to %d%s {" % (ind, iv, lb, ub, "" if st == 1 else " step %d" % st))
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
MEMREF = re.compile(r"memref<([\dx]+)xf32(?:, strided<\[([-\d, ]+)\](?:, offset: (-?\d+))?>)?>")


def parse_view(type_text):
    m = MEMREF.match(type_text)
    if m is None:
        return None             # a dynamic offset (a batched nest): not parsed
    shape = [int(v) for v in m.group(1).split("x")]
    if m.group(2):
        strides = [int(v) for v in m.group(2).split(",")]
        offset = int(m.group(3) or 0)
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
        source, tensors = {}, {}
        for line in chunk.splitlines():
            m = DEF.match(line)
            if m:
                source[m.group(1)] = m.group(2)
            m = TO_TENSOR.match(line)
            if m:
                root = m.group(2)
                while root in source:
                    root = source[root]
                tensors[m.group(1)] = (root, parse_view(m.group(3)))
        views = []
        for t in tile.groups()[:3]:
            root, view = tensors[t]
            views.append(("ABC"[int(root[len("%arg"):])], view))
        loops = sum(1 for line in chunk.splitlines() if "affine.for " in line)
        results.append((tuple(int(v) for v in tile.groups()[3:]), views, loops))
    return results


def linear(shape, index):
    address = 0
    for d, i in zip(shape, index):
        address = address * d + i
    return address


def check_views(nest, mnk, views):
    """(V) and (B) on the compiler's own views; None when they hold."""
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
               "batched": 0}
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
                # batched: the batch loops re-emitted around the tiles; the
                # views carry dynamic offsets (checked end to end, not here)
                if got[2] != verdict[2]:
                    mismatches.append((i, nest, "batched over %d loop(s), compiler emitted %d affine.for"
                                                % (verdict[2], got[2])))
                    continue
                feature["batched"] += 1
                continue
            bad = check_views(nest, got[0], got[1])
            if bad:
                mismatches.append((i, nest, "views: " + bad))
                continue
            views = dict((nm, v) for nm, v in got[1])
            feature["transposed"] += any(abs(v[1][0]) < abs(v[1][1]) and v[0][0] > 1 and v[0][1] > 1 for v in views.values())
            feature["reversed"] += any(s < 0 for v in views.values() for s, n in zip(v[1], v[0]) if n > 1)
            feature["extent_one"] += 1 in mnk
            feature["rank_not_2"] += any(len(nest.ops[k][0]) != 2 for k in "ABC")
            feature["once"] += any(trips(*l) == 1 for l in nest.loops)
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
              "a loop that runs once %(once)d, batched %(batched)d" % feature)
    print("mismatches: %d" % len(mismatches))


if __name__ == "__main__":
    main()
