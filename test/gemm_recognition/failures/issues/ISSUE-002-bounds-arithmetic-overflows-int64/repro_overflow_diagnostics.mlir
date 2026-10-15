// ISSUE-002, diagnostics: a nest without a normalized form is reported as an
// overflow if, and only if, a checked operation of the normalization actually
// failed (LogicalMatmul::normalizationOverflow, set by
// normalizeLogicalCoordinates). An earlier version of the integration
// inferred "overflow" from the Presburger source domain having constant
// bounds, and so called the first three nests below overflows although no
// value in them comes near 2^63 (measured 2026-10-15). An empty loop (SQ-2)
// is reported as an empty GEMM (LogicalMatmul::normalizationEmptyLoop, since
// 2026-10-15; before, it got the wording meant for symbolic bounds); every
// other nest keeps "the loop bounds are not static ...", which the showcase
// predictions of SQ-3 check.
//
// No INT64_MIN literal here: parsing it needs MLIR at or after llvm-project
// e803069070cf (see repro_overflow_int64min_literal.mlir).
//
// RUN: systolic-opt --systolic-declare-fpga-fleet --test-affine-matmul-discovery \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=DISC
// RUN: systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic=report=true \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=SYS
//
// DISC-NOT:  normalized form overflows
// DISC:      dense logical GEMM: no (the loop bounds are not static, so the iteration domain has no static normalized extent)
// DISC-NOT:  normalized form overflows
// DISC:      dense logical GEMM: no (the loop bounds are not static, so the iteration domain has no static normalized extent)
// DISC-NOT:  normalized form overflows
// DISC:      dense logical GEMM: no (the loop bounds are not static, so the iteration domain has no static normalized extent)
// DISC-NOT:  normalized form overflows
// DISC:      dense logical GEMM: no (a loop of the nest has no iteration (an empty GEMM: M, N or K is 0))
// DISC-NOT:  normalized form overflows
// DISC:      dense logical GEMM: no (a loop of the nest has no iteration (an empty GEMM: M, N or K is 0))
// DISC-NOT:  loop bounds are not static
// DISC:      dense logical GEMM: no (the iteration domain's normalized form overflows 64-bit arithmetic)
// DISC-NOT:  loop bounds are not static
// DISC:      dense logical GEMM: no (the iteration domain's normalized form overflows 64-bit arithmetic)
// DISC-NOT:  loop bounds are not static
// DISC:      dense logical GEMM: no (the iteration domain's normalized form overflows 64-bit arithmetic)
//
// SYS-NOT:   normalized form overflows
// SYS:       remark: affine GEMM not lowered to systolic.matmul_tile: the loop bounds are not static, so the iteration domain has no static normalized extent
// SYS-NOT:   normalized form overflows
// SYS:       remark: affine GEMM not lowered to systolic.matmul_tile: the loop bounds are not static, so the iteration domain has no static normalized extent
// SYS-NOT:   normalized form overflows
// SYS:       remark: affine GEMM not lowered to systolic.matmul_tile: the loop bounds are not static, so the iteration domain has no static normalized extent
// SYS-NOT:   normalized form overflows
// SYS:       remark: affine GEMM not lowered to systolic.matmul_tile: a loop of the nest has no iteration (an empty GEMM: M, N or K is 0)
// SYS-NOT:   normalized form overflows
// SYS:       remark: affine GEMM not lowered to systolic.matmul_tile: a loop of the nest has no iteration (an empty GEMM: M, N or K is 0)
// SYS-NOT:   loop bounds are not static
// SYS:       remark: affine GEMM not lowered to systolic.matmul_tile: the iteration domain's normalized form overflows 64-bit arithmetic
// SYS-NOT:   loop bounds are not static
// SYS:       remark: affine GEMM not lowered to systolic.matmul_tile: the iteration domain's normalized form overflows 64-bit arithmetic
// SYS-NOT:   loop bounds are not static
// SYS:       remark: affine GEMM not lowered to systolic.matmul_tile: the iteration domain's normalized form overflows 64-bit arithmetic
// SYS-NOT:   lowered as

// upper bound min(8, 16): not one constant bound; no overflow anywhere
func.func @min_const_ub(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to min affine_map<() -> (8, 16)>() {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i, %k] : memref<8x8xf32>
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
// upper bound min(s0, 8): not one constant bound; no overflow anywhere
func.func @min_sym_ub(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}, %n: index) {
  affine.for %i = 0 to min affine_map<()[s0] -> (s0, 8)>()[%n] {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i, %k] : memref<8x8xf32>
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
// j < i + 1: a bound that depends on another loop (not a box); no overflow anywhere
func.func @triangular(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to affine_map<(d0) -> (d0 + 1)>(%i) {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i, %k] : memref<8x8xf32>
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
// an empty loop, 0 to 0 (SQ-2); no overflow anywhere
func.func @empty_zero(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 0 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i, %k] : memref<8x8xf32>
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
// an empty loop, 8 to 0 (SQ-2); no overflow anywhere
func.func @empty_reversed(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 8 to 0 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i, %k] : memref<8x8xf32>
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
// genuine overflow: upper - lower = 2^63 + 1 does not fit
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
// genuine overflow: T x step = 4 * 2^62 does not fit
func.func @step_overflow(%A: memref<?x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<?x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 2 step 4611686018427387904 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i * 4, %k] : memref<?x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x8xf32>
        %c = affine.load %C[%i * 4, %j] : memref<?x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i * 4, %j] : memref<?x8xf32>
      }
    }
  }
  return
}

// -----
// genuine overflow: T x origin = 4 * 2^62 does not fit
func.func @origin_overflow(%A: memref<?x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<?x8xf32> {llvm.noalias}) {
  affine.for %i = 4611686018427387904 to 4611686018427387906 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i * 4, %k] : memref<?x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x8xf32>
        %c = affine.load %C[%i * 4, %j] : memref<?x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i * 4, %j] : memref<?x8xf32>
      }
    }
  }
  return
}
