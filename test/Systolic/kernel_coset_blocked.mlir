// RUN: %python %S/Inputs/kernel_coset_blocked.py systolic-opt %s %t
// Phase M4b.2b-1: Static-K block tiling with exact remainder handling. The
// runner independently interprets the emitted affine nest (step-Kdev block
// loop, affine.if full-block predicate, 1xK / Kx1 gather slabs, K=1 remainder
// loop) and checks every physical MAC address, multiplicity and per-fiber
// accumulation against a brute-force source enumeration, across Kdev values and
// a wider domain, plus that the default (Kdev=1) output and every unsupported
// case is unchanged / declined.
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
