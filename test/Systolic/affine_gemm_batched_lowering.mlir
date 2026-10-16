// Batched GEMMs (ISSUE-006, docs/AFFINE_GEMM_MAPPING.md section 2): the
// coordinates every operand reads (a partition prefix P*) are not tiled; the
// nest's loops that drive them -- with their own bounds and steps, in their
// original order -- are re-emitted around the tiles, so that every batch
// iteration runs the GEMM once on its own slice of each operand: exactly the
// original iteration domain. The batch index of an operand's batch dimension
// is the coordinate's expression over the new loops' counters (affine.apply),
// a dynamic offset of the operand's subview. The I, J, K views are built as
// for a plain GEMM. Nothing is required of the batch coordinate beyond what
// the loops already are (no density, no injectivity): two batch iterations
// that address the same slice accumulate into it twice, as the loop does.
//
// Refused: a loop that drives a batch coordinate and one of I, J, K (the
// batches are coupled: C[b][b + i], no box per batch value); a batch
// coordinate the operands read through different expressions (A[b + 1] with
// C[b]: one view per operand of a batch coordinate is not lowered); an
// element-wise product (every coordinate a batch one).
//
// RUN: systolic-opt "--affine-matmul-to-systolic=allow-fp-reassociation=true" \
// RUN:   -split-input-file %s | FileCheck %s
// RUN: systolic-opt "--affine-matmul-to-systolic=report=true allow-fp-reassociation=true classify=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=REMARK
// RUN: systolic-opt --test-affine-matmul-discovery -split-input-file %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s --check-prefix=DISC
//
// Through the fleet and down to the LLVM dialect -- a 16x16x16 batched GEMM is 10 tiles
// per batch iteration, each tile a slice of the batch's view (a subview of a
// subview with a dynamic offset: the LLVM pipeline lowers the batch loop
// before it expands the strided metadata, see SystolicPipelines.cpp). The
// 10 runtime calls sit inside the loop over the 2 batches.
// RUN: systolic-opt --systolic-declare-fpga-fleet "--affine-matmul-to-systolic=allow-fp-reassociation=true" \
// RUN:   --lower-systolic-matmul-tile-to-fpga --systolic-runtime-to-llvm -split-input-file %s \
// RUN:   | FileCheck %s --check-prefix=LLVM
// LLVM:          llvm.func @batched_tiles(
// LLVM-COUNT-10: llvm.call @fpga_matmul_tiled_auto_scheduled
// LLVM-NOT:      llvm.call @fpga_matmul_tiled_auto_scheduled
// LLVM-NOT:      affine.

// The canonical batched GEMM: C[b] += A[b] B[b], b in [0, 2).
// CHECK-LABEL: func.func @batched
// CHECK:       affine.for %[[B:.*]] = 0 to 2 {
// CHECK:         %[[IDX:.*]] = affine.apply #{{.*}}(%[[B]])
// CHECK:         memref.subview %arg0[%[[IDX]], 0, 0] [1, 4, 4] [1, 1, 1] : memref<2x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
// CHECK:         memref.subview %arg1[%[[IDX]], 0, 0] [1, 4, 4] [1, 1, 1]
// CHECK:         %[[CV:.*]] = memref.subview %arg2[%[[IDX]], 0, 0] [1, 4, 4] [1, 1, 1]
// CHECK:         systolic.matmul_tile {{.*}} m = 4 n = 4 k = 4
// CHECK:         bufferization.materialize_in_destination {{.*}} in restrict writable %[[CV]]
// CHECK-NOT:   affine.for
// REMARK:      remark: affine GEMM 4x4x4 lowered as 1 exact region(s), batched: 2 batch iteration(s) of 1 batch loop(s); the enclosing loops run it 2 time(s)
// DISC:        dense logical GEMM: yes, I/J/K on loops 1/2/3, batched over loop(s) 0 (2 batch iteration(s))
func.func @batched(%A: memref<2x4x4xf32> {llvm.noalias}, %B: memref<2x4x4xf32> {llvm.noalias}, %C: memref<2x4x4xf32> {llvm.noalias}) {
  affine.for %b = 0 to 2 {
    affine.for %i = 0 to 4 {
      affine.for %j = 0 to 4 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%b, %i, %k] : memref<2x4x4xf32>
          %bb = affine.load %B[%b, %k, %j] : memref<2x4x4xf32>
          %c = affine.load %C[%b, %i, %j] : memref<2x4x4xf32>
          %p = arith.mulf %a, %bb : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%b, %i, %j] : memref<2x4x4xf32>
        }
      }
    }
  }
  return
}

// -----
// The batch loop in the middle of the nest (i outside it), with a step and
// an origin: b = 1, 3, 5 of 7 slices (the loop is re-emitted as it is), B
// shared by the batches? no -- read at b as well. The batch loop moves
// outward: its iterations are independent GEMMs into their own C slices.
// CHECK-LABEL: func.func @batch_middle_step
// CHECK:       affine.for %[[B:.*]] = 1 to 7 step 2 {
// CHECK:         %[[IDX:.*]] = affine.apply #{{.*}}(%[[B]])
// CHECK:         memref.subview %arg0[%[[IDX]], 0, 0] [1, 8, 8] [1, 1, 1] : memref<7x8x8xf32> to memref<8x8xf32, strided<[8, 1], offset: ?>>
// CHECK:         systolic.matmul_tile {{.*}} m = 8 n = 8 k = 8
// CHECK-NOT:   affine.for
// REMARK:      remark: affine GEMM 8x8x8 lowered as 1 exact region(s), batched: 3 batch iteration(s) of 1 batch loop(s); the enclosing loops run it 3 time(s)
// DISC:        dense logical GEMM: yes, I/J/K on loops 0/2/3, batched over loop(s) 1 (3 batch iteration(s))
func.func @batch_middle_step(%A: memref<7x8x8xf32> {llvm.noalias}, %B: memref<7x8x8xf32> {llvm.noalias}, %C: memref<7x8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %b = 1 to 7 step 2 {
      affine.for %j = 0 to 8 {
        affine.for %k = 0 to 8 {
          %a = affine.load %A[%b, %i, %k] : memref<7x8x8xf32>
          %bb = affine.load %B[%b, %k, %j] : memref<7x8x8xf32>
          %c = affine.load %C[%b, %i, %j] : memref<7x8x8xf32>
          %p = arith.mulf %a, %bb : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%b, %i, %j] : memref<7x8x8xf32>
        }
      }
    }
  }
  return
}

// -----
// Two batch coordinates (p, q), each its own loop and dimension; the second
// batch index uses only the inner loop (its affine.apply still takes both
// counters).
// CHECK-LABEL: func.func @two_batch_coordinates
// CHECK:       affine.for %[[P:.*]] = 0 to 2 {
// CHECK:         affine.for %[[Q:.*]] = 0 to 3 {
// CHECK:           %[[IP:.*]] = affine.apply #{{.*}}(%[[P]], %[[Q]])
// CHECK:           %[[IQ:.*]] = affine.apply #{{.*}}(%[[P]], %[[Q]])
// CHECK:           memref.subview %arg0[%[[IP]], %[[IQ]], 0, 0] [1, 1, 4, 4] [1, 1, 1, 1] : memref<2x3x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
// CHECK:           systolic.matmul_tile {{.*}} m = 4 n = 4 k = 4
// REMARK:      remark: affine GEMM 4x4x4 lowered as 1 exact region(s), batched: 6 batch iteration(s) of 2 batch loop(s); the enclosing loops run it 6 time(s)
// DISC:        dense logical GEMM: yes, I/J/K on loops 2/3/4, batched over loop(s) 0+1 (6 batch iteration(s))
func.func @two_batch_coordinates(%A: memref<2x3x4x4xf32> {llvm.noalias}, %B: memref<2x3x4x4xf32> {llvm.noalias}, %C: memref<2x3x4x4xf32> {llvm.noalias}) {
  affine.for %p = 0 to 2 {
    affine.for %q = 0 to 3 {
      affine.for %i = 0 to 4 {
        affine.for %j = 0 to 4 {
          affine.for %k = 0 to 4 {
            %a = affine.load %A[%p, %q, %i, %k] : memref<2x3x4x4xf32>
            %bb = affine.load %B[%p, %q, %k, %j] : memref<2x3x4x4xf32>
            %c = affine.load %C[%p, %q, %i, %j] : memref<2x3x4x4xf32>
            %m = arith.mulf %a, %bb : f32
            %s = arith.addf %c, %m : f32
            affine.store %s, %C[%p, %q, %i, %j] : memref<2x3x4x4xf32>
          }
        }
      }
    }
  }
  return
}

// -----
// A batch with an offset and a batch dimension in the middle of A
// (A[i][b + 2][k]); the counting-down form of the front end (b = 2 - t).
// The batch index is 2 - t + 2 = 4 - t: slices 4, 3, 2.
// CHECK-LABEL: func.func @batch_offset_down_inner
// CHECK:       affine.for %[[T:.*]] = 0 to 3 {
// CHECK:         %[[IDX:.*]] = affine.apply #{{.*}}(%[[T]])
// CHECK:         memref.subview %arg0[0, %[[IDX]], 0] [4, 1, 4] [1, 1, 1] : memref<4x5x4xf32> to memref<4x4xf32, strided<[20, 1], offset: ?>>
// CHECK:         memref.subview %arg1[%[[IDX]], 0, 0] [1, 4, 4] [1, 1, 1]
// CHECK:         systolic.matmul_tile {{.*}} m = 4 n = 4 k = 4
// REMARK:      remark: affine GEMM 4x4x4 lowered as 1 exact region(s), batched: 3 batch iteration(s) of 1 batch loop(s); the enclosing loops run it 3 time(s)
// DISC:        dense logical GEMM: yes, I/J/K on loops 1/2/3, batched over loop(s) 0 (3 batch iteration(s))
func.func @batch_offset_down_inner(%A: memref<4x5x4xf32> {llvm.noalias}, %B: memref<5x4x4xf32> {llvm.noalias}, %C: memref<5x4x4xf32> {llvm.noalias}) {
  affine.for %t = 0 to 3 {
    affine.for %i = 0 to 4 {
      affine.for %j = 0 to 4 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%i, 4 - %t, %k] : memref<4x5x4xf32>
          %bb = affine.load %B[4 - %t, %k, %j] : memref<5x4x4xf32>
          %c = affine.load %C[4 - %t, %i, %j] : memref<5x4x4xf32>
          %m = arith.mulf %a, %bb : f32
          %s = arith.addf %c, %m : f32
          affine.store %s, %C[4 - %t, %i, %j] : memref<5x4x4xf32>
        }
      }
    }
  }
  return
}

// -----
// Coupled: the batch loop also drives I (C[b][b + i]): refused.
// CHECK-LABEL: func.func @batch_coupled
// CHECK:       affine.for
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      remark: affine GEMM not lowered to systolic.matmul_tile: two logical coordinates are driven by the same loop (loop 0 drives a batch coordinate and I) [PROVEN_INVALID]
// DISC:        dense logical GEMM: no (two logical coordinates are driven by the same loop (loop 0 drives a batch coordinate and I))
func.func @batch_coupled(%A: memref<2x5x4xf32> {llvm.noalias}, %B: memref<2x4x4xf32> {llvm.noalias}, %C: memref<2x5x4xf32> {llvm.noalias}) {
  affine.for %b = 0 to 2 {
    affine.for %i = 0 to 4 {
      affine.for %j = 0 to 4 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%b, %b + %i, %k] : memref<2x5x4xf32>
          %bb = affine.load %B[%b, %k, %j] : memref<2x4x4xf32>
          %c = affine.load %C[%b, %b + %i, %j] : memref<2x5x4xf32>
          %m = arith.mulf %a, %bb : f32
          %s = arith.addf %c, %m : f32
          affine.store %s, %C[%b, %b + %i, %j] : memref<2x5x4xf32>
        }
      }
    }
  }
  return
}

// -----
// A batch coordinate read through different expressions (A[b + 1], C[b]):
// each operand at its own batch index (ISSUE-007, 2026-10-16; refused before
// as "one per-operand view of a batch coordinate is not lowered").
// CHECK-LABEL: func.func @batch_views_differ
// CHECK:       affine.for %[[B:.*]] = 0 to 2 {
// CHECK:         %[[A1:.*]] = affine.apply #{{[a-z0-9_]+}}(%[[B]])
// CHECK:         memref.subview %arg0[%[[A1]], 0, 0] [1, 4, 4] [1, 1, 1]
// CHECK:         %[[B0:.*]] = affine.apply #{{[a-z0-9_]+}}(%[[B]])
// CHECK:         memref.subview %arg1[%[[B0]], 0, 0] [1, 4, 4] [1, 1, 1]
// CHECK:         memref.subview %arg2[%[[B0]], 0, 0] [1, 4, 4] [1, 1, 1]
// CHECK:         systolic.matmul_tile {{.*}} m = 4 n = 4 k = 4
// REMARK:      remark: affine GEMM 4x4x4 lowered as 1 exact region(s), batched: 2 batch iteration(s) of 1 batch loop(s)
// DISC:        batch terms A: dimension 0 = 1 + 1*x0
// DISC:        dense logical GEMM: yes, I/J/K on loops 1/2/3, batched over loop(s) 0 (2 batch iteration(s))
func.func @batch_views_differ(%A: memref<3x4x4xf32> {llvm.noalias}, %B: memref<2x4x4xf32> {llvm.noalias}, %C: memref<2x4x4xf32> {llvm.noalias}) {
  affine.for %b = 0 to 2 {
    affine.for %i = 0 to 4 {
      affine.for %j = 0 to 4 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%b + 1, %i, %k] : memref<3x4x4xf32>
          %bb = affine.load %B[%b, %k, %j] : memref<2x4x4xf32>
          %c = affine.load %C[%b, %i, %j] : memref<2x4x4xf32>
          %m = arith.mulf %a, %bb : f32
          %s = arith.addf %c, %m : f32
          affine.store %s, %C[%b, %i, %j] : memref<2x4x4xf32>
        }
      }
    }
  }
  return
}

// -----
// An element-wise product: every coordinate is read by all three operands.
// CHECK-LABEL: func.func @elementwise
// CHECK:       affine.for
// CHECK-NOT:   systolic.matmul_tile
// DISC:        reason: 2 coordinate(s) are read by all three operands and none by two: an element-wise product (a batched GEMM with M = N = K = 1), not a matrix multiplication
// DISC-NEXT:   class: PROVEN_INVALID
func.func @elementwise(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      %a = affine.load %A[%i, %j] : memref<4x4xf32>
      %bb = affine.load %B[%i, %j] : memref<4x4xf32>
      %c = affine.load %C[%i, %j] : memref<4x4xf32>
      %m = arith.mulf %a, %bb : f32
      %s = arith.addf %c, %m : f32
      affine.store %s, %C[%i, %j] : memref<4x4xf32>
    }
  }
  return
}

// -----
// The reduction index in the output with a single-iteration j: C[i][k] +=
// A[i][k] * B[k][1]. Every operand reads k (a batch coordinate), j runs once
// (N = 1), nothing is reduced (K = 1): per batch a column scaled by a scalar.
// Correct to lower, but not a matrix multiplication by the specification's
// list ("the reduction index in the output"): refused conservatively, open
// spec question SQ-5.
// CHECK-LABEL: func.func @batch_scalar_rows
// CHECK:       affine.for
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      remark: affine GEMM not lowered to systolic.matmul_tile: a batched GEMM whose tiles are a vector scaled by a scalar (two of M, N, K are 1) is not lowered pending spec question SQ-5 (the reduction index in the output) [VALID_BUT_LOWERING_UNSUPPORTED]
// DISC:        dense logical GEMM: no (a batched GEMM whose tiles are a vector scaled by a scalar
func.func @batch_scalar_rows(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x2xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 1 to 2 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i, %k] : memref<8x8xf32>
        %bb = affine.load %B[%k, %j] : memref<8x2xf32>
        %c = affine.load %C[%i, %k] : memref<8x8xf32>
        %m = arith.mulf %a, %bb : f32
        %s = arith.addf %c, %m : f32
        affine.store %s, %C[%i, %k] : memref<8x8xf32>
      }
    }
  }
  return
}

// -----
// An empty batch loop (0 to 0): the whole nest is empty, SQ-2 as for any
// empty loop -- refused with the empty-GEMM reason, not lowered.
// CHECK-LABEL: func.func @batch_empty
// CHECK:       affine.for
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      remark: affine GEMM not lowered to systolic.matmul_tile: a loop of the nest has no iteration (an empty GEMM: M, N or K is 0) [PROVEN_VALID]
func.func @batch_empty(%A: memref<2x4x4xf32> {llvm.noalias}, %B: memref<2x4x4xf32> {llvm.noalias}, %C: memref<2x4x4xf32> {llvm.noalias}) {
  affine.for %b = 0 to 0 {
    affine.for %i = 0 to 4 {
      affine.for %j = 0 to 4 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%b, %i, %k] : memref<2x4x4xf32>
          %bb = affine.load %B[%b, %k, %j] : memref<2x4x4xf32>
          %c = affine.load %C[%b, %i, %j] : memref<2x4x4xf32>
          %m = arith.mulf %a, %bb : f32
          %s = arith.addf %c, %m : f32
          affine.store %s, %C[%b, %i, %j] : memref<2x4x4xf32>
        }
      }
    }
  }
  return
}

// -----
// 16x16x16 per batch, 2 batches: 10 tiles of the fleet per batch iteration
// (checked down to LLVM by the LLVM RUN line; the other prefixes see one
// region without a fleet).
// CHECK-LABEL: func.func @batched_tiles
// CHECK:       affine.for %{{.*}} = 0 to 2 {
// CHECK:         systolic.matmul_tile {{.*}} m = 16 n = 16 k = 16
// REMARK:      remark: affine GEMM 16x16x16 lowered as 1 exact region(s), batched: 2 batch iteration(s) of 1 batch loop(s); the enclosing loops run it 2 time(s)
// DISC:        dense logical GEMM: yes, I/J/K on loops 1/2/3, batched over loop(s) 0 (2 batch iteration(s))
func.func @batched_tiles(%A: memref<2x16x16xf32> {llvm.noalias}, %B: memref<2x16x16xf32> {llvm.noalias}, %C: memref<2x16x16xf32> {llvm.noalias}) {
  affine.for %b = 0 to 2 {
    affine.for %i = 0 to 16 {
      affine.for %j = 0 to 16 {
        affine.for %k = 0 to 16 {
          %a = affine.load %A[%b, %i, %k] : memref<2x16x16xf32>
          %bb = affine.load %B[%b, %k, %j] : memref<2x16x16xf32>
          %c = affine.load %C[%b, %i, %j] : memref<2x16x16xf32>
          %m = arith.mulf %a, %bb : f32
          %s = arith.addf %c, %m : f32
          affine.store %s, %C[%b, %i, %j] : memref<2x16x16xf32>
        }
      }
    }
  }
  return
}
// REMARK-NOT:  remark:
