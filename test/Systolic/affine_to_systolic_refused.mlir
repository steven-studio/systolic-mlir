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
// affine_gemm_batched_lowering.mlir @batch_coupled). What stayed refused was a C view that
// is not injective within one batch: C[b][i + j] writes C[b][1] from (I, J) = (0, 1) and
// (1, 0) of the same batch.
// RE-DECIDED 2026-10-21 (docs section 25, ordered output batches of a non-injective C):
// lowered. The collision is a reduction of an accumulation, and it is kept by making the
// output role I (loop 1) batch loops too: b (loop 0, read by A, B and C) and i (loop 1,
// demoted) are the two outermost loops of the nest, so the 2 * 4 = 8 batches run in the
// order written, each reading C, adding its tile and writing C back before the next one
// reads it. Per batch the tile is the GEMV C[b][i + J] += A[b][i][K] * B[b][K][J],
// M x N x K = 1 x 4 x 4, whose four outputs are distinct (G2 holds on every tile); only the
// order over K inside one element changes (allow-fp-reassociation). Numerically the same
// kernel through systolic-cc --verify, sync and --submission async, soft and
// SOFT_RUNTIME_MOCK_HARDWARE=1, integer data, nonzero initial C: bitwise equal to the loops
// as written, with 10 elements of C updated by two or more batches. Its assignment twin
// @batched_assign_collision (next) stays refused.
// CHECK-LABEL: func.func @batched
// CHECK:       affine.for %{{.*}} = 0 to 2 {
// CHECK-NEXT:    affine.for %{{.*}} = 0 to 4 {
// CHECK:           memref.subview %arg2[%{{.*}}, %{{.*}}] [1, 4] [1, 1] : memref<2x7xf32> to memref<4xf32, strided<[1], offset: ?>>
// CHECK:           systolic.matmul_tile {{.*}} m = 1 n = 4 k = 4
// CHECK-NOT:       systolic.matmul_tile
// CHECK:           bufferization.materialize_in_destination
// DISC:        batch terms C: dimension 0 = 0 + 1*x0, dimension 1 += 1*x1
// DISC-NEXT:   statement: C = C + A * B (accumulation)
// DISC-NEXT:   partition: loop(s) 1 demoted to batch loops
// DISC:        dense logical GEMM: yes, I/J/K on loops /2/3, batched over loop(s) 0+1 (8 batch iteration(s))
// DISC-NEXT:   dense extents: M 1, N 4, K 4
// DISC-NEXT:   logical bijection: verified
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

// The assignment twin of @batched (stays refused, docs sections 20 and 25):
// C[b][i + j] = A[b][i] * B[b][j] writes C[b][1] from (i, j) = (0, 1) and (1, 0) of one
// batch; an assignment keeps only the last product, so batching i would not make it the
// GEMM of A and B -- the ordered output batches are made for accumulations only.
// CHECK-LABEL: func.func @batched_assign_collision
// CHECK-NOT:   systolic.matmul_tile
// DISC:        reason: C's view is not injective on the (I, J) box: the outputs with ordinals (I, J) and (I + 1, J + -1) write the same element of C: the assignment C = A * B overwrites it, only the last product survives
// DISC-NEXT:   class: PROVEN_INVALID
func.func @batched_assign_collision(%A: memref<2x4xf32> {llvm.noalias},
                                    %B: memref<2x4xf32> {llvm.noalias},
                                    %C: memref<2x7xf32> {llvm.noalias}) {
  affine.for %b = 0 to 2 {
    affine.for %i = 0 to 4 {
      affine.for %j = 0 to 4 {
        %a = affine.load %A[%b, %i] : memref<2x4xf32>
        %bb = affine.load %B[%b, %j] : memref<2x4xf32>
        %p = arith.mulf %a, %bb : f32
        affine.store %p, %C[%b, %i + %j] : memref<2x7xf32>
      }
    }
  }
  return
}
// -----

// C[i + j] (one dimension of C for both roles): the outputs (I, J) = (0, 1) and (1, 0)
// write C[1], so one tile would lose an update -- C's view is not injective (G2, docs/
// AFFINE_GEMM_MAPPING.md section 19.3). (A batched decomposition over j would be valid;
// it was not constructed.) Until 2026-10-16 this file held C[i + j][j] with A[i + j][k]
// here, refused as "loop 1 drives I and J"; that nest is a batched GEMV since then
// (affine_gemm_batch_views.mlir @c_mixes_batch).
// RE-DECIDED 2026-10-21 (docs section 25, ordered output batches of a non-injective C):
// lowered. The accumulation's colliding outputs are a reduction that must be kept: the
// output role I (loop 0, the outermost loop of the nest) becomes a batch loop, so the 4
// batches run in the order written and each reads the C the previous one wrote. Per batch
// the tile is the GEMV C[i + J] += A[i][K] * B[K][J], M x N x K = 1 x 4 x 4 (B shared),
// injective on its four outputs. Numerically (the same kernel through systolic-cc --verify,
// sync and async, soft and mock, integer data, nonzero initial C): bitwise equal to the
// loops as written; C[1] .. C[5] receive contributions of 2, 3, 4, 3 and 2 batches, each
// nonzero. Its assignment twin @c_not_injective_assign (next) stays refused.
// CHECK-LABEL: func.func @c_not_injective
// CHECK:       affine.for %{{.*}} = 0 to 4 {
// CHECK:         memref.subview %arg2[%{{.*}}] [4] [1] : memref<7xf32> to memref<4xf32, strided<[1], offset: ?>>
// CHECK:         systolic.matmul_tile {{.*}} m = 1 n = 4 k = 4
// CHECK-NOT:     systolic.matmul_tile
// CHECK:         bufferization.materialize_in_destination
// DISC:        batch terms C: dimension 0 += 1*x0
// DISC-NEXT:   statement: C = C + A * B (accumulation)
// DISC-NEXT:   partition: loop(s) 0 demoted to batch loops
// DISC:        dense logical GEMM: yes, I/J/K on loops /1/2, batched over loop(s) 0 (4 batch iteration(s))
// DISC-NEXT:   dense extents: M 1, N 4, K 4
// DISC-NEXT:   logical bijection: verified
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

// The assignment twin of @c_not_injective (stays refused, docs sections 20 and 25):
// C[i + j] = A[i] * B[j] overwrites C[1] .. C[5]; only the last product survives.
// CHECK-LABEL: func.func @c_not_injective_assign
// CHECK-NOT:   systolic.matmul_tile
// DISC:        reason: C's view is not injective on the (I, J) box: the outputs with ordinals (I, J) and (I + 1, J + -1) write the same element of C: the assignment C = A * B overwrites it, only the last product survives
// DISC-NEXT:   class: PROVEN_INVALID
func.func @c_not_injective_assign(%A: memref<4xf32> {llvm.noalias},
                                  %B: memref<4xf32> {llvm.noalias},
                                  %C: memref<7xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      %a = affine.load %A[%i] : memref<4xf32>
      %b = affine.load %B[%j] : memref<4xf32>
      %p = arith.mulf %a, %b : f32
      affine.store %p, %C[%i + %j] : memref<7xf32>
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
