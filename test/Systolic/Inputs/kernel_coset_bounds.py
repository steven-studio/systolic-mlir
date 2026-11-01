"""Phase F-KC-1: the kernel-coset offload proves every physical A/B/C access
inside its declared STATIC memref extents over the EXACT source iteration
domain before any IR is touched, and fails closed otherwise.

The proof (RankPartitionKernelCosetBounds.h) is the integer emptiness of
D ∩ {f_q(x) <= -1} and D ∩ {f_q(x) >= d_q} for every operand access dimension
q, over the source domain D (loop bounds AND affine guards) -- never a bounding
box of the loops or of the generated tiles.

Oracle (TEST ONLY): a brute enumeration of the source domain evaluating every
access index against the declared extents (`violations`); for accepted cases
the emitted nest is additionally expanded by kernel_coset_macs and compared
with the source MAC multiset, per-output fibers and C store/load sets.

Accepted (every access proven in bounds; exact at Kdev 1 and 2):
  P1 C[2i+3j] over the triangle j<=i, C sized exactly 36 (max 35)
  P2 loops i<16, j<=i, k<9 with the guard i<=7; A 8x9, B 9x8, C 36: the loop
     bounding box would be out of bounds for A, B and C, the executed domain is not
  P3 CASE5 shape C[2i+3j][0] += A[2i+3j][k]*B[k][0] with tight 36x1 / 36x9 / 9x1
Refused, out of bounds (the oracle finds a violating source point; no tile; IR
byte-identical to flag-off; the remark names the operand and index):
  N1 A[i-1][k]            negative A index
  N2 A declared 7x9       A upper bound
  N3 B[k-1][j]            B lower bound
  N4 B declared 9x7       B upper bound (j reaches 7)
  N5 C[2i+3j-1]           C lower bound
  N6 C declared 35        C upper bound (only i=j=7 violates)
  N7 A declared 8x8       output image valid, operand A column k reaches 8
  N8 C[2i+3j][1] in 36x1  2-D C column violation
Refused, UNKNOWN (no static extent; nothing is manufactured; IR byte-identical):
  U1 A memref<?x9xf32>    U2 C memref<?xf32>
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
P1_TEXT = '\n'.join(x for x in Path(fixture).read_text().splitlines()
                    if not x.startswith('//'))

KC = 'lowered by the experimental symbolic kernel-coset offload'
GUARD = 'affine.if existence guard'
DECLINE = 'kernel-coset offload not applied: '
GATE = 'static memref bounds not proven over the source domain (Phase F-KC-1): '


def run(name, text, flag, kdev=1):
    src = root / (name + '.mlir'); dst = root / (name + '.out.mlir')
    src.write_text(text)
    o = 'general-affine=true allow-fp-reassociation=true report=true'
    if flag:
        o += f' kernel-coset-offload=true kernel-coset-kdev={kdev}'
    cmd = [opt, '--systolic-declare-fpga-fleet', '--affine-matmul-to-systolic=' + o, str(src), '-o', str(dst)]
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
    return tuple(None if v == '?' else int(v) for v in s.split('x'))


def lin(idx, shp):
    a, stride = 0, 1
    for v, sz in zip(reversed(idx), reversed(shp)):
        a += v * stride; stride *= sz
    return a


class Stmt:
    def __init__(self, name, text, points, cshape, ashape, bshape, cidx, aidx, bidx):
        self.name, self.text, self.points = name, text, points
        self.shapes = (shape(cshape), shape(ashape), shape(bshape))
        self.fns = (cidx, aidx, bidx)

    def violations(self):
        """Brute oracle: every (operand, dim, value) outside [0, extent) over the
        source domain; a dynamic extent is reported as ('dynamic', ...)."""
        out = []
        for x in self.points():
            for op, (fn, shp) in enumerate(zip(self.fns, self.shapes)):
                for q, (v, sz) in enumerate(zip(fn(*x), shp)):
                    if sz is None:
                        out.append(('dynamic', 'CAB'[op], q))
                    elif not 0 <= v < sz:
                        out.append(('CAB'[op], q, v))
        return out

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


TRI8 = ('affine.for %i = 0 to 8 { affine.for %j = 0 to affine_map<(d0) -> (d0 + 1)>(%i) '
        '{ affine.for %k = 0 to 9 {', '}}}')
TRI16_GATE7 = ('affine.for %i = 0 to 16 { affine.for %j = 0 to affine_map<(d0) -> (d0 + 1)>(%i) '
               '{ affine.for %k = 0 to 9 { affine.if affine_set<(d0) : (7 - d0 >= 0)>(%i) {', '}}}}')
tri = lambda: ((i, j, k) for i in range(8) for j in range(i + 1) for k in range(9))
C1 = lambda i, j, k: (2 * i + 3 * j,)
A_ik = lambda i, j, k: (i, k)
B_kj = lambda i, j, k: (k, j)


def stmt(name, cshape, ashape, bshape, loops, cidx, aidx, bidx, points, cfn, afn, bfn):
    return Stmt(name, module(name.split('-')[0].lower(), cshape, ashape, bshape, *loops, cidx, aidx, bidx),
                points, cshape, ashape, bshape, cfn, afn, bfn)


P1 = Stmt('P1-tri-exact-C', P1_TEXT, tri, '36', '8x9', '9x8', C1, A_ik, B_kj)
P2 = stmt('P2-gated-i-le-7', '36', '8x9', '9x8', TRI16_GATE7, '2 * %i + 3 * %j', '%i, %k', '%k, %j',
          lambda: ((i, j, k) for i in range(16) for j in range(i + 1) for k in range(9) if i <= 7),
          C1, A_ik, B_kj)
P3 = stmt('P3-case5-tight', '36x1', '36x9', '9x1', TRI8, '2 * %i + 3 * %j, 0', '2 * %i + 3 * %j, %k', '%k, 0',
          tri, lambda i, j, k: (2 * i + 3 * j, 0), lambda i, j, k: (2 * i + 3 * j, k), lambda i, j, k: (k, 0))


def check_exact(stmt, kdevs=(1, 2)):
    assert stmt.violations() == [], (stmt.name, 'the oracle must find every access in bounds')
    off_ir, _ = run(stmt.name + '-off', stmt.text, flag=False)
    assert 'systolic.matmul_tile' not in off_ir, (stmt.name, 'an existing route owns it')
    bm, bf, bo = stmt.brute()
    for kdev in kdevs:
        ir, log = run(f'{stmt.name}-k{kdev}', stmt.text, flag=True, kdev=kdev)
        assert KC in log and GATE not in log, (stmt.name, kdev, 'must be accepted', log)
        assert 'systolic.matmul_tile' in ir and 'arith.mulf' not in ir, (stmt.name, kdev, 'body')
        trace = {}
        jobs, macs = kernel_coset_macs(ir, trace)
        assert Counter(macs) == bm, (stmt.name, kdev, 'MAC multiset')
        gf = defaultdict(Counter)
        for a, b, c in macs:
            gf[c][(a, b)] += 1
        assert gf == bf, (stmt.name, kdev, 'per-output fibers')
        stores = [addr for role, addr in trace.get('store', []) if role == 0]
        loads = [addr for role, addr in trace.get('load', []) if role == 0]
        assert set(stores) == bo and len(stores) == len(bo), (stmt.name, kdev, 'C stores')
        assert set(loads) == bo and len(loads) == len(bo), (stmt.name, kdev, 'C loads')
        ks = {int(x.split('=')[1]) for x in re.findall(r'k = [0-9]+', ir)}
        assert max(ks) <= 16, (stmt.name, kdev, 'k_max', ks)
    print(f'{stmt.name}: every A/B/C access proven in bounds over the exact source domain; '
          f'{len(bo)} outputs / {sum(bm.values())} MACs exact at Kdev {list(kdevs)}: PASS')


check_exact(P1)
check_exact(P2)
check_exact(P3)


def refused(stmt, operand, dim, kind):
    """kind: 'negative' | 'upper' | 'dynamic'."""
    v = stmt.violations()
    if kind == 'dynamic':
        assert any(x[0] == 'dynamic' and x[1] == operand and x[2] == dim for x in v), (stmt.name, 'oracle: dynamic extent')
        reason = GATE + f'operand {operand} index {dim}: dynamic memref extent'
    else:
        bad = [x for x in v if x[0] == operand and x[1] == dim]
        assert bad, (stmt.name, 'the oracle must find a violating source point', v[:3])
        if kind == 'negative':
            assert any(x[2] < 0 for x in bad), (stmt.name, 'oracle: a negative index')
            reason = GATE + f'operand {operand} index {dim} is negative at some point of the source domain'
        else:
            ext = stmt.shapes['CAB'.index(operand)][dim]
            assert any(x[2] >= ext for x in bad), (stmt.name, 'oracle: an index >= extent')
            reason = GATE + f'operand {operand} index {dim} reaches or exceeds the extent {ext}'
    ir, log = run(stmt.name, stmt.text, flag=True)
    assert 'systolic.matmul_tile' not in ir and KC not in log, (stmt.name, 'must not offload')
    base, _ = run(stmt.name + '-off', stmt.text, flag=False)
    assert ir == base, (stmt.name, 'refusal must leave the IR byte-identical to flag-off')
    assert 'systolic.matmul_tile' not in base, (stmt.name, 'no existing route owns it either')
    assert ir.count('arith.mulf') == 1 and ir.count('affine.store') == 1, (stmt.name, 'source MAC body kept')
    assert reason in log, (stmt.name, reason, log)
    print(f'{stmt.name}: refused ({reason[len(GATE):]}), no tile, IR byte-identical: PASS')


refused(stmt('N1-A-negative', '36', '8x9', '9x8', TRI8, '2 * %i + 3 * %j', '%i - 1, %k', '%k, %j',
             tri, C1, lambda i, j, k: (i - 1, k), B_kj), 'A', 0, 'negative')
refused(stmt('N2-A-upper', '36', '7x9', '9x8', TRI8, '2 * %i + 3 * %j', '%i, %k', '%k, %j',
             tri, C1, A_ik, B_kj), 'A', 0, 'upper')
refused(stmt('N3-B-lower', '36', '8x9', '9x8', TRI8, '2 * %i + 3 * %j', '%i, %k', '%k - 1, %j',
             tri, C1, A_ik, lambda i, j, k: (k - 1, j)), 'B', 0, 'negative')
refused(stmt('N4-B-upper', '36', '8x9', '9x7', TRI8, '2 * %i + 3 * %j', '%i, %k', '%k, %j',
             tri, C1, A_ik, B_kj), 'B', 1, 'upper')
refused(stmt('N5-C-lower', '36', '8x9', '9x8', TRI8, '2 * %i + 3 * %j - 1', '%i, %k', '%k, %j',
             tri, lambda i, j, k: (2 * i + 3 * j - 1,), A_ik, B_kj), 'C', 0, 'negative')
refused(stmt('N6-C-upper', '35', '8x9', '9x8', TRI8, '2 * %i + 3 * %j', '%i, %k', '%k, %j',
             tri, C1, A_ik, B_kj), 'C', 0, 'upper')
refused(stmt('N7-A-col-upper', '36', '8x8', '9x8', TRI8, '2 * %i + 3 * %j', '%i, %k', '%k, %j',
             tri, C1, A_ik, B_kj), 'A', 1, 'upper')
refused(stmt('N8-C2d-col-upper', '36x1', '36x9', '9x1', TRI8, '2 * %i + 3 * %j, 1', '2 * %i + 3 * %j, %k', '%k, 0',
             tri, lambda i, j, k: (2 * i + 3 * j, 1), lambda i, j, k: (2 * i + 3 * j, k), lambda i, j, k: (k, 0)),
        'C', 1, 'upper')
refused(stmt('U1-A-dynamic', '36', '?x9', '9x8', TRI8, '2 * %i + 3 * %j', '%i, %k', '%k, %j',
             tri, C1, A_ik, B_kj), 'A', 0, 'dynamic')
refused(stmt('U2-C-dynamic', '?', '8x9', '9x8', TRI8, '2 * %i + 3 * %j', '%i, %k', '%k, %j',
             tri, C1, A_ik, B_kj), 'C', 0, 'dynamic')
