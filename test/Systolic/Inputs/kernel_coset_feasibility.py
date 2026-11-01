"""Phase M5b-0: fail-closed refusal of a coupled parameter-only constraint.

The fixture C[i+j] += A[i][k] * B[k][k] (0<=i,j<8, 0<=k<9) is refused by every
existing route (diagonal B view; 576 MACs over the enumeration budget) and so
reaches the kernel-coset path under the flag. Its reduced domain D' has
0 <= y0 - y1 <= 3 with zero tile coefficient (output/peel coupling): the M4b.1
analysis used to drop such rows, and the committed emitter then ran the
chambers outside D' (phantom MACs reading A at a negative row). Observable
checks (no legality re-implementation):
  A existing routes refuse the fixture (no matmul_tile, MAC body kept);
  B flag on, Kdev 1 and 2: the kernel-coset DECLINES with the parameter-only-
    constraint reason, emits no tile, and the output is byte-identical to the
    flag-off output (fail closed: no additional MAC, source nest preserved);
  C the committed axis-aligned skew C[i+j-2k+6] still offloads and is exact
    (64 MACs / 13 fibers vs the independent brute source) -- the fix does not
    over-refuse the existing class.
"""
from collections import Counter
from pathlib import Path
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'SystolicCC' / 'Inputs'))
from general_affine_oracle import kernel_coset_macs, skew_source

opt, fixture, temp = sys.argv[1:4]
root = Path(temp); root.mkdir(parents=True, exist_ok=True)
diag = '\n'.join(x for x in Path(fixture).read_text().splitlines()
                 if not x.startswith('//'))

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
REASON = ('kernel-coset offload not applied: a parameter-only constraint of the '
          'reduced domain couples the output and peel coordinates')


def run(name, text, flag, kdev=2):
    src = root / (name + '.mlir'); dst = root / (name + '.out.mlir')
    src.write_text(text)
    o = 'general-affine=true report=true'
    if flag:
        o += f' kernel-coset-offload=true kernel-coset-kdev={kdev}'
    p = subprocess.run([opt, '--systolic-declare-fpga-fleet',
                        '--affine-matmul-to-systolic=' + o, str(src), '-o', str(dst)],
                       capture_output=True, text=True)
    assert p.returncode == 0, (name, p.stderr)
    return dst.read_text(), p.stderr


# ---- A: every existing route refuses the fixture (reachability) ------------
off_ir, off_log = run('diag-off', diag, flag=False)
assert 'systolic.matmul_tile' not in off_ir, 'A: an existing route owns the fixture'
assert off_ir.count('arith.mulf') == 1 and off_ir.count('affine.for') == 3, 'A: body'
print('A diagonal-B C[i+j]: refused by every existing route, MAC body kept: PASS')

# ---- B: the kernel-coset is reached and refuses, fail closed ---------------
for kdev in (1, 2):
    on_ir, on_log = run(f'diag-k{kdev}', diag, flag=True, kdev=kdev)
    assert KC not in on_log, (kdev, 'B: must not offload')
    assert REASON in on_log, (kdev, 'B: coupled parameter-only constraint reason', on_log)
    assert 'systolic.matmul_tile' not in on_ir, (kdev, 'B: no tile')
    assert on_ir == off_ir, (kdev, 'B: flag-on output must be byte-identical to flag-off')
    assert on_ir.count('arith.mulf') == 1 and on_ir.count('affine.store') == 1, (kdev, 'B: one MAC')
print('B flag on (Kdev 1, 2): kernel-coset declines "parameter-only constraint couples '
      'output and peel", no tile, source IR byte-identical: PASS')

# ---- C: the committed axis-aligned class still offloads exactly ------------
on_ir, on_log = run('skew', SKEW, flag=True)
assert KC in on_log and 'systolic.matmul_tile' in on_ir, 'C: skew must still offload'
jobs, macs = kernel_coset_macs(on_ir)
assert Counter(macs) == Counter(skew_source()) and len(macs) == 64, 'C: skew exact 64'
assert len({c for _, _, c in macs}) == 13, 'C: 13 fibers'
print('C axis-aligned skew: still offloaded, exact 64 MACs / 13 fibers: PASS')
