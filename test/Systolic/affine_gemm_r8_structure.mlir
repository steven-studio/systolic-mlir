// R8, the structural recognition relation (docs/AFFINE_GEMM_MAPPING.md section 22): the
// subscripts' coefficient vectors on the loop counters as written, the loop classes by readers,
// the reduction loops Z (read by a factor, not by C) and the shared loops S (read by both
// factors), the ranks of the factors' reduction index vectors on Z (linear independence only),
// intersect(A,B) (some a.b != 0), shared loop variables (some sum |a_t||b_t| > 0), the literal
// reading, and the one-dimensional operands (Phase 2). A DIAGNOSTIC: every case also pins the
// recognizer's verdict without reduction-batch partitioning (section 23), unchanged.
//
// RUN: systolic-opt "--test-affine-matmul-discovery=r8=true" -split-input-file %s \
// RUN:   -o /dev/null 2>&1 | FileCheck %s --check-prefix=R8
// RUN: systolic-opt --test-affine-matmul-discovery -split-input-file %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s --check-prefix=OFF
// OFF-NOT: R8 structure

// T1 (ground truth REJECT): C[i][j] += A[i][x0-x1] * B[x0+x1][j]
// R8:        A rows: {{\[\[}}1, 0, 0, 0], [0, 0, 1, -1]]
// R8-NEXT:   B rows: {{\[\[}}0, 0, 1, 1], [0, 1, 0, 0]]
// R8-NEXT:   C rows: {{\[\[}}1, 0, 0, 0], [0, 1, 0, 0]]
// R8-NEXT:   loop 0: readers A C -> M or OB
// R8-NEXT:   loop 1: readers B C -> N or OB
// R8-NEXT:   loop 2: readers A B -> K or RB (contraction block)
// R8-NEXT:   loop 3: readers A B -> K or RB (contraction block)
// R8-NEXT:   reduction loops Z: 2 3; shared loops S: 2 3; groups: {2 3}
// R8-NEXT:   on Z: rank A 1, rank B 1, rank [A; B] 2 (row spaces meet in dimension 0), coupling rank(A B^T) 0
// R8-NEXT:   batch block coupling: 0
// R8-NEXT:   literal, loops of two or more iterations: rank(A B^T) 0, rank(A C^T) 1, rank(B C^T) 1; nonzero A.B pairs 0 (not a dimension)
// R8-NEXT:   operand ranks: A 2, B 2, C 2
// R8-NEXT:   intersect(A,B) (some a.b != 0 on Z): no; shared loop variables (some sum |a_t||b_t| > 0): yes
// R8-NEXT: recognition result: failure
// R8-NEXT:   reason: logical coordinate K (the first factor's dimension 1 d2 - d3, the second factor's dimensio
// R8-NEXT:   class: UNKNOWN
func.func @t1(%A: memref<4x6xf32> {llvm.noalias}, %B: memref<8x5xf32> {llvm.noalias}, %C: memref<4x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 5 {
      affine.for %x0 = 3 to 6 {
        affine.for %x1 = 0 to 3 {
          %va = affine.load %A[%i, %x0 - %x1] : memref<4x6xf32>
          %vb = affine.load %B[%x0 + %x1, %j] : memref<8x5xf32>
          %vc = affine.load %C[%i, %j] : memref<4x5xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %vc, %prod : f32
          affine.store %sum, %C[%i, %j] : memref<4x5xf32>
        }
      }
    }
  }
  return
}
// -----
// T2 (ground truth ACCEPT): C[i][j] += A[i][x0-x1] * B[x0][j]
// R8:        A rows: {{\[\[}}1, 0, 0, 0], [0, 0, 1, -1]]
// R8-NEXT:   B rows: {{\[\[}}0, 0, 1, 0], [0, 1, 0, 0]]
// R8-NEXT:   C rows: {{\[\[}}1, 0, 0, 0], [0, 1, 0, 0]]
// R8-NEXT:   loop 0: readers A C -> M or OB
// R8-NEXT:   loop 1: readers B C -> N or OB
// R8-NEXT:   loop 2: readers A B -> K or RB (contraction block)
// R8-NEXT:   loop 3: readers A -> RB (one factor only)
// R8-NEXT:   reduction loops Z: 2 3; shared loops S: 2; groups: {2}
// R8-NEXT:   on Z: rank A 1, rank B 1, rank [A; B] 2 (row spaces meet in dimension 0), coupling rank(A B^T) 1
// R8-NEXT:   batch block coupling: 0
// R8-NEXT:   literal, loops of two or more iterations: rank(A B^T) 1, rank(A C^T) 1, rank(B C^T) 1; nonzero A.B pairs 1 (not a dimension)
// R8-NEXT:   operand ranks: A 2, B 2, C 2
// R8-NEXT:   intersect(A,B) (some a.b != 0 on Z): yes; shared loop variables (some sum |a_t||b_t| > 0): yes
// R8-NEXT: recognition result: failure
// R8-NEXT:   reason: logical coordinate K (the first factor's dimension 1 d2 - d3, the second factor's dimensio
// R8-NEXT:   class: UNKNOWN
func.func @t2(%A: memref<4x6xf32> {llvm.noalias}, %B: memref<6x5xf32> {llvm.noalias}, %C: memref<4x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 5 {
      affine.for %x0 = 3 to 6 {
        affine.for %x1 = 0 to 3 {
          %va = affine.load %A[%i, %x0 - %x1] : memref<4x6xf32>
          %vb = affine.load %B[%x0, %j] : memref<6x5xf32>
          %vc = affine.load %C[%i, %j] : memref<4x5xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %vc, %prod : f32
          affine.store %sum, %C[%i, %j] : memref<4x5xf32>
        }
      }
    }
  }
  return
}
// -----
// T3 (ground truth ACCEPT): C[i][j] += A[i][i][k][2k] * B[k][j][2j]
// R8:        A rows: {{\[\[}}1, 0, 0], [1, 0, 0], [0, 0, 1], [0, 0, 2]]
// R8-NEXT:   B rows: {{\[\[}}0, 0, 1], [0, 1, 0], [0, 2, 0]]
// R8-NEXT:   C rows: {{\[\[}}1, 0, 0], [0, 1, 0]]
// R8-NEXT:   loop 0: readers A C -> M or OB
// R8-NEXT:   loop 1: readers B C -> N or OB
// R8-NEXT:   loop 2: readers A B -> K or RB (contraction block)
// R8-NEXT:   reduction loops Z: 2; shared loops S: 2; groups: {2}
// R8-NEXT:   on Z: rank A 1, rank B 1, rank [A; B] 1 (row spaces meet in dimension 1), coupling rank(A B^T) 1
// R8-NEXT:   batch block coupling: 0
// R8-NEXT:   literal, loops of two or more iterations: rank(A B^T) 1, rank(A C^T) 1, rank(B C^T) 1; nonzero A.B pairs 2 (not a dimension)
// R8-NEXT:   operand ranks: A 2, B 2, C 2
// R8-NEXT:   intersect(A,B) (some a.b != 0 on Z): yes; shared loop variables (some sum |a_t||b_t| > 0): yes
// R8:        dense logical GEMM: yes, I/J/K on loops 0/1/2
func.func @t3(%A: memref<3x3x5x10xf32> {llvm.noalias}, %B: memref<5x4x8xf32> {llvm.noalias}, %C: memref<3x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 5 {
        %va = affine.load %A[%i, %i, %k, 2 * %k] : memref<3x3x5x10xf32>
        %vb = affine.load %B[%k, %j, 2 * %j] : memref<5x4x8xf32>
        %vc = affine.load %C[%i, %j] : memref<3x4xf32>
        %prod = arith.mulf %va, %vb : f32
        %sum = arith.addf %vc, %prod : f32
        affine.store %sum, %C[%i, %j] : memref<3x4xf32>
      }
    }
  }
  return
}
// -----
// T4 (ground truth Phase 2 pending): C[10i+j] += A[20i+k][20i+k] * B[50k+j][j]
// R8:        A rows: {{\[\[}}20, 0, 1], [20, 0, 1]]
// R8-NEXT:   B rows: {{\[\[}}0, 1, 50], [0, 1, 0]]
// R8-NEXT:   C rows: {{\[\[}}10, 1, 0]]
// R8-NEXT:   loop 0: readers A C -> M or OB
// R8-NEXT:   loop 1: readers B C -> N or OB
// R8-NEXT:   loop 2: readers A B -> K or RB (contraction block)
// R8-NEXT:   reduction loops Z: 2; shared loops S: 2; groups: {2}
// R8-NEXT:   on Z: rank A 1, rank B 1, rank [A; B] 1 (row spaces meet in dimension 1), coupling rank(A B^T) 1
// R8-NEXT:   batch block coupling: 0
// R8-NEXT:   literal, loops of two or more iterations: rank(A B^T) 1, rank(A C^T) 1, rank(B C^T) 1; nonzero A.B pairs 2 (not a dimension)
// R8-NEXT:   operand ranks: A 1, B 2, C 1; one-dimensional (Phase 2): A C
// R8-NEXT:   intersect(A,B) (some a.b != 0 on Z): yes; shared loop variables (some sum |a_t||b_t| > 0): yes
// R8:        dense logical GEMM: yes, I/J/K on loops 0/1/2
func.func @t4(%A: memref<24x24xf32> {llvm.noalias}, %B: memref<160x10xf32> {llvm.noalias}, %C: memref<20xf32> {llvm.noalias}) {
  affine.for %i = 0 to 2 {
    affine.for %j = 0 to 10 {
      affine.for %k = 0 to 4 {
        %va = affine.load %A[20 * %i + %k, 20 * %i + %k] : memref<24x24xf32>
        %vb = affine.load %B[50 * %k + %j, %j] : memref<160x10xf32>
        %vc = affine.load %C[10 * %i + %j] : memref<20xf32>
        %prod = arith.mulf %va, %vb : f32
        %sum = arith.addf %vc, %prod : f32
        affine.store %sum, %C[10 * %i + %j] : memref<20xf32>
      }
    }
  }
  return
}
// -----
// T5 (ground truth ACCEPT): C[i][j] += A[i][k] * B[l][j]
// R8:        A rows: {{\[\[}}1, 0, 0, 0], [0, 0, 1, 0]]
// R8-NEXT:   B rows: {{\[\[}}0, 0, 0, 1], [0, 1, 0, 0]]
// R8-NEXT:   C rows: {{\[\[}}1, 0, 0, 0], [0, 1, 0, 0]]
// R8-NEXT:   loop 0: readers A C -> M or OB
// R8-NEXT:   loop 1: readers B C -> N or OB
// R8-NEXT:   loop 2: readers A -> RB (one factor only)
// R8-NEXT:   loop 3: readers B -> RB (one factor only)
// R8-NEXT:   reduction loops Z: 2 3; shared loops S: none; groups:
// R8-NEXT:   on Z: rank A 1, rank B 1, rank [A; B] 2 (row spaces meet in dimension 0), coupling rank(A B^T) 0
// R8-NEXT:   batch block coupling: 0
// R8-NEXT:   literal, loops of two or more iterations: rank(A B^T) 0, rank(A C^T) 1, rank(B C^T) 1; nonzero A.B pairs 0 (not a dimension)
// R8-NEXT:   operand ranks: A 2, B 2, C 2
// R8-NEXT:   intersect(A,B) (some a.b != 0 on Z): no; shared loop variables (some sum |a_t||b_t| > 0): no
// R8-NEXT: recognition result: failure
// R8-NEXT:   reason: a coordinate is read by the first factor only: the other operands would be broadcast along
// R8-NEXT:   class: VALID_BUT_LOWERING_UNSUPPORTED
func.func @t5(%A: memref<4x3xf32> {llvm.noalias}, %B: memref<2x5xf32> {llvm.noalias}, %C: memref<4x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 3 {
        affine.for %l = 0 to 2 {
          %va = affine.load %A[%i, %k] : memref<4x3xf32>
          %vb = affine.load %B[%l, %j] : memref<2x5xf32>
          %vc = affine.load %C[%i, %j] : memref<4x5xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %vc, %prod : f32
          affine.store %sum, %C[%i, %j] : memref<4x5xf32>
        }
      }
    }
  }
  return
}
// -----
// standard GEMM: C[i][j] += A[i][k] * B[k][j]
// R8:        A rows: {{\[\[}}1, 0, 0], [0, 0, 1]]
// R8-NEXT:   B rows: {{\[\[}}0, 0, 1], [0, 1, 0]]
// R8-NEXT:   C rows: {{\[\[}}1, 0, 0], [0, 1, 0]]
// R8-NEXT:   loop 0: readers A C -> M or OB
// R8-NEXT:   loop 1: readers B C -> N or OB
// R8-NEXT:   loop 2: readers A B -> K or RB (contraction block)
// R8-NEXT:   reduction loops Z: 2; shared loops S: 2; groups: {2}
// R8-NEXT:   on Z: rank A 1, rank B 1, rank [A; B] 1 (row spaces meet in dimension 1), coupling rank(A B^T) 1
// R8-NEXT:   batch block coupling: 0
// R8-NEXT:   literal, loops of two or more iterations: rank(A B^T) 1, rank(A C^T) 1, rank(B C^T) 1; nonzero A.B pairs 1 (not a dimension)
// R8-NEXT:   operand ranks: A 2, B 2, C 2
// R8-NEXT:   intersect(A,B) (some a.b != 0 on Z): yes; shared loop variables (some sum |a_t||b_t| > 0): yes
// R8:        dense logical GEMM: yes, I/J/K on loops 0/1/2
func.func @gemm(%A: memref<4x3xf32> {llvm.noalias}, %B: memref<3x5xf32> {llvm.noalias}, %C: memref<4x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 3 {
        %va = affine.load %A[%i, %k] : memref<4x3xf32>
        %vb = affine.load %B[%k, %j] : memref<3x5xf32>
        %vc = affine.load %C[%i, %j] : memref<4x5xf32>
        %prod = arith.mulf %va, %vb : f32
        %sum = arith.addf %vc, %prod : f32
        affine.store %sum, %C[%i, %j] : memref<4x5xf32>
      }
    }
  }
  return
}
// -----
// batched GEMM: C[b][i][j] += A[b][i][k] * B[b][k][j]
// R8:        A rows: {{\[\[}}1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 0, 1]]
// R8-NEXT:   B rows: {{\[\[}}1, 0, 0, 0], [0, 0, 0, 1], [0, 0, 1, 0]]
// R8-NEXT:   C rows: {{\[\[}}1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0]]
// R8-NEXT:   loop 0: readers A B C -> OB (batch)
// R8-NEXT:   loop 1: readers A C -> M or OB
// R8-NEXT:   loop 2: readers B C -> N or OB
// R8-NEXT:   loop 3: readers A B -> K or RB (contraction block)
// R8-NEXT:   reduction loops Z: 3; shared loops S: 3; groups: {3}
// R8-NEXT:   on Z: rank A 1, rank B 1, rank [A; B] 1 (row spaces meet in dimension 1), coupling rank(A B^T) 1
// R8-NEXT:   batch block coupling: 1
// R8-NEXT:   literal, loops of two or more iterations: rank(A B^T) 2, rank(A C^T) 2, rank(B C^T) 2; nonzero A.B pairs 2 (not a dimension)
// R8-NEXT:   operand ranks: A 3, B 3, C 3
// R8-NEXT:   intersect(A,B) (some a.b != 0 on Z): yes; shared loop variables (some sum |a_t||b_t| > 0): yes
// R8:        dense logical GEMM: yes, I/J/K on loops 1/2/3, batched over loop(s) 0 (2 batch iteration(s))
func.func @batched(%A: memref<2x4x3xf32> {llvm.noalias}, %B: memref<2x3x5xf32> {llvm.noalias}, %C: memref<2x4x5xf32> {llvm.noalias}) {
  affine.for %b = 0 to 2 {
    affine.for %i = 0 to 4 {
      affine.for %j = 0 to 5 {
        affine.for %k = 0 to 3 {
          %va = affine.load %A[%b, %i, %k] : memref<2x4x3xf32>
          %vb = affine.load %B[%b, %k, %j] : memref<2x3x5xf32>
          %vc = affine.load %C[%b, %i, %j] : memref<2x4x5xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %vc, %prod : f32
          affine.store %sum, %C[%b, %i, %j] : memref<2x4x5xf32>
        }
      }
    }
  }
  return
}
// -----
// two reduction coordinates (R2): C[i][j] += A[i][k][l] * B[k][l][j]
// R8:        A rows: {{\[\[}}1, 0, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]]
// R8-NEXT:   B rows: {{\[\[}}0, 0, 1, 0], [0, 0, 0, 1], [0, 1, 0, 0]]
// R8-NEXT:   C rows: {{\[\[}}1, 0, 0, 0], [0, 1, 0, 0]]
// R8-NEXT:   loop 0: readers A C -> M or OB
// R8-NEXT:   loop 1: readers B C -> N or OB
// R8-NEXT:   loop 2: readers A B -> K or RB (contraction block)
// R8-NEXT:   loop 3: readers A B -> K or RB (contraction block)
// R8-NEXT:   reduction loops Z: 2 3; shared loops S: 2 3; groups: {2} {3}
// R8-NEXT:   on Z: rank A 2, rank B 2, rank [A; B] 2 (row spaces meet in dimension 2), coupling rank(A B^T) 2
// R8-NEXT:   batch block coupling: 0
// R8-NEXT:   literal, loops of two or more iterations: rank(A B^T) 2, rank(A C^T) 1, rank(B C^T) 1; nonzero A.B pairs 2 (not a dimension)
// R8-NEXT:   operand ranks: A 3, B 3, C 2
// R8-NEXT:   intersect(A,B) (some a.b != 0 on Z): yes; shared loop variables (some sum |a_t||b_t| > 0): yes
// R8-NEXT: recognition result: failure
// R8-NEXT:   reason: 2 separate coordinates are read by both factors (the reduction): one GEMM role over severa
// R8-NEXT:   class: VALID_BUT_LOWERING_UNSUPPORTED
func.func @two_k(%A: memref<4x3x2xf32> {llvm.noalias}, %B: memref<3x2x5xf32> {llvm.noalias}, %C: memref<4x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 3 {
        affine.for %l = 0 to 2 {
          %va = affine.load %A[%i, %k, %l] : memref<4x3x2xf32>
          %vb = affine.load %B[%k, %l, %j] : memref<3x2x5xf32>
          %vc = affine.load %C[%i, %j] : memref<4x5xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %vc, %prod : f32
          affine.store %sum, %C[%i, %j] : memref<4x5xf32>
        }
      }
    }
  }
  return
}
// -----
// two M loops (R7): C[i][p][j] += A[i][p][k] * B[k][j]
// R8:        A rows: {{\[\[}}1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 0, 1]]
// R8-NEXT:   B rows: {{\[\[}}0, 0, 0, 1], [0, 0, 1, 0]]
// R8-NEXT:   C rows: {{\[\[}}1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0]]
// R8-NEXT:   loop 0: readers A C -> M or OB
// R8-NEXT:   loop 1: readers A C -> M or OB
// R8-NEXT:   loop 2: readers B C -> N or OB
// R8-NEXT:   loop 3: readers A B -> K or RB (contraction block)
// R8-NEXT:   reduction loops Z: 3; shared loops S: 3; groups: {3}
// R8-NEXT:   on Z: rank A 1, rank B 1, rank [A; B] 1 (row spaces meet in dimension 1), coupling rank(A B^T) 1
// R8-NEXT:   batch block coupling: 0
// R8-NEXT:   literal, loops of two or more iterations: rank(A B^T) 1, rank(A C^T) 2, rank(B C^T) 1; nonzero A.B pairs 1 (not a dimension)
// R8-NEXT:   operand ranks: A 3, B 2, C 3
// R8-NEXT:   intersect(A,B) (some a.b != 0 on Z): yes; shared loop variables (some sum |a_t||b_t| > 0): yes
// R8:        dense logical GEMM: yes, I/J/K on loops 0/2/3, batched over loop(s) 1 (2 batch iteration(s))
func.func @two_m(%A: memref<4x2x3xf32> {llvm.noalias}, %B: memref<3x5xf32> {llvm.noalias}, %C: memref<4x2x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %p = 0 to 2 {
      affine.for %j = 0 to 5 {
        affine.for %k = 0 to 3 {
          %va = affine.load %A[%i, %p, %k] : memref<4x2x3xf32>
          %vb = affine.load %B[%k, %j] : memref<3x5xf32>
          %vc = affine.load %C[%i, %p, %j] : memref<4x2x5xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %vc, %prod : f32
          affine.store %sum, %C[%i, %p, %j] : memref<4x2x5xf32>
        }
      }
    }
  }
  return
}
// -----
// two N loops (R7): C[i][j][q] += A[i][k] * B[k][j][q]
// R8:        A rows: {{\[\[}}1, 0, 0, 0], [0, 0, 0, 1]]
// R8-NEXT:   B rows: {{\[\[}}0, 0, 0, 1], [0, 1, 0, 0], [0, 0, 1, 0]]
// R8-NEXT:   C rows: {{\[\[}}1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0]]
// R8-NEXT:   loop 0: readers A C -> M or OB
// R8-NEXT:   loop 1: readers B C -> N or OB
// R8-NEXT:   loop 2: readers B C -> N or OB
// R8-NEXT:   loop 3: readers A B -> K or RB (contraction block)
// R8-NEXT:   reduction loops Z: 3; shared loops S: 3; groups: {3}
// R8-NEXT:   on Z: rank A 1, rank B 1, rank [A; B] 1 (row spaces meet in dimension 1), coupling rank(A B^T) 1
// R8-NEXT:   batch block coupling: 0
// R8-NEXT:   literal, loops of two or more iterations: rank(A B^T) 1, rank(A C^T) 1, rank(B C^T) 2; nonzero A.B pairs 1 (not a dimension)
// R8-NEXT:   operand ranks: A 2, B 3, C 3
// R8-NEXT:   intersect(A,B) (some a.b != 0 on Z): yes; shared loop variables (some sum |a_t||b_t| > 0): yes
// R8:        dense logical GEMM: yes, I/J/K on loops 0/1/3, batched over loop(s) 2 (2 batch iteration(s))
func.func @two_n(%A: memref<4x3xf32> {llvm.noalias}, %B: memref<3x5x2xf32> {llvm.noalias}, %C: memref<4x5x2xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 5 {
      affine.for %q = 0 to 2 {
        affine.for %k = 0 to 3 {
          %va = affine.load %A[%i, %k] : memref<4x3xf32>
          %vb = affine.load %B[%k, %j, %q] : memref<3x5x2xf32>
          %vc = affine.load %C[%i, %j, %q] : memref<4x5x2xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %vc, %prod : f32
          affine.store %sum, %C[%i, %j, %q] : memref<4x5x2xf32>
        }
      }
    }
  }
  return
}
// -----
// negative coefficient and constants: C[i][j] += A[i][3-k] * B[k+1][j+2]
// R8:        A rows: {{\[\[}}1, 0, 0], [0, 0, -1]]
// R8-NEXT:   B rows: {{\[\[}}0, 0, 1], [0, 1, 0]]
// R8-NEXT:   C rows: {{\[\[}}1, 0, 0], [0, 1, 0]]
// R8-NEXT:   loop 0: readers A C -> M or OB
// R8-NEXT:   loop 1: readers B C -> N or OB
// R8-NEXT:   loop 2: readers A B -> K or RB (contraction block)
// R8-NEXT:   reduction loops Z: 2; shared loops S: 2; groups: {2}
// R8-NEXT:   on Z: rank A 1, rank B 1, rank [A; B] 1 (row spaces meet in dimension 1), coupling rank(A B^T) 1
// R8-NEXT:   batch block coupling: 0
// R8-NEXT:   literal, loops of two or more iterations: rank(A B^T) 1, rank(A C^T) 1, rank(B C^T) 1; nonzero A.B pairs 1 (not a dimension)
// R8-NEXT:   operand ranks: A 2, B 2, C 2
// R8-NEXT:   intersect(A,B) (some a.b != 0 on Z): yes; shared loop variables (some sum |a_t||b_t| > 0): yes
// R8:        dense logical GEMM: yes, I/J/K on loops 0/1/2
func.func @neg_coef(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<5x7xf32> {llvm.noalias}, %C: memref<4x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 4 {
        %va = affine.load %A[%i, 3 - %k] : memref<4x4xf32>
        %vb = affine.load %B[%k + 1, %j + 2] : memref<5x7xf32>
        %vc = affine.load %C[%i, %j] : memref<4x5xf32>
        %prod = arith.mulf %va, %vb : f32
        %sum = arith.addf %vc, %prod : f32
        affine.store %sum, %C[%i, %j] : memref<4x5xf32>
      }
    }
  }
  return
}
// -----
// non-unit stride, written vs ordinal: C[i][j] += A[i][2k] * B[k][j], k in [0,8) step 2
// R8:        A rows: {{\[\[}}1, 0, 0], [0, 0, 2]]
// R8-NEXT:   B rows: {{\[\[}}0, 0, 1], [0, 1, 0]]
// R8-NEXT:   C rows: {{\[\[}}1, 0, 0], [0, 1, 0]]
// R8-NEXT:   loop 0: readers A C -> M or OB
// R8-NEXT:   loop 1: readers B C -> N or OB
// R8-NEXT:   loop 2: readers A B -> K or RB (contraction block)
// R8-NEXT:   reduction loops Z: 2; shared loops S: 2; groups: {2}
// R8-NEXT:   on Z: rank A 1, rank B 1, rank [A; B] 1 (row spaces meet in dimension 1), coupling rank(A B^T) 1
// R8-NEXT:   batch block coupling: 0
// R8-NEXT:   literal, loops of two or more iterations: rank(A B^T) 1, rank(A C^T) 1, rank(B C^T) 1; nonzero A.B pairs 1 (not a dimension)
// R8-NEXT:   operand ranks: A 2, B 2, C 2
// R8-NEXT:   intersect(A,B) (some a.b != 0 on Z): yes; shared loop variables (some sum |a_t||b_t| > 0): yes
// R8:        dense logical GEMM: yes, I/J/K on loops 0/1/2
func.func @stride(%A: memref<4x16xf32> {llvm.noalias}, %B: memref<8x5xf32> {llvm.noalias}, %C: memref<4x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 8 step 2 {
        %va = affine.load %A[%i, 2 * %k] : memref<4x16xf32>
        %vb = affine.load %B[%k, %j] : memref<8x5xf32>
        %vc = affine.load %C[%i, %j] : memref<4x5xf32>
        %prod = arith.mulf %va, %vb : f32
        %sum = arith.addf %vc, %prod : f32
        affine.store %sum, %C[%i, %j] : memref<4x5xf32>
      }
    }
  }
  return
}
// -----
// one K coordinate on two loops: C[i][j] += A[i][4k0+k1] * B[4k0+k1][j]
// R8:        A rows: {{\[\[}}1, 0, 0, 0], [0, 0, 4, 1]]
// R8-NEXT:   B rows: {{\[\[}}0, 0, 4, 1], [0, 1, 0, 0]]
// R8-NEXT:   C rows: {{\[\[}}1, 0, 0, 0], [0, 1, 0, 0]]
// R8-NEXT:   loop 0: readers A C -> M or OB
// R8-NEXT:   loop 1: readers B C -> N or OB
// R8-NEXT:   loop 2: readers A B -> K or RB (contraction block)
// R8-NEXT:   loop 3: readers A B -> K or RB (contraction block)
// R8-NEXT:   reduction loops Z: 2 3; shared loops S: 2 3; groups: {2 3}
// R8-NEXT:   on Z: rank A 1, rank B 1, rank [A; B] 1 (row spaces meet in dimension 1), coupling rank(A B^T) 1
// R8-NEXT:   batch block coupling: 0
// R8-NEXT:   literal, loops of two or more iterations: rank(A B^T) 1, rank(A C^T) 1, rank(B C^T) 1; nonzero A.B pairs 1 (not a dimension)
// R8-NEXT:   operand ranks: A 2, B 2, C 2
// R8-NEXT:   intersect(A,B) (some a.b != 0 on Z): yes; shared loop variables (some sum |a_t||b_t| > 0): yes
// R8:        dense logical GEMM: yes, I/J/K on loops 0/1/2+3
func.func @merged_k(%A: memref<4x12xf32> {llvm.noalias}, %B: memref<12x5xf32> {llvm.noalias}, %C: memref<4x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 5 {
      affine.for %k0 = 0 to 3 {
        affine.for %k1 = 0 to 4 {
          %va = affine.load %A[%i, 4 * %k0 + %k1] : memref<4x12xf32>
          %vb = affine.load %B[4 * %k0 + %k1, %j] : memref<12x5xf32>
          %vc = affine.load %C[%i, %j] : memref<4x5xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %vc, %prod : f32
          affine.store %sum, %C[%i, %j] : memref<4x5xf32>
        }
      }
    }
  }
  return
}
// -----
// rank-1 coupling, non-dense coordinate (stays refused): C[i][j] += A[i][k0+k1] * B[k0+k1][j]
// R8:        A rows: {{\[\[}}1, 0, 0, 0], [0, 0, 1, 1]]
// R8-NEXT:   B rows: {{\[\[}}0, 0, 1, 1], [0, 1, 0, 0]]
// R8-NEXT:   C rows: {{\[\[}}1, 0, 0, 0], [0, 1, 0, 0]]
// R8-NEXT:   loop 0: readers A C -> M or OB
// R8-NEXT:   loop 1: readers B C -> N or OB
// R8-NEXT:   loop 2: readers A B -> K or RB (contraction block)
// R8-NEXT:   loop 3: readers A B -> K or RB (contraction block)
// R8-NEXT:   reduction loops Z: 2 3; shared loops S: 2 3; groups: {2 3}
// R8-NEXT:   on Z: rank A 1, rank B 1, rank [A; B] 1 (row spaces meet in dimension 1), coupling rank(A B^T) 1
// R8-NEXT:   batch block coupling: 0
// R8-NEXT:   literal, loops of two or more iterations: rank(A B^T) 1, rank(A C^T) 1, rank(B C^T) 1; nonzero A.B pairs 1 (not a dimension)
// R8-NEXT:   operand ranks: A 2, B 2, C 2
// R8-NEXT:   intersect(A,B) (some a.b != 0 on Z): yes; shared loop variables (some sum |a_t||b_t| > 0): yes
// R8:        dense logical GEMM: no (logical coordinate K is driven by 2 loops that do not enumerate it densely (scales must grow by the finer loops' extents; with all but one of them as batch loops it would be a batched GEMM, which is not constructed))
func.func @noninjective(%A: memref<4x5xf32> {llvm.noalias}, %B: memref<5x5xf32> {llvm.noalias}, %C: memref<4x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 5 {
      affine.for %k0 = 0 to 3 {
        affine.for %k1 = 0 to 3 {
          %va = affine.load %A[%i, %k0 + %k1] : memref<4x5xf32>
          %vb = affine.load %B[%k0 + %k1, %j] : memref<5x5xf32>
          %vc = affine.load %C[%i, %j] : memref<4x5xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %vc, %prod : f32
          affine.store %sum, %C[%i, %j] : memref<4x5xf32>
        }
      }
    }
  }
  return
}
// -----
// E1 holds, E2 fails: C[i][j] += A[i][k0+2k1] * B[k0+k1][j]
// R8:        A rows: {{\[\[}}1, 0, 0, 0], [0, 0, 1, 2]]
// R8-NEXT:   B rows: {{\[\[}}0, 0, 1, 1], [0, 1, 0, 0]]
// R8-NEXT:   C rows: {{\[\[}}1, 0, 0, 0], [0, 1, 0, 0]]
// R8-NEXT:   loop 0: readers A C -> M or OB
// R8-NEXT:   loop 1: readers B C -> N or OB
// R8-NEXT:   loop 2: readers A B -> K or RB (contraction block)
// R8-NEXT:   loop 3: readers A B -> K or RB (contraction block)
// R8-NEXT:   reduction loops Z: 2 3; shared loops S: 2 3; groups: {2 3}
// R8-NEXT:   on Z: rank A 1, rank B 1, rank [A; B] 2 (row spaces meet in dimension 0), coupling rank(A B^T) 1
// R8-NEXT:   batch block coupling: 0
// R8-NEXT:   literal, loops of two or more iterations: rank(A B^T) 1, rank(A C^T) 1, rank(B C^T) 1; nonzero A.B pairs 1 (not a dimension)
// R8-NEXT:   operand ranks: A 2, B 2, C 2
// R8-NEXT:   intersect(A,B) (some a.b != 0 on Z): yes; shared loop variables (some sum |a_t||b_t| > 0): yes
// R8-NEXT: recognition result: failure
// R8-NEXT:   reason: logical coordinate K (the first factor's dimension 1 d2 + d3 * 2, the second factor's dime
// R8-NEXT:   class: UNKNOWN
func.func @coupled_k(%A: memref<4x6xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<4x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 5 {
      affine.for %k0 = 0 to 2 {
        affine.for %k1 = 0 to 3 {
          %va = affine.load %A[%i, %k0 + 2 * %k1] : memref<4x6xf32>
          %vb = affine.load %B[%k0 + %k1, %j] : memref<4x5xf32>
          %vc = affine.load %C[%i, %j] : memref<4x5xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %vc, %prod : f32
          affine.store %sum, %C[%i, %j] : memref<4x5xf32>
        }
      }
    }
  }
  return
}
// -----
// rank A 2, rank B 1 on S (E1 and E2 fail): C[i][j] += A[i][x0][x1] * B[x0+x1][j]
// R8:        A rows: {{\[\[}}1, 0, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]]
// R8-NEXT:   B rows: {{\[\[}}0, 0, 1, 1], [0, 1, 0, 0]]
// R8-NEXT:   C rows: {{\[\[}}1, 0, 0, 0], [0, 1, 0, 0]]
// R8-NEXT:   loop 0: readers A C -> M or OB
// R8-NEXT:   loop 1: readers B C -> N or OB
// R8-NEXT:   loop 2: readers A B -> K or RB (contraction block)
// R8-NEXT:   loop 3: readers A B -> K or RB (contraction block)
// R8-NEXT:   reduction loops Z: 2 3; shared loops S: 2 3; groups: {2 3}
// R8-NEXT:   on Z: rank A 2, rank B 1, rank [A; B] 2 (row spaces meet in dimension 1), coupling rank(A B^T) 1
// R8-NEXT:   batch block coupling: 0
// R8-NEXT:   literal, loops of two or more iterations: rank(A B^T) 1, rank(A C^T) 1, rank(B C^T) 1; nonzero A.B pairs 2 (not a dimension)
// R8-NEXT:   operand ranks: A 3, B 2, C 2
// R8-NEXT:   intersect(A,B) (some a.b != 0 on Z): yes; shared loop variables (some sum |a_t||b_t| > 0): yes
// R8-NEXT: recognition result: failure
// R8-NEXT:   reason: the first factor reads logical coordinate K through two different combinations of its loop
// R8-NEXT:   class: VALID_BUT_LOWERING_UNSUPPORTED
func.func @conv(%A: memref<4x3x3xf32> {llvm.noalias}, %B: memref<5x5xf32> {llvm.noalias}, %C: memref<4x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 5 {
      affine.for %x0 = 0 to 3 {
        affine.for %x1 = 0 to 3 {
          %va = affine.load %A[%i, %x0, %x1] : memref<4x3x3xf32>
          %vb = affine.load %B[%x0 + %x1, %j] : memref<5x5xf32>
          %vc = affine.load %C[%i, %j] : memref<4x5xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %vc, %prod : f32
          affine.store %sum, %C[%i, %j] : memref<4x5xf32>
        }
      }
    }
  }
  return
}
// -----
// classes cancel in the literal reading: C[b][i][j] += A[b+k][i] * B[b-k+2][j]
// R8:        A rows: {{\[\[}}1, 0, 0, 1], [0, 1, 0, 0]]
// R8-NEXT:   B rows: {{\[\[}}1, 0, 0, -1], [0, 0, 1, 0]]
// R8-NEXT:   C rows: {{\[\[}}1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0]]
// R8-NEXT:   loop 0: readers A B C -> OB (batch)
// R8-NEXT:   loop 1: readers A C -> M or OB
// R8-NEXT:   loop 2: readers B C -> N or OB
// R8-NEXT:   loop 3: readers A B -> K or RB (contraction block)
// R8-NEXT:   reduction loops Z: 3; shared loops S: 3; groups: {3}
// R8-NEXT:   on Z: rank A 1, rank B 1, rank [A; B] 1 (row spaces meet in dimension 1), coupling rank(A B^T) 1
// R8-NEXT:   batch block coupling: 1
// R8-NEXT:   literal, loops of two or more iterations: rank(A B^T) 0, rank(A C^T) 2, rank(B C^T) 2; nonzero A.B pairs 0 (not a dimension)
// R8-NEXT:   literal and restricted readings differ: dot products of different loop classes cancel or add (section 22.4)
// R8-NEXT:   operand ranks: A 2, B 2, C 3
// R8-NEXT:   intersect(A,B) (some a.b != 0 on Z): yes; shared loop variables (some sum |a_t||b_t| > 0): yes
// R8:        dense logical GEMM: yes, I/J/K on loops 1/2/3, batched over loop(s) 0 (2 batch iteration(s))
func.func @cancel(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<2x4x5xf32> {llvm.noalias}) {
  affine.for %b = 0 to 2 {
    affine.for %i = 0 to 4 {
      affine.for %j = 0 to 5 {
        affine.for %k = 0 to 3 {
          %va = affine.load %A[%b + %k, %i] : memref<4x4xf32>
          %vb = affine.load %B[%b - %k + 2, %j] : memref<4x5xf32>
          %vc = affine.load %C[%b, %i, %j] : memref<2x4x5xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %vc, %prod : f32
          affine.store %sum, %C[%b, %i, %j] : memref<2x4x5xf32>
        }
      }
    }
  }
  return
}
// -----
// invalid dimension mapping: C[i][i] += A[i][k] * B[k][j]
// R8:        A rows: {{\[\[}}1, 0, 0], [0, 0, 1]]
// R8-NEXT:   B rows: {{\[\[}}0, 0, 1], [0, 1, 0]]
// R8-NEXT:   C rows: {{\[\[}}1, 0, 0], [1, 0, 0]]
// R8-NEXT:   loop 0: readers A C -> M or OB
// R8-NEXT:   loop 1: readers B -> RB (one factor only)
// R8-NEXT:   loop 2: readers A B -> K or RB (contraction block)
// R8-NEXT:   reduction loops Z: 1 2; shared loops S: 2; groups: {2}
// R8-NEXT:   on Z: rank A 1, rank B 2, rank [A; B] 2 (row spaces meet in dimension 1), coupling rank(A B^T) 1
// R8-NEXT:   batch block coupling: 0
// R8-NEXT:   literal, loops of two or more iterations: rank(A B^T) 1, rank(A C^T) 1, rank(B C^T) 0; nonzero A.B pairs 1 (not a dimension)
// R8-NEXT:   operand ranks: A 2, B 2, C 1; one-dimensional (Phase 2): C
// R8-NEXT:   intersect(A,B) (some a.b != 0 on Z): yes; shared loop variables (some sum |a_t||b_t| > 0): yes
// R8-NEXT: recognition result: failure
// R8-NEXT:   reason: a coordinate is read by the second factor only: the other operands would be broadcast alon
// R8-NEXT:   class: VALID_BUT_LOWERING_UNSUPPORTED
func.func @diag_c(%A: memref<4x3xf32> {llvm.noalias}, %B: memref<3x5xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 3 {
        %va = affine.load %A[%i, %k] : memref<4x3xf32>
        %vb = affine.load %B[%k, %j] : memref<3x5xf32>
        %vc = affine.load %C[%i, %i] : memref<4x4xf32>
        %prod = arith.mulf %va, %vb : f32
        %sum = arith.addf %vc, %prod : f32
        affine.store %sum, %C[%i, %i] : memref<4x4xf32>
      }
    }
  }
  return
}
// -----
// GEMV, one-dimensional factor: y[i] += M[i][k] * x[k]
// R8:        A rows: {{\[\[}}1, 0], [0, 1]]
// R8-NEXT:   B rows: {{\[\[}}0, 1]]
// R8-NEXT:   C rows: {{\[\[}}1, 0]]
// R8-NEXT:   loop 0: readers A C -> M or OB
// R8-NEXT:   loop 1: readers A B -> K or RB (contraction block)
// R8-NEXT:   reduction loops Z: 1; shared loops S: 1; groups: {1}
// R8-NEXT:   on Z: rank A 1, rank B 1, rank [A; B] 1 (row spaces meet in dimension 1), coupling rank(A B^T) 1
// R8-NEXT:   batch block coupling: 0
// R8-NEXT:   literal, loops of two or more iterations: rank(A B^T) 1, rank(A C^T) 1, rank(B C^T) 0; nonzero A.B pairs 1 (not a dimension)
// R8-NEXT:   operand ranks: A 2, B 1, C 1; one-dimensional (Phase 2): B C
// R8-NEXT:   intersect(A,B) (some a.b != 0 on Z): yes; shared loop variables (some sum |a_t||b_t| > 0): yes
// R8:        dense logical GEMM: yes, I/J/K on loops 0//1
func.func @gemv(%A: memref<4x3xf32> {llvm.noalias}, %B: memref<3xf32> {llvm.noalias}, %C: memref<4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %k = 0 to 3 {
      %va = affine.load %A[%i, %k] : memref<4x3xf32>
      %vb = affine.load %B[%k] : memref<3xf32>
      %vc = affine.load %C[%i] : memref<4xf32>
      %prod = arith.mulf %va, %vb : f32
      %sum = arith.addf %vc, %prod : f32
      affine.store %sum, %C[%i] : memref<4xf32>
    }
  }
  return
}
// -----
// C reads three loops through two dimensions: C[i+p][j] += A[p][i][k] * B[p][k][j]
// R8:        A rows: {{\[\[}}1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 0, 1]]
// R8-NEXT:   B rows: {{\[\[}}1, 0, 0, 0], [0, 0, 0, 1], [0, 0, 1, 0]]
// R8-NEXT:   C rows: {{\[\[}}1, 1, 0, 0], [0, 0, 1, 0]]
// R8-NEXT:   loop 0: readers A B C -> OB (batch)
// R8-NEXT:   loop 1: readers A C -> M or OB
// R8-NEXT:   loop 2: readers B C -> N or OB
// R8-NEXT:   loop 3: readers A B -> K or RB (contraction block)
// R8-NEXT:   reduction loops Z: 3; shared loops S: 3; groups: {3}
// R8-NEXT:   on Z: rank A 1, rank B 1, rank [A; B] 1 (row spaces meet in dimension 1), coupling rank(A B^T) 1
// R8-NEXT:   batch block coupling: 1
// R8-NEXT:   literal, loops of two or more iterations: rank(A B^T) 2, rank(A C^T) 1, rank(B C^T) 2; nonzero A.B pairs 2 (not a dimension)
// R8-NEXT:   operand ranks: A 3, B 3, C 2
// R8-NEXT:   intersect(A,B) (some a.b != 0 on Z): yes; shared loop variables (some sum |a_t||b_t| > 0): yes
// R8:        dense logical GEMM: yes, I/J/K on loops 1/2/3, batched over loop(s) 0 (2 batch iteration(s))
func.func @overlap_c(%A: memref<2x4x3xf32> {llvm.noalias}, %B: memref<2x3x5xf32> {llvm.noalias}, %C: memref<5x5xf32> {llvm.noalias}) {
  affine.for %p = 0 to 2 {
    affine.for %i = 0 to 4 {
      affine.for %j = 0 to 5 {
        affine.for %k = 0 to 3 {
          %va = affine.load %A[%p, %i, %k] : memref<2x4x3xf32>
          %vb = affine.load %B[%p, %k, %j] : memref<2x3x5xf32>
          %vc = affine.load %C[%i + %p, %j] : memref<5x5xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %vc, %prod : f32
          affine.store %sum, %C[%i + %p, %j] : memref<5x5xf32>
        }
      }
    }
  }
  return
}
// -----
// a loop of one iteration is a constant: C[i][j] += A[i][k+r] * B[k][j], r in [0,1)
// R8:        A rows: {{\[\[}}1, 0, 0, 0], [0, 0, 1, 1]]
// R8-NEXT:   B rows: {{\[\[}}0, 0, 1, 0], [0, 1, 0, 0]]
// R8-NEXT:   C rows: {{\[\[}}1, 0, 0, 0], [0, 1, 0, 0]]
// R8-NEXT:   loop 0: readers A C -> M or OB
// R8-NEXT:   loop 1: readers B C -> N or OB
// R8-NEXT:   loop 2: readers A B -> K or RB (contraction block)
// R8-NEXT:   loop 3: one iteration (a constant)
// R8-NEXT:   reduction loops Z: 2; shared loops S: 2; groups: {2}
// R8-NEXT:   on Z: rank A 1, rank B 1, rank [A; B] 1 (row spaces meet in dimension 1), coupling rank(A B^T) 1
// R8-NEXT:   batch block coupling: 0
// R8-NEXT:   literal, loops of two or more iterations: rank(A B^T) 1, rank(A C^T) 1, rank(B C^T) 1; nonzero A.B pairs 1 (not a dimension)
// R8-NEXT:   operand ranks: A 2, B 2, C 2
// R8-NEXT:   intersect(A,B) (some a.b != 0 on Z): yes; shared loop variables (some sum |a_t||b_t| > 0): yes
// R8:        dense logical GEMM: yes, I/J/K on loops 0/1/2
func.func @once(%A: memref<4x3xf32> {llvm.noalias}, %B: memref<3x5xf32> {llvm.noalias}, %C: memref<4x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 3 {
        affine.for %r = 0 to 1 {
          %va = affine.load %A[%i, %k + %r] : memref<4x3xf32>
          %vb = affine.load %B[%k, %j] : memref<3x5xf32>
          %vc = affine.load %C[%i, %j] : memref<4x5xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %vc, %prod : f32
          affine.store %sum, %C[%i, %j] : memref<4x5xf32>
        }
      }
    }
  }
  return
}
// -----
// A parallelotope domain (k in [i, i+3)): the domain normalization rewrites the counters
// (x = U x'), so R8 -- defined on the counters as written -- is not analysed.
// R8:        not analysed: the domain normalization rewrote the counters of this nest (x = U x'), they are not the counters as written
// R8:        dense logical GEMM: yes, I/J/K on loops /1/2, batched over loop(s) 0 (4 batch iteration(s))
func.func @normalized(%A: memref<4x8xf32> {llvm.noalias}, %B: memref<8x5xf32> {llvm.noalias}, %C: memref<4x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 5 {
      affine.for %k = affine_map<(d0) -> (d0)>(%i) to affine_map<(d0) -> (d0 + 3)>(%i) {
        %va = affine.load %A[%i, %k] : memref<4x8xf32>
        %vb = affine.load %B[%k, %j] : memref<8x5xf32>
        %vc = affine.load %C[%i, %j] : memref<4x5xf32>
        %prod = arith.mulf %va, %vb : f32
        %sum = arith.addf %vc, %prod : f32
        affine.store %sum, %C[%i, %j] : memref<4x5xf32>
      }
    }
  }
  return
}
// -----
// A loop used by no access (r): printed as such; Rule E does not look at it.
// R8:        A rows: {{\[\[}}1, 0, 0, 0], [0, 0, 1, 0]]
// R8-NEXT:   B rows: {{\[\[}}0, 0, 1, 0], [0, 1, 0, 0]]
// R8-NEXT:   C rows: {{\[\[}}1, 0, 0, 0], [0, 1, 0, 0]]
// R8-NEXT:   loop 0: readers A C -> M or OB
// R8-NEXT:   loop 1: readers B C -> N or OB
// R8-NEXT:   loop 2: readers A B -> K or RB (contraction block)
// R8-NEXT:   loop 3: used by no access
// R8-NEXT:   reduction loops Z: 2; shared loops S: 2; groups: {2}
// R8-NEXT:   on Z: rank A 1, rank B 1, rank [A; B] 1 (row spaces meet in dimension 1), coupling rank(A B^T) 1
// R8-NEXT:   batch block coupling: 0
// R8-NEXT:   literal, loops of two or more iterations: rank(A B^T) 1, rank(A C^T) 1, rank(B C^T) 1; nonzero A.B pairs 1 (not a dimension)
// R8-NEXT:   operand ranks: A 2, B 2, C 2
// R8-NEXT:   intersect(A,B) (some a.b != 0 on Z): yes; shared loop variables (some sum |a_t||b_t| > 0): yes
// R8-NEXT: recognition result: failure
// R8-NEXT:   reason: loop 3 of the nest is used by no access (it repeats the accumulation; moved outward by the
// R8-NEXT:   class: UNKNOWN
func.func @unused(%A: memref<4x3xf32> {llvm.noalias}, %B: memref<3x5xf32> {llvm.noalias}, %C: memref<4x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 3 {
        affine.for %r = 0 to 2 {
          %va = affine.load %A[%i, %k] : memref<4x3xf32>
          %vb = affine.load %B[%k, %j] : memref<3x5xf32>
          %vc = affine.load %C[%i, %j] : memref<4x5xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %vc, %prod : f32
          affine.store %sum, %C[%i, %j] : memref<4x5xf32>
        }
      }
    }
  }
  return
}
// -----
// An exact normalization with U = I (k < min(8, i + 8) is k < 8 on the box): the counters stay
// the counters as written, so R8 is analysed.
// R8:        A rows: {{\[\[}}1, 0, 0], [0, 0, 1]]
// R8-NEXT:   B rows: {{\[\[}}0, 0, 1], [0, 1, 0]]
// R8-NEXT:   C rows: {{\[\[}}1, 0, 0], [0, 1, 0]]
// R8-NEXT:   loop 0: readers A C -> M or OB
// R8-NEXT:   loop 1: readers B C -> N or OB
// R8-NEXT:   loop 2: readers A B -> K or RB (contraction block)
// R8-NEXT:   reduction loops Z: 2; shared loops S: 2; groups: {2}
// R8-NEXT:   on Z: rank A 1, rank B 1, rank [A; B] 1 (row spaces meet in dimension 1), coupling rank(A B^T) 1
// R8-NEXT:   batch block coupling: 0
// R8-NEXT:   literal, loops of two or more iterations: rank(A B^T) 1, rank(A C^T) 1, rank(B C^T) 1; nonzero A.B pairs 1 (not a dimension)
// R8-NEXT:   operand ranks: A 2, B 2, C 2
// R8-NEXT:   intersect(A,B) (some a.b != 0 on Z): yes; shared loop variables (some sum |a_t||b_t| > 0): yes
// R8:        dense logical GEMM: yes, I/J/K on loops 0/1/2
func.func @identity_normalized(%A: memref<4x8xf32> {llvm.noalias}, %B: memref<8x5xf32> {llvm.noalias}, %C: memref<4x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to min affine_map<(d0) -> (8, d0 + 8)>(%i) {
        %va = affine.load %A[%i, %k] : memref<4x8xf32>
        %vb = affine.load %B[%k, %j] : memref<8x5xf32>
        %vc = affine.load %C[%i, %j] : memref<4x5xf32>
        %prod = arith.mulf %va, %vb : f32
        %sum = arith.addf %vc, %prod : f32
        affine.store %sum, %C[%i, %j] : memref<4x5xf32>
      }
    }
  }
  return
}
// -----
// Large proportional coefficients (2^32 k on both factors): the rows are divided by their gcd
// before the dot products, so the Gram matrix does not overflow.
// R8:        A rows: {{\[\[}}1, 0, 0], [0, 0, 4294967296]]
// R8-NEXT:   B rows: {{\[\[}}0, 0, 4294967296], [0, 1, 0]]
// R8-NEXT:   C rows: {{\[\[}}1, 0, 0], [0, 1, 0]]
// R8-NEXT:   loop 0: readers A C -> M or OB
// R8-NEXT:   loop 1: readers B C -> N or OB
// R8-NEXT:   loop 2: readers A B -> K or RB (contraction block)
// R8-NEXT:   reduction loops Z: 2; shared loops S: 2; groups: {2}
// R8-NEXT:   on Z: rank A 1, rank B 1, rank [A; B] 1 (row spaces meet in dimension 1), coupling rank(A B^T) 1
// R8-NEXT:   batch block coupling: 0
// R8-NEXT:   literal, loops of two or more iterations: rank(A B^T) 1, rank(A C^T) 1, rank(B C^T) 1; nonzero A.B pairs 1 (not a dimension)
// R8-NEXT:   operand ranks: A 2, B 2, C 2
// R8-NEXT:   intersect(A,B) (some a.b != 0 on Z): yes; shared loop variables (some sum |a_t||b_t| > 0): yes
// R8:        dense logical GEMM: yes, I/J/K on loops 0/1/2
func.func @large_coefficients(%A: memref<4x17179869184xf32> {llvm.noalias}, %B: memref<17179869184x5xf32> {llvm.noalias}, %C: memref<4x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 3 {
        %va = affine.load %A[%i, %k * 4294967296] : memref<4x17179869184xf32>
        %vb = affine.load %B[%k * 4294967296, %j] : memref<17179869184x5xf32>
        %vc = affine.load %C[%i, %j] : memref<4x5xf32>
        %prod = arith.mulf %va, %vb : f32
        %sum = arith.addf %vc, %prod : f32
        affine.store %sum, %C[%i, %j] : memref<4x5xf32>
      }
    }
  }
  return
}
