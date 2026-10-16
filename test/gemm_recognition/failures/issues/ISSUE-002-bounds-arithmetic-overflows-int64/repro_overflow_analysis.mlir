// ISSUE-002, continued: the arithmetic of the recognizer itself
// (AffineMatmulAnalysis.cpp), which repro_overflow.mlir does not reach.
//
// After the fix recorded in ISSUE-002.md, these were still plain int64:
// extractLinearForm (a sum or product of coefficients and constants),
// the trip count in normalizeLogicalCoordinates / recoverLogicalExtent
// (upper - lower + step - 1), the full-rank test of T (fraction-free
// elimination) and the Rational arithmetic of its inverse. Each could wrap
// around and let the wrapped value take part in recognition, partitioning or
// lowering:
//
//   @mul_overflow   (i + 2^62) * 2: the constant 2^63 wrapped to INT64_MIN
//                   and became A's view offset; the nest was refused only
//                   because that wrapped offset happened to be negative
//                   ("out of bounds") -- now refused at recognition;
//   @trip_overflow  i from -2^62 to 2^62 + 1: the trip count wrapped
//                   ("row extent: static -9223372036854775807") -- now unknown,
//                   and the dense partition names the overflow;
//   @exact_rank     coefficients 2^62 on I, J and K: the rank test's products
//                   overflowed and it answered "invertible: no" for a
//                   diagonal T -- now an exact (arbitrary-precision)
//                   determinant; the nest (extent 1, in bounds) is lowered;
//   @add_overflow:  a control, refused at recognition before and after. MLIR
//                   itself merges i * 2^62 + i * 2^62 into i * 2^63, which its
//                   folder wraps to INT64_MIN, so the pass sees a coefficient
//                   of INT64_MIN and matchSharedCoordinate's INT64_MIN guard
//                   refuses it; the checked addition of extractLinearForm is
//                   exercised by @add_overflow_unmerged in
//                   repro_overflow_guards.mlir instead.
//
// Every literal here is in (-2^63, 2^63): the file parses with any MLIR. The
// case that needs the INT64_MIN literal (parsed only by MLIR at or after
// llvm-project e803069070cf) is in repro_overflow_int64min_literal.mlir.
//
// Written with the fix; no expected-failure marker.
//
// RUN: systolic-opt --systolic-declare-fpga-fleet --test-affine-matmul-discovery \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=DISC
// RUN: systolic-opt --systolic-declare-fpga-fleet "--affine-matmul-to-systolic=report=true allow-fp-reassociation=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=SYS
//
// DISC-NOT:  row extent: static -
// DISC:      recognition result: failure
// DISC:      recognition result: failure
// DISC:      recognition result: success
// DISC:      row extent: unknown
// DISC:      dense logical GEMM: no (the iteration domain's normalized form overflows 64-bit arithmetic)
// DISC:      recognition result: success
// DISC:      invertible: yes
// DISC:      dense logical GEMM: yes, I/J/K on loops 0/1/2
// DISC-NOT:  row extent: static -
//
// SYS-NOT:   out of bounds
// SYS:       remark: affine GEMM not lowered to systolic.matmul_tile: the iteration domain's normalized form overflows 64-bit arithmetic
// SYS-NOT:   out of bounds
// SYS:       remark: affine GEMM 1x1x1 lowered as 1 exact region(s)

// A's row i * 2^62 + i * 2^62: the coefficient sum 2^63 does not fit
func.func @add_overflow(%A: memref<?x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<?x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 2 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i * 4611686018427387904 + %k * 0 + %i * 4611686018427387904, %k] : memref<?x8xf32>
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

// -----
// A's row (i + 2^62) * 2: the constant 2^63 does not fit
func.func @mul_overflow(%A: memref<?x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<?x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 2 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[(%i + 4611686018427387904) * 2, %k] : memref<?x8xf32>
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

// -----
// upper - lower = 2^63 + 1 does not fit: no trip count, no normalized form
func.func @trip_overflow(%A: memref<?x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<?x8xf32> {llvm.noalias}) {
  affine.for %i = -4611686018427387904 to 4611686018427387905 {
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

// -----
// T = diag(2^62, 2^62, 2^62) is invertible; every subscript is 0 (extent 1)
func.func @exact_rank(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 1 {
    affine.for %j = 0 to 1 {
      affine.for %k = 0 to 1 {
        %a = affine.load %A[%i * 4611686018427387904, %k * 4611686018427387904] : memref<8x8xf32>
        %b = affine.load %B[%k * 4611686018427387904, %j * 4611686018427387904] : memref<8x8xf32>
        %c = affine.load %C[%i * 4611686018427387904, %j * 4611686018427387904] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i * 4611686018427387904, %j * 4611686018427387904] : memref<8x8xf32>
      }
    }
  }
  return
}
