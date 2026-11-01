"""Phase M4d-2: refusal-only symbolic kernel-coset dispatch regression.

Observable-behavior checks (no legality re-implementation). The kernel-coset
path is off by default and, when enabled, runs LAST and claims only candidates
every existing route refused:

  A base GEMM (rank-2)   existing route owns; flag IR == no-flag IR (no steal)
  C singular-output      enumeration owns; flag IR == no-flag IR; dense_ir ok
  D skew (rank-1, kappa=2) refused by existing; kernel-coset claims; exact 64/13
  E repeated(9)          refused by both (pointBudget not bypassed)
  F skew strict f32      kernel-coset declines (FP), body preserved
  G skew aliasing        kernel-coset declines, body preserved
  H flag off             byte-identical to baseline (the A/C/D/E no-flag runs)

B (guarded priority / no double ownership) is covered by A and C: an existing-
owned candidate produces identical IR with and without the flag, so kernel-coset
never steals or double-lowers it.
"""
from collections import Counter
from pathlib import Path
import re
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'SystolicCC' / 'Inputs'))
from general_affine_oracle import dense_ir, source, kernel_coset_macs, skew_source

opt, fixture, temp = sys.argv[1:4]
root = Path(temp); root.mkdir(parents=True, exist_ok=True)
base = '\n'.join(x for x in Path(fixture).read_text().splitlines()
                 if not x.startswith('//'))
singular = re.sub(r'(%C\[)(.*?)(\])',
                  lambda m: m[1] + m[2].rsplit(',', 1)[0] + ', 0' + m[3], base)


def repeated(count):
    s = base.index('    affine.for %i'); e = base.index('    func.return')
    return base[:s] + f'    affine.for %p = 0 to {count} {{\n' + base[s:e] + '    }\n' + base[e:]


SKEW = """module {
  func.func @skew(%C: memref<16xf32> {llvm.noalias}, %A: memref<16x16xf32> {llvm.noalias}, %B: memref<16x16xf32> {llvm.noalias}) {
    affine.for %i = 0 to 4 { affine.for %j = 0 to 4 { affine.for %k = 0 to 4 {
      %t0 = affine.load %C[(((%i + %j) - (2 * %k)) + 6)] : memref<16xf32>
      %t1 = affine.load %A[%i, %k] : memref<16x16xf32>
      %t2 = affine.load %B[%k, %j] : memref<16x16xf32>
      %t3 = arith.mulf %t1, %t2 fastmath<reassoc> : f32
      %t4 = arith.addf %t0, %t3 fastmath<reassoc> : f32
      affine.store %t4, %C[(((%i + %j) - (2 * %k)) + 6)] : memref<16xf32>
    }}}
    func.return
  }
}
"""
KC = 'lowered by the experimental symbolic kernel-coset offload'


def run(name, text, flag, extra=''):
    src = root / (name + '.mlir'); dst = root / (name + '.out.mlir')
    src.write_text(text)
    o = 'general-affine=true report=true classify=true'
    if flag:
        o += ' kernel-coset-offload=true kernel-coset-kdev=2'
    o += (' ' + extra) if extra else ''
    p = subprocess.run([opt, '--systolic-declare-fpga-fleet',
                        '--affine-matmul-to-systolic=' + o, str(src), '-o', str(dst)],
                       capture_output=True, text=True)
    assert p.returncode == 0, (name, p.stderr)
    return dst.read_text(), p.stderr


def grouping(points):
    f = {}
    for a, b, c in points:
        f.setdefault(c, Counter())[(a, b)] += 1
    return f


# ---- A + H + B(no-steal): rank-2 base owned by existing, identical under flag
off_ir, _ = run('base', base, flag=False)
on_ir, on_log = run('base', base, flag=True)
assert off_ir == on_ir, 'A/H base: flag must not change existing-owned IR'
assert KC not in on_log, 'A base: kernel-coset must not claim the rank-2 base'
assert Counter(dense_ir(on_ir, 16)[1]) == Counter(source(1, 16)), 'A base MACs'
print('A/B/H base (rank-2): existing route owns, byte-identical under flag, no steal: PASS')

# ---- C: singular-output owned by enumeration, identical under flag ---------
off_ir, _ = run('singular', singular, flag=False)
on_ir, on_log = run('singular', singular, flag=True)
assert off_ir == on_ir, 'C singular: flag must not change enumeration-owned IR'
assert KC not in on_log, 'C singular: kernel-coset must not claim it'
exp_s = [(a, b, (c // 16) * 16) for a, b, c in source(1, 16)]
assert Counter(dense_ir(on_ir, 16)[1]) == Counter(exp_s) and len(grouping(exp_s)) == 13
print('C singular-output: enumeration owns (dense_ir 64/13), unchanged under flag: PASS')

# ---- D: skew refused by existing, claimed + exact by kernel-coset ----------
off_ir, _ = run('skew', SKEW, flag=False)
assert 'systolic.matmul_tile' not in off_ir, 'D skew must be refused by existing routes'
on_ir, on_log = run('skew', SKEW, flag=True)
assert KC in on_log and 'systolic.matmul_tile' in on_ir, 'D skew must be claimed by kernel-coset'
j, macs = kernel_coset_macs(on_ir)
assert Counter(macs) == Counter(skew_source()) and grouping(macs) == grouping(skew_source())
assert len(macs) == 64 and len(grouping(macs)) == 13
print('D skew (rank-1, refused by existing): kernel-coset claims, exact 64/13: PASS')

# ---- E: repeated(9) refused by both (pointBudget not bypassed) -------------
r9 = repeated(9)
assert 'systolic.matmul_tile' not in run('rep9', r9, flag=False)[0], 'E rep9 default refused'
on_ir, on_log = run('rep9', r9, flag=True)
assert 'systolic.matmul_tile' not in on_ir and KC not in on_log, 'E rep9 must stay refused under flag'
print('E repeated(9): refused by both, pointBudget not bypassed: PASS')

# ---- F: strict FP skew declines (body preserved) ---------------------------
on_ir, on_log = run('skew-strict', SKEW.replace(' fastmath<reassoc>', ''), flag=True)
assert 'systolic.matmul_tile' not in on_ir and 'arith.mulf' in on_ir, 'F body preserved'
assert 'kernel-coset offload not applied: floating-point reassociation is not permitted' in on_log
print('F skew strict f32: kernel-coset declines (FP), body preserved: PASS')

# ---- G: aliasing skew declines (body preserved) ----------------------------
on_ir, on_log = run('skew-alias', SKEW.replace(' {llvm.noalias}', ''), flag=True)
assert 'systolic.matmul_tile' not in on_ir and 'arith.mulf' in on_ir, 'G body preserved'
assert 'kernel-coset offload not applied: general affine factors may alias the output' in on_log
print('G skew aliasing: kernel-coset declines, body preserved: PASS')
