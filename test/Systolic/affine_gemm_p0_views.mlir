// P0 operand views (docs/AFFINE_GEMM_MAPPING.md sections 5, 6, 9): negative
// strides, transposes, extent-one roles and dimensions, operands of rank 1
// and 3, folded constant bounds, the floating-point policy, aliasing. Every
// expected view below is derived from the subscripts by hand (the derivation
// is in each function's comment); the brute-force oracle
// (affine_gemm_mapping_oracle.mlir) checks the same kinds of views
// semantically on random nests, and test/SystolicCC/affine_p0.c runs them.
//
// A view is printed as memref.subview %arg[offsets] [sizes] [strides] over
// the operand's dimensions: a negative stride is a dimension the operand reads
// backwards (offset = its largest index); a transpose follows when the
// operand stores the roles in the other order; a size-1 dimension that is not
// kept is an extent-one (constant) dimension.
//
// RUN: systolic-opt "--affine-matmul-to-systolic=allow-fp-reassociation=true" \
// RUN:   -split-input-file %s | FileCheck %s
// RUN: systolic-opt "--affine-matmul-to-systolic=report=true allow-fp-reassociation=true classify=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=REMARK
// RUN: systolic-opt --test-affine-matmul-discovery -split-input-file %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s --check-prefix=DISC
//
// The default (no permission): only the nest whose accumulation carries the
// `reassoc` fast-math flag is lowered; every other valid GEMM stays as written,
// classified PROVEN_VALID, with the reason.
// RUN: systolic-opt "--affine-matmul-to-systolic=report=true classify=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=STRICT
//
// STRICT:     remark: affine GEMM not lowered to systolic.matmul_tile: floating-point reassociation is not permitted: the systolic execution sums each tile from zero and adds the sum to C, a different order of IEEE additions than the loop's (mark the accumulation reassoc -- systolic-cc --fp-reassociation -- or pass allow-fp-reassociation) [PROVEN_VALID]
// STRICT:     remark: affine GEMM 8x8x8 lowered as 1 exact region(s)
// STRICT-NOT: lowered as

// 1. A negative-stride A operand: A[7 - i][k]. C row i reads A row 7 - i:
//    A's rows run backwards from 7, stride -1; B and C whole.
// CHECK-LABEL: func.func @neg_a
// CHECK-NOT:   affine.for
// CHECK:       memref.subview %arg0[7, 0] [8, 8] [-1, 1]
// CHECK-NOT:   memref.subview
// CHECK:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 8
// REMARK:      remark: affine GEMM 8x8x8 lowered as 1 exact region(s)
func.func @neg_a(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[7 - %i, %k] : memref<8x8xf32>
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
// 2. A negative-stride B operand: B[k][7 - j]: B's columns from 7, stride -1.
// CHECK-LABEL: func.func @neg_b
// CHECK:       memref.subview %arg1[0, 7] [8, 8] [1, -1]
// CHECK:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 8
// REMARK:      remark: affine GEMM 8x8x8 lowered as 1 exact region(s)
func.func @neg_b(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i, %k] : memref<8x8xf32>
        %b = affine.load %B[%k, 7 - %j] : memref<8x8xf32>
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
// 3. A negative-stride output view: C[i][7 - j]. The tile's result is written
//    back through the same view (materialize_in_destination).
// CHECK-LABEL: func.func @neg_c
// CHECK:       %[[CV:.*]] = memref.subview %arg2[0, 7] [8, 8] [1, -1]
// CHECK:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 8
// CHECK:       bufferization.materialize_in_destination {{.*}} in restrict writable %[[CV]]
// REMARK:      remark: affine GEMM 8x8x8 lowered as 1 exact region(s)
func.func @neg_c(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i, %k] : memref<8x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x8xf32>
        %c = affine.load %C[%i, 7 - %j] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, 7 - %j] : memref<8x8xf32>
      }
    }
  }
  return
}

// -----
// 4. A decreasing induction sequence: for (i = 10; i >= 0; i -= 2), as the
//    front end emits it -- the ordinal t in [0, 6), i = 10 - 2t -- with A read
//    at i and C at i / 2 (spelled 5 - t). Both decrease in t: A's rows 10, 8,
//    .., 0 (offset 10, stride -2), C's rows 5, 4, .., 0 (offset 5, stride -1).
// CHECK-LABEL: func.func @decreasing_sequence
// CHECK:       memref.subview %arg0[10, 0] [6, 8] [-2, 1]
// CHECK:       memref.subview %arg2[5, 0] [6, 8] [-1, 1]
// CHECK:       systolic.matmul_tile {{.*}} m = 6 n = 8 k = 8
// REMARK:      remark: affine GEMM 6x8x8 lowered as 1 exact region(s)
func.func @decreasing_sequence(%A: memref<11x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<6x8xf32> {llvm.noalias}) {
  affine.for %t = 0 to 6 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[10 - %t * 2, %k] : memref<11x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x8xf32>
        %c = affine.load %C[5 - %t, %j] : memref<6x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[5 - %t, %j] : memref<6x8xf32>
      }
    }
  }
  return
}

// -----
// 5. A negative stride with a non-unit loop step: k = 1, 4, 7, 10, 13
//    (1 to 16 step 3); A[i][k] reads columns 1 .. 13 step 3, B[15 - k][j] rows
//    14, 11, .., 2 (offset 14, stride -3).
// CHECK-LABEL: func.func @neg_step
// CHECK:       memref.subview %arg0[0, 1] [8, 5] [1, 3]
// CHECK:       memref.subview %arg1[14, 0] [5, 8] [-3, 1]
// CHECK:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 5
// REMARK:      remark: affine GEMM 8x8x5 lowered as 1 exact region(s)
func.func @neg_step(%A: memref<8x16xf32> {llvm.noalias}, %B: memref<16x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 1 to 16 step 3 {
        %a = affine.load %A[%i, %k] : memref<8x16xf32>
        %b = affine.load %B[15 - %k, %j] : memref<16x8xf32>
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
// 6. A negative stride with a nonzero offset and stride 2: A[20 - 2i][k]
//    (rows 20, 18, .., 6), with C[i + 5][j] (rows 5 .. 12).
// CHECK-LABEL: func.func @neg_offset
// CHECK:       memref.subview %arg0[20, 0] [8, 8] [-2, 1]
// CHECK:       memref.subview %arg2[5, 0] [8, 8] [1, 1]
// CHECK:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 8
// REMARK:      remark: affine GEMM 8x8x8 lowered as 1 exact region(s)
func.func @neg_offset(%A: memref<21x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[20 - %i * 2, %k] : memref<21x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x8xf32>
        %c = affine.load %C[%i + 5, %j] : memref<16x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i + 5, %j] : memref<16x8xf32>
      }
    }
  }
  return
}

// -----
// 7. A negative stride and a transpose: A[10 - k][i] stores K in dimension
//    0 (backwards from 10) and I in dimension 1: the subview keeps the
//    memref's order, the transpose makes it (I, K).
// CHECK-LABEL: func.func @neg_transposed
// CHECK:       %[[SV:.*]] = memref.subview %arg0[10, 0] [8, 8] [-1, 1]
// CHECK:       memref.transpose %[[SV]] (d0, d1) -> (d1, d0)
// CHECK:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 8
// REMARK:      remark: affine GEMM 8x8x8 lowered as 1 exact region(s)
func.func @neg_transposed(%A: memref<12x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[10 - %k, %i] : memref<12x8xf32>
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
// 8. A negative-stride view whose logical origin is neither the first nor
//    the last element of the allocation: A[12 - i][k] in a 16-row A starts at
//    row 12 and runs down to row 5; rows 0..4 and 13..15 are never touched.
// CHECK-LABEL: func.func @neg_mid_origin
// CHECK:       memref.subview %arg0[12, 0] [8, 8] [-1, 1]
// CHECK:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 8
// REMARK:      remark: affine GEMM 8x8x8 lowered as 1 exact region(s)
func.func @neg_mid_origin(%A: memref<16x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[12 - %i, %k] : memref<16x8xf32>
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
// 9. Near the integer limits: A (dynamic rows) read backwards from row
//    2^60 - 1: the view's linear offset is (2^60 - 1) * 8 = 2^63 - 8, the
//    largest multiple of 8 that fits -- every address checked, none wrapped.
// CHECK-LABEL: func.func @neg_near_limit
// CHECK:       memref.subview %arg0[1152921504606846975, 0] [8, 8] [-1, 1] {{.*}} offset: 9223372036854775800
// CHECK:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 8
// REMARK:      remark: affine GEMM 8x8x8 lowered as 1 exact region(s)
func.func @neg_near_limit(%A: memref<?x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[1152921504606846975 - %i, %k] : memref<?x8xf32>
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
// 9b. One row further: the linear offset 2^60 * 8 = 2^63 does not fit;
//     refused (UNKNOWN), never wrapped.
// CHECK-LABEL: func.func @neg_past_limit
// CHECK:       affine.for
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      remark: affine GEMM not lowered to systolic.matmul_tile: A's element addresses overflow 64-bit arithmetic [UNKNOWN]
func.func @neg_past_limit(%A: memref<?x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[1152921504606846976 - %i, %k] : memref<?x8xf32>
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
// 10. Invalid negative-stride accesses: A[6 - i][k] reaches row -1 (i = 7),
//     C[i][8 - j] column 8 of 8: the program itself is out of bounds
//     (undefined); refused, PROVEN_INVALID, nothing accessed.
// CHECK-LABEL: func.func @neg_oob_low
// CHECK:       affine.for
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      remark: affine GEMM not lowered to systolic.matmul_tile: A is accessed out of bounds along dimension 0 [PROVEN_INVALID]
func.func @neg_oob_low(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[6 - %i, %k] : memref<8x8xf32>
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
// CHECK-LABEL: func.func @neg_oob_high
// CHECK:       affine.for
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      remark: affine GEMM not lowered to systolic.matmul_tile: C is accessed out of bounds along dimension 1 [PROVEN_INVALID]
func.func @neg_oob_high(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i, %k] : memref<8x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x8xf32>
        %c = affine.load %C[%i, 8 - %j] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, 8 - %j] : memref<8x8xf32>
      }
    }
  }
  return
}

// -----
// Transposes: C = A^T B (A[k][i]), C = A B^T (B[j][k]), C^T = A B (C[j][i]).
// The factor assignment that transposes least is chosen: for C[j][i] that
// is A' = B^T... -- here C^T += A B is lowered as C' = C^T, one transpose.
// CHECK-LABEL: func.func @transposed_b
// CHECK:       %[[SV:.*]] = memref.subview %arg1[0, 0] [8, 8] [1, 1]
// CHECK:       memref.transpose %[[SV]] (d0, d1) -> (d1, d0)
// CHECK:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 8
// REMARK:      remark: affine GEMM 8x8x8 lowered as 1 exact region(s)
func.func @transposed_b(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i, %k] : memref<8x8xf32>
        %b = affine.load %B[%j, %k] : memref<8x8xf32>
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
// CHECK-LABEL: func.func @transposed_c
// CHECK:       %[[SV:.*]] = memref.subview %arg2[0, 0] [8, 4] [1, 1]
// CHECK:       %[[TR:.*]] = memref.transpose %[[SV]] (d0, d1) -> (d1, d0)
// CHECK:       systolic.matmul_tile {{.*}} m = 4 n = 8 k = 8
// CHECK:       bufferization.materialize_in_destination {{.*}} in restrict writable %[[TR]]
// REMARK:      remark: affine GEMM 4x8x8 lowered as 1 exact region(s)
func.func @transposed_c(%A: memref<4x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i, %k] : memref<4x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x8xf32>
        %c = affine.load %C[%j, %i] : memref<8x4xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%j, %i] : memref<8x4xf32>
      }
    }
  }
  return
}

// -----
// Extent one and other ranks. A matrix-vector product y[i] += M[i][k] * x[k]
// (two loops, rank-1 x and y): N = 1, x and y get a unit column
// (expand_shape).
// CHECK-LABEL: func.func @gemv
// CHECK:       %[[X:.*]] = memref.subview %arg1[0] [8] [1]
// CHECK:       memref.expand_shape %[[X]] {{\[\[}}0, 1]] output_shape [8, 1]
// CHECK:       %[[Y:.*]] = memref.subview %arg2[0] [8] [1]
// CHECK:       memref.expand_shape %[[Y]] {{\[\[}}0, 1]] output_shape [8, 1]
// CHECK:       systolic.matmul_tile {{.*}} m = 8 n = 1 k = 8
// REMARK:      remark: affine GEMM 8x1x8 lowered as 1 exact region(s)
func.func @gemv(%M: memref<8x8xf32> {llvm.noalias}, %x: memref<8xf32> {llvm.noalias}, %y: memref<8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %k = 0 to 8 {
      %a = affine.load %M[%i, %k] : memref<8x8xf32>
      %b = affine.load %x[%k] : memref<8xf32>
      %c = affine.load %y[%i] : memref<8xf32>
      %p = arith.mulf %a, %b : f32
      %s = arith.addf %c, %p : f32
      affine.store %s, %y[%i] : memref<8xf32>
    }
  }
  return
}

// -----
// An outer product: K = 1 through constant subscripts (A[i][3], B[3][j]).
// CHECK-LABEL: func.func @outer
// CHECK:       memref.subview %arg0[0, 3] [8, 1] [1, 1]
// CHECK:       memref.subview %arg1[3, 0] [1, 8] [1, 1]
// CHECK:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 1
// REMARK:      remark: affine GEMM 8x8x1 lowered as 1 exact region(s)
func.func @outer(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      %a = affine.load %A[%i, 3] : memref<8x8xf32>
      %b = affine.load %B[3, %j] : memref<8x8xf32>
      %c = affine.load %C[%i, %j] : memref<8x8xf32>
      %p = arith.mulf %a, %b : f32
      %s = arith.addf %c, %p : f32
      affine.store %s, %C[%i, %j] : memref<8x8xf32>
    }
  }
  return
}

// -----
// Rank 3 with a fixed middle dimension (A[i][2][k]) and a rank-3 C whose
// leading dimension is fixed (C[1][i][j]): rank-reducing subviews.
// CHECK-LABEL: func.func @rank3
// CHECK:       memref.subview %arg0[0, 2, 0] [8, 1, 8] [1, 1, 1] : memref<8x3x8xf32> to memref<8x8xf32, strided<[24, 1], offset: 16>>
// CHECK:       memref.subview %arg2[1, 0, 0] [1, 8, 8] [1, 1, 1] : memref<2x8x8xf32> to memref<8x8xf32, strided<[8, 1], offset: 64>>
// CHECK:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 8
// REMARK:      remark: affine GEMM 8x8x8 lowered as 1 exact region(s)
func.func @rank3(%A: memref<8x3x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<2x8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i, 2, %k] : memref<8x3x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x8xf32>
        %c = affine.load %C[1, %i, %j] : memref<2x8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[1, %i, %j] : memref<2x8x8xf32>
      }
    }
  }
  return
}

// -----
// Folded constant bounds (P0.6): max(0, -3) to min(8, 16) is 0 to 8.
// CHECK-LABEL: func.func @minmax_bounds
// CHECK:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 8
// REMARK:      remark: affine GEMM 8x8x8 lowered as 1 exact region(s)
func.func @minmax_bounds(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = max affine_map<() -> (0, -3)>() to min affine_map<() -> (8, 16)>() {
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
// Folded bounds of a stepped loop (P0.6): k1 = max(-2, 0) to 8 step 4 is
// k1 = 0, 4, and K = k1 + k2 is 0 .. 7. The source domain needs the stride
// (k1 - 0) mod 4 = 0 from the folded bound: MLIR's own domain keeps a stride
// only for a lower bound of one constant, and with every k1 in [0, 7] the
// regions did not cover it -- the nest was refused ("the partition is not
// exact: the regions do not cover the source iteration domain").
// CHECK-LABEL: func.func @minmax_bounds_split_k
// CHECK:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 8
// REMARK:      remark: affine GEMM 8x8x8 lowered as 1 exact region(s)
func.func @minmax_bounds_split_k(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k1 = max affine_map<() -> (-2, 0)>() to 8 step 4 {
        affine.for %k2 = 0 to 4 {
          %a = affine.load %A[%i, %k1 + %k2] : memref<8x8xf32>
          %b = affine.load %B[%k1 + %k2, %j] : memref<8x8xf32>
          %c = affine.load %C[%i, %j] : memref<8x8xf32>
          %p = arith.mulf %a, %b : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%i, %j] : memref<8x8xf32>
        }
      }
    }
  }
  return
}

// -----
// The permission in the IR: the accumulation carries `reassoc` (what
// systolic-cfront --fp-reassociation emits). Lowered even by default.
// CHECK-LABEL: func.func @reassoc_flag
// CHECK:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 8
// REMARK:      remark: affine GEMM 8x8x8 lowered as 1 exact region(s)
// DISC:        reduction: reassoc permitted
func.func @reassoc_flag(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i, %k] : memref<8x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x8xf32>
        %c = affine.load %C[%i, %j] : memref<8x8xf32>
        %p = arith.mulf %a, %b fastmath<reassoc> : f32
        %s = arith.addf %c, %p fastmath<reassoc> : f32
        affine.store %s, %C[%i, %j] : memref<8x8xf32>
      }
    }
  }
  return
}

// -----
// Aliasing: A and C one memref without restrict -- A may overlap C (A's
// region rows 8..15 is in fact disjoint from C's 0..7, but that is not
// analysed): refused, UNKNOWN (the premise of section 7 is not established).
// CHECK-LABEL: func.func @alias
// CHECK:       affine.for
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      remark: affine GEMM not lowered to systolic.matmul_tile: A may overlap C; declare the arguments llvm.noalias (C restrict) or pass assume-no-alias [UNKNOWN]
func.func @alias(%A: memref<16x8xf32>, %B: memref<8x8xf32>) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i + 8, %k] : memref<16x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x8xf32>
        %c = affine.load %A[%i, %j] : memref<16x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %A[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}

// -----
// A batch coordinate outside the leading dimensions (A[i][p][k], B[p][k][j],
// C[p][i][j]): a batched GEMM, lowered since ISSUE-006 (2026-10-15) with the
// batch loop re-emitted around the tiles; A's batch dimension is its middle
// one (a dynamic offset there).
// CHECK-LABEL: func.func @batch_inner
// CHECK:       affine.for %[[P:.*]] = 0 to 2 {
// CHECK:         %[[IDX:.*]] = affine.apply #{{.*}}(%[[P]])
// CHECK:         memref.subview %arg0[0, %[[IDX]], 0] [4, 1, 4] [1, 1, 1]
// CHECK:         memref.subview %arg1[%[[IDX]], 0, 0] [1, 4, 4] [1, 1, 1]
// CHECK:         memref.subview %arg2[%[[IDX]], 0, 0] [1, 4, 4] [1, 1, 1]
// CHECK:         systolic.matmul_tile {{.*}} m = 4 n = 4 k = 4
// CHECK-NOT:   affine.for
// REMARK:      remark: affine GEMM 4x4x4 lowered as 1 exact region(s), batched: 2 batch iteration(s) of 1 batch loop(s); the enclosing loops run it 2 time(s)
// DISC:        dense logical GEMM: yes, I/J/K on loops 1/2/3, batched over loop(s) 0 (2 batch iteration(s))
func.func @batch_inner(%A: memref<4x2x4xf32> {llvm.noalias}, %B: memref<2x4x4xf32> {llvm.noalias}, %C: memref<2x4x4xf32> {llvm.noalias}) {
  affine.for %p = 0 to 2 {
    affine.for %i = 0 to 4 {
      affine.for %j = 0 to 4 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%i, %p, %k] : memref<4x2x4xf32>
          %b = affine.load %B[%p, %k, %j] : memref<2x4x4xf32>
          %c = affine.load %C[%p, %i, %j] : memref<2x4x4xf32>
          %m = arith.mulf %a, %b : f32
          %s = arith.addf %c, %m : f32
          affine.store %s, %C[%p, %i, %j] : memref<2x4x4xf32>
        }
      }
    }
  }
  return
}
// REMARK-NOT:  remark:
