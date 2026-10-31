// RUN: %python %S/Inputs/kernel_coset_audit.py systolic-opt %s %t
// Phase M4d-1 candidate-classification audit (ANALYSIS, additive). Locks in the
// CURRENT production routing of the general-affine candidates relative to the
// off-by-default kernel-coset path, so any future M4d-2 dispatch change is
// detected. It changes no production behavior and no existing expectation: it
// runs the real compiler at default and (separately) with kernel-coset-offload
// set, and checks which path owns each candidate, that the single rank-1
// overlap (singular-output) lowers to the identical MAC multiset, and that the
// pointBudget negative (repeated(9)) is not bypassed. The module below is the
// CASE1 det-9 base; the runner derives the singular-output and repeated(9)
// variants with the same transforms general_affine_domains uses.
module {
  func.func @general(%C: memref<16x16xf32> {llvm.noalias}, %A: memref<16x16xf32> {llvm.noalias}, %B: memref<16x16xf32> {llvm.noalias}) {
    affine.for %i = 0 to 4 {
      affine.for %j = 0 to 4 {
        affine.for %k = 0 to 4 {
          %t0 = affine.load %C[(((%i + %j) - (2 * %k)) + 6), (((%i - (2 * %j)) + %k) + 6)] : memref<16x16xf32>
          %t1 = affine.load %A[(((%i + %j) - (2 * %k)) + 6), ((%i + %j) + %k)] : memref<16x16xf32>
          %t2 = affine.load %B[((%i + %j) + %k), (((%i - (2 * %j)) + %k) + 6)] : memref<16x16xf32>
          %t3 = arith.mulf %t1, %t2 fastmath<reassoc> : f32
          %t4 = arith.addf %t0, %t3 fastmath<reassoc> : f32
          affine.store %t4, %C[(((%i + %j) - (2 * %k)) + 6), (((%i - (2 * %j)) + %k) + 6)] : memref<16x16xf32>
        }
      }
    }
    func.return
  }
}
