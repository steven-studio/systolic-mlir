// Exact non-box iteration domains (docs/AFFINE_GEMM_MAPPING.md section 19.13, 2026-10-18).
// A band whose bounds do not shift together (a triangle that degenerates, a max / min of
// forms with different linear parts) is rewritten onto a box when its exact integer
// iteration set -- built from the bounds and compared with the Presburger library's
// integer-exact set equality -- is a constant-extent parallelotope whose shift rows are
// those of the written bounds (or zero); a stepped loop's max of different forms needs one
// of them to be the max at every outer point entering it. Repetition loops in any shape (a
// triangle) whose bounds read only other repetition loops are hoisted around the GEMM -- when
// that reorders iterations, only if the memrefs the body reads provably do not overlap the one
// it writes (past loops of one iteration it reorders nothing). A written bound whose
// products or value can leave 64 bits at an outer point entering its loop leaves the nest as
// written (the program's set is then not the mathematical one). Otherwise the nest is left as
// written: PROVEN_INVALID when the accesses tell the iterations apart (their stacked map is
// injective), the factors and C are three buffers, every bound is a linear form within 64 bits
// and the domain is not centrally symmetric (no affine image of a box); UNKNOWN otherwise.
// Every lowered GEMM's logical mapping is certified a bijection onto its box (discovery:
// "logical bijection: verified"). Executed end to end through the C front end:
// test/SystolicCC/affine_exact_domains.c.
//
// RUN: systolic-opt "--affine-matmul-to-systolic=allow-fp-reassociation=true" \
// RUN:   -split-input-file %s | FileCheck %s
// RUN: systolic-opt "--affine-matmul-to-systolic=report=true allow-fp-reassociation=true classify=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=REMARK
// RUN: systolic-opt --test-affine-matmul-discovery -split-input-file %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s --check-prefix=DISC

// B. A box in disguise: i in [2, 25) step 2, j in [i, 4) -- a triangle whose rows are empty
// except i = 2 (j's range [i, 4) is empty for i >= 4). The written bounds do not shift together;
// the exact integer set (Presburger) is {2} x {2, 3} x [0, 4), a box: rewritten onto it and
// lowered as the 1 x 2 x 4 GEMM of row 2, columns 2..3.
// CHECK-LABEL: func.func @disguise_empty_outer
// CHECK:       memref.subview %arg0[2, 0] [1, 4] [1, 1]
// CHECK:       memref.subview %arg1[0, 2] [4, 2] [1, 1]
// CHECK:       memref.subview %arg2[2, 2] [1, 2] [1, 1]
// CHECK:       systolic.matmul_tile {{.*}} m = 1 n = 2 k = 4
// REMARK:      exact iteration set: the written bounds do not shift together, their integer points are the parallelotope below (integer-set equality, docs section 19.13)
// REMARK-NEXT: loop 0: x0 = x0', box [2, 3) step 2
// REMARK-NEXT: loop 1: x1 = x1', box [2, 4) step 1
// REMARK-NEXT: loop 2: x2 = x2', box [0, 4) step 1
// REMARK:      affine GEMM 1x2x4 lowered as 1 exact region(s)
// DISC:        domain normalization: 1 nest(s)
// DISC-NEXT:   exact iteration set: the written bounds do not shift together, their integer points are the parallelotope below (integer-set equality, docs section 19.13)
// DISC:        dense extents: M 1, N 2, K 4
// DISC-NEXT:   logical bijection: verified (the iteration domain maps one to one onto the box of the GEMM)
func.func @disguise_empty_outer(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 2 to 25 step 2 {
    affine.for %j = affine_map<(d0) -> (d0)>(%i) to 4 {
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
// B. A max / min of forms with different linear parts, inactive on the domain (i - 8 < 0,
// i + 9 > 4): the exact set is the 4 x 4 x 4 box.
// CHECK-LABEL: func.func @disguise_inactive_max
// CHECK:       systolic.matmul_tile {{.*}} m = 4 n = 4 k = 4
// REMARK:      exact iteration set: the written bounds do not shift together, their integer points are the parallelotope below (integer-set equality, docs section 19.13)
// REMARK:      affine GEMM 4x4x4 lowered as 1 exact region(s)
func.func @disguise_inactive_max(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = max affine_map<(d0) -> (0, d0 - 8)>(%i) to min affine_map<(d0) -> (4, d0 + 9)>(%i) {
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
// B. A sheared box in disguise: j in [max(i, 2i - 8), i + 4) -- 2i - 8 < i on the domain, so
// the set is the parallelotope j - i in [0, 4): the exact path finds the shift row of the first
// lower bound (x1 = x1' + x0) and the box nest is the plain 4 x 4 x 4 GEMM.
// CHECK-LABEL: func.func @disguise_shifted
// CHECK:       systolic.matmul_tile {{.*}} m = 4 n = 4 k = 4
// REMARK:      exact iteration set: the written bounds do not shift together, their integer points are the parallelotope below (integer-set equality, docs section 19.13)
// REMARK-NEXT: loop 0: x0 = x0', box [0, 4) step 1
// REMARK-NEXT: loop 1: x1 = x1' + 1*x0, box [0, 4) step 1
// REMARK:      affine GEMM 4x4x4 lowered as 1 exact region(s)
func.func @disguise_shifted(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = max affine_map<(d0) -> (d0, d0 * 2 - 8)>(%i) to affine_map<(d0) -> (d0 + 4)>(%i) {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<4x4xf32>
        %b = affine.load %B[%k, %j - %i] : memref<4x4xf32>
        %c = affine.load %C[%i, %j - %i] : memref<4x4xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j - %i] : memref<4x4xf32>
      }
    }
  }
  return
}

// -----
// B, steps. A stepped loop whose lower bound is a max of forms with different linear parts:
// the congruence is relative to whichever is the max, so one of them must be the max at every
// outer point entering the loop -- 0 here (2i - 8 < 0), decided exactly. k in {0, 2, 4, 6}:
// stride-2 views of A's columns and B's rows.
// CHECK-LABEL: func.func @stepped_dominant_max
// CHECK:       memref.subview %arg0[0, 0] [4, 4] [1, 2]
// CHECK:       memref.subview %arg1[0, 0] [4, 4] [2, 1]
// CHECK:       systolic.matmul_tile {{.*}} m = 4 n = 4 k = 4
// REMARK:      loop 2: x2 = x2', box [0, 7) step 2
// REMARK:      affine GEMM 4x4x4 lowered as 1 exact region(s)
// DISC:        loop 2: x2 = x2', box [0, 7) step 2
// DISC:        T diag(step) a signed permutation: no
func.func @stepped_dominant_max(%A: memref<4x8xf32> {llvm.noalias}, %B: memref<8x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = max affine_map<(d0) -> (0, d0 * 2 - 8)>(%i) to 8 step 2 {
        %a = affine.load %A[%i, %k] : memref<4x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x4xf32>
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
// Repetition loops in a triangle (r1 in [0, 3), r0 in [r1, 2): 3 points) between GEMM loops:
// their bounds read only each other, the GEMM loops' bounds only GEMM loops, so the iteration
// set is a product and every GEMM point runs 3 times. Hoisted outward (a legal interchange: no
// dependence is carried by j), the 4 x 4 x 4 GEMM runs inside them, 3 times -- counted exactly.
// (The discovery pass does not hoist: it reports the repetition loop, UNKNOWN.)
// CHECK-LABEL: func.func @rep_triangle_between
// CHECK-NEXT:    affine.for %[[R1:[a-z0-9]+]] = 0 to 3 {
// CHECK-NEXT:      affine.for %{{.*}} = #map(%[[R1]]) to 2 {
// CHECK-NEXT:        %[[A:.*]] = bufferization.to_tensor %arg0 restrict :
// CHECK-NEXT:        %[[B:.*]] = bufferization.to_tensor %arg1 restrict :
// CHECK-NEXT:        %[[C:.*]] = bufferization.to_tensor %arg2 restrict writable :
// CHECK-NEXT:        systolic.matmul_tile %[[A]], %[[B]], %[[C]] m = 4 n = 4 k = 4
// REMARK:      affine GEMM 4x4x4 lowered as 1 exact region(s); the enclosing loops run it 3 time(s)
// DISC:        reason: loop 1 of the nest is used by no access
// DISC-NEXT:   class: UNKNOWN
func.func @rep_triangle_between(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %j = 0 to 4 {
    affine.for %r1 = 0 to 3 {
      affine.for %r0 = affine_map<(d0) -> (d0)>(%r1) to 2 {
        affine.for %k = 0 to 4 {
          affine.for %i = 0 to 4 {
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
// A repetition triangle as a prefix around a shifted GEMM band (k in [i, i + 4)): the whole
// band is neither family, the band below the prefix (whose counters nothing below reads) is
// the constant-extent family: normalized there, lowered, run 3 times.
// CHECK-LABEL: func.func @rep_triangle_prefix_shifted
// CHECK-NEXT:    affine.for %[[R1:[a-z0-9]+]] = 0 to 2 {
// CHECK-NEXT:      affine.for %{{.*}} = #map(%[[R1]]) to 2 {
// CHECK-NEXT:        %[[A:.*]] = bufferization.to_tensor %arg0 restrict :
// CHECK-NEXT:        %[[B:.*]] = bufferization.to_tensor %arg1 restrict :
// CHECK-NEXT:        %[[C:.*]] = bufferization.to_tensor %arg2 restrict writable :
// CHECK-NEXT:        systolic.matmul_tile %[[A]], %[[B]], %[[C]] m = 4 n = 4 k = 4
// REMARK:      loop 2: x2 = x2' + 1*x0, box [0, 4) step 1
// REMARK:      affine GEMM 4x4x4 lowered as 1 exact region(s); the enclosing loops run it 3 time(s)
func.func @rep_triangle_prefix_shifted(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %r1 = 0 to 2 {
    affine.for %r0 = affine_map<(d0) -> (d0)>(%r1) to 2 {
      affine.for %i = 0 to 4 {
        affine.for %j = 0 to 4 {
          affine.for %k = affine_map<(d0) -> (d0)>(%i) to affine_map<(d0) -> (d0 + 4)>(%i) {
            %a = affine.load %A[%i, %k - %i] : memref<4x4xf32>
            %b = affine.load %B[%k - %i, %j] : memref<4x4xf32>
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
// E. Duplicated logical iterations with a non-uniform count: the repetition loop r in [i, 4)
// reads the GEMM loop i, so row i accumulates (4 - i) times -- not a GEMM of A and B. Not
// hoisted (its bound reads a dimension), not normalized: the recognizer decides nothing,
// UNKNOWN; the lowering pass says nothing (it reports lowering refusals only): no remark at
// any of its four loops.
// CHECK-LABEL: func.func @rep_reads_dimension
// CHECK-NOT:   systolic.matmul_tile
// REMARK-NOT:  affine_gemm_exact_domains.mlir:[[@LINE+7]]:{{[0-9]+}}: remark:
// REMARK-NOT:  affine_gemm_exact_domains.mlir:[[@LINE+7]]:{{[0-9]+}}: remark:
// REMARK-NOT:  affine_gemm_exact_domains.mlir:[[@LINE+7]]:{{[0-9]+}}: remark:
// REMARK-NOT:  affine_gemm_exact_domains.mlir:[[@LINE+7]]:{{[0-9]+}}: remark:
// DISC:        reason: loop 1 of the nest is used by no access
// DISC-NEXT:   class: UNKNOWN
func.func @rep_reads_dimension(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %r = affine_map<(d0) -> (d0)>(%i) to 4 {
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

// -----
// D. Missing logical iterations: a genuine triangle (j in [i, 4)) -- the accesses tell the
// iterations apart (C[i][j], A[i][k], B[k][j]: rank 3), the set is not centrally symmetric, so
// no affine image of a box: PROVEN_INVALID, with a point whose mirror is missing.
// CHECK-LABEL: func.func @genuine_triangle
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      the iteration domain is not centrally symmetric -- the point ({{[-0-9, ]+}}) is in it, its mirror ({{[-0-9, ]+}}) about the midpoint of its lexicographic extremes is not --, so it is no affine image of a box; the accesses tell the iterations apart (their stacked map is injective){{.*}}[PROVEN_INVALID]
// DISC:        dense logical GEMM: no (the iteration domain is not centrally symmetric
// DISC-NEXT:   bounds:
// DISC-NEXT:   class: PROVEN_INVALID
func.func @genuine_triangle(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = affine_map<(d0) -> (d0)>(%i) to 4 {
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
// A symmetric non-parallelotope (rows of 2, 3, 3, 2 points: a hexagon, symmetric about its
// centre): nothing proves it a GEMM or not -- it is not a parallelotope with the bounds' shift
// rows, and symmetry does not exclude every parametrization: UNKNOWN, never PROVEN_INVALID.
// CHECK-LABEL: func.func @symmetric_hexagon
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      the loop bounds are not static, so the iteration domain has no static normalized extent{{.*}}[UNKNOWN]
func.func @symmetric_hexagon(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = max affine_map<(d0) -> (0, d0 - 1)>(%i) to min affine_map<(d0) -> (d0 + 2, 4)>(%i) {
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
// E. Duplicated logical iterations on a box: K read as k + l by both factors -- (k, l) = (0, 1)
// and (1, 0) read the same pair of elements, so k + l is not one dense coordinate.
// RE-DECIDED 2026-10-21 (docs section 23, reduction-batch partitioning on by default): this
// was VALID_BUT_LOWERING_UNSUPPORTED because the demotion of a loop to a batch loop was
// deferred (sections 19.2 and 21); the 2026-10-21 research policy accepts it. k (loop 2, read by
// both factors, not by C) is the reduction batch loop and l (loop 3) is K: for each k, a 4x4x4
// GEMM reads A[i][k + K] and B[k + K][j] through views offset by k and accumulates into the same
// C, the k iterations in order. Every iteration runs once (the repeated products keep their
// multiplicity). Ground truth: A[i][k0 + k1] * B[k0 + k1][j] of section 23.5
// (test/SystolicCC/affine_reduction_batches.c).
// CHECK-LABEL: func.func @duplicated_k
// CHECK-NOT:   affine.load
// CHECK:       affine.for %{{.*}} = 0 to 4 {
// CHECK-NOT:   affine.load
// CHECK:       memref.subview %arg0[0, %{{.*}}] [4, 4] [1, 1]
// CHECK:       memref.subview %arg1[%{{.*}}, 0] [4, 4] [1, 1]
// CHECK:       bufferization.to_tensor %arg2 restrict writable
// CHECK:       systolic.matmul_tile {{.*}} m = 4 n = 4 k = 4
// CHECK-NEXT:  bufferization.materialize_in_destination {{.*}} in restrict writable %arg2
// CHECK-NOT:   affine.load
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      affine_gemm_exact_domains.mlir:[[@LINE+14]]:{{[0-9]+}}: remark: affine GEMM 4x4x4 lowered as 1 exact region(s), 1 reduction batch loop(s) (not read by C, accumulated into C in order), batched: 4 batch iteration(s) of 1 batch loop(s); the enclosing loops run it 4 time(s)
// DISC:        reduction map: (d0, d1, d2, d3) -> (d3)
// DISC:        phiA: (d0, d1, d2, d3) -> (d0, d1, d3)
// DISC-NEXT:   phiB: (d0, d1, d2, d3) -> (d0, d3, d2)
// DISC-NEXT:   phiC: (d0, d1, d2, d3) -> (d0, d1, d2)
// DISC:        batch terms A: dimension 1 += 1*x2
// DISC-NEXT:   batch terms B: dimension 0 += 1*x2
// DISC-NEXT:   statement: C = C + A * B (accumulation)
// DISC-NEXT:   reduction batch: loop(s) 2 (not read by C
// DISC:        reduction extent: static 4
// DISC-NEXT:   dense logical GEMM: yes, I/J/K on loops 0/1/3, batched over loop(s) 2 (4 batch iteration(s))
// DISC-NEXT:   dense extents: M 4, N 4, K 4
// DISC-NEXT:   logical bijection: verified (the iteration domain maps one to one onto the box of the GEMM)
func.func @duplicated_k(%A: memref<4x7xf32> {llvm.noalias}, %B: memref<7x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        affine.for %l = 0 to 4 {
          %a = affine.load %A[%i, %k + %l] : memref<4x7xf32>
          %b = affine.load %B[%k + %l, %j] : memref<7x4xf32>
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
// C. det T = 2 and a valid GEMM: C's row index is 2i (the task's T = [cI; cJ; q] has det 2 and
// T diag(step) is no signed permutation -- its image is a sublattice, not a complete box of
// index values), on a domain in disguise. The determinant decides nothing: the exact bijection
// certificate on the ordinals proves the mapping, and the lowering writes C through a
// stride-2 view.
// CHECK-LABEL: func.func @det2_strided_c
// CHECK:       memref.subview %arg2[0, 0] [4, 4] [2, 1]
// CHECK:       systolic.matmul_tile {{.*}} m = 4 n = 4 k = 4
// DISC:        T = {{\[\[}}2, 0, 0], [0, 1, 0], [0, 0, 1]], d = [0, 0, 0], det 2, not unimodular, T diag(step) a signed permutation: no
// DISC:        logical bijection: verified (the iteration domain maps one to one onto the box of the GEMM)
func.func @det2_strided_c(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<8x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = max affine_map<(d0) -> (0, d0 - 5)>(%i) to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<4x4xf32>
        %b = affine.load %B[%k, %j] : memref<4x4xf32>
        %c = affine.load %C[%i * 2, %j] : memref<8x4xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i * 2, %j] : memref<8x4xf32>
      }
    }
  }
  return
}

// -----
// G. A x A through two different views (A . A^T) on a domain in disguise: two read-only
// tensors of %A -- the whole memref and its transpose.
// CHECK-LABEL: func.func @axa_two_views_disguised
// CHECK:       %[[A:.*]] = bufferization.to_tensor %arg0 restrict
// CHECK:       memref.transpose
// CHECK:       %[[AT:.*]] = bufferization.to_tensor %{{.*}} restrict
// CHECK:       systolic.matmul_tile %[[A]], %[[AT]], %{{.*}} m = 4 n = 4 k = 4
func.func @axa_two_views_disguised(%A: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = max affine_map<(d0) -> (0, d0 - 4)>(%i) to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<4x4xf32>
        %b = affine.load %A[%j, %k] : memref<4x4xf32>
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
// H. A x A through one view (A . A) on a domain in disguise: one tensor serves both factors.
// CHECK-LABEL: func.func @axa_same_view_disguised
// CHECK:       %[[A:.*]] = bufferization.to_tensor %arg0 restrict
// CHECK:       systolic.matmul_tile %[[A]], %[[A]], %{{.*}} m = 4 n = 4 k = 4
func.func @axa_same_view_disguised(%A: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = max affine_map<(d0) -> (0, d0 - 4)>(%i) to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<4x4xf32>
        %b = affine.load %A[%k, %j] : memref<4x4xf32>
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
// I. The output is a factor's memref (A[i][j] += A[i][k] * B[k][j]) on a domain in disguise:
// the domain is normalized, the lowering refuses the aliasing conservatively.
// CHECK-LABEL: func.func @alias_c_disguised
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      A may overlap C{{.*}}[UNKNOWN]
func.func @alias_c_disguised(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = max affine_map<(d0) -> (0, d0 - 4)>(%i) to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<4x4xf32>
        %b = affine.load %B[%k, %j] : memref<4x4xf32>
        %c = affine.load %A[%i, %j] : memref<4x4xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %A[%i, %j] : memref<4x4xf32>
      }
    }
  }
  return
}

// -----
// J. K = 1 in disguise: k in [3, min(4, 9 - i)) is {3} everywhere -- the 4 x 4 x 1 GEMM of
// A's column 3 and B's row 3 (a degenerate reduction is no reason to refuse).
// CHECK-LABEL: func.func @k1_disguised
// CHECK:       memref.subview %arg0[0, 3] [4, 1] [1, 1]
// CHECK:       memref.subview %arg1[3, 0] [1, 4] [1, 1]
// CHECK:       systolic.matmul_tile {{.*}} m = 4 n = 4 k = 1
// REMARK:      loop 2: x2 = x2', box [3, 4) step 1
// REMARK:      affine GEMM 4x4x1 lowered as 1 exact region(s)
// DISC:        2-D (K = 1: a degenerate reduction, the non-degenerate rule does not apply)
// DISC:        dense extents: M 4, N 4, K 1
func.func @k1_disguised(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 3 to min affine_map<(d0) -> (4, d0 * -1 + 9)>(%i) {
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
// K. K = 1 that must stay refused: k in [4, min(5, 9 - i)) is {4}, column 4 of a 4-wide A --
// out of bounds: PROVEN_INVALID.
// CHECK-LABEL: func.func @k1_oob_disguised
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      A is accessed out of bounds along dimension 1 [PROVEN_INVALID]
func.func @k1_oob_disguised(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 4 to min affine_map<(d0) -> (5, d0 * -1 + 9)>(%i) {
        %a = affine.load %A[%i, %k] : memref<4x4xf32>
        %b = affine.load %B[0, %j] : memref<4x4xf32>
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
// L. Out of bounds after the normalization: j in [max(0, i - 4), 5) is [0, 5), column 4 of B
// and C on 4-wide memrefs: PROVEN_INVALID.
// CHECK-LABEL: func.func @oob_disguised
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      B is accessed out of bounds along dimension 1 [PROVEN_INVALID]
func.func @oob_disguised(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = max affine_map<(d0) -> (0, d0 - 4)>(%i) to 5 {
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
// M. A bound whose product leaves 64 bits: max(0, i * 2^62) needs 2^63 at i = 2 (lower-affine's
// i * 2^62 is an `arith.muli ... overflow<nsw>`, poison there) -- the mathematical set is not
// the run-time one, so neither the exact normalization nor the symmetry disproof reasons on
// it: UNKNOWN. (Its mathematical set {0} x [0, 4) x [0, 4) is symmetric anyway; the
// asymmetric variant @overflow_bound_asymmetric below stays UNKNOWN only by this guard.)
// CHECK-LABEL: func.func @overflow_bound
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      the loop bounds are not static, so the iteration domain has no static normalized extent{{.*}}[UNKNOWN]
// DISC:        dense logical GEMM: no (the loop bounds are not static
// DISC-NEXT:   bounds:
// DISC-NEXT:   class: UNKNOWN
func.func @overflow_bound(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = max affine_map<(d0) -> (0, d0 * 4611686018427387904)>(%i) to 4 {
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
// M. 64 bits, a result a fold drops: k's lower bound max(i, i - (2^63 - 1)) folds to i (one
// linear part) for the stride, but the run time evaluates both results, and the second leaves
// 64 bits at i = -4 .. -2 (the wrapped value empties the row). Every written result is checked:
// the exact path refuses, the nest stays as written, UNKNOWN (review of 2026-10-18: the fold
// dropped the result before the check, and the 4 x 4 x 4 GEMM was lowered).
// CHECK-LABEL: func.func @fold_drops_overflowing_result
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      affine_gemm_exact_domains.mlir:[[@LINE+2]]:3: remark: affine GEMM not lowered to systolic.matmul_tile: the loop bounds are not static, so the iteration domain has no static normalized extent{{.*}}[UNKNOWN]
func.func @fold_drops_overflowing_result(%A: memref<4x8xf32> {llvm.noalias}, %B: memref<8x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = -4 to 0 {
    affine.for %j = max affine_map<(d0) -> (0, d0 - 10)>(%i) to 4 {
      affine.for %k = max affine_map<(d0) -> (d0, d0 - 9223372036854775807)>(%i) to affine_map<(d0) -> (d0 + 8)>(%i) step 2 {
        %a = affine.load %A[%i + 4, %k - %i] : memref<4x8xf32>
        %b = affine.load %B[%k - %i, %j] : memref<8x4xf32>
        %c = affine.load %C[%i + 4, %j] : memref<4x4xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i + 4, %j] : memref<4x4xf32>
      }
    }
  }
  return
}

// -----
// M. 64 bits on the constant-extent path: k in [i * 2^62 + c, i * 2^62 + c + 4) shifts
// together (family F), but at i = 1 the upper bound 2^63 + 2 leaves 64 bits (the run time wraps
// it negative and runs no k). The written bounds are checked on this path too: left as written,
// UNKNOWN (review of 2026-10-18: it was normalized and lowered as 2 x 4 x 4).
// CHECK-LABEL: func.func @shifted_bound_overflow
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      affine_gemm_exact_domains.mlir:[[@LINE+2]]:3: remark: affine GEMM not lowered to systolic.matmul_tile: the loop bounds are not static, so the iteration domain has no static normalized extent{{.*}}[UNKNOWN]
func.func @shifted_bound_overflow(%A: memref<2x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<2x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 2 {
    affine.for %j = 0 to 4 {
      affine.for %k = affine_map<(d0) -> (d0 * 4611686018427387904 + 4611686018427387902)>(%i) to affine_map<(d0) -> (d0 * 4611686018427387904 + 4611686018427387906)>(%i) {
        %a = affine.load %A[%i, %k - %i * 4611686018427387904 - 4611686018427387902] : memref<2x4xf32>
        %b = affine.load %B[%k - %i * 4611686018427387904 - 4611686018427387902, %j] : memref<4x4xf32>
        %c = affine.load %C[%i, %j] : memref<2x4xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<2x4xf32>
      }
    }
  }
  return
}

// -----
// M. 64 bits in a composed access: x1 = x1' + 2^61 x0, x2 = x2' + 2 x1 compose A[x1, x2] to a
// term 2^62 x0, which is 2^63 at x0 = 2 -- an `arith.muli ... overflow<nsw>` the written access
// does not have (every written bound and subscript fits). Every composed product must fit over
// the box: left as written, UNKNOWN (review of 2026-10-18).
// CHECK-LABEL: func.func @composed_product_overflow
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      affine_gemm_exact_domains.mlir:[[@LINE+2]]:3: remark: affine GEMM not lowered to systolic.matmul_tile: the loop bounds are not static, so the iteration domain has no static normalized extent{{.*}}[UNKNOWN]
func.func @composed_product_overflow(%A: memref<2x4xf32> {llvm.noalias}, %B: memref<4x3xf32> {llvm.noalias}, %C: memref<2x3xf32> {llvm.noalias}) {
  affine.for %x0 = 2 to 3 {
    affine.for %x1 = affine_map<(d0) -> (d0 * 2305843009213693952 - 4611686018427387904)>(%x0) to affine_map<(d0) -> (d0 * 2305843009213693952 - 4611686018427387902)>(%x0) {
      affine.for %x2 = affine_map<(d0) -> (d0 * 2)>(%x1) to affine_map<(d0) -> (d0 * 2 + 2)>(%x1) {
        %a = affine.load %A[%x1, %x2] : memref<2x4xf32>
        %b = affine.load %B[%x2, %x0] : memref<4x3xf32>
        %c = affine.load %C[%x1, %x0] : memref<2x3xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%x1, %x0] : memref<2x3xf32>
      }
    }
  }
  return
}

// -----
// E. A strip-mined reduction with a remainder: k = 2 k1 + k0 on {(0, 0), (0, 1), (1, 0)} takes
// 0, 1, 2 once each -- the updates are exactly those of the 4 x 4 x 3 GEMM, on a set that is not
// centrally symmetric. The accesses do not tell the iterations apart (their stacked map has
// rank 3 for 4 loops), so the symmetry argument proves nothing: UNKNOWN, never PROVEN_INVALID
// (review of 2026-10-18: it was PROVEN_INVALID; demoting k1 is the deferred decomposition).
// CHECK-LABEL: func.func @remainder_tile_k
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      affine_gemm_exact_domains.mlir:[[@LINE+2]]:3: remark: affine GEMM not lowered to systolic.matmul_tile: the loop bounds are not static, so the iteration domain has no static normalized extent{{.*}}[UNKNOWN]
func.func @remainder_tile_k(%A: memref<4x3xf32> {llvm.noalias}, %B: memref<3x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k1 = 0 to 2 {
        affine.for %k0 = 0 to affine_map<(d0) -> (-d0 + 2)>(%k1) {
          %a = affine.load %A[%i, %k1 * 2 + %k0] : memref<4x3xf32>
          %b = affine.load %B[%k1 * 2 + %k0, %j] : memref<3x4xf32>
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
// M. A bound MLIR's flattening wraps: j < min(4, (3i + 1) 2^62) has the coefficient 3 * 2^62,
// which does not fit in 64 bits (the flattened constraint system holds -2^62, a set with points
// the program never runs). The proof's set is built from checked linear forms, which refuse it:
// UNKNOWN (review of 2026-10-18: it was PROVEN_INVALID about the phantom set).
// CHECK-LABEL: func.func @flattening_wraps
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      affine_gemm_exact_domains.mlir:[[@LINE+2]]:3: remark: affine GEMM not lowered to systolic.matmul_tile: the loop bounds are not static, so the iteration domain has no static normalized extent{{.*}}[UNKNOWN]
func.func @flattening_wraps(%A: memref<2x2xf32> {llvm.noalias}, %B: memref<2x4xf32> {llvm.noalias}, %C: memref<2x4xf32> {llvm.noalias}) {
  affine.for %i = -1 to 1 {
    affine.for %j = 0 to min affine_map<(d0) -> (4, (d0 * 3 + 1) * 4611686018427387904)>(%i) {
      affine.for %k = 0 to affine_map<(d0) -> (d0 + 2)>(%i) {
        %a = affine.load %A[%i + 1, %k] : memref<2x2xf32>
        %b = affine.load %B[%k, %j] : memref<2x4xf32>
        %c = affine.load %C[%i + 1, %j] : memref<2x4xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i + 1, %j] : memref<2x4xf32>
      }
    }
  }
  return
}

// -----
// M. A mod in a bound: (i mod 5) 10^18 + 8223372036854775807 leaves 64 bits at i = -1
// ((-1) mod 5 = 4). A bound with floordiv / ceildiv / mod has no linear form: not attempted,
// UNKNOWN (review of 2026-10-18: an interval bound of mod missed the overflow).
// CHECK-LABEL: func.func @mod_bound
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      affine_gemm_exact_domains.mlir:[[@LINE+2]]:3: remark: affine GEMM not lowered to systolic.matmul_tile: the loop bounds are not static, so the iteration domain has no static normalized extent{{.*}}[UNKNOWN]
func.func @mod_bound(%A: memref<2x2xf32> {llvm.noalias}, %B: memref<2x4xf32> {llvm.noalias}, %C: memref<2x4xf32> {llvm.noalias}) {
  affine.for %i = -1 to 1 {
    affine.for %j = 0 to min affine_map<(d0) -> (4, (d0 mod 5) * 1000000000000000000 + 8223372036854775807)>(%i) {
      affine.for %k = 0 to affine_map<(d0) -> (d0 + 2)>(%i) {
        %a = affine.load %A[%i + 1, %k] : memref<2x2xf32>
        %b = affine.load %B[%k, %j] : memref<2x4xf32>
        %c = affine.load %C[%i + 1, %j] : memref<2x4xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i + 1, %j] : memref<2x4xf32>
      }
    }
  }
  return
}

// -----
// M. A bound evaluated where its loop is entered: j < min(4, 4 - 2^62 i) is computed at
// i = 0 .. 3, and at i = 1 .. 3 the product leaves 64 bits -- although the set's own i is 0
// only. The bounds are checked over the outer loops' own iteration set: UNKNOWN (review of
// 2026-10-18: the check over the set's counter ranges let PROVEN_INVALID through).
// CHECK-LABEL: func.func @outer_point_overflow
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      affine_gemm_exact_domains.mlir:[[@LINE+2]]:3: remark: affine GEMM not lowered to systolic.matmul_tile: the loop bounds are not static, so the iteration domain has no static normalized extent{{.*}}[UNKNOWN]
func.func @outer_point_overflow(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to min affine_map<(d0) -> (4, d0 * -4611686018427387904 + 4)>(%i) {
      affine.for %k = max affine_map<(d0, d1) -> (0, d1 + d0 * 5 - 15)>(%i, %j) to min affine_map<(d0, d1) -> (4, d1 + d0 * 5 + 1)>(%i, %j) {
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
// D. A stepped loop whose lower bound is a max of constants (max(0, -3), step 2): its stride
// counts from 0 exactly, so the triangle (j in [i, 4)) is proven: PROVEN_INVALID (review of
// 2026-10-18: only a single constant lower bound was accepted).
// CHECK-LABEL: func.func @stepped_max_of_constants
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      affine_gemm_exact_domains.mlir:[[@LINE+5]]:3: remark: affine GEMM not lowered to systolic.matmul_tile: the iteration domain is not centrally symmetric -- the point ({{[-0-9, ]+}}) is in it{{.*}}[PROVEN_INVALID]
// DISC:        dense logical GEMM: no (the iteration domain is not centrally symmetric
// DISC-NEXT:   bounds:
// DISC-NEXT:   class: PROVEN_INVALID
func.func @stepped_max_of_constants(%A: memref<4x8xf32> {llvm.noalias}, %B: memref<8x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = affine_map<(d0) -> (d0)>(%i) to 4 {
      affine.for %k = max affine_map<() -> (0, -3)>() to 8 step 2 {
        %a = affine.load %A[%i, %k] : memref<4x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x4xf32>
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
// G/D. A x A on a genuine triangle: the factors are one buffer and the product commutes, so two
// views of it may exchange their roles from point to point -- the symmetry argument is not
// applied: UNKNOWN.
// CHECK-LABEL: func.func @axa_triangle
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      affine_gemm_exact_domains.mlir:[[@LINE+2]]:3: remark: affine GEMM not lowered to systolic.matmul_tile: the loop bounds are not static, so the iteration domain has no static normalized extent{{.*}}[UNKNOWN]
func.func @axa_triangle(%A: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = affine_map<(d0) -> (d0)>(%i) to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<4x4xf32>
        %b = affine.load %A[%j, %k] : memref<4x4xf32>
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
// A stepped loop with max(0, 2i - 1): neither result is the max at every outer point (0 at
// i = 0, 2i - 1 after it), so its stride has no single origin -- not normalized; the disproof
// does not attempt a stepped loop whose lower bound reads a counter: UNKNOWN.
// CHECK-LABEL: func.func @stepped_no_dominant
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      affine_gemm_exact_domains.mlir:[[@LINE+2]]:3: remark: affine GEMM not lowered to systolic.matmul_tile: the loop bounds are not static, so the iteration domain has no static normalized extent{{.*}}[UNKNOWN]
func.func @stepped_no_dominant(%A: memref<4x8xf32> {llvm.noalias}, %B: memref<8x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = max affine_map<(d0) -> (0, d0 * 2 - 1)>(%i) to 8 step 2 {
        %a = affine.load %A[%i, %k] : memref<4x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x4xf32>
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
// M. @overflow_bound with k < j + 1: its mathematical set {0} x {(j, k) : k <= j} is not
// centrally symmetric, but j's bound needs 2^63 at i = 2: the guard keeps it UNKNOWN (review of
// 2026-10-18: it was PROVEN_INVALID).
// CHECK-LABEL: func.func @overflow_bound_asymmetric
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      affine_gemm_exact_domains.mlir:[[@LINE+2]]:3: remark: affine GEMM not lowered to systolic.matmul_tile: the loop bounds are not static, so the iteration domain has no static normalized extent{{.*}}[UNKNOWN]
func.func @overflow_bound_asymmetric(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = max affine_map<(d0) -> (0, d0 * 4611686018427387904)>(%i) to 4 {
      affine.for %k = 0 to affine_map<(d0) -> (d0 + 1)>(%j) {
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
// The repetition triangle of @rep_triangle_between with arguments that may overlap (no
// llvm.noalias): the dependence test sees one memref at a time, so it cannot decide the
// interchange -- not hoisted, the nest stays as written (its CPU code keeps the program's
// order), with a remark (review of 2026-10-18: it was hoisted, then refused for the alias, and
// the permuted nest ran on the CPU).
// CHECK-LABEL: func.func @alias_hoist_triangle
// CHECK-NEXT:    affine.for %{{.*}} = 0 to 4 {
// CHECK-NEXT:      affine.for %[[R1:[a-z0-9]+]] = 0 to 3 {
// CHECK-NEXT:        affine.for %{{.*}} = #map(%[[R1]]) to 2 {
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      affine_gemm_exact_domains.mlir:[[@LINE+2]]:3: remark: affine GEMM not lowered to systolic.matmul_tile: loop 2 of the nest repeats the accumulation inside it, and moving it outward is a legal loop interchange only if the memrefs the nest reads do not overlap the one it writes; declare the arguments llvm.noalias (C restrict) [UNKNOWN]
func.func @alias_hoist_triangle(%A: memref<4x4xf32>, %B: memref<4x4xf32>, %C: memref<4x4xf32>) {
  affine.for %j = 0 to 4 {
    affine.for %r1 = 0 to 3 {
      affine.for %r0 = affine_map<(d0) -> (d0)>(%r1) to 2 {
        affine.for %k = 0 to 4 {
          affine.for %i = 0 to 4 {
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
// A repetition loop that never runs (s0 + 3 to s0 + 1: the bounds differ by -2): the GEMM is
// lowered and counted 0 times (review of 2026-10-18: the negative difference was cast to
// uint64, 2^64 - 2 times).
// CHECK-LABEL: func.func @rep_never_runs
// REMARK:      affine_gemm_exact_domains.mlir:[[@LINE+3]]:5: remark: affine GEMM 4x4x4 lowered as 1 exact region(s); the enclosing loops run it 0 time(s)
func.func @rep_never_runs(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}, %n: index) {
  affine.for %r = affine_map<()[s0] -> (s0 + 3)>()[%n] to affine_map<()[s0] -> (s0 + 1)>()[%n] {
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
  }
  return
}

// -----
// A loop of one iteration leads the nest, then a repetition loop, with arguments that may
// overlap: moving r outward past t, which runs once, keeps the program's order, so the hoist
// needs neither the alias rule nor the dependence test and permutes the band without a remark
// (review of 2026-10-19: it refused with the alias remark, and the inner GEMM candidate was
// then recognized and refused again). The GEMM is recognized below r and t and refused once,
// for the alias.
// CHECK-LABEL: func.func @leading_once_alias
// CHECK-NEXT:    affine.for %{{.*}} = 0 to 2 {
// CHECK-NEXT:      affine.for %{{.*}} = 0 to 1 {
// CHECK-NOT:   systolic.matmul_tile
// REMARK-NOT:  repeats the accumulation
// REMARK:      affine_gemm_exact_domains.mlir:[[@LINE+5]]:7: remark: affine GEMM not lowered to systolic.matmul_tile: A may overlap C{{.*}}[UNKNOWN]
// REMARK-NOT:  repeats the accumulation
func.func @leading_once_alias(%A: memref<4x4xf32>, %B: memref<4x4xf32>, %C: memref<4x4xf32>) {
  affine.for %t = 0 to 1 {
    affine.for %r = 0 to 2 {
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
    }
  }
  return
}

// -----
// The same with a one-iteration loop that the accesses read (A[t + i][k]) and restrict
// arguments: r moves out past t (without the tests, which would allow it here too --
// @leading_once_alias tells the two paths apart), and the GEMM over (t, i, j, k) -- t a
// dimension of one value (Lemma 0) -- is lowered and run twice.
// CHECK-LABEL: func.func @leading_once_read
// CHECK:         systolic.matmul_tile
// REMARK:      affine_gemm_exact_domains.mlir:[[@LINE+2]]:3: remark: affine GEMM 4x4x4 lowered as 1 exact region(s); the enclosing loops run it 2 time(s)
func.func @leading_once_read(%A: memref<5x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %t = 1 to 2 {
    affine.for %r = 0 to 2 {
      affine.for %i = 0 to 4 {
        affine.for %j = 0 to 4 {
          affine.for %k = 0 to 4 {
            %a = affine.load %A[%t + %i, %k] : memref<5x4xf32>
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
// A repetition loop of one iteration whose bounds read an outer repetition counter (r0 in
// [r1, r1 + 1)): no constant on the domain, so not a dimension -- it repeats nothing, and the
// triangle r1, r0, r2 moves out past i with the dependence test (review of 2026-10-19: it was
// taken for a dimension with bounds that are not constant, and the nest stayed as written,
// unrecognized). The GEMM runs once per point of the triangle: 1 + 1 + 1 values of r0 and
// 3 + 2 + 1 of r2 -- 6 times.
// CHECK-LABEL: func.func @shifted_once_repetition
// CHECK:         systolic.matmul_tile
// REMARK:      affine_gemm_exact_domains.mlir:[[@LINE+2]]:3: remark: affine GEMM 4x4x4 lowered as 1 exact region(s); the enclosing loops run it 6 time(s)
func.func @shifted_once_repetition(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %r1 = 0 to 3 {
      affine.for %r0 = affine_map<(d0) -> (d0)>(%r1) to affine_map<(d0) -> (d0 + 1)>(%r1) {
        affine.for %r2 = affine_map<(d0) -> (d0)>(%r1) to 3 {
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
    }
  }
  return
}

// -----
// A loop of one iteration whose bounds are a max / min of constants (max(0, -1) to min(1, 4)),
// innermost: a dimension of one value for the recognizer (Lemma 0, its bounds folded) and,
// since 2026-10-19, for the hoist too -- nothing to move, lowered with no hoist remark (review
// of 2026-10-19: the hoist took it for a repetition loop that may run twice and refused, before
// the recognizer lowered the nest).
// CHECK-LABEL: func.func @maxmin_once_inner
// CHECK:         systolic.matmul_tile
// REMARK-NOT:  repeats the accumulation
// REMARK:      affine_gemm_exact_domains.mlir:[[@LINE+2]]:3: remark: affine GEMM 4x4x4 lowered as 1 exact region(s)
func.func @maxmin_once_inner(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        affine.for %t = max affine_map<() -> (0, -1)>() to min affine_map<() -> (1, 4)>() {
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
// The same loop between i and j, with arguments that may overlap: a dimension, so there is
// nothing to move -- the nest is recognized and refused once, for the alias (the hoist's alias
// remark came first before 2026-10-19).
// CHECK-LABEL: func.func @maxmin_once_alias
// CHECK-NOT:   systolic.matmul_tile
// REMARK-NOT:  repeats the accumulation
// REMARK:      affine_gemm_exact_domains.mlir:[[@LINE+2]]:3: remark: affine GEMM not lowered to systolic.matmul_tile: A may overlap C{{.*}}[UNKNOWN]
func.func @maxmin_once_alias(%A: memref<4x4xf32>, %B: memref<4x4xf32>, %C: memref<4x4xf32>) {
  affine.for %i = 0 to 4 {
    affine.for %t = max affine_map<() -> (0, -1)>() to min affine_map<() -> (1, 4)>() {
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

// -----
// A repetition loop whose trip count its bounds do not fix (0 to %n) inside i, with arguments
// that may overlap: it may run twice, so the interchange may reorder iterations, and the alias
// rule keeps the nest as written, with the hoist's remark (an unknown count counts as "may run
// twice"; review of 2026-10-19: no test pinned that default).
// CHECK-LABEL: func.func @symbol_range_inside
// CHECK-NEXT:    affine.for %{{.*}} = 0 to 4 {
// CHECK-NEXT:      affine.for %{{.*}} = 0 to %{{.*}} {
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      affine_gemm_exact_domains.mlir:[[@LINE+2]]:3: remark: affine GEMM not lowered to systolic.matmul_tile: loop 1 of the nest repeats the accumulation inside it, and moving it outward is a legal loop interchange only if the memrefs the nest reads do not overlap the one it writes; declare the arguments llvm.noalias (C restrict) [UNKNOWN]
func.func @symbol_range_inside(%A: memref<4x4xf32>, %B: memref<4x4xf32>, %C: memref<4x4xf32>, %n: index) {
  affine.for %i = 0 to 4 {
    affine.for %r = 0 to %n {
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

// -----
// A repetition loop of one iteration (r0 in [r1, r1 + 1)) inside the reduction loop k, under the
// repetition triangle r1, r2, with arguments that may overlap: it runs once at every point, so
// moving it out past i, j, k keeps the program's order -- permuted without a test -- and the
// GEMM is refused once, for the alias (it stayed inside k before 2026-10-19).
// CHECK-LABEL: func.func @once_inside_reduction
// CHECK-NEXT:    affine.for %[[R1:[a-z0-9]+]] = 0 to 3 {
// CHECK-NEXT:      affine.for %{{.*}} = #map(%[[R1]]) to 3 {
// CHECK-NEXT:        affine.for %{{.*}} = #map(%[[R1]]) to #map1(%[[R1]]) {
// CHECK-NEXT:          affine.for %{{.*}} = 0 to 4 {
// CHECK-NOT:   systolic.matmul_tile
// REMARK-NOT:  repeats the accumulation
// REMARK:      affine_gemm_exact_domains.mlir:[[@LINE+4]]:7: remark: affine GEMM not lowered to systolic.matmul_tile: A may overlap C{{.*}}[UNKNOWN]
func.func @once_inside_reduction(%A: memref<4x4xf32>, %B: memref<4x4xf32>, %C: memref<4x4xf32>) {
  affine.for %r1 = 0 to 3 {
    affine.for %r2 = affine_map<(d0) -> (d0)>(%r1) to 3 {
      affine.for %i = 0 to 4 {
        affine.for %j = 0 to 4 {
          affine.for %k = 0 to 4 {
            affine.for %r0 = affine_map<(d0) -> (d0)>(%r1) to affine_map<(d0) -> (d0 + 1)>(%r1) {
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
// The same with restrict arguments: lowered, run once per point of the triangle r1, r2 --
// 3 + 2 + 1 = 6 times (r0 adds no repetition).
// CHECK-LABEL: func.func @once_inside_reduction_restrict
// CHECK:         systolic.matmul_tile
// REMARK:      affine_gemm_exact_domains.mlir:[[@LINE+4]]:7: remark: affine GEMM 4x4x4 lowered as 1 exact region(s); the enclosing loops run it 6 time(s)
func.func @once_inside_reduction_restrict(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %r1 = 0 to 3 {
    affine.for %r2 = affine_map<(d0) -> (d0)>(%r1) to 3 {
      affine.for %i = 0 to 4 {
        affine.for %j = 0 to 4 {
          affine.for %k = 0 to 4 {
            affine.for %r0 = affine_map<(d0) -> (d0)>(%r1) to affine_map<(d0) -> (d0 + 1)>(%r1) {
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
// A repetition loop r whose constant bounds list the counter of the one-iteration loop t as an
// operand they do not read (affine_map<(d0) -> (0)>(%t)), with arguments that may overlap. The
// hoist keeps r inside t: when t was a dimension (Lemma 0), boundsOk refused it for naming one
// (review of 2026-10-19: before that it moved r out past t without a test, and the IR named %t
// before its definition -- "operand #0 does not dominate this use"); since round 4 t, whose
// counter r's bounds name, is a repetition loop itself, and the two are already outside the
// GEMM loops. The GEMM is recognized below r and t and refused once, for the alias.
// CHECK-LABEL: func.func @dead_operand_once
// CHECK-NEXT:    affine.for %[[T:[a-z0-9]+]] = 0 to 1 {
// CHECK-NEXT:      affine.for %{{.*}} = #map(%[[T]]) to #map1(%[[T]]) {
// CHECK-NOT:   systolic.matmul_tile
// REMARK-NOT:  repeats the accumulation
// REMARK:      affine_gemm_exact_domains.mlir:[[@LINE+4]]:7: remark: affine GEMM not lowered to systolic.matmul_tile: A may overlap C{{.*}}[UNKNOWN]
func.func @dead_operand_once(%A: memref<4x4xf32>, %B: memref<4x4xf32>, %C: memref<4x4xf32>) {
  affine.for %t = 0 to 1 {
    affine.for %r = affine_map<(d0) -> (0)>(%t) to affine_map<(d0) -> (2)>(%t) {
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
    }
  }
  return
}

// -----
// Such an operand on a repetition loop inside the dimension i whose counter it names (restrict
// arguments): not moved, the nest stays as written, unrecognized, with no remark (review of
// 2026-10-19: moved out past i and lowered, the IR then named the erased %i -- a crash).
// CHECK-LABEL: func.func @dead_operand_inside
// CHECK-NEXT:    affine.for %[[I:[a-z0-9]+]] = 0 to 4 {
// CHECK-NEXT:      affine.for %{{.*}} = #map(%[[I]]) to #map1(%[[I]]) {
// CHECK-NOT:   systolic.matmul_tile
// REMARK-NOT:  affine GEMM
func.func @dead_operand_inside(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %r = affine_map<(d0) -> (0)>(%i) to affine_map<(d0) -> (2)>(%i) {
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

// -----
// The accumulator's load and its store name one element on the domain through different maps
// (C[x1 + x2 + 3, 3] and C[x2 + 3, 3], x1 = 0): the stacked access map with the load -- the
// one the recognizer ranks -- has full rank, the one with the store does not. The disproof
// applies: PROVEN_INVALID (oracle seed 153, case_299; review of 2026-10-19: no test pinned which
// of the two maps is ranked).
// CHECK-LABEL: func.func @load_rank_full
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      affine_gemm_exact_domains.mlir:[[@LINE+2]]:3: remark: affine GEMM not lowered to systolic.matmul_tile: the iteration domain is not centrally symmetric{{.*}}[PROVEN_INVALID]
func.func @load_rank_full(%A: memref<12x7xf32> {llvm.noalias}, %B: memref<7x5xf32> {llvm.noalias}, %C: memref<10x7xf32> {llvm.noalias}) {
  affine.for %x0 = 3 to 4 {
    affine.for %x1 = max affine_map<() -> (-2, 0)>() to 1 {
      affine.for %x2 = -3 to min affine_map<() -> (5, 8, 8)>() step 3 {
        affine.for %x3 = affine_map<(d0) -> (d0 * -2 + 6)>(%x2) to 8 {
          %a = affine.load %A[%x2 + 8, %x1 * -1 + %x2 * -2 + %x3 * -1 + 12] : memref<12x7xf32>
          %b = affine.load %B[3, %x0 * -3 + %x2 * 4 + %x3 * 2 - 1] : memref<7x5xf32>
          %c = affine.load %C[%x1 + %x2 + 3, 3] : memref<10x7xf32>
          %p = arith.mulf %a, %b : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%x2 + 3, 3] : memref<10x7xf32>
        }
      }
    }
  }
  return
}

// -----
// The reverse: the map with the store has full rank, the one with the load does not (C[-2 x0 -
// x2 + 18, x1] loaded, C[-x2 + 8, x1] stored, x0 = 5). The disproof does not apply: UNKNOWN
// (oracle seed 255, case_22).
// CHECK-LABEL: func.func @store_rank_full
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      affine_gemm_exact_domains.mlir:[[@LINE+2]]:3: remark: affine GEMM not lowered to systolic.matmul_tile: the loop bounds are not static{{.*}}[UNKNOWN]
func.func @store_rank_full(%A: memref<1x4x8xf32> {llvm.noalias}, %B: memref<7x6xf32> {llvm.noalias}, %C: memref<4x7xf32> {llvm.noalias}) {
  affine.for %x0 = 5 to min affine_map<() -> (6, 8)>() step 3 {
    affine.for %x1 = max affine_map<() -> (3, 5)>() to min affine_map<() -> (8, 7, 10)>() {
      affine.for %x2 = 6 to 9 {
        affine.for %x3 = affine_map<(d0, d1) -> (d0 + d1 * -2)>(%x0, %x1) to min affine_map<() -> (7, 5, 4)>() {
          %a = affine.load %A[0, %x0 + %x1 * -2 + %x3 * -1 + 3, %x0 * -2 + %x2 * -1 + 20] : memref<1x4x8xf32>
          %b = affine.load %B[%x0 * 2 + %x1 * -4 + %x3 * -2 + 6, %x1 - 4] : memref<7x6xf32>
          %c = affine.load %C[%x0 * -2 + %x2 * -1 + 18, %x1] : memref<4x7xf32>
          %p = arith.mulf %b, %a : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%x2 * -1 + 8, %x1] : memref<4x7xf32>
        }
      }
    }
  }
  return
}

// -----
// A repetition loop whose bounds read the counter of a leading loop of one iteration (r0 in
// [r1 + t, 2)), with a repetition loop q inside the GEMM loop i to move (restrict arguments): t,
// whose counter a bound reads, is a repetition loop of one iteration -- it cannot run twice --
// and moves out with its readers, and q is hoisted past i with the tests. The GEMM runs once per
// point of t, r1, r0 (3) and q (2): 6 times (review of 2026-10-19, round 4: t was a dimension,
// boundsOk refused r0 for reading it, and the nest stayed on the host).
// CHECK-LABEL: func.func @once_read_by_repetition
// CHECK:         systolic.matmul_tile
// REMARK:      affine_gemm_exact_domains.mlir:[[@LINE+5]]:9: remark: affine GEMM 4x4x4 lowered as 1 exact region(s); the enclosing loops run it 6 time(s)
func.func @once_read_by_repetition(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %t = 0 to 1 {
    affine.for %r1 = 0 to 3 {
      affine.for %r0 = affine_map<(d0, d1) -> (d0 + d1)>(%r1, %t) to 2 {
        affine.for %i = 0 to 4 {
          affine.for %q = 0 to 2 {
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
      }
    }
  }
  return
}

// -----
// The same with arguments that may overlap: q inside i would reorder iterations, so the alias
// rule keeps the nest as written, with the hoist's remark -- one refusal.
// CHECK-LABEL: func.func @once_read_alias
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      affine_gemm_exact_domains.mlir:[[@LINE+2]]:3: remark: affine GEMM not lowered to systolic.matmul_tile: loop 4 of the nest repeats the accumulation inside it, and moving it outward is a legal loop interchange only if the memrefs the nest reads do not overlap the one it writes; declare the arguments llvm.noalias (C restrict) [UNKNOWN]
func.func @once_read_alias(%A: memref<4x4xf32>, %B: memref<4x4xf32>, %C: memref<4x4xf32>) {
  affine.for %t = 0 to 1 {
    affine.for %r1 = 0 to 3 {
      affine.for %r0 = affine_map<(d0, d1) -> (d0 + d1)>(%r1, %t) to 2 {
        affine.for %i = 0 to 4 {
          affine.for %q = 0 to 2 {
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
      }
    }
  }
  return
}

// -----
// A loop of one iteration whose bounds are a max / min of constants, read by a repetition
// triangle inside i (r1 in [t, 3), r0 in [r1, 3)): the three move out of i together, and the
// GEMM runs 3 + 2 + 1 = 6 times (round 4 of the review of 2026-10-19: folding t's bounds made it
// a dimension, and the previous build's lowering was lost).
// CHECK-LABEL: func.func @maxmin_once_read
// CHECK:         systolic.matmul_tile
// REMARK:      affine_gemm_exact_domains.mlir:[[@LINE+3]]:5: remark: affine GEMM 4x4x4 lowered as 1 exact region(s); the enclosing loops run it 6 time(s)
func.func @maxmin_once_read(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %t = max affine_map<() -> (0, -1)>() to min affine_map<() -> (1, 4)>() {
    affine.for %i = 0 to 4 {
      affine.for %r1 = affine_map<(d0) -> (d0)>(%t) to 3 {
        affine.for %r0 = affine_map<(d0) -> (d0)>(%r1) to 3 {
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
    }
  }
  return
}

// -----
// A loop of one iteration whose upper bound is min(0, 2^63 - 1): the second result's
// difference to the lower bound does not fit in 64 bits, and it is never the min -- the trip
// count is 1, a dimension, nothing to move (round 4: the count was unknown, the hoist refused
// the nest and the recognizer then lowered it).
// CHECK-LABEL: func.func @ovf_once_inner
// CHECK:         systolic.matmul_tile
// REMARK-NOT:  repeats the accumulation
// REMARK:      affine_gemm_exact_domains.mlir:[[@LINE+2]]:3: remark: affine GEMM 4x4x4 lowered as 1 exact region(s)
func.func @ovf_once_inner(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        affine.for %t = -1 to min affine_map<() -> (0, 9223372036854775807)>() {
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
// A repetition loop of two iterations inside the reduction loop whose lower bound is
// max(-2^63, 0): MLIR's dependence analysis negates the constant in 64-bit arithmetic, which
// wraps, and found no dependence -- the hoist moved t out of k, which reorders the accumulation
// (under strict floating point the nest was then refused and its CPU code ran permuted). A
// bound or subscript constant of 2^48 or more leaves the nest as written, UNKNOWN (round 4).
// CHECK-LABEL: func.func @min_folded
// CHECK-NEXT:    affine.for %{{.*}} = 0 to 4 {
// CHECK-NEXT:      affine.for %{{.*}} = 0 to 4 {
// CHECK-NEXT:        affine.for %{{.*}} = 0 to 4 {
// CHECK-NEXT:          affine.for %{{.*}} = max #map() to 2 {
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      affine_gemm_exact_domains.mlir:[[@LINE+2]]:3: remark: affine GEMM not lowered to systolic.matmul_tile: loop 3 of the nest repeats the accumulation inside it, and whether moving it outward reorders dependent iterations is not decided (a bound or subscript has a constant of 2^48 or more, beyond the dependence analysis' 64-bit arithmetic) [UNKNOWN]
func.func @min_folded(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        affine.for %t = max affine_map<() -> (-9223372036854775807 - 1, 0)>() to 2 {
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
