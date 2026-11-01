"""Phase M5b-2: exact symbolic rank-2 kernel-coset emission, checked against an
independent brute enumeration of the SOURCE statement (test only).

For every claimed emission (Kdev 1, 2 and a wider block) the interpreter
kernel_coset_macs expands the emitted nest and the test asserts
  * physical (A, B, C) MAC multiset == source multiset (not just the count);
  * per-output (A, B) multiplicities == source;
  * the set of C read-modify-write STORE addresses == the source output image,
    one store (and one load) per output -- no phantom and no repeated visit;
  * every matmul_tile k <= k_max (16);
  * the source MAC body is gone and the Phase M5b-2 remark is present.
Positive cases (all refused by every existing route -- checked -- so the
kernel-coset is the owner): A2 triangular domain with a transposed A view,
B2 C[2i][j] over a triangle (even rows only), C2 C[i+j][i-j+11] over a triangle
(parallelogram, parity), D 0<=k<=min(i+j,8) (output-dependent fiber), E2
C[2i][3j] over a triangle (sparse lattice), F det-9 at 9^3 and 16^3 (index-3
lattice, non-box projection). Ownership preserved: the box-domain shapes A, B,
C, E are owned by the existing routes and keep byte-identical IR under the
flag. G: the committed rank-one skew fixtures still take the Phase M4b.2 path
and stay exact. H: refusals (strict FP, aliasing, no fleet, Kdev 0, non-unit
reduction gate, congruence guard, replication axis) leave the IR byte-identical
to the flag-off output.
"""
from collections import Counter, defaultdict
from pathlib import Path
import re
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'SystolicCC' / 'Inputs'))
from general_affine_oracle import kernel_coset_macs, skew_source, skew_macs

opt, fixture, temp = sys.argv[1:4]
root = Path(temp); root.mkdir(parents=True, exist_ok=True)
DET9_9 = '\n'.join(x for x in Path(fixture).read_text().splitlines()
                   if not x.startswith('//'))

KC = 'lowered by the experimental symbolic kernel-coset offload'
M5B2 = 'Phase M5b-2, rank-2 exact symbolic output scan'
M4B2 = '(Phase M4b.2)'
DECLINE = 'kernel-coset offload not applied: '


def run(name, text, flag, kdev=2, fleet=True, extra=''):
    src = root / (name + '.mlir'); dst = root / (name + '.out.mlir')
    src.write_text(text)
    o = 'general-affine=true report=true'
    if flag:
        o += f' kernel-coset-offload=true kernel-coset-kdev={kdev}'
    if extra:
        o += ' ' + extra
    cmd = [opt] + (['--systolic-declare-fpga-fleet'] if fleet else []) + \
          ['--affine-matmul-to-systolic=' + o, str(src), '-o', str(dst)]
    p = subprocess.run(cmd, capture_output=True, text=True)
    (root / (name + '.log')).write_text(p.stderr)
    assert p.returncode == 0, (name, p.stderr)
    return dst.read_text(), p.stderr


def module(fn, cshape, ashape, bshape, loops, closes, cidx, aidx, bidx,
           fm=' fastmath<reassoc>', noalias=' {llvm.noalias}'):
    return f"""module {{
  func.func @{fn}(%C: memref<{cshape}xf32>{noalias}, %A: memref<{ashape}xf32>{noalias}, %B: memref<{bshape}xf32>{noalias}) {{
    {loops}
      %t0 = affine.load %C[{cidx}] : memref<{cshape}xf32>
      %t1 = affine.load %A[{aidx}] : memref<{ashape}xf32>
      %t2 = affine.load %B[{bidx}] : memref<{bshape}xf32>
      %t3 = arith.mulf %t1, %t2{fm} : f32
      %t4 = arith.addf %t0, %t3{fm} : f32
      affine.store %t4, %C[{cidx}] : memref<{cshape}xf32>
    {closes}
    func.return
  }}
}}
"""


def box(n, k):
    return f'affine.for %i = 0 to {n} {{ affine.for %j = 0 to {n} {{ affine.for %k = 0 to {k} {{', '}}}'


def tri(n, k):
    return (f'affine.for %i = 0 to {n} {{ affine.for %j = 0 to affine_map<(d0) -> (d0 + 1)>(%i) '
            f'{{ affine.for %k = 0 to {k} {{', '}}}')


def dep(n, k):
    return (f'affine.for %i = 0 to {n} {{ affine.for %j = 0 to {n} '
            f'{{ affine.for %k = 0 to min affine_map<(d0, d1) -> (d0 + d1 + 1, {k})>(%i, %j) {{', '}}}')


def shape(s):
    return tuple(int(v) for v in s.split('x'))


def lin(idx, shp):
    a, stride = 0, 1
    for v, sz in zip(reversed(idx), reversed(shp)):
        assert 0 <= v < sz, (idx, shp)
        a += v * stride; stride *= sz
    return a


class Stmt:
    """A fixture plus its brute-force source semantics."""

    def __init__(self, name, text, points, cshape, ashape, bshape, cidx, aidx, bidx):
        self.name, self.text, self.points = name, text, points
        self.shapes = (shape(cshape), shape(ashape), shape(bshape))
        self.fns = (cidx, aidx, bidx)

    def brute(self):
        macs, fibers, outputs = Counter(), defaultdict(Counter), set()
        for x in self.points():
            c = lin(self.fns[0](*x), self.shapes[0])
            a = lin(self.fns[1](*x), self.shapes[1])
            b = lin(self.fns[2](*x), self.shapes[2])
            macs[(a, b, c)] += 1
            fibers[c][(a, b)] += 1
            outputs.add(c)
        return macs, fibers, outputs


def tri_points(n, k):
    return lambda: ((i, j, kk) for i in range(n) for j in range(i + 1) for kk in range(k))


def box_points(n, k):
    return lambda: ((i, j, kk) for i in range(n) for j in range(n) for kk in range(k))


def dep_points(n, k):
    return lambda: ((i, j, kk) for i in range(n) for j in range(n) for kk in range(min(i + j + 1, k)))


def det9_stmt(name, text, n, off, size):
    return Stmt(name, text, box_points(n, n), f'{size}x{size}', f'{size}x{size}', f'{size}x{size}',
                lambda i, j, k: (i + j - 2 * k + off, i - 2 * j + k + off),
                lambda i, j, k: (i + j - 2 * k + off, i + j + k),
                lambda i, j, k: (i + j + k, i - 2 * j + k + off))


L, C_ = tri(12, 9)
A2 = Stmt('A2-tri-At', module('a2', '12x12', '9x12', '9x12', L, C_, '%i, %j', '%k, %i', '%k, %j'),
          tri_points(12, 9), '12x12', '9x12', '9x12',
          lambda i, j, k: (i, j), lambda i, j, k: (k, i), lambda i, j, k: (k, j))
B2 = Stmt('B2-C2i-j-tri', module('b2', '24x12', '12x9', '9x12', L, C_, '2 * %i, %j', '%i, %k', '%k, %j'),
          tri_points(12, 9), '24x12', '12x9', '9x12',
          lambda i, j, k: (2 * i, j), lambda i, j, k: (i, k), lambda i, j, k: (k, j))
C2 = Stmt('C2-skew-tri', module('c2', '24x24', '12x9', '9x12', L, C_, '%i + %j, %i - %j + 11', '%i, %k', '%k, %j'),
          tri_points(12, 9), '24x24', '12x9', '9x12',
          lambda i, j, k: (i + j, i - j + 11), lambda i, j, k: (i, k), lambda i, j, k: (k, j))
E2 = Stmt('E2-C2i-3j-tri', module('e2', '24x36', '12x9', '9x12', L, C_, '2 * %i, 3 * %j', '%i, %k', '%k, %j'),
          tri_points(12, 9), '24x36', '12x9', '9x12',
          lambda i, j, k: (2 * i, 3 * j), lambda i, j, k: (i, k), lambda i, j, k: (k, j))
LD, CD = dep(10, 9)
D = Stmt('D-kdep', module('d', '10x10', '10x9', '9x10', LD, CD, '%i, %j', '%i, %k', '%k, %j'),
         dep_points(10, 9), '10x10', '10x9', '9x10',
         lambda i, j, k: (i, j), lambda i, j, k: (i, k), lambda i, j, k: (k, j))
F9 = det9_stmt('F-det9-9', DET9_9, 9, 18, 40)
L16, C16 = box(16, 16)
F16 = det9_stmt('F-det9-16', module('f16', '64x64', '64x64', '64x64', L16, C16,
                                    '%i + %j - 2 * %k + 32, %i - 2 * %j + %k + 32',
                                    '%i + %j - 2 * %k + 32, %i + %j + %k',
                                    '%i + %j + %k, %i - 2 * %j + %k + 32'), 16, 32, 64)


def check_exact(stmt, kdevs, expect_outputs=None, expect_macs=None):
    off_ir, _ = run(stmt.name + '-off', stmt.text, flag=False)
    assert 'systolic.matmul_tile' not in off_ir, (stmt.name, 'an existing route owns it')
    bm, bf, bo = stmt.brute()
    if expect_outputs is not None:
        assert len(bo) == expect_outputs, (stmt.name, len(bo))
    if expect_macs is not None:
        assert sum(bm.values()) == expect_macs, (stmt.name, sum(bm.values()))
    headers = None
    for kdev in kdevs:
        ir, log = run(f'{stmt.name}-k{kdev}', stmt.text, flag=True, kdev=kdev)
        assert KC in log and M5B2 in log, (stmt.name, kdev, 'M5b-2 remark')
        assert 'systolic.matmul_tile' in ir and 'arith.mulf' not in ir, (stmt.name, kdev, 'body')
        trace = {}
        jobs, macs = kernel_coset_macs(ir, trace)
        gm = Counter(macs)
        assert gm == bm, (stmt.name, kdev, 'MAC multiset', sum((gm - bm).values()), sum((bm - gm).values()))
        gf = defaultdict(Counter)
        for a, b, c in macs:
            gf[c][(a, b)] += 1
        assert gf == bf, (stmt.name, kdev, 'per-output multiplicities')
        stores = [addr for role, addr in trace.get('store', []) if role == 0]
        loads = [addr for role, addr in trace.get('load', []) if role == 0]
        assert set(stores) == bo and len(stores) == len(bo), (stmt.name, kdev, 'C stores', len(stores), len(bo))
        assert set(loads) == bo and len(loads) == len(bo), (stmt.name, kdev, 'C loads', len(loads), len(bo))
        ks = {int(x.split('=')[1]) for x in re.findall(r'k = [0-9]+', ir)}
        assert max(ks) <= 16, (stmt.name, kdev, 'k_max', ks)
        if kdev == 1:
            headers = [l.strip() for l in ir.splitlines() if 'affine.for' in l]
    print(f'{stmt.name}: {len(bo)} outputs / {sum(bm.values())} MACs exact at Kdev {list(kdevs)}: '
          f'multiset, per-output multiplicities, one C load+store per output, k<=16: PASS')
    return headers


# ---- positive rank-2 cases ------------------------------------------------
hdr = {}
hdr['A2'] = check_exact(A2, (1, 2, 4), expect_outputs=78, expect_macs=702)
hdr['B2'] = check_exact(B2, (1, 2, 4), expect_outputs=78, expect_macs=702)
hdr['C2'] = check_exact(C2, (1, 2, 4), expect_outputs=78, expect_macs=702)
hdr['D'] = check_exact(D, (1, 2, 4), expect_outputs=100, expect_macs=780)
hdr['E2'] = check_exact(E2, (1, 2, 4), expect_outputs=78, expect_macs=702)
hdr['F9'] = check_exact(F9, (1, 2, 4), expect_macs=729)
hdr['F16'] = check_exact(F16, (1, 2, 16), expect_outputs=721, expect_macs=4096)
# the inner output loop is bounded by affine maps of the outer IV in every
# non-box projection (a single face prints as `#map(%iv)`, several as
# `max #map(%iv)` / `min #map(%iv)`); the box-projection case D has constants.
for name in ('A2', 'B2', 'C2', 'E2', 'F9', 'F16'):
    assert re.search(r'affine\.for %\w+ = (max )?#map\d*\(%\w+\) to (min )?#map\d*\(%\w+\)', hdr[name][1]), \
        (name, hdr[name][:2])
# D: box output image, kappa = 1 -> two output loops (outer constant) and the
# tile loop; its inner level may carry a redundant face from the projection.
assert re.fullmatch(r'affine\.for %\w+ = 0 to 10 \{', hdr['D'][0]) and len(hdr['D']) == 3, hdr['D']
for name, h in hdr.items():
    print(f'   {name} Kdev=1 nest: ' + ' | '.join(x[:60] for x in h))

# ---- ownership preserved: box-domain shapes stay with the existing routes --
LB, CB = box(8, 9)
owned = {
    'A-tri-unit': module('a', '12x12', '12x9', '9x12', L, C_, '%i, %j', '%i, %k', '%k, %j'),
    'B-C2i-j-box': module('b', '16x8', '8x9', '9x8', LB, CB, '2 * %i, %j', '%i, %k', '%k, %j'),
    'C-skew-box': module('c', '16x16', '8x9', '9x8', LB, CB, '%i + %j, %i - %j + 7', '%i, %k', '%k, %j'),
    'E-C2i-3j-box': module('e', '16x24', '8x9', '9x8', LB, CB, '2 * %i, 3 * %j', '%i, %k', '%k, %j'),
    'box-GEMM': module('g', '8x8', '8x9', '9x8', LB, CB, '%i, %j', '%i, %k', '%k, %j'),
    # the det-9 base gated by 1 <= 2k <= i has 144 MACs: the enumeration route
    # owns it (under budget, in bounds); the kernel-coset never sees it.
    'det9-gated-enum-owned': DET9_9.replace(
        'affine.for %k = 0 to 9 {',
        'affine.for %k = 0 to 9 { affine.if affine_set<(d0, d1) : (d0 - 2 * d1 >= 0, 2 * d1 - 1 >= 0)>(%i, %k) {'
    ).replace('        }\n      }\n    }\n    func.return', '        }}\n      }\n    }\n    func.return'),
}
for name, text in owned.items():
    off_ir, _ = run(name + '-off', text, flag=False)
    on_ir, on_log = run(name + '-on', text, flag=True)
    assert 'systolic.matmul_tile' in off_ir, (name, 'expected an existing route to own it')
    assert on_ir == off_ir and KC not in on_log, (name, 'kernel-coset must not steal an owned candidate')
print('ownership preserved: A (unit triangle), B, C, E (box domains), box GEMM owned by existing '
      'routes, byte-identical under the flag: PASS')

# ---- G: the committed rank-one fixtures still take the M4b.2 path ---------
SKEW = module('skew', '16', '16x16', '16x16', *box(4, 4), '(((%i + %j) - (2 * %k)) + 6)', '%i, %k', '%k, %j')
for kdev in (1, 2):
    ir, log = run(f'G-skew-k{kdev}', SKEW, flag=True, kdev=kdev)
    assert KC in log and M4B2 in log and M5B2 not in log, ('G skew', kdev)
    j, macs = kernel_coset_macs(ir)
    assert Counter(macs) == Counter(skew_source()) and len(macs) == 64, ('G skew', kdev)
    if kdev == 1:
        assert ir.count('affine.for') == 3 and ir.count('systolic.matmul_tile') == 1
WSK = module('wsk', '64', '16x16', '16x16', *box(16, 16), '(((%i + %j) - (2 * %k)) + 32)', '%i, %k', '%k, %j')
ir, log = run('G-wsk-k2', WSK, flag=True, kdev=2)
assert KC in log and M4B2 in log and M5B2 not in log, 'G wsk'
j, macs = kernel_coset_macs(ir)
assert Counter(macs) == Counter(skew_macs(16, 32, 16)) and len(macs) == 4096, 'G wsk exact'
print('G rank-one skew / wider skew: Phase M4b.2 path, exact 64 / 4096 MACs: PASS')

# ---- H: refusals on the rank-2 fixture leave the IR byte-identical ---------
off_ir, _ = run('H-off', DET9_9, flag=False)


def refused(name, text, reason, **kw):
    """Flag on must not offload, must leave the IR byte-identical to the
    flag-off output, and (when the candidate reaches the kernel-coset gate)
    must log the given decline reason."""
    ir, log = run(name, text, flag=True, **kw)
    assert 'systolic.matmul_tile' not in ir and KC not in log, (name, 'must not offload')
    # the flag-off reference is run with the same fleet declaration (the
    # systolic.device ops are part of the output)
    base, _ = run(name + '-off', text, flag=False, fleet=kw.get('fleet', True))
    assert ir == base, (name, 'refusal must leave the IR byte-identical')
    assert ir.count('arith.mulf') == 1 and ir.count('affine.store') == 1, (name, 'source MAC body')
    if reason:
        assert reason in log, (name, reason, log)


refused('H-strict', DET9_9.replace(' fastmath<reassoc>', ''),
        DECLINE + 'floating-point reassociation is not permitted')
refused('H-alias', DET9_9.replace(' {llvm.noalias}', ''),
        DECLINE + 'general affine factors may alias the output')
refused('H-nofleet', DET9_9, DECLINE + 'kernel-coset offload requires a declared fleet', fleet=False)
refused('H-kdev0', DET9_9, DECLINE + 'kernel-coset Kdev must be >= 1', kdev=0)
# Non-unit reduction gate 1 <= 2k <= i in the IDENTITY frame (C[i][j], U = I,
# so the coefficient 2 on the kernel column survives): the triangle with the
# transposed A at N=16 (644 gated MACs: over budget, refused by every route)
# must be declined by the exact output scan, not approximated.
LG, CG = tri(16, 9)
gate = module('gate', '16x16', '9x16', '9x16',
              LG.replace('affine.for %k = 0 to 9 {',
                         'affine.for %k = 0 to 9 { affine.if affine_set<(d0, d1) : (d0 - 2 * d1 >= 0, 2 * d1 - 1 >= 0)>(%i, %k) {'),
              CG + '}', '%i, %j', '%k, %i', '%k, %j')
refused('H-nonunit-gate', gate, 'exact output scan (Phase M5b-2): UNKNOWN: non-unit coefficient')
mod = DET9_9.replace('affine.for %k = 0 to 9 {',
                     'affine.for %k = 0 to 9 { affine.if affine_set<(d0) : (d0 mod 2 == 0)>(%i) {')
mod = mod.replace('        }\n      }\n    }\n    func.return', '        }}\n      }\n    }\n    func.return')
refused('H-congruence', mod, DECLINE + 'source domain carries symbols or congruence locals')
rep = DET9_9.replace('    affine.for %i = 0 to 9 {', '    affine.for %p = 0 to 2 {\n    affine.for %i = 0 to 9 {')
rep = rep.replace('    }\n    func.return', '    }\n    }\n    func.return')
refused('H-replication', rep, '')
# F3: an EMPTY source domain (zero-trip reduction loop) must be refused -- the
# committed path built output loops from the meaningless constant bounds of
# the infeasible system and read-modify-wrote C at 120 addresses the source
# never touches (some out of bounds).
refused('H-empty', DET9_9.replace('affine.for %k = 0 to 9 {', 'affine.for %k = 5 to 5 {'),
        DECLINE + 'empty source domain (zero-trip loop nest)')
print('H refusals (strict FP, aliasing, no fleet, Kdev 0, non-unit reduction gate, congruence guard, '
      'replication axis, empty domain): no tile, IR byte-identical to flag-off: PASS')
