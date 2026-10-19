// ISSUE-002: our INT64_MIN and checked-addition guards, reached WITHOUT the
// INT64_MIN literal (every literal here is in (-2^63, 2^63), so the file
// parses with any MLIR).
//
// A coefficient of exactly INT64_MIN is written as -2^62 + (-2^62): MLIR
// merges the like terms into `d0 * -9223372036854775808` with a constant
// right-hand side (measured with llvm-project 7615da10b75c); if an MLIR did
// not merge them, extractLinearForm's checked addition would produce the same
// INT64_MIN (it fits). Either way the pass sees an INT64_MIN coefficient.
//
//   @min_coef_one_operand  A's row has coefficient INT64_MIN, C's row is i.
//                          matchSharedCoordinate's guard (an INT64_MIN
//                          coefficient has no absolute value for the gcd)
//                          refuses it: measured with gdb on a Debug build, the
//                          guard's `return 0` executes with coefficient =
//                          -9223372036854775808. (Every build refuses this
//                          nest at recognition -- builds without
//                          matchSharedCoordinate by expression mismatch -- so
//                          this function guards against a crash or acceptance,
//                          not a wording.)
//   @min_coef_identical    A and C both i * INT64_MIN: T = diag(INT64_MIN, 1, 1)
//                          is invertible. The exact determinant says so (builds
//                          with the int64 elimination answered "no"); Rational's
//                          INT64_MIN guard skips the debug-only inverse and
//                          buildExactLogicalDomain's checked -INT64_MIN leaves
//                          the optional domain unset (both measured with gdb);
//                          the nest is refused because I decreases along its
//                          loop with |coefficient| = 2^63, which reading the
//                          loop backwards (getDenseLogicalGemm, for downward
//                          coordinates) would need and int64 cannot hold.
//   @add_overflow_unmerged (i + 2^62 - 1) * 2 + 2^62: MLIR keeps the expression
//                          unsimplified, the product fits (2^63 - 2) and the
//                          sum does not: extractLinearForm's checked addition
//                          refuses it at recognition. Without the check the
//                          wrapped constant became A's view offset and the nest
//                          was refused only as "out of bounds" (measured on the
//                          Mac fix).
//
// RUN: systolic-opt --systolic-declare-fpga-fleet --test-affine-matmul-discovery \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=DISC
// RUN: systolic-opt --systolic-declare-fpga-fleet "--affine-matmul-to-systolic=report=true allow-fp-reassociation=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=SYS
//
// DISC:      recognition result: failure
// DISC:      recognition result: success
// DISC:      invertible: yes
// DISC:      dense logical GEMM: no (logical coordinate I's scale or extent overflows 64-bit arithmetic)
// DISC:      recognition result: failure
// DISC-NOT:  recognition result: success
//
// SYS-NOT:   out of bounds
// SYS:       remark: affine GEMM not lowered to systolic.matmul_tile: logical coordinate I's scale or extent overflows 64-bit arithmetic
// SYS-NOT:   remark:

// A's row i * INT64_MIN (as -2^62 i - 2^62 i), C's row i
func.func @min_coef_one_operand(%A: memref<?x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<?x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 2 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i * -4611686018427387904 + %i * -4611686018427387904, %k] : memref<?x8xf32>
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
// A's and C's row both i * INT64_MIN
func.func @min_coef_identical(%A: memref<?x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<?x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 2 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i * -4611686018427387904 + %i * -4611686018427387904, %k] : memref<?x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x8xf32>
        %c = affine.load %C[%i * -4611686018427387904 + %i * -4611686018427387904, %j] : memref<?x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i * -4611686018427387904 + %i * -4611686018427387904, %j] : memref<?x8xf32>
      }
    }
  }
  return
}

// -----
// A's row (i + 2^62 - 1) * 2 + 2^62: the sum 2^63 - 2 + 2^62 does not fit
func.func @add_overflow_unmerged(%A: memref<?x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<?x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 2 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[(%i + 4611686018427387903) * 2 + 4611686018427387904, %k] : memref<?x8xf32>
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
