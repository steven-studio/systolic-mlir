"""Phase M4d-1 candidate-classification audit (additive, analysis-only).

Runs the real compiler on the CASE1 det-9 base and its singular-output and
repeated(9) variants, at default and with kernel-coset-offload set, and asserts
the CURRENT routing and the correctness/safety facts that gate M4d-2:

  base (rank-2 output)        default: enumeration (dense_ir, 64/37)
                              kc flag: kernel-coset DECLINES "scan plan UNKNOWN"
                                       -> still enumeration, IR UNCHANGED
  singular-output (rank-1)    default: enumeration (dense_ir, 64/13)
                              kc flag: kernel-coset CLAIMS it (symbolic nest)
                                       -> dense_ir CANNOT parse it, but
                                          kernel_coset_macs validates the SAME
                                          64 MACs / 13 fibers / per-fiber multiset
  repeated(9) (rank-2, 576)   default: refused (over pointBudget)
                              kc flag: still refused (kernel-coset declines
                                       rank-2 before enumeration -> budget NOT
                                       bypassed)

No production behavior, option default, or existing expectation is changed; the
expected MAC values come from the committed general_affine_oracle.source().
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


def run(name, text, kc):
    src = root / (name + '.mlir'); dst = root / (name + '.out.mlir')
    src.write_text(text)
    o = 'general-affine=true report=true classify=true'
    if kc:
        o += ' kernel-coset-offload=true kernel-coset-kdev=2'
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


KC = 'lowered by the experimental symbolic kernel-coset offload'

# A rank-1, kappa=2 singular skew: the one class every existing route refuses
# and the certified kernel-coset owns (SAFE_SYMBOLIC).
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

# ---- EXISTING_PATH_REQUIRED (rank-2 base): existing route owns; byte-identical
# under the flag; kernel-coset never claims it.
exp_base = source(1, 16)
d_ir, _ = run('base', base, kc=False)
k_ir, k_log = run('base', base, kc=True)
assert k_ir == d_ir, 'base: flag must not change existing-owned IR (no steal, no double)'
assert KC not in k_log, 'base must not be claimed by kernel-coset'
assert Counter(dense_ir(d_ir, 16)[1]) == Counter(exp_base), 'base MACs (enumeration)'
print('EXISTING_PATH_REQUIRED base (rank-2): existing route owns, byte-identical under flag: PASS')

# ---- EXISTING_PATH_REQUIRED (rank-1 singular-output): enumeration owns it; the
# refusal-only dispatch keeps it there (Test C) -- NOT migrated to kernel-coset.
exp_sing = [(a, b, (c // 16) * 16) for a, b, c in exp_base]
d_ir, _ = run('singular', singular, kc=False)
k_ir, k_log = run('singular', singular, kc=True)
assert k_ir == d_ir, 'singular: flag must not change enumeration-owned IR'
assert KC not in k_log, 'singular must NOT be claimed by kernel-coset under refusal-only'
assert Counter(dense_ir(d_ir, 16)[1]) == Counter(exp_sing) and len(grouping(exp_sing)) == 13
print('EXISTING_PATH_REQUIRED singular-output (rank-1): enumeration owns (dense_ir 64/13), '
      'unchanged under flag -- not migrated: PASS')

# ---- SAFE_SYMBOLIC (refused-but-supported skew): existing routes refuse; the
# kernel-coset claims it and lowers it exactly, with NO source-point enumeration.
off_ir, _ = run('skew', SKEW, kc=False)
assert 'systolic.matmul_tile' not in off_ir, 'skew must be refused by every existing route'
on_ir, on_log = run('skew', SKEW, kc=True)
assert KC in on_log and 'systolic.matmul_tile' in on_ir, 'skew must be claimed by kernel-coset'
j, macs = kernel_coset_macs(on_ir)
assert Counter(macs) == Counter(skew_source()) and grouping(macs) == grouping(skew_source())
assert len(macs) == 64 and len(grouping(macs)) == 13, 'skew exact 64/13'
# no enumeration / per-point / per-fiber materialization: symbolic affine nest,
# a handful of static tiles (not 64 enumerated points, not 13 per-fiber tiles).
assert 'affine.for' in on_ir and on_ir.count('systolic.matmul_tile') <= 4, 'symbolic, not enumerated'
print('SAFE_SYMBOLIC skew (refused by existing): kernel-coset claims, exact 64/13, '
      'symbolic (no enumeration): PASS')

# ---- UNSUPPORTED / pointBudget: repeated(9) refused by both; budget unchanged
r9 = repeated(9)
assert 'systolic.matmul_tile' not in run('rep9', r9, kc=False)[0], 'repeated(9) refused by default (over budget)'
k_ir, k_log = run('rep9', r9, kc=True)
assert 'systolic.matmul_tile' not in k_ir and KC not in k_log, 'repeated(9) stays refused; budget not bypassed'
print('UNSUPPORTED repeated(9) (rank-2, 576>512): refused by both, pointBudget not bypassed: PASS')

# ---- no double ownership: only the refused skew is claimed; existing-owned
# base and singular are not (their IR is byte-identical under the flag above).
assert KC in run('skew', SKEW, kc=True)[1]
assert KC not in run('base', base, kc=True)[1] and KC not in run('singular', singular, kc=True)[1]
print('no double ownership: kernel-coset claims only the genuinely-refused skew: PASS')
