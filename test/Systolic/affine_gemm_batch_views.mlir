// Batch-dependent operand views (ISSUE-007, docs/AFFINE_GEMM_MAPPING.md
// section 16): a loop read by all three operands is a batch loop; the factors
// may read it anywhere -- in a dimension of its own or as an affine offset of a
// role's dimension (A[4i - j][5k]) -- each through its own expression, while C
// must read every batch coordinate alone in a dimension of its own. For every
// value of the batch loops the role loops form an ordinary dense GEMM of
// strided views whose base offsets are affine in the batch counters; the
// lowering re-emits the batch loops around the tiles and computes each such
// offset with an affine.apply on their counters.
//
// RUN: systolic-opt "--affine-matmul-to-systolic=allow-fp-reassociation=true" \
// RUN:   -split-input-file %s | FileCheck %s
// RUN: systolic-opt "--affine-matmul-to-systolic=report=true allow-fp-reassociation=true classify=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=REMARK
// RUN: systolic-opt --test-affine-matmul-discovery -split-input-file %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s --check-prefix=DISC
//
// Through the fleet down to the LLVM dialect: the views with dynamic offsets
// survive tiling, staging and the strided-metadata expansion.
// RUN: systolic-opt --systolic-declare-fpga-fleet "--affine-matmul-to-systolic=allow-fp-reassociation=true" \
// RUN:   --lower-systolic-matmul-tile-to-fpga --systolic-runtime-to-llvm -split-input-file %s \
// RUN:   | FileCheck %s --check-prefix=LLVM
// LLVM-LABEL: llvm.func @phase1_target(
// LLVM:       llvm.call @fpga_matmul_tiled_auto_scheduled
// LLVM-LABEL: llvm.func @phase1_tiles(
// LLVM:       llvm.call @fpga_matmul_tiled_auto_scheduled
// LLVM-LABEL: llvm.func @two_batch_offsets(
// LLVM:       llvm.call @fpga_matmul_tiled_auto_scheduled
// LLVM-NOT:   affine.

// The Phase 1 target: C[8i][j] += A[4i - j][5k] * B[5k][j], i in [1, 5),
// j in [0, 4), k in [0, 4). Readers: i by A and C (I), k by A and B (K), j by
// all three (a batch loop). C reads j alone in its dimension 1 and I in its
// dimension 0; A reads I in dimension 0 with the batch offset -j; B reads j
// alone in its dimension 1; no loop drives J (N = 1). Per batch value j:
// C(8I, j) += sum_K A(4I - j, 5K) * B(5K, j), M = 4, N = 1, K = 4. A's row
// index runs over [4*1 - 3, 4*4 - 0] = [1, 16], inside its 17 rows.
// CHECK-LABEL: func.func @phase1_target
// CHECK:       affine.for %[[J:.*]] = 0 to 4 {
// CHECK:         %[[AOFF:.*]] = affine.apply #{{[a-z0-9_]+}}(%[[J]])
// CHECK:         memref.subview %arg0[%[[AOFF]], 0] [4, 4] [4, 5]
// CHECK:         systolic.matmul_tile {{.*}} m = 4 n = 1 k = 4
// REMARK:      remark: affine GEMM 4x1x4 lowered as 1 exact region(s), batched: 4 batch iteration(s) of 1 batch loop(s)
// DISC:        recognition result: success
// DISC:        batch terms A: dimension 0 += -1*x1
// DISC:        batch terms B: dimension 1 = 0 + 1*x1
// DISC:        batch terms C: dimension 1 = 0 + 1*x1
// DISC:        dense logical GEMM: yes, I/J/K on loops 0//2, batched over loop(s) 1 (4 batch iteration(s))
func.func @phase1_target(%A: memref<17x16xf32> {llvm.noalias}, %B: memref<16x4xf32> {llvm.noalias}, %C: memref<33x4xf32> {llvm.noalias}) {
  affine.for %i = 1 to 5 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i * 4 - %j, %k * 5] : memref<17x16xf32>
        %b = affine.load %B[%k * 5, %j] : memref<16x4xf32>
        %c = affine.load %C[%i * 8, %j] : memref<33x4xf32>
        %m = arith.mulf %a, %b : f32
        %s = arith.addf %c, %m : f32
        affine.store %s, %C[%i * 8, %j] : memref<33x4xf32>
      }
    }
  }
  return
}

// -----
// The same mapping with i in [0, 4): A's row index 4i - j reaches -3 (i = 0,
// j = 3). The mapping is recognized and dense, but the program itself reads
// outside A: refused by the bounds check, never lowered.
// CHECK-LABEL: func.func @phase1_from_zero
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      remark: affine GEMM not lowered to systolic.matmul_tile: A is accessed out of bounds along dimension 0 [PROVEN_INVALID]
// DISC:        recognition result: success
// DISC:        dense logical GEMM: yes, I/J/K on loops 0//2, batched over loop(s) 1 (4 batch iteration(s))
func.func @phase1_from_zero(%A: memref<13x16xf32> {llvm.noalias}, %B: memref<16x4xf32> {llvm.noalias}, %C: memref<25x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i * 4 - %j, %k * 5] : memref<13x16xf32>
        %b = affine.load %B[%k * 5, %j] : memref<16x4xf32>
        %c = affine.load %C[%i * 8, %j] : memref<25x4xf32>
        %m = arith.mulf %a, %b : f32
        %s = arith.addf %c, %m : f32
        affine.store %s, %C[%i * 8, %j] : memref<25x4xf32>
      }
    }
  }
  return
}

// -----
// The same mapping, large enough to need several tiles per batch value:
// i in [1, 17), k in [0, 16) -- M = 16, N = 1, K = 16 per batch value.
// A's row index runs over [1, 64], its column over [0, 75].
// CHECK-LABEL: func.func @phase1_tiles
// CHECK:       affine.for %[[J:.*]] = 0 to 4 {
// CHECK:         systolic.matmul_tile {{.*}} m = 16 n = 1 k = 16
// REMARK:      remark: affine GEMM 16x1x16 lowered as 1 exact region(s), batched: 4 batch iteration(s) of 1 batch loop(s)
func.func @phase1_tiles(%A: memref<65x76xf32> {llvm.noalias}, %B: memref<76x4xf32> {llvm.noalias}, %C: memref<129x4xf32> {llvm.noalias}) {
  affine.for %i = 1 to 17 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 16 {
        %a = affine.load %A[%i * 4 - %j, %k * 5] : memref<65x76xf32>
        %b = affine.load %B[%k * 5, %j] : memref<76x4xf32>
        %c = affine.load %C[%i * 8, %j] : memref<129x4xf32>
        %m = arith.mulf %a, %b : f32
        %s = arith.addf %c, %m : f32
        affine.store %s, %C[%i * 8, %j] : memref<129x4xf32>
      }
    }
  }
  return
}

// -----
// The second factor's view moves with the batch: B[k][j + b]. Per batch value
// b, B'(K, J) = B(K, J + b).
// CHECK-LABEL: func.func @b_offset
// CHECK:       affine.for %[[BB:.*]] = 0 to 2 {
// CHECK:         %[[AIDX:.*]] = affine.apply #{{[a-z0-9_]+}}(%[[BB]])
// CHECK:         memref.subview %arg0[%[[AIDX]], 0, 0] [1, 4, 4] [1, 1, 1]
// CHECK:         memref.subview %arg1[0, %[[AIDX]]] [4, 4] [1, 1]
// CHECK:         systolic.matmul_tile {{.*}} m = 4 n = 4 k = 4
// REMARK:      remark: affine GEMM 4x4x4 lowered as 1 exact region(s), batched: 2 batch iteration(s) of 1 batch loop(s)
// DISC:        recognition result: success
// DISC:        batch terms B: dimension 1 += 1*x0
// DISC:        dense logical GEMM: yes, I/J/K on loops 1/2/3, batched over loop(s) 0 (2 batch iteration(s))
func.func @b_offset(%A: memref<2x4x4xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<2x4x4xf32> {llvm.noalias}) {
  affine.for %b = 0 to 2 {
    affine.for %i = 0 to 4 {
      affine.for %j = 0 to 4 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%b, %i, %k] : memref<2x4x4xf32>
          %bb = affine.load %B[%k, %j + %b] : memref<4x5xf32>
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
// Each operand reads the batch coordinate through its own expression, in a
// dimension of its own: A[b + 1], B[b], C[b] (refused before ISSUE-007 as
// "one per-operand view of a batch coordinate is not lowered").
// CHECK-LABEL: func.func @per_operand_batch_index
// CHECK:       affine.for %{{.*}} = 0 to 2 {
// CHECK:         systolic.matmul_tile {{.*}} m = 4 n = 4 k = 4
// REMARK:      remark: affine GEMM 4x4x4 lowered as 1 exact region(s), batched: 2 batch iteration(s) of 1 batch loop(s)
// DISC:        recognition result: success
// DISC:        batch terms A: dimension 0 = 1 + 1*x0
// DISC:        batch terms B: dimension 0 = 0 + 1*x0
// DISC:        batch terms C: dimension 0 = 0 + 1*x0
func.func @per_operand_batch_index(%A: memref<3x4x4xf32> {llvm.noalias}, %B: memref<2x4x4xf32> {llvm.noalias}, %C: memref<2x4x4xf32> {llvm.noalias}) {
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
// Five loops, two batch coordinates p and q, offsets in both factors:
// C[p][q][i][j] += A[p][i + q][k] * B[k + p][j + q]. Per (p, q):
// A'(I, K) = A(p, I + q, K), B'(K, J) = B(K + p, J + q), C'(I, J) = C(p, q, I, J).
// CHECK-LABEL: func.func @two_batch_offsets
// CHECK:       affine.for %{{.*}} = 0 to 2 {
// CHECK:         affine.for %{{.*}} = 0 to 3 {
// CHECK:           systolic.matmul_tile {{.*}} m = 4 n = 4 k = 4
// REMARK:      remark: affine GEMM 4x4x4 lowered as 1 exact region(s), batched: 6 batch iteration(s) of 2 batch loop(s)
// DISC:        recognition result: success
// DISC:        batch terms A: dimension 0 = 0 + 1*x0, dimension 1 += 1*x1
// DISC:        batch terms B: dimension 0 += 1*x0, dimension 1 += 1*x1
// DISC:        batch terms C: dimension 0 = 0 + 1*x0, dimension 1 = 0 + 1*x1
// DISC:        dense logical GEMM: yes, I/J/K on loops 2/3/4, batched over loop(s) 0+1 (6 batch iteration(s))
func.func @two_batch_offsets(%A: memref<2x6x4xf32> {llvm.noalias}, %B: memref<5x6xf32> {llvm.noalias}, %C: memref<2x3x4x4xf32> {llvm.noalias}) {
  affine.for %p = 0 to 2 {
    affine.for %q = 0 to 3 {
      affine.for %i = 0 to 4 {
        affine.for %j = 0 to 4 {
          affine.for %k = 0 to 4 {
            %a = affine.load %A[%p, %i + %q, %k] : memref<2x6x4xf32>
            %bb = affine.load %B[%k + %p, %j + %q] : memref<5x6xf32>
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
// C may read a batch coordinate in several dimensions of its own: C[b][b][i][j]
// (each one a batch dimension; the slice is a function of b alone), the
// operands of different ranks (the analysis by groups, not the positional
// form, which accepts a repeated prefix loop too).
// CHECK-LABEL: func.func @c_batch_two_dims
// CHECK:       affine.for %{{.*}} = 0 to 2 {
// CHECK:         systolic.matmul_tile {{.*}} m = 4 n = 4 k = 4
// REMARK:      remark: affine GEMM 4x4x4 lowered as 1 exact region(s), batched: 2 batch iteration(s) of 1 batch loop(s)
// DISC:        batch terms C: dimension 0 = 0 + 1*x0, dimension 1 = 0 + 1*x0
func.func @c_batch_two_dims(%A: memref<2x4x4xf32> {llvm.noalias}, %B: memref<2x4x4xf32> {llvm.noalias}, %C: memref<2x2x4x4xf32> {llvm.noalias}) {
  affine.for %b = 0 to 2 {
    affine.for %i = 0 to 4 {
      affine.for %j = 0 to 4 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%b, %i, %k] : memref<2x4x4xf32>
          %bb = affine.load %B[%b, %k, %j] : memref<2x4x4xf32>
          %c = affine.load %C[%b, %b, %i, %j] : memref<2x2x4x4xf32>
          %m = arith.mulf %a, %bb : f32
          %s = arith.addf %c, %m : f32
          affine.store %s, %C[%b, %b, %i, %j] : memref<2x2x4x4xf32>
        }
      }
    }
  }
  return
}

// -----
// A prefix loop the positional form also reads as K: C[b][i][j] +=
// A[b][i][b] * B[b][b][j]. Positionally (P = b, I = i, J = j, K = b) loop b
// would drive the batch and K, which the dense partition refuses; by readers b
// is a batch loop (all three operands read it), A's dimension 2 and B's
// dimension 1 are batch dimensions, and no loop drives K: per b the rank-1
// update C[b] += A[b][:, b] B[b][b, :] (K = 1). The analysis by groups decides
// such a reading (the positional one is kept only when that analysis refuses
// too, as for C[b][b + i][j] in affine_gemm_partition_coupled.mlir).
// CHECK-LABEL: func.func @prefix_reused_as_k
// CHECK:       affine.for %{{.*}} = 0 to 2 {
// CHECK:         systolic.matmul_tile {{.*}} m = 4 n = 4 k = 1
// REMARK:      remark: affine GEMM 4x4x1 lowered as 1 exact region(s), batched: 2 batch iteration(s) of 1 batch loop(s)
// DISC:        batch terms A: dimension 0 = 0 + 1*x0, dimension 2 = 0 + 1*x0
func.func @prefix_reused_as_k(%A: memref<2x4x2xf32> {llvm.noalias}, %B: memref<2x2x4xf32> {llvm.noalias}, %C: memref<2x4x4xf32> {llvm.noalias}) {
  affine.for %b = 0 to 2 {
    affine.for %i = 0 to 4 {
      affine.for %j = 0 to 4 {
        %a = affine.load %A[%b, %i, %b] : memref<2x4x2xf32>
        %bb = affine.load %B[%b, %b, %j] : memref<2x2x4xf32>
        %c = affine.load %C[%b, %i, %j] : memref<2x4x4xf32>
        %m = arith.mulf %a, %bb : f32
        %s = arith.addf %c, %m : f32
        affine.store %s, %C[%b, %i, %j] : memref<2x4x4xf32>
      }
    }
  }
  return
}

// -----
// RE-DECIDED 2026-10-16 (docs/AFFINE_GEMM_MAPPING.md section 19.3, no C purity):
// C[i + j][j] += A[i + j][k] * B[k][j] (showcase case_087). j is read by all three
// operands -- a batch loop -- and C may read it in a role dimension: per j the GEMV
// C'(I) = C[I + j][j], A'(I, K) = A[I + j][K], B'(K) = B[K][j], C' injective and the
// batches writing different columns. Lowered, one 4 x 1 x 4 tile per j (it was
// refused: "loop 1 drives I and J" in the positional reading).
// CHECK-LABEL: func.func @c_mixes_batch
// CHECK:       affine.for %[[J:.*]] = 0 to 4 {
// CHECK:         %[[O:.*]] = affine.apply #{{.*}}(%[[J]])
// CHECK:         memref.subview %arg0[%[[O]], 0] [4, 4] [1, 1]
// CHECK:         memref.subview %arg1[0, %[[O]]] [4, 1] [1, 1]
// CHECK:         memref.subview %arg2[%[[O]], %[[O]]] [4, 1] [1, 1]
// CHECK:         systolic.matmul_tile {{.*}} m = 4 n = 1 k = 4
// REMARK:      remark: affine GEMM 4x1x4 lowered as 1 exact region(s), batched: 4 batch iteration(s) of 1 batch loop(s)
// DISC:        batch terms C: dimension 0 += 1*x1, dimension 1 = 0 + 1*x1
func.func @c_mixes_batch(%A: memref<8x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<8x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i + %j, %k] : memref<8x4xf32>
        %bb = affine.load %B[%k, %j] : memref<4x4xf32>
        %c = affine.load %C[%i + %j, %j] : memref<8x4xf32>
        %m = arith.mulf %a, %bb : f32
        %s = arith.addf %c, %m : f32
        affine.store %s, %C[%i + %j, %j] : memref<8x4xf32>
      }
    }
  }
  return
}

// -----
// RE-DECIDED 2026-10-16 (section 19.3): C[i + k][j] += A[i][k] * B[k][j] (showcase
// case_127). k is read by all three operands -- a batch loop; per k the rank-1 update
// C'(I, J) = C[I + k][J] += A[I][k] * B[k][J] (K = 1), injective per batch; the windows
// of different batches overlap and add up in order. Lowered, one 4 x 4 x 1 tile per k.
// CHECK-LABEL: func.func @c_skewed
// CHECK:       affine.for %[[K:.*]] = 0 to 4 {
// CHECK:         %[[O:.*]] = affine.apply #{{.*}}(%[[K]])
// CHECK:         memref.subview %arg0[0, %[[O]]] [4, 1] [1, 1]
// CHECK:         memref.subview %arg1[%[[O]], 0] [1, 4] [1, 1]
// CHECK:         memref.subview %arg2[%[[O]], 0] [4, 4] [1, 1]
// CHECK:         systolic.matmul_tile {{.*}} m = 4 n = 4 k = 1
// REMARK:      remark: affine GEMM 4x4x1 lowered as 1 exact region(s), batched: 4 batch iteration(s) of 1 batch loop(s)
// DISC:        batch terms C: dimension 0 += 1*x2
func.func @c_skewed(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<8x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<4x4xf32>
        %bb = affine.load %B[%k, %j] : memref<4x4xf32>
        %c = affine.load %C[%i + %k, %j] : memref<8x4xf32>
        %m = arith.mulf %a, %bb : f32
        %s = arith.addf %c, %m : f32
        affine.store %s, %C[%i + %k, %j] : memref<8x4xf32>
      }
    }
  }
  return
}

// -----
// RE-DECIDED 2026-10-16 (section 19): A[i][i + k] -- A's dimension 1 combines I and K,
// the skewed view A'(I, K) = A[I][I + K] of a GEMM (showcase case_126). Recognized by the
// readers analysis and verified; its view is a memref.reinterpret_cast, not a subview.
// CHECK-LABEL: func.func @coupled_a
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      remark: affine GEMM not lowered to systolic.matmul_tile: A has a skewed or merged view
// DISC:        layout A: general view: dimension 0 = 0 + 1*I; dimension 1 = 0 + 1*I + 1*K
func.func @coupled_a(%A: memref<4x8xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %i + %k] : memref<4x8xf32>
        %bb = affine.load %B[%k, %j] : memref<4x4xf32>
        %c = affine.load %C[%i, %j] : memref<4x4xf32>
        %m = arith.mulf %a, %bb : f32
        %s = arith.addf %c, %m : f32
        affine.store %s, %C[%i, %j] : memref<4x4xf32>
      }
    }
  }
  return
}

// -----
// MUST STAY REJECTED. B[k][i] with C[i][j]: i is read by all three operands
// (a batch loop), so nothing but C reads j -- a broadcast along j.
// CHECK-LABEL: func.func @wrong_coordinate_b
// CHECK-NOT:   systolic.matmul_tile
// DISC:        reason: a coordinate is read by C only
func.func @wrong_coordinate_b(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<4x4xf32>
        %bb = affine.load %B[%k, %i] : memref<4x4xf32>
        %c = affine.load %C[%i, %j] : memref<4x4xf32>
        %m = arith.mulf %a, %bb : f32
        %s = arith.addf %c, %m : f32
        affine.store %s, %C[%i, %j] : memref<4x4xf32>
      }
    }
  }
  return
}

// -----
// MUST STAY REJECTED (counterexample to "a reduction direction exists"):
// C[i][j] += A[i][k0] * B[k1][j]. Along k0 + k1, C is constant and both Ka and
// Kb change, yet the nest is (sum_u A[i][u]) * (sum_v B[v][j]), not A * B:
// each reduction loop is read by one factor only.
// CHECK-LABEL: func.func @outer_of_sums
// CHECK-NOT:   systolic.matmul_tile
// DISC:        reason: a coordinate is read by the first factor only
func.func @outer_of_sums(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k0 = 0 to 4 {
        affine.for %k1 = 0 to 4 {
          %a = affine.load %A[%i, %k0] : memref<4x4xf32>
          %bb = affine.load %B[%k1, %j] : memref<4x4xf32>
          %c = affine.load %C[%i, %j] : memref<4x4xf32>
          %m = arith.mulf %a, %bb : f32
          %s = arith.addf %c, %m : f32
          affine.store %s, %C[%i, %j] : memref<4x4xf32>
        }
      }
    }
  }
  return
}

// -----
// MUST STAY REJECTED (counterexample to "Ka and Kb change together"):
// A[i][k0 + k1] * B[k0 + k1][j] visits K = 1, 2, 3 more than once (a weighted
// sum): the reduction is not a bijection onto [0, K) (Theorem 3 (S2)).
// CHECK-LABEL: func.func @noninjective_reduction
// CHECK-NOT:   systolic.matmul_tile
// DISC:        recognition result: success
// DISC:        dense logical GEMM: no (
func.func @noninjective_reduction(%A: memref<4x8xf32> {llvm.noalias}, %B: memref<8x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k0 = 0 to 4 {
        affine.for %k1 = 0 to 4 {
          %a = affine.load %A[%i, %k0 + %k1] : memref<4x8xf32>
          %bb = affine.load %B[%k0 + %k1, %j] : memref<8x4xf32>
          %c = affine.load %C[%i, %j] : memref<4x4xf32>
          %m = arith.mulf %a, %bb : f32
          %s = arith.addf %c, %m : f32
          affine.store %s, %C[%i, %j] : memref<4x4xf32>
        }
      }
    }
  }
  return
}

// -----
// MUST STAY REJECTED: every coordinate a batch one (an element-wise product).
// CHECK-LABEL: func.func @elementwise
// CHECK-NOT:   systolic.matmul_tile
// DISC:        reason: 1 coordinate(s) are read by all three operands and none by two
func.func @elementwise(%A: memref<8xf32> {llvm.noalias}, %B: memref<8xf32> {llvm.noalias}, %C: memref<8xf32> {llvm.noalias}) {
  affine.for %b = 0 to 8 {
    %a = affine.load %A[%b] : memref<8xf32>
    %bb = affine.load %B[%b] : memref<8xf32>
    %c = affine.load %C[%b] : memref<8xf32>
    %m = arith.mulf %a, %bb : f32
    %s = arith.addf %c, %m : f32
    affine.store %s, %C[%b] : memref<8xf32>
  }
  return
}

// -----
// REVIEW 1 (wrong lowering found by review): a batch loop bounded by a min
// of constants, j in [0, min(4, 3)) = [0, 3). The batch loop must be
// re-emitted with the folded bound (three iterations): with the bound map's
// first result it would run j = 3 too, outside B's and C's three columns.
// CHECK-LABEL: func.func @batch_min_bound
// CHECK:       affine.for %{{.*}} = 0 to 3 {
// CHECK:         systolic.matmul_tile {{.*}} m = 4 n = 1 k = 4
// REMARK:      remark: affine GEMM 4x1x4 lowered as 1 exact region(s), batched: 3 batch iteration(s) of 1 batch loop(s)
func.func @batch_min_bound(%A: memref<17x16xf32> {llvm.noalias}, %B: memref<16x3xf32> {llvm.noalias}, %C: memref<33x3xf32> {llvm.noalias}) {
  affine.for %i = 1 to 5 {
    affine.for %j = 0 to min affine_map<() -> (4, 3)>() {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i * 4 - %j, %k * 5] : memref<17x16xf32>
        %b = affine.load %B[%k * 5, %j] : memref<16x3xf32>
        %c = affine.load %C[%i * 8, %j] : memref<33x3xf32>
        %m = arith.mulf %a, %b : f32
        %s = arith.addf %c, %m : f32
        affine.store %s, %C[%i * 8, %j] : memref<33x3xf32>
      }
    }
  }
  return
}

// -----
// REVIEW 1: a batch loop bounded below by a max, j in [max(0, 2), 4) = [2, 4).
// CHECK-LABEL: func.func @batch_max_bound
// CHECK:       affine.for %{{.*}} = 2 to 4 {
// CHECK:         systolic.matmul_tile {{.*}} m = 4 n = 1 k = 4
// REMARK:      remark: affine GEMM 4x1x4 lowered as 1 exact region(s), batched: 2 batch iteration(s) of 1 batch loop(s)
func.func @batch_max_bound(%A: memref<17x16xf32> {llvm.noalias}, %B: memref<16x4xf32> {llvm.noalias}, %C: memref<33x4xf32> {llvm.noalias}) {
  affine.for %i = 1 to 5 {
    affine.for %j = max affine_map<() -> (0, 2)>() to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i * 4 - %j, %k * 5] : memref<17x16xf32>
        %b = affine.load %B[%k * 5, %j] : memref<16x4xf32>
        %c = affine.load %C[%i * 8, %j] : memref<33x4xf32>
        %m = arith.mulf %a, %b : f32
        %s = arith.addf %c, %m : f32
        affine.store %s, %C[%i * 8, %j] : memref<33x4xf32>
      }
    }
  }
  return
}

// -----
// REVIEW 1: the positional form's batch loop bounded by a min (ISSUE-006
// shape): b in [0, min(4, 2)) on two slices.
// CHECK-LABEL: func.func @positional_batch_min_bound
// CHECK:       affine.for %{{.*}} = 0 to 2 {
// CHECK:         systolic.matmul_tile {{.*}} m = 4 n = 4 k = 4
// REMARK:      remark: affine GEMM 4x4x4 lowered as 1 exact region(s), batched: 2 batch iteration(s) of 1 batch loop(s)
func.func @positional_batch_min_bound(%A: memref<2x4x4xf32> {llvm.noalias}, %B: memref<2x4x4xf32> {llvm.noalias}, %C: memref<2x4x4xf32> {llvm.noalias}) {
  affine.for %b = 0 to min affine_map<() -> (4, 2)>() {
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
// REVIEW 2 (refusal of a valid mapping found by review): the positional form
// reads K = k + j with J = j -- loop j would drive J and K. j is read by all
// three operands: a batch loop; per j, C[:, j] += A[:, j .. j + 3] B[j .. j + 3, j],
// a 4 x 1 x 4 product. The analysis by groups decides.
// CHECK-LABEL: func.func @j_in_k
// CHECK:       affine.for %{{.*}} = 0 to 4 {
// CHECK:         systolic.matmul_tile {{.*}} m = 4 n = 1 k = 4
// REMARK:      remark: affine GEMM 4x1x4 lowered as 1 exact region(s), batched: 4 batch iteration(s) of 1 batch loop(s)
func.func @j_in_k(%A: memref<4x8xf32> {llvm.noalias}, %B: memref<8x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k + %j] : memref<4x8xf32>
        %bb = affine.load %B[%k + %j, %j] : memref<8x4xf32>
        %c = affine.load %C[%i, %j] : memref<4x4xf32>
        %m = arith.mulf %a, %bb : f32
        %s = arith.addf %c, %m : f32
        affine.store %s, %C[%i, %j] : memref<4x4xf32>
      }
    }
  }
  return
}

// -----
// REVIEW 2: K = k + i with I = i: loop i is read by all three operands (a
// batch loop); per i a 1 x 4 x 4 product.
// CHECK-LABEL: func.func @i_in_k
// CHECK:       affine.for %{{.*}} = 0 to 4 {
// CHECK:         systolic.matmul_tile {{.*}} m = 1 n = 4 k = 4
// REMARK:      remark: affine GEMM 1x4x4 lowered as 1 exact region(s), batched: 4 batch iteration(s) of 1 batch loop(s)
func.func @i_in_k(%A: memref<4x8xf32> {llvm.noalias}, %B: memref<8x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k + %i] : memref<4x8xf32>
        %bb = affine.load %B[%k + %i, %j] : memref<8x4xf32>
        %c = affine.load %C[%i, %j] : memref<4x4xf32>
        %m = arith.mulf %a, %bb : f32
        %s = arith.addf %c, %m : f32
        affine.store %s, %C[%i, %j] : memref<4x4xf32>
      }
    }
  }
  return
}

// -----
// REVIEW 3 (refusal of a valid mapping, since ISSUE-006): a positional prefix
// constant on the domain (t runs once) is no batch coordinate: the nest is
// the plain 5 x 1 x 1 GEMM C[1][:, 0] += A[1][:, 1] B[1][1][0] (SQ-4), not a
// batched one, so SQ-5 does not apply.
// CHECK-LABEL: func.func @constant_prefix
// CHECK:       systolic.matmul_tile {{.*}} m = 5 n = 1 k = 1
// REMARK:      remark: affine GEMM 5x1x1 lowered as 1 exact region(s)
func.func @constant_prefix(%A: memref<2x5x3xf32> {llvm.noalias}, %B: memref<2x3x2xf32> {llvm.noalias}, %C: memref<2x5x2xf32> {llvm.noalias}) {
  affine.for %t = 1 to 2 {
    affine.for %i = 0 to 5 {
      %a = affine.load %A[%t, %i, 1] : memref<2x5x3xf32>
      %bb = affine.load %B[%t, 1, 0] : memref<2x3x2xf32>
      %c = affine.load %C[%t, %i, 0] : memref<2x5x2xf32>
      %m = arith.mulf %a, %bb : f32
      %s = arith.addf %c, %m : f32
      affine.store %s, %C[%t, %i, 0] : memref<2x5x2xf32>
    }
  }
  return
}

// -----
// REVIEW 1 with P0.6: a stepped batch loop with folded bounds,
// b = max(-1, 1) to min(9, 8) step 3 = 1, 4, 7, re-emitted as 1 to 8 step 3;
// K = k1 + k2 with k1 = max(0, -3) to 8 step 4 -- refused before the source
// domain took the stride from the folded lower bound.
// CHECK-LABEL: func.func @batch_stepped_split
// CHECK:       affine.for %{{.*}} = 1 to 8 step 3 {
// CHECK:         systolic.matmul_tile {{.*}} m = 4 n = 4 k = 8
// REMARK:      remark: affine GEMM 4x4x8 lowered as 1 exact region(s), batched: 3 batch iteration(s) of 1 batch loop(s)
func.func @batch_stepped_split(%A: memref<8x4x8xf32> {llvm.noalias}, %B: memref<8x8x4xf32> {llvm.noalias}, %C: memref<8x4x4xf32> {llvm.noalias}) {
  affine.for %b = max affine_map<() -> (-1, 1)>() to min affine_map<() -> (9, 8)>() step 3 {
    affine.for %i = 0 to 4 {
      affine.for %j = 0 to 4 {
        affine.for %k1 = max affine_map<() -> (0, -3)>() to 8 step 4 {
          affine.for %k2 = 0 to 4 {
            %a = affine.load %A[%b, %i, %k1 + %k2] : memref<8x4x8xf32>
            %bb = affine.load %B[%b, %k1 + %k2, %j] : memref<8x8x4xf32>
            %c = affine.load %C[%b, %i, %j] : memref<8x4x4xf32>
            %m = arith.mulf %a, %bb : f32
            %s = arith.addf %c, %m : f32
            affine.store %s, %C[%b, %i, %j] : memref<8x4x4xf32>
          }
        }
      }
    }
  }
  return
}
