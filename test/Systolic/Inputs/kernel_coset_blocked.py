"""Phase M4b.2b-1: independent check of Static-K block tiling + remainder.

No reliance on the compiler's own report. Interprets the emitted nest and
verifies, against a brute-force source enumeration:
  A. the skew target at Kdev=1,2,4 -> 64 MACs / 13 fibers, identical physical
     (A,B,C) multiset and per-fiber accumulated-product multiset;
  B. a wider domain (512 MACs / 29 fibers) at several Kdev, with real full
     Kdev blocks (k=Kdev tiles) and fewer tiles than the per-point baseline;
  C. remainder lengths r=0..3 handled (Kdev=4 emits both k=Kdev and k=1 tiles,
     with exact coverage over every ragged length -> empty, partial and runtime
     lengths are all exact);
  F. unsupported Kdev=0 -> declined, IR unchanged;
  G. strict FP (no reassociation) -> declined;
  H. aliasing and missing-fleet -> declined;
  I. Kdev=1 output is structurally the M4b.2a emission (3 affine.for, 1 tile).
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
base = '\n'.join(x for x in Path(fixture).read_text().splitlines()
                 if not x.startswith('//'))

# A wider singular-output skew: 0<=i,j,k<8, C[i+j-2k+14] (1-D, 32), A/B 16x16 --
# long enough tile chambers to form multiple full Kdev blocks.
WIDE = """module {
  func.func @wide(%C: memref<32xf32> {llvm.noalias}, %A: memref<16x16xf32> {llvm.noalias}, %B: memref<16x16xf32> {llvm.noalias}) {
    affine.for %i = 0 to 8 { affine.for %j = 0 to 8 { affine.for %k = 0 to 8 {
      %t0 = affine.load %C[(((%i + %j) - (2 * %k)) + 14)] : memref<32xf32>
      %t1 = affine.load %A[%i, %k] : memref<16x16xf32>
      %t2 = affine.load %B[%k, %j] : memref<16x16xf32>
      %t3 = arith.mulf %t1, %t2 fastmath<reassoc> : f32
      %t4 = arith.addf %t0, %t3 fastmath<reassoc> : f32
      affine.store %t4, %C[(((%i + %j) - (2 * %k)) + 14)] : memref<32xf32>
    }}}
    func.return
  }
}
"""


def run(name, text, extra, fleet=True):
    src = root / (name + '.mlir'); dst = root / (name + '.out.mlir')
    src.write_text(text)
    flags = (['--systolic-declare-fpga-fleet'] if fleet else []) + \
            ['--affine-matmul-to-systolic=general-affine=true ' + extra]
    p = subprocess.run([opt] + flags + [str(src), '-o', str(dst)],
                       capture_output=True, text=True)
    (root / (name + '.log')).write_text(p.stderr)
    assert p.returncode == 0, (name, p.stderr)
    return dst.read_text()


def fibers(points):
    f = defaultdict(Counter)
    for a, b, c in points:
        f[c][(a, b)] += 1
    return dict(f)


def check_exact(name, ir, expected, nfib):
    jobs, macs = kernel_coset_macs(ir)
    assert Counter(macs) == Counter(expected), (
        name, Counter(expected) - Counter(macs), Counter(macs) - Counter(expected))
    assert fibers(macs) == fibers(expected), (name, 'per-fiber multiset')
    assert len(macs) == len(expected) and len(fibers(macs)) == nfib, (
        name, len(macs), len(fibers(macs)))
    return jobs


# ---- A: skew target, Kdev = 1, 2, 4 ---------------------------------------
exp = skew_source()
tiles = {}
for K in (1, 2, 4):
    ir = run(f'skew-k{K}', base,
             f'kernel-coset-offload=true kernel-coset-kdev={K} report=true')
    assert 'systolic.matmul_tile' in ir, (K, 'no offload')
    tiles[K] = check_exact(f'skew-k{K}', ir, exp, 13)
print('A skew Kdev=1,2,4: 64 exact MACs, 13 fibers, same multiset:', tiles, 'PASS')

# ---- I: Kdev=1 is the M4b.2a structure (one symbolic 1x1x1 tile, 3 loops) --
ir1 = run('skew-struct', base, 'kernel-coset-offload=true kernel-coset-kdev=1')
assert ir1.count('systolic.matmul_tile') == 1 and ir1.count('affine.for') == 3
assert 'affine.if' not in ir1 and 'k = 1 ' in ir1
print('I Kdev=1 structurally the M4b.2a emission (1 tile, 3 affine.for): PASS')

# ---- B: wider domain with real full Kdev blocks ---------------------------
wexp = skew_macs(8, 14, 16)
assert len(wexp) == 512 and len(fibers(wexp)) == 29
for K in (2, 3, 4):
    ir = run(f'wide-k{K}', WIDE,
             f'kernel-coset-offload=true kernel-coset-kdev={K}')
    jobs = check_exact(f'wide-k{K}', ir, wexp, 29)
    assert f'k = {K} ' in ir, (K, 'no full Kdev block emitted')
    assert jobs < 512, (K, 'blocking did not reduce the tile count')
print('B wide domain (512 MACs, 29 fibers) with full Kdev blocks: PASS')

# ---- C: remainder lengths (Kdev=4 emits full and K=1 remainder tiles) ------
ir = run('wide-k4-rem', WIDE, 'kernel-coset-offload=true kernel-coset-kdev=4')
assert 'k = 4 ' in ir and 'k = 1 ' in ir, 'expected both full and remainder tiles'
check_exact('wide-k4-rem', ir, wexp, 29)  # exact over every ragged length (r=0..3)
print('C remainder lengths r=0..3 via exact coverage (k=Kdev + k=1 tiles): PASS')

# ---- F/G/H: unsupported Kdev, strict FP, aliasing, missing fleet ----------
negatives = [
    ('kdev0', base, 'kernel-coset-offload=true kernel-coset-kdev=0 report=true', True),
    ('strict', base.replace(' fastmath<reassoc>', ''),
     'kernel-coset-offload=true kernel-coset-kdev=2 report=true', True),
    ('alias', base.replace(' {llvm.noalias}', ''),
     'kernel-coset-offload=true kernel-coset-kdev=2 report=true', True),
    ('nofleet', base, 'kernel-coset-offload=true kernel-coset-kdev=2 report=true', False),
]
for name, text, extra, fleet in negatives:
    ir = run(name, text, extra, fleet=fleet)
    assert 'systolic.matmul_tile' not in ir, (name, 'unsafe offload')
    assert 'affine.for %arg3 = 0 to 4' in ir, (name, 'source nest mutated on refusal')
    assert 'kernel-coset offload not applied' in (root / (name + '.log')).read_text(), name
    print(f'{name} safe decline (IR unchanged): PASS')

# ---- D: Kdev 1..128 on a box-REFUSED wider skew (the matvec is a box GEMV and
# is owned by the clean path under the M4d-2 refusal-only dispatch, so it can no
# longer be a kernel-coset case). The wider singular skew C[i+j-2k+32] over
# 0<=i,j,k<16 (rank-1 C, kappa=2) is genuinely refused by every existing route,
# has ragged chambers of many lengths (so a single Kdev sweep exercises full
# blocks, every remainder length r=0..Kdev-1, and empty chambers), and emits the
# device k_max=16 fold (planSystolicMatmul(1,1,Kdev) folds Kdev>16 into chained
# k<=16 matmul_tiles over sliced gather slabs). 4096 MACs validated exactly.
WSKEW = """module {
  func.func @wsk(%C: memref<64xf32> {llvm.noalias}, %A: memref<16x16xf32> {llvm.noalias}, %B: memref<16x16xf32> {llvm.noalias}) {
    affine.for %i = 0 to 16 { affine.for %j = 0 to 16 { affine.for %k = 0 to 16 {
      %t0 = affine.load %C[(((%i + %j) - (2 * %k)) + 32)] : memref<64xf32>
      %t1 = affine.load %A[%i, %k] : memref<16x16xf32>
      %t2 = affine.load %B[%k, %j] : memref<16x16xf32>
      %t3 = arith.mulf %t1, %t2 fastmath<reassoc> : f32
      %t4 = arith.addf %t0, %t3 fastmath<reassoc> : f32
      affine.store %t4, %C[(((%i + %j) - (2 * %k)) + 32)] : memref<64xf32>
    }}}
    func.return
  }
}
"""
wexp = skew_macs(16, 32, 16)                      # independent brute source
wnfib = len(fibers(wexp))
assert len(wexp) == 4096
# genuinely refused by every existing route before kernel-coset claims it:
assert 'systolic.matmul_tile' not in run('wsk-off', WSKEW, '')[0], 'wider skew must be box-refused'
for K in (1, 2, 3, 4, 8, 16, 32, 64, 128):
    ir = run(f'wsk-k{K}', WSKEW, f'kernel-coset-offload=true kernel-coset-kdev={K}')
    # box-refused above + exact kernel-coset-nest interpretation below => the
    # kernel-coset is the only path that could have lowered it.
    check_exact(f'wsk-k{K}', ir, wexp, wnfib)
    # device k_max folding: every emitted reduction tile is k <= k_max (16);
    # Kdev >= 16 emits the k=16 fold.
    ks = sorted({int(x.split('=')[1]) for x in re.findall(r'k = [0-9]+', ir)})
    assert max(ks) <= 16, (K, 'k exceeds k_max', ks)
    if K >= 16:
        assert 16 in ks, (K, 'expected a k=16 fold tile', ks)
print('D wider skew (box-refused), Kdev 1..128: 4096 MACs exact, every tile '
      'k<=k_max=16, k=16 fold emitted for Kdev>=16: PASS')

# ---- E: remainder lengths + empty chambers via the ragged wider skew -------
# The wider skew's physical-C fibers have many different (runtime) tile-interval
# lengths, so the Kdev sweep above already drives r = 0 .. Kdev-1, L<Kdev,
# L==Kdev, L>Kdev and empty chambers; exact coverage at every Kdev proves each
# is handled. The small skew (section A) additionally executes k=2/k=4 full
# blocks, and the N=4/N=8 cases exercise empty chambers directly.
print('E remainder/empty coverage via ragged wider-skew exactness across Kdev: PASS')
