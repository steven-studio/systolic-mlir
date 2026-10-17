// The generalized affine GEMM mapping (docs/AFFINE_GEMM_MAPPING.md section 19): the
// readers analysis, C's injectivity per batch (G2), SQ-5 decided, and the three specified
// tests. What each case is, mathematically, is in its comment; the layouts and reasons
// below are the recognizer's (DISC), the remarks the lowering's (REMARK). Skewed and
// merged views are recognized and verified (G1, G2); their lowering (a
// memref.reinterpret_cast view) is the next phase, so they are refused here as
// VALID_BUT_LOWERING_UNSUPPORTED.
//
// RUN: systolic-opt "--affine-matmul-to-systolic=allow-fp-reassociation=true" \
// RUN:   -split-input-file %s | FileCheck %s
// RUN: systolic-opt "--affine-matmul-to-systolic=report=true allow-fp-reassociation=true classify=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=REMARK
// RUN: systolic-opt --test-affine-matmul-discovery -split-input-file %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s --check-prefix=DISC

// TEST 1: C[i][j] += A[i][i + k] * B[j + k][j]. Readers: i (A, C) = I, j (B, C) = J,
// k (A, B) = K. tau = (i, j, k), a bijection onto the 4 x 4 x 4 box; the views
// A'(I, K) = A[I][I + K] and B'(K, J) = B[J + K][J] are offset by the output coordinate
// (skewed: A's dimension 1 combines I and K, B's dimension 0 K and J); Ka != Kb
// physically, the same K at every iteration; C'(I, J) = C[I][J] is injective; A's
// columns reach 6 < 7, B's rows 6 < 7. A GEMM -- refused before section 19 ("reads one
// logical coordinate in two of its dimensions").
// CHECK-LABEL: func.func @test1
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      remark: affine GEMM not lowered to systolic.matmul_tile: A has a skewed or merged view (a dimension combining both of its roles, or a role across several dimensions: docs/AFFINE_GEMM_MAPPING.md section 19), which is not lowered yet [VALID_BUT_LOWERING_UNSUPPORTED]
// DISC:        recognition result: success
// DISC:        layout A: general view: dimension 0 = 0 + 1*I; dimension 1 = 0 + 1*I + 1*K
// DISC-NEXT:   layout B: general view: dimension 0 = 0 + 1*K + 1*J; dimension 1 = 0 + 1*J
// DISC:        dense logical GEMM: yes, I/J/K on loops 0/1/2
func.func @test1(%A: memref<4x7xf32> {llvm.noalias}, %B: memref<7x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %i + %k] : memref<4x7xf32>
        %bb = affine.load %B[%j + %k, %j] : memref<7x4xf32>
        %c = affine.load %C[%i, %j] : memref<4x4xf32>
        %p = arith.mulf %a, %bb : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<4x4xf32>
      }
    }
  }
  return
}
// -----
// TEST 2 on a box (i in [2,10), j in [0,6), k in [0,2): every index >= 0):
// C[i+j-2k][i+2j+k] += A[i+j-2k][i+j+k] * B[i+j+k][i+2j+k]. Every loop is read by A, B
// and C -- all three are batch loops: no row, column or reduction coordinate. The
// direction keeping C fixed, d = (5,-3,1), is no loop direction (the six-vector
// conditions hold, affine_gemm_geometry.mlir): the 96 iterations write 87 outputs, 9 of
// them twice, from different batch iterations. Not a GEMM (section 19.6).
// CHECK-LABEL: func.func @test2_box
// CHECK-NOT:   systolic.matmul_tile
// DISC:        reason: 1 coordinate(s) are read by all three operands and none by two: an element-wise product
// DISC-NEXT:   class: PROVEN_INVALID
func.func @test2_box(%A: memref<15x16xf32> {llvm.noalias}, %B: memref<16x21xf32> {llvm.noalias}, %C: memref<15x21xf32> {llvm.noalias}) {
  affine.for %i = 2 to 10 {
    affine.for %j = 0 to 6 {
      affine.for %k = 0 to 2 {
        %a = affine.load %A[%i + %j - %k * 2, %i + %j + %k] : memref<15x16xf32>
        %bb = affine.load %B[%i + %j + %k, %i + %j * 2 + %k] : memref<16x21xf32>
        %c = affine.load %C[%i + %j - %k * 2, %i + %j * 2 + %k] : memref<15x21xf32>
        %p = arith.mulf %a, %bb : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i + %j - %k * 2, %i + %j * 2 + %k] : memref<15x21xf32>
      }
    }
  }
  return
}
// -----
// TEST 2 on the lattice domain D* (section 19.6): with u_I = (1,0,-1), u_J = (-1,1,0),
// d = (5,-3,1) and x0 = (5,-1,1), D* = {x0 + a u_I + J' u_J + b d}, (a, J', b) in
// [0,2)^3. On D* the subscripts are Ic = 2 + 3a, Jc = 4 + J', Ka = Kb = 5 + 3b, so the
// nest over D* performs exactly the updates of this program over (a, J', b) -- the
// reparametrization is a bijection (det [u_I u_J d] = 3, injective; its image is D*).
// D* itself needs a congruence (i + j + k = 5 mod 3) in its bounds, which an affine.for
// chain of constant-extent bounds does not express; the reparametrized program is a GEMM
// of stride-3 views, lowered as one 2 x 2 x 2 tile; the reduction direction is d.
// CHECK-LABEL: func.func @test2_reparametrized
// CHECK:       memref.subview %arg0[2, 5] [2, 2] [3, 3]
// CHECK:       memref.subview %arg1[5, 4] [2, 2] [3, 1]
// CHECK:       memref.subview %arg2[2, 4] [2, 2] [3, 1]
// CHECK:       systolic.matmul_tile {{.*}} m = 2 n = 2 k = 2
// REMARK:      remark: affine GEMM 2x2x2 lowered as 1 exact region(s)
// DISC:        T[0] = [3 0 0]
// DISC-NEXT:   T[1] = [0 1 0]
// DISC-NEXT:   T[2] = [0 0 3]
func.func @test2_reparametrized(%A: memref<9x11xf32> {llvm.noalias}, %B: memref<11x6xf32> {llvm.noalias}, %C: memref<9x6xf32> {llvm.noalias}) {
  affine.for %ra = 0 to 2 {
    affine.for %rj = 0 to 2 {
      affine.for %rb = 0 to 2 {
        %a = affine.load %A[%ra * 3 + 2, %rb * 3 + 5] : memref<9x11xf32>
        %bb = affine.load %B[%rb * 3 + 5, %rj + 4] : memref<11x6xf32>
        %c = affine.load %C[%ra * 3 + 2, %rj + 4] : memref<9x6xf32>
        %p = arith.mulf %a, %bb : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%ra * 3 + 2, %rj + 4] : memref<9x6xf32>
      }
    }
  }
  return
}
// -----
// TEST 3: C[i][j] += A[i][j + k] * B[i + k][j]. i and j are read by all three operands
// (batch loops), k by both factors (the reduction). Per (i, j) the dot product
// C[i][j] += sum_K A[i][j + K] * B[i + K][j], with the views A'(0, K) = A[i][j + K] and
// B'(K, 0) = B[i + K][j], offsets affine in (i, j). A batched GEMM with M = N = 1 (SQ-5,
// decided 2026-10-16: lowered, one 1 x 1 x 4 tile per (i, j)). Not one GEMM: no
// decomposition of the box gives it an I or a J (section 19.6).
// CHECK-LABEL: func.func @test3
// CHECK:       affine.for %{{.*}} = 0 to 4 {
// CHECK:         affine.for %{{.*}} = 0 to 4 {
// CHECK:           systolic.matmul_tile {{.*}} m = 1 n = 1 k = 4
// REMARK:      remark: affine GEMM 1x1x4 lowered as 1 exact region(s), batched: 16 batch iteration(s) of 2 batch loop(s)
// DISC:        batch terms A: dimension 0 = 0 + 1*x0, dimension 1 += 1*x1
// DISC-NEXT:   batch terms B: dimension 0 += 1*x0, dimension 1 = 0 + 1*x1
// DISC:        dense logical GEMM: yes, I/J/K on loops //2, batched over loop(s) 0+1 (16 batch iteration(s))
func.func @test3(%A: memref<4x7xf32> {llvm.noalias}, %B: memref<7x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %j + %k] : memref<4x7xf32>
        %bb = affine.load %B[%i + %k, %j] : memref<7x4xf32>
        %c = affine.load %C[%i, %j] : memref<4x4xf32>
        %p = arith.mulf %a, %bb : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<4x4xf32>
      }
    }
  }
  return
}
// -----
// A skewed output view, C[i + j][j] += A[i][k] * B[k][j]: C'(I, J) = C[I + J][J], injective
// (rank 2). A GEMM (section 19.3); refused before as "C reads one logical coordinate in
// two of its dimensions". (case_087 is the different nest with A[i + j][k].)
// CHECK-LABEL: func.func @skewed_c
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      remark: affine GEMM not lowered to systolic.matmul_tile: C has a skewed or merged view
// DISC:        layout C: general view: dimension 0 = 0 + 1*I + 1*J; dimension 1 = 0 + 1*J
func.func @skewed_c(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<7x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<4x4xf32>
        %bb = affine.load %B[%k, %j] : memref<4x4xf32>
        %c = affine.load %C[%i + %j, %j] : memref<7x4xf32>
        %p = arith.mulf %a, %bb : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i + %j, %j] : memref<7x4xf32>
      }
    }
  }
  return
}
// -----
// Flat 1-D arrays, the C idiom C[i*4 + j] += A[i*4 + k] * B[k*4 + j]: every operand reads
// both of its roles in its one dimension (merged views); C[4I + J] is injective on the
// 4 x 4 box. A GEMM; refused before as a coupled mapping.
// CHECK-LABEL: func.func @flat
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      remark: affine GEMM not lowered to systolic.matmul_tile: A has a skewed or merged view
// DISC:        layout A: general view: dimension 0 = 0 + 4*I + 1*K
// DISC-NEXT:   layout B: general view: dimension 0 = 0 + 4*K + 1*J
// DISC-NEXT:   layout C: general view: dimension 0 = 0 + 4*I + 1*J
func.func @flat(%A: memref<16xf32> {llvm.noalias}, %B: memref<16xf32> {llvm.noalias}, %C: memref<16xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i * 4 + %k] : memref<16xf32>
        %bb = affine.load %B[%k * 4 + %j] : memref<16xf32>
        %c = affine.load %C[%i * 4 + %j] : memref<16xf32>
        %p = arith.mulf %a, %bb : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i * 4 + %j] : memref<16xf32>
      }
    }
  }
  return
}
// -----
// A Hankel view, C[i][j] += A[i + k] * B[k][j] (1-D A): A'(I, K) = A[I + K] repeats
// elements (not injective -- a read view may overlap itself). A GEMM.
// CHECK-LABEL: func.func @hankel
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      remark: affine GEMM not lowered to systolic.matmul_tile: A has a skewed or merged view
// DISC:        layout A: general view: dimension 0 = 0 + 1*I + 1*K
func.func @hankel(%A: memref<7xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i + %k] : memref<7xf32>
        %bb = affine.load %B[%k, %j] : memref<4x4xf32>
        %c = affine.load %C[%i, %j] : memref<4x4xf32>
        %p = arith.mulf %a, %bb : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<4x4xf32>
      }
    }
  }
  return
}
// -----
// MUST BE REFUSED (G2): C[i*4 + j] with j in [0,5): (I, J) = (0, 4) and (1, 0) both write
// C[4]; one tile would lose an update. (With j as a batch loop it would be a valid batched
// GEMV, which the recognizer does not construct.)
// CHECK-LABEL: func.func @flat_not_injective
// CHECK-NOT:   systolic.matmul_tile
// DISC:        reason: C's view is not injective on the (I, J) box: the outputs with ordinals (I, J) and (I + 1, J + -4) write the same element of C
// DISC-NEXT:   class: VALID_BUT_LOWERING_UNSUPPORTED
func.func @flat_not_injective(%A: memref<16xf32> {llvm.noalias}, %B: memref<20xf32> {llvm.noalias}, %C: memref<20xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i * 4 + %k] : memref<16xf32>
        %bb = affine.load %B[%k * 5 + %j] : memref<20xf32>
        %c = affine.load %C[%i * 4 + %j] : memref<20xf32>
        %p = arith.mulf %a, %bb : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i * 4 + %j] : memref<20xf32>
      }
    }
  }
  return
}
// -----
// MUST BE REFUSED (SQ-1, open): A[4 i0 + i1] with C[8 i0 + i1] -- the restrictions of A's
// and C's subscripts to the I loops are not proportional: no single coordinate I.
// CHECK-LABEL: func.func @sq1
// CHECK-NOT:   systolic.matmul_tile
// DISC:        reason: logical coordinate I (the first factor's dimension 0 d0 * 4 + d1, C's dimension 0 d0 * 8 + d1): the two subscripts are not multiples of one linear form
// DISC-NEXT:   class: UNKNOWN
func.func @sq1(%A: memref<8x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<16x4xf32> {llvm.noalias}) {
  affine.for %i0 = 0 to 2 {
    affine.for %i1 = 0 to 4 {
      affine.for %j = 0 to 4 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%i0 * 4 + %i1, %k] : memref<8x4xf32>
          %bb = affine.load %B[%k, %j] : memref<4x4xf32>
          %c = affine.load %C[%i0 * 8 + %i1, %j] : memref<16x4xf32>
          %p = arith.mulf %a, %bb : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%i0 * 8 + %i1, %j] : memref<16x4xf32>
        }
      }
    }
  }
  return
}
// -----
// MUST BE REFUSED: C[i0][i1][j] with A[i0][i1][k] -- the I loops i0 and i1 are read
// through two different dimensions: two coordinates in one role (a merged, reshaped
// view would be needed).
// CHECK-LABEL: func.func @separate_coordinates
// CHECK-NOT:   systolic.matmul_tile
// DISC:        reason: 2 separate coordinates are read by the first factor and C
// DISC-NEXT:   class: VALID_BUT_LOWERING_UNSUPPORTED
func.func @separate_coordinates(%A: memref<2x4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<2x4x4xf32> {llvm.noalias}) {
  affine.for %i0 = 0 to 2 {
    affine.for %i1 = 0 to 4 {
      affine.for %j = 0 to 4 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%i0, %i1, %k] : memref<2x4x4xf32>
          %bb = affine.load %B[%k, %j] : memref<4x4xf32>
          %c = affine.load %C[%i0, %i1, %j] : memref<2x4x4xf32>
          %p = arith.mulf %a, %bb : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%i0, %i1, %j] : memref<2x4x4xf32>
        }
      }
    }
  }
  return
}
// -----
// MUST BE REFUSED (non-degeneracy, section 19.1): a batched product with M = N = K = 1.
// The positional form reads C[b][1][2] += A[b][1][3] * B[b][3][2] (b the prefix, distinct
// constants as I, J, K); the dense partition refuses it as element-wise -- every loop is a
// batch loop (the analyses by groups and by readers refuse the same nest the same way,
// @test2_box). Lowered by mistake between the SQ-5 decision and 2026-10-18 (review finding).
// CHECK-LABEL: func.func @elementwise_positional
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      remark: affine GEMM not lowered to systolic.matmul_tile: every loop is read by all three operands: a batched product with M = N = K = 1 is an element-wise product, not a GEMM (docs/AFFINE_GEMM_MAPPING.md section 19.1) [PROVEN_INVALID]
// DISC:        dense logical GEMM: no (every loop is read by all three operands: a batched product with M = N = K = 1 is an element-wise product
// DISC-NEXT:   class: PROVEN_INVALID
func.func @elementwise_positional(%A: memref<4x2x4xf32> {llvm.noalias}, %B: memref<4x4x3xf32> {llvm.noalias}, %C: memref<4x2x3xf32> {llvm.noalias}) {
  affine.for %b = 0 to 4 {
    %a = affine.load %A[%b, 1, 3] : memref<4x2x4xf32>
    %bb = affine.load %B[%b, 3, 2] : memref<4x4x3xf32>
    %c = affine.load %C[%b, 1, 2] : memref<4x2x3xf32>
    %p = arith.mulf %a, %bb : f32
    %s = arith.addf %c, %p : f32
    affine.store %s, %C[%b, 1, 2] : memref<4x2x3xf32>
  }
  return
}
// -----
// The same with the role loops running once (constants on the domain, Lemma 0): refused
// the same way. (A 1 x 1 x 1 GEMM WITHOUT a batch loop stays a GEMM, SQ-4.)
// CHECK-LABEL: func.func @elementwise_positional_once
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      remark: affine GEMM not lowered to systolic.matmul_tile: every loop is read by all three operands: a batched product with M = N = K = 1 is an element-wise product, not a GEMM (docs/AFFINE_GEMM_MAPPING.md section 19.1) [PROVEN_INVALID]
// DISC:        dense logical GEMM: no (every loop is read by all three operands: a batched product with M = N = K = 1 is an element-wise product
func.func @elementwise_positional_once(%A: memref<4x4x8xf32> {llvm.noalias}, %B: memref<4x8x4xf32> {llvm.noalias}, %C: memref<4x4x4xf32> {llvm.noalias}) {
  affine.for %b = 0 to 4 {
    affine.for %i = 2 to 3 {
      affine.for %j = 3 to 4 {
        affine.for %k = 5 to 6 {
          %a = affine.load %A[%b, %i, %k] : memref<4x4x8xf32>
          %bb = affine.load %B[%b, %k, %j] : memref<4x8x4xf32>
          %c = affine.load %C[%b, %i, %j] : memref<4x4x4xf32>
          %p = arith.mulf %a, %bb : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%b, %i, %j] : memref<4x4x4xf32>
        }
      }
    }
  }
  return
}
// -----
// NOT DECIDED (section 19.7): Test 1 on the triangle j >= i. The readers in the original
// counters decide nothing on a domain that is not a box, so the readers analysis's refusal
// is UNKNOWN, not a claim of validity by demotion. (The triangle is provably not the affine
// image of a box -- 19.7 -- but that is the specification's proof, not the compiler's.)
// CHECK-LABEL: func.func @test1_triangle
// CHECK-NOT:   systolic.matmul_tile
// DISC:        reason: logical coordinate J is driven by 1 loops that do not enumerate it densely (or its loops' bounds are not constant){{.*}} -- on a domain that is not a box the readers decide nothing
// DISC-NEXT:   class: UNKNOWN
func.func @test1_triangle(%A: memref<8x11xf32> {llvm.noalias}, %B: memref<11x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = affine_map<(d0) -> (d0)>(%i) to 8 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %i + %k] : memref<8x11xf32>
        %bb = affine.load %B[%j + %k, %j] : memref<11x8xf32>
        %c = affine.load %C[%i, %j] : memref<8x8xf32>
        %p = arith.mulf %a, %bb : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<8x8xf32>
      }
    }
  }
  return
}
// -----
// NOT DECIDED (SQ-3 and section 19.7): Test 1 with a symbolic reduction bound.
// CHECK-LABEL: func.func @test1_symbolic
// CHECK-NOT:   systolic.matmul_tile
// DISC:        reason: logical coordinate K is driven by 1 loops that do not enumerate it densely (or its loops' bounds are not constant){{.*}} -- on a domain that is not a box the readers decide nothing
// DISC-NEXT:   class: UNKNOWN
func.func @test1_symbolic(%A: memref<4x?xf32> {llvm.noalias}, %B: memref<?x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}, %n: index) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to %n {
        %a = affine.load %A[%i, %i + %k] : memref<4x?xf32>
        %bb = affine.load %B[%j + %k, %j] : memref<?x4xf32>
        %c = affine.load %C[%i, %j] : memref<4x4xf32>
        %p = arith.mulf %a, %bb : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<4x4xf32>
      }
    }
  }
  return
}
// -----
// NOT DECIDED (section 19.7): a batched GEMM on a constant-extent parallelotope. With
// j in [i, i + 4) and k in [j, j + 2), xi(I, J, p) = (I, I + J, I + J + p) is a unimodular
// bijection from the box [0,4) x [0,4) x [0,2) and per p the nest is the 4 x 4 x 1 GEMM
// C[I][J][p] += A[I][p] * B[J][p]. In the original counters every loop is read by all three
// operands; on a box that would be element-wise (PROVEN_INVALID), here it is UNKNOWN: the
// parametrization is not constructed.
// CHECK-LABEL: func.func @parallelotope_rank1
// CHECK-NOT:   systolic.matmul_tile
// DISC:        reason: 1 coordinate(s) are read by all three operands and none by two: an element-wise product (a batched GEMM with M = N = K = 1), not a matrix multiplication -- on a domain that is not a box the readers decide nothing
// DISC-NEXT:   class: UNKNOWN
func.func @parallelotope_rank1(%A: memref<4x2xf32> {llvm.noalias}, %B: memref<4x2xf32> {llvm.noalias}, %C: memref<4x4x2xf32> {llvm.noalias}) {
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
// An empty role loop (k in [0, 0)) in a nest the readers analysis decides: no box to check
// the radixes or G2 on; recognized with its layouts and reported by the dense partition as
// the empty GEMM, as for a role-pure nest (SQ-2: refused, PROVEN_VALID).
// CHECK-LABEL: func.func @test1_empty
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      remark: affine GEMM not lowered to systolic.matmul_tile: a loop of the nest has no iteration (an empty GEMM: M, N or K is 0) [PROVEN_VALID]
// DISC:        layout A: general view: dimension 0 = 0 + 1*I; dimension 1 = 0 + 1*I + 1*K
// DISC:        dense logical GEMM: no (a loop of the nest has no iteration (an empty GEMM: M, N or K is 0))
// DISC-NEXT:   class: PROVEN_VALID
func.func @test1_empty(%A: memref<4x7xf32> {llvm.noalias}, %B: memref<7x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 0 {
        %a = affine.load %A[%i, %i + %k] : memref<4x7xf32>
        %bb = affine.load %B[%j + %k, %j] : memref<7x4xf32>
        %c = affine.load %C[%i, %j] : memref<4x4xf32>
        %p = arith.mulf %a, %bb : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<4x4xf32>
      }
    }
  }
  return
}
// -----
// A repetition loop inside the reduction: the lowering pass cannot move it outward (the
// interchange would reorder dependent iterations) and the recognizer, which does not
// analyse a nest with such a loop, labels its refusal UNKNOWN (not PROVEN_INVALID: the
// specification lists this as a documented limitation, README section 2).
// CHECK-LABEL: func.func @repetition_inside_reduction
// CHECK-NOT:   systolic.matmul_tile
// REMARK:      remark: affine GEMM not lowered to systolic.matmul_tile: loop 3 of the nest repeats the accumulation inside it, and moving it outward is not a legal loop interchange (it would reorder dependent iterations)
// DISC:        reason: loop 3 of the nest is used by no access (it repeats the accumulation; moved outward by the lowering pass when that is a legal interchange -- here it was not, so the mapping is not decided)
// DISC-NEXT:   class: UNKNOWN
func.func @repetition_inside_reduction(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        affine.for %r = 0 to 2 {
          %a = affine.load %A[%i, %k] : memref<4x4xf32>
          %bb = affine.load %B[%k, %j] : memref<4x4xf32>
          %c = affine.load %C[%i, %j] : memref<4x4xf32>
          %p = arith.mulf %a, %bb : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%i, %j] : memref<4x4xf32>
        }
      }
    }
  }
  return
}
// -----
// A repetition loop between the two loops of a matrix-vector product, C[i] += A[i][k] *
// B[k] with r in between: moved outward by the lowering pass (a band of three with two GEMM
// dimensions; until 2026-10-18 the hoist required a band of four with three dimensions and
// the nest stayed on the host). Lowered as a 4 x 1 x 4 GEMM run twice.
// CHECK-LABEL: func.func @matvec_repetition_between
// CHECK:       affine.for %{{.*}} = 0 to 2 {
// CHECK:         systolic.matmul_tile {{.*}} m = 4 n = 1 k = 4
// REMARK:      remark: affine GEMM 4x1x4 lowered as 1 exact region(s); the enclosing loops run it 2 time(s)
func.func @matvec_repetition_between(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4xf32> {llvm.noalias}, %C: memref<4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %r = 0 to 2 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<4x4xf32>
        %bb = affine.load %B[%k] : memref<4xf32>
        %c = affine.load %C[%i] : memref<4xf32>
        %p = arith.mulf %a, %bb : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i] : memref<4xf32>
      }
    }
  }
  return
}

// REMARK-NOT: remark:
