// RUN: %python %S/Inputs/kernel_coset_dispatch.py systolic-opt %s %t
// Phase M4d-2: refusal-only symbolic kernel-coset dispatch. The runner checks
// observable behavior: the kernel-coset path (off by default) runs LAST and
// only claims a candidate every existing clean/guarded/enumeration route
// refused; a candidate an existing route owns keeps byte-identical IR under the
// flag (no ownership steal, no double lowering). The module is the CASE1 det-9
// base (rank-2, enumeration-owned); the runner derives singular-output and
// repeated(9) from it and embeds a rank-1 skew (the one genuinely refused case
// the certified kernel-coset path owns).
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
