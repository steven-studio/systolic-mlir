// RUN: systolic-opt "--affine-matmul-to-systolic=report=true classify=true" \
// RUN:   -split-input-file %s 2>/dev/null | FileCheck %s --check-prefix=STRICT
// RUN: systolic-opt "--affine-matmul-to-systolic=report=true classify=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=SREMARK
// RUN: systolic-opt "--affine-matmul-to-systolic=report=true classify=true allow-fp-reassociation=true" \
// RUN:   -split-input-file %s 2>/dev/null | FileCheck %s
// RUN: systolic-opt "--affine-matmul-to-systolic=report=true classify=true allow-fp-reassociation=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=REMARK
//
// The hoist of repetition loops (docs/AFFINE_GEMM_MAPPING.md section 19.13, review of
// 2026-10-19 round 5) moves a loop no access reads outward when MLIR's dependence analysis
// proves the move reorders no dependent iterations. Two defects let it reorder the
// accumulation into C, also under strict floating point (wrong integer results):
//   R4H-1: an analysis that FAILS (a semi-affine subscript or bound) was read as "no
//          dependence"; now the hoist is not decided and the nest stays as written.
//   R4H-2: the 2^48 guard checked the maps as written only; a 2^63 constant reaching the
//          analysis through a constant or affine.apply operand, an affine.parallel bound or
//          an affine.if condition wrapped its 64-bit arithmetic. The guard now checks every
//          input as the analysis composes it.
// Each R4H case must keep its loops in the source order (no hoist) in both modes; each was
// also compiled with LLVM 24 and compared bitwise with its source on integer and float data
// (before the fix: 5 of 20 integer elements and 7 of 16 floats wrong). The controls are
// decided as before, and a legal hoist is still made.

// R4H-2: the bound max(-2^63, 0) reached through a constant symbol operand.
// STRICT-LABEL: func.func @g1_cst_sym
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = max {{#[a-z]+[0-9]*}}()[{{%c[-_0-9a-z]*}}] to 2 {
// STRICT-NOT: systolic.matmul_tile
// SREMARK: remark: affine GEMM not lowered to systolic.matmul_tile: loop 3 of the nest repeats the accumulation inside it, and whether moving it outward reorders dependent iterations is not decided (a bound or subscript has a constant of 2^48 or more, beyond the dependence analysis' 64-bit arithmetic) [UNKNOWN]
// CHECK-LABEL: func.func @g1_cst_sym
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = max {{#[a-z]+[0-9]*}}()[{{%c[-_0-9a-z]*}}] to 2 {
// CHECK-NOT: systolic.matmul_tile
// REMARK: remark: affine GEMM not lowered to systolic.matmul_tile: loop 3 of the nest repeats the accumulation inside it, and whether moving it outward reorders dependent iterations is not decided (a bound or subscript has a constant of 2^48 or more, beyond the dependence analysis' 64-bit arithmetic) [UNKNOWN]
func.func @g1_cst_sym(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  %big = arith.constant -9223372036854775808 : index

  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        affine.for %t = max affine_map<()[s0] -> (s0, 0)>()[%big] to 2 {
          %a = affine.load %A[%i, %k] : memref<4x4xf32>
          %b = affine.load %B[%k, %j] : memref<4x4xf32>
          %c = affine.load %C[%i, %j] : memref<4x4xf32>
          %p = arith.mulf %a, %b : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%i, %j] : memref<4x4xf32>
        }
      }
    }
  }

  return
}

// -----

// R4H-2: the same constant through a dim operand.
// STRICT-LABEL: func.func @g1_cst_dim
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = max {{#[a-z]+[0-9]*}}({{%c[-_0-9a-z]*}}) to 2 {
// STRICT-NOT: systolic.matmul_tile
// SREMARK: remark: affine GEMM not lowered to systolic.matmul_tile: loop 3 of the nest repeats the accumulation inside it, and whether moving it outward reorders dependent iterations is not decided (a bound or subscript has a constant of 2^48 or more, beyond the dependence analysis' 64-bit arithmetic) [UNKNOWN]
// CHECK-LABEL: func.func @g1_cst_dim
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = max {{#[a-z]+[0-9]*}}({{%c[-_0-9a-z]*}}) to 2 {
// CHECK-NOT: systolic.matmul_tile
// REMARK: remark: affine GEMM not lowered to systolic.matmul_tile: loop 3 of the nest repeats the accumulation inside it, and whether moving it outward reorders dependent iterations is not decided (a bound or subscript has a constant of 2^48 or more, beyond the dependence analysis' 64-bit arithmetic) [UNKNOWN]
func.func @g1_cst_dim(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  %big = arith.constant -9223372036854775808 : index

  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        affine.for %t = max affine_map<(d0) -> (d0, 0)>(%big) to 2 {
          %a = affine.load %A[%i, %k] : memref<4x4xf32>
          %b = affine.load %B[%k, %j] : memref<4x4xf32>
          %c = affine.load %C[%i, %j] : memref<4x4xf32>
          %p = arith.mulf %a, %b : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%i, %j] : memref<4x4xf32>
        }
      }
    }
  }

  return
}

// -----

// R4H-2: the same constant through an affine.apply operand.
// STRICT-LABEL: func.func @g2_apply
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = max {{#[a-z]+[0-9]*}}()[%0] to 2 {
// STRICT-NOT: systolic.matmul_tile
// SREMARK: remark: affine GEMM not lowered to systolic.matmul_tile: loop 3 of the nest repeats the accumulation inside it, and whether moving it outward reorders dependent iterations is not decided (a bound or subscript has a constant of 2^48 or more, beyond the dependence analysis' 64-bit arithmetic) [UNKNOWN]
// CHECK-LABEL: func.func @g2_apply
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = max {{#[a-z]+[0-9]*}}()[%0] to 2 {
// CHECK-NOT: systolic.matmul_tile
// REMARK: remark: affine GEMM not lowered to systolic.matmul_tile: loop 3 of the nest repeats the accumulation inside it, and whether moving it outward reorders dependent iterations is not decided (a bound or subscript has a constant of 2^48 or more, beyond the dependence analysis' 64-bit arithmetic) [UNKNOWN]
func.func @g2_apply(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  %m = affine.apply affine_map<() -> (-9223372036854775807 - 1)>()

  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        affine.for %t = max affine_map<()[s0] -> (s0, 0)>()[%m] to 2 {
          %a = affine.load %A[%i, %k] : memref<4x4xf32>
          %b = affine.load %B[%k, %j] : memref<4x4xf32>
          %c = affine.load %C[%i, %j] : memref<4x4xf32>
          %p = arith.mulf %a, %b : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%i, %j] : memref<4x4xf32>
        }
      }
    }
  }

  return
}

// -----

// R4H-2: the constant in the bound of an enclosing affine.parallel.
// STRICT-LABEL: func.func @g3_parallel
// STRICT: affine.parallel (%{{[a-z0-9]+}}) = (max(--9223372036854775808, 0)) to (1) {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 2 {
// STRICT-NOT: systolic.matmul_tile
// SREMARK: remark: affine GEMM not lowered to systolic.matmul_tile: loop 3 of the nest repeats the accumulation inside it, and whether moving it outward reorders dependent iterations is not decided (a bound or subscript has a constant of 2^48 or more, beyond the dependence analysis' 64-bit arithmetic) [UNKNOWN]
// CHECK-LABEL: func.func @g3_parallel
// CHECK: affine.parallel (%{{[a-z0-9]+}}) = (max(--9223372036854775808, 0)) to (1) {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 2 {
// CHECK-NOT: systolic.matmul_tile
// REMARK: remark: affine GEMM not lowered to systolic.matmul_tile: loop 3 of the nest repeats the accumulation inside it, and whether moving it outward reorders dependent iterations is not decided (a bound or subscript has a constant of 2^48 or more, beyond the dependence analysis' 64-bit arithmetic) [UNKNOWN]
func.func @g3_parallel(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {

  affine.parallel (%q) = (max(-9223372036854775808, 0)) to (1) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        affine.for %t = 0 to 2 {
          %a = affine.load %A[%i, %k] : memref<4x4xf32>
          %b = affine.load %B[%k, %j] : memref<4x4xf32>
          %c = affine.load %C[%i, %j] : memref<4x4xf32>
          %p = arith.mulf %a, %b : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%i, %j] : memref<4x4xf32>
        }
      }
    }
  }
  }
  return
}

// -----

// R4H-2: a coefficient 2^63 in the condition of an enclosing affine.if.
// STRICT-LABEL: func.func @g4_if_loop
// STRICT: affine.for %{{[a-z0-9]+}} = 2 to 3 {
// STRICT: affine.if {{#[a-z]+[0-9]*}}(%{{[a-z0-9]+}}) {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 2 {
// STRICT-NOT: systolic.matmul_tile
// SREMARK: remark: affine GEMM not lowered to systolic.matmul_tile: loop 3 of the nest repeats the accumulation inside it, and whether moving it outward reorders dependent iterations is not decided (a bound or subscript has a constant of 2^48 or more, beyond the dependence analysis' 64-bit arithmetic) [UNKNOWN]
// CHECK-LABEL: func.func @g4_if_loop
// CHECK: affine.for %{{[a-z0-9]+}} = 2 to 3 {
// CHECK: affine.if {{#[a-z]+[0-9]*}}(%{{[a-z0-9]+}}) {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 2 {
// CHECK-NOT: systolic.matmul_tile
// REMARK: remark: affine GEMM not lowered to systolic.matmul_tile: loop 3 of the nest repeats the accumulation inside it, and whether moving it outward reorders dependent iterations is not decided (a bound or subscript has a constant of 2^48 or more, beyond the dependence analysis' 64-bit arithmetic) [UNKNOWN]
func.func @g4_if_loop(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %d = 2 to 3 {
  affine.if affine_set<(d0) : (d0 * 4611686018427387904 * 2 >= 0)>(%d) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        affine.for %t = 0 to 2 {
          %a = affine.load %A[%i, %k] : memref<4x4xf32>
          %b = affine.load %B[%k, %j] : memref<4x4xf32>
          %c = affine.load %C[%i, %j] : memref<4x4xf32>
          %p = arith.mulf %a, %b : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%i, %j] : memref<4x4xf32>
        }
      }
    }
  }
  }
  }
  return
}

// -----

// R4H-1: a semi-affine subscript C[i * symbol(n)][j]: the dependence analysis fails.
// STRICT-LABEL: func.func @g5_semiaffine
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 2 {
// STRICT-NOT: systolic.matmul_tile
// SREMARK: remark: affine GEMM not lowered to systolic.matmul_tile: loop 3 of the nest repeats the accumulation inside it, and whether moving it outward reorders dependent iterations is not decided (the dependence analysis fails on an access or bound of the nest, e.g. a semi-affine subscript) [UNKNOWN]
// CHECK-LABEL: func.func @g5_semiaffine
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 2 {
// CHECK-NOT: systolic.matmul_tile
// REMARK: remark: affine GEMM not lowered to systolic.matmul_tile: loop 3 of the nest repeats the accumulation inside it, and whether moving it outward reorders dependent iterations is not decided (the dependence analysis fails on an access or bound of the nest, e.g. a semi-affine subscript) [UNKNOWN]
func.func @g5_semiaffine(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}, %n: index) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        affine.for %t = 0 to 2 {
          %a = affine.load %A[%i, %k] : memref<4x4xf32>
          %b = affine.load %B[%k, %j] : memref<4x4xf32>
          %c = affine.load %C[%i * symbol(%n), %j] : memref<4x4xf32>
          %p = arith.mulf %a, %b : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%i * symbol(%n), %j] : memref<4x4xf32>
        }
      }
    }
  }
  return
}

// -----

// R4H-1: a semi-affine loop bound.
// STRICT-LABEL: func.func @g5_semiaffine_bound
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to {{#[a-z]+[0-9]*}}()[%{{[a-z0-9]+}}] {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 2 {
// STRICT-NOT: systolic.matmul_tile
// SREMARK: remark: affine GEMM not lowered to systolic.matmul_tile: loop 4 of the nest repeats the accumulation inside it, and whether moving it outward reorders dependent iterations is not decided (the dependence analysis fails on an access or bound of the nest, e.g. a semi-affine subscript) [UNKNOWN]
// CHECK-LABEL: func.func @g5_semiaffine_bound
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to {{#[a-z]+[0-9]*}}()[%{{[a-z0-9]+}}] {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 2 {
// CHECK-NOT: systolic.matmul_tile
// REMARK: remark: affine GEMM not lowered to systolic.matmul_tile: loop 4 of the nest repeats the accumulation inside it, and whether moving it outward reorders dependent iterations is not decided (the dependence analysis fails on an access or bound of the nest, e.g. a semi-affine subscript) [UNKNOWN]
func.func @g5_semiaffine_bound(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}, %n: index) {
  affine.for %o = 0 to affine_map<()[s0] -> (s0 * s0)>()[%n] {
    affine.for %i = 0 to 4 {
      affine.for %j = 0 to 4 {
        affine.for %k = 0 to 4 {
          affine.for %t = 0 to 2 {
            %a = affine.load %A[%i, %k] : memref<4x4xf32>
            %b = affine.load %B[%k, %j] : memref<4x4xf32>
            %c = affine.load %C[%i, %j] : memref<4x4xf32>
            %p = arith.mulf %a, %b : f32
            %s = arith.addf %c, %p : f32
            affine.store %s, %C[%i, %j] : memref<4x4xf32>
          }
        }
      }
    }
  }
  return
}

// -----

// R4H-1: C[i * symbol(ld) + j], a strided C with a run-time leading dimension.
// STRICT-LABEL: func.func @g5_strided
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 2 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT-NOT: systolic.matmul_tile
// SREMARK: remark: affine GEMM not lowered to systolic.matmul_tile: loop 1 of the nest repeats the accumulation inside it, and whether moving it outward reorders dependent iterations is not decided (the dependence analysis fails on an access or bound of the nest, e.g. a semi-affine subscript) [UNKNOWN]
// CHECK-LABEL: func.func @g5_strided
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 2 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK-NOT: systolic.matmul_tile
// REMARK: remark: affine GEMM not lowered to systolic.matmul_tile: loop 1 of the nest repeats the accumulation inside it, and whether moving it outward reorders dependent iterations is not decided (the dependence analysis fails on an access or bound of the nest, e.g. a semi-affine subscript) [UNKNOWN]
func.func @g5_strided(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<16xf32> {llvm.noalias}, %ld: index) {
  affine.for %i = 0 to 4 {
    affine.for %r = 0 to 2 {
      affine.for %j = 0 to 4 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%i, %k] : memref<4x4xf32>
          %b = affine.load %B[%k, %j] : memref<4x4xf32>
          %c = affine.load %C[%i * symbol(%ld) + %j] : memref<16xf32>
          %p = arith.mulf %a, %b : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%i * symbol(%ld) + %j] : memref<16xf32>
        }
      }
    }
  }
  return
}

// -----

// R4H-1: C[i * symbol(ld) + j] stored, C[i * symbol(ld) + j + 1] read (a dependence ahead).
// STRICT-LABEL: func.func @g5_ahead
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 2 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT-NOT: systolic.matmul_tile
// SREMARK: remark: affine GEMM not lowered to systolic.matmul_tile: loop 1 of the nest repeats the accumulation inside it, and whether moving it outward reorders dependent iterations is not decided (the dependence analysis fails on an access or bound of the nest, e.g. a semi-affine subscript) [UNKNOWN]
// CHECK-LABEL: func.func @g5_ahead
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 2 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK-NOT: systolic.matmul_tile
// REMARK: remark: affine GEMM not lowered to systolic.matmul_tile: loop 1 of the nest repeats the accumulation inside it, and whether moving it outward reorders dependent iterations is not decided (the dependence analysis fails on an access or bound of the nest, e.g. a semi-affine subscript) [UNKNOWN]
func.func @g5_ahead(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<20xf32> {llvm.noalias}, %ld: index) {
  affine.for %i = 0 to 4 {
    affine.for %r = 0 to 2 {
      affine.for %j = 0 to 4 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%i, %k] : memref<4x4xf32>
          %b = affine.load %B[%k, %j] : memref<4x4xf32>
          %c = affine.load %C[%i * symbol(%ld) + %j + 1] : memref<20xf32>
          %p = arith.mulf %a, %b : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%i * symbol(%ld) + %j] : memref<20xf32>
        }
      }
    }
  }
  return
}

// -----

// control: max(-5, 0), small constants: decided as before.
// STRICT-LABEL: func.func @g1_ctrl_m5
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = max {{#[a-z]+[0-9]*}}()[{{%c[-_0-9a-z]*}}] to 2 {
// STRICT-NOT: systolic.matmul_tile
// SREMARK: remark: affine GEMM not lowered to systolic.matmul_tile: loop 3 of the nest repeats the accumulation inside it, and moving it outward is not a legal loop interchange (it would reorder dependent iterations) [UNKNOWN]
// CHECK-LABEL: func.func @g1_ctrl_m5
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = max {{#[a-z]+[0-9]*}}()[{{%c[-_0-9a-z]*}}] to 2 {
// CHECK-NOT: systolic.matmul_tile
// REMARK: remark: affine GEMM not lowered to systolic.matmul_tile: loop 3 of the nest repeats the accumulation inside it, and moving it outward is not a legal loop interchange (it would reorder dependent iterations) [UNKNOWN]
func.func @g1_ctrl_m5(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  %m5 = arith.constant -5 : index

  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        affine.for %t = max affine_map<()[s0] -> (s0, 0)>()[%m5] to 2 {
          %a = affine.load %A[%i, %k] : memref<4x4xf32>
          %b = affine.load %B[%k, %j] : memref<4x4xf32>
          %c = affine.load %C[%i, %j] : memref<4x4xf32>
          %p = arith.mulf %a, %b : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%i, %j] : memref<4x4xf32>
        }
      }
    }
  }

  return
}

// -----

// control: an affine.apply of a small constant.
// STRICT-LABEL: func.func @g2_apply_ctrl
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = max {{#[a-z]+[0-9]*}}()[%0] to 2 {
// STRICT-NOT: systolic.matmul_tile
// SREMARK: remark: affine GEMM not lowered to systolic.matmul_tile: loop 3 of the nest repeats the accumulation inside it, and moving it outward is not a legal loop interchange (it would reorder dependent iterations) [UNKNOWN]
// CHECK-LABEL: func.func @g2_apply_ctrl
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = max {{#[a-z]+[0-9]*}}()[%0] to 2 {
// CHECK-NOT: systolic.matmul_tile
// REMARK: remark: affine GEMM not lowered to systolic.matmul_tile: loop 3 of the nest repeats the accumulation inside it, and moving it outward is not a legal loop interchange (it would reorder dependent iterations) [UNKNOWN]
func.func @g2_apply_ctrl(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  %m = affine.apply affine_map<() -> (-5)>()

  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        affine.for %t = max affine_map<()[s0] -> (s0, 0)>()[%m] to 2 {
          %a = affine.load %A[%i, %k] : memref<4x4xf32>
          %b = affine.load %B[%k, %j] : memref<4x4xf32>
          %c = affine.load %C[%i, %j] : memref<4x4xf32>
          %p = arith.mulf %a, %b : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%i, %j] : memref<4x4xf32>
        }
      }
    }
  }

  return
}

// -----

// control: an affine.parallel with small bounds.
// STRICT-LABEL: func.func @g3_parallel_ctrl
// STRICT: affine.parallel (%{{[a-z0-9]+}}) = (max(-5, 0)) to (1) {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 2 {
// STRICT-NOT: systolic.matmul_tile
// SREMARK: remark: affine GEMM not lowered to systolic.matmul_tile: loop 3 of the nest repeats the accumulation inside it, and moving it outward is not a legal loop interchange (it would reorder dependent iterations) [UNKNOWN]
// CHECK-LABEL: func.func @g3_parallel_ctrl
// CHECK: affine.parallel (%{{[a-z0-9]+}}) = (max(-5, 0)) to (1) {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 2 {
// CHECK-NOT: systolic.matmul_tile
// REMARK: remark: affine GEMM not lowered to systolic.matmul_tile: loop 3 of the nest repeats the accumulation inside it, and moving it outward is not a legal loop interchange (it would reorder dependent iterations) [UNKNOWN]
func.func @g3_parallel_ctrl(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {

  affine.parallel (%q) = (max(-5, 0)) to (1) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        affine.for %t = 0 to 2 {
          %a = affine.load %A[%i, %k] : memref<4x4xf32>
          %b = affine.load %B[%k, %j] : memref<4x4xf32>
          %c = affine.load %C[%i, %j] : memref<4x4xf32>
          %p = arith.mulf %a, %b : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%i, %j] : memref<4x4xf32>
        }
      }
    }
  }
  }
  return
}

// -----

// control: an affine.if with a small condition.
// STRICT-LABEL: func.func @g4_if_loop_ctrl
// STRICT: affine.for %{{[a-z0-9]+}} = 2 to 3 {
// STRICT: affine.if {{#[a-z]+[0-9]*}}(%{{[a-z0-9]+}}) {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 2 {
// STRICT-NOT: systolic.matmul_tile
// SREMARK: remark: affine GEMM not lowered to systolic.matmul_tile: loop 3 of the nest repeats the accumulation inside it, and moving it outward is not a legal loop interchange (it would reorder dependent iterations) [UNKNOWN]
// CHECK-LABEL: func.func @g4_if_loop_ctrl
// CHECK: affine.for %{{[a-z0-9]+}} = 2 to 3 {
// CHECK: affine.if {{#[a-z]+[0-9]*}}(%{{[a-z0-9]+}}) {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 2 {
// CHECK-NOT: systolic.matmul_tile
// REMARK: remark: affine GEMM not lowered to systolic.matmul_tile: loop 3 of the nest repeats the accumulation inside it, and moving it outward is not a legal loop interchange (it would reorder dependent iterations) [UNKNOWN]
func.func @g4_if_loop_ctrl(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %d = 2 to 3 {
  affine.if affine_set<(d0) : (d0 * 4 >= 0)>(%d) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        affine.for %t = 0 to 2 {
          %a = affine.load %A[%i, %k] : memref<4x4xf32>
          %b = affine.load %B[%k, %j] : memref<4x4xf32>
          %c = affine.load %C[%i, %j] : memref<4x4xf32>
          %p = arith.mulf %a, %b : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%i, %j] : memref<4x4xf32>
        }
      }
    }
  }
  }
  }
  return
}

// -----

// control: the strided nest with an affine subscript.
// STRICT-LABEL: func.func @g5_strided_ctrl
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 2 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT-NOT: systolic.matmul_tile
// SREMARK: remark: affine GEMM not lowered to systolic.matmul_tile: loop 1 of the nest repeats the accumulation inside it, and moving it outward is not a legal loop interchange (it would reorder dependent iterations) [UNKNOWN]
// CHECK-LABEL: func.func @g5_strided_ctrl
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 2 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK-NOT: systolic.matmul_tile
// REMARK: remark: affine GEMM not lowered to systolic.matmul_tile: loop 1 of the nest repeats the accumulation inside it, and moving it outward is not a legal loop interchange (it would reorder dependent iterations) [UNKNOWN]
func.func @g5_strided_ctrl(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<16xf32> {llvm.noalias}, %ld: index) {
  affine.for %i = 0 to 4 {
    affine.for %r = 0 to 2 {
      affine.for %j = 0 to 4 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%i, %k] : memref<4x4xf32>
          %b = affine.load %B[%k, %j] : memref<4x4xf32>
          %c = affine.load %C[%i + %j] : memref<16xf32>
          %p = arith.mulf %a, %b : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%i + %j] : memref<16xf32>
        }
      }
    }
  }
  return
}

// -----

// control: the ahead nest with an affine subscript.
// STRICT-LABEL: func.func @g5_ahead_ctrl
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 2 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT-NOT: systolic.matmul_tile
// SREMARK: remark: affine GEMM not lowered to systolic.matmul_tile: loop 1 of the nest repeats the accumulation inside it, and moving it outward is not a legal loop interchange (it would reorder dependent iterations) [UNKNOWN]
// CHECK-LABEL: func.func @g5_ahead_ctrl
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 2 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// CHECK-NOT: systolic.matmul_tile
// REMARK: remark: affine GEMM not lowered to systolic.matmul_tile: loop 1 of the nest repeats the accumulation inside it, and moving it outward is not a legal loop interchange (it would reorder dependent iterations) [UNKNOWN]
func.func @g5_ahead_ctrl(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<20xf32> {llvm.noalias}, %ld: index) {
  affine.for %i = 0 to 4 {
    affine.for %r = 0 to 2 {
      affine.for %j = 0 to 4 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%i, %k] : memref<4x4xf32>
          %b = affine.load %B[%k, %j] : memref<4x4xf32>
          %c = affine.load %C[%i + %j + 1] : memref<20xf32>
          %p = arith.mulf %a, %b : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%i + %j] : memref<20xf32>
        }
      }
    }
  }
  return
}

// -----

// control: a legal hoist (each C element is updated by one i): still made.
// STRICT-LABEL: func.func @legal_hoist
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 2 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT: affine.for %{{[a-z0-9]+}} = 0 to 4 {
// STRICT-NOT: systolic.matmul_tile
// SREMARK: remark: affine GEMM not lowered to systolic.matmul_tile: floating-point reassociation is not permitted: the systolic execution sums each tile from zero and adds the sum to C, a different order of IEEE additions than the loop's (mark the accumulation reassoc -- systolic-cc --fp-reassociation -- or pass allow-fp-reassociation) [PROVEN_VALID]
// CHECK-LABEL: func.func @legal_hoist
// CHECK: affine.for %{{[a-z0-9]+}} = 0 to 2 {
// CHECK: systolic.matmul_tile
// REMARK: remark: affine GEMM 4x4x4 lowered as 1 exact region(s); the enclosing loops run it 2 time(s)
func.func @legal_hoist(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %r = 0 to 2 {
      affine.for %j = 0 to 4 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%i, %k] : memref<4x4xf32>
          %b = affine.load %B[%k, %j] : memref<4x4xf32>
          %c = affine.load %C[%i, %j] : memref<4x4xf32>
          %p = arith.mulf %a, %b : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%i, %j] : memref<4x4xf32>
        }
      }
    }
  }
  return
}
