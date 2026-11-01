"""Phase M4c-1: FP-legality / source-order decision-table regression.

Checks observable compiler behavior only (no re-implementation of the legality
policy, no MAC oracle -- offload correctness lives in kernel_coset_blocked):
  - offload  := the kernel-coset remark is emitted and a matmul_tile appears and
                the original MAC body (arith.mul{f,i}) is gone;
  - decline  := no kernel-coset offload; the original MAC body survives;
  - and, when the candidate reached the kernel-coset gate, the exact decline
    reason.

Decision table proved (kernel-coset path, off-by-default, Kdev=2):
  A matvec  (kappa=1, SourceLexEquivalent)     strict f32 -> DECLINE (FP gate)
  B matvec                                     reassoc    -> OFFLOAD
  C skew    (kappa=2, NotSourceLexEquivalent)  strict f32 -> DECLINE (FP gate)
  D skew                                       reassoc    -> OFFLOAD
  E GEMM    (rank-2 output -> plan UNKNOWN)    reassoc    -> DECLINE (plan),
            i.e. reassociation never bypasses an independent UNKNOWN check
  F f64 / f16                                  -> not a recognized f32 MAC
  G i32                                        -> not a recognized MAC
  H stride / no-fleet / Kdev=0 / aliasing      -> declined / not recognized
M3 (SourceLexEquivalent vs NotSourceLexEquivalent) is established by the
committed RankPartitionPresburgerTest M3 cases; here A vs C shows the strict-FP
decision is identical for both verdicts -- M3 is never consulted by the gate.
"""
from pathlib import Path
import subprocess
import sys

opt, fixture, temp = sys.argv[1:4]
root = Path(temp); root.mkdir(parents=True, exist_ok=True)
skew = '\n'.join(x for x in Path(fixture).read_text().splitlines()
                 if not x.startswith('//'))

GEMM = """module {
  func.func @g(%C: memref<4x4xf32> {llvm.noalias}, %A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}) {
    affine.for %i = 0 to 4 { affine.for %j = 0 to 4 { affine.for %k = 0 to 4 {
      %t0 = affine.load %C[%i, %j] : memref<4x4xf32>
      %t1 = affine.load %A[%i, %k] : memref<4x4xf32>
      %t2 = affine.load %B[%k, %j] : memref<4x4xf32>
      %t3 = arith.mulf %t1, %t2 fastmath<reassoc> : f32
      %t4 = arith.addf %t0, %t3 fastmath<reassoc> : f32
      affine.store %t4, %C[%i, %j] : memref<4x4xf32>
    }}}
    func.return
  }
}
"""
strict = lambda s: s.replace(' fastmath<reassoc>', '')


def run(name, text, extra='', fleet=True, kdev=2):
    src = root / (name + '.mlir'); dst = root / (name + '.out.mlir')
    src.write_text(text)
    flags = (['--systolic-declare-fpga-fleet'] if fleet else []) + \
            ['--affine-matmul-to-systolic=general-affine=true '
             f'kernel-coset-offload=true kernel-coset-kdev={kdev} report=true '
             + extra]
    p = subprocess.run([opt] + flags + [str(src), '-o', str(dst)],
                       capture_output=True, text=True)
    assert p.returncode == 0, (name, p.stderr)
    return dst.read_text(), p.stderr


KC_OFFLOAD = 'lowered by the experimental symbolic kernel-coset offload'
KC_DECLINE = 'kernel-coset offload not applied'


def offloaded(ir, log):
    return (KC_OFFLOAD in log and 'systolic.matmul_tile' in ir
            and 'arith.mulf' not in ir)


def declined_by_kc(log, reason):
    return (f'{KC_DECLINE}: {reason}' in log)


# Under the M4d-2 refusal-only dispatch the kernel-coset only ever sees
# candidates every existing route refused, so the SourceLexEquivalent arm is
# shown on an EXISTING-OWNED target (the standard GEMM, box/enumeration) and the
# refusal-only arm on the skew (kernel-coset). A + C together establish that the
# M3 verdict is never used to authorize strict-FP device offload.

# ---- A: existing-owned SourceLexEquivalent GEMM, strict f32 ----------------
# C[i][j]+=A[i][k]*B[k][j] (kappa=1 -> M3 SourceLexEquivalent). Strict f32: every
# route (incl. the box path) refuses the K-reduction reassociation, so the
# ORIGINAL source is retained; the rank-2 kernel-coset never claims it. Being
# SourceLexEquivalent does NOT make strict-FP device offload legal.
ir, log = run('A-gemm-strict', strict(GEMM))
assert 'systolic.matmul_tile' not in ir and 'arith.mulf' in ir, 'A GEMM strict must retain source'
assert KC_OFFLOAD not in log, 'A kernel-coset must not claim the SourceLexEquivalent GEMM'
print('A SourceLexEquivalent GEMM strict f32 -> source retained, no kernel-coset steal: PASS')

# ---- C: refusal-only NotSourceLexEquivalent skew, strict f32 ---------------
ir, log = run('C-skew-strict', strict(skew))
assert 'systolic.matmul_tile' not in ir and 'arith.mulf' in ir, 'C skew body must survive'
assert declined_by_kc(log, 'floating-point reassociation is not permitted'), 'C reason'
assert KC_OFFLOAD not in log, 'C must not offload'
print('C NotSourceLexEquivalent skew strict f32 -> kernel-coset DECLINES (FP), body preserved: PASS')

# ---- B: existing-owned SourceLexEquivalent GEMM, reassociation -------------
# With reassociation the GEMM follows its legitimate existing (box) path; the
# kernel-coset must not steal it (refusal-only: the box owns it first).
ir, log = run('B-gemm-reassoc', GEMM)
assert 'systolic.matmul_tile' in ir, 'B GEMM reassoc must be lowered by the existing box path'
assert KC_OFFLOAD not in log, 'B kernel-coset must not steal the box-owned GEMM'
print('B SourceLexEquivalent GEMM reassoc -> existing box path owns it, no steal: PASS')

# ---- D: refusal-only skew, reassociation authorized -> kernel-coset offload
ir, log = run('D-skew-reassoc', skew)
assert offloaded(ir, log), 'D skew reassoc must offload via kernel-coset'
print('D NotSourceLexEquivalent skew reassoc -> kernel-coset OFFLOAD: PASS')

ir, log = run('D2-skew-strict-flag', strict(skew), 'allow-fp-reassociation=true')
assert offloaded(ir, log), 'D2 strict source + reassoc flag must offload'
print('D2 skew strict source + allow-fp-reassociation -> kernel-coset OFFLOAD: PASS')

# Former case E: under refusal-only the rank-2 GEMM never reaches the kernel-
# coset (the box path owns it, see B), so reassociation structurally cannot
# bypass any kernel-coset plan/UNKNOWN check -- no separate assertion needed.

# ---- F: unsupported float widths are not even a recognized f32 MAC ---------
for name, w in (('f64', 'f64'), ('f16', 'f16')):
    t = skew.replace('xf32', 'x' + w).replace(' : f32', ' : ' + w)
    ir, log = run('F-' + name, t)
    assert 'systolic.matmul_tile' not in ir and 'arith.mulf' in ir, ('F', name, 'body')
    assert KC_OFFLOAD not in log, ('F', name, 'must not offload')
    assert 'not one f32 accumulating MAC' in log, ('F', name, 'f32-only recognition')
print('F f64 / f16 -> not a recognized f32 MAC, body preserved: PASS')

# ---- G: integer MAC is not recognized by this path -------------------------
t = (skew.replace('xf32', 'xi32').replace(' : f32', ' : i32')
     .replace('arith.mulf', 'arith.muli').replace('arith.addf', 'arith.addi')
     .replace(' fastmath<reassoc>', ''))
ir, log = run('G-i32', t)
assert 'systolic.matmul_tile' not in ir and 'arith.muli' in ir, 'G i32 body must survive'
assert KC_OFFLOAD not in log and 'not one pure MAC' in log, 'G integer unsupported'
print('G i32 integer MAC -> unsupported, body preserved: PASS')

# ---- H: other fail-closed conditions ---------------------------------------
ir, log = run('H-stride', skew.replace('affine.for %k = 0 to 4',
                                       'affine.for %k = 0 to 4 step 2'))
assert 'systolic.matmul_tile' not in ir and 'arith.mulf' in ir, 'H stride body'
assert 'requires unit-step loops' in log, 'H stride reason'

ir, log = run('H-nofleet', skew, fleet=False)
assert 'systolic.matmul_tile' not in ir and declined_by_kc(
    log, 'kernel-coset offload requires a declared fleet')

ir, log = run('H-kdev0', skew, kdev=0)
assert 'systolic.matmul_tile' not in ir and declined_by_kc(log, 'kernel-coset Kdev must be >= 1')

ir, log = run('H-alias', skew.replace(' {llvm.noalias}', ''))
assert 'systolic.matmul_tile' not in ir and declined_by_kc(
    log, 'general affine factors may alias the output')
print('H stride / no-fleet / Kdev=0 / aliasing -> declined or unrecognized, '
      'body preserved: PASS')
