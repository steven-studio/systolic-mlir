// The geometric screening (docs/AFFINE_GEMM_MAPPING.md section 19, layer 1:
// screenAffineMatmulGeometry): loop readers, ranks in the full and in the role space
// (batch loops removed), C's integer kernel, the reduction loops and the six-vector
// conditions A-D. Necessary conditions only -- these lines decide nothing; the
// recognizer does. The randomized cross-check against an independent implementation
// is affine_gemm_geometry_random.mlir.
//
// RUN: systolic-opt "--test-affine-matmul-discovery=geometry=true" -split-input-file %s \
// RUN:   -o /dev/null 2>&1 | FileCheck %s --check-prefix=GEO

// Test 1: C[i][j] += A[i][i+k] * B[j+k][j] (i, j, k in [0,4)). The readers make i, j, k the roles I, J, K;
// the six-vector conditions A-D hold in the full and in the role space. (Whether it is a GEMM --
// the decomposition, the views, C's injectivity, the bounds -- is not decided here.)
// GEO:      geometry screening (necessary conditions only, not a decision):
// GEO-NEXT:   loop 0: trip 4, readers A C -> I
// GEO-NEXT:   loop 1: trip 4, readers B C -> J
// GEO-NEXT:   loop 2: trip 4, readers A B -> K (reduction)
// GEO-NEXT:   A ordinal rows: {{\[\[}}1, 0, 0], [1, 0, 1]]
// GEO-NEXT:   B ordinal rows: {{\[\[}}0, 1, 1], [0, 1, 0]]
// GEO-NEXT:   C ordinal rows: {{\[\[}}1, 0, 0], [0, 1, 0]]
// GEO-NEXT:   rank of C: 2 (role space 2); rank of all subscripts: 3 (role space 3)
// GEO-NEXT:   C kernel: {{\[\[}}0, 0, 1]]
// GEO-NEXT:   reduction loops: 2
// GEO-NEXT:   six-vector conditions (full space): A yes, B yes, C yes (d = [0, 0, 1]), D yes yes yes
// GEO-NEXT:   six-vector conditions (role space): A yes, B yes, C yes (d = [0, 0, 1]), D yes yes yes
func.func @test1(%A: memref<4x7xf32> {llvm.noalias}, %B: memref<7x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
      %a = affine.load %A[%i, %i + %k] : memref<4x7xf32>
      %b = affine.load %B[%j + %k, %j] : memref<7x4xf32>
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
// Test 2: C[i+j-2k][i+2j+k] += A[i+j-2k][i+j+k] * B[i+j+k][i+2j+k] on the box i in [2,10),
// j in [0,6), k in [0,2) (every index >= 0). Every loop is read by A, B and C: all three are batch
// loops and the role space is empty. In the full loop space the six-vector conditions A-D all hold
// (d = (5,-3,1) keeps C fixed and moves A's and B's reduction index): necessary, not sufficient --
// d joins two points of the box but is no loop direction, so the iterations it joins are different
// batch iterations writing one C element, not one reduction.
// GEO:      geometry screening (necessary conditions only, not a decision):
// GEO-NEXT:   loop 0: trip 8, readers A B C -> batch
// GEO-NEXT:   loop 1: trip 6, readers A B C -> batch
// GEO-NEXT:   loop 2: trip 2, readers A B C -> batch
// GEO-NEXT:   A ordinal rows: {{\[\[}}1, 1, -2], [1, 1, 1]]
// GEO-NEXT:   B ordinal rows: {{\[\[}}1, 1, 1], [1, 2, 1]]
// GEO-NEXT:   C ordinal rows: {{\[\[}}1, 1, -2], [1, 2, 1]]
// GEO-NEXT:   rank of C: 2 (role space 0); rank of all subscripts: 3 (role space 0)
// GEO-NEXT:   C kernel: {{\[\[}}5, -3, 1]]
// GEO-NEXT:   reduction loops: none
// GEO-NEXT:   C kernel direction [5, -3, 1] joins two points of the box and is not a reduction: two iterations write one C element
// GEO-NEXT:   six-vector conditions (full space): A yes, B yes, C yes (d = [5, -3, 1]), D yes yes yes
// GEO-NEXT:   six-vector conditions (role space): A no, B no, C no, D no no no
func.func @test2(%A: memref<15x16xf32> {llvm.noalias}, %B: memref<16x21xf32> {llvm.noalias}, %C: memref<15x21xf32> {llvm.noalias}) {
  affine.for %i = 2 to 10 {
    affine.for %j = 0 to 6 {
      affine.for %k = 0 to 2 {
      %a = affine.load %A[%i + %j - %k * 2, %i + %j + %k] : memref<15x16xf32>
      %b = affine.load %B[%i + %j + %k, %i + %j * 2 + %k] : memref<16x21xf32>
      %c = affine.load %C[%i + %j - %k * 2, %i + %j * 2 + %k] : memref<15x21xf32>
      %p = arith.mulf %a, %b : f32
      %s = arith.addf %c, %p : f32
      affine.store %s, %C[%i + %j - %k * 2, %i + %j * 2 + %k] : memref<15x21xf32>
      }
    }
  }
  return
}
// -----
// Test 3: C[i][j] += A[i][j+k] * B[i+k][j]. i and j are read by all three operands (batch loops),
// k by both factors only (the reduction): A-D hold in the full space, while the role space keeps k
// alone -- one dot product per (i, j).
// GEO:      geometry screening (necessary conditions only, not a decision):
// GEO-NEXT:   loop 0: trip 4, readers A B C -> batch
// GEO-NEXT:   loop 1: trip 4, readers A B C -> batch
// GEO-NEXT:   loop 2: trip 4, readers A B -> K (reduction)
// GEO-NEXT:   A ordinal rows: {{\[\[}}1, 0, 0], [0, 1, 1]]
// GEO-NEXT:   B ordinal rows: {{\[\[}}1, 0, 1], [0, 1, 0]]
// GEO-NEXT:   C ordinal rows: {{\[\[}}1, 0, 0], [0, 1, 0]]
// GEO-NEXT:   rank of C: 2 (role space 0); rank of all subscripts: 3 (role space 1)
// GEO-NEXT:   C kernel: {{\[\[}}0, 0, 1]]
// GEO-NEXT:   reduction loops: 2
// GEO-NEXT:   six-vector conditions (full space): A yes, B yes, C yes (d = [0, 0, 1]), D yes yes yes
// GEO-NEXT:   six-vector conditions (role space): A no, B no, C yes (d = [0, 0, 1]), D no no no
func.func @test3(%A: memref<4x7xf32> {llvm.noalias}, %B: memref<7x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
      %a = affine.load %A[%i, %j + %k] : memref<4x7xf32>
      %b = affine.load %B[%i + %k, %j] : memref<7x4xf32>
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
// A skewed only (showcase case_126): the same screening as Test 1.
// GEO:      geometry screening (necessary conditions only, not a decision):
// GEO-NEXT:   loop 0: trip 4, readers A C -> I
// GEO-NEXT:   loop 1: trip 4, readers B C -> J
// GEO-NEXT:   loop 2: trip 4, readers A B -> K (reduction)
// GEO-NEXT:   A ordinal rows: {{\[\[}}1, 0, 0], [1, 0, 1]]
// GEO-NEXT:   B ordinal rows: {{\[\[}}0, 0, 1], [0, 1, 0]]
// GEO-NEXT:   C ordinal rows: {{\[\[}}1, 0, 0], [0, 1, 0]]
// GEO-NEXT:   rank of C: 2 (role space 2); rank of all subscripts: 3 (role space 3)
// GEO-NEXT:   C kernel: {{\[\[}}0, 0, 1]]
// GEO-NEXT:   reduction loops: 2
// GEO-NEXT:   six-vector conditions (full space): A yes, B yes, C yes (d = [0, 0, 1]), D yes yes yes
// GEO-NEXT:   six-vector conditions (role space): A yes, B yes, C yes (d = [0, 0, 1]), D yes yes yes
func.func @v_skew_a_only(%A: memref<4x7xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
      %a = affine.load %A[%i, %i + %k] : memref<4x7xf32>
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
// Flat 1-D arrays, C[4i+j] += A[4i+k] * B[4k+j]: C's kernel holds (1,-4,0), which joins no two
// points of the box (|-4| >= 4): no collision. Not two dimensions each: no six-vector conditions.
// GEO:      geometry screening (necessary conditions only, not a decision):
// GEO-NEXT:   loop 0: trip 4, readers A C -> I
// GEO-NEXT:   loop 1: trip 4, readers B C -> J
// GEO-NEXT:   loop 2: trip 4, readers A B -> K (reduction)
// GEO-NEXT:   A ordinal rows: {{\[\[}}4, 0, 1]]
// GEO-NEXT:   B ordinal rows: {{\[\[}}0, 1, 4]]
// GEO-NEXT:   C ordinal rows: {{\[\[}}4, 1, 0]]
// GEO-NEXT:   rank of C: 1 (role space 1); rank of all subscripts: 3 (role space 3)
// GEO-NEXT:   C kernel: {{\[\[}}1, -4, 0], [0, 0, 1]]
// GEO-NEXT:   reduction loops: 2
func.func @v_flat(%A: memref<16xf32> {llvm.noalias}, %B: memref<16xf32> {llvm.noalias}, %C: memref<16xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
      %a = affine.load %A[%i * 4 + %k] : memref<16xf32>
      %b = affine.load %B[%k * 4 + %j] : memref<16xf32>
      %c = affine.load %C[%i * 4 + %j] : memref<16xf32>
      %p = arith.mulf %a, %b : f32
      %s = arith.addf %c, %p : f32
      affine.store %s, %C[%i * 4 + %j] : memref<16xf32>
      }
    }
  }
  return
}
// -----
// A skewed output view C[i+j][j] (rank 2): no collision.
// GEO:      geometry screening (necessary conditions only, not a decision):
// GEO-NEXT:   loop 0: trip 4, readers A C -> I
// GEO-NEXT:   loop 1: trip 4, readers B C -> J
// GEO-NEXT:   loop 2: trip 4, readers A B -> K (reduction)
// GEO-NEXT:   A ordinal rows: {{\[\[}}1, 0, 0], [0, 0, 1]]
// GEO-NEXT:   B ordinal rows: {{\[\[}}0, 0, 1], [0, 1, 0]]
// GEO-NEXT:   C ordinal rows: {{\[\[}}1, 1, 0], [0, 1, 0]]
// GEO-NEXT:   rank of C: 2 (role space 2); rank of all subscripts: 3 (role space 3)
// GEO-NEXT:   C kernel: {{\[\[}}0, 0, 1]]
// GEO-NEXT:   reduction loops: 2
// GEO-NEXT:   six-vector conditions (full space): A yes, B yes, C yes (d = [0, 0, 1]), D yes yes yes
// GEO-NEXT:   six-vector conditions (role space): A yes, B yes, C yes (d = [0, 0, 1]), D yes yes yes
func.func @v_skew_c(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<7x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
      %a = affine.load %A[%i, %k] : memref<4x4xf32>
      %b = affine.load %B[%k, %j] : memref<4x4xf32>
      %c = affine.load %C[%i + %j, %j] : memref<7x4xf32>
      %p = arith.mulf %a, %b : f32
      %s = arith.addf %c, %p : f32
      affine.store %s, %C[%i + %j, %j] : memref<7x4xf32>
      }
    }
  }
  return
}
// -----
// C[i+j] (1-D): (1,-1,0) keeps C fixed and joins two points, (0,1) and (1,0) of the box.
// GEO:      geometry screening (necessary conditions only, not a decision):
// GEO-NEXT:   loop 0: trip 4, readers A C -> I
// GEO-NEXT:   loop 1: trip 4, readers B C -> J
// GEO-NEXT:   loop 2: trip 4, readers A B -> K (reduction)
// GEO-NEXT:   A ordinal rows: {{\[\[}}1, 0, 0], [0, 0, 1]]
// GEO-NEXT:   B ordinal rows: {{\[\[}}0, 0, 1], [0, 1, 0]]
// GEO-NEXT:   C ordinal rows: {{\[\[}}1, 1, 0]]
// GEO-NEXT:   rank of C: 1 (role space 1); rank of all subscripts: 3 (role space 3)
// GEO-NEXT:   C kernel: {{\[\[}}1, -1, 0], [0, 0, 1]]
// GEO-NEXT:   reduction loops: 2
// GEO-NEXT:   C kernel direction [1, -1, 0] joins two points of the box and is not a reduction: two iterations write one C element
func.func @v_c_not_injective(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<7xf32> {llvm.noalias}) {
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
// A shifted reduction loop, t in [i, i+4): the domain is not a box (a parallelotope),
// so the box screening does not apply (the recognizer's domain parametrization, section 19).
// GEO:      geometry screening (necessary conditions only, not a decision):
// GEO-NEXT:   not screened: loop 2's lower bound depends on another loop's counter: the iteration domain is not a box
func.func @shifted_reduction(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
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
