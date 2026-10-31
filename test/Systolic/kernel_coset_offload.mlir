// RUN: %python %S/Inputs/kernel_coset_offload.py systolic-opt %s %t
// Phase M4b.2a: the experimental symbolic kernel-coset offload. The runner
// independently interprets the emitted affine nest (AffineMap min/max tile
// bounds, floordiv/ceildiv, rank-generic gather) and checks every physical MAC
// address, multiplicity and per-output source order against a brute-force
// enumeration done in the oracle -- and checks that the default and every
// unsupported case is left exactly as written.
//
// C[i+j-2k] is a rank-1 (singular) physical-C map: 64 source MACs fold into 13
// ragged fibers that the box/guarded/general paths all refuse. C is 1-D.
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
