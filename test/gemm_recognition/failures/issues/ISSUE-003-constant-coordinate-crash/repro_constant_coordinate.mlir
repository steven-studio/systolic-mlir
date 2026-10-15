// ISSUE-003: a logical coordinate that no loop drives (a constant subscript
// shared by the two operands that index it: C[0][j] with A[0][k]) crashed
// getDenseLogicalGemm. The recognizer accepts the nest (identical
// subscripts), every loop drives J or K, the list of I's loops is empty and
// `T[d][loops.front()]` read past it: SIGSEGV in release builds, the
// assertion `!empty()` of SmallVector::front in builds with assertions, in
// both --test-affine-matmul-discovery and --affine-matmul-to-systolic.
// Reachable from C (systolic-cfront translates
// `C[0][j] += A[0][kk*8+k] * B[kk*8+k][j]` to exactly @const_row_tiled_k).
//
// Now refused by the dense partition with its own reason. Whether a constant
// coordinate (a GEMM with M, N or K = 1 spelled with a constant subscript)
// should instead be offloaded is an open specification question (SQ-4 in
// test/gemm_recognition/SPEC_QUESTIONS.md); this test fixes only "refused
// with a reason, never a crash".
//
// RUN: systolic-opt --systolic-declare-fpga-fleet --test-affine-matmul-discovery \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=DISC
// RUN: systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic=report=true \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=SYS
//
// DISC:      recognition result: success
// DISC:      dense logical GEMM: no (logical coordinate I is driven by no loop (a constant subscript))
// DISC:      recognition result: success
// DISC:      dense logical GEMM: no (logical coordinate J is driven by no loop (a constant subscript))
// DISC:      recognition result: success
// DISC:      dense logical GEMM: no (logical coordinate K is driven by no loop (a constant subscript))
//
// SYS:       remark: affine GEMM not lowered to systolic.matmul_tile: logical coordinate I is driven by no loop (a constant subscript)
// SYS:       remark: affine GEMM not lowered to systolic.matmul_tile: logical coordinate J is driven by no loop (a constant subscript)
// SYS:       remark: affine GEMM not lowered to systolic.matmul_tile: logical coordinate K is driven by no loop (a constant subscript)
// SYS-NOT:   lowered as

// I is the constant 0 (A[0][..], C[0][..]); j drives J, kk and k drive K
func.func @const_row_tiled_k(%A: memref<1x16xf32> {llvm.noalias}, %B: memref<16x8xf32> {llvm.noalias}, %C: memref<1x8xf32> {llvm.noalias}) {
  affine.for %j = 0 to 8 {
    affine.for %kk = 0 to 2 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[0, %kk * 8 + %k] : memref<1x16xf32>
        %b = affine.load %B[%kk * 8 + %k, %j] : memref<16x8xf32>
        %c = affine.load %C[0, %j] : memref<1x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[0, %j] : memref<1x8xf32>
      }
    }
  }
  return
}

// -----
// J is the constant 0 (B[..][0], C[..][0]); i drives I, kk and k drive K
func.func @const_col_tiled_k(%A: memref<8x16xf32> {llvm.noalias}, %B: memref<16x1xf32> {llvm.noalias}, %C: memref<8x1xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %kk = 0 to 2 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i, %kk * 8 + %k] : memref<8x16xf32>
        %b = affine.load %B[%kk * 8 + %k, 0] : memref<16x1xf32>
        %c = affine.load %C[%i, 0] : memref<8x1xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, 0] : memref<8x1xf32>
      }
    }
  }
  return
}

// -----
// K is the constant 0 (A[..][0], B[0][..]); ii and i drive I, j drives J
func.func @const_red_tiled_i(%A: memref<16x1xf32> {llvm.noalias}, %B: memref<1x8xf32> {llvm.noalias}, %C: memref<16x8xf32> {llvm.noalias}) {
  affine.for %ii = 0 to 2 {
    affine.for %i = 0 to 8 {
      affine.for %j = 0 to 8 {
        %a = affine.load %A[%ii * 8 + %i, 0] : memref<16x1xf32>
        %b = affine.load %B[0, %j] : memref<1x8xf32>
        %c = affine.load %C[%ii * 8 + %i, %j] : memref<16x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%ii * 8 + %i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
