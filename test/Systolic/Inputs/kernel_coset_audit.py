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
from general_affine_oracle import dense_ir, source, kernel_coset_macs

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

# ---- base (rank-2 output): kernel-coset must NOT claim it; unchanged --------
exp_base = source(1, 16)
d_ir, _ = run('base', base, kc=False)
assert Counter(dense_ir(d_ir, 16)[1]) == Counter(exp_base), 'base default'
k_ir, k_log = run('base', base, kc=True)
assert KC not in k_log, 'base must not be claimed by kernel-coset'
assert 'kernel-coset offload not applied: kernel-coset scan plan UNKNOWN' in k_log, 'base rank-2 decline'
assert Counter(dense_ir(k_ir, 16)[1]) == Counter(exp_base), 'base under flag unchanged'
print('EXISTING_PATH_REQUIRED base (rank-2): kernel-coset declines, enumeration owns, unchanged: PASS')

# ---- singular-output (rank-1): the single overlap -------------------------
exp_sing = [(a, b, (c // 16) * 16) for a, b, c in exp_base]
d_ir, _ = run('singular', singular, kc=False)
assert Counter(dense_ir(d_ir, 16)[1]) == Counter(exp_sing) and len(grouping(exp_sing)) == 13, 'singular default'
k_ir, k_log = run('singular', singular, kc=True)
assert KC in k_log, 'singular IS claimed by kernel-coset under the flag'
# the committed general_affine_domains oracle (dense_ir) cannot read the nest...
try:
    dense_ir(k_ir, 16); parsed = True
except AssertionError:
    parsed = False
assert not parsed, 'dense_ir unexpectedly parsed the symbolic nest'
# ...but the symbolic oracle validates the IDENTICAL MAC multiset / grouping.
j, macs = kernel_coset_macs(k_ir)
assert Counter(macs) == Counter(exp_sing), 'singular kernel-coset MAC multiset must equal enumeration'
assert grouping(macs) == grouping(exp_sing) and len(grouping(macs)) == 13, 'singular per-fiber multiset'
print('SYMBOLIC_BUT_ORACLE_UNSUPPORTED singular-output: kernel-coset exact (64/13), '
      'dense_ir cannot validate, kernel_coset_macs can: PASS')

# ---- repeated(9): pointBudget negative must NOT be bypassed ----------------
r9 = repeated(9)
d_ir, _ = run('rep9', r9, kc=False)
assert 'systolic.matmul_tile' not in d_ir, 'repeated(9) must be refused by default (over budget)'
k_ir, k_log = run('rep9', r9, kc=True)
assert 'systolic.matmul_tile' not in k_ir, 'repeated(9) must stay refused under the flag'
assert KC not in k_log, 'kernel-coset must not claim the rank-2 repeated(9) (would bypass budget)'
print('pointBudget preserved repeated(9) (rank-2, 576>512): refused by both, budget not bypassed: PASS')

# ---- no duplicate lowering: singular is rerouted exclusively; base is not --
assert KC in run('singular', singular, kc=True)[1] and KC not in run('base', base, kc=True)[1]
print('no duplicate ownership: flag reroutes only singular-output, base stays enumeration: PASS')
