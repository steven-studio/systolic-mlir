"""Finding F-KC-5: several kernel-coset candidates in one module.

The dispatcher lowers the surviving candidates one at a time, and a successful
lowering erases its nest. It used to re-test each candidate's isolation against
the whole list on every iteration, so it read the block pointer of operations an
earlier iteration had freed: a heap use-after-free from the second candidate on,
visible as a segmentation fault from the third. This test pins the repair and,
just as much, what the repair must not change.

Every compile runs with freed memory poisoned and the allocator's thread cache
off, so a stale read faults at once where it would otherwise go unnoticed. (The
two settings are ignored by anything but glibc; they only add sensitivity.)

Each module below is compiled and checked as follows, at Kdev 1 and 2:

  no crash         exit 0, and the emitted module parses, verifies and passes
                   the tile lowering, so no pass left an invalid handle in it;
  lowered once     exactly one lowering remark per expected nest, on that nest's
                   source line, in source order (the dispatcher's order);
  refused          exactly one refusal remark per refused nest, with its reason;
  never reached    no kernel-coset remark at all for nests the dispatcher skips,
                   and the kappa > 2 refusal on the one shape that gets through;
  independence     a function holding a lowered nest prints exactly as it does
                   when compiled alone, so no neighbour changed it;
  ownership        every other function prints exactly as it does with the flag
                   off: a refused or skipped candidate keeps its IR, and a nest
                   an existing route owns keeps that route's output;
  exact MACs       the interpreter's physical (A, B, C) multiset equals a brute
                   enumeration of the source, and C is read and written exactly
                   once per output of each nest, nowhere else.
"""
from collections import Counter
import os
from pathlib import Path
import re
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'SystolicCC' / 'Inputs'))
from general_affine_oracle import kernel_coset_macs

opt, temp = sys.argv[1:3]
root = Path(temp); root.mkdir(parents=True, exist_ok=True)

ENV = dict(os.environ, MALLOC_PERTURB_='165', GLIBC_TUNABLES='glibc.malloc.tcache_count=0')
FLAGS = 'general-affine=true report=true'
LOWERED = 'lowered by the experimental symbolic kernel-coset offload'
REFUSED = 'kernel-coset offload not applied: '
REASSOC = ' fastmath<reassoc>'
NOT_REASSOCIABLE = 'floating-point reassociation is not permitted'
MAY_ALIAS = 'general affine factors may alias the output'
OUT_OF_BOUNDS = 'static memref bounds not proven'
KAPPA = 'kappa>2'


def tri(n, k):
    return (f'affine.for %i = 0 to {n} {{ affine.for %j = 0 to affine_map<(d0) -> (d0 + 1)>(%i) '
            f'{{ affine.for %k = 0 to {k} {{', '}}}',
            [(i, j, kk) for i in range(n) for j in range(i + 1) for kk in range(k)])


def box(n, k):
    return (f'affine.for %i = 0 to {n} {{ affine.for %j = 0 to {n} {{ affine.for %k = 0 to {k} {{', '}}}',
            [(i, j, kk) for i in range(n) for j in range(n) for kk in range(k)])


def dep(n, k):
    return (f'affine.for %i = 0 to {n} {{ affine.for %j = 0 to {n} {{ affine.for %k = 0 to min '
            f'affine_map<(d0, d1) -> (d0 + d1 + 1, {k})>(%i, %j) {{', '}}}',
            [(i, j, kk) for i in range(n) for j in range(n) for kk in range(min(i + j + 1, k))])


class Nest:
    """One loop nest. `outcome` is 'lowered', a refusal reason, or None for a nest
    the kernel-coset dispatcher never reports on. `points`, with the three address
    functions, is the brute source semantics of a lowered nest."""

    def __init__(self, loops, cidx, aidx, bidx, outcome, fns=None, fm=REASSOC):
        self.open, self.close, self.points = loops
        self.idx = (cidx, aidx, bidx)
        self.outcome, self.fns, self.fm = outcome, fns, fm
        self.line = None


class Kernel:
    def __init__(self, name, shapes, nests, noalias=True, oracle=True, owned=False):
        self.name, self.shapes, self.nests = name, shapes, nests
        self.noalias, self.oracle, self.owned = noalias, oracle, owned

    def lowers(self):
        return any(n.outcome == 'lowered' for n in self.nests)


def text_of(kernel):
    """The function's lines, with each nest's first line noted as an offset."""
    c, a, b = (f'memref<{s}xf32>' for s in kernel.shapes)
    attr = ' {llvm.noalias}' if kernel.noalias else ''
    lines = [f'  func.func @{kernel.name}(%C: {c}{attr}, %A: {a}{attr}, %B: {b}{attr}) {{']
    offsets = []
    for nest in kernel.nests:
        ci, ai, bi = nest.idx
        offsets.append(len(lines))
        lines += [f'    {nest.open}',
                  f'      %t0 = affine.load %C[{ci}] : {c}',
                  f'      %t1 = affine.load %A[{ai}] : {a}',
                  f'      %t2 = affine.load %B[{bi}] : {b}',
                  f'      %t3 = arith.mulf %t1, %t2{nest.fm} : f32',
                  f'      %t4 = arith.addf %t0, %t3{nest.fm} : f32',
                  f'      affine.store %t4, %C[{ci}] : {c}',
                  f'    {nest.close}']
    lines += ['    func.return', '  }']
    return lines, offsets


def module_of(kernels):
    lines = ['module {']
    for kernel in kernels:
        body, offsets = text_of(kernel)
        for nest, off in zip(kernel.nests, offsets):
            nest.line = len(lines) + off + 1
        lines += body
    lines.append('}')
    return '\n'.join(lines) + '\n'


def compile_(path, flag, kdev, local=False):
    opts = FLAGS + (f' kernel-coset-offload=true kernel-coset-kdev={kdev}' if flag else '')
    cmd = [opt, '--systolic-declare-fpga-fleet', '--affine-matmul-to-systolic=' + opts, str(path)]
    if local:
        cmd.append('--mlir-print-local-scope')
    r = subprocess.run(cmd, capture_output=True, text=True, env=ENV)
    assert r.returncode == 0, (path.name, 'exit', r.returncode, r.stderr[-600:])
    return r


def function(text, name):
    """The printed lines of one function, closing brace included."""
    lines = text.splitlines()
    start = next(i for i, s in enumerate(lines) if s.startswith(f'  func.func @{name}('))
    end = next(i for i in range(start, len(lines)) if lines[i] == '  }')
    return lines[start:end + 1]


def interpretable(text, name):
    """One function with the module's maps and sets, for the interpreter."""
    header = [s for s in text.splitlines() if s.startswith('#')]
    return '\n'.join(header + ['module {'] + function(text, name) + ['}']) + '\n'


def shape(s):
    return tuple(int(v) for v in s.split('x'))


def lin(idx, shp):
    a, stride = 0, 1
    for v, sz in zip(reversed(idx), reversed(shp)):
        assert 0 <= v < sz, (idx, shp)
        a += v * stride; stride *= sz
    return a


def exact(kernel, text):
    """The emitted nests of `kernel` against the brute source: returns its MACs."""
    trace = {'load': [], 'store': []}
    _, macs = kernel_coset_macs(interpretable(text, kernel.name), trace)
    cs, asp, bs = (shape(s) for s in kernel.shapes)
    want, visits = Counter(), Counter()
    for nest in kernel.nests:
        cfn, afn, bfn = nest.fns
        seen = set()
        for x in nest.points:
            c = lin(cfn(*x), cs)
            want[(lin(afn(*x), asp), lin(bfn(*x), bs), c)] += 1
            seen.add(c)
        visits.update(seen)
    assert Counter(macs) == want, (kernel.name, 'MAC multiset')
    stores = Counter(a for role, a in trace['store'] if role == 0)
    loads = Counter(a for role, a in trace['load'] if role == 0)
    assert stores == visits and loads == visits, (kernel.name, 'C visits')
    return sum(want.values())


def holey(name, n=12, k=9):
    """C[2i+3j] over a triangle: holes in the image, the guarded rank-1 path."""
    return Kernel(name, (str(5 * n - 4), f'{n}x{k}', f'{k}x{n}'),
                  [Nest(tri(n, k), '2 * %i + 3 * %j', '%i, %k', '%k, %j', 'lowered',
                        (lambda i, j, kk: (2 * i + 3 * j,), lambda i, j, kk: (i, kk), lambda i, j, kk: (kk, j)))])


def rows2(name, n=12, k=9):
    """C[2i][j] over a triangle: a rank-2 image."""
    return Kernel(name, (f'{2 * n - 1}x{n}', f'{n}x{k}', f'{k}x{n}'),
                  [Nest(tri(n, k), '2 * %i, %j', '%i, %k', '%k, %j', 'lowered',
                        (lambda i, j, kk: (2 * i, j), lambda i, j, kk: (i, kk), lambda i, j, kk: (kk, j)))])


def skew(name, n=10, k=12):
    """C[i+j-2k+o] over a box: a contiguous image, the first emission path."""
    o = 2 * (k - 1)
    return Kernel(name, (str(2 * (n - 1) + o + 1), f'{n}x{k}', f'{k}x{n}'),
                  [Nest(box(n, k), f'%i + %j - 2 * %k + {o}', '%i, %k', '%k, %j', 'lowered',
                        (lambda i, j, kk: (i + j - 2 * kk + o,), lambda i, j, kk: (i, kk), lambda i, j, kk: (kk, j)))])


def kdep(name, n=10, k=9):
    """k < min(i+j+1, K): the reduction depends on the output."""
    return Kernel(name, (f'{n}x{n}', f'{n}x{k}', f'{k}x{n}'),
                  [Nest(dep(n, k), '%i, %j', '%i, %k', '%k, %j', 'lowered',
                        (lambda i, j, kk: (i, j), lambda i, j, kk: (i, kk), lambda i, j, kk: (kk, j)))])


def strict(name):
    return Kernel(name, ('56', '12x9', '9x12'),
                  [Nest(tri(12, 9), '2 * %i + 3 * %j', '%i, %k', '%k, %j', NOT_REASSOCIABLE, fm='')])


def aliased(name):
    return Kernel(name, ('56', '12x9', '9x12'),
                  [Nest(tri(12, 9), '2 * %i + 3 * %j', '%i, %k', '%k, %j', MAY_ALIAS)], noalias=False)


def small_c(name):
    return Kernel(name, ('50', '12x9', '9x12'),
                  [Nest(tri(12, 9), '2 * %i + 3 * %j', '%i, %k', '%k, %j', OUT_OF_BOUNDS)])


def dynamic(name):
    return Kernel(name, ('?', '?x?', '?x?'),
                  [Nest(tri(12, 9), '2 * %i + 3 * %j', '%i, %k', '%k, %j', OUT_OF_BOUNDS)])


def deep(name):
    """Four loops, kappa 2: two nested candidates, so the dispatcher skips both."""
    loops = ('affine.for %i = 0 to 6 { affine.for %j = 0 to affine_map<(d0) -> (d0 + 1)>(%i) '
             '{ affine.for %k = 0 to 4 { affine.for %l = 0 to 4 {', '}}}}', [])
    return Kernel(name, ('6x6', '6x4x4', '4x4x6'), [Nest(loops, '%i, %j', '%i, %k, %l', '%k, %l, %j', None)])


def kappa3(name):
    """Five loops, kappa 3, under a coupled guard: unreachable by the dispatcher."""
    guard = 'affine_set<(d0, d1, d2, d3, d4) : (d0 + d1 + 4 - d2 - d3 - d4 >= 0)>(%i, %j, %k, %l, %m)'
    loops = ('affine.for %i = 0 to 4 { affine.for %j = 0 to 4 { affine.for %k = 0 to 8 { '
             f'affine.for %l = 0 to 8 {{ affine.for %m = 0 to 8 {{ affine.if {guard} {{', '}}}}}}', [])
    return Kernel(name, ('4x4', '4x8x8x8', '8x8x8x4'),
                  [Nest(loops, '%i, %j', '%i, %k, %l, %m', '%k, %l, %m, %j', None)])


def under_guard(name, cidx, cshape, cfn, outcome):
    """An affine.if under the outer loop, so that loop is no candidate: the inner
    three loops are one lone candidate, the dispatcher reaches the four-loop nest,
    and what the lowering erases is a proper ancestor of the candidate's anchor."""
    loops = ('affine.for %i = 0 to 6 { affine.if affine_set<(d0) : (d0 - 1 >= 0)>(%i) { '
             'affine.for %j = 0 to affine_map<(d0) -> (d0 + 1)>(%i) { affine.for %k = 0 to 4 { '
             'affine.for %l = 0 to 4 {', '}}}}}',
             [(i, j, k, l) for i in range(1, 6) for j in range(i + 1) for k in range(4) for l in range(4)])
    fns = (cfn, lambda i, j, k, l: (i, k, l), lambda i, j, k, l: (k, l, j))
    return Kernel(name, (cshape, '6x4x4', '4x4x6'),
                  [Nest(loops, cidx, '%i, %k, %l', '%k, %l, %j', outcome, fns)])


def above_anchor(name):
    """kappa 2 under the guard: lowered, erasing above the anchor."""
    return under_guard(name, '%i, %j', '6x6', lambda i, j, k, l: (i, j), 'lowered')


def kappa3_reached(name):
    """kappa 3 under the guard: the one shape that reaches the kappa gate. It stays
    refused."""
    return under_guard(name, '2 * %i + 3 * %j', '26', lambda i, j, k, l: (2 * i + 3 * j,), KAPPA)


def shared_parent(name):
    """Two candidates under one outer loop: not nested in each other, so both are
    isolated, but neither is a single chain from that loop, so neither lowers."""
    a = tri(12, 9)
    return Kernel(name, ('56', '12x9', '9x12'),
                  [Nest((f'affine.for %r = 0 to 1 {{ {a[0]}', a[1], []), '2 * %i + 3 * %j', '%i, %k', '%k, %j', None),
                   Nest((a[0], a[1] + ' }', []), '3 * %i + 2 * %j', '%i, %k', '%k, %j', None)], oracle=False)


def gemm(name):
    """A box GEMM: the fast path owns it, the kernel-coset never sees it."""
    return Kernel(name, ('4x4', '4x4', '4x4'),
                  [Nest(box(4, 4), '%i, %j', '%i, %k', '%k, %j', None)], oracle=False, owned=True)


def enumerated(name):
    """The det-9 base of kernel_coset_audit: the enumerating route owns it."""
    u, v = '%i + %j - 2 * %k + 6', '%i - 2 * %j + %k + 6'
    return Kernel(name, ('16x16', '16x16', '16x16'),
                  [Nest(box(4, 4), f'{u}, {v}', f'{u}, %i + %j + %k', f'%i + %j + %k, {v}', None)],
                  oracle=False, owned=True)


def small_tri(name):
    """Small, and still refused by every earlier route: the kernel-coset lowers it."""
    return Kernel(name, ('7', '3x3', '3x3'),
                  [Nest(tri(3, 3), '2 * %i + %j', '%i, %k', '%k, %j', 'lowered',
                        (lambda i, j, kk: (2 * i + j,), lambda i, j, kk: (i, kk), lambda i, j, kk: (kk, j)))])


def siblings(name):
    """Three lowered nests in one function, on the same memrefs."""
    n, k = 12, 9
    fns = (lambda i, j, kk: (2 * i + 3 * j,), lambda i, j, kk: (i, kk), lambda i, j, kk: (kk, j))
    nest = lambda m: Nest(tri(m, k), '2 * %i + 3 * %j', '%i, %k', '%k, %j', 'lowered', fns)
    return Kernel(name, ('56', f'{n}x{k}', f'{k}x{n}'), [nest(12), nest(11), nest(10)])


def mixed_siblings(name):
    """Lowered, refused, lowered, in one function. The refused nest stays between
    the two emissions, which the interpreter cannot read, so identity decides."""
    fns = (lambda i, j, kk: (2 * i + 3 * j,), lambda i, j, kk: (i, kk), lambda i, j, kk: (kk, j))
    return Kernel(name, ('56', '12x9', '9x12'),
                  [Nest(tri(12, 9), '2 * %i + 3 * %j', '%i, %k', '%k, %j', 'lowered', fns),
                   Nest(tri(12, 9), '2 * %i + 3 * %j', '%i, %k', '%k, %j', NOT_REASSOCIABLE, fm=''),
                   Nest(tri(11, 9), '2 * %i + 3 * %j', '%i, %k', '%k, %j', 'lowered', fns)], oracle=False)


def spread():
    return [holey('h1'), rows2('r1'), skew('s1'), kdep('d1'), holey('h2', 14, 9), rows2('r2', 11, 8),
            skew('s2', 9, 12), kdep('d2', 11, 8)]


def mixed():
    return [gemm('g1'), holey('h1'), strict('st'), rows2('r1'), deep('de'), skew('s1'), aliased('al'), kappa3('k3'),
            kdep('d1'), small_c('oo'), above_anchor('a1'), dynamic('dy'), shared_parent('sh'), kappa3_reached('kr'),
            small_tri('t1'), gemm('g2'), enumerated('e1'), holey('h2', 14, 9)]


MODULES = {
    'one': lambda: [holey('h1')],
    'two': lambda: [holey('h1'), rows2('r1')],
    'three': lambda: [holey('h1'), rows2('r1'), kdep('d1')],
    'eight': spread,
    'erased_above_anchor': lambda: [above_anchor('a1'), above_anchor('a2'), above_anchor('a3')],
    'mixed': mixed,
    'mixed_reversed': lambda: list(reversed(mixed())),
    'nothing_lowers': lambda: [strict('st'), aliased('al'), small_c('oo'), dynamic('dy'), deep('de'), kappa3('k3'),
                               kappa3_reached('kr'), shared_parent('sh')],
    'siblings': lambda: [holey('h1'), siblings('sib'), mixed_siblings('ms'), rows2('r1')],
}

solo_cache = {}


def solo(kernel, kdev):
    """`kernel` compiled as the only function of its module."""
    key = (kernel.name, kernel.shapes, tuple(n.open for n in kernel.nests), kdev)
    if key not in solo_cache:
        path = root / f'solo_{kernel.name}_{len(solo_cache)}.mlir'
        path.write_text(module_of([kernel]))
        solo_cache[key] = function(compile_(path, True, kdev, local=True).stdout, kernel.name)
    return solo_cache[key]


def check(name, factory, kdev):
    kernels = factory()
    text = module_of(kernels)
    path = root / f'{name}_k{kdev}.mlir'
    path.write_text(text)
    on = compile_(path, True, kdev)
    on_local = compile_(path, True, kdev, local=True).stdout
    off_local = compile_(path, False, kdev, local=True).stdout

    remarks = [(int(m[1]), m[2]) for m in re.finditer(r'\.mlir:(\d+):\d+: remark: (.*)', on.stderr)]
    nests = [n for k in kernels for n in k.nests]
    got = [line for line, msg in remarks if LOWERED in msg]
    want = [n.line for n in nests if n.outcome == 'lowered']
    assert got == want, (name, kdev, 'lowering remarks: expected one per nest, in source order', got, want)
    got = [(line, msg) for line, msg in remarks if REFUSED in msg]
    want = [n for n in nests if n.outcome not in (None, 'lowered')]
    assert [line for line, _ in got] == [n.line for n in want], (name, kdev, 'refusal remarks', got)
    for (_, msg), nest in zip(got, want):
        assert nest.outcome in msg, (name, kdev, nest.outcome, msg)

    macs = 0
    for kernel in kernels:
        printed = function(on_local, kernel.name)
        if kernel.lowers():
            assert printed == solo(kernel, kdev), (name, kdev, kernel.name, 'differs from the kernel alone')
        else:
            assert printed == function(off_local, kernel.name), (name, kdev, kernel.name, 'differs from the flag off')
        if kernel.owned:
            tile = any('systolic.matmul_tile' in s for s in printed)
            assert tile and not any('arith.mulf' in s for s in printed), (name, kdev, kernel.name, 'owner lost it')
        # A lowered nest leaves no source MAC behind, a refused nest keeps its own.
        if all(n.outcome is not None for n in kernel.nests):
            left = sum(s.count('arith.mulf') for s in printed)
            assert left == sum(n.outcome != 'lowered' for n in kernel.nests), (name, kdev, kernel.name, left)
        if kernel.lowers() and kernel.oracle:
            macs += exact(kernel, on.stdout)

    if not any(k.lowers() for k in kernels):
        assert on_local == off_local, (name, kdev, 'a module with nothing to lower changed')

    # The result is a valid module: it parses, verifies, and survives the next pass.
    out = root / f'{name}_k{kdev}.out.mlir'
    out.write_text(on.stdout)
    for cmd in ([opt, str(out)], [opt, '--lower-systolic-matmul-tile-to-fpga=phys-k-max-8x8=16 phys-k-max-4x4=16', str(out)]):
        r = subprocess.run(cmd, capture_output=True, text=True, env=ENV)
        assert r.returncode == 0, (name, kdev, cmd[1], r.stderr[-400:])
    return len(kernels), sum(1 for n in nests if n.outcome == 'lowered'), macs


for name, factory in MODULES.items():
    for kdev in (1, 2):
        functions, lowered, macs = check(name, factory, kdev)
        print(f'{name} Kdev {kdev}: {functions} functions, {lowered} nests lowered once in order, '
              f'{macs} MACs exact, the rest unchanged, output valid: PASS')
