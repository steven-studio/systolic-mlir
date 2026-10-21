// Rank-1 affine accesses and a C that is not injective on the (I, J) box
// (S4, docs/AFFINE_GEMM_MAPPING.md section 25 as the recognizer's comments
// name it). An ACCUMULATION whose C view writes one element from two outputs
// of the (I, J) box (G2 fails: C[10i + j] with j < 11, C[i + j], C[4i + j]
// with j < 5) is not refused when one output role -- I first, else J -- can
// become batch loops that, with the other batch loops, form an OUTERMOST
// prefix of the nest's loops of two or more iterations, with constant trip
// counts, on a box domain whose counters are the ones as written. The batch
// loop then runs around the tile in the program's order, and each batch reads
// C, adds its tile and writes C back (to_tensor of C, matmul_tile,
// materialize_in_destination inside the batch loop) before the next batch
// reads it: every element of C receives the contributions of different
// batches in the order written; only the order inside one tile (over K)
// changes, under the permission to reassociate. Every tile is checked
// injective (G2) again and the bijection exact.
// RE-DECIDED 2026-10-21 (S4): c10_j11, cipj, c4_j5, t4_j11 and j_outer_j11
// were refused at recognition (G2, class VALID_BUT_LOWERING_UNSUPPORTED, "a
// decomposition with the J (or I) loops as batch loops would be valid, and is
// not constructed"); they are now that decomposition. Numerical evidence:
// test/SystolicCC/affine_rank1.c compares each bitwise with the loops as
// written (nonzero initial C), synchronous and asynchronous.
// What stays refused, with the reason and class of before: an assignment
// whose C collides (only the last product survives); an accumulation whose
// output role loops are not an outermost prefix (k outermost: the batches
// would not run in the order written); a sheared domain (its counters are
// rewritten by the domain normalization, U != I); a non-box (triangular)
// domain. The rank-1 factor accesses (A[5i + k][5i + k], A[i][2k][k], the
// Hankel A[3i + k] * B[4k + j], the merged dense K, T4 with an injective C)
// are lowered exactly as before S4 (regression).
//
// RUN: systolic-opt --test-affine-matmul-discovery -split-input-file %s \
// RUN:   -o /dev/null 2>&1 | FileCheck %s --check-prefix=DISC
// RUN: systolic-opt "--affine-matmul-to-systolic=report=true allow-fp-reassociation=true classify=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=REMARK
// RUN: systolic-opt "--affine-matmul-to-systolic=allow-fp-reassociation=true" \
// RUN:   -split-input-file %s | FileCheck %s
// RUN: systolic-opt "--affine-matmul-to-systolic=report=true classify=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=STRICT
// STRICT-NOT: lowered as

// (1) C[10i + j], j < 10: C is injective (a flat C with row stride 10): the plain 2 x 10 x 4 GEMM, no demotion
// DISC:        recognition result: success
// DISC:        layout C: general view: dimension 0 = 0 + 10*I + 1*J
// DISC-NOT:    partition:
// DISC:        dense logical GEMM: yes, I/J/K on loops 0/1/2{{$}}
// DISC-NEXT:   dense extents: M 2, N 10, K 4
// DISC-NEXT:   logical bijection: verified
// REMARK:      remark: affine GEMM 2x10x4 lowered as 1 exact region(s){{$}}
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-po{{.*}}[PROVEN_VALID]
// CHECK-LABEL: func.func @c10_j10(
// CHECK-NOT:   affine.for
// CHECK:       memref.reinterpret_cast %arg2 to offset: [0], sizes: [2, 10], strides: [10, 1] : memref<20xf32>
// CHECK:       bufferization.to_tensor {{.*}} restrict writable
// CHECK-NEXT:  systolic.matmul_tile {{.*}} m = 2 n = 10 k = 4
// CHECK-NEXT:  bufferization.materialize_in_destination {{.*}} in restrict writable
// CHECK-NEXT:  return
func.func @c10_j10(%A: memref<2x4xf32> {llvm.noalias}, %B: memref<4x10xf32> {llvm.noalias}, %C: memref<20xf32> {llvm.noalias}) {
  affine.for %i = 0 to 2 {
    affine.for %j = 0 to 10 {
      affine.for %k = 0 to 4 {
        %va = affine.load %A[%i, %k] : memref<2x4xf32>
        %vb = affine.load %B[%k, %j] : memref<4x10xf32>
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
// (2) C[10i + j], j < 11: (0, 10) and (1, 0) write C[10] -- a collision across rows.
// RE-DECIDED 2026-10-21 (S4, was G2 refused): i (loop 0, outermost) demoted, two
// 1 x 11 x 4 batches in order; batch 1 reads C[10] as batch 0 wrote it
// DISC:        recognition result: success
// DISC:        batch terms C: dimension 0 += 10*x0
// DISC:        statement: C = C + A * B (accumulation)
// DISC-NEXT:   partition: loop(s) 0 demoted to batch loops
// DISC:        dense logical GEMM: yes, I/J/K on loops /1/2, batched over loop(s) 0 (2 batch iteration(s))
// DISC-NEXT:   dense extents: M 1, N 11, K 4
// DISC-NEXT:   logical bijection: verified
// REMARK:      remark: affine GEMM 1x11x4 lowered as 1 exact region(s), partitioned on 1 demoted loop(s), batched: 2 batch iteration(s) of 1 batch loop(s); the enclosing loops run it 2 time(s)
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-po{{.*}}[PROVEN_VALID]
// CHECK-LABEL: func.func @c10_j11(
// CHECK:       affine.for %{{.*}} = 0 to 2 {
// CHECK:       bufferization.to_tensor {{.*}} restrict writable
// CHECK-NEXT:  systolic.matmul_tile {{.*}} m = 1 n = 11 k = 4
// CHECK-NEXT:  bufferization.materialize_in_destination {{.*}} in restrict writable
// CHECK-NEXT:  }
// CHECK-NEXT:  return
func.func @c10_j11(%A: memref<2x4xf32> {llvm.noalias}, %B: memref<4x11xf32> {llvm.noalias}, %C: memref<21xf32> {llvm.noalias}) {
  affine.for %i = 0 to 2 {
    affine.for %j = 0 to 11 {
      affine.for %k = 0 to 4 {
        %va = affine.load %A[%i, %k] : memref<2x4xf32>
        %vb = affine.load %B[%k, %j] : memref<4x11xf32>
        %vc = affine.load %C[10 * %i + %j] : memref<21xf32>
        %prod = arith.mulf %va, %vb : f32
        %sum = arith.addf %vc, %prod : f32
        affine.store %sum, %C[10 * %i + %j] : memref<21xf32>
      }
    }
  }
  return
}
// -----
// (3) C[i + j]: overlapping outputs (every anti-diagonal).
// RE-DECIDED 2026-10-21 (S4, was G2 refused): i demoted, four 1 x 5 x 3 batches
// DISC:        recognition result: success
// DISC:        batch terms C: dimension 0 += 1*x0
// DISC:        statement: C = C + A * B (accumulation)
// DISC-NEXT:   partition: loop(s) 0 demoted to batch loops
// DISC:        dense logical GEMM: yes, I/J/K on loops /1/2, batched over loop(s) 0 (4 batch iteration(s))
// DISC-NEXT:   dense extents: M 1, N 5, K 3
// DISC-NEXT:   logical bijection: verified
// REMARK:      remark: affine GEMM 1x5x3 lowered as 1 exact region(s), partitioned on 1 demoted loop(s), batched: 4 batch iteration(s) of 1 batch loop(s); the enclosing loops run it 4 time(s)
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-po{{.*}}[PROVEN_VALID]
// CHECK-LABEL: func.func @cipj(
// CHECK:       affine.for %{{.*}} = 0 to 4 {
// CHECK:       bufferization.to_tensor {{.*}} restrict writable
// CHECK-NEXT:  systolic.matmul_tile {{.*}} m = 1 n = 5 k = 3
// CHECK-NEXT:  bufferization.materialize_in_destination {{.*}} in restrict writable
// CHECK-NEXT:  }
// CHECK-NEXT:  return
func.func @cipj(%A: memref<4x3xf32> {llvm.noalias}, %B: memref<3x5xf32> {llvm.noalias}, %C: memref<8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 3 {
        %va = affine.load %A[%i, %k] : memref<4x3xf32>
        %vb = affine.load %B[%k, %j] : memref<3x5xf32>
        %vc = affine.load %C[%i + %j] : memref<8xf32>
        %prod = arith.mulf %va, %vb : f32
        %sum = arith.addf %vc, %prod : f32
        affine.store %sum, %C[%i + %j] : memref<8xf32>
      }
    }
  }
  return
}
// -----
// (4) C[4i + j], j < 4: injective, the plain 3 x 4 x 3 GEMM on a flat C
// DISC:        recognition result: success
// DISC:        layout C: general view: dimension 0 = 0 + 4*I + 1*J
// DISC-NOT:    partition:
// DISC:        dense logical GEMM: yes, I/J/K on loops 0/1/2{{$}}
// DISC-NEXT:   dense extents: M 3, N 4, K 3
// DISC-NEXT:   logical bijection: verified
// REMARK:      remark: affine GEMM 3x4x3 lowered as 1 exact region(s){{$}}
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-po{{.*}}[PROVEN_VALID]
// CHECK-LABEL: func.func @c4_j4(
// CHECK-NOT:   affine.for
// CHECK:       bufferization.to_tensor {{.*}} restrict writable
// CHECK-NEXT:  systolic.matmul_tile {{.*}} m = 3 n = 4 k = 3
// CHECK-NEXT:  bufferization.materialize_in_destination {{.*}} in restrict writable
// CHECK-NEXT:  return
func.func @c4_j4(%A: memref<3x3xf32> {llvm.noalias}, %B: memref<3x4xf32> {llvm.noalias}, %C: memref<12xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 3 {
        %va = affine.load %A[%i, %k] : memref<3x3xf32>
        %vb = affine.load %B[%k, %j] : memref<3x4xf32>
        %vc = affine.load %C[4 * %i + %j] : memref<12xf32>
        %prod = arith.mulf %va, %vb : f32
        %sum = arith.addf %vc, %prod : f32
        affine.store %sum, %C[4 * %i + %j] : memref<12xf32>
      }
    }
  }
  return
}
// -----
// (5) C[4i + j], j < 5: (i, 4) and (i + 1, 0) collide.
// RE-DECIDED 2026-10-21 (S4, was G2 refused): i demoted, three 1 x 5 x 3 batches
// DISC:        recognition result: success
// DISC:        batch terms C: dimension 0 += 4*x0
// DISC:        statement: C = C + A * B (accumulation)
// DISC-NEXT:   partition: loop(s) 0 demoted to batch loops
// DISC:        dense logical GEMM: yes, I/J/K on loops /1/2, batched over loop(s) 0 (3 batch iteration(s))
// DISC-NEXT:   dense extents: M 1, N 5, K 3
// DISC-NEXT:   logical bijection: verified
// REMARK:      remark: affine GEMM 1x5x3 lowered as 1 exact region(s), partitioned on 1 demoted loop(s), batched: 3 batch iteration(s) of 1 batch loop(s); the enclosing loops run it 3 time(s)
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-po{{.*}}[PROVEN_VALID]
// CHECK-LABEL: func.func @c4_j5(
// CHECK:       affine.for %{{.*}} = 0 to 3 {
// CHECK:       bufferization.to_tensor {{.*}} restrict writable
// CHECK-NEXT:  systolic.matmul_tile {{.*}} m = 1 n = 5 k = 3
// CHECK-NEXT:  bufferization.materialize_in_destination {{.*}} in restrict writable
// CHECK-NEXT:  }
// CHECK-NEXT:  return
func.func @c4_j5(%A: memref<3x3xf32> {llvm.noalias}, %B: memref<3x5xf32> {llvm.noalias}, %C: memref<13xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 3 {
        %va = affine.load %A[%i, %k] : memref<3x3xf32>
        %vb = affine.load %B[%k, %j] : memref<3x5xf32>
        %vc = affine.load %C[4 * %i + %j] : memref<13xf32>
        %prod = arith.mulf %va, %vb : f32
        %sum = arith.addf %vc, %prod : f32
        affine.store %sum, %C[4 * %i + %j] : memref<13xf32>
      }
    }
  }
  return
}
// -----
// (6) rank-1 regression: A[5i + k][5i + k] (a 2-D memref read along its diagonal), k < 5: unchanged by S4
// DISC:        recognition result: success
// DISC:        layout A: general view: dimension 0 = 0 + 5*I + 1*K; dimension 1 = 0 + 5*I + 1*K
// DISC-NOT:    partition:
// DISC:        dense logical GEMM: yes, I/J/K on loops 0/1/2{{$}}
// DISC-NEXT:   dense extents: M 4, N 3, K 5
// DISC-NEXT:   logical bijection: verified
// REMARK:      remark: affine GEMM 4x3x5 lowered as 1 exact region(s){{$}}
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-po{{.*}}[PROVEN_VALID]
// CHECK-LABEL: func.func @a5ik_k5(
// CHECK-NOT:   affine.for
// CHECK:       memref.reinterpret_cast %arg0 to offset: [0], sizes: [4, 5], strides: [105, 21] : memref<20x20xf32>
// CHECK:       bufferization.to_tensor {{.*}} restrict writable
// CHECK-NEXT:  systolic.matmul_tile {{.*}} m = 4 n = 3 k = 5
// CHECK-NEXT:  bufferization.materialize_in_destination {{.*}} in restrict writable
// CHECK-NEXT:  return
func.func @a5ik_k5(%A: memref<20x20xf32> {llvm.noalias}, %B: memref<5x3xf32> {llvm.noalias}, %C: memref<4x3xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 3 {
      affine.for %k = 0 to 5 {
        %va = affine.load %A[5 * %i + %k, 5 * %i + %k] : memref<20x20xf32>
        %vb = affine.load %B[%k, %j] : memref<5x3xf32>
        %vc = affine.load %C[%i, %j] : memref<4x3xf32>
        %prod = arith.mulf %va, %vb : f32
        %sum = arith.addf %vc, %prod : f32
        affine.store %sum, %C[%i, %j] : memref<4x3xf32>
      }
    }
  }
  return
}
// -----
// (6) rank-1 regression: A[5i + k][5i + k], k < 7: the read view overlaps itself (allowed for a factor): unchanged by S4
// DISC:        recognition result: success
// DISC:        layout A: general view: dimension 0 = 0 + 5*I + 1*K; dimension 1 = 0 + 5*I + 1*K
// DISC-NOT:    partition:
// DISC:        dense logical GEMM: yes, I/J/K on loops 0/1/2{{$}}
// DISC-NEXT:   dense extents: M 4, N 3, K 7
// DISC-NEXT:   logical bijection: verified
// REMARK:      remark: affine GEMM 4x3x7 lowered as 1 exact region(s){{$}}
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-po{{.*}}[PROVEN_VALID]
// CHECK-LABEL: func.func @a5ik_k7(
// CHECK-NOT:   affine.for
// CHECK:       memref.reinterpret_cast %arg0 to offset: [0], sizes: [4, 7], strides: [115, 23] : memref<22x22xf32>
// CHECK:       bufferization.to_tensor {{.*}} restrict writable
// CHECK-NEXT:  systolic.matmul_tile {{.*}} m = 4 n = 3 k = 7
// CHECK-NEXT:  bufferization.materialize_in_destination {{.*}} in restrict writable
// CHECK-NEXT:  return
func.func @a5ik_k7(%A: memref<22x22xf32> {llvm.noalias}, %B: memref<7x3xf32> {llvm.noalias}, %C: memref<4x3xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 3 {
      affine.for %k = 0 to 7 {
        %va = affine.load %A[5 * %i + %k, 5 * %i + %k] : memref<22x22xf32>
        %vb = affine.load %B[%k, %j] : memref<7x3xf32>
        %vc = affine.load %C[%i, %j] : memref<4x3xf32>
        %prod = arith.mulf %va, %vb : f32
        %sum = arith.addf %vc, %prod : f32
        affine.store %sum, %C[%i, %j] : memref<4x3xf32>
      }
    }
  }
  return
}
// -----
// (6) rank-1 regression: A[i][2k][k] (k in two dimensions): unchanged by S4
// DISC:        recognition result: success
// DISC:        layout A: general view: dimension 0 = 0 + 1*I; dimension 1 = 0 + 2*K; dimension 2 = 0 + 1*K
// DISC-NOT:    partition:
// DISC:        dense logical GEMM: yes, I/J/K on loops 0/1/2{{$}}
// DISC-NEXT:   dense extents: M 3, N 5, K 4
// DISC-NEXT:   logical bijection: verified
// REMARK:      remark: affine GEMM 3x5x4 lowered as 1 exact region(s){{$}}
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-po{{.*}}[PROVEN_VALID]
// CHECK-LABEL: func.func @a2kk(
// CHECK-NOT:   affine.for
// CHECK:       memref.reinterpret_cast %arg0 to offset: [0], sizes: [3, 4], strides: [32, 9] : memref<3x8x4xf32>
// CHECK:       bufferization.to_tensor {{.*}} restrict writable
// CHECK-NEXT:  systolic.matmul_tile {{.*}} m = 3 n = 5 k = 4
// CHECK-NEXT:  bufferization.materialize_in_destination {{.*}} in restrict writable
// CHECK-NEXT:  return
func.func @a2kk(%A: memref<3x8x4xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 4 {
        %va = affine.load %A[%i, 2 * %k, %k] : memref<3x8x4xf32>
        %vb = affine.load %B[%k, %j] : memref<4x5xf32>
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
// (6) rank-1 regression: Hankel factors A[3i + k] * B[4k + j] (1-D, overlapping read views): unchanged by S4
// DISC:        recognition result: success
// DISC:        layout A: general view: dimension 0 = 0 + 3*I + 1*K
// DISC-NEXT:   layout B: general view: dimension 0 = 0 + 4*K + 1*J
// DISC-NOT:    partition:
// DISC:        dense logical GEMM: yes, I/J/K on loops 0/1/2{{$}}
// DISC-NEXT:   dense extents: M 3, N 5, K 4
// DISC-NEXT:   logical bijection: verified
// REMARK:      remark: affine GEMM 3x5x4 lowered as 1 exact region(s){{$}}
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-po{{.*}}[PROVEN_VALID]
// CHECK-LABEL: func.func @hankel_34(
// CHECK-NOT:   affine.for
// CHECK:       memref.reinterpret_cast %arg0 to offset: [0], sizes: [3, 4], strides: [3, 1] : memref<10xf32>
// CHECK:       memref.reinterpret_cast %arg1 to offset: [0], sizes: [4, 5], strides: [4, 1] : memref<17xf32>
// CHECK:       bufferization.to_tensor {{.*}} restrict writable
// CHECK-NEXT:  systolic.matmul_tile {{.*}} m = 3 n = 5 k = 4
// CHECK-NEXT:  bufferization.materialize_in_destination {{.*}} in restrict writable
// CHECK-NEXT:  return
func.func @hankel_34(%A: memref<10xf32> {llvm.noalias}, %B: memref<17xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 4 {
        %va = affine.load %A[3 * %i + %k] : memref<10xf32>
        %vb = affine.load %B[4 * %k + %j] : memref<17xf32>
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
// (6) rank-1 regression: one dense K on two loops, A[i][4k0 + k1] * B[4k0 + k1][j]: unchanged by S4
// DISC:        recognition result: success
// DISC-NOT:    partition:
// DISC:        dense logical GEMM: yes, I/J/K on loops 0/1/2+3{{$}}
// DISC-NEXT:   dense extents: M 4, N 5, K 12
// DISC-NEXT:   logical bijection: verified
// REMARK:      remark: affine GEMM 4x5x12 lowered as 1 exact region(s){{$}}
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-po{{.*}}[PROVEN_VALID]
// CHECK-LABEL: func.func @merged_k(
// CHECK-NOT:   affine.for
// CHECK:       bufferization.to_tensor {{.*}} restrict writable
// CHECK-NEXT:  systolic.matmul_tile {{.*}} m = 4 n = 5 k = 12
// CHECK-NEXT:  bufferization.materialize_in_destination {{.*}} in restrict writable
// CHECK-NEXT:  return
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
// (6) T4, j < 10: C[10i + j] += A[20i + k][20i + k] * B[50k + j][j] -- C injective: the plain 2 x 10 x 4 GEMM, unchanged by S4
// DISC:        recognition result: success
// DISC:        layout C: general view: dimension 0 = 0 + 10*I + 1*J
// DISC-NOT:    partition:
// DISC:        dense logical GEMM: yes, I/J/K on loops 0/1/2{{$}}
// DISC-NEXT:   dense extents: M 2, N 10, K 4
// DISC-NEXT:   logical bijection: verified
// REMARK:      remark: affine GEMM 2x10x4 lowered as 1 exact region(s){{$}}
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-po{{.*}}[PROVEN_VALID]
// CHECK-LABEL: func.func @t4_j10(
// CHECK-NOT:   affine.for
// CHECK:       memref.reinterpret_cast %arg0 to offset: [0], sizes: [2, 4], strides: [500, 25] : memref<24x24xf32>
// CHECK:       memref.reinterpret_cast %arg1 to offset: [0], sizes: [4, 10], strides: [500, 11] : memref<160x10xf32>
// CHECK:       bufferization.to_tensor {{.*}} restrict writable
// CHECK-NEXT:  systolic.matmul_tile {{.*}} m = 2 n = 10 k = 4
// CHECK-NEXT:  bufferization.materialize_in_destination {{.*}} in restrict writable
// CHECK-NEXT:  return
func.func @t4_j10(%A: memref<24x24xf32> {llvm.noalias}, %B: memref<160x10xf32> {llvm.noalias}, %C: memref<20xf32> {llvm.noalias}) {
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
// (6) T4, j < 11 (B has 50*3 + 10 + 1 = 161 rows): C collides at C[10].
// RE-DECIDED 2026-10-21 (S4, was G2 refused): i demoted, two 1 x 11 x 4 batches;
// the A view of batch i starts at 500 i, the B view is shared
// DISC:        recognition result: success
// DISC:        batch terms A: dimension 0 = 0 + 20*x0, dimension 1 = 0 + 20*x0
// DISC:        batch terms C: dimension 0 += 10*x0
// DISC:        statement: C = C + A * B (accumulation)
// DISC-NEXT:   partition: loop(s) 0 demoted to batch loops
// DISC:        dense logical GEMM: yes, I/J/K on loops /1/2, batched over loop(s) 0 (2 batch iteration(s))
// DISC-NEXT:   dense extents: M 1, N 11, K 4
// DISC-NEXT:   logical bijection: verified
// REMARK:      remark: affine GEMM 1x11x4 lowered as 1 exact region(s), partitioned on 1 demoted loop(s), batched: 2 batch iteration(s) of 1 batch loop(s); the enclosing loops run it 2 time(s)
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-po{{.*}}[PROVEN_VALID]
// CHECK-LABEL: func.func @t4_j11(
// CHECK:       affine.for %{{.*}} = 0 to 2 {
// CHECK:       memref.reinterpret_cast %arg0 to offset: [%{{.*}}], sizes: [1, 4], strides: [1, 25] : memref<24x24xf32>
// CHECK:       memref.reinterpret_cast %arg1 to offset: [0], sizes: [4, 11], strides: [550, 12] : memref<161x11xf32>
// CHECK:       bufferization.to_tensor {{.*}} restrict writable
// CHECK-NEXT:  systolic.matmul_tile {{.*}} m = 1 n = 11 k = 4
// CHECK-NEXT:  bufferization.materialize_in_destination {{.*}} in restrict writable
// CHECK-NEXT:  }
// CHECK-NEXT:  return
func.func @t4_j11(%A: memref<24x24xf32> {llvm.noalias}, %B: memref<161x11xf32> {llvm.noalias}, %C: memref<21xf32> {llvm.noalias}) {
  affine.for %i = 0 to 2 {
    affine.for %j = 0 to 11 {
      affine.for %k = 0 to 4 {
        %va = affine.load %A[20 * %i + %k, 20 * %i + %k] : memref<24x24xf32>
        %vb = affine.load %B[50 * %k + %j, %j] : memref<161x11xf32>
        %vc = affine.load %C[10 * %i + %j] : memref<21xf32>
        %prod = arith.mulf %va, %vb : f32
        %sum = arith.addf %vc, %prod : f32
        affine.store %sum, %C[10 * %i + %j] : memref<21xf32>
      }
    }
  }
  return
}
// -----
// (7) assignment collision C[10i + j] = A[i] * B[j], j < 11: only the last product survives -- refused as before (no K)
// DISC:        recognition result: failure
// DISC-NEXT:   reason: C's view is not injective on the (I, J) box: the outputs with ordinals (I, J) and (I + 1, J + -10) write the same element of C: the assignment C = A * B overwrites it, only the last product survives -- not the GEMM of A and B (docs/AFFINE_GEMM_MAPPING.md section 20)
// DISC-NEXT:   class: PROVEN_INVALID
// CHECK-LABEL: func.func @assign_c10_j11(
// CHECK-NOT:   systolic.matmul_tile
func.func @assign_c10_j11(%A: memref<2xf32> {llvm.noalias}, %B: memref<11xf32> {llvm.noalias}, %C: memref<21xf32> {llvm.noalias}) {
  affine.for %i = 0 to 2 {
    affine.for %j = 0 to 11 {
      %va = affine.load %A[%i] : memref<2xf32>
      %vb = affine.load %B[%j] : memref<11xf32>
      %prod = arith.mulf %va, %vb : f32
      affine.store %prod, %C[10 * %i + %j] : memref<21xf32>
    }
  }
  return
}
// -----
// (7) assignment collision C[i + j] = A[i] * B[j]: refused as before
// DISC:        recognition result: failure
// DISC-NEXT:   reason: C's view is not injective on the (I, J) box: the outputs with ordinals (I, J) and (I + 1, J + -1) write the same element of C: the assignment C = A * B overwrites it, only the last product survives -- not the GEMM of A and B (docs/AFFINE_GEMM_MAPPING.md section 20)
// DISC-NEXT:   class: PROVEN_INVALID
// CHECK-LABEL: func.func @assign_cipj(
// CHECK-NOT:   systolic.matmul_tile
func.func @assign_cipj(%A: memref<4xf32> {llvm.noalias}, %B: memref<5xf32> {llvm.noalias}, %C: memref<8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 5 {
      %va = affine.load %A[%i] : memref<4xf32>
      %vb = affine.load %B[%j] : memref<5xf32>
      %prod = arith.mulf %va, %vb : f32
      affine.store %prod, %C[%i + %j] : memref<8xf32>
    }
  }
  return
}
// -----
// (8) the reduction loop outermost: for k { for i { for j { C[10i + j] += A[i][k] * B[k][j] }}}, j < 11:
// neither i nor j with the batch loops is an outermost prefix (k runs around them), so a batch
// would not see the contributions in the order written: refused as before
// DISC:        recognition result: failure
// DISC-NEXT:   reason: C's view is not injective on the (I, J) box: the outputs with ordinals (I, J) and (I + 1, J + -10) write the same element of C, so one tile would lose an update (G2, docs/AFFINE_GEMM_MAPPING.md section 19.3); a decomposition with the J (or I) loops as batch loops would be valid, and is not constructed
// DISC-NEXT:   class: VALID_BUT_LOWERING_UNSUPPORTED
// CHECK-LABEL: func.func @k_outer_j11(
// CHECK-NOT:   systolic.matmul_tile
func.func @k_outer_j11(%A: memref<2x3xf32> {llvm.noalias}, %B: memref<3x11xf32> {llvm.noalias}, %C: memref<21xf32> {llvm.noalias}) {
  affine.for %k = 0 to 3 {
    affine.for %i = 0 to 2 {
      affine.for %j = 0 to 11 {
        %va = affine.load %A[%i, %k] : memref<2x3xf32>
        %vb = affine.load %B[%k, %j] : memref<3x11xf32>
        %vc = affine.load %C[10 * %i + %j] : memref<21xf32>
        %prod = arith.mulf %va, %vb : f32
        %sum = arith.addf %vc, %prod : f32
        affine.store %sum, %C[10 * %i + %j] : memref<21xf32>
      }
    }
  }
  return
}
// -----
// (8) j outermost: for j { for i { for k { ... }}}, j < 11: i (loop 1) is no outermost prefix,
// j (loop 0) is: J demoted instead, eleven 2 x 1 x 3 batches (C column view with stride 10).
// RE-DECIDED 2026-10-21 (S4, was G2 refused)
// DISC:        recognition result: success
// DISC:        view C: [10*I + 0, 1*J + 0]
// DISC:        batch terms C: dimension 0 += 1*x0
// DISC:        statement: C = C + A * B (accumulation)
// DISC-NEXT:   partition: loop(s) 0 demoted to batch loops
// DISC:        dense logical GEMM: yes, I/J/K on loops 1//2, batched over loop(s) 0 (11 batch iteration(s))
// DISC-NEXT:   dense extents: M 2, N 1, K 3
// DISC-NEXT:   logical bijection: verified
// REMARK:      remark: affine GEMM 2x1x3 lowered as 1 exact region(s), partitioned on 1 demoted loop(s), batched: 11 batch iteration(s) of 1 batch loop(s); the enclosing loops run it 11 time(s)
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-po{{.*}}[PROVEN_VALID]
// CHECK-LABEL: func.func @j_outer_j11(
// CHECK:       affine.for %{{.*}} = 0 to 11 {
// CHECK:       memref.subview %arg2[%{{.*}}] [2] [10] : memref<21xf32>
// CHECK:       bufferization.to_tensor {{.*}} restrict writable
// CHECK-NEXT:  systolic.matmul_tile {{.*}} m = 2 n = 1 k = 3
// CHECK-NEXT:  bufferization.materialize_in_destination {{.*}} in restrict writable
// CHECK-NEXT:  }
// CHECK-NEXT:  return
func.func @j_outer_j11(%A: memref<2x3xf32> {llvm.noalias}, %B: memref<3x11xf32> {llvm.noalias}, %C: memref<21xf32> {llvm.noalias}) {
  affine.for %j = 0 to 11 {
    affine.for %i = 0 to 2 {
      affine.for %k = 0 to 3 {
        %va = affine.load %A[%i, %k] : memref<2x3xf32>
        %vb = affine.load %B[%k, %j] : memref<3x11xf32>
        %vc = affine.load %C[10 * %i + %j] : memref<21xf32>
        %prod = arith.mulf %va, %vb : f32
        %sum = arith.addf %vc, %prod : f32
        affine.store %sum, %C[10 * %i + %j] : memref<21xf32>
      }
    }
  }
  return
}
// -----
// (9) a sheared collision: t in [i, i + 11), C[10i + t - i]: the domain normalization rewrites
// the counters (U != I), so no ordered output batches are made: refused as before
// DISC:        domain normalization: 1 nest(s)
// DISC:        recognition result: failure
// DISC-NEXT:   reason: C's view is not injective on the (I, J) box: the outputs with ordinals (I, J) and (I + 1, J + -10) write the same element of C, so one tile would lose an update (G2, docs/AFFINE_GEMM_MAPPING.md section 19.3); a decomposition with the J (or I) loops as batch loops would be valid, and is not constructed
// DISC-NEXT:   class: VALID_BUT_LOWERING_UNSUPPORTED
// REMARK:      remark: affine multiply-accumulate nest: iteration domain normalized to a box (x = U x', docs/AFFINE_GEMM_MAPPING.md section 19.12): loop 1: x1 = x1' + 1*x0, box [0, 11) step 1
// REMARK-NOT:  remark: affine GEMM
// CHECK-LABEL: func.func @sheared_j11(
// CHECK-NOT:   systolic.matmul_tile
func.func @sheared_j11(%A: memref<2x4xf32> {llvm.noalias}, %B: memref<4x11xf32> {llvm.noalias}, %C: memref<21xf32> {llvm.noalias}) {
  affine.for %i = 0 to 2 {
    affine.for %t = affine_map<(d0) -> (d0)>(%i) to affine_map<(d0) -> (d0 + 11)>(%i) {
      affine.for %k = 0 to 4 {
        %va = affine.load %A[%i, %k] : memref<2x4xf32>
        %vb = affine.load %B[%k, %t - %i] : memref<4x11xf32>
        %vc = affine.load %C[10 * %i + %t - %i] : memref<21xf32>
        %prod = arith.mulf %va, %vb : f32
        %sum = arith.addf %vc, %prod : f32
        affine.store %sum, %C[10 * %i + %t - %i] : memref<21xf32>
      }
    }
  }
  return
}
// -----
// (9) a non-box collision: j < i + 10, C[10i + j] (C[20] from (1, 10) and (2, 0)): refused as before
// DISC:        recognition result: failure
// DISC-NEXT:   reason: logical coordinate J is driven by 1 loops that do not enumerate it densely
// DISC-NEXT:   class: UNKNOWN
// CHECK-LABEL: func.func @triangle_j(
// CHECK-NOT:   systolic.matmul_tile
func.func @triangle_j(%A: memref<3x4xf32> {llvm.noalias}, %B: memref<4x12xf32> {llvm.noalias}, %C: memref<32xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to affine_map<(d0) -> (d0 + 10)>(%i) {
      affine.for %k = 0 to 4 {
        %va = affine.load %A[%i, %k] : memref<3x4xf32>
        %vb = affine.load %B[%k, %j] : memref<4x12xf32>
        %vc = affine.load %C[10 * %i + %j] : memref<32xf32>
        %prod = arith.mulf %va, %vb : f32
        %sum = arith.addf %vc, %prod : f32
        affine.store %sum, %C[10 * %i + %j] : memref<32xf32>
      }
    }
  }
  return
}
