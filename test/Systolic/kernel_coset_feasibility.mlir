// RUN: %python %S/Inputs/kernel_coset_feasibility.py systolic-opt %s %t
// Phase M5b-0 (finding F1 of the M5a design review): a candidate every existing
// route refuses (B is read on its diagonal, so no GEMM/GEMV role assignment
// exists; 576 MACs exceed the enumeration budget) reaches the kernel-coset
// path. Its physical-C row [1,1,0] (k absent) gives the HNF U rows [1,-1,0],
// [0,1,0],[0,0,1]: the reduced domain carries 0 <= y0 - y1 <= 3 as inequalities
// with ZERO tile coefficient, coupling the output coordinate y0 and the peel
// coordinate y1. Before M5b-0 the M4b.1 analysis dropped them ("captured by the
// outer box bounds") and the emitted nest executed the (y0, y1) chambers outside
// D' -- MACs the source never performs, reading A at a NEGATIVE row (the
// independent interpreter stopped at A[-1][0] on this very module). The runner
// checks the repaired behavior: refused with the parameter-only-constraint
// reason at Kdev 1 and 2, no matmul_tile, the source nest byte-identical to the
// flag-off output, and the axis-aligned skew (the committed class) still
// offloaded exactly.
module {
  func.func @diag(%C: memref<16xf32> {llvm.noalias}, %A: memref<8x9xf32> {llvm.noalias}, %B: memref<9x9xf32> {llvm.noalias}) {
    affine.for %i = 0 to 8 {
      affine.for %j = 0 to 8 {
        affine.for %k = 0 to 9 {
          %t0 = affine.load %C[%i + %j] : memref<16xf32>
          %t1 = affine.load %A[%i, %k] : memref<8x9xf32>
          %t2 = affine.load %B[%k, %k] : memref<9x9xf32>
          %t3 = arith.mulf %t1, %t2 fastmath<reassoc> : f32
          %t4 = arith.addf %t0, %t3 fastmath<reassoc> : f32
          affine.store %t4, %C[%i + %j] : memref<16xf32>
        }
      }
    }
    func.return
  }
}
