// RUN: systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic=allow-fp-reassociation=true \
// RUN:   -split-input-file -verify-diagnostics %s | FileCheck %s
// A nest refused at recognition gets no remark (it is not reported as a GEMM at all); the
// discovery pass prints its reason:
// RUN: systolic-opt --test-affine-matmul-discovery -split-input-file %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s --check-prefix=DISC

// GEMMs --affine-matmul-to-systolic recognizes but cannot show the lowering
// correct for. Each is left exactly as it was, with a remark saying why.

// Plain memref arguments may overlap: C written while A is read.
// CHECK-LABEL: func.func @may_alias
// CHECK:       affine.for
// CHECK-NOT:   systolic.matmul_tile
func.func @may_alias(%A: memref<4x4xf32>, %B: memref<4x4xf32>,
                     %C: memref<4x4xf32>) {
  // expected-remark @+1 {{affine GEMM not lowered to systolic.matmul_tile: A may overlap C; declare the arguments llvm.noalias (C restrict) or pass assume-no-alias}}
  affine.for %i = 0 to 4 {
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
  return
}

// -----

// A batched GEMM is lowered since 2026-10-15 (ISSUE-006), with batch-dependent views of
// the factors since ISSUE-007 and of C too since 2026-10-16 (C[b][i + b][j] is lowered:
// affine_gemm_batched_lowering.mlir @batch_coupled). What stays refused is a C view that
// is not injective within one batch: C[b][i + j] writes C[b][1] from (I, J) = (0, 1) and
// (1, 0) of the same batch.
// CHECK-LABEL: func.func @batched
// CHECK-NOT:   systolic.matmul_tile
// DISC:        reason: C's view is not injective on the (I, J) box: the outputs with ordinals (I, J) and (I + 1, J + -1) write the same element of C
// DISC-NEXT:   class: VALID_BUT_LOWERING_UNSUPPORTED
func.func @batched(%A: memref<2x4x4xf32> {llvm.noalias},
                   %B: memref<2x4x4xf32> {llvm.noalias},
                   %C: memref<2x7xf32> {llvm.noalias}) {
  affine.for %b = 0 to 2 {
    affine.for %i = 0 to 4 {
      affine.for %j = 0 to 4 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%b, %i, %k] : memref<2x4x4xf32>
          %bb = affine.load %B[%b, %k, %j] : memref<2x4x4xf32>
          %c = affine.load %C[%b, %i + %j] : memref<2x7xf32>
          %p = arith.mulf %a, %bb : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%b, %i + %j] : memref<2x7xf32>
        }
      }
    }
  }
  return
}

// -----

// C[i + j] (one dimension of C for both roles): the outputs (I, J) = (0, 1) and (1, 0)
// write C[1], so one tile would lose an update -- C's view is not injective (G2, docs/
// AFFINE_GEMM_MAPPING.md section 19.3). (A batched decomposition over j would be valid;
// it is not constructed.) Until 2026-10-16 this file held C[i + j][j] with A[i + j][k]
// here, refused as "loop 1 drives I and J"; that nest is a batched GEMV since then
// (affine_gemm_batch_views.mlir @c_mixes_batch).
// CHECK-LABEL: func.func @c_not_injective
// CHECK-NOT:   systolic.matmul_tile
// DISC:        reason: C's view is not injective on the (I, J) box: the outputs with ordinals (I, J) and (I + 1, J + -1) write the same element of C
// DISC-NEXT:   class: VALID_BUT_LOWERING_UNSUPPORTED
func.func @c_not_injective(%A: memref<4x4xf32> {llvm.noalias},
                           %B: memref<4x4xf32> {llvm.noalias},
                           %C: memref<7xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<4x4xf32>
        %b = affine.load %B[%k, %j] : memref<4x4xf32>
        %c = affine.load %C[%i + %j] : memref<7xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i + %j] : memref<7xf32>
      }
    }
  }
  return
}

// -----

// Rows 5 + 8 * q reach 157 of a 104-row memref.
// CHECK-LABEL: func.func @out_of_bounds
// CHECK-NOT:   systolic.matmul_tile
func.func @out_of_bounds(%A: memref<104x4xf32> {llvm.noalias},
                         %B: memref<4x4xf32> {llvm.noalias},
                         %C: memref<104x4xf32> {llvm.noalias}) {
  // expected-remark @+1 {{A is accessed out of bounds along dimension 0}}
  affine.for %q = 0 to 20 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[5 + 8 * %q, %k] : memref<104x4xf32>
        %b = affine.load %B[%k, %j] : memref<4x4xf32>
        %c = affine.load %C[5 + 8 * %q, %j] : memref<104x4xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[5 + 8 * %q, %j] : memref<104x4xf32>
      }
    }
  }
  return
}

// -----

// A symbolic trip count: no static M for the plan.
// CHECK-LABEL: func.func @symbolic
// CHECK-NOT:   systolic.matmul_tile
func.func @symbolic(%A: memref<?x4xf32> {llvm.noalias},
                    %B: memref<4x4xf32> {llvm.noalias},
                    %C: memref<?x4xf32> {llvm.noalias}, %n: index) {
  // expected-remark @+1 {{the loop bounds are not static, so the iteration domain has no static normalized extent}}
  affine.for %i = 0 to %n {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<?x4xf32>
        %b = affine.load %B[%k, %j] : memref<4x4xf32>
        %c = affine.load %C[%i, %j] : memref<?x4xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<?x4xf32>
      }
    }
  }
  return
}

// -----

// Not recognized at all (no remark): C is initialized between the j and k
// loops, which a GEMM replacing the nest would drop.
// CHECK-LABEL: func.func @initialized
// CHECK:       affine.store %{{.*}}, %arg2
// CHECK-NOT:   systolic.matmul_tile
func.func @initialized(%A: memref<4x4xf32> {llvm.noalias},
                       %B: memref<4x4xf32> {llvm.noalias},
                       %C: memref<4x4xf32> {llvm.noalias}) {
  %zero = arith.constant 0.0 : f32
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.store %zero, %C[%i, %j] : memref<4x4xf32>
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
  return
}

// -----

// Not recognized: another side effect in the MAC loop.
// CHECK-LABEL: func.func @side_effect
// CHECK:       func.call @observe
// CHECK-NOT:   systolic.matmul_tile
func.func private @observe(f32)
func.func @side_effect(%A: memref<4x4xf32> {llvm.noalias},
                       %B: memref<4x4xf32> {llvm.noalias},
                       %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<4x4xf32>
        %b = affine.load %B[%k, %j] : memref<4x4xf32>
        %c = affine.load %C[%i, %j] : memref<4x4xf32>
        %p = arith.mulf %a, %b : f32
        func.call @observe(%p) : (f32) -> ()
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<4x4xf32>
      }
    }
  }
  return
}

// -----

// C is a view (here a cast) of A's allocation: a fresh allocation, but the
// same buffer, so it may overlap.
// CHECK-LABEL: func.func @same_buffer
// CHECK-NOT:   systolic.matmul_tile
func.func @same_buffer(%B: memref<4x4xf32>) {
  %A = memref.alloc() : memref<4x4xf32>
  %C = memref.cast %A : memref<4x4xf32> to memref<4x4xf32>
  // expected-remark @+1 {{A may overlap C}}
  affine.for %i = 0 to 4 {
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
  return
}

// -----

// C += C x B, even with C restrict: A is C.
// CHECK-LABEL: func.func @self
// CHECK-NOT:   systolic.matmul_tile
func.func @self(%B: memref<4x4xf32> {llvm.noalias},
                %C: memref<4x4xf32> {llvm.noalias}) {
  // expected-remark @+1 {{A may overlap C}}
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %C[%i, %k] : memref<4x4xf32>
        %b = affine.load %B[%k, %j] : memref<4x4xf32>
        %c = affine.load %C[%i, %j] : memref<4x4xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<4x4xf32>
      }
    }
  }
  return
}
