// The geometric screening (docs/AFFINE_GEMM_MAPPING.md section 19, layer 1:
// screenAffineMatmulGeometry): loop readers, ranks in the full and in the role space
// (batch loops removed), C's integer kernel, the reduction loops, the six-vector
// conditions A-D and the task algorithm of 2026-10-18 line by line (section 19.10).
// Necessary conditions only -- these lines decide nothing; the recognizer does. The randomized cross-check against an independent implementation
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
// GEO-NEXT:   task algorithm (full space): rank(Ic,Jc)=2 yes, rank(Ia,Ka)=2 yes, rank(Kb,Jb)=2 yes, rank(six)=3 yes, rank(Ia,Ka,Kb,Jb)=3 yes, A varies along null(Ic,Jc) yes, B varies along null(Ic,Jc) yes, swap(Ia,Ka) no, swap(Kb,Jb) no -> passes
// GEO-NEXT:   six-vector conditions (role space): A yes, B yes, C yes (d = [0, 0, 1]), D yes yes yes
// GEO-NEXT:   task algorithm (role space): rank(Ic,Jc)=2 yes, rank(Ia,Ka)=2 yes, rank(Kb,Jb)=2 yes, rank(six)=3 yes, rank(Ia,Ka,Kb,Jb)=3 yes, A varies along null(Ic,Jc) yes, B varies along null(Ic,Jc) yes, swap(Ia,Ka) no, swap(Kb,Jb) no -> passes
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
// GEO-NEXT:   task algorithm (full space): rank(Ic,Jc)=2 yes, rank(Ia,Ka)=2 yes, rank(Kb,Jb)=2 yes, rank(six)=3 yes, rank(Ia,Ka,Kb,Jb)=3 yes, A varies along null(Ic,Jc) yes, B varies along null(Ic,Jc) yes, swap(Ia,Ka) no, swap(Kb,Jb) no -> passes
// GEO-NEXT:   six-vector conditions (role space): A no, B no, C no, D no no no
// GEO-NEXT:   task algorithm (role space): rank(Ic,Jc)=2 no, rank(Ia,Ka)=2 no, rank(Kb,Jb)=2 no, rank(six)=3 no, rank(Ia,Ka,Kb,Jb)=3 no, A varies along null(Ic,Jc) no, B varies along null(Ic,Jc) no, swap(Ia,Ka) no, swap(Kb,Jb) no -> error
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
// GEO-NEXT:   task algorithm (full space): rank(Ic,Jc)=2 yes, rank(Ia,Ka)=2 yes, rank(Kb,Jb)=2 yes, rank(six)=3 yes, rank(Ia,Ka,Kb,Jb)=3 yes, A varies along null(Ic,Jc) yes, B varies along null(Ic,Jc) yes, swap(Ia,Ka) no, swap(Kb,Jb) no -> passes
// GEO-NEXT:   six-vector conditions (role space): A no, B no, C yes (d = [0, 0, 1]), D no no no
// GEO-NEXT:   task algorithm (role space): rank(Ic,Jc)=2 no, rank(Ia,Ka)=2 no, rank(Kb,Jb)=2 no, rank(six)=3 no, rank(Ia,Ka,Kb,Jb)=3 no, A varies along null(Ic,Jc) yes, B varies along null(Ic,Jc) yes, swap(Ia,Ka) no, swap(Kb,Jb) no -> error
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
// GEO-NEXT:   task algorithm (full space): rank(Ic,Jc)=2 yes, rank(Ia,Ka)=2 yes, rank(Kb,Jb)=2 yes, rank(six)=3 yes, rank(Ia,Ka,Kb,Jb)=3 yes, A varies along null(Ic,Jc) yes, B varies along null(Ic,Jc) yes, swap(Ia,Ka) no, swap(Kb,Jb) no -> passes
// GEO-NEXT:   six-vector conditions (role space): A yes, B yes, C yes (d = [0, 0, 1]), D yes yes yes
// GEO-NEXT:   task algorithm (role space): rank(Ic,Jc)=2 yes, rank(Ia,Ka)=2 yes, rank(Kb,Jb)=2 yes, rank(six)=3 yes, rank(Ia,Ka,Kb,Jb)=3 yes, A varies along null(Ic,Jc) yes, B varies along null(Ic,Jc) yes, swap(Ia,Ka) no, swap(Kb,Jb) no -> passes
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
// GEO-NEXT:   task algorithm (full space): rank(Ic,Jc)=2 yes, rank(Ia,Ka)=2 yes, rank(Kb,Jb)=2 yes, rank(six)=3 yes, rank(Ia,Ka,Kb,Jb)=3 yes, A varies along null(Ic,Jc) yes, B varies along null(Ic,Jc) yes, swap(Ia,Ka) no, swap(Kb,Jb) no -> passes
// GEO-NEXT:   six-vector conditions (role space): A yes, B yes, C yes (d = [0, 0, 1]), D yes yes yes
// GEO-NEXT:   task algorithm (role space): rank(Ic,Jc)=2 yes, rank(Ia,Ka)=2 yes, rank(Kb,Jb)=2 yes, rank(six)=3 yes, rank(Ia,Ka,Kb,Jb)=3 yes, A varies along null(Ic,Jc) yes, B varies along null(Ic,Jc) yes, swap(Ia,Ka) no, swap(Kb,Jb) no -> passes
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
// A shifted reduction loop, t in [i, i+4): the domain is a constant-extent parallelotope.
// Since 2026-10-18 (section 19.12) the domain normalization rewrites it into the box nest
// x = U x' (t = t' + i, so A[i][t'], B[t'][j]) before anything is screened, and the screening
// sees the plain 4 x 4 x 4 GEMM. (Until then: "not screened: loop 2's lower bound depends on
// another loop's counter".)
// GEO:      domain normalization: 1 nest(s)
// GEO-NEXT:   loop 2: x2 = x2' + 1*x0, box [0, 4) step 1
// GEO:      geometry screening (necessary conditions only, not a decision):
// GEO-NEXT:   loop 0: trip 4, readers A C -> I
// GEO-NEXT:   loop 1: trip 4, readers B C -> J
// GEO-NEXT:   loop 2: trip 4, readers A B -> K (reduction)
// GEO:        task algorithm (full space): rank(Ic,Jc)=2 yes, rank(Ia,Ka)=2 yes, rank(Kb,Jb)=2 yes, rank(six)=3 yes, rank(Ia,Ka,Kb,Jb)=3 yes, A varies along null(Ic,Jc) yes, B varies along null(Ic,Jc) yes, swap(Ia,Ka) no, swap(Kb,Jb) no -> passes
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
// -----
// ========================================================================================
// The task algorithm of 2026-10-18 (docs/AFFINE_GEMM_MAPPING.md section 19.10), line by
// line, each line failing ALONE where that is possible; N = null(Ic, Jc). The screening
// reports the lines; it decides nothing -- what the recognizer makes of each nest is in
// affine_gemm_generalized.mlir and the showcase (the comments say which).
// ========================================================================================
// rank(Ic, Jc) = 2 fails alone: C[i][i] (showcase case_123; j is then read by B only).
// N = span(e_j, e_k) has dimension 2: A varies along it through Ka . e_k (the whole basis
// is checked, not one vector). Valid by demotion (j a batch loop), not constructed.
// GEO:      geometry screening (necessary conditions only, not a decision):
// GEO-NEXT:   loop 0: trip 4, readers A C -> I
// GEO-NEXT:   loop 1: trip 4, readers B -> one reader (a broadcast)
// GEO-NEXT:   loop 2: trip 4, readers A B -> K (reduction)
// GEO-NEXT:   A ordinal rows: {{\[\[}}1, 0, 0], [0, 0, 1]]
// GEO-NEXT:   B ordinal rows: {{\[\[}}0, 0, 1], [0, 1, 0]]
// GEO-NEXT:   C ordinal rows: {{\[\[}}1, 0, 0], [1, 0, 0]]
// GEO-NEXT:   rank of C: 1 (role space 1); rank of all subscripts: 3 (role space 3)
// GEO-NEXT:   C kernel: {{\[\[}}0, 1, 0], [0, 0, 1]]
// GEO-NEXT:   reduction loops: 2
// GEO-NEXT:   C kernel direction [0, 1, 0] joins two points of the box and is not a reduction: two iterations write one C element
// GEO-NEXT:   six-vector conditions (full space): A no, B yes, C yes (d = [0, 0, 1]), D yes yes yes
// GEO-NEXT:   task algorithm (full space): rank(Ic,Jc)=2 no, rank(Ia,Ka)=2 yes, rank(Kb,Jb)=2 yes, rank(six)=3 yes, rank(Ia,Ka,Kb,Jb)=3 yes, A varies along null(Ic,Jc) yes, B varies along null(Ic,Jc) yes, swap(Ia,Ka) no, swap(Kb,Jb) no -> error
// GEO-NEXT:   six-vector conditions (role space): A no, B yes, C yes (d = [0, 0, 1]), D yes yes yes
// GEO-NEXT:   task algorithm (role space): rank(Ic,Jc)=2 no, rank(Ia,Ka)=2 yes, rank(Kb,Jb)=2 yes, rank(six)=3 yes, rank(Ia,Ka,Kb,Jb)=3 yes, A varies along null(Ic,Jc) yes, B varies along null(Ic,Jc) yes, swap(Ia,Ka) no, swap(Kb,Jb) no -> error
func.func @alg_fail_rank_c(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
      %a = affine.load %A[%i, %k] : memref<4x4xf32>
      %b = affine.load %B[%k, %j] : memref<4x4xf32>
      %c = affine.load %C[%i, %i] : memref<4x4xf32>
      %p = arith.mulf %a, %b : f32
      %s = arith.addf %c, %p : f32
      affine.store %s, %C[%i, %i] : memref<4x4xf32>
      }
    }
  }
  return
}
// -----
// rank(Ia, Ka) = 2 fails alone: A[i + k][2i + 2k] reads one direction in both dimensions.
// Every other line passes (A varies along N = e_k through both subscripts, so no swap).
// Section 19.1 admits it: A'(I, K) = A[I + K][2(I + K)] is an affine view (a line of A), a
// GEMM with a general view -- the line is not necessary for a GEMM of affine views.
// GEO:      geometry screening (necessary conditions only, not a decision):
// GEO-NEXT:   loop 0: trip 4, readers A C -> I
// GEO-NEXT:   loop 1: trip 4, readers B C -> J
// GEO-NEXT:   loop 2: trip 4, readers A B -> K (reduction)
// GEO-NEXT:   A ordinal rows: {{\[\[}}1, 0, 1], [2, 0, 2]]
// GEO-NEXT:   B ordinal rows: {{\[\[}}0, 0, 1], [0, 1, 0]]
// GEO-NEXT:   C ordinal rows: {{\[\[}}1, 0, 0], [0, 1, 0]]
// GEO-NEXT:   rank of C: 2 (role space 2); rank of all subscripts: 3 (role space 3)
// GEO-NEXT:   C kernel: {{\[\[}}0, 0, 1]]
// GEO-NEXT:   reduction loops: 2
// GEO-NEXT:   six-vector conditions (full space): A yes, B yes, C yes (d = [0, 0, 1]), D no yes yes
// GEO-NEXT:   task algorithm (full space): rank(Ic,Jc)=2 yes, rank(Ia,Ka)=2 no, rank(Kb,Jb)=2 yes, rank(six)=3 yes, rank(Ia,Ka,Kb,Jb)=3 yes, A varies along null(Ic,Jc) yes, B varies along null(Ic,Jc) yes, swap(Ia,Ka) no, swap(Kb,Jb) no -> error
// GEO-NEXT:   six-vector conditions (role space): A yes, B yes, C yes (d = [0, 0, 1]), D no yes yes
// GEO-NEXT:   task algorithm (role space): rank(Ic,Jc)=2 yes, rank(Ia,Ka)=2 no, rank(Kb,Jb)=2 yes, rank(six)=3 yes, rank(Ia,Ka,Kb,Jb)=3 yes, A varies along null(Ic,Jc) yes, B varies along null(Ic,Jc) yes, swap(Ia,Ka) no, swap(Kb,Jb) no -> error
func.func @alg_fail_rank_a(%A: memref<7x14xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
      %a = affine.load %A[%i + %k, %i * 2 + %k * 2] : memref<7x14xf32>
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
// rank(Kb, Jb) = 2 fails alone: B[k + j][2k + 2j], the mirror image of the previous case.
// GEO:      geometry screening (necessary conditions only, not a decision):
// GEO-NEXT:   loop 0: trip 4, readers A C -> I
// GEO-NEXT:   loop 1: trip 4, readers B C -> J
// GEO-NEXT:   loop 2: trip 4, readers A B -> K (reduction)
// GEO-NEXT:   A ordinal rows: {{\[\[}}1, 0, 0], [0, 0, 1]]
// GEO-NEXT:   B ordinal rows: {{\[\[}}0, 1, 1], [0, 2, 2]]
// GEO-NEXT:   C ordinal rows: {{\[\[}}1, 0, 0], [0, 1, 0]]
// GEO-NEXT:   rank of C: 2 (role space 2); rank of all subscripts: 3 (role space 3)
// GEO-NEXT:   C kernel: {{\[\[}}0, 0, 1]]
// GEO-NEXT:   reduction loops: 2
// GEO-NEXT:   six-vector conditions (full space): A yes, B yes, C yes (d = [0, 0, 1]), D yes no yes
// GEO-NEXT:   task algorithm (full space): rank(Ic,Jc)=2 yes, rank(Ia,Ka)=2 yes, rank(Kb,Jb)=2 no, rank(six)=3 yes, rank(Ia,Ka,Kb,Jb)=3 yes, A varies along null(Ic,Jc) yes, B varies along null(Ic,Jc) yes, swap(Ia,Ka) no, swap(Kb,Jb) no -> error
// GEO-NEXT:   six-vector conditions (role space): A yes, B yes, C yes (d = [0, 0, 1]), D yes no yes
// GEO-NEXT:   task algorithm (role space): rank(Ic,Jc)=2 yes, rank(Ia,Ka)=2 yes, rank(Kb,Jb)=2 no, rank(six)=3 yes, rank(Ia,Ka,Kb,Jb)=3 yes, A varies along null(Ic,Jc) yes, B varies along null(Ic,Jc) yes, swap(Ia,Ka) no, swap(Kb,Jb) no -> error
func.func @alg_fail_rank_b(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<7x14xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
      %a = affine.load %A[%i, %k] : memref<4x4xf32>
      %b = affine.load %B[%k + %j, %k * 2 + %j * 2] : memref<7x14xf32>
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
// rank(six) = 3 fails alone (it is 4): a fourth loop l read by B only (and j by C only),
// C[i][j] += A[i][k] * B[k][l]. N = span(e_k, e_l); A and B vary along it; no swap. By
// section 19.1 a batched GEMM with stride-0 views (j and l as roles the other operands
// are constant along, or batch loops): the recognizer refuses the broadcast,
// VALID_BUT_LOWERING_UNSUPPORTED. The first candidate is the inner chain (j, k, l), not
// screened: i is outside it.
// GEO:      geometry screening (necessary conditions only, not a decision):
// GEO-NEXT:   not screened: an access index is not a counter of the nest's loops, or an access map has symbols
// GEO:      geometry screening (necessary conditions only, not a decision):
// GEO-NEXT:   loop 0: trip 4, readers A C -> I
// GEO-NEXT:   loop 1: trip 4, readers C -> one reader (a broadcast)
// GEO-NEXT:   loop 2: trip 4, readers A B -> K (reduction)
// GEO-NEXT:   loop 3: trip 4, readers B -> one reader (a broadcast)
// GEO-NEXT:   A ordinal rows: {{\[\[}}1, 0, 0, 0], [0, 0, 1, 0]]
// GEO-NEXT:   B ordinal rows: {{\[\[}}0, 0, 1, 0], [0, 0, 0, 1]]
// GEO-NEXT:   C ordinal rows: {{\[\[}}1, 0, 0, 0], [0, 1, 0, 0]]
// GEO-NEXT:   rank of C: 2 (role space 2); rank of all subscripts: 4 (role space 4)
// GEO-NEXT:   C kernel: {{\[\[}}0, 0, 1, 0], [0, 0, 0, 1]]
// GEO-NEXT:   reduction loops: 2
// GEO-NEXT:   C kernel direction [0, 0, 0, 1] joins two points of the box and is not a reduction: two iterations write one C element
// GEO-NEXT:   six-vector conditions (full space): A yes, B no, C yes (d = [0, 0, 1, 0]), D yes yes yes
// GEO-NEXT:   task algorithm (full space): rank(Ic,Jc)=2 yes, rank(Ia,Ka)=2 yes, rank(Kb,Jb)=2 yes, rank(six)=3 no, rank(Ia,Ka,Kb,Jb)=3 yes, A varies along null(Ic,Jc) yes, B varies along null(Ic,Jc) yes, swap(Ia,Ka) no, swap(Kb,Jb) no -> error
// GEO-NEXT:   six-vector conditions (role space): A yes, B no, C yes (d = [0, 0, 1, 0]), D yes yes yes
// GEO-NEXT:   task algorithm (role space): rank(Ic,Jc)=2 yes, rank(Ia,Ka)=2 yes, rank(Kb,Jb)=2 yes, rank(six)=3 no, rank(Ia,Ka,Kb,Jb)=3 yes, A varies along null(Ic,Jc) yes, B varies along null(Ic,Jc) yes, swap(Ia,Ka) no, swap(Kb,Jb) no -> error
func.func @alg_fail_rank_six(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        affine.for %l = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<4x4xf32>
        %b = affine.load %B[%k, %l] : memref<4x4xf32>
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
// rank(Ia, Ka, Kb, Jb) = 3 fails alone (it is 2): B[k][i] with C[i][j] (showcase case_124),
// j read by C only. Valid by demotion (j a batch loop: a batch of dot products), not
// constructed.
// GEO:      geometry screening (necessary conditions only, not a decision):
// GEO-NEXT:   loop 0: trip 4, readers A B C -> batch
// GEO-NEXT:   loop 1: trip 4, readers C -> one reader (a broadcast)
// GEO-NEXT:   loop 2: trip 4, readers A B -> K (reduction)
// GEO-NEXT:   A ordinal rows: {{\[\[}}1, 0, 0], [0, 0, 1]]
// GEO-NEXT:   B ordinal rows: {{\[\[}}0, 0, 1], [1, 0, 0]]
// GEO-NEXT:   C ordinal rows: {{\[\[}}1, 0, 0], [0, 1, 0]]
// GEO-NEXT:   rank of C: 2 (role space 1); rank of all subscripts: 3 (role space 2)
// GEO-NEXT:   C kernel: {{\[\[}}0, 0, 1]]
// GEO-NEXT:   reduction loops: 2
// GEO-NEXT:   six-vector conditions (full space): A yes, B yes, C yes (d = [0, 0, 1]), D yes yes no
// GEO-NEXT:   task algorithm (full space): rank(Ic,Jc)=2 yes, rank(Ia,Ka)=2 yes, rank(Kb,Jb)=2 yes, rank(six)=3 yes, rank(Ia,Ka,Kb,Jb)=3 no, A varies along null(Ic,Jc) yes, B varies along null(Ic,Jc) yes, swap(Ia,Ka) no, swap(Kb,Jb) no -> error
// GEO-NEXT:   six-vector conditions (role space): A no, B no, C yes (d = [0, 0, 1]), D no no no
// GEO-NEXT:   task algorithm (role space): rank(Ic,Jc)=2 no, rank(Ia,Ka)=2 no, rank(Kb,Jb)=2 no, rank(six)=3 no, rank(Ia,Ka,Kb,Jb)=3 no, A varies along null(Ic,Jc) yes, B varies along null(Ic,Jc) yes, swap(Ia,Ka) no, swap(Kb,Jb) no -> error
func.func @alg_fail_rank_four(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
      %a = affine.load %A[%i, %k] : memref<4x4xf32>
      %b = affine.load %B[%k, %i] : memref<4x4xf32>
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
// "dot(Ia, N) == 0 and dot(Ka, N) == 0" fails alone: A[i][j] with C[i][j] and B[k][j] -- A is
// constant along N = e_k. k is read by B only: by section 19.1 a GEMM with a stride-0 view
// of A along K (or k a batch loop); the recognizer refuses the broadcast.
// GEO:      geometry screening (necessary conditions only, not a decision):
// GEO-NEXT:   loop 0: trip 4, readers A C -> I
// GEO-NEXT:   loop 1: trip 4, readers A B C -> batch
// GEO-NEXT:   loop 2: trip 4, readers B -> one reader (a broadcast)
// GEO-NEXT:   A ordinal rows: {{\[\[}}1, 0, 0], [0, 1, 0]]
// GEO-NEXT:   B ordinal rows: {{\[\[}}0, 0, 1], [0, 1, 0]]
// GEO-NEXT:   C ordinal rows: {{\[\[}}1, 0, 0], [0, 1, 0]]
// GEO-NEXT:   rank of C: 2 (role space 1); rank of all subscripts: 3 (role space 2)
// GEO-NEXT:   C kernel: {{\[\[}}0, 0, 1]]
// GEO-NEXT:   reduction loops: none
// GEO-NEXT:   C kernel direction [0, 0, 1] joins two points of the box and is not a reduction: two iterations write one C element
// GEO-NEXT:   six-vector conditions (full space): A yes, B yes, C no, D yes yes yes
// GEO-NEXT:   task algorithm (full space): rank(Ic,Jc)=2 yes, rank(Ia,Ka)=2 yes, rank(Kb,Jb)=2 yes, rank(six)=3 yes, rank(Ia,Ka,Kb,Jb)=3 yes, A varies along null(Ic,Jc) no, B varies along null(Ic,Jc) yes, swap(Ia,Ka) no, swap(Kb,Jb) no -> error
// GEO-NEXT:   six-vector conditions (role space): A no, B no, C no, D no no no
// GEO-NEXT:   task algorithm (role space): rank(Ic,Jc)=2 no, rank(Ia,Ka)=2 no, rank(Kb,Jb)=2 no, rank(six)=3 no, rank(Ia,Ka,Kb,Jb)=3 no, A varies along null(Ic,Jc) no, B varies along null(Ic,Jc) yes, swap(Ia,Ka) no, swap(Kb,Jb) no -> error
func.func @alg_fail_a_varies(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
      %a = affine.load %A[%i, %j] : memref<4x4xf32>
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
// "dot(Kb, N) == 0 and dot(Jb, N) == 0" fails alone: B[i][j] with A[i][k] (B constant along
// N = e_k; k read by A only).
// GEO:      geometry screening (necessary conditions only, not a decision):
// GEO-NEXT:   loop 0: trip 4, readers A B C -> batch
// GEO-NEXT:   loop 1: trip 4, readers B C -> J
// GEO-NEXT:   loop 2: trip 4, readers A -> one reader (a broadcast)
// GEO-NEXT:   A ordinal rows: {{\[\[}}1, 0, 0], [0, 0, 1]]
// GEO-NEXT:   B ordinal rows: {{\[\[}}1, 0, 0], [0, 1, 0]]
// GEO-NEXT:   C ordinal rows: {{\[\[}}1, 0, 0], [0, 1, 0]]
// GEO-NEXT:   rank of C: 2 (role space 1); rank of all subscripts: 3 (role space 2)
// GEO-NEXT:   C kernel: {{\[\[}}0, 0, 1]]
// GEO-NEXT:   reduction loops: none
// GEO-NEXT:   C kernel direction [0, 0, 1] joins two points of the box and is not a reduction: two iterations write one C element
// GEO-NEXT:   six-vector conditions (full space): A yes, B yes, C no, D yes yes yes
// GEO-NEXT:   task algorithm (full space): rank(Ic,Jc)=2 yes, rank(Ia,Ka)=2 yes, rank(Kb,Jb)=2 yes, rank(six)=3 yes, rank(Ia,Ka,Kb,Jb)=3 yes, A varies along null(Ic,Jc) yes, B varies along null(Ic,Jc) no, swap(Ia,Ka) no, swap(Kb,Jb) no -> error
// GEO-NEXT:   six-vector conditions (role space): A no, B no, C no, D no no no
// GEO-NEXT:   task algorithm (role space): rank(Ic,Jc)=2 no, rank(Ia,Ka)=2 no, rank(Kb,Jb)=2 no, rank(six)=3 no, rank(Ia,Ka,Kb,Jb)=3 no, A varies along null(Ic,Jc) yes, B varies along null(Ic,Jc) no, swap(Ia,Ka) no, swap(Kb,Jb) no -> error
func.func @alg_fail_b_varies(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
      %a = affine.load %A[%i, %k] : memref<4x4xf32>
      %b = affine.load %B[%i, %j] : memref<4x4xf32>
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
// Both swaps: A[k][i] (Ia varies along N = e_k, Ka does not) and B[j][k] (Kb does not, Jb
// does): the transposed factors. Every "return error" line passes; the recognizer lowers
// the nest with both operands transposed (affine_transpose_ab.c).
// GEO:      geometry screening (necessary conditions only, not a decision):
// GEO-NEXT:   loop 0: trip 4, readers A C -> I
// GEO-NEXT:   loop 1: trip 4, readers B C -> J
// GEO-NEXT:   loop 2: trip 4, readers A B -> K (reduction)
// GEO-NEXT:   A ordinal rows: {{\[\[}}0, 0, 1], [1, 0, 0]]
// GEO-NEXT:   B ordinal rows: {{\[\[}}0, 1, 0], [0, 0, 1]]
// GEO-NEXT:   C ordinal rows: {{\[\[}}1, 0, 0], [0, 1, 0]]
// GEO-NEXT:   rank of C: 2 (role space 2); rank of all subscripts: 3 (role space 3)
// GEO-NEXT:   C kernel: {{\[\[}}0, 0, 1]]
// GEO-NEXT:   reduction loops: 2
// GEO-NEXT:   six-vector conditions (full space): A yes, B yes, C no, D yes yes no
// GEO-NEXT:   task algorithm (full space): rank(Ic,Jc)=2 yes, rank(Ia,Ka)=2 yes, rank(Kb,Jb)=2 yes, rank(six)=3 yes, rank(Ia,Ka,Kb,Jb)=3 yes, A varies along null(Ic,Jc) yes, B varies along null(Ic,Jc) yes, swap(Ia,Ka) yes, swap(Kb,Jb) yes -> passes
// GEO-NEXT:   six-vector conditions (role space): A yes, B yes, C no, D yes yes no
// GEO-NEXT:   task algorithm (role space): rank(Ic,Jc)=2 yes, rank(Ia,Ka)=2 yes, rank(Kb,Jb)=2 yes, rank(six)=3 yes, rank(Ia,Ka,Kb,Jb)=3 yes, A varies along null(Ic,Jc) yes, B varies along null(Ic,Jc) yes, swap(Ia,Ka) yes, swap(Kb,Jb) yes -> passes
func.func @alg_both_swaps(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
      %a = affine.load %A[%k, %i] : memref<4x4xf32>
      %b = affine.load %B[%j, %k] : memref<4x4xf32>
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
// N of dimension 2 with a loop no access uses (r): N = span(e_k, e_r). A varies along N
// through e_k only -- the check runs over the whole basis (a single basis vector, e_r,
// would say "constant"). Every line passes. (The recognizer: the repetition loop inside
// the reduction is not a legal hoist, affine_to_systolic_depth.mlir; the first candidate
// is the inner chain, not screened.)
// GEO:      geometry screening (necessary conditions only, not a decision):
// GEO-NEXT:   not screened: an access index is not a counter of the nest's loops, or an access map has symbols
// GEO:      geometry screening (necessary conditions only, not a decision):
// GEO-NEXT:   loop 0: trip 4, readers A C -> I
// GEO-NEXT:   loop 1: trip 4, readers B C -> J
// GEO-NEXT:   loop 2: trip 4, readers A B -> K (reduction)
// GEO-NEXT:   loop 3: trip 2, readers -> no reader
// GEO-NEXT:   A ordinal rows: {{\[\[}}1, 0, 0, 0], [0, 0, 1, 0]]
// GEO-NEXT:   B ordinal rows: {{\[\[}}0, 0, 1, 0], [0, 1, 0, 0]]
// GEO-NEXT:   C ordinal rows: {{\[\[}}1, 0, 0, 0], [0, 1, 0, 0]]
// GEO-NEXT:   rank of C: 2 (role space 2); rank of all subscripts: 3 (role space 3)
// GEO-NEXT:   C kernel: {{\[\[}}0, 0, 1, 0], [0, 0, 0, 1]]
// GEO-NEXT:   reduction loops: 2
// GEO-NEXT:   C kernel direction [0, 0, 0, 1] joins two points of the box and is not a reduction: two iterations write one C element
// GEO-NEXT:   six-vector conditions (full space): A yes, B yes, C yes (d = [0, 0, 1, 0]), D yes yes yes
// GEO-NEXT:   task algorithm (full space): rank(Ic,Jc)=2 yes, rank(Ia,Ka)=2 yes, rank(Kb,Jb)=2 yes, rank(six)=3 yes, rank(Ia,Ka,Kb,Jb)=3 yes, A varies along null(Ic,Jc) yes, B varies along null(Ic,Jc) yes, swap(Ia,Ka) no, swap(Kb,Jb) no -> passes
// GEO-NEXT:   six-vector conditions (role space): A yes, B yes, C yes (d = [0, 0, 1, 0]), D yes yes yes
// GEO-NEXT:   task algorithm (role space): rank(Ic,Jc)=2 yes, rank(Ia,Ka)=2 yes, rank(Kb,Jb)=2 yes, rank(six)=3 yes, rank(Ia,Ka,Kb,Jb)=3 yes, A varies along null(Ic,Jc) yes, B varies along null(Ic,Jc) yes, swap(Ia,Ka) no, swap(Kb,Jb) no -> passes
func.func @alg_nullity_two(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        affine.for %r = 0 to 2 {
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
// The K = 1 GEMM, the outer product C[i][j] += A[i][3] * B[3][j] (SQ-4, lowered): N = {0},
// so "dot(Ia, N) == 0 and dot(Ka, N) == 0" holds vacuously -- the algorithm's lines 2-7 all
// fail. Section 19.1 keeps it a GEMM (K absent, E_K = 1): the lines presuppose a reduction
// loop and are not necessary for a GEMM of affine views.
// GEO:      geometry screening (necessary conditions only, not a decision):
// GEO-NEXT:   loop 0: trip 4, readers A C -> I
// GEO-NEXT:   loop 1: trip 4, readers B C -> J
// GEO-NEXT:   A ordinal rows: {{\[\[}}1, 0], [0, 0]]
// GEO-NEXT:   B ordinal rows: {{\[\[}}0, 0], [0, 1]]
// GEO-NEXT:   C ordinal rows: {{\[\[}}1, 0], [0, 1]]
// GEO-NEXT:   rank of C: 2 (role space 2); rank of all subscripts: 2 (role space 2)
// GEO-NEXT:   C kernel: []
// GEO-NEXT:   reduction loops: none
// GEO-NEXT:   six-vector conditions (full space): A yes, B no, C no, D no no yes
// GEO-NEXT:   task algorithm (full space): rank(Ic,Jc)=2 yes, rank(Ia,Ka)=2 no, rank(Kb,Jb)=2 no, rank(six)=3 no, rank(Ia,Ka,Kb,Jb)=3 no, A varies along null(Ic,Jc) no, B varies along null(Ic,Jc) no, swap(Ia,Ka) no, swap(Kb,Jb) no -> error
// GEO-NEXT:   six-vector conditions (role space): A yes, B no, C no, D no no yes
// GEO-NEXT:   task algorithm (role space): rank(Ic,Jc)=2 yes, rank(Ia,Ka)=2 no, rank(Kb,Jb)=2 no, rank(six)=3 no, rank(Ia,Ka,Kb,Jb)=3 no, A varies along null(Ic,Jc) no, B varies along null(Ic,Jc) no, swap(Ia,Ka) no, swap(Kb,Jb) no -> error
func.func @alg_outer_product_k1(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      %a = affine.load %A[%i, 3] : memref<4x4xf32>
      %b = affine.load %B[3, %j] : memref<4x4xf32>
      %c = affine.load %C[%i, %j] : memref<4x4xf32>
      %p = arith.mulf %a, %b : f32
      %s = arith.addf %c, %p : f32
      affine.store %s, %C[%i, %j] : memref<4x4xf32>
    }
  }
  return
}
