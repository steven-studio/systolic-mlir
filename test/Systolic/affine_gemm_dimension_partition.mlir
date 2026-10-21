// Dimension-intersection partitioning (docs/AFFINE_GEMM_MAPPING.md section 21,
// 2026-10-19). The loops each operand reads classify the iteration dimensions:
// read by all three -- a batch loop (ISSUE-006/007, unchanged); by A and C --
// the row role I, or, when those loops are not one dense coordinate, a
// partition of the GEMM with B shared; by B and C -- J or a partition with A
// shared; by C only -- a partition with both factors shared; by A and B -- the
// reduction K, never a partition (a second reduction coordinate is a
// partial-C reduction, refused); by one factor only -- a broadcast, refused.
// A demoted (partitioning) loop must be read by C alone in a dimension of its
// own: the partitions write disjoint slabs of C.
// RE-DECIDED 2026-10-21 (docs section 23, reduction-batch partitioning on by
// default): a loop not read by C -- a second coordinate read by both factors,
// or a coordinate read by one factor only -- is a reduction batch (RB) of an
// accumulation C += A * B: one GEMM per value with offset factor views, all
// accumulated into the same slab of C in the original order (cases 8 and 10).
//
// RUN: systolic-opt "--affine-matmul-to-systolic=allow-fp-reassociation=true" \
// RUN:   -split-input-file %s | FileCheck %s
// RUN: systolic-opt "--affine-matmul-to-systolic=report=true allow-fp-reassociation=true classify=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=REMARK
// RUN: systolic-opt --test-affine-matmul-discovery -split-input-file %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s --check-prefix=DISC

// 1. Triple intersection: C[b][i][j] += A[b][i][k] * B[b][k][j]. b is read by
//    all three operands: a batch loop in every decomposition (unchanged).
// CHECK-LABEL: func.func @triple
// CHECK:       affine.for %[[B:.*]] = 0 to 2 {
// CHECK:         memref.subview %arg0[%{{.*}}, 0, 0] [1, 3, 4] [1, 1, 1]
// CHECK:         memref.subview %arg1[%{{.*}}, 0, 0] [1, 4, 5] [1, 1, 1]
// CHECK:         memref.subview %arg2[%{{.*}}, 0, 0] [1, 3, 5] [1, 1, 1]
// CHECK:         systolic.matmul_tile {{.*}} m = 3 n = 5 k = 4
// REMARK:      remark: affine GEMM 3x5x4 lowered as 1 exact region(s), batched: 2 batch iteration(s) of 1 batch loop(s); the enclosing loops run it 2 time(s)
// DISC:        recognition result: success
// DISC-NOT:    partition: loop(s)
// DISC:        dense logical GEMM: yes, I/J/K on loops 1/2/3, batched over loop(s) 0 (2 batch iteration(s))
func.func @triple(%A: memref<2x3x4xf32> {llvm.noalias}, %B: memref<2x4x5xf32> {llvm.noalias}, %C: memref<2x3x5xf32> {llvm.noalias}) {
  affine.for %b = 0 to 2 {
    affine.for %i = 0 to 3 {
      affine.for %j = 0 to 5 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%b, %i, %k] : memref<2x3x4xf32>
          %bb = affine.load %B[%b, %k, %j] : memref<2x4x5xf32>
          %c = affine.load %C[%b, %i, %j] : memref<2x3x5xf32>
          %p = arith.mulf %a, %bb : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%b, %i, %j] : memref<2x3x5xf32>
        }
      }
    }
  }
  return
}

// -----

// 2. C and A share an extra dimension: C[p][i][j] += A[p][i][k] * B[k][j].
//    p and i are both read by A and C in separate dimensions: two coordinates
//    in the row role. i (extent 3) stays I, p (extent 2) partitions the GEMM;
//    B, which does not read p, is the whole memref shared by both partitions.
// CHECK-LABEL: func.func @ca_extra
// CHECK:       affine.for %[[P:.*]] = 0 to 2 {
// CHECK:         memref.subview %arg0[%{{.*}}, 0, 0] [1, 3, 4] [1, 1, 1]
// CHECK:         bufferization.to_tensor %arg1 restrict : memref<4x5xf32>
// CHECK:         memref.subview %arg2[%{{.*}}, 0, 0] [1, 3, 5] [1, 1, 1]
// CHECK:         systolic.matmul_tile {{.*}} m = 3 n = 5 k = 4
// REMARK:      remark: affine GEMM 3x5x4 lowered as 1 exact region(s), partitioned on 1 demoted loop(s), batched: 2 batch iteration(s) of 1 batch loop(s); the enclosing loops run it 2 time(s)
// DISC:        recognition result: success
// DISC:        partition: loop(s) 0 demoted to batch loops
// DISC:        dense logical GEMM: yes, I/J/K on loops 1/2/3, batched over loop(s) 0 (2 batch iteration(s))
func.func @ca_extra(%A: memref<2x3x4xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<2x3x5xf32> {llvm.noalias}) {
  affine.for %p = 0 to 2 {
    affine.for %i = 0 to 3 {
      affine.for %j = 0 to 5 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%p, %i, %k] : memref<2x3x4xf32>
          %bb = affine.load %B[%k, %j] : memref<4x5xf32>
          %c = affine.load %C[%p, %i, %j] : memref<2x3x5xf32>
          %p0 = arith.mulf %a, %bb : f32
          %s = arith.addf %c, %p0 : f32
          affine.store %s, %C[%p, %i, %j] : memref<2x3x5xf32>
        }
      }
    }
  }
  return
}

// -----

// 3. C and B share an extra dimension: C[q][i][j] += A[i][k] * B[q][k][j].
//    q partitions, A is shared.
// CHECK-LABEL: func.func @cb_extra
// CHECK:       affine.for %{{.*}} = 0 to 2 {
// CHECK:         bufferization.to_tensor %arg0 restrict : memref<3x4xf32>
// CHECK:         memref.subview %arg1[%{{.*}}, 0, 0] [1, 4, 5] [1, 1, 1]
// CHECK:         systolic.matmul_tile {{.*}} m = 3 n = 5 k = 4
// REMARK:      remark: affine GEMM 3x5x4 lowered as 1 exact region(s), partitioned on 1 demoted loop(s), batched: 2 batch iteration(s) of 1 batch loop(s)
// DISC:        partition: loop(s) 0 demoted to batch loops
func.func @cb_extra(%A: memref<3x4xf32> {llvm.noalias}, %B: memref<2x4x5xf32> {llvm.noalias}, %C: memref<2x3x5xf32> {llvm.noalias}) {
  affine.for %q = 0 to 2 {
    affine.for %i = 0 to 3 {
      affine.for %j = 0 to 5 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%i, %k] : memref<3x4xf32>
          %bb = affine.load %B[%q, %k, %j] : memref<2x4x5xf32>
          %c = affine.load %C[%q, %i, %j] : memref<2x3x5xf32>
          %p0 = arith.mulf %a, %bb : f32
          %s = arith.addf %c, %p0 : f32
          affine.store %s, %C[%q, %i, %j] : memref<2x3x5xf32>
        }
      }
    }
  }
  return
}

// -----

// 4. Two extra C-and-A dimensions and the choice rule: C[p0][p1][i][j] +=
//    A[p0][p1][i][k] * B[k][j] with extents 2, 3, 3. The coordinate of the
//    largest extent stays the role; p1 and i tie (3), the one read in C's
//    innermost dimension (i) wins; p0 and p1 partition.
// CHECK-LABEL: func.func @ca_extra_two
// CHECK:       affine.for %{{.*}} = 0 to 2 {
// CHECK:         affine.for %{{.*}} = 0 to 3 {
// CHECK:           systolic.matmul_tile {{.*}} m = 3 n = 5 k = 4
// REMARK:      remark: affine GEMM 3x5x4 lowered as 1 exact region(s), partitioned on 2 demoted loop(s), batched: 6 batch iteration(s) of 2 batch loop(s)
// DISC:        partition: loop(s) 0+1 demoted to batch loops
// DISC:        dense logical GEMM: yes, I/J/K on loops 2/3/4, batched over loop(s) 0+1 (6 batch iteration(s))
func.func @ca_extra_two(%A: memref<2x3x3x4xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<2x3x3x5xf32> {llvm.noalias}) {
  affine.for %p0 = 0 to 2 {
    affine.for %p1 = 0 to 3 {
      affine.for %i = 0 to 3 {
        affine.for %j = 0 to 5 {
          affine.for %k = 0 to 4 {
            %a = affine.load %A[%p0, %p1, %i, %k] : memref<2x3x3x4xf32>
            %bb = affine.load %B[%k, %j] : memref<4x5xf32>
            %c = affine.load %C[%p0, %p1, %i, %j] : memref<2x3x3x5xf32>
            %p = arith.mulf %a, %bb : f32
            %s = arith.addf %c, %p : f32
            affine.store %s, %C[%p0, %p1, %i, %j] : memref<2x3x3x5xf32>
          }
        }
      }
    }
  }
  return
}

// -----

// 5. The choice rule, the other way: C[i][p][j] += A[i][p][k] * B[k][j] with i
//    of extent 2 and p of extent 3: p (the larger) stays I, i partitions --
//    not the loop order, the extents decide.
// CHECK-LABEL: func.func @choice_by_extent
// CHECK:       affine.for %{{.*}} = 0 to 2 {
// CHECK:         systolic.matmul_tile {{.*}} m = 3 n = 5 k = 4
// REMARK:      remark: affine GEMM 3x5x4 lowered as 1 exact region(s), partitioned on 1 demoted loop(s), batched: 2 batch iteration(s) of 1 batch loop(s)
// DISC:        partition: loop(s) 0 demoted to batch loops
// DISC:        dense logical GEMM: yes, I/J/K on loops 1/2/3, batched over loop(s) 0 (2 batch iteration(s))
func.func @choice_by_extent(%A: memref<2x3x4xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<2x3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 2 {
    affine.for %p = 0 to 3 {
      affine.for %j = 0 to 5 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%i, %p, %k] : memref<2x3x4xf32>
          %bb = affine.load %B[%k, %j] : memref<4x5xf32>
          %c = affine.load %C[%i, %p, %j] : memref<2x3x5xf32>
          %p0 = arith.mulf %a, %bb : f32
          %s = arith.addf %c, %p0 : f32
          affine.store %s, %C[%i, %p, %j] : memref<2x3x5xf32>
        }
      }
    }
  }
  return
}

// -----

// 6. C only: C[b][i][j] += A[i][k] * B[k][j]. b is read by C alone: a
//    partition with both factors shared (every slab gets the same A B).
// CHECK-LABEL: func.func @c_only
// CHECK:       affine.for %{{.*}} = 0 to 2 {
// CHECK:         bufferization.to_tensor %arg0 restrict : memref<3x4xf32>
// CHECK:         bufferization.to_tensor %arg1 restrict : memref<4x5xf32>
// CHECK:         memref.subview %arg2[%{{.*}}, 0, 0] [1, 3, 5] [1, 1, 1]
// CHECK:         systolic.matmul_tile {{.*}} m = 3 n = 5 k = 4
// REMARK:      remark: affine GEMM 3x5x4 lowered as 1 exact region(s), partitioned on 1 demoted loop(s), batched: 2 batch iteration(s) of 1 batch loop(s)
// DISC:        partition: loop(s) 0 demoted to batch loops
func.func @c_only(%A: memref<3x4xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<2x3x5xf32> {llvm.noalias}) {
  affine.for %b = 0 to 2 {
    affine.for %i = 0 to 3 {
      affine.for %j = 0 to 5 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%i, %k] : memref<3x4xf32>
          %bb = affine.load %B[%k, %j] : memref<4x5xf32>
          %c = affine.load %C[%b, %i, %j] : memref<2x3x5xf32>
          %p = arith.mulf %a, %bb : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%b, %i, %j] : memref<2x3x5xf32>
        }
      }
    }
  }
  return
}

// -----

// 7. A merged factor view across the partition: C[p][i][j] += A[i + 3p][k] *
//    B[k][j]. A reads p and i in one dimension (one merged coordinate for A),
//    C in two: only the demotion of p is valid -- A's rows are offset by 3p
//    per partition (a batch term on A's row dimension, ISSUE-007).
// CHECK-LABEL: func.func @merged_factor
// CHECK:       affine.for %{{.*}} = 0 to 2 {
// CHECK:         memref.subview %arg0[%{{.*}}, 0] [3, 4] [1, 1]
// CHECK:         systolic.matmul_tile {{.*}} m = 3 n = 5 k = 4
// REMARK:      remark: affine GEMM 3x5x4 lowered as 1 exact region(s), partitioned on 1 demoted loop(s), batched: 2 batch iteration(s) of 1 batch loop(s)
// DISC:        partition: loop(s) 0 demoted to batch loops
func.func @merged_factor(%A: memref<6x4xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<2x3x5xf32> {llvm.noalias}) {
  affine.for %p = 0 to 2 {
    affine.for %i = 0 to 3 {
      affine.for %j = 0 to 5 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%i + %p * 3, %k] : memref<6x4xf32>
          %bb = affine.load %B[%k, %j] : memref<4x5xf32>
          %c = affine.load %C[%p, %i, %j] : memref<2x3x5xf32>
          %p0 = arith.mulf %a, %bb : f32
          %s = arith.addf %c, %p0 : f32
          affine.store %s, %C[%p, %i, %j] : memref<2x3x5xf32>
        }
      }
    }
  }
  return
}

// -----

// 8. A second reduction coordinate, C[i][j] += A[r][i][k] * B[r][k][j].
//    RE-DECIDED 2026-10-21 (docs section 23, reduction-batch partitioning on by
//    default): r is read by A and B, not by C. Section 21 refused it (a
//    partial-C reduction); it is now a reduction batch: r (loop 0) is RB, k
//    (loop 3) is K, i/j are I/J. Each r runs the GEMM A[r] * B[r] (both factor
//    views offset by r) and accumulates into the whole of C (no subview of C)
//    in the original order r, then k: every product once, the domain and every
//    access preserved (ground truth: partial.c). Not a partition.
// CHECK-LABEL: func.func @partial_reduction
// CHECK:       affine.for %{{.*}} = 0 to 2 {
// CHECK:         memref.subview %arg0[%{{.*}}, 0, 0] [1, 3, 4] [1, 1, 1]
// CHECK:         memref.subview %arg1[%{{.*}}, 0, 0] [1, 4, 5] [1, 1, 1]
// CHECK:         bufferization.to_tensor %arg2 restrict writable : memref<3x5xf32>
// CHECK:         systolic.matmul_tile {{.*}} m = 3 n = 5 k = 4
// CHECK:         bufferization.materialize_in_destination %{{.*}} in restrict writable %arg2 : (tensor<3x5xf32>, memref<3x5xf32>)
// REMARK:      remark: affine GEMM 3x5x4 lowered as 1 exact region(s), 1 reduction batch loop(s) (not read by C, accumulated into C in order), batched: 2 batch iteration(s) of 1 batch loop(s); the enclosing loops run it 2 time(s)
// DISC-NOT:    partition: loop(s)
// DISC:        layout A: roles in dimensions (1, 2)
// DISC-NEXT:   layout B: roles in dimensions (1, 2)
// DISC-NEXT:   batch terms A: dimension 0 = 0 + 1*x0
// DISC-NEXT:   batch terms B: dimension 0 = 0 + 1*x0
// DISC-NEXT:   statement: C = C + A * B (accumulation)
// DISC-NEXT:   reduction batch: loop(s) 0 (not read by C: for each value a GEMM with offset factor views, all accumulated into the same slab of C in order)
// DISC:        dense logical GEMM: yes, I/J/K on loops 1/2/3, batched over loop(s) 0 (2 batch iteration(s))
func.func @partial_reduction(%A: memref<2x3x4xf32> {llvm.noalias}, %B: memref<2x4x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %r = 0 to 2 {
    affine.for %i = 0 to 3 {
      affine.for %j = 0 to 5 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%r, %i, %k] : memref<2x3x4xf32>
          %bb = affine.load %B[%r, %k, %j] : memref<2x4x5xf32>
          %c = affine.load %C[%i, %j] : memref<3x5xf32>
          %p = arith.mulf %a, %bb : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%i, %j] : memref<3x5xf32>
        }
      }
    }
  }
  return
}

// -----

// 9. Refused: the partitions' outputs overlap, C[p + i][j] += A[p][i][k] *
//    B[k][j]. p would partition, but C reads it together with the row
//    coordinate: rows 1 and 2 belong to both partitions. A legal sequential
//    accumulation, not an independent partition: valid, not constructed.
// CHECK-LABEL: func.func @overlapping_partitions
// CHECK-NOT:   systolic.matmul_tile
// DISC:        reason: loop 0 would partition the GEMM (dimension-intersection partitioning, docs/AFFINE_GEMM_MAPPING.md section 21), but C reads it in dimension 0 together with a role coordinate: the partitions' outputs would not be independent slabs of C (overlapping, or one merged dimension) -- not constructed
// DISC-NEXT:   class: VALID_BUT_LOWERING_UNSUPPORTED
func.func @overlapping_partitions(%A: memref<2x3x4xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<5x5xf32> {llvm.noalias}) {
  affine.for %p = 0 to 2 {
    affine.for %i = 0 to 3 {
      affine.for %j = 0 to 5 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%p, %i, %k] : memref<2x3x4xf32>
          %bb = affine.load %B[%k, %j] : memref<4x5xf32>
          %c = affine.load %C[%p + %i, %j] : memref<5x5xf32>
          %p0 = arith.mulf %a, %bb : f32
          %s = arith.addf %c, %p0 : f32
          affine.store %s, %C[%p + %i, %j] : memref<5x5xf32>
        }
      }
    }
  }
  return
}

// -----

// 10. A loop read by one factor only, C[i][j] += A[i][s][k] * B[k][j].
//     RE-DECIDED 2026-10-21 (docs section 23, reduction-batch partitioning on
//     by default): section 21 refused it as a broadcast (section 10); s is not
//     read by C, so it is a reduction batch: s (loop 2) is RB, k (loop 3) is
//     K, i/j (loops 0/1) are I/J. Each s runs the GEMM A[:, s, :] * B with B
//     the whole memref (no batch term on B: shared, every product
//     A[i][s][k] * B[k][j] still formed once) and accumulates into the whole
//     of C in the original order s, then k (ground truth: an RB with one
//     factor constant along it, like a_only.c / T5.c).
// CHECK-LABEL: func.func @one_factor_only
// CHECK:       affine.for %{{.*}} = 0 to 2 {
// CHECK:         memref.subview %arg0[0, %{{.*}}, 0] [3, 1, 4] [1, 1, 1]
// CHECK:         bufferization.to_tensor %arg1 restrict : memref<4x5xf32>
// CHECK:         bufferization.to_tensor %arg2 restrict writable : memref<3x5xf32>
// CHECK:         systolic.matmul_tile {{.*}} m = 3 n = 5 k = 4
// CHECK:         bufferization.materialize_in_destination %{{.*}} in restrict writable %arg2 : (tensor<3x5xf32>, memref<3x5xf32>)
// REMARK:      remark: affine GEMM 3x5x4 lowered as 1 exact region(s), 1 reduction batch loop(s) (not read by C, accumulated into C in order), batched: 2 batch iteration(s) of 1 batch loop(s); the enclosing loops run it 2 time(s)
// DISC-NOT:    partition: loop(s)
// DISC:        layout A: roles in dimensions (0, 2)
// DISC-NEXT:   batch terms A: dimension 1 = 0 + 1*x2
// DISC-NEXT:   statement: C = C + A * B (accumulation)
// DISC-NEXT:   reduction batch: loop(s) 2 (not read by C: for each value a GEMM with offset factor views, all accumulated into the same slab of C in order)
// DISC:        dense logical GEMM: yes, I/J/K on loops 0/1/3, batched over loop(s) 2 (2 batch iteration(s))
func.func @one_factor_only(%A: memref<3x2x4xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %s = 0 to 2 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%i, %s, %k] : memref<3x2x4xf32>
          %bb = affine.load %B[%k, %j] : memref<4x5xf32>
          %c = affine.load %C[%i, %j] : memref<3x5xf32>
          %p = arith.mulf %a, %bb : f32
          %t = arith.addf %c, %p : f32
          affine.store %t, %C[%i, %j] : memref<3x5xf32>
        }
      }
    }
  }
  return
}

// -----

// 11. Refused: an assignment whose partitions would overlap, C[i + b][j] =
//     A[b][i] * B[b][j] (b read by all three: a batch loop; the assignment's
//     slabs overlap): PROVEN_INVALID by the written-once proof (section 20).
// CHECK-LABEL: func.func @assign_overwrite_batch
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      remark: affine GEMM not lowered to systolic.matmul_tile: two iterations of the assignment C = A * B write the same element of C
func.func @assign_overwrite_batch(%A: memref<2x3xf32> {llvm.noalias}, %B: memref<2x5xf32> {llvm.noalias}, %C: memref<4x5xf32> {llvm.noalias}) {
  affine.for %b = 0 to 2 {
    affine.for %i = 0 to 3 {
      affine.for %j = 0 to 5 {
        %a = affine.load %A[%b, %i] : memref<2x3xf32>
        %bb = affine.load %B[%b, %j] : memref<2x5xf32>
        %p = arith.mulf %a, %bb : f32
        affine.store %p, %C[%i + %b, %j] : memref<4x5xf32>
      }
    }
  }
  return
}

// -----

// 12. Refused: a cross-partition dependence through the output, A[p + 1][i][j]
//     += A[p][i][k] * B[k][j] -- partition p + 1 reads what partition p
//     wrote: the factor may overlap C (UNKNOWN), nothing lowered.
// CHECK-LABEL: func.func @cross_partition_alias
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      remark: affine GEMM not lowered to systolic.matmul_tile: A may overlap C; declare the arguments llvm.noalias (C restrict) or pass assume-no-alias [UNKNOWN]
func.func @cross_partition_alias(%A: memref<3x3x5xf32>, %B: memref<5x5xf32> {llvm.noalias}) {
  affine.for %p = 0 to 2 {
    affine.for %i = 0 to 3 {
      affine.for %j = 0 to 5 {
        affine.for %k = 0 to 5 {
          %a = affine.load %A[%p, %i, %k] : memref<3x3x5xf32>
          %bb = affine.load %B[%k, %j] : memref<5x5xf32>
          %c = affine.load %A[%p + 1, %i, %j] : memref<3x3x5xf32>
          %p0 = arith.mulf %a, %bb : f32
          %s = arith.addf %c, %p0 : f32
          affine.store %s, %A[%p + 1, %i, %j] : memref<3x3x5xf32>
        }
      }
    }
  }
  return
}
