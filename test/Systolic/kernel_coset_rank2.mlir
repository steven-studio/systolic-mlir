// RUN: %python %S/Inputs/kernel_coset_rank2.py systolic-opt %s %t
// Phase M5b-2: exact symbolic rank-2 kernel-coset emission. The module is the
// det-9 rank-2 base C[i+j-2k+18][i-2j+k+18] over 0<=i,j,k<9 (729 MACs: over
// the enumeration budget, non-unit views: refused by every existing route),
// whose physical-C image is an index-3 lattice inside a non-box projection.
// The runner derives the other fixtures (triangular domains with transposed or
// non-unit output views, an output-dependent reduction bound, the 16^3 det-9
// case, the box-owned shapes, the rank-one regression fixtures and the
// refusals) and checks every claimed emission against an independent brute
// source: exact physical (A,B,C) MAC multiset, per-output multiplicities, the
// set and count of C read-modify-write visits (one per output, none outside
// the image), every tile k <= k_max, and byte-identical IR wherever the
// kernel-coset must not claim.
module {
  func.func @det9(%C: memref<40x40xf32> {llvm.noalias}, %A: memref<40x40xf32> {llvm.noalias}, %B: memref<40x40xf32> {llvm.noalias}) {
    affine.for %i = 0 to 9 {
      affine.for %j = 0 to 9 {
        affine.for %k = 0 to 9 {
          %t0 = affine.load %C[%i + %j - 2 * %k + 18, %i - 2 * %j + %k + 18] : memref<40x40xf32>
          %t1 = affine.load %A[%i + %j - 2 * %k + 18, %i + %j + %k] : memref<40x40xf32>
          %t2 = affine.load %B[%i + %j + %k, %i - 2 * %j + %k + 18] : memref<40x40xf32>
          %t3 = arith.mulf %t1, %t2 fastmath<reassoc> : f32
          %t4 = arith.addf %t0, %t3 fastmath<reassoc> : f32
          affine.store %t4, %C[%i + %j - 2 * %k + 18, %i - 2 * %j + %k + 18] : memref<40x40xf32>
        }
      }
    }
    func.return
  }
}
