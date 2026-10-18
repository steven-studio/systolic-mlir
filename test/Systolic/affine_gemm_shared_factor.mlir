// One array as both factors (A x A), docs/AFFINE_GEMM_MAPPING.md sections 7
// and 19.12. When the first factor's (I, K) view and the second's (K, J) view
// are the same element map, ONE read-only tensor serves both operands. When
// they are two different views of the memref (A . A^T, a skew read by one
// factor only, a stride-2 view against its transpose, a factor read backwards)
// each operand gets its own view and its own read-only tensor (since
// 2026-10-18, section 19.12; refused as VALID_BUT_LOWERING_UNSUPPORTED between
// 2026-10-16 and 2026-10-18). Until the same day the lowering also searched
// the eight orientations of the coordinates for one in which the two views
// coincide and wrote C through a reversed view when it found one; the two
// views supersede that: the tile lowering stages every A and B tile from its
// own view's buffer, C is written at its own view, and C is provably disjoint
// from the shared base or the nest is refused. Mirrored by casespec.py (no
// refusal; the spec accepts).
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
// Both factors read K downward through the same skew: A'(I, K) = A[I][I + 3 - K]
// is offset 3, strides (8, -1), B'(K, J) = A[3 - K][3 - K + J] is offset 24,
// strides (-8, 1): two views of A (they would coincide with K enumerated
// downward, which the lowering no longer tries), two tensors, C written directly.
// CHECK-LABEL: func.func @axa_both_rev_skew
// CHECK:       %[[VA:.*]] = memref.reinterpret_cast %arg0 to offset: [3], sizes: [4, 4], strides: [8, -1]
// CHECK:       %[[TA:.*]] = bufferization.to_tensor %[[VA]] restrict
// CHECK:       %[[VB:.*]] = memref.reinterpret_cast %arg0 to offset: [24], sizes: [4, 4], strides: [-8, 1]
// CHECK:       %[[TB:.*]] = bufferization.to_tensor %[[VB]] restrict
// CHECK-NOT:   memref.subview
// CHECK:       systolic.matmul_tile %[[TA]], %[[TB]], %{{.*}} m = 4 n = 4 k = 4
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
// A[3 - i][k] * A[k][j]: the first factor is A with its rows reversed (a
// subview of stride -1), the second the whole memref: two tensors, and C is
// written directly (until 2026-10-18 I was enumerated downward so that both
// factors were the whole memref, and C was written through a reversed view).
// CHECK-LABEL: func.func @axa_rev_i
// CHECK:       %[[VA:.*]] = memref.subview %arg0[3, 0] [4, 4] [-1, 1]
// CHECK:       %[[TA:.*]] = bufferization.to_tensor %[[VA]] restrict
// CHECK:       %[[TB:.*]] = bufferization.to_tensor %arg0 restrict
// CHECK-NOT:   memref.subview %arg1
// CHECK:       systolic.matmul_tile %[[TA]], %[[TB]], %{{.*}} m = 4 n = 4 k = 4
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
// A[i][k] * A[3 - k][j]: the whole memref and A with its rows reversed: two
// tensors, C written directly.
// CHECK-LABEL: func.func @axa_one_rev
// CHECK:       %[[TA:.*]] = bufferization.to_tensor %arg0 restrict
// CHECK:       %[[VB:.*]] = memref.subview %arg0[3, 0] [4, 4] [-1, 1]
// CHECK:       %[[TB:.*]] = bufferization.to_tensor %[[VB]] restrict
// CHECK-NOT:   memref.subview %arg1
// CHECK:       systolic.matmul_tile %[[TA]], %[[TB]], %{{.*}} m = 4 n = 4 k = 4
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
// A skew read by one factor only: A[I][I + K] against A[K][J] -- in no
// orientation one tensor, so two views: the skewed reinterpret_cast for the
// first factor, the plain 4 x 4 subview for the second, two tensors.
// CHECK-LABEL: func.func @axa_diff_skew
// CHECK:       %[[V0:.*]] = memref.reinterpret_cast %arg0 to offset: [0], sizes: [4, 4], strides: [8, 1]
// CHECK:       %[[T0:.*]] = bufferization.to_tensor %[[V0]]
// CHECK:       %[[V1:.*]] = memref.subview %arg0[0, 0] [4, 4] [1, 1] : memref<4x7xf32> to memref<4x4xf32, strided<[7, 1]>>
// CHECK:       %[[T1:.*]] = bufferization.to_tensor %[[V1]]
// CHECK:       systolic.matmul_tile %[[T0]], %[[T1]], %{{.*}} m = 4 n = 4 k = 4
// REMARK:      affine GEMM 4x4x4 lowered as 1 exact region(s)
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
// A . A^T: A[i][k] against A[j][k] (B' = A^T): a transpose is no orientation,
// so two views -- the whole memref and its transpose -- and two tensors.
// CHECK-LABEL: func.func @aat
// CHECK:       %[[T0:.*]] = bufferization.to_tensor %arg0 restrict
// CHECK:       %[[TR:.*]] = memref.transpose %{{.*}} (d0, d1) -> (d1, d0)
// CHECK:       %[[T1:.*]] = bufferization.to_tensor %[[TR]]
// CHECK:       systolic.matmul_tile %[[T0]], %[[T1]], %{{.*}} m = 4 n = 4 k = 4
// REMARK:      affine GEMM 4x4x4 lowered as 1 exact region(s)
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
// A[0:8:2, 0:4] (strides (16, 1)) are two views, two tensors.
// CHECK-LABEL: func.func @axa_k2
// CHECK:       %[[V0:.*]] = memref.subview %arg0[0, 0] [4, 4] [1, 2] : memref<8x8xf32> to memref<4x4xf32, strided<[8, 2]>>
// CHECK:       %[[T0:.*]] = bufferization.to_tensor %[[V0]]
// CHECK:       %[[V1:.*]] = memref.subview %arg0[0, 0] [4, 4] [2, 1] : memref<8x8xf32> to memref<4x4xf32, strided<[16, 1]>>
// CHECK:       %[[T1:.*]] = bufferization.to_tensor %[[V1]]
// CHECK:       systolic.matmul_tile %[[T0]], %[[T1]], %{{.*}} m = 4 n = 4 k = 4
// REMARK:      affine GEMM 4x4x4 lowered as 1 exact region(s)
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

// -----
// C aliasing the shared factor: C += A . A with C = A itself (one memref for
// all three operands): the output overlaps the inputs, refused unless proven
// disjoint (docs section 7) -- the A x A views do not change that.
// CHECK-LABEL: func.func @axa_c_is_a
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      affine GEMM not lowered to systolic.matmul_tile: A may overlap C; declare the arguments llvm.noalias (C restrict) or pass assume-no-alias [UNKNOWN]
func.func @axa_c_is_a(%A: memref<4x4xf32>) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<4x4xf32>
        %b = affine.load %A[%k, %j] : memref<4x4xf32>
        %c = affine.load %A[%i, %j] : memref<4x4xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %A[%i, %j] : memref<4x4xf32>
      }
    }
  }
  return
}
