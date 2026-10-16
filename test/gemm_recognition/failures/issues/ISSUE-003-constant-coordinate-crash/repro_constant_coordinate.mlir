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
// Fixed: the empty list is never indexed. Until 2026-10-15 the nest was then
// refused with its own reason, pending spec question SQ-4; SQ-4 is decided
// since (test/gemm_recognition/SPEC_QUESTIONS.md, docs/AFFINE_GEMM_MAPPING.md
// section 2, P0.5): a coordinate no loop drives is an extent-one coordinate,
// M, N or K = 1, and the nest is offloaded -- 1 x 8 x 16, 8 x 1 x 16 and
// 16 x 8 x 1 below, the extents the loops give the other two coordinates.
// The test still guards the crash (the dense partition handles the empty
// loop list).
//
// RUN: systolic-opt --systolic-declare-fpga-fleet --test-affine-matmul-discovery \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=DISC
// RUN: systolic-opt --systolic-declare-fpga-fleet "--affine-matmul-to-systolic=report=true allow-fp-reassociation=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=SYS
//
// DISC:      recognition result: success
// DISC:      dense logical GEMM: yes, I/J/K on loops /0/1+2
// DISC:      recognition result: success
// DISC:      dense logical GEMM: yes, I/J/K on loops 0//1+2
// DISC:      recognition result: success
// DISC:      dense logical GEMM: yes, I/J/K on loops 0+1/2/
//
// SYS:       remark: affine GEMM 1x8x16 lowered as
// SYS:       remark: affine GEMM 8x1x16 lowered as
// SYS:       remark: affine GEMM 16x8x1 lowered as
// SYS-NOT:   not lowered

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
