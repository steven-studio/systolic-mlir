// One array as both factors (A x A), docs/AFFINE_GEMM_MAPPING.md section 7:
// the lowering serves both through ONE read-only tensor, so the first
// factor's (I, K) view and the second's (K, J) view must be the same element
// map. Each GEMM coordinate may be enumerated upward or downward -- a
// relabelling of the ordinals that changes no product and no sum --, so when
// the two views differ the lowering tries the seven other orientations
// (lowerOne, 2026-10-18: K alone first, then I, J, then pairs) and uses the
// first in which they coincide for all three operands; two different views of
// one memref in every orientation (A . A^T, a skew read by one factor only, a
// stride-2 view against its transpose) stay VALID_BUT_LOWERING_UNSUPPORTED.
// Mirrored by casespec.py (_one_tensor_serves_both,
// one_tensor_serves_both_factors).
//
// RUN: systolic-opt "--affine-matmul-to-systolic=allow-fp-reassociation=true" \
// RUN:   -split-input-file %s | FileCheck %s
// RUN: systolic-opt "--affine-matmul-to-systolic=report=true allow-fp-reassociation=true classify=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=REMARK

// The same skewed view read twice: A'(I, K) = A[I][I + K] and B'(K, J) =
// A[K][K + J] are both offset 0, strides (8, 1): one reinterpret_cast feeds
// both operands (review finding of 2026-10-18: the predictor refused it).
// CHECK-LABEL: func.func @axa_same_skew
// CHECK:       %[[V:.*]] = memref.reinterpret_cast %arg0 to offset: [0], sizes: [4, 4], strides: [8, 1]
// CHECK:       %[[T:.*]] = bufferization.to_tensor %[[V]]
// CHECK:       systolic.matmul_tile %[[T]], %[[T]], %{{.*}} m = 4 n = 4 k = 4
// REMARK:      affine GEMM 4x4x4 lowered as 1 exact region(s)
func.func @axa_same_skew(%A: memref<4x7xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %i + %k] : memref<4x7xf32>
        %b = affine.load %A[%k, %k + %j] : memref<4x7xf32>
        %c = affine.load %C[%i, %j] : memref<4x4xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<4x4xf32>
      }
    }
  }
  return
}

// -----
// Both factors read K downward through the same skew: with K enumerated
// downward both are A[p][p + q], offset 0, strides (8, 1); C's view is
// untouched (K alone is tried first).
// CHECK-LABEL: func.func @axa_both_rev_skew
// CHECK:       %[[V:.*]] = memref.reinterpret_cast %arg0 to offset: [0], sizes: [4, 4], strides: [8, 1]
// CHECK:       %[[T:.*]] = bufferization.to_tensor %[[V]]
// CHECK-NOT:   memref.subview
// CHECK:       systolic.matmul_tile %[[T]], %[[T]], %{{.*}} m = 4 n = 4 k = 4
// REMARK:      affine GEMM 4x4x4 lowered as 1 exact region(s)
func.func @axa_both_rev_skew(%A: memref<4x7xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %i + 3 - %k] : memref<4x7xf32>
        %b = affine.load %A[3 - %k, 3 - %k + %j] : memref<4x7xf32>
        %c = affine.load %C[%i, %j] : memref<4x4xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<4x4xf32>
      }
    }
  }
  return
}

// -----
// A[3 - i][k] * A[k][j]: with I enumerated downward both factors are the
// whole memref and C is written through a reversed view (rows 3..0).
// CHECK-LABEL: func.func @axa_rev_i
// CHECK:       %[[T:.*]] = bufferization.to_tensor %arg0 restrict
// CHECK:       memref.subview %arg1[3, 0] [4, 4] [-1, 1]
// CHECK:       systolic.matmul_tile %[[T]], %[[T]], %{{.*}} m = 4 n = 4 k = 4
// REMARK:      affine GEMM 4x4x4 lowered as 1 exact region(s)
func.func @axa_rev_i(%A: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[3 - %i, %k] : memref<4x4xf32>
        %b = affine.load %A[%k, %j] : memref<4x4xf32>
        %c = affine.load %C[%i, %j] : memref<4x4xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<4x4xf32>
      }
    }
  }
  return
}

// -----
// A[i][k] * A[3 - k][j]: K reversed alone does not do it (A[p][3 - q] against
// A[q][c]); I reversed does: both factors are A with its rows reversed, C is
// written rows 3..0.
// CHECK-LABEL: func.func @axa_one_rev
// CHECK:       %[[V:.*]] = memref.subview %arg0[3, 0] [4, 4] [-1, 1]
// CHECK:       %[[T:.*]] = bufferization.to_tensor %[[V]]
// CHECK:       memref.subview %arg1[3, 0] [4, 4] [-1, 1]
// CHECK:       systolic.matmul_tile %[[T]], %[[T]], %{{.*}} m = 4 n = 4 k = 4
// REMARK:      affine GEMM 4x4x4 lowered as 1 exact region(s)
func.func @axa_one_rev(%A: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<4x4xf32>
        %b = affine.load %A[3 - %k, %j] : memref<4x4xf32>
        %c = affine.load %C[%i, %j] : memref<4x4xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<4x4xf32>
      }
    }
  }
  return
}

// -----
// A skew read by one factor only: A[I][I + K] against A[K][J] in no
// orientation one tensor.
// CHECK-LABEL: func.func @axa_diff_skew
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      affine GEMM not lowered to systolic.matmul_tile: A and B are different views of the same memref [VALID_BUT_LOWERING_UNSUPPORTED]
func.func @axa_diff_skew(%A: memref<4x7xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %i + %k] : memref<4x7xf32>
        %b = affine.load %A[%k, %j] : memref<4x7xf32>
        %c = affine.load %C[%i, %j] : memref<4x4xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<4x4xf32>
      }
    }
  }
  return
}

// -----
// A . A^T: A[i][k] against A[j][k] (B' = A^T): a transpose is no orientation.
// CHECK-LABEL: func.func @aat
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      affine GEMM not lowered to systolic.matmul_tile: A and B are different views of the same memref [VALID_BUT_LOWERING_UNSUPPORTED]
func.func @aat(%A: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<4x4xf32>
        %b = affine.load %A[%j, %k] : memref<4x4xf32>
        %c = affine.load %C[%i, %j] : memref<4x4xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<4x4xf32>
      }
    }
  }
  return
}

// -----
// A[i][2k] * A[2k][j]: A' = A[0:4, 0:8:2] (strides (8, 2)) and B' =
// A[0:8:2, 0:4] (strides (16, 1)) are two views.
// CHECK-LABEL: func.func @axa_k2
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      affine GEMM not lowered to systolic.matmul_tile: A and B are different views of the same memref [VALID_BUT_LOWERING_UNSUPPORTED]
func.func @axa_k2(%A: memref<8x8xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, 2 * %k] : memref<8x8xf32>
        %b = affine.load %A[2 * %k, %j] : memref<8x8xf32>
        %c = affine.load %C[%i, %j] : memref<4x4xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<4x4xf32>
      }
    }
  }
  return
}
