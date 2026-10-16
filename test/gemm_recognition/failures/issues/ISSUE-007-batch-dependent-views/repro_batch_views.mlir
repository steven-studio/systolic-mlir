// ISSUE-007: the showcase's batch-dependent views at the MLIR level --
// ext_015 (the Phase 1 target, A's row moving with j), ext_017 (A[i + j][k]
// with B[k][2j]) and ext_018 (each operand at its own batch index) lowered
// with the batch loop around the tiles; case_127's shape (C's row mixing the
// batch coordinate) stays refused, without a remark (not recognized).
//
// RUN: systolic-opt "--affine-matmul-to-systolic=report=true allow-fp-reassociation=true classify=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s
//
// CHECK:     remark: affine GEMM 4x1x4 lowered as 1 exact region(s), batched: 4 batch iteration(s) of 1 batch loop(s)
// CHECK:     remark: affine GEMM 8x1x8 lowered as 1 exact region(s), batched: 8 batch iteration(s) of 1 batch loop(s)
// CHECK:     remark: affine GEMM 4x4x4 lowered as 1 exact region(s), batched: 2 batch iteration(s) of 1 batch loop(s)
// CHECK-NOT: remark:

// ext_015: C[8i][j] += A[4i - j][5k] * B[5k][j], i in [1, 5)
func.func @a_row_moves_with_j(%A: memref<17x16xf32> {llvm.noalias}, %B: memref<16x4xf32> {llvm.noalias}, %C: memref<33x4xf32> {llvm.noalias}) {
  affine.for %i = 1 to 5 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i * 4 - %j, %k * 5] : memref<17x16xf32>
        %b = affine.load %B[%k * 5, %j] : memref<16x4xf32>
        %c = affine.load %C[%i * 8, %j] : memref<33x4xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i * 8, %j] : memref<33x4xf32>
      }
    }
  }
  return
}

// -----
// ext_017: C[i][j] += A[i + j][k] * B[k][2j]
func.func @a_rows_skewed_by_column(%A: memref<15x8xf32> {llvm.noalias}, %B: memref<8x16xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i + %j, %k] : memref<15x8xf32>
        %b = affine.load %B[%k, %j * 2] : memref<8x16xf32>
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
// ext_018: A[b + 1] with B[b] and C[b]
func.func @batch_index_per_operand(%A: memref<3x4x4xf32> {llvm.noalias}, %B: memref<2x4x4xf32> {llvm.noalias}, %C: memref<2x4x4xf32> {llvm.noalias}) {
  affine.for %b = 0 to 2 {
    affine.for %i = 0 to 4 {
      affine.for %j = 0 to 4 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%b + 1, %i, %k] : memref<3x4x4xf32>
          %bb = affine.load %B[%b, %k, %j] : memref<2x4x4xf32>
          %c = affine.load %C[%b, %i, %j] : memref<2x4x4xf32>
          %p = arith.mulf %a, %bb : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%b, %i, %j] : memref<2x4x4xf32>
        }
      }
    }
  }
  return
}

// -----
// case_127's shape: C[i + k][j] -- C's row mixes the batch loop k. Refused.
func.func @c_skewed(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<8x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<4x4xf32>
        %bb = affine.load %B[%k, %j] : memref<4x4xf32>
        %c = affine.load %C[%i + %k, %j] : memref<8x4xf32>
        %p = arith.mulf %a, %bb : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i + %k, %j] : memref<8x4xf32>
      }
    }
  }
  return
}
