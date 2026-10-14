// ISSUE-001 reproducers: the recognizer rejects a GEMM whenever the two operands
// that share a logical coordinate spell it with different affine expressions,
// however trivially (a constant offset or a stride on one side only).
//
// Each function is one variant; the comment above it says what the recognizer
// is expected to do by the specification, and what commit 6e25528 does.  By
// the specification all seven variants are dense GEMMs, so the RUN line
// expects seven successes; at 6e25528 only v2 succeeds, hence the
// expected-failure marker (XPASS = fixed; then drop the marker line).  Or run
// scripts/run_issue.sh ISSUE-001, which records the actual output in this
// directory.
//
// XFAIL: *
// RUN: systolic-opt --systolic-declare-fpga-fleet --test-affine-matmul-discovery \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s
// CHECK-COUNT-7: recognition result: success
//
// Every variant computes a valid GEMM: C'(I,J) += A'(I,K) B'(K,J) over the
// 8x8x8 loop domain, with each operand's own view (row/column offsets or
// strides).  Arrays are big enough for every subscript (A, B: 8 rows of 32
// columns as declared in C; here memref<?x32xf32>).

// -----
// v1_case_065_offsets_on_C_only
// specification: ACCEPT (a dense GEMM).  Commit 6e25528: recognition result: FAILURE (ISSUE-001: C's expressions differ from A's and B's)
func.func @v1_case_065_offsets_on_C_only(%A: memref<?x32xf32> {llvm.noalias}, %B: memref<?x32xf32> {llvm.noalias}, %C: memref<?x32xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %c = affine.load %C[(%i + 8), (%j + 16)] : memref<?x32xf32>
        %a = affine.load %A[%i, %k] : memref<?x32xf32>
        %b = affine.load %B[%k, %j] : memref<?x32xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[(%i + 8), (%j + 16)] : memref<?x32xf32>
      }
    }
  }
  return
}

// -----
// v2_control_same_offsets_everywhere
// specification: ACCEPT (a dense GEMM).  Commit 6e25528: recognition result: success (the offsets are no problem when every operand spells the coordinate the same way)
func.func @v2_control_same_offsets_everywhere(%A: memref<?x32xf32> {llvm.noalias}, %B: memref<?x32xf32> {llvm.noalias}, %C: memref<?x32xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %c = affine.load %C[(%i + 8), (%j + 16)] : memref<?x32xf32>
        %a = affine.load %A[(%i + 8), %k] : memref<?x32xf32>
        %b = affine.load %B[%k, (%j + 16)] : memref<?x32xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[(%i + 8), (%j + 16)] : memref<?x32xf32>
      }
    }
  }
  return
}

// -----
// v3_row_offset_on_C_only
// specification: ACCEPT (a dense GEMM).  Commit 6e25528: recognition result: FAILURE (only I differs)
func.func @v3_row_offset_on_C_only(%A: memref<?x32xf32> {llvm.noalias}, %B: memref<?x32xf32> {llvm.noalias}, %C: memref<?x32xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %c = affine.load %C[(%i + 8), %j] : memref<?x32xf32>
        %a = affine.load %A[%i, %k] : memref<?x32xf32>
        %b = affine.load %B[%k, %j] : memref<?x32xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[(%i + 8), %j] : memref<?x32xf32>
      }
    }
  }
  return
}

// -----
// v4_column_offset_on_C_only
// specification: ACCEPT (a dense GEMM).  Commit 6e25528: recognition result: FAILURE (only J differs)
func.func @v4_column_offset_on_C_only(%A: memref<?x32xf32> {llvm.noalias}, %B: memref<?x32xf32> {llvm.noalias}, %C: memref<?x32xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %c = affine.load %C[%i, (%j + 16)] : memref<?x32xf32>
        %a = affine.load %A[%i, %k] : memref<?x32xf32>
        %b = affine.load %B[%k, %j] : memref<?x32xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, (%j + 16)] : memref<?x32xf32>
      }
    }
  }
  return
}

// -----
// v5_reduction_offset_differs_between_A_and_B
// specification: ACCEPT (a dense GEMM).  Commit 6e25528: recognition result: FAILURE (only K differs, between A and B)
func.func @v5_reduction_offset_differs_between_A_and_B(%A: memref<?x32xf32> {llvm.noalias}, %B: memref<?x32xf32> {llvm.noalias}, %C: memref<?x32xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %c = affine.load %C[%i, %j] : memref<?x32xf32>
        %a = affine.load %A[%i, (%k + 1)] : memref<?x32xf32>
        %b = affine.load %B[(%k + 2), %j] : memref<?x32xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<?x32xf32>
      }
    }
  }
  return
}

// -----
// v6_case_094_row_stride_on_C_only
// specification: ACCEPT (a dense GEMM).  Commit 6e25528: recognition result: FAILURE (a stride instead of an offset: the same rule)
func.func @v6_case_094_row_stride_on_C_only(%A: memref<?x32xf32> {llvm.noalias}, %B: memref<?x32xf32> {llvm.noalias}, %C: memref<?x32xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %c = affine.load %C[(%i * 2), %j] : memref<?x32xf32>
        %a = affine.load %A[%i, %k] : memref<?x32xf32>
        %b = affine.load %B[%k, %j] : memref<?x32xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[(%i * 2), %j] : memref<?x32xf32>
      }
    }
  }
  return
}

// -----
// v7_ext_008_row_offset_on_A_only
// specification: ACCEPT (a dense GEMM).  Commit 6e25528: recognition result: FAILURE (the offset on A instead of C: the same rule)
func.func @v7_ext_008_row_offset_on_A_only(%A: memref<?x32xf32> {llvm.noalias}, %B: memref<?x32xf32> {llvm.noalias}, %C: memref<?x32xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %c = affine.load %C[%i, %j] : memref<?x32xf32>
        %a = affine.load %A[(%i + 2), %k] : memref<?x32xf32>
        %b = affine.load %B[%k, %j] : memref<?x32xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<?x32xf32>
      }
    }
  }
  return
}
