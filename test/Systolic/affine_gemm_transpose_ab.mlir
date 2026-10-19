// A and B transposed, alone or together, through the whole lowering
// (docs/AFFINE_GEMM_MAPPING.md section 5, Phase 2 of 2026-10-16). A transposed
// operand stores its two roles in the other order: A[k][i] holds A'(I, K) in
// dimensions (1, 0), B[j][k] holds B'(K, J) in dimensions (1, 0). The view is
// the rank-reducing subview of the memref (signed strides) followed by
// memref.transpose, so that the tile reads the logical matrix; each case below
// checks which operand gets the transpose, the subview's offsets and strides,
// and the tile's extents. Addresses are checked element by element by the
// oracle (affine_gemm_mapping_oracle.mlir) and by execution
// (test/SystolicCC/affine_transpose_ab.c).
//
// RUN: systolic-opt "--affine-matmul-to-systolic=allow-fp-reassociation=true" \
// RUN:   -split-input-file %s | FileCheck %s
// RUN: systolic-opt "--affine-matmul-to-systolic=report=true allow-fp-reassociation=true classify=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=REMARK
// RUN: systolic-opt --test-affine-matmul-discovery -split-input-file %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s --check-prefix=DISC
//
// Through the fleet down to the LLVM dialect (staging copies of transposed
// and strided slices into contiguous scratch tiles):
// RUN: systolic-opt --systolic-declare-fpga-fleet "--affine-matmul-to-systolic=allow-fp-reassociation=true" \
// RUN:   --lower-systolic-matmul-tile-to-fpga --systolic-runtime-to-llvm -split-input-file %s \
// RUN:   | FileCheck %s --check-prefix=LLVM
// LLVM-LABEL:    llvm.func @tt_tiles(
// LLVM-COUNT-10: llvm.call @fpga_matmul_tiled_auto_scheduled
// LLVM-LABEL:    llvm.func @tt_batch_views(
// LLVM:          llvm.call @fpga_matmul_tiled_auto_scheduled
// LLVM-NOT:      affine.

// 1. Neither transposed: no memref.transpose (the whole memrefs are the views).
// CHECK-LABEL: func.func @nn
// CHECK-NOT:   memref.transpose
// CHECK:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 8
// REMARK:      remark: affine GEMM 8x8x8 lowered as 1 exact region(s)
// DISC:        dense logical GEMM: yes, I/J/K on loops 0/1/2
func.func @nn(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i, %k] : memref<8x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x8xf32>
        %c = affine.load %C[%i, %j] : memref<8x8xf32>
        %m = arith.mulf %a, %b : f32
        %s = arith.addf %c, %m : f32
        affine.store %s, %C[%i, %j] : memref<8x8xf32>
      }
    }
  }
  return
}

// -----
// 2. A transposed (A[k][i]): A's subview, then the transpose; B untouched.
// CHECK-LABEL: func.func @tn
// CHECK:       %[[SA:.*]] = memref.subview %arg0[0, 0] [8, 8] [1, 1]
// CHECK:       memref.transpose %[[SA]] (d0, d1) -> (d1, d0)
// CHECK-NOT:   memref.transpose
// CHECK:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 8
// DISC:        layout A: roles in dimensions (1, 0) transposed
// DISC-NOT:    layout B:
// DISC:        dense logical GEMM: yes, I/J/K on loops 0/1/2
func.func @tn(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%k, %i] : memref<8x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x8xf32>
        %c = affine.load %C[%i, %j] : memref<8x8xf32>
        %m = arith.mulf %a, %b : f32
        %s = arith.addf %c, %m : f32
        affine.store %s, %C[%i, %j] : memref<8x8xf32>
      }
    }
  }
  return
}

// -----
// 3. B transposed (B[j][k]): only B's view is transposed.
// CHECK-LABEL: func.func @nt
// CHECK:       %[[SB:.*]] = memref.subview %arg1[0, 0] [8, 8] [1, 1]
// CHECK:       memref.transpose %[[SB]] (d0, d1) -> (d1, d0)
// CHECK-NOT:   memref.transpose
// CHECK:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 8
// DISC:        layout B: roles in dimensions (1, 0) transposed
// DISC:        dense logical GEMM: yes, I/J/K on loops 0/1/2
func.func @nt(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i, %k] : memref<8x8xf32>
        %b = affine.load %B[%j, %k] : memref<8x8xf32>
        %c = affine.load %C[%i, %j] : memref<8x8xf32>
        %m = arith.mulf %a, %b : f32
        %s = arith.addf %c, %m : f32
        affine.store %s, %C[%i, %j] : memref<8x8xf32>
      }
    }
  }
  return
}

// -----
// 4. A and B transposed (A[k][i], B[j][k]): both views transposed; the shared
// reduction coordinate K is dimension 0 of A and dimension 1 of B. With a
// rectangular shape (M = 4, N = 6, K = 8) a swapped role would not type-check.
// CHECK-LABEL: func.func @tt
// CHECK:       %[[SA:.*]] = memref.subview %arg0[0, 0] [8, 4] [1, 1]
// CHECK:       %[[TA:.*]] = memref.transpose %[[SA]] (d0, d1) -> (d1, d0) : memref<8x4xf32{{.*}}> to memref<4x8xf32
// CHECK:       %[[SB:.*]] = memref.subview %arg1[0, 0] [6, 8] [1, 1]
// CHECK:       %[[TB:.*]] = memref.transpose %[[SB]] (d0, d1) -> (d1, d0) : memref<6x8xf32{{.*}}> to memref<8x6xf32
// CHECK:       systolic.matmul_tile {{.*}} m = 4 n = 6 k = 8
// REMARK:      remark: affine GEMM 4x6x8 lowered as 1 exact region(s)
// DISC:        layout A: roles in dimensions (1, 0) transposed
// DISC-NEXT:   layout B: roles in dimensions (1, 0) transposed
// DISC:        dense logical GEMM: yes, I/J/K on loops 0/1/2
func.func @tt(%A: memref<8x4xf32> {llvm.noalias}, %B: memref<6x8xf32> {llvm.noalias}, %C: memref<4x6xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 6 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%k, %i] : memref<8x4xf32>
        %b = affine.load %B[%j, %k] : memref<6x8xf32>
        %c = affine.load %C[%i, %j] : memref<4x6xf32>
        %m = arith.mulf %a, %b : f32
        %s = arith.addf %c, %m : f32
        affine.store %s, %C[%i, %j] : memref<4x6xf32>
      }
    }
  }
  return
}

// -----
// 5. Both transposed with offsets: A[k + 2][i + 3], B[j + 1][k + 2]. Both
// spell the reduction coordinate k + 2, so K is that expression (views with
// offset 0 along K); the subviews start at (2, 3) and (1, 2), transposed.
// CHECK-LABEL: func.func @tt_offset
// CHECK:       %[[SA:.*]] = memref.subview %arg0[2, 3] [8, 4] [1, 1]
// CHECK:       memref.transpose %[[SA]] (d0, d1) -> (d1, d0)
// CHECK:       %[[SB:.*]] = memref.subview %arg1[1, 2] [6, 8] [1, 1]
// CHECK:       memref.transpose %[[SB]] (d0, d1) -> (d1, d0)
// CHECK:       systolic.matmul_tile {{.*}} m = 4 n = 6 k = 8
// DISC:        view A: [1*I + 3, 1*K + 0]
// DISC-NEXT:   view B: [1*K + 0, 1*J + 1]
func.func @tt_offset(%A: memref<10x7xf32> {llvm.noalias}, %B: memref<7x10xf32> {llvm.noalias}, %C: memref<4x6xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 6 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%k + 2, %i + 3] : memref<10x7xf32>
        %b = affine.load %B[%j + 1, %k + 2] : memref<7x10xf32>
        %c = affine.load %C[%i, %j] : memref<4x6xf32>
        %m = arith.mulf %a, %b : f32
        %s = arith.addf %c, %m : f32
        affine.store %s, %C[%i, %j] : memref<4x6xf32>
      }
    }
  }
  return
}

// -----
// 6. Both transposed with strides: A[2k][3i], B[2j][3k] -- strides 2 and 3 in
// the subviews, the transposes after them.
// CHECK-LABEL: func.func @tt_stride
// CHECK:       %[[SA:.*]] = memref.subview %arg0[0, 0] [8, 4] [2, 3]
// CHECK:       memref.transpose %[[SA]] (d0, d1) -> (d1, d0)
// CHECK:       %[[SB:.*]] = memref.subview %arg1[0, 0] [6, 8] [2, 3]
// CHECK:       memref.transpose %[[SB]] (d0, d1) -> (d1, d0)
// CHECK:       systolic.matmul_tile {{.*}} m = 4 n = 6 k = 8
// DISC:        view A: [3*I + 0, 2*K + 0]
// DISC-NEXT:   view B: [3*K + 0, 2*J + 0]
func.func @tt_stride(%A: memref<15x10xf32> {llvm.noalias}, %B: memref<11x22xf32> {llvm.noalias}, %C: memref<4x6xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 6 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%k * 2, %i * 3] : memref<15x10xf32>
        %b = affine.load %B[%j * 2, %k * 3] : memref<11x22xf32>
        %c = affine.load %C[%i, %j] : memref<4x6xf32>
        %m = arith.mulf %a, %b : f32
        %s = arith.addf %c, %m : f32
        affine.store %s, %C[%i, %j] : memref<4x6xf32>
      }
    }
  }
  return
}

// -----
// 7. Both transposed, A read backwards along both of its roles relative to B
// and C: A[7 - k][3 - i] with B[j][k] -- negative strides in A's subview
// (offsets: the largest indices), then the transposes. (Read backwards by both
// factors alike, A[7 - k] with B[..][7 - k], K is the expression 7 - k itself
// and no view runs backwards.)
// CHECK-LABEL: func.func @tt_negative
// CHECK:       %[[SA:.*]] = memref.subview %arg0[7, 3] [8, 4] [-1, -1]
// CHECK:       memref.transpose %[[SA]] (d0, d1) -> (d1, d0)
// CHECK:       %[[SB:.*]] = memref.subview %arg1[0, 0] [6, 8] [1, 1]
// CHECK:       memref.transpose %[[SB]] (d0, d1) -> (d1, d0)
// CHECK:       systolic.matmul_tile {{.*}} m = 4 n = 6 k = 8
func.func @tt_negative(%A: memref<8x4xf32> {llvm.noalias}, %B: memref<6x8xf32> {llvm.noalias}, %C: memref<4x6xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 6 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[-%k + 7, -%i + 3] : memref<8x4xf32>
        %b = affine.load %B[%j, %k] : memref<6x8xf32>
        %c = affine.load %C[%i, %j] : memref<4x6xf32>
        %m = arith.mulf %a, %b : f32
        %s = arith.addf %c, %m : f32
        affine.store %s, %C[%i, %j] : memref<4x6xf32>
      }
    }
  }
  return
}

// -----
// B transposed and read backwards along J (not covered by an executed test
// before Phase 2): B[7 - j][k].
// CHECK-LABEL: func.func @nt_negative
// CHECK:       %[[SB:.*]] = memref.subview %arg1[7, 0] [8, 8] [-1, 1]
// CHECK:       memref.transpose %[[SB]] (d0, d1) -> (d1, d0)
// CHECK:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 8
func.func @nt_negative(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i, %k] : memref<8x8xf32>
        %b = affine.load %B[-%j + 7, %k] : memref<8x8xf32>
        %c = affine.load %C[%i, %j] : memref<8x8xf32>
        %m = arith.mulf %a, %b : f32
        %s = arith.addf %c, %m : f32
        affine.store %s, %C[%i, %j] : memref<8x8xf32>
      }
    }
  }
  return
}

// -----
// 8. Both transposed, several tiles: 16 x 16 x 16 (checked down to the LLVM
// dialect -- ten tiles on the fleet, each a slice of the transposed views).
// CHECK-LABEL: func.func @tt_tiles
// CHECK:       memref.transpose
// CHECK:       memref.transpose
// CHECK:       systolic.matmul_tile {{.*}} m = 16 n = 16 k = 16
func.func @tt_tiles(%A: memref<16x16xf32> {llvm.noalias}, %B: memref<16x16xf32> {llvm.noalias}, %C: memref<16x16xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 16 {
      affine.for %k = 0 to 16 {
        %a = affine.load %A[%k, %i] : memref<16x16xf32>
        %b = affine.load %B[%j, %k] : memref<16x16xf32>
        %c = affine.load %C[%i, %j] : memref<16x16xf32>
        %m = arith.mulf %a, %b : f32
        %s = arith.addf %c, %m : f32
        affine.store %s, %C[%i, %j] : memref<16x16xf32>
      }
    }
  }
  return
}

// -----
// 9. Both transposed with batch-dependent views (ISSUE-007):
// C[b][i][j] += A[k][i + b] * B[j][k + b]. b is read by all three operands;
// per b, A'(I, K) = A(K, I + b) and B'(K, J) = B(J, K + b): transposed views
// whose offsets move with the batch.
// CHECK-LABEL: func.func @tt_batch_views
// CHECK:       affine.for %[[B:.*]] = 0 to 2 {
// CHECK:         %[[IDX:.*]] = affine.apply #{{[a-z0-9_]+}}(%[[B]])
// CHECK:         %[[SA:.*]] = memref.subview %arg0[0, %[[IDX]]] [4, 4] [1, 1]
// CHECK:         memref.transpose %[[SA]] (d0, d1) -> (d1, d0)
// CHECK:         %[[SB:.*]] = memref.subview %arg1[0, %[[IDX]]] [4, 4] [1, 1]
// CHECK:         memref.transpose %[[SB]] (d0, d1) -> (d1, d0)
// CHECK:         systolic.matmul_tile {{.*}} m = 4 n = 4 k = 4
// REMARK:      remark: affine GEMM 4x4x4 lowered as 1 exact region(s), batched: 2 batch iteration(s) of 1 batch loop(s)
// DISC:        layout A: roles in dimensions (1, 0) transposed
// DISC-NEXT:   layout B: roles in dimensions (1, 0) transposed
// DISC:        batch terms A: dimension 1 += 1*x0
// DISC-NEXT:   batch terms B: dimension 1 += 1*x0
func.func @tt_batch_views(%A: memref<4x5xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<2x4x4xf32> {llvm.noalias}) {
  affine.for %b = 0 to 2 {
    affine.for %i = 0 to 4 {
      affine.for %j = 0 to 4 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%k, %i + %b] : memref<4x5xf32>
          %bb = affine.load %B[%j, %k + %b] : memref<4x5xf32>
          %c = affine.load %C[%b, %i, %j] : memref<2x4x4xf32>
          %m = arith.mulf %a, %bb : f32
          %s = arith.addf %c, %m : f32
          affine.store %s, %C[%b, %i, %j] : memref<2x4x4xf32>
        }
      }
    }
  }
  return
}
