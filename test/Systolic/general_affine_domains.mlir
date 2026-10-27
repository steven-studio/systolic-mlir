// RUN: %python %S/Inputs/general_affine_domains.py systolic-opt %s %t
// The runner independently checks physical MAC addresses, multiplicity and order.
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
