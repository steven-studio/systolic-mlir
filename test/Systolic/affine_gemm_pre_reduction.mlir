// Pre-reduction of the factors (docs/AFFINE_GEMM_MAPPING.md section 24,
// Lemma 7.6), option fp-distribute of --affine-matmul-to-systolic (off by
// default; systolic-cc --fp-distribute). In an accumulation C += A * B on a box
// as written whose factors share no loop C does not read (L_AB empty), the
// loops read by one factor only are summed into that factor first:
//   sum_{z_A, z_B} A(o, z_A) B(o, z_B) = (sum_{z_A} A(o, z_A)) (sum_{z_B} B(o, z_B))
// A' (indexed by the loops A and C read) and B' are fresh buffers, zeroed and
// summed on the host; the nest becomes C += A' * B' over the loops C reads,
// its accumulation marked reassoc. fp-distribute is the permission to take
// floating point as commutative, associative and distributive: the result is
// NOT bitwise that of the loops as written (the numerical differential test is
// test/SystolicCC/affine_pre_reduction.c). allow-fp-reassociation does not
// imply it.
//
// The premises (the NOT cases below each break one): the multiply-accumulate
// body; constant bounds and a nonempty domain (a box); no loop read by both
// factors and not by C; at least one loop read by one factor only; A and B
// provably disjoint from C; unit steps on the loops that index A' and B' (a
// summed loop may be stepped). Assignments C = A * B are never pre-reduced
// (last write wins applies to them instead, section 20.5, and only with the
// permission to reassociate).
//
// The checks pin, in order: the remarks with fp-distribute (prefix REMARK, the
// pre-reduction, then the GEMM it leaves), the IR (prefix CHECK, the buffers,
// their zeroing and summing loops, the K = 1 tile; no memref.dealloc: the
// pipeline's buffer deallocation frees A' and B'), the
// lowering without fp-distribute (prefix OFF, the reduction-batch lowering of
// section 23, unchanged), fp-distribute without allow-fp-reassociation (prefix
// DIST, the derived nest carries reassoc, so it lowers; the nests left alone
// are refused as before), and no option at all (prefix STRICT, strict floating
// point, nothing pre-reduced, every accumulation refused).
//
// RUN: systolic-opt "--affine-matmul-to-systolic=report=true allow-fp-reassociation=true classify=true fp-distribute=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=REMARK
// RUN: systolic-opt "--affine-matmul-to-systolic=allow-fp-reassociation=true fp-distribute=true" \
// RUN:   -split-input-file %s 2>/dev/null | FileCheck %s
// RUN: systolic-opt "--affine-matmul-to-systolic=report=true allow-fp-reassociation=true classify=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=OFF
// RUN: systolic-opt "--affine-matmul-to-systolic=report=true classify=true fp-distribute=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=DIST
// RUN: systolic-opt "--affine-matmul-to-systolic=report=true classify=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=STRICT
// OFF-NOT: pre-reduced
// STRICT-NOT: pre-reduced
// STRICT-NOT: lowered as
// T5: C[i][j] += A[i][k] * B[l][j]: both factors pre-reduced (A' over k, B' over l), then a K = 1 GEMM
// REMARK:      remark: affine multiply-accumulate nest pre-reduced (--fp-distribute, docs/AFFINE_GEMM_MAPPING.md section 24): the first factor summed over loop(s) 2, the second factor summed over loop(s) 3 before the GEMM (no loop is read by both factors and not by C)
// REMARK:      remark: affine GEMM 3x5x1 lowered as 1 exact region(s){{$}}
// REMARK:      arith.addf {{.*}} fastmath<reassoc> : f32
// CHECK-LABEL: func.func @t5(
// CHECK:       %[[AP:.*]] = memref.alloc() : memref<3xf32>
// CHECK:       %[[Z:.*]] = arith.constant 0.000000e+00 : f32
// CHECK:       affine.for %[[I:.*]] = 0 to 3 {
// CHECK-NEXT:    affine.store %[[Z]], %[[AP]][%[[I]]] : memref<3xf32>
// CHECK:       affine.for %[[I:.*]] = 0 to 3 {
// CHECK-NEXT:    affine.for %[[K:.*]] = 0 to 4 {
// CHECK-NEXT:      %[[T:.*]] = affine.load %arg0[%[[I]], %[[K]]] : memref<3x4xf32>
// CHECK-NEXT:      %[[P:.*]] = affine.load %[[AP]][%[[I]]] : memref<3xf32>
// CHECK-NEXT:      %[[S:.*]] = arith.addf %[[P]], %[[T]] : f32
// CHECK-NEXT:      affine.store %[[S]], %[[AP]][%[[I]]] : memref<3xf32>
// CHECK:       %[[BP:.*]] = memref.alloc() : memref<5xf32>
// CHECK:       affine.store %cst{{.*}}, %[[BP]][{{.*}}] : memref<5xf32>
// CHECK:       affine.for %[[J:.*]] = 0 to 5 {
// CHECK-NEXT:    affine.for %[[L:.*]] = 0 to 4 {
// CHECK-NEXT:      %[[T:.*]] = affine.load %arg1[%[[L]], %[[J]]] : memref<4x5xf32>
// CHECK-NEXT:      %[[P:.*]] = affine.load %[[BP]][%[[J]]] : memref<5xf32>
// CHECK-NEXT:      %[[S:.*]] = arith.addf %[[P]], %[[T]] : f32
// CHECK-NEXT:      affine.store %[[S]], %[[BP]][%[[J]]] : memref<5xf32>
// CHECK:       memref.subview %[[AP]][0] [3] [1]
// CHECK:       memref.subview %[[BP]][0] [5] [1]
// CHECK:       systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1 :
// CHECK-NOT:   memref.dealloc
// CHECK:       return
// OFF:         remark: affine GEMM 3x5x1 lowered as 1 exact region(s), 2 reduction batch loop(s) (not read by C, accumulated into C in order), batched: 16 batch iteration(s) of 2 batch loop(s); the enclosing loops run it 16 time(s)
// DIST:        remark: affine multiply-accumulate nest pre-reduced (--fp-distribute, docs/AFFINE_GEMM_MAPPING.md section 24): the first factor summed over loop(s) 2, the second factor summed over loop(s) 3 before
// DIST:        remark: affine GEMM 3x5x1 lowered as 1 exact region(s){{$}}
// DIST:        arith.addf {{.*}} fastmath<reassoc> : f32
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-point reassociation is not permitted
func.func @t5(%A: memref<3x4xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 4 {
        affine.for %l = 0 to 4 {
          %va = affine.load %A[%i, %k] : memref<3x4xf32>
          %vb = affine.load %B[%l, %j] : memref<4x5xf32>
          %vc = affine.load %C[%i, %j] : memref<3x5xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %vc, %prod : f32
          affine.store %sum, %C[%i, %j] : memref<3x5xf32>
        }
      }
    }
  }
  return
}
// -----
// only B pre-reduced: C[i][j] += A[i] * B[l][j] (l read by B only; A has no summed loop and is used as it is)
// REMARK:      remark: affine multiply-accumulate nest pre-reduced (--fp-distribute, docs/AFFINE_GEMM_MAPPING.md section 24): the first factor summed over loop(s) none, the second factor summed over loop(s) 2 before
// REMARK:      remark: affine GEMM 3x5x1 lowered as 1 exact region(s){{$}}
// CHECK-LABEL: func.func @b_only(
// CHECK:       %[[BP:.*]] = memref.alloc() : memref<5xf32>
// CHECK-NOT:   memref.alloc
// CHECK:       arith.addf
// CHECK:       memref.subview %arg0[0] [3] [1]
// CHECK:       memref.subview %[[BP]][0] [5] [1]
// CHECK:       systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1 :
// CHECK-NOT:   memref.dealloc
// CHECK:       return
// OFF:         remark: affine GEMM 3x5x1 lowered as 1 exact region(s), 1 reduction batch loop(s) (not read by C, accumulated into C in order), batched: 4 batch iteration(s) of 1 batch loop(s); the enclosing loops run it 4 time(s)
// DIST:        remark: affine multiply-accumulate nest pre-reduced (--fp-distribute, docs/AFFINE_GEMM_MAPPING.md section 24): the first factor summed over loop(s) none, the second factor summed over loop(s) 2 before
// DIST:        remark: affine GEMM 3x5x1 lowered as 1 exact region(s){{$}}
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-point reassociation is not permitted
func.func @b_only(%A: memref<3xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %l = 0 to 4 {
        %va = affine.load %A[%i] : memref<3xf32>
        %vb = affine.load %B[%l, %j] : memref<4x5xf32>
        %vc = affine.load %C[%i, %j] : memref<3x5xf32>
        %prod = arith.mulf %va, %vb : f32
        %sum = arith.addf %vc, %prod : f32
        affine.store %sum, %C[%i, %j] : memref<3x5xf32>
      }
    }
  }
  return
}
// -----
// only B pre-reduced, C[i][k] += A[i][k] * B[k][j]: B'[k] = sum_j B[k][j]; k (read by A, B' and C) becomes an output batch loop
// REMARK:      remark: affine multiply-accumulate nest pre-reduced (--fp-distribute, docs/AFFINE_GEMM_MAPPING.md section 24): the first factor summed over loop(s) none, the second factor summed over loop(s) 2 before
// REMARK:      remark: affine GEMM 3x1x1 lowered as 1 exact region(s), batched: 4 batch iteration(s) of 1 batch loop(s); the enclosing loops run it 4 time(s)
// CHECK-LABEL: func.func @b_only_ik(
// CHECK:       %[[BP:.*]] = memref.alloc() : memref<4xf32>
// CHECK:       affine.store %cst{{.*}}, %[[BP]][{{.*}}] : memref<4xf32>
// CHECK:       affine.for %[[K:.*]] = 0 to 4 {
// CHECK-NEXT:    affine.for %[[J:.*]] = 0 to 5 {
// CHECK-NEXT:      affine.load %arg1[%[[K]], %[[J]]] : memref<4x5xf32>
// CHECK:       affine.for
// CHECK:       memref.subview %[[BP]]
// CHECK:       systolic.matmul_tile {{.*}} m = 3 n = 1 k = 1 :
// CHECK-NOT:   memref.dealloc
// CHECK:       return
// OFF:         remark: affine GEMM 3x1x1 lowered as 1 exact region(s), 1 reduction batch loop(s) (not read by C, accumulated into C in order), batched: 20 batch iteration(s) of 2 batch loop(s); the enclosing loops run it 20 time(s)
// DIST:        remark: affine multiply-accumulate nest pre-reduced (--fp-distribute, docs/AFFINE_GEMM_MAPPING.md section 24): the first factor summed over loop(s) none, the second factor summed over loop(s) 2 before
// DIST:        remark: affine GEMM 3x1x1 lowered as
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-point reassociation is not permitted
func.func @b_only_ik(%A: memref<3x4xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<3x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %k = 0 to 4 {
      affine.for %j = 0 to 5 {
        %va = affine.load %A[%i, %k] : memref<3x4xf32>
        %vb = affine.load %B[%k, %j] : memref<4x5xf32>
        %vc = affine.load %C[%i, %k] : memref<3x4xf32>
        %prod = arith.mulf %va, %vb : f32
        %sum = arith.addf %vc, %prod : f32
        affine.store %sum, %C[%i, %k] : memref<3x4xf32>
      }
    }
  }
  return
}
// -----
// only A pre-reduced, over two loops (s, k), and the product first in the sum (C = A * B + C): A'[i] = sum over s, k of A[i][s][k]
// REMARK:      remark: affine multiply-accumulate nest pre-reduced (--fp-distribute, docs/AFFINE_GEMM_MAPPING.md section 24): the first factor summed over loop(s) 2+3, the second factor summed over loop(s) none before
// REMARK:      remark: affine GEMM 3x5x1 lowered as 1 exact region(s){{$}}
// CHECK-LABEL: func.func @a_only(
// CHECK:       %[[AP:.*]] = memref.alloc() : memref<3xf32>
// CHECK:       affine.store %cst{{.*}}, %[[AP]][{{.*}}] : memref<3xf32>
// CHECK-NOT:   memref.alloc
// CHECK:       affine.for %[[I:.*]] = 0 to 3 {
// CHECK-NEXT:    affine.for %[[S:.*]] = 0 to 2 {
// CHECK-NEXT:      affine.for %[[K:.*]] = 0 to 4 {
// CHECK-NEXT:        affine.load %arg0[%[[I]], %[[S]], %[[K]]] : memref<3x2x4xf32>
// CHECK-NEXT:        affine.load %[[AP]][%[[I]]] : memref<3xf32>
// CHECK-NEXT:        arith.addf
// CHECK-NEXT:        affine.store {{.*}}, %[[AP]][%[[I]]] : memref<3xf32>
// CHECK:       memref.subview %[[AP]][0] [3] [1]
// CHECK:       memref.subview %arg1[0] [5] [1]
// CHECK:       systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1 :
// CHECK-NOT:   memref.dealloc
// CHECK:       return
// OFF:         remark: affine GEMM 3x5x1 lowered as 1 exact region(s), 2 reduction batch loop(s) (not read by C, accumulated into C in order), batched: 8 batch iteration(s) of 2 batch loop(s); the enclosing loops run it 8 time(s)
// DIST:        remark: affine multiply-accumulate nest pre-reduced (--fp-distribute, docs/AFFINE_GEMM_MAPPING.md section 24): the first factor summed over loop(s) 2+3, the second factor summed over loop(s) none before
// DIST:        remark: affine GEMM 3x5x1 lowered as 1 exact region(s){{$}}
// DIST:        arith.addf {{.*}} fastmath<reassoc> : f32
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-point reassociation is not permitted
func.func @a_only(%A: memref<3x2x4xf32> {llvm.noalias}, %B: memref<5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %s = 0 to 2 {
        affine.for %k = 0 to 4 {
          %va = affine.load %A[%i, %s, %k] : memref<3x2x4xf32>
          %vb = affine.load %B[%j] : memref<5xf32>
          %vc = affine.load %C[%i, %j] : memref<3x5xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %prod, %vc : f32
          affine.store %sum, %C[%i, %j] : memref<3x5xf32>
        }
      }
    }
  }
  return
}
// -----
// an output batch loop (b, read by all three) with pre-reduction: A'[b][i] = sum over k of A[b][i][k], B'[b][j] = sum over l of B[b][l][j]
// REMARK:      remark: affine multiply-accumulate nest pre-reduced (--fp-distribute, docs/AFFINE_GEMM_MAPPING.md section 24): the first factor summed over loop(s) 3, the second factor summed over loop(s) 4 before
// REMARK:      remark: affine GEMM 3x5x1 lowered as 1 exact region(s), batched: 2 batch iteration(s) of 1 batch loop(s); the enclosing loops run it 2 time(s)
// CHECK-LABEL: func.func @ob(
// CHECK:       %[[AP:.*]] = memref.alloc() : memref<2x3xf32>
// CHECK:       affine.store %cst{{.*}}, %[[AP]][{{.*}}] : memref<2x3xf32>
// CHECK:       affine.for %[[B:.*]] = 0 to 2 {
// CHECK-NEXT:    affine.for %[[I:.*]] = 0 to 3 {
// CHECK-NEXT:      affine.for %[[K:.*]] = 0 to 4 {
// CHECK-NEXT:        affine.load %arg0[%[[B]], %[[I]], %[[K]]] : memref<2x3x4xf32>
// CHECK-NEXT:        affine.load %[[AP]][%[[B]], %[[I]]] : memref<2x3xf32>
// CHECK:       %[[BP:.*]] = memref.alloc() : memref<2x5xf32>
// CHECK:       affine.store %cst{{.*}}, %[[BP]][{{.*}}] : memref<2x5xf32>
// CHECK:       affine.for %[[B:.*]] = 0 to 2 {
// CHECK-NEXT:    affine.for %[[J:.*]] = 0 to 5 {
// CHECK-NEXT:      affine.for %[[L:.*]] = 0 to 4 {
// CHECK-NEXT:        affine.load %arg1[%[[B]], %[[L]], %[[J]]] : memref<2x4x5xf32>
// CHECK-NEXT:        affine.load %[[BP]][%[[B]], %[[J]]] : memref<2x5xf32>
// CHECK:       affine.for %{{.*}} = 0 to 2 {
// CHECK:         memref.subview %[[AP]]
// CHECK:         memref.subview %[[BP]]
// CHECK:         systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1 :
// CHECK-NOT:   memref.dealloc
// CHECK:       return
// OFF:         remark: affine GEMM 3x5x1 lowered as 1 exact region(s), 2 reduction batch loop(s) (not read by C, accumulated into C in order), batched: 32 batch iteration(s) of 3 batch loop(s); the enclosing loops run it 32 time(s)
// DIST:        remark: affine multiply-accumulate nest pre-reduced (--fp-distribute, docs/AFFINE_GEMM_MAPPING.md section 24): the first factor summed over loop(s) 3, the second factor summed over loop(s) 4 before
// DIST:        remark: affine GEMM 3x5x1 lowered as
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-point reassociation is not permitted
func.func @ob(%A: memref<2x3x4xf32> {llvm.noalias}, %B: memref<2x4x5xf32> {llvm.noalias}, %C: memref<2x3x5xf32> {llvm.noalias}) {
  affine.for %b = 0 to 2 {
    affine.for %i = 0 to 3 {
      affine.for %j = 0 to 5 {
        affine.for %k = 0 to 4 {
          affine.for %l = 0 to 4 {
            %va = affine.load %A[%b, %i, %k] : memref<2x3x4xf32>
            %vb = affine.load %B[%b, %l, %j] : memref<2x4x5xf32>
            %vc = affine.load %C[%b, %i, %j] : memref<2x3x5xf32>
            %prod = arith.mulf %va, %vb : f32
            %sum = arith.addf %vc, %prod : f32
            affine.store %sum, %C[%b, %i, %j] : memref<2x3x5xf32>
          }
        }
      }
    }
  }
  return
}
// -----
// a negative coefficient and constants on the summed loops (A[i][5 - k], B[2l + 1][j + 1]), a kept loop from 1 (i): A' is indexed by i - 1
// REMARK:      remark: affine multiply-accumulate nest pre-reduced (--fp-distribute, docs/AFFINE_GEMM_MAPPING.md section 24): the first factor summed over loop(s) 2, the second factor summed over loop(s) 3 before
// REMARK:      remark: affine GEMM 3x5x1 lowered as 1 exact region(s){{$}}
// CHECK-LABEL: func.func @neg_const(
// CHECK:       %[[AP:.*]] = memref.alloc() : memref<3xf32>
// CHECK:       affine.store %cst{{.*}}, %[[AP]][{{.*}}] : memref<3xf32>
// CHECK:       affine.for %[[I:.*]] = 1 to 4 {
// CHECK-NEXT:    affine.for %[[K:.*]] = 0 to 4 {
// CHECK-NEXT:      affine.load %arg0[%[[I]], -%[[K]] + 5] : memref<4x6xf32>
// CHECK-NEXT:      affine.load %[[AP]][%[[I]] - 1] : memref<3xf32>
// CHECK-NEXT:      arith.addf
// CHECK-NEXT:      affine.store {{.*}}, %[[AP]][%[[I]] - 1] : memref<3xf32>
// CHECK:       %[[BP:.*]] = memref.alloc() : memref<5xf32>
// CHECK:       affine.store %cst{{.*}}, %[[BP]][{{.*}}] : memref<5xf32>
// CHECK:       affine.for %[[J:.*]] = 0 to 5 {
// CHECK-NEXT:    affine.for %[[L:.*]] = 0 to 4 {
// CHECK-NEXT:      affine.load %arg1[%[[L]] * 2 + 1, %[[J]] + 1] : memref<9x6xf32>
// CHECK:       memref.subview %arg2[1, 0] [3, 5] [1, 1]
// CHECK:       systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1 :
// CHECK-NOT:   memref.dealloc
// CHECK:       return
// OFF:         remark: affine GEMM 3x5x1 lowered as 1 exact region(s), 2 reduction batch loop(s) (not read by C, accumulated into C in order), batched: 16 batch iteration(s) of 2 batch loop(s); the enclosing loops run it 16 time(s)
// DIST:        remark: affine multiply-accumulate nest pre-reduced (--fp-distribute, docs/AFFINE_GEMM_MAPPING.md section 24): the first factor summed over loop(s) 2, the second factor summed over loop(s) 3 before
// DIST:        remark: affine GEMM 3x5x1 lowered as
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-point reassociation is not permitted
func.func @neg_const(%A: memref<4x6xf32> {llvm.noalias}, %B: memref<9x6xf32> {llvm.noalias}, %C: memref<4x5xf32> {llvm.noalias}) {
  affine.for %i = 1 to 4 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 4 {
        affine.for %l = 0 to 4 {
          %va = affine.load %A[%i, 5 - %k] : memref<4x6xf32>
          %vb = affine.load %B[2 * %l + 1, %j + 1] : memref<9x6xf32>
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
// a stepped summed loop (l = 0, 2, 4, 6, read by B only): allowed, B'[j] = sum over l of B[l][j]
// REMARK:      remark: affine multiply-accumulate nest pre-reduced (--fp-distribute, docs/AFFINE_GEMM_MAPPING.md section 24): the first factor summed over loop(s) 2, the second factor summed over loop(s) 3 before
// REMARK:      remark: affine GEMM 3x5x1 lowered as 1 exact region(s){{$}}
// CHECK-LABEL: func.func @stepped_sum(
// CHECK:       %[[BP:.*]] = memref.alloc() : memref<5xf32>
// CHECK:       affine.store %cst{{.*}}, %[[BP]][{{.*}}] : memref<5xf32>
// CHECK:       affine.for %[[J:.*]] = 0 to 5 {
// CHECK-NEXT:    affine.for %[[L:.*]] = 0 to 7 step 2 {
// CHECK-NEXT:      affine.load %arg1[%[[L]], %[[J]]] : memref<8x5xf32>
// CHECK:       systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1 :
// CHECK-NOT:   memref.dealloc
// CHECK:       return
// OFF:         remark: affine GEMM 3x5x1 lowered as 1 exact region(s), 2 reduction batch loop(s) (not read by C, accumulated into C in order), batched: 16 batch iteration(s) of 2 batch loop(s); the enclosing loops run it 16 time(s)
// DIST:        remark: affine multiply-accumulate nest pre-reduced (--fp-distribute, docs/AFFINE_GEMM_MAPPING.md section 24): the first factor summed over loop(s) 2, the second factor summed over loop(s) 3 before
// DIST:        remark: affine GEMM 3x5x1 lowered as
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-point reassociation is not permitted
func.func @stepped_sum(%A: memref<3x4xf32> {llvm.noalias}, %B: memref<8x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 4 {
        affine.for %l = 0 to 8 step 2 {
          %va = affine.load %A[%i, %k] : memref<3x4xf32>
          %vb = affine.load %B[%l, %j] : memref<8x5xf32>
          %vc = affine.load %C[%i, %j] : memref<3x5xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %vc, %prod : f32
          affine.store %sum, %C[%i, %j] : memref<3x5xf32>
        }
      }
    }
  }
  return
}
// -----
// NOT pre-reduced -- L_AB nonempty: T2, C[i][j] += A[i][x0 - x1 + 3] * B[x0][j] (x0 read by both factors, not by C): the reduction-batch lowering of section 23, as without fp-distribute
// REMARK-NOT:  pre-reduced
// REMARK:      remark: affine GEMM 3x5x4 lowered as 1 exact region(s), 1 reduction batch loop(s) (not read by C, accumulated into C in order), batched: 4 batch iteration(s) of 1 batch loop(s); the enclosing loops run it 4 time(s)
// CHECK-LABEL: func.func @t2(
// CHECK-NOT:   memref.alloc
// CHECK:       systolic.matmul_tile {{.*}} m = 3 n = 5 k = 4 :
// CHECK-NOT:   memref.dealloc
// CHECK:       return
// OFF:         remark: affine GEMM 3x5x4 lowered as 1 exact region(s), 1 reduction batch loop(s) (not read by C, accumulated into C in order), batched: 4 batch iteration(s) of 1 batch loop(s); the enclosing loops run it 4 time(s)
// DIST-NOT:    pre-reduced
// DIST:        remark: affine GEMM not lowered to systolic.matmul_tile: floating-point reassociation is not permitted
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-point reassociation is not permitted
func.func @t2(%A: memref<3x7xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %x0 = 0 to 4 {
        affine.for %x1 = 0 to 4 {
          %va = affine.load %A[%i, %x0 - %x1 + 3] : memref<3x7xf32>
          %vb = affine.load %B[%x0, %j] : memref<4x5xf32>
          %vc = affine.load %C[%i, %j] : memref<3x5xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %vc, %prod : f32
          affine.store %sum, %C[%i, %j] : memref<3x5xf32>
        }
      }
    }
  }
  return
}
// -----
// NOT pre-reduced -- a stepped loop that would index A' (i, step 2, read by A and C): the reduction-batch lowering
// REMARK-NOT:  pre-reduced
// REMARK:      remark: affine GEMM 3x5x1 lowered as 1 exact region(s), 2 reduction batch loop(s) (not read by C, accumulated into C in order), batched: 16 batch iteration(s) of 2 batch loop(s); the enclosing loops run it 16 time(s)
// CHECK-LABEL: func.func @stepped_keep(
// CHECK-NOT:   memref.alloc
// CHECK:       systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1 :
// CHECK-NOT:   memref.dealloc
// CHECK:       return
// OFF:         remark: affine GEMM 3x5x1 lowered as 1 exact region(s), 2 reduction batch loop(s) (not read by C, accumulated into C in order), batched: 16 batch iteration(s) of 2 batch loop(s); the enclosing loops run it 16 time(s)
// DIST-NOT:    pre-reduced
// DIST:        remark: affine GEMM not lowered to systolic.matmul_tile: floating-point reassociation is not permitted
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-point reassociation is not permitted
func.func @stepped_keep(%A: memref<6x4xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<6x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 6 step 2 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 4 {
        affine.for %l = 0 to 4 {
          %va = affine.load %A[%i, %k] : memref<6x4xf32>
          %vb = affine.load %B[%l, %j] : memref<4x5xf32>
          %vc = affine.load %C[%i, %j] : memref<6x5xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %vc, %prod : f32
          affine.store %sum, %C[%i, %j] : memref<6x5xf32>
        }
      }
    }
  }
  return
}
// -----
// NOT pre-reduced -- arguments without llvm.noalias: A and B may overlap C (refused, as without fp-distribute)
// REMARK-NOT:  pre-reduced
// REMARK:      remark: affine GEMM not lowered to systolic.matmul_tile: A may overlap C; declare the arguments llvm.noalias (C restrict) or pass assume-no-alias [UNKNOWN]
// CHECK-LABEL: func.func @may_alias(
// CHECK-NOT:   memref.alloc
// CHECK-NOT:   systolic.matmul_tile
// CHECK:       arith.addf %{{.*}}, %{{.*}} : f32
// CHECK:       return
// OFF:         remark: affine GEMM not lowered to systolic.matmul_tile: A may overlap C
// DIST-NOT:    pre-reduced
// DIST:        remark: affine GEMM not lowered to systolic.matmul_tile: A may overlap C
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: A may overlap C
func.func @may_alias(%A: memref<3x4xf32>, %B: memref<4x5xf32>, %C: memref<3x5xf32>) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 4 {
        affine.for %l = 0 to 4 {
          %va = affine.load %A[%i, %k] : memref<3x4xf32>
          %vb = affine.load %B[%l, %j] : memref<4x5xf32>
          %vc = affine.load %C[%i, %j] : memref<3x5xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %vc, %prod : f32
          affine.store %sum, %C[%i, %j] : memref<3x5xf32>
        }
      }
    }
  }
  return
}
// -----
// NOT pre-reduced -- an assignment, the twin of T5 (C[i][j] = A[i][k] * B[l][j]): with allow-fp-reassociation, last write wins (k, l cut to their last iteration) and the K = 1 assignment into a zeroed C, never a sum of products
// REMARK-NOT:  pre-reduced
// REMARK:      remark: affine assignment nest C = A * B: last write wins (docs/AFFINE_GEMM_MAPPING.md section 20.5): loop(s) 2+3 of the nest
// REMARK:      remark: affine GEMM 3x5x1 lowered as 1 exact region(s) (an assignment C = A B: C zeroed before the tiles)
// CHECK-LABEL: func.func @assign_t5(
// CHECK-NOT:   memref.alloc
// CHECK:       memref.subview %arg0[0, 3] [3, 1] [1, 1]
// CHECK:       memref.subview %arg1[3, 0] [1, 5] [1, 1]
// CHECK:       systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1 :
// CHECK-NOT:   memref.dealloc
// CHECK:       return
// OFF:         remark: affine assignment nest C = A * B: last write wins
// OFF:         remark: affine GEMM 3x5x1 lowered as 1 exact region(s) (an assignment C = A B: C zeroed before the tiles)
// DIST-NOT:    pre-reduced
// Without allow-fp-reassociation (DIST, STRICT; the product is not marked
// reassoc) nothing is cut: the nest as written is refused at recognition.
// DIST-NOT:    last write wins
// STRICT-NOT:  last write wins
func.func @assign_t5(%A: memref<3x4xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 4 {
        affine.for %l = 0 to 4 {
          %va = affine.load %A[%i, %k] : memref<3x4xf32>
          %vb = affine.load %B[%l, %j] : memref<4x5xf32>
          %prod = arith.mulf %va, %vb : f32
          affine.store %prod, %C[%i, %j] : memref<3x5xf32>
        }
      }
    }
  }
  return
}
// -----
// NOT pre-reduced -- a domain that is not a box (l from k): no GEMM recognized, the nest stays as written
// REMARK-NOT:  pre-reduced
// CHECK-LABEL: func.func @non_box(
// CHECK-NOT:   memref.alloc
// CHECK-NOT:   systolic.matmul_tile
// CHECK:       affine.for %{{.*}} = #map(%{{.*}}) to 4 {
// CHECK:       arith.addf %{{.*}}, %{{.*}} : f32
// CHECK:       return
// DIST-NOT:    pre-reduced
#lb = affine_map<(d0) -> (d0)>
func.func @non_box(%A: memref<3x4xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 4 {
        affine.for %l = #lb(%k) to 4 {
          %va = affine.load %A[%i, %k] : memref<3x4xf32>
          %vb = affine.load %B[%l, %j] : memref<4x5xf32>
          %vc = affine.load %C[%i, %j] : memref<3x5xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %vc, %prod : f32
          affine.store %sum, %C[%i, %j] : memref<3x5xf32>
        }
      }
    }
  }
  return
}
