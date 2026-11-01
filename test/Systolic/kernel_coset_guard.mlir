// RUN: %python %S/Inputs/kernel_coset_guard.py systolic-opt %s %t
// Phase M5b-3b: guarded symbolic output lowering. The module is C[2i+3j] +=
// A[i][k]*B[k][j] over the triangle 0<=j<=i<12, 0<=k<9 (702 MACs: over the
// enumeration budget; non-box domain: refused by every existing route). Its
// rank-one image has HOLES (the values 2i+3j not attained on the triangle),
// so the committed M1 contiguity certificate does not apply; the Phase M5b-3a
// analysis scans the rational shadow of the output coordinate and the emitter
// wraps the C load, the fiber nest and the C store in an affine.if integer
// existence guard  AND_{i,j} floor(U_i/b_i) - ceil(L_j/a_j) >= 0  over the
// reduction coordinate's faces. The runner derives the other fixtures
// (non-box rank-2 image C[2i+3j][i-j], the equality i == 2k, the gate
// 2k <= i, the unit-elimination 2i+3j-k >= 0, the committed rank-one
// 2i+j-k path, empty domains, negative bounds, refusals, ownership) and checks
// every claimed emission against an independent brute source: exact physical
// (A,B,C) MAC multiset, per-output multiplicities, one C load and one store per
// output and none outside the image, every tile k <= k_max.
module {
  func.func @holes(%C: memref<60xf32> {llvm.noalias}, %A: memref<12x9xf32> {llvm.noalias}, %B: memref<9x12xf32> {llvm.noalias}) {
    affine.for %i = 0 to 12 {
      affine.for %j = 0 to affine_map<(d0) -> (d0 + 1)>(%i) {
        affine.for %k = 0 to 9 {
          %t0 = affine.load %C[2 * %i + 3 * %j] : memref<60xf32>
          %t1 = affine.load %A[%i, %k] : memref<12x9xf32>
          %t2 = affine.load %B[%k, %j] : memref<9x12xf32>
          %t3 = arith.mulf %t1, %t2 fastmath<reassoc> : f32
          %t4 = arith.addf %t0, %t3 fastmath<reassoc> : f32
          affine.store %t4, %C[2 * %i + 3 * %j] : memref<60xf32>
        }
      }
    }
    func.return
  }
}
