// ISSUE-002: the bounds arithmetic of the affine GEMM path (the coordinate's
// values in getDenseLogicalGemm / restrictToBox, each operand's last index in
// getLogicalOperand) was plain int64 arithmetic.  With a coefficient near
// 2^63 the products wrap around, and a wrapped-around "last index" can pass
// the out-of-bounds check: at 6e25528, @wraps_to_zero (rows i * 2^62 for
// i < 5: the last row 2^64 wraps to 0) was refused only because the Presburger
// exact-cover check happened to fail, with the wrong reason, and @view_overflow
// (reachable since the per-operand views of ISSUE-001) was not refused by the
// bounds check at all.  Fixed with llvm::checkedMul / checkedAdd: an overflow
// is a refusal with its own reason, never a wrapped value.  Permanent
// regression test (no expected-failure marker: fixed with the issue).
//
// RUN: systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic=report=true \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s
//
// CHECK: remark: affine GEMM not lowered to systolic.matmul_tile: logical coordinate I's scale or extent overflows 64-bit arithmetic
// CHECK: remark: affine GEMM not lowered to systolic.matmul_tile: logical coordinate I's scale or extent overflows 64-bit arithmetic
// CHECK: remark: affine GEMM not lowered to systolic.matmul_tile: A's subscripts along dimension 0 overflow 64-bit arithmetic
// CHECK: remark: affine GEMM 1x8x8 lowered as
// CHECK-NOT: wrong

// the last row 2^62 * 4 = 2^64 wraps to 0 in int64: must be refused for the overflow
func.func @wraps_to_zero(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 5 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i * 4611686018427387904, %k] : memref<8x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x8xf32>
        %c = affine.load %C[%i * 4611686018427387904, %j] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i * 4611686018427387904, %j] : memref<8x8xf32>
      }
    }
  }
  return
}

// -----
// the coordinate's exclusive end 2^61 * 4 = 2^63 does not fit (its last value does)
func.func @end_overflows(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i * 2305843009213693952, %k] : memref<8x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x8xf32>
        %c = affine.load %C[%i * 2305843009213693952, %j] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i * 2305843009213693952, %j] : memref<8x8xf32>
      }
    }
  }
  return
}

// -----
// per-operand views (ISSUE-001): the coordinate i is small, A's view of it has
// offset 2^62 and stride 2^62, whose last index 2^62 + 3 * 2^62 = 2^64 overflows
func.func @view_overflow(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i * 4611686018427387904 + 4611686018427387904, %k] : memref<8x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x8xf32>
        %c = affine.load %C[%i * 4611686018427387904, %j] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i * 4611686018427387904, %j] : memref<8x8xf32>
      }
    }
  }
  return
}

// -----
// the control: stride 2^62 on a coordinate of extent 1 is in bounds (row 0 only)
// and every product fits; it is lowered
func.func @single_row_huge_stride(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 1 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i * 4611686018427387904, %k] : memref<8x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x8xf32>
        %c = affine.load %C[%i * 4611686018427387904, %j] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i * 4611686018427387904, %j] : memref<8x8xf32>
      }
    }
  }
  return
}
