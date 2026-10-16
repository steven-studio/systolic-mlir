// Loop directions and steps at the MLIR level, in the form the C front end
// emits (affine.for only counts up: a C loop that counts down becomes the
// ordinal loop 0 to N and its counter the affine expression LB - S * t).
// getDenseLogicalGemm reads a loop along which a coordinate decreases
// backwards (offset' = the smallest value, scale = |coefficient|), so the
// coordinate's values -- not their visiting order -- define each operand's
// strided view. The row sets below are what the C loops visit:
//   up_step2        1 to 6 step 2           rows 1, 3, 5
//   down_ge_odd     i = 5; i >= 0; i -= 2   rows 5, 3, 1 (the same rows)
//   down_gt_even    i = 6; i > 0;  i -= 2   rows 6, 4, 2
//   down_ge_even    i = 6; i >= 0; i -= 2   rows 6, 4, 2, 0
//   down_reduction  k = 7; k >= 0; --k      the reduction reversed
//   split_fine_down I = 4 ii + (3 - i)      rows 3 2 1 0 7 6 5 4 = 0..7
//   empty_up / empty_down                   no iteration: refused (SQ-2)
//   opposite_directions  A[7 - i] with C[i]: C row i reads A row 7 - i, a
//                        negative-stride view (docs/AFFINE_GEMM_MAPPING.md
//                        section 5): with the ordinal w (i's order), A row
//                        7 - w (offset 7, stride -1) and C row w (whole C). Refused until the
//                        P0 negative-stride support (2026-10-15).
//
// RUN: systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic=allow-fp-reassociation=true \
// RUN:   -split-input-file %s | FileCheck %s
// RUN: systolic-opt --systolic-declare-fpga-fleet "--affine-matmul-to-systolic=report=true allow-fp-reassociation=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=REMARK
//
// CHECK-LABEL: func.func @up_step2
// CHECK:       memref.subview %{{.*}}[1, 0] [3, 8] [2, 1]
// CHECK:       memref.subview %{{.*}}[1, 0] [3, 8] [2, 1]
// CHECK:       systolic.matmul_tile
// CHECK-LABEL: func.func @down_ge_odd
// CHECK-NOT:   affine.for
// CHECK:       memref.subview %{{.*}}[1, 0] [3, 8] [2, 1]
// CHECK:       memref.subview %{{.*}}[1, 0] [3, 8] [2, 1]
// CHECK:       systolic.matmul_tile
// CHECK-LABEL: func.func @down_gt_even
// CHECK-NOT:   affine.for
// CHECK:       memref.subview %{{.*}}[2, 0] [3, 8] [2, 1]
// CHECK:       memref.subview %{{.*}}[2, 0] [3, 8] [2, 1]
// CHECK-LABEL: func.func @down_ge_even
// CHECK-NOT:   affine.for
// CHECK:       memref.subview %{{.*}}[0, 0] [4, 8] [2, 1]
// CHECK:       memref.subview %{{.*}}[0, 0] [4, 8] [2, 1]
// CHECK-LABEL: func.func @down_reduction
// CHECK-NOT:   affine.for
// CHECK:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 8
// CHECK-LABEL: func.func @split_fine_down
// CHECK-NOT:   affine.for
// CHECK:       memref.subview %{{.*}}[0, 0] [8, 8] [1, 1]
// CHECK:       memref.subview %{{.*}}[0, 0] [8, 8] [1, 1]
// CHECK-LABEL: func.func @empty_up
// CHECK:       affine.for
// CHECK-NOT:   systolic.matmul_tile
// CHECK-LABEL: func.func @empty_down
// CHECK:       affine.for
// CHECK-NOT:   systolic.matmul_tile
// CHECK-LABEL: func.func @opposite_directions
// CHECK-NOT:   affine.for
// CHECK:       memref.subview %arg0[7, 0] [8, 8] [-1, 1]
// CHECK-NOT:   memref.subview %arg2
// CHECK:       systolic.matmul_tile
//
// REMARK: remark: affine GEMM 3x8x8 lowered as
// REMARK: remark: affine GEMM 3x8x8 lowered as
// REMARK: remark: affine GEMM 3x8x8 lowered as
// REMARK: remark: affine GEMM 4x8x8 lowered as
// REMARK: remark: affine GEMM 8x8x8 lowered as
// REMARK: remark: affine GEMM 8x8x8 lowered as
// REMARK: remark: affine GEMM not lowered to systolic.matmul_tile: a loop of the nest has no iteration (an empty GEMM: M, N or K is 0)
// REMARK: remark: affine GEMM not lowered to systolic.matmul_tile: a loop of the nest has no iteration (an empty GEMM: M, N or K is 0)
// REMARK: remark: affine GEMM 8x8x8 lowered as
// REMARK-NOT: decreases
// REMARK-NOT: remark:

// A: for (i = 1; i < 6; i += 2): rows 1, 3, 5
func.func @up_step2(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 1 to 6 step 2 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i, %k] : memref<8x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x8xf32>
        %c = affine.load %C[%i, %j] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<8x8xf32>
      }
    }
  }
  return
}

// -----
// B: for (i = 5; i >= 0; i -= 2), as the front end emits it: rows 5, 3, 1
func.func @down_ge_odd(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[(5 - %i * 2), %k] : memref<8x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x8xf32>
        %c = affine.load %C[(5 - %i * 2), %j] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[(5 - %i * 2), %j] : memref<8x8xf32>
      }
    }
  }
  return
}

// -----
// B: for (i = 6; i > 0; i -= 2): rows 6, 4, 2
func.func @down_gt_even(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[(6 - %i * 2), %k] : memref<8x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x8xf32>
        %c = affine.load %C[(6 - %i * 2), %j] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[(6 - %i * 2), %j] : memref<8x8xf32>
      }
    }
  }
  return
}

// -----
// B: for (i = 6; i >= 0; i -= 2): rows 6, 4, 2, 0
func.func @down_ge_even(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[(6 - %i * 2), %k] : memref<8x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x8xf32>
        %c = affine.load %C[(6 - %i * 2), %j] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[(6 - %i * 2), %j] : memref<8x8xf32>
      }
    }
  }
  return
}

// -----
// B: for (k = 7; k >= 0; --k): the reduction counts down
func.func @down_reduction(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i, (7 - %k)] : memref<8x8xf32>
        %b = affine.load %B[(7 - %k), %j] : memref<8x8xf32>
        %c = affine.load %C[%i, %j] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<8x8xf32>
      }
    }
  }
  return
}

// -----
// I = 4 ii + (3 - i): rows 3, 2, 1, 0, 7, 6, 5, 4 of a 16-row C
func.func @split_fine_down(%A: memref<16x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<16x8xf32> {llvm.noalias}) {
  affine.for %ii = 0 to 2 {
    affine.for %j = 0 to 8 {
      affine.for %i = 0 to 4 {
        affine.for %k = 0 to 8 {
          %a = affine.load %A[%ii * 4 + 3 - %i, %k] : memref<16x8xf32>
          %b = affine.load %B[%k, %j] : memref<8x8xf32>
          %c = affine.load %C[%ii * 4 + 3 - %i, %j] : memref<16x8xf32>
          %p = arith.mulf %a, %b : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%ii * 4 + 3 - %i, %j] : memref<16x8xf32>
        }
      }
    }
  }
  return
}

// -----
// C: for (i = 8; i < 8; ++i): no iteration
func.func @empty_up(%A: memref<?x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<?x8xf32> {llvm.noalias}) {
  affine.for %i = 8 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i, %k] : memref<?x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x8xf32>
        %c = affine.load %C[%i, %j] : memref<?x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<?x8xf32>
      }
    }
  }
  return
}

// -----
// D: for (i = 0; i > 0; --i), as the front end emits it: no iteration
func.func @empty_down(%A: memref<?x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<?x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 0 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[(0 - %i), %k] : memref<?x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x8xf32>
        %c = affine.load %C[(0 - %i), %j] : memref<?x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[(0 - %i), %j] : memref<?x8xf32>
      }
    }
  }
  return
}

// -----
// A's rows run down while C's run up (A[7 - i] with C[i]): no strided view of A
func.func @opposite_directions(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[(7 - %i), %k] : memref<8x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x8xf32>
        %c = affine.load %C[%i, %j] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<8x8xf32>
      }
    }
  }
  return
}
