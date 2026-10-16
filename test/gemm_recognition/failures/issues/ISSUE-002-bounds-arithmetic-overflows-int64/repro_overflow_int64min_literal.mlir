// ISSUE-002, cases that need the INT64_MIN literal -9223372036854775808.
//
// MINIMUM MLIR: llvm-project e803069070cf (2026-08-14, "[mlir][AffineExpr]
// Fix UBSan signed-integer overflow in subtraction operators"). Before it,
// AffineParser::parseIntegerExpr rejects 9223372036854775808 with "constant
// too large for index", systolic-opt exits non-zero and both RUN lines fail
// (read in that commit's parent; NOT run here -- no older MLIR was built).
// Measured only with llvm-project 7615da10b75c. Kept apart so that the other
// ISSUE-002 tests do not depend on this parser behaviour; the INT64_MIN guards
// themselves are exercised literal-free in repro_overflow_guards.mlir.
//
// The checks are what holds whatever the parser and folder make of the
// literal: never lowered, never reported as an overflow of the normalization.
//
//   @int64_min            i * -9223372036854775808 on A and C. With 7615da10
//                         MLIR represents it as d0 * (-(-9223372036854775808)),
//                         a non-constant right-hand side, so extractLinearForm
//                         refuses it structurally (recognition failure) -- it
//                         never reaches the INT64_MIN guards.
//   @empty_int64min_ub    an empty loop 0 to INT64_MIN (SQ-2): not an
//                         overflow -- reported as an empty GEMM (the
//                         integration's first heuristic called it an
//                         overflow).
//
// RUN: systolic-opt --systolic-declare-fpga-fleet --test-affine-matmul-discovery \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=DISC
// RUN: systolic-opt --systolic-declare-fpga-fleet "--affine-matmul-to-systolic=report=true allow-fp-reassociation=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=SYS
//
// DISC-NOT:  dense logical GEMM: yes
// DISC-NOT:  normalized form overflows
//
// SYS-NOT:   lowered as
// SYS-NOT:   normalized form overflows

// a coefficient of INT64_MIN, written as the literal
func.func @int64_min(%A: memref<?x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<?x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 1 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i * -9223372036854775808, %k] : memref<?x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x8xf32>
        %c = affine.load %C[%i * -9223372036854775808, %j] : memref<?x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i * -9223372036854775808, %j] : memref<?x8xf32>
      }
    }
  }
  return
}

// -----
// an empty loop whose upper bound is INT64_MIN
func.func @empty_int64min_ub(%A: memref<?x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<?x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to -9223372036854775808 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i, %k] : memref<?x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x8xf32>
        %c = affine.load %C[%i, %j] : memref<?x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<?x8xf32>
      }
    }
  }
  return
}
