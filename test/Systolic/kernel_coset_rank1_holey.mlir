// RUN: %python %S/Inputs/kernel_coset_rank1_holey.py systolic-opt %s %t
// Phase M5b-3c: rank-one output images with holes or congruence restrictions.
// The module is the SystolicCC/general_affine.c CASE5 shape over the 8x8x9 box:
// C[2i+3j][0] += A[2i+3j][k] * B[k][0] (2-D C with a constant column; the fast
// path refuses it -- its logical row is driven by two loops -- and the guarded
// path's device-job and guarded-MAC decisions refuse it, so under the default
// policy it stays on the host). Its rank-one image {2i+3j} has holes (1 and 34
// on this box), so the committed M1 contiguity certificate does not apply; the
// Phase M5b-3a analysis scans the rational shadow of the output coordinate and
// the Phase M5b-3b emitter wraps the C load, the fiber nest and the C store in
// the affine.if integer existence guard. The runner derives the other rank-one
// fixtures (holey triangles with unit and non-unit H, an offset, larger
// coefficients, unit reduction gates), checks every emission against a brute
// enumeration of the source, and pins the still-UNKNOWN boundary (two non-unit
// kernel eliminations; locals) and the ownership of box images by the existing
// routes.
module {
  func.func @case5(%C: memref<40x16xf32> {llvm.noalias}, %A: memref<40x16xf32> {llvm.noalias}, %B: memref<16x16xf32> {llvm.noalias}) {
    affine.for %i = 0 to 8 {
      affine.for %j = 0 to 8 {
        affine.for %k = 0 to 9 {
          %t0 = affine.load %C[2 * %i + 3 * %j, 0] : memref<40x16xf32>
          %t1 = affine.load %A[2 * %i + 3 * %j, %k] : memref<40x16xf32>
          %t2 = affine.load %B[%k, 0] : memref<16x16xf32>
          %t3 = arith.mulf %t1, %t2 fastmath<reassoc> : f32
          %t4 = arith.addf %t0, %t3 fastmath<reassoc> : f32
          affine.store %t4, %C[2 * %i + 3 * %j, 0] : memref<40x16xf32>
        }
      }
    }
    func.return
  }
}
