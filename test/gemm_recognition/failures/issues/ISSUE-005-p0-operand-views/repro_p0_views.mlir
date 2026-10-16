// ISSUE-005: the showcase cases whose expectation the P0 operand views changed
// (OUT_OF_SCOPE -> ACCEPT), at the MLIR level: a transposed A (case_091), B
// (case_092) and C (case_093), a matrix-vector product (case_083) and one
// operand reading a shared coordinate backwards (mlir_010). Lowered with the
// reassociation permission; refused, with the reason, without it.
//
// RUN: systolic-opt "--affine-matmul-to-systolic=report=true allow-fp-reassociation=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=SYS
// RUN: systolic-opt --affine-matmul-to-systolic=report=true -split-input-file %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s --check-prefix=STRICT
//
// SYS:      remark: affine GEMM 8x8x8 lowered as
// SYS:      remark: affine GEMM 8x8x8 lowered as
// SYS:      remark: affine GEMM 8x8x8 lowered as
// SYS:      remark: affine GEMM 8x1x8 lowered as
// SYS:      remark: affine GEMM 8x8x8 lowered as
// SYS-NOT:  not lowered
//
// STRICT-COUNT-5: remark: affine GEMM not lowered to systolic.matmul_tile: floating-point reassociation is not permitted
// STRICT-NOT:     lowered as

// case_091: C[i][j] += A[k][i] * B[k][j]
func.func @transposed_a(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%k, %i] : memref<8x8xf32>
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
// case_092: C[i][j] += A[i][k] * B[j][k]
func.func @transposed_b(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i, %k] : memref<8x8xf32>
        %b = affine.load %B[%j, %k] : memref<8x8xf32>
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
// case_093: C[j][i] += A[i][k] * B[k][j]
func.func @transposed_c(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i, %k] : memref<8x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x8xf32>
        %c = affine.load %C[%j, %i] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%j, %i] : memref<8x8xf32>
      }
    }
  }
  return
}

// -----
// case_083: y[i] += M[i][j] * x[j]
func.func @matrix_vector(%M: memref<8x8xf32> {llvm.noalias}, %x: memref<8xf32> {llvm.noalias}, %y: memref<8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      %a = affine.load %M[%i, %j] : memref<8x8xf32>
      %b = affine.load %x[%j] : memref<8xf32>
      %c = affine.load %y[%i] : memref<8xf32>
      %p = arith.mulf %a, %b : f32
      %s = arith.addf %c, %p : f32
      affine.store %s, %y[%i] : memref<8xf32>
    }
  }
  return
}

// -----
// mlir_010: C[i][j] += A[7 - i][k] * B[k][j]
func.func @reversed_coordinate_on_a_only(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[7 - %i, %k] : memref<8x8xf32>
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
