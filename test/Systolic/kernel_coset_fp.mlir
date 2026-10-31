// RUN: %python %S/Inputs/kernel_coset_fp.py systolic-opt %s %t
// Phase M4c-1: FP-legality / source-order decision-table regression. The runner
// checks OBSERVABLE compiler behavior (offload vs decline, the decline reason,
// and whether the original MAC body survives) -- it does not re-implement the
// legality policy. It proves, in particular, that a SourceLexEquivalent target
// (the matrix-vector reduction, kappa=1) still DECLINES under strict f32: the
// M3 source-order verdict is never treated as a device reduction-order
// equivalence proof. The skew fixture below (kappa=2, NotSourceLexEquivalent,
// rank-1 C) is the base; the runner derives the element-type / stride / rank-2
// variants from it and an embedded matrix-vector and GEMM.
module {
  func.func @skew(%C: memref<16xf32> {llvm.noalias}, %A: memref<16x16xf32> {llvm.noalias}, %B: memref<16x16xf32> {llvm.noalias}) {
    affine.for %i = 0 to 4 {
      affine.for %j = 0 to 4 {
        affine.for %k = 0 to 4 {
          %t0 = affine.load %C[(((%i + %j) - (2 * %k)) + 6)] : memref<16xf32>
          %t1 = affine.load %A[%i, %k] : memref<16x16xf32>
          %t2 = affine.load %B[%k, %j] : memref<16x16xf32>
          %t3 = arith.mulf %t1, %t2 fastmath<reassoc> : f32
          %t4 = arith.addf %t0, %t3 fastmath<reassoc> : f32
          affine.store %t4, %C[(((%i + %j) - (2 * %k)) + 6)] : memref<16xf32>
        }
      }
    }
    func.return
  }
}
