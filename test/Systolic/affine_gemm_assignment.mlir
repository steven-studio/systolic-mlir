// Assignment nests C = A * B (docs/AFFINE_GEMM_MAPPING.md section 20, 2026-10-19).
// A nest whose innermost body stores the product of two loads -- no accumulator
// load -- is the K = 1 GEMM into a zeroed C, C = 0 + A B, when every element
// of C is written exactly once: the recognizer refuses a loop of two or more
// iterations that no subscript of C reads (the last product would win), the
// lowering proves C's index map injective on the whole domain
// (verifyOutputWrittenOnce: batches may not overlap in C) and zeroes C's
// logical window before the tiles accumulate into it. The rank-1 outer
// products C[i][j] = A[i] * B[j] and C[i][j] = A[j] * B[i] are such nests.
//
// RUN: systolic-opt "--affine-matmul-to-systolic=allow-fp-reassociation=true" \
// RUN:   -split-input-file %s | FileCheck %s
// RUN: systolic-opt "--affine-matmul-to-systolic=report=true allow-fp-reassociation=true classify=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=REMARK
// RUN: systolic-opt --test-affine-matmul-discovery -split-input-file %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s --check-prefix=DISC
//
// The strict default: an assignment is offloaded only with the permission to
// reassociate, like an accumulation (the tiles start from +0: a -0 product
// becomes +0); the permission is the product's `reassoc` flag.
// RUN: systolic-opt "--affine-matmul-to-systolic=report=true classify=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=STRICT
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-point reassociation is not permitted: the systolic execution sums each tile from zero (+0) and writes the sum to C, so a -0 product of the assignment C = A * B becomes +0 (mark the product reassoc -- systolic-cc --fp-reassociation -- or pass allow-fp-reassociation) [PROVEN_VALID]
// STRICT:      remark: affine GEMM 3x5x1 lowered as 1 exact region(s) (an assignment C = A B: C zeroed before the tiles)
// STRICT-NOT:  lowered as

// 1. The rank-1 outer product C[i][j] = A[i] * B[j], 3 x 5: A is the 3 x 1
//    column (memref.expand_shape), B the 1 x 5 row, C zeroed then the K = 1
//    tile accumulates into it.
// CHECK-LABEL: func.func @outer_assign
// CHECK:       %[[A:.*]] = memref.expand_shape %{{.*}} {{\[\[}}0, 1]] output_shape [3, 1]
// CHECK:       %[[B:.*]] = memref.expand_shape %{{.*}} {{\[\[}}0, 1]] output_shape [1, 5]
// CHECK:       %[[ZERO:.*]] = arith.constant 0.000000e+00 : f32
// CHECK:       affine.for %[[I:.*]] = 0 to 3 {
// CHECK-NEXT:    affine.for %[[J:.*]] = 0 to 5 {
// CHECK-NEXT:      affine.store %[[ZERO]], %arg2[%[[I]], %[[J]]] : memref<3x5xf32>
// CHECK:       %[[C:.*]] = bufferization.to_tensor %arg2 restrict writable
// CHECK:       systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1
// CHECK:       bufferization.materialize_in_destination {{.*}} in restrict writable %arg2
// REMARK:      remark: affine GEMM 3x5x1 lowered as 1 exact region(s) (an assignment C = A B: C zeroed before the tiles)
// DISC:        recognition result: success
// DISC:        statement: C = A * B (assignment: every element of C written once, lowered as the K = 1 GEMM into a zeroed C)
// DISC:        dense logical GEMM: yes
func.func @outer_assign(%A: memref<3xf32> {llvm.noalias}, %B: memref<5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      %a = affine.load %A[%i] : memref<3xf32>
      %b = affine.load %B[%j] : memref<5xf32>
      %p = arith.mulf %a, %b : f32
      affine.store %p, %C[%i, %j] : memref<3x5xf32>
    }
  }
  return
}

// -----

// 2. The transposed outer product C[i][j] = A[j] * B[i]: the recognizer takes
//    C = B' A' (the factors swapped), the same 3 x 5 x 1 GEMM. The
//    permission to reassociate is the product's flag (the STRICT run above
//    lowers this nest and no other).
// CHECK-LABEL: func.func @outer_assign_swapped
// CHECK:       affine.store %{{.*}}, %arg2[%{{.*}}, %{{.*}}] : memref<3x5xf32>
// CHECK:       systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1
// REMARK:      remark: affine GEMM 3x5x1 lowered as 1 exact region(s) (an assignment C = A B: C zeroed before the tiles)
// DISC:        recognition result: success
// DISC:        reduction: reassoc permitted
// DISC:        statement: C = A * B (assignment
func.func @outer_assign_swapped(%A: memref<5xf32> {llvm.noalias}, %B: memref<3xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      %a = affine.load %A[%j] : memref<5xf32>
      %b = affine.load %B[%i] : memref<3xf32>
      %p = arith.mulf %a, %b fastmath<reassoc> : f32
      affine.store %p, %C[%i, %j] : memref<3x5xf32>
    }
  }
  return
}

// -----

// 3. The accumulation twin C[i][j] += A[i] * B[j] is unchanged: no zeroing,
//    C's prior value is kept.
// CHECK-LABEL: func.func @outer_acc
// CHECK-NOT:   affine.store
// CHECK:       systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1
// REMARK:      remark: affine GEMM 3x5x1 lowered as 1 exact region(s){{$}}
// DISC:        recognition result: success
// DISC:        statement: C = C + A * B (accumulation)
func.func @outer_acc(%A: memref<3xf32> {llvm.noalias}, %B: memref<5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      %a = affine.load %A[%i] : memref<3xf32>
      %b = affine.load %B[%j] : memref<5xf32>
      %c = affine.load %C[%i, %j] : memref<3x5xf32>
      %p = arith.mulf %a, %b : f32
      %s = arith.addf %c, %p : f32
      affine.store %s, %C[%i, %j] : memref<3x5xf32>
    }
  }
  return
}

// -----

// 4. A batched assignment C[b][i][j] = A[b][i] * B[b][j]: one 3 x 5 x 1 GEMM
//    per batch value, its own C slab zeroed inside the batch loop.
// CHECK-LABEL: func.func @batched_assign
// CHECK:       affine.for %[[BATCH:.*]] = 0 to 2 {
// CHECK:         %[[CV:.*]] = memref.subview %arg2[%{{.*}}, 0, 0] [1, 3, 5] [1, 1, 1]
// CHECK:         affine.for %[[I:.*]] = 0 to 3 {
// CHECK-NEXT:      affine.for %[[J:.*]] = 0 to 5 {
// CHECK-NEXT:        affine.store %{{.*}}, %[[CV]][%[[I]], %[[J]]]
// CHECK:         systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1
// REMARK:      remark: affine GEMM 3x5x1 lowered as 1 exact region(s) (an assignment C = A B: C zeroed before the tiles), batched: 2 batch iteration(s) of 1 batch loop(s); the enclosing loops run it 2 time(s)
// DISC:        recognition result: success
// DISC:        statement: C = A * B (assignment
func.func @batched_assign(%A: memref<2x3xf32> {llvm.noalias}, %B: memref<2x5xf32> {llvm.noalias}, %C: memref<2x3x5xf32> {llvm.noalias}) {
  affine.for %b = 0 to 2 {
    affine.for %i = 0 to 3 {
      affine.for %j = 0 to 5 {
        %a = affine.load %A[%b, %i] : memref<2x3xf32>
        %bb = affine.load %B[%b, %j] : memref<2x5xf32>
        %p = arith.mulf %a, %bb : f32
        affine.store %p, %C[%b, %i, %j] : memref<2x3x5xf32>
      }
    }
  }
  return
}

// -----

// 5. A strided and offset assignment C[2i + 1][j] = A[i + 1] * B[2j]: the
//    views are the operands' own; the rows C does not write are untouched
//    (only the 3 x 5 window is zeroed, through the strided subview).
// CHECK-LABEL: func.func @strided_assign
// CHECK:       %[[CV:.*]] = memref.subview %arg2[1, 0] [3, 5] [2, 1] : memref<8x5xf32> to memref<3x5xf32, strided<[10, 1], offset: 5>>
// CHECK:       affine.store %{{.*}}, %[[CV]][%{{.*}}, %{{.*}}] : memref<3x5xf32, strided<[10, 1], offset: 5>>
// CHECK:       systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1
// REMARK:      remark: affine GEMM 3x5x1 lowered as 1 exact region(s) (an assignment C = A B: C zeroed before the tiles)
// DISC:        recognition result: success
func.func @strided_assign(%A: memref<4xf32> {llvm.noalias}, %B: memref<10xf32> {llvm.noalias}, %C: memref<8x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      %a = affine.load %A[%i + 1] : memref<4xf32>
      %b = affine.load %B[%j * 2] : memref<10xf32>
      %p = arith.mulf %a, %b : f32
      affine.store %p, %C[%i * 2 + 1, %j] : memref<8x5xf32>
    }
  }
  return
}

// -----

// 6. Not a GEMM: C[i][j] = A[i][k] * B[k][j] with k of 4 iterations. Each
//    element of C is written four times; only the last product survives.
//    PROVEN_INVALID at recognition, nothing lowered.
// CHECK-LABEL: func.func @overwrite_k
// CHECK-NOT:   systolic.matmul_tile
// REMARK-NOT:  lowered as
// DISC:        recognition result: failure
// DISC-NEXT:   reason: the statement overwrites C (C = A * B, no accumulation) and loop 2 of the nest, which runs more than once, is not a subscript of C: every iteration of it writes the same elements of C again and only the last product survives -- not the GEMM of A and B (an assignment is the K = 1 GEMM into a zeroed C only when every element of C is written once: docs/AFFINE_GEMM_MAPPING.md section 20)
// DISC-NEXT:   class: PROVEN_INVALID
func.func @overwrite_k(%A: memref<3x4xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<3x4xf32>
        %b = affine.load %B[%k, %j] : memref<4x5xf32>
        %p = arith.mulf %a, %b : f32
        affine.store %p, %C[%i, %j] : memref<3x5xf32>
      }
    }
  }
  return
}

// -----

// 7. Not a GEMM: batches overlapping in C, C[i + b][j] = A[b][i] * B[b][j].
//    Per batch C is injective (the recognizer accepts it, as it does the
//    accumulation C[i + b][j] += ..., a batched GEMM of views); over the whole
//    domain rows 1 and 2 are written twice: the lowering's injectivity proof
//    refuses it, PROVEN_INVALID.
// CHECK-LABEL: func.func @batch_overlap_assign
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      remark: affine GEMM not lowered to systolic.matmul_tile: two iterations of the assignment C = A * B write the same element of C (its subscripts are not injective on the iteration domain): the later product overwrites the earlier one -- not the GEMM of A and B (docs/AFFINE_GEMM_MAPPING.md section 20) [PROVEN_INVALID]
// DISC:        recognition result: success
// DISC:        statement: C = A * B (assignment
func.func @batch_overlap_assign(%A: memref<2x3xf32> {llvm.noalias}, %B: memref<2x5xf32> {llvm.noalias}, %C: memref<4x5xf32> {llvm.noalias}) {
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

// 8. Not a GEMM: the element-wise product C[i] = A[i] * B[i] (the one loop is
//    read by all three operands): PROVEN_INVALID, not an outer product.
// CHECK-LABEL: func.func @elementwise_assign
// CHECK-NOT:   systolic.matmul_tile
// DISC:        recognition result: failure
// DISC-NEXT:   reason: 1 coordinate(s) are read by all three operands and none by two: an element-wise product
// DISC-NEXT:   class: PROVEN_INVALID
func.func @elementwise_assign(%A: memref<5xf32> {llvm.noalias}, %B: memref<5xf32> {llvm.noalias}, %C: memref<5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 5 {
    %a = affine.load %A[%i] : memref<5xf32>
    %b = affine.load %B[%i] : memref<5xf32>
    %p = arith.mulf %a, %b : f32
    affine.store %p, %C[%i] : memref<5xf32>
  }
  return
}

// -----

// 9. Not a GEMM: a flattened C whose stride is smaller than the column extent,
//    C[4i + j] = A[i] * B[j] with j in [0, 5): (i, j) and (i + 1, j - 4) write
//    one element. For an accumulation this is "valid, not constructed" (a
//    tile would lose an update); for an assignment the later product
//    overwrites the earlier one: PROVEN_INVALID.
// CHECK-LABEL: func.func @flat_collision_assign
// CHECK-NOT:   systolic.matmul_tile
// DISC:        recognition result: failure
// DISC-NEXT:   reason: C's view is not injective on the (I, J) box: the outputs with ordinals (I, J) and (I + 1, J + -4) write the same element of C: the assignment C = A * B overwrites it, only the last product survives -- not the GEMM of A and B (docs/AFFINE_GEMM_MAPPING.md section 20)
// DISC-NEXT:   class: PROVEN_INVALID
func.func @flat_collision_assign(%A: memref<3xf32> {llvm.noalias}, %B: memref<5xf32> {llvm.noalias}, %C: memref<15xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      %a = affine.load %A[%i] : memref<3xf32>
      %b = affine.load %B[%j] : memref<5xf32>
      %p = arith.mulf %a, %b : f32
      affine.store %p, %C[%i * 4 + %j] : memref<15xf32>
    }
  }
  return
}

// -----

// 10. C[i][j] = A[0] * B[j]: i is read by C only. Since 2026-10-19 (section 21)
//     such a loop partitions the GEMM with both factors shared: per i the
//     1 x 5 x 1 assignment C[i][:] = A[0] * B[:], its slab zeroed first (was
//     refused as a broadcast).
// CHECK-LABEL: func.func @broadcast_assign
// CHECK:       affine.for %{{.*}} = 0 to 3 {
// CHECK:         affine.store %{{.*}} : memref<1x5xf32, strided<[5, 1], offset: ?>>
// CHECK:         systolic.matmul_tile {{.*}} m = 1 n = 5 k = 1
// REMARK:      remark: affine GEMM 1x5x1 lowered as 1 exact region(s) (an assignment C = A B: C zeroed before the tiles), partitioned on 1 demoted loop(s), batched: 3 batch iteration(s) of 1 batch loop(s)
// DISC:        recognition result: success
// DISC:        partition: loop(s) 0 demoted to batch loops
func.func @broadcast_assign(%A: memref<3xf32> {llvm.noalias}, %B: memref<5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      %a = affine.load %A[0] : memref<3xf32>
      %b = affine.load %B[%j] : memref<5xf32>
      %p = arith.mulf %a, %b : f32
      affine.store %p, %C[%i, %j] : memref<3x5xf32>
    }
  }
  return
}

// -----

// 11. Not a GEMM: the product plus a constant, C[i][j] = A[i] * B[j] + 1
//     (two loads, an addf whose operand is not the accumulator).
// CHECK-LABEL: func.func @plus_constant
// CHECK-NOT:   systolic.matmul_tile
// DISC:        recognition result: failure
// DISC-NEXT:   reason: the stored value is not an arith.addf of the accumulator and a product (C = C + A * B) nor an arith.mulf of two loads (C = A * B)
// DISC-NEXT:   class: PROVEN_INVALID
func.func @plus_constant(%A: memref<3xf32> {llvm.noalias}, %B: memref<5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  %one = arith.constant 1.0 : f32
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      %a = affine.load %A[%i] : memref<3xf32>
      %b = affine.load %B[%j] : memref<5xf32>
      %p = arith.mulf %a, %b : f32
      %s = arith.addf %p, %one : f32
      affine.store %s, %C[%i, %j] : memref<3x5xf32>
    }
  }
  return
}

// -----

// 12. Not lowered: C may alias a factor, A[i][j] = A[i][0] * B[j] (the nest
//     overwrites A[i][0] at j = 0 and reads it after): UNKNOWN, stays as
//     written, as the accumulation form does.
// CHECK-LABEL: func.func @alias_assign
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      remark: affine GEMM not lowered to systolic.matmul_tile: A may overlap C; declare the arguments llvm.noalias (C restrict) or pass assume-no-alias [UNKNOWN]
func.func @alias_assign(%A: memref<3x5xf32>, %B: memref<5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      %a = affine.load %A[%i, 0] : memref<3x5xf32>
      %b = affine.load %B[%j] : memref<5xf32>
      %p = arith.mulf %a, %b : f32
      affine.store %p, %A[%i, %j] : memref<3x5xf32>
    }
  }
  return
}
