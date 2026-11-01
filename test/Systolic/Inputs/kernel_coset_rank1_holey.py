"""Phase M5b-3c: rank-one output images with holes or congruence restrictions,
checked against an independent brute enumeration of the SOURCE statement.

Investigation result (2026-11-01): the extension X3 of the M5b-3 design review
("rank-1 images without the M1 contiguity certificate go through the guarded
scan", limitation L3) is already delivered by Phase M5b-3b, which placed
buildKernelCosetGuardedOutputScan behind the M1 scan plan for EVERY output rank.
This test pins the exact boundary of the rank-one class.

Positive cases (each refused by every existing route flag-off; exact under the
flag at Kdev 1, 2 and 4):
  R1 C[i+2j]    over the triangle j <= i        unit output map, holey image
  R2 C[4i+6j]   over the triangle               non-unit H = 2 and holes
  R3 C[2i+3j+5] over the triangle               offset
  R4 C[3i+5j]   over the triangle               larger coefficients, more holes
  R5 C[2i+3j]   over the box, gates k<=i, k<=j  unit-coefficient reduction gates
  R6 C[2i+3j][0] += A[2i+3j][k] * B[k][0], box  SystolicCC/general_affine.c CASE5
                                                 shape (2-D C, constant column)
  R7 the CASE5 shape over the triangle j <= i
For every emission the interpreter kernel_coset_macs expands the nest -- including
the affine.if existence guard -- and the test asserts
  * physical (A, B, C) MAC multiset == source multiset;
  * per-output (A, B) multiplicities == source (the emitted reduction fiber of
    every output c is exactly { x in D | h_C(x) = c });
  * the set of C STORE (and LOAD) addresses == the source output image S, one
    each per output: VisitedOutputs = S, no visit outside S, no repeat;
  * A and B are read only at source addresses;
  * every matmul_tile k <= k_max (16);
  * the path: 'Phase M5b-3b', 'rank-1 exact symbolic output scan', the
    existence guard (an affine.if is present in the Kdev=1 nest).
Still UNKNOWN (fail closed: no tile, IR byte-identical to flag-off):
  N1 C[2i+3j] with 2k <= i   both reduction coordinates need a non-unit elimination
  N2 C[i+2j]  with 2k <= j   idem: the kernel direction of i+2j carries a 2
  N3 C[2i+3j] with i == 2k   idem (an equality)
  N4 C[i floordiv 2]         a local (floordiv) in the access
Owned by existing routes (flag-off already lowered; the flag changes nothing):
  O1 C[2i+3j], O2 C[2i], O3 C[4i+6j], O4 C[3i+1], each over the 8x8x9 box.
"""
from collections import Counter, defaultdict
from pathlib import Path
import re
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'SystolicCC' / 'Inputs'))
from general_affine_oracle import kernel_coset_macs

opt, fixture, temp = sys.argv[1:4]
root = Path(temp); root.mkdir(parents=True, exist_ok=True)
CASE5 = '\n'.join(x for x in Path(fixture).read_text().splitlines()
                  if not x.startswith('//'))

KC = 'lowered by the experimental symbolic kernel-coset offload'
RANK1 = 'rank-1 exact symbolic output scan'
GUARD = 'affine.if existence guard'
DECLINE = 'kernel-coset offload not applied: '
BOTH = 'both reduction coordinates need a non-unit elimination'


def run(name, text, flag, kdev=1, fleet=True):
    src = root / (name + '.mlir'); dst = root / (name + '.out.mlir')
    src.write_text(text)
    o = 'general-affine=true allow-fp-reassociation=true report=true'
    if flag:
        o += f' kernel-coset-offload=true kernel-coset-kdev={kdev}'
    cmd = [opt] + (['--systolic-declare-fpga-fleet'] if fleet else []) + \
          ['--affine-matmul-to-systolic=' + o, str(src), '-o', str(dst)]
    p = subprocess.run(cmd, capture_output=True, text=True)
    (root / (name + '.log')).write_text(p.stderr)
    assert p.returncode == 0, (name, p.stderr)
    return dst.read_text(), p.stderr


def module(fn, cshape, ashape, bshape, loops, closes, cidx, aidx, bidx):
    return f"""module {{
  func.func @{fn}(%C: memref<{cshape}xf32> {{llvm.noalias}}, %A: memref<{ashape}xf32> {{llvm.noalias}}, %B: memref<{bshape}xf32> {{llvm.noalias}}) {{
    {loops}
      %t0 = affine.load %C[{cidx}] : memref<{cshape}xf32>
      %t1 = affine.load %A[{aidx}] : memref<{ashape}xf32>
      %t2 = affine.load %B[{bidx}] : memref<{bshape}xf32>
      %t3 = arith.mulf %t1, %t2 fastmath<reassoc> : f32
      %t4 = arith.addf %t0, %t3 fastmath<reassoc> : f32
      affine.store %t4, %C[{cidx}] : memref<{cshape}xf32>
    {closes}
    func.return
  }}
}}
"""


def shape(s):
    return tuple(int(v) for v in s.split('x'))


def lin(idx, shp):
    a, stride = 0, 1
    for v, sz in zip(reversed(idx), reversed(shp)):
        assert 0 <= v < sz, (idx, shp)
        a += v * stride; stride *= sz
    return a


class Stmt:
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


BOX8 = ('affine.for %i = 0 to 8 { affine.for %j = 0 to 8 { affine.for %k = 0 to 9 {', '}}}')
TRI8 = ('affine.for %i = 0 to 8 { affine.for %j = 0 to affine_map<(d0) -> (d0 + 1)>(%i) '
        '{ affine.for %k = 0 to 9 {', '}}}')
BOX8_2GATES = ('affine.for %i = 0 to 8 { affine.for %j = 0 to 8 { affine.for %k = 0 to 9 '
               '{ affine.if affine_set<(d0, d1, d2) : (d0 - d2 >= 0, d1 - d2 >= 0)>(%i, %j, %k) {', '}}}}')
BOX8_2K_LE_I = ('affine.for %i = 0 to 8 { affine.for %j = 0 to 8 { affine.for %k = 0 to 9 '
                '{ affine.if affine_set<(d0, d1, d2) : (d0 - 2 * d2 >= 0)>(%i, %j, %k) {', '}}}}')
BOX8_2K_LE_J = ('affine.for %i = 0 to 8 { affine.for %j = 0 to 8 { affine.for %k = 0 to 9 '
                '{ affine.if affine_set<(d0, d1, d2) : (d1 - 2 * d2 >= 0)>(%i, %j, %k) {', '}}}}')
BOX8_I_EQ_2K = ('affine.for %i = 0 to 8 { affine.for %j = 0 to 8 { affine.for %k = 0 to 9 '
                '{ affine.if affine_set<(d0, d1, d2) : (d0 - 2 * d2 == 0)>(%i, %j, %k) {', '}}}}')
box = lambda: ((i, j, k) for i in range(8) for j in range(8) for k in range(9))
tri = lambda: ((i, j, k) for i in range(8) for j in range(i + 1) for k in range(9))
A_ik = lambda i, j, k: (i, k)
B_kj = lambda i, j, k: (k, j)

R1 = Stmt('R1-ip2j-tri', module('r1', '24', '8x9', '9x8', *TRI8, '%i + 2 * %j', '%i, %k', '%k, %j'),
          tri, '24', '8x9', '9x8', lambda i, j, k: (i + 2 * j,), A_ik, B_kj)
R2 = Stmt('R2-4i6j-tri', module('r2', '72', '8x9', '9x8', *TRI8, '4 * %i + 6 * %j', '%i, %k', '%k, %j'),
          tri, '72', '8x9', '9x8', lambda i, j, k: (4 * i + 6 * j,), A_ik, B_kj)
R3 = Stmt('R3-2i3jp5-tri', module('r3', '48', '8x9', '9x8', *TRI8, '2 * %i + 3 * %j + 5', '%i, %k', '%k, %j'),
          tri, '48', '8x9', '9x8', lambda i, j, k: (2 * i + 3 * j + 5,), A_ik, B_kj)
R4 = Stmt('R4-3i5j-tri', module('r4', '60', '8x9', '9x8', *TRI8, '3 * %i + 5 * %j', '%i, %k', '%k, %j'),
          tri, '60', '8x9', '9x8', lambda i, j, k: (3 * i + 5 * j,), A_ik, B_kj)
R5 = Stmt('R5-2i3j-two-gates', module('r5', '40', '8x9', '9x8', *BOX8_2GATES, '2 * %i + 3 * %j', '%i, %k', '%k, %j'),
          lambda: ((i, j, k) for i in range(8) for j in range(8) for k in range(9) if k <= i and k <= j),
          '40', '8x9', '9x8', lambda i, j, k: (2 * i + 3 * j,), A_ik, B_kj)
R6 = Stmt('R6-case5-box', CASE5, box, '40x16', '40x16', '16x16',
          lambda i, j, k: (2 * i + 3 * j, 0), lambda i, j, k: (2 * i + 3 * j, k), lambda i, j, k: (k, 0))
R7 = Stmt('R7-case5-tri', module('r7', '40x16', '40x16', '16x16', *TRI8, '2 * %i + 3 * %j, 0',
                                 '2 * %i + 3 * %j, %k', '%k, 0'),
          tri, '40x16', '40x16', '16x16',
          lambda i, j, k: (2 * i + 3 * j, 0), lambda i, j, k: (2 * i + 3 * j, k), lambda i, j, k: (k, 0))


def check_exact(stmt, kdevs, expect_outputs, expect_macs):
    off_ir, _ = run(stmt.name + '-off', stmt.text, flag=False)
    assert 'systolic.matmul_tile' not in off_ir, (stmt.name, 'an existing route owns it')
    bm, bf, bo = stmt.brute()
    assert len(bo) == expect_outputs and sum(bm.values()) == expect_macs, (stmt.name, len(bo), sum(bm.values()))
    # a holey image: a proper subset of its shadow interval (in C's linear addresses)
    assert len(bo) < len(range(min(bo), max(bo) + 1)), (stmt.name, 'must have holes')
    for kdev in kdevs:
        ir, log = run(f'{stmt.name}-k{kdev}', stmt.text, flag=True, kdev=kdev)
        assert KC in log and 'Phase M5b-3b' in log and RANK1 in log and GUARD in log, (stmt.name, kdev, 'path', log)
        assert 'systolic.matmul_tile' in ir and 'arith.mulf' not in ir, (stmt.name, kdev, 'body')
        if kdev == 1:
            assert 'affine.if' in ir, (stmt.name, 'the existence guard must be present (Kdev=1)')
        trace = {}
        jobs, macs = kernel_coset_macs(ir, trace)
        gm = Counter(macs)
        assert gm == bm, (stmt.name, kdev, 'MAC multiset', sum((gm - bm).values()), sum((bm - gm).values()))
        gf = defaultdict(Counter)
        for a, b, c in macs:
            gf[c][(a, b)] += 1
        assert gf == bf, (stmt.name, kdev, 'per-output reduction fibers')
        stores = [addr for role, addr in trace.get('store', []) if role == 0]
        loads = [addr for role, addr in trace.get('load', []) if role == 0]
        assert set(stores) == bo and len(stores) == len(bo), (stmt.name, kdev, 'C stores', len(stores), len(bo))
        assert set(loads) == bo and len(loads) == len(bo), (stmt.name, kdev, 'C loads', len(loads), len(bo))
        assert {addr for role, addr in trace.get('load', []) if role == 1} == {a for a, _, _ in bm}, (stmt.name, kdev, 'A reads')
        assert {addr for role, addr in trace.get('load', []) if role == 2} == {b for _, b, _ in bm}, (stmt.name, kdev, 'B reads')
        ks = {int(x.split('=')[1]) for x in re.findall(r'k = [0-9]+', ir)}
        assert max(ks) <= 16, (stmt.name, kdev, 'k_max', ks)
    holes = len(range(min(bo), max(bo) + 1)) - len(bo)
    print(f'{stmt.name}: {len(bo)} outputs ({holes} holes in the shadow) / {sum(bm.values())} MACs exact at '
          f'Kdev {list(kdevs)} (Phase M5b-3b rank-1 shadow+guard): multiset, per-output fibers, one C '
          f'load+store per output, none outside the image, A/B reads at source addresses only, k<=16: PASS')


check_exact(R1, (1, 2, 4), expect_outputs=21, expect_macs=324)
check_exact(R2, (1, 2, 4), expect_outputs=30, expect_macs=324)
check_exact(R3, (1, 2, 4), expect_outputs=30, expect_macs=324)
check_exact(R4, (1, 2, 4), expect_outputs=36, expect_macs=324)
check_exact(R5, (1, 2, 4), expect_outputs=34, expect_macs=204)
check_exact(R6, (1, 2, 4), expect_outputs=34, expect_macs=576)
check_exact(R7, (1, 2, 4), expect_outputs=30, expect_macs=324)


# ---- still UNKNOWN: fail closed, IR byte-identical to flag-off ------------
def refused(name, text, reason):
    ir, log = run(name, text, flag=True)
    assert 'systolic.matmul_tile' not in ir and KC not in log, (name, 'must not offload')
    base, _ = run(name + '-off', text, flag=False)
    assert ir == base, (name, 'refusal must leave the IR byte-identical')
    assert 'systolic.matmul_tile' not in base, (name, 'no existing route owns it either')
    assert ir.count('arith.mulf') == 1 and ir.count('affine.store') == 1, (name, 'source MAC body kept')
    if reason:
        assert reason in log, (name, reason, log)


refused('N1-2i3j-2k-le-i', module('n1', '40', '8x9', '9x8', *BOX8_2K_LE_I, '2 * %i + 3 * %j', '%i, %k', '%k, %j'), BOTH)
refused('N2-ip2j-2k-le-j', module('n2', '24', '8x9', '9x8', *BOX8_2K_LE_J, '%i + 2 * %j', '%i, %k', '%k, %j'), BOTH)
refused('N3-2i3j-i-eq-2k', module('n3', '40', '8x9', '9x8', *BOX8_I_EQ_2K, '2 * %i + 3 * %j', '%i, %k', '%k, %j'), BOTH)
refused('N4-floordiv', module('n4', '8', '8x9', '9x8', *BOX8, '%i floordiv 2', '%i, %k', '%k, %j'), '')
print('N1-N3 (two non-unit kernel eliminations: gate 2k<=i, gate 2k<=j on C[i+2j], equality i==2k) and '
      'N4 (a floordiv local): UNKNOWN, no tile, IR byte-identical to flag-off: PASS')

# ---- unchanged ownership: box images of one or two output loops stay with the
# existing routes (the batched/strided fast path); the flag changes nothing.
for name, cidx, cshape in (('O1-2i3j-box', '2 * %i + 3 * %j', '40'), ('O2-2i-box', '2 * %i', '16'),
                           ('O3-4i6j-box', '4 * %i + 6 * %j', '80'), ('O4-3ip1-box', '3 * %i + 1', '24')):
    text = module('o', cshape, '8x9', '9x8', *BOX8, cidx, '%i, %k', '%k, %j')
    off_ir, _ = run(name + '-off', text, flag=False)
    on_ir, on_log = run(name + '-on', text, flag=True)
    assert 'systolic.matmul_tile' in off_ir and on_ir == off_ir and KC not in on_log, (name, 'ownership')
print('O1-O4 box images owned by existing routes, byte-identical under the flag: PASS')
