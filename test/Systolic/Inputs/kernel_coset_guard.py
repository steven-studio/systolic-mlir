"""Phase M5b-3b: guarded symbolic output lowering, checked against an
independent brute enumeration of the SOURCE statement (test only).

For every claimed emission (Kdev 1, 2 and a wider block) the interpreter
kernel_coset_macs expands the emitted nest -- including the affine.if existence
guard -- and the test asserts
  * physical (A, B, C) MAC multiset == source multiset;
  * per-output (A, B) multiplicities == source;
  * the set of C read-modify-write STORE (and LOAD) addresses == the source
    output image, one each per output: no visit outside the image, no repeat;
  * every matmul_tile k <= k_max (16);
  * the expected path: 'Phase M5b-3b' with 'affine.if existence guard' for a
    shadow+guard case (an affine.if is present in the Kdev=1 nest), 'Phase
    M5b-3b' with the 'scan' certificate (no guard) for a shadow-level case,
    'Phase M4b.2' for the committed rank-one path.
Positive cases (each refused by every existing route -- checked):
  G1 C[2i+3j] over a triangle (holes in a rank-one image) ... shadow+guard
  G2 C[2i+3j][i-j+11] over a triangle (non-box rank-2 image) ... scan
  G3 C[i][j] with the equality guard i == 2k (even rows only) ... shadow+guard
  G4 C[i][j] with the gate 1 <= 2k <= i, transposed A ........ shadow+guard
  G5 C[i][j] with 2i+3j-k >= 0 (unit reduction coefficient) .. scan
  G6 C[2i+j-k+8] over a box (committed rank-one contiguity) .. M4b.2 path
  G8 C[2i+3j+7] over negative loop bounds, j <= i+1 .......... shadow+guard
Refusals (IR byte-identical to the flag-off output): empty domains on the
M1 path (F3) and on the guarded path, strict FP, aliasing, no fleet, Kdev 0,
an overflowing index coefficient. Ownership: the committed rank-one skew still
takes the M4b.2 path; box-domain shapes stay with the existing routes.
"""
from collections import Counter, defaultdict
from pathlib import Path
import re
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'SystolicCC' / 'Inputs'))
from general_affine_oracle import kernel_coset_macs, skew_source

opt, fixture, temp = sys.argv[1:4]
root = Path(temp); root.mkdir(parents=True, exist_ok=True)
HOLES = '\n'.join(x for x in Path(fixture).read_text().splitlines()
                  if not x.startswith('//'))

KC = 'lowered by the experimental symbolic kernel-coset offload'
GUARD = 'affine.if existence guard'
DECLINE = 'kernel-coset offload not applied: '


def run(name, text, flag, kdev=2, fleet=True):
    src = root / (name + '.mlir'); dst = root / (name + '.out.mlir')
    src.write_text(text)
    o = 'general-affine=true report=true'
    if flag:
        o += f' kernel-coset-offload=true kernel-coset-kdev={kdev}'
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


TRI12 = ('affine.for %i = 0 to 12 { affine.for %j = 0 to affine_map<(d0) -> (d0 + 1)>(%i) '
         '{ affine.for %k = 0 to 9 {', '}}}')
TRI16G = ('affine.for %i = 0 to 16 { affine.for %j = 0 to affine_map<(d0) -> (d0 + 1)>(%i) '
          '{ affine.for %k = 0 to 9 { affine.if affine_set<(d0, d1) : (d0 - 2 * d1 >= 0, 2 * d1 - 1 >= 0)>(%i, %k) {',
          '}}}}')
BOX40EQ = ('affine.for %i = 0 to 40 { affine.for %j = 0 to 40 { affine.for %k = 0 to 20 '
           '{ affine.if affine_set<(d0, d1) : (d0 - 2 * d1 == 0)>(%i, %k) {', '}}}}')
BOX10G = ('affine.for %i = 0 to 10 { affine.for %j = 0 to 10 { affine.for %k = 0 to 9 '
          '{ affine.if affine_set<(d0, d1, d2) : (2 * d0 + 3 * d1 - d2 >= 0)>(%i, %j, %k) {', '}}}}')
BOX8 = ('affine.for %i = 0 to 8 { affine.for %j = 0 to 8 { affine.for %k = 0 to 9 {', '}}}')
NEG = ('affine.for %i = -2 to 2 { affine.for %j = -1 to affine_map<(d0) -> (d0 + 2)>(%i) '
       '{ affine.for %k = 0 to 60 {', '}}}')

G1 = Stmt('G1-C2i3j-tri', HOLES,
          lambda: ((i, j, k) for i in range(12) for j in range(i + 1) for k in range(9)),
          '60', '12x9', '9x12',
          lambda i, j, k: (2 * i + 3 * j,), lambda i, j, k: (i, k), lambda i, j, k: (k, j))
G2 = Stmt('G2-C2i3j-imj-tri', module('g2', '60x24', '12x9', '9x12', *TRI12,
                                      '2 * %i + 3 * %j, %i - %j + 11', '%i, %k', '%k, %j'),
          lambda: ((i, j, k) for i in range(12) for j in range(i + 1) for k in range(9)),
          '60x24', '12x9', '9x12',
          lambda i, j, k: (2 * i + 3 * j, i - j + 11), lambda i, j, k: (i, k), lambda i, j, k: (k, j))
G3 = Stmt('G3-eq-i-2k', module('g3', '40x40', '20x40', '20x40', *BOX40EQ, '%i, %j', '%k, %i', '%k, %j'),
          lambda: ((i, j, k) for i in range(40) for j in range(40) for k in range(20) if i == 2 * k),
          '40x40', '20x40', '20x40',
          lambda i, j, k: (i, j), lambda i, j, k: (k, i), lambda i, j, k: (k, j))
G4 = Stmt('G4-gate-2k-le-i', module('g4', '16x16', '9x16', '9x16', *TRI16G, '%i, %j', '%k, %i', '%k, %j'),
          lambda: ((i, j, k) for i in range(16) for j in range(i + 1) for k in range(9)
                   if i - 2 * k >= 0 and 2 * k - 1 >= 0),
          '16x16', '9x16', '9x16',
          lambda i, j, k: (i, j), lambda i, j, k: (k, i), lambda i, j, k: (k, j))
G5 = Stmt('G5-2i3j-k-ge0', module('g5', '10x10', '10x9', '9x10', *BOX10G, '%i, %j', '%i, %k', '%k, %j'),
          lambda: ((i, j, k) for i in range(10) for j in range(10) for k in range(9) if 2 * i + 3 * j - k >= 0),
          '10x10', '10x9', '9x10',
          lambda i, j, k: (i, j), lambda i, j, k: (i, k), lambda i, j, k: (k, j))
G6 = Stmt('G6-2ipjmk', module('g6', '30', '8x9', '9x8', *BOX8, '2 * %i + %j - %k + 8', '%i, %k', '%k, %j'),
          lambda: ((i, j, k) for i in range(8) for j in range(8) for k in range(9)),
          '30', '8x9', '9x8',
          lambda i, j, k: (2 * i + j - k + 8,), lambda i, j, k: (i, k), lambda i, j, k: (k, j))
G8 = Stmt('G8-negative-bounds', module('g8', '16', '4x60', '60x4', *NEG,
                                       '2 * %i + 3 * %j + 7', '%i + 2, %k', '%k, %j + 1'),
          lambda: ((i, j, k) for i in range(-2, 2) for j in range(-1, i + 2) for k in range(60)),
          '16', '4x60', '60x4',
          lambda i, j, k: (2 * i + 3 * j + 7,), lambda i, j, k: (i + 2, k), lambda i, j, k: (k, j + 1))


def check_exact(stmt, kdevs, phase, guard, expect_outputs=None, expect_macs=None):
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
        assert KC in log and phase in log, (stmt.name, kdev, 'path', log)
        assert (GUARD in log) == guard, (stmt.name, kdev, 'guard remark')
        assert 'systolic.matmul_tile' in ir and 'arith.mulf' not in ir, (stmt.name, kdev, 'body')
        if kdev == 1:
            assert ('affine.if' in ir) == guard, (stmt.name, 'affine.if present iff guarded (Kdev=1)')
            headers = [l.strip() for l in ir.splitlines() if 'affine.for' in l or 'affine.if' in l]
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
    print(f'{stmt.name}: {len(bo)} outputs / {sum(bm.values())} MACs exact at Kdev {list(kdevs)} '
          f'({phase}{", guarded" if guard else ""}): multiset, per-output multiplicities, one C load+store '
          f'per output, none outside the image, k<=16: PASS')
    return headers


hdr = {}
hdr['G1'] = check_exact(G1, (1, 2, 4), 'Phase M5b-3b', True, expect_macs=702)
hdr['G2'] = check_exact(G2, (1, 2, 4), 'Phase M5b-3b', False, expect_outputs=78, expect_macs=702)
hdr['G3'] = check_exact(G3, (1, 2), 'Phase M5b-3b', True, expect_outputs=800, expect_macs=800)
hdr['G4'] = check_exact(G4, (1, 2, 4), 'Phase M5b-3b', True, expect_outputs=133, expect_macs=644)
hdr['G5'] = check_exact(G5, (1, 2, 4), 'Phase M5b-3b', False, expect_outputs=100, expect_macs=869)
hdr['G6'] = check_exact(G6, (1, 2, 4), '(Phase M4b.2)', False, expect_macs=576)
hdr['G8'] = check_exact(G8, (1, 2, 16), 'Phase M5b-3b', True, expect_macs=600)
# the holes: G1's image is a proper subset of the shadow interval
bm1, bf1, bo1 = G1.brute()
assert len(bo1) < max(bo1) - min(bo1) + 1, 'G1 must have holes'
print(f'G1 holes: {max(bo1) - min(bo1) + 1 - len(bo1)} values of the shadow [{min(bo1)}, {max(bo1)}] are not outputs')
for name, h in hdr.items():
    print(f'   {name} Kdev=1 nest: ' + ' | '.join(x[:58] for x in h))

# ---- refusals leave the IR byte-identical -------------------------------
def refused(name, text, reason, **kw):
    ir, log = run(name, text, flag=True, **kw)
    assert 'systolic.matmul_tile' not in ir and KC not in log, (name, 'must not offload')
    base, _ = run(name + '-off', text, flag=False, fleet=kw.get('fleet', True))
    assert ir == base, (name, 'refusal must leave the IR byte-identical')
    assert ir.count('arith.mulf') == 1 and ir.count('affine.store') == 1, (name, 'source MAC body')
    if reason:
        assert reason in log, (name, reason, log)


# empty domains (zero-trip reduction loop): the M1 count returns 0/0 for ANY
# empty domain (box or not), so both the rank-2 det-9 and the non-box holes
# fixture reach the F3 refusal before the guarded analysis (whose own
# "empty source domain" decline is the second line of defense).
DET9 = module('det9', '40x40', '40x40', '40x40', 'affine.for %i = 0 to 9 { affine.for %j = 0 to 9 { affine.for %k = 5 to 5 {', '}}}',
              '%i + %j - 2 * %k + 18, %i - 2 * %j + %k + 18', '%i + %j - 2 * %k + 18, %i + %j + %k',
              '%i + %j + %k, %i - 2 * %j + %k + 18')
refused('H-empty-M1', DET9, DECLINE + 'empty source domain (zero-trip loop nest)')
refused('H-empty-holes', HOLES.replace('affine.for %k = 0 to 9 {', 'affine.for %k = 3 to 3 {'),
        DECLINE + 'empty source domain (zero-trip loop nest)')
refused('H-strict', HOLES.replace(' fastmath<reassoc>', ''), DECLINE + 'floating-point reassociation is not permitted')
refused('H-alias', HOLES.replace(' {llvm.noalias}', ''), DECLINE + 'general affine factors may alias the output')
refused('H-nofleet', HOLES, DECLINE + 'kernel-coset offload requires a declared fleet', fleet=False)
refused('H-kdev0', HOLES, DECLINE + 'kernel-coset Kdev must be >= 1', kdev=0)
refused('H-overflow', HOLES.replace('2 * %i + 3 * %j', '4611686018427387904 * %i + 3 * %j'), '')
print('H refusals (empty on both paths, strict FP, aliasing, no fleet, Kdev 0, overflow): '
      'no tile, IR byte-identical to flag-off: PASS')

# ---- unchanged ownership / committed paths --------------------------------
SKEW = module('skew', '16', '16x16', '16x16', 'affine.for %i = 0 to 4 { affine.for %j = 0 to 4 { affine.for %k = 0 to 4 {',
              '}}}', '(((%i + %j) - (2 * %k)) + 6)', '%i, %k', '%k, %j')
ir, log = run('U-skew', SKEW, flag=True, kdev=1)
assert KC in log and '(Phase M4b.2)' in log and GUARD not in log and 'affine.if' not in ir
j, macs = kernel_coset_macs(ir)
assert Counter(macs) == Counter(skew_source()) and len(macs) == 64 and ir.count('affine.for') == 3
for name, text in (('U-boxGEMM', module('g', '8x8', '8x9', '9x8', *BOX8, '%i, %j', '%i, %k', '%k, %j')),
                   ('U-C2i3j-box', module('b', '40', '8x9', '9x8', *BOX8, '2 * %i + 3 * %j', '%i, %k', '%k, %j'))):
    off_ir, _ = run(name + '-off', text, flag=False)
    on_ir, on_log = run(name + '-on', text, flag=True)
    assert 'systolic.matmul_tile' in off_ir and on_ir == off_ir and KC not in on_log, (name, 'ownership')
print('unchanged: committed rank-one skew on the M4b.2 path (exact 64, no guard); box GEMM and box '
      'C[2i+3j] owned by existing routes, byte-identical under the flag: PASS')
