// Domain normalization (docs/AFFINE_GEMM_MAPPING.md sections 19.7 and 19.12,
// 2026-10-18): a perfect multiply-accumulate band whose loop bounds form a
// constant-extent parallelotope -- every loop's lower and upper bound the same
// linear form of the outer counters plus constants -- is rewritten into the
// equivalent box nest x = U x' (U lower unitriangular: a bijection of Z^n that
// preserves the lexicographic order), and the box pipeline runs on it. Every
// other non-box domain (an extent that varies with an outer counter, a
// floordiv bound, a symbol, max/min of a counter and a constant) is untouched
// and stays UNKNOWN, never PROVEN_INVALID.
//
// RUN: systolic-opt "--affine-matmul-to-systolic=allow-fp-reassociation=true" \
// RUN:   -split-input-file %s | FileCheck %s
// RUN: systolic-opt "--affine-matmul-to-systolic=report=true allow-fp-reassociation=true classify=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=REMARK
// RUN: systolic-opt --test-affine-matmul-discovery -split-input-file %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s --check-prefix=DISC

// j in [i, i + 4) with C[i][j - i], B[k][j - i]: j = j' + i, the plain 4 x 4 x 4
// GEMM on the box (section 19.7's example family).
// CHECK-LABEL: func.func @shift_j
// CHECK:       systolic.matmul_tile {{.*}} m = 4 n = 4 k = 4
// REMARK:      iteration domain normalized to a box (x = U x', docs/AFFINE_GEMM_MAPPING.md section 19.12): loop 1: x1 = x1' + 1*x0, box [0, 4) step 1
// REMARK:      affine GEMM 4x4x4 lowered as 1 exact region(s)
// DISC:        domain normalization: 1 nest(s)
// DISC-NEXT:   loop 1: x1 = x1' + 1*x0, box [0, 4) step 1
// DISC:        exact mapping (task of 2026-10-17, over the counters of the nest as analysed): 3-D, T = {{\[\[}}1, 0, 0], [0, 1, 0], [0, 0, 1]], d = [0, 0, 0], det 1, unimodular, T diag(step) a signed permutation: yes
// DISC:        dense logical GEMM: yes, I/J/K on loops 0/1/2
// DISC-NEXT:   dense extents: M 4, N 4, K 4
func.func @shift_j(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = affine_map<(d0) -> (d0)>(%i) to affine_map<(d0) -> (d0 + 4)>(%i) {
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
// The shifted reduction t in [i, i + 4) (section 19.7): t = t' + i, K = t'.
// CHECK-LABEL: func.func @shift_k
// CHECK:       systolic.matmul_tile {{.*}} m = 4 n = 4 k = 4
// REMARK:      loop 2: x2 = x2' + 1*x0, box [0, 4) step 1
// REMARK:      affine GEMM 4x4x4 lowered as 1 exact region(s)
// DISC:        domain normalization: 1 nest(s)
// DISC-NEXT:   loop 2: x2 = x2' + 1*x0, box [0, 4) step 1
func.func @shift_k(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %t = affine_map<(d0) -> (d0)>(%i) to affine_map<(d0) -> (d0 + 4)>(%i) {
        %a = affine.load %A[%i, %t - %i] : memref<4x4xf32>
        %b = affine.load %B[%t - %i, %j] : memref<4x4xf32>
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
// A skewed C on the parallelotope: j in [i, i + 4) with C[i][j] -- after the
// shift C[i][j' + i], the skewed view of section 19.11 (strides (8, 1) on the
// 4 x 7 array): the role box is complete; in the task's C-identity coordinates
// T = [[1,0,0],[1,1,0],[0,0,1]] is unimodular but its image is a parallelogram.
// CHECK-LABEL: func.func @shift_skewed_c
// CHECK:       memref.reinterpret_cast %arg2 to offset: [0], sizes: [4, 4], strides: [8, 1]
// CHECK:       systolic.matmul_tile {{.*}} m = 4 n = 4 k = 4
// REMARK:      affine GEMM 4x4x4 lowered as 1 exact region(s)
// DISC:        exact mapping (task of 2026-10-17, over the counters of the nest as analysed): 3-D, T = {{\[\[}}1, 0, 0], [1, 1, 0], [0, 0, 1]], d = [0, 0, 0], det 1, unimodular, T diag(step) a signed permutation: no
func.func @shift_skewed_c(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x7xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = affine_map<(d0) -> (d0)>(%i) to affine_map<(d0) -> (d0 + 4)>(%i) {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<4x4xf32>
        %b = affine.load %B[%k, %j - %i] : memref<4x4xf32>
        %c = affine.load %C[%i, %j] : memref<4x7xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<4x7xf32>
      }
    }
  }
  return
}

// -----
// Coefficient 2 and a step: j in [2i, 2i + 8) step 2 with C[i][j - 2i]: j = j' + 2 i,
// j' in [0, 8) step 2, stride-2 views along J.
// CHECK-LABEL: func.func @shift_two
// CHECK:       memref.subview %arg1[0, 0] [4, 4] [1, 2]
// CHECK:       systolic.matmul_tile {{.*}} m = 4 n = 4 k = 4
// REMARK:      loop 1: x1 = x1' + 2*x0, box [0, 8) step 2
// REMARK:      affine GEMM 4x4x4 lowered as 1 exact region(s)
func.func @shift_two(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x8xf32> {llvm.noalias}, %C: memref<4x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = affine_map<(d0) -> (2 * d0)>(%i) to affine_map<(d0) -> (2 * d0 + 8)>(%i) step 2 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<4x4xf32>
        %b = affine.load %B[%k, %j - 2 * %i] : memref<4x8xf32>
        %c = affine.load %C[%i, %j - 2 * %i] : memref<4x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j - 2 * %i] : memref<4x8xf32>
      }
    }
  }
  return
}

// -----
// Two chained shifts, j in [i, i + 4) and k in [j, j + 2): U = [[1,0,0],[1,1,0],[1,1,1]]
// (the remark states each loop's own shift, k = k' + j; U composes them, x2 = x2' + x0' + x1');
// on the box every loop of the body is read by two operands -- I, J and a batch
// loop k' (K = 1 per batch): the batched rank-1 updates of affine_gemm_generalized
// @parallelotope_rank1, here with the K extent read from the dense partition.
// CHECK-LABEL: func.func @shift_chain
// CHECK:       systolic.matmul_tile {{.*}} m = 4 n = 4 k = 1
// REMARK:      loop 1: x1 = x1' + 1*x0, box [0, 4) step 1
// REMARK-NEXT: loop 2: x2 = x2' + 1*x1, box [0, 2) step 1
// DISC:        dense extents: M 4, N 4, K 1
func.func @shift_chain(%A: memref<4x2xf32> {llvm.noalias}, %B: memref<4x2xf32> {llvm.noalias}, %C: memref<4x4x2xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = affine_map<(d0) -> (d0)>(%i) to affine_map<(d0) -> (d0 + 4)>(%i) {
      affine.for %k = affine_map<(d0) -> (d0)>(%j) to affine_map<(d0) -> (d0 + 2)>(%j) {
        %a = affine.load %A[%i, %k - %j] : memref<4x2xf32>
        %bb = affine.load %B[%j - %i, %k - %j] : memref<4x2xf32>
        %c = affine.load %C[%i, %j - %i, %k - %j] : memref<4x4x2xf32>
        %p = arith.mulf %a, %bb : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j - %i, %k - %j] : memref<4x4x2xf32>
      }
    }
  }
  return
}

// -----
// K = 1 on a parallelotope: j in [i, i + 4) with constant reduction subscripts.
// The degenerate path: the 2-D exact mapping, no reduction direction, lowered 4 x 4 x 1.
// CHECK-LABEL: func.func @shift_k1
// CHECK:       systolic.matmul_tile {{.*}} m = 4 n = 4 k = 1
// REMARK:      affine GEMM 4x4x1 lowered as 1 exact region(s)
// DISC:        exact mapping (task of 2026-10-17, over the counters of the nest as analysed): 2-D (K = 1: a degenerate reduction, the non-degenerate rule does not apply), T = {{\[\[}}1, 0], [0, 1]], d = [0, 0], det 1, unimodular, T diag(step) a signed permutation: yes
// DISC-NEXT:   no reduction direction: K has no loop
// DISC:        dense extents: M 4, N 4, K 1
func.func @shift_k1(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = affine_map<(d0) -> (d0)>(%i) to affine_map<(d0) -> (d0 + 4)>(%i) {
      %a = affine.load %A[%i, 0] : memref<4x4xf32>
      %b = affine.load %B[0, %j - %i] : memref<4x4xf32>
      %c = affine.load %C[%i, %j - %i] : memref<4x4xf32>
      %p = arith.mulf %a, %b : f32
      %s = arith.addf %c, %p : f32
      affine.store %s, %C[%i, %j - %i] : memref<4x4xf32>
    }
  }
  return
}

// -----
// NOT IN THE FAMILY: the extent varies with the outer counter (a triangle,
// j in [i, 8)): untouched, UNKNOWN (showcase mlir_009: provably not a GEMM by
// 19.7's symmetry argument, which the compiler does not make).
// CHECK-LABEL: func.func @triangle
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      affine GEMM not lowered to systolic.matmul_tile: the loop bounds are not static, so the iteration domain has no static normalized extent (loop 1's lower bound depends on another loop's counter: the iteration domain is not a box) [UNKNOWN]
// DISC-NOT:    domain normalization
// DISC:        class: UNKNOWN
func.func @triangle(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = affine_map<(d0) -> (d0)>(%i) to 8 {
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
// NOT IN THE FAMILY: a floordiv in a bound (j in [i floordiv 2, i floordiv 2 + 4)):
// a real-space shift by i/2 that is no integer map; untouched, UNKNOWN.
// CHECK-LABEL: func.func @floordiv_bound
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      [UNKNOWN]
// DISC-NOT:    domain normalization
// DISC:        class: UNKNOWN
func.func @floordiv_bound(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x6xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = affine_map<(d0) -> (d0 floordiv 2)>(%i) to affine_map<(d0) -> (d0 floordiv 2 + 4)>(%i) {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<4x4xf32>
        %b = affine.load %B[%k, %j] : memref<4x4xf32>
        %c = affine.load %C[%i, %j] : memref<4x6xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<4x6xf32>
      }
    }
  }
  return
}

// -----
// NOT IN THE FAMILY: a symbolic bound; untouched, UNKNOWN (SQ-3).
// CHECK-LABEL: func.func @symbol_bound
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      [UNKNOWN]
// DISC-NOT:    domain normalization
func.func @symbol_bound(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}, %n: index) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to %n {
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
// NOT IN THE FAMILY: max of a counter and a constant (a clipped parallelogram);
// untouched, UNKNOWN.
// CHECK-LABEL: func.func @max_bound
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      [UNKNOWN]
// DISC-NOT:    domain normalization
func.func @max_bound(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = max affine_map<(d0) -> (d0 - 2, 0)>(%i) to affine_map<(d0) -> (d0 + 2)>(%i) {
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
// IN THE FAMILY BUT NOT A GEMM: j in [i, i + 4) with C[i + j] (1-D C): after the
// shift C[2i + j'] -- two outputs collide ((i, j') = (0, 2) and (1, 0)), G2 fails:
// VALID_BUT_LOWERING_UNSUPPORTED (a batched GEMM by demotion, section 19.3),
// not lowered. Normalization never turns an invalid nest into a lowered one.
// (The lowering pass is silent on a recognizer refusal; the class is in DISC.)
// CHECK-LABEL: func.func @shift_c_not_injective
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      loop 1: x1 = x1' + 1*x0, box [0, 4) step 1
// REMARK-NOT:  lowered as
// DISC:        domain normalization: 1 nest(s)
// DISC:        reason: C's view is not injective on the (I, J) box
// DISC:        class: VALID_BUT_LOWERING_UNSUPPORTED
func.func @shift_c_not_injective(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<16xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = affine_map<(d0) -> (d0)>(%i) to affine_map<(d0) -> (d0 + 4)>(%i) {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<4x4xf32>
        %b = affine.load %B[%k, %j - %i] : memref<4x4xf32>
        %c = affine.load %C[%i + %j] : memref<16xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i + %j] : memref<16xf32>
      }
    }
  }
  return
}

// -----
// IN THE FAMILY BUT OUT OF BOUNDS: j in [i, i + 4) with C[i][j] on a 4 x 4 C --
// column j reaches 6: PROVEN_INVALID after the shift (G3), not lowered.
// CHECK-LABEL: func.func @shift_oob
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      C is accessed out of bounds along dimension 1 [PROVEN_INVALID]
// DISC:        domain normalization: 1 nest(s)
func.func @shift_oob(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = affine_map<(d0) -> (d0)>(%i) to affine_map<(d0) -> (d0 + 4)>(%i) {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<4x4xf32>
        %b = affine.load %B[%k, %j - %i] : memref<4x4xf32>
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
// NOT NORMALIZED: an access reads a symbol (%n) under shifted bounds. The rewrite
// of the accesses needs every operand to be a counter of the band, so the nest is
// left as written and the recognizer says what it said before normalization existed:
// UNKNOWN (the symbol), not PROVEN_INVALID; the lowering pass is silent on it.
// CHECK-LABEL: func.func @shift_symbol_access
// CHECK-NOT:   systolic.matmul_tile
// REMARK-NOT:  lowered as
// DISC-NOT:    domain normalization
// DISC:        reason: an access index is not a counter of the nest's loops, or an access map has symbols
// DISC-NEXT:   class: UNKNOWN
func.func @shift_symbol_access(%A: memref<4x8xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}, %n: index) {
  affine.for %i = 0 to 4 {
    affine.for %j = affine_map<(d0) -> (d0)>(%i) to affine_map<(d0) -> (d0 + 4)>(%i) {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k + symbol(%n)] : memref<4x8xf32>
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
// A REPETITION LOOP BETWEEN the shift's source and the shifted loop: r (2 iterations,
// read by no access) sits between i and j in [i, i + 4). The rewritten accesses name
// only the counters they read, so r stays a repetition loop: moved outward (a legal
// interchange: no dependence is carried by i), the GEMM is lowered and run twice.
// CHECK-LABEL: func.func @shift_over_repetition
// CHECK:       systolic.matmul_tile {{.*}} m = 4 n = 4 k = 4
// REMARK:      loop 2: x2 = x2' + 1*x0, box [0, 4) step 1
// REMARK:      affine GEMM 4x4x4 lowered as 1 exact region(s); the enclosing loops run it 2 time(s)
// DISC:        domain normalization: 1 nest(s)
// DISC-NEXT:   loop 2: x2 = x2' + 1*x0, box [0, 4) step 1
func.func @shift_over_repetition(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %r = 0 to 2 {
      affine.for %j = affine_map<(d0) -> (d0)>(%i) to affine_map<(d0) -> (d0 + 4)>(%i) {
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
  }
  return
}

// -----
// A REPETITION LOOP THAT CANNOT MOVE: r between i and j in [i, i + 4) with C[i + j].
// After the shift C[2i + j'] is written by several i iterations ((0, 2) and (1, 0)),
// so moving r ahead of i would reorder dependent iterations: the nest keeps r inside
// it, and the recognizer, seeing a loop no access uses, says UNKNOWN -- not
// PROVEN_INVALID, which an r named by the rewritten accesses would have produced.
// CHECK-LABEL: func.func @shift_repetition_stuck
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      loop 2: x2 = x2' + 1*x0, box [0, 4) step 1
// REMARK:      loop 1 of the nest repeats the accumulation inside it, and moving it outward is not a legal loop interchange
// DISC:        domain normalization: 1 nest(s)
// DISC:        class: UNKNOWN
func.func @shift_repetition_stuck(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<16xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %r = 0 to 2 {
      affine.for %j = affine_map<(d0) -> (d0)>(%i) to affine_map<(d0) -> (d0 + 4)>(%i) {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%i, %k] : memref<4x4xf32>
          %b = affine.load %B[%k, %j - %i] : memref<4x4xf32>
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
