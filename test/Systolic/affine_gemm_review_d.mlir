// Review fixes of Phase D (docs/AFFINE_GEMM_MAPPING.md section 19.11,
// 2026-10-18): loops that run once are iteration dimensions whether or not an
// access names their counter (Lemma 0; an all-constant nest of such loops is
// the 1 x 1 x 1 GEMM, SQ-4; the hoist pass reads them so too), and the
// general view's bounds test is checked arithmetic throughout (a wrapped sum
// once let an out-of-bounds nest pass and emitted a negative offset).
//
// RUN: systolic-opt "--affine-matmul-to-systolic=report=true allow-fp-reassociation=true classify=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=REMARK
// RUN: systolic-opt "--affine-matmul-to-systolic=allow-fp-reassociation=true" \
// RUN:   -split-input-file %s | FileCheck %s

// Two loops of one iteration, constant subscripts: the 1 x 1 x 1 GEMM
// (UNKNOWN before: "loop 0 of the nest is used by no access").
// CHECK-LABEL: func.func @all_once_constant
// CHECK:       memref.subview %arg0[1, 0] [1, 1] [1, 1]
// CHECK:       memref.subview %arg1[0, 0] [1, 1] [1, 1]
// CHECK:       memref.subview %arg2[2, 0] [1, 1] [1, 1]
// CHECK:       systolic.matmul_tile {{.*}} m = 1 n = 1 k = 1
// REMARK:      affine GEMM 1x1x1 lowered as 1 exact region(s)
func.func @all_once_constant(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %k = 1 to 2 {
    affine.for %i = 0 to 1 {
      %a = affine.load %A[1, 0] : memref<4x4xf32>
      %b = affine.load %B[0, 0] : memref<4x4xf32>
      %c = affine.load %C[2, 0] : memref<4x4xf32>
      %p = arith.mulf %a, %b : f32
      %s = arith.addf %c, %p : f32
      affine.store %s, %C[2, 0] : memref<4x4xf32>
    }
  }
  return
}

// -----
// A loop of one iteration outside a repetition loop of four: the hoist moves
// the repetition loop outward (the once-loop is a dimension), the 1 x 1 x 1
// GEMM runs four times.
// CHECK-LABEL: func.func @once_outside_repetition
// CHECK:       affine.for %{{.*}} = 0 to 4 {
// CHECK:         systolic.matmul_tile {{.*}} m = 1 n = 1 k = 1
// REMARK:      affine GEMM 1x1x1 lowered as 1 exact region(s); the enclosing loops run it 4 time(s)
func.func @once_outside_repetition(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %k = 1 to 2 {
    affine.for %i = 0 to 4 {
      %a = affine.load %A[1, 0] : memref<4x4xf32>
      %b = affine.load %B[0, 0] : memref<4x4xf32>
      %c = affine.load %C[2, 0] : memref<4x4xf32>
      %p = arith.mulf %a, %b : f32
      %s = arith.addf %c, %p : f32
      affine.store %s, %C[2, 0] : memref<4x4xf32>
    }
  }
  return
}

// -----
// A repetition loop of four iterations alone: nothing to fold, the mapping is
// not decided (as before).
// CHECK-LABEL: func.func @repetition_only
// CHECK-NOT:   systolic.matmul_tile
// REMARK-NOT:  lowered as
func.func @repetition_only(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    %a = affine.load %A[1, 0] : memref<4x4xf32>
    %b = affine.load %B[0, 0] : memref<4x4xf32>
    %c = affine.load %C[2, 0] : memref<4x4xf32>
    %p = arith.mulf %a, %b : f32
    %s = arith.addf %c, %p : f32
    affine.store %s, %C[2, 0] : memref<4x4xf32>
  }
  return
}

// -----
// An unused once-loop between the K and J loops: a dimension of one value,
// the 4 x 4 x 4 GEMM is lowered (hoisted outward by the lowering pass).
// CHECK-LABEL: func.func @once_between
// CHECK:       systolic.matmul_tile {{.*}} m = 4 n = 4 k = 4
// REMARK:      affine GEMM 4x4x4 lowered as 1 exact region(s)
func.func @once_between(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %k = 0 to 4 {
      affine.for %t = 5 to 6 {
        affine.for %j = 0 to 4 {
          %a = affine.load %A[%i, %k] : memref<4x4xf32>
          %b = affine.load %B[%k, %j] : memref<4x4xf32>
          %c = affine.load %C[%i, %j] : memref<4x4xf32>
          %p = arith.mulf %a, %b : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%i, %j] : memref<4x4xf32>
        }
      }
    }
  }
  return
}

// -----
// G3 in checked arithmetic: A[i][i + 2^61 k] with k in [3, 5) -- the
// dimension's largest index 3 * 2^61 + 2^61 = 2^63 does not fit; the sum
// used to wrap to a negative value that passed the bounds test (and the nest
// was lowered with an offset of 2^62).
// CHECK-LABEL: func.func @skew_overflow_high
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      affine GEMM not lowered to systolic.matmul_tile: A's subscripts along dimension 1 overflow 64-bit arithmetic [UNKNOWN]
func.func @skew_overflow_high(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 3 to 5 {
        %a = affine.load %A[%i, %i + %k * 2305843009213693952] : memref<4x4xf32>
        %b = affine.load %B[%k, %j] : memref<4x4xf32>
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
// The mirror image: A[i][i - 2^62 k], k in [1, 3): the smallest index
// -2^62 - 2^62 = -2^63 is INT64_MIN (the dynamic marker); the terms fit, the
// index is negative, and the nest is refused for what it is -- an
// out-of-bounds access (before the fix a wrapped partial sum let it through to
// an invalid reinterpret_cast offset and the pass failed).
// CHECK-LABEL: func.func @skew_overflow_low
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      affine GEMM not lowered to systolic.matmul_tile: A is accessed out of bounds along dimension 1 [PROVEN_INVALID]
func.func @skew_overflow_low(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 1 to 3 {
        %a = affine.load %A[%i, %i - %k * 4611686018427387904] : memref<4x4xf32>
        %b = affine.load %B[%k, %j] : memref<4x4xf32>
        %c = affine.load %C[%i, %j] : memref<4x4xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<4x4xf32>
      }
    }
  }
  return
}
