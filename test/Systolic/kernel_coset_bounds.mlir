// RUN: %python %S/Inputs/kernel_coset_bounds.py systolic-opt %s %t
// Phase F-KC-1: static memref bounds gate of the kernel-coset offload. Before
// any IR is touched, every physical A/B/C access must be PROVEN inside its
// declared static memref extents over the exact source iteration domain (loop
// bounds and affine guards): for every access dimension q the sets
// D ∩ {f_q(x) <= -1} and D ∩ {f_q(x) >= d_q} must be integer-empty
// (RankPartitionKernelCosetBounds.h). Dynamic extents are UNKNOWN and refused;
// nothing is ever manufactured from a bounded-looking generated loop or tile.
// The module is the accepted case P1: C[2i+3j] += A[i][k]*B[k][j] over the
// triangle 0<=j<=i<8, 0<=k<9, with C sized EXACTLY 36 (the image's maximum is
// 35). The runner derives the other accepted cases (a guard that excludes the
// out-of-bounds part of the loop box; the CASE5 shape with tight shapes), the
// refused out-of-bounds cases for A, B and C on both sides, the 2-D C column,
// and the dynamic-extent UNKNOWNs, each checked against a brute enumeration of
// the source domain.
module {
  func.func @p1(%C: memref<36xf32> {llvm.noalias}, %A: memref<8x9xf32> {llvm.noalias}, %B: memref<9x8xf32> {llvm.noalias}) {
    affine.for %i = 0 to 8 {
      affine.for %j = 0 to affine_map<(d0) -> (d0 + 1)>(%i) {
        affine.for %k = 0 to 9 {
          %t0 = affine.load %C[2 * %i + 3 * %j] : memref<36xf32>
          %t1 = affine.load %A[%i, %k] : memref<8x9xf32>
          %t2 = affine.load %B[%k, %j] : memref<9x8xf32>
          %t3 = arith.mulf %t1, %t2 fastmath<reassoc> : f32
          %t4 = arith.addf %t0, %t3 fastmath<reassoc> : f32
          affine.store %t4, %C[2 * %i + 3 * %j] : memref<36xf32>
        }
      }
    }
    func.return
  }
}
