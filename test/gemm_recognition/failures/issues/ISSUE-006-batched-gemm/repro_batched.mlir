// ISSUE-006: the showcase's batched cases at the MLIR level -- case_085 (one
// batch coordinate) and ext_011 (two) -- lowered with the batch loops around
// the tiles; case_086's shape (B shared by every batch) stayed refused until
// 2026-10-19: the dimension-intersection partitioning (docs section 21) lowers
// it as a partition on b with B shared.
//
// RUN: systolic-opt "--affine-matmul-to-systolic=report=true allow-fp-reassociation=true classify=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s
//
// CHECK:     remark: affine GEMM 4x4x4 lowered as 1 exact region(s), batched: 2 batch iteration(s) of 1 batch loop(s); the enclosing loops run it 2 time(s)
// CHECK:     remark: affine GEMM 4x4x4 lowered as 1 exact region(s), batched: 4 batch iteration(s) of 2 batch loop(s); the enclosing loops run it 4 time(s)
// CHECK:     remark: affine GEMM 4x4x4 lowered as 1 exact region(s), partitioned on 1 demoted loop(s), batched: 2 batch iteration(s) of 1 batch loop(s); the enclosing loops run it 2 time(s)
// CHECK-NOT: remark:

// case_085: C[b][i][j] += A[b][i][k] * B[b][k][j]
func.func @batched_gemm(%A: memref<2x4x4xf32> {llvm.noalias}, %B: memref<2x4x4xf32> {llvm.noalias}, %C: memref<2x4x4xf32> {llvm.noalias}) {
  affine.for %b = 0 to 2 {
    affine.for %i = 0 to 4 {
      affine.for %j = 0 to 4 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%b, %i, %k] : memref<2x4x4xf32>
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
// ext_011: two batch coordinates
func.func @batched_two(%A: memref<2x2x4x4xf32> {llvm.noalias}, %B: memref<2x2x4x4xf32> {llvm.noalias}, %C: memref<2x2x4x4xf32> {llvm.noalias}) {
  affine.for %b0 = 0 to 2 {
    affine.for %b1 = 0 to 2 {
      affine.for %i = 0 to 4 {
        affine.for %j = 0 to 4 {
          affine.for %k = 0 to 4 {
            %a = affine.load %A[%b0, %b1, %i, %k] : memref<2x2x4x4xf32>
            %bb = affine.load %B[%b0, %b1, %k, %j] : memref<2x2x4x4xf32>
            %c = affine.load %C[%b0, %b1, %i, %j] : memref<2x2x4x4xf32>
            %p = arith.mulf %a, %bb : f32
            %s = arith.addf %c, %p : f32
            affine.store %s, %C[%b0, %b1, %i, %j] : memref<2x2x4x4xf32>
          }
        }
      }
    }
  }
  return
}

// -----
// case_086: B shared by every batch (no batch subscript): b is read by A and
// C only -- a second I coordinate; since 2026-10-19 a partition on b (section 21).
func.func @batched_shared_b(%A: memref<2x4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<2x4x4xf32> {llvm.noalias}) {
  affine.for %b = 0 to 2 {
    affine.for %i = 0 to 4 {
      affine.for %j = 0 to 4 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%b, %i, %k] : memref<2x4x4xf32>
          %bb = affine.load %B[%k, %j] : memref<4x4xf32>
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
