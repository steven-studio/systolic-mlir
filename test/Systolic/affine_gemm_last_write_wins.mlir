// Last write wins (docs/AFFINE_GEMM_MAPPING.md section 20.5, on by default;
// last-write-wins=false turns it off). In an assignment C = A * B on a box as
// written (constant bounds, a nonempty domain), whose body is exactly two
// loads, their product and its store, and whose factors provably do not
// overlap C, every loop of two or more iterations that C's store does not read
// is cut to its last iteration, [last, last + 1) with last = lower +
// (trips - 1) * step: for each element of C, the iterations that write it form
// a product with that loop's whole range, so the last of them (in the order
// written) has that loop at its last value. The nest keeps its order and its
// final values; it is not made an accumulation. Whether it is then lowered is
// decided by the assignment rules (section 20: written once, the K = 1 GEMM
// into a zeroed C). The cut is made only where the nest may be lowered: with
// the permission to reassociate (allow-fp-reassociation, or the product marked
// reassoc, case 16); under strict floating point the nest stays exactly as
// written.
//
// Prefix REMARK, the cut (naming the loops by depth) and the GEMM remark.
// RUN: systolic-opt "--affine-matmul-to-systolic=report=true allow-fp-reassociation=true classify=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=REMARK
// Prefix CHECK, the lowering (the subviews at the last iteration, the K = 1 tile).
// RUN: systolic-opt "--affine-matmul-to-systolic=allow-fp-reassociation=true" \
// RUN:   -split-input-file %s | FileCheck %s
// Prefix CUT, the cut's values, with the permission: on a nest cut and then
// lowered, the factors' subviews start at the cut loops' last values (their
// strided offsets are the linearized last element); on a nest cut but not
// lowered (case 9) and on those not cut (cases 10-12), the loop bounds.
// RUN: systolic-opt "--affine-matmul-to-systolic=allow-fp-reassociation=true" \
// RUN:   -split-input-file %s | FileCheck %s --check-prefix=CUT
// Prefix ASIS and STRICT, strict floating point (no allow-fp-reassociation, a
// product without reassoc): nothing is cut, every loop keeps its bounds as
// written and nothing is lowered -- except case 16, whose product is marked
// reassoc: it is cut and lowered without the option.
// RUN: systolic-opt "--affine-matmul-to-systolic=report=true classify=true" \
// RUN:   -split-input-file %s 2>/dev/null | FileCheck %s --check-prefix=ASIS
// RUN: systolic-opt "--affine-matmul-to-systolic=report=true classify=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=STRICT
// STRICT-NOT:      last write wins
// STRICT-NOT:      lowered as
// STRICT:          remark: affine assignment nest C = A * B: last write wins (docs/AFFINE_GEMM_MAPPING.md section 20.5): loop(s) 2 of the nest, which C does not read, run their last iteration only (the earlier ones write values overwritten later)
// STRICT:          remark: affine GEMM 3x5x1 lowered as 1 exact region(s) (an assignment C = A B: C zeroed before the tiles){{$}}
// STRICT-NOT:      last write wins
// STRICT-NOT:      lowered as
// Prefix WONCE, the recognizer on the rewritten nests (case 9 refused by the
// written-once rule, not by the loop C does not read).
// RUN: systolic-opt "--affine-matmul-to-systolic=allow-fp-reassociation=true" \
// RUN:   -split-input-file %s | systolic-opt --test-affine-matmul-discovery \
// RUN:   -split-input-file - -o /dev/null 2>&1 | FileCheck %s --check-prefix=WONCE
// Prefix OFF, last-write-wins=false, the recognition as before: every nest with a
// loop C does not read of two or more iterations stays as written, refused.
// RUN: systolic-opt "--affine-matmul-to-systolic=allow-fp-reassociation=true last-write-wins=false" \
// RUN:   -split-input-file %s | FileCheck %s --check-prefix=OFF
// RUN: systolic-opt "--affine-matmul-to-systolic=report=true allow-fp-reassociation=true classify=true last-write-wins=false" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=OFFREM
// OFFREM-NOT:  last write wins
// OFFREM:      remark: affine GEMM 3x5x1 lowered as 1 exact region(s) (an assignment C = A B: C zeroed before the tiles)
// OFFREM-NOT:  last write wins
// OFFREM:      remark: affine GEMM 3x5x4 lowered as 1 exact region(s){{$}}
// OFFREM-NOT:  lowered as

// 1. K outermost: for k, i, j: C[i][j] = A[i][k] * B[k][j]. Only k = 3 survives:
//    k is cut to [3, 4), the nest is then the 3 x 5 x 1 GEMM of column 3 of A
//    and row 3 of B. Without the cut (OFF) it is refused (loop 0 not a
//    subscript of C).
// REMARK:      remark: affine assignment nest C = A * B: last write wins (docs/AFFINE_GEMM_MAPPING.md section 20.5): loop(s) 0 of the nest, which C does not read, run their last iteration only (the earlier ones write values overwritten later)
// REMARK:      remark: affine GEMM 3x5x1 lowered as 1 exact region(s) (an assignment C = A B: C zeroed before the tiles)
// CHECK-LABEL: func.func @k_outer(
// CHECK:       memref.subview %arg0[0, 3]
// CHECK:       memref.subview %arg1[3, 0]
// CHECK-NOT:   affine.load
// CHECK:       systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1
// CHECK-NEXT:  bufferization.materialize_in_destination
// CUT-LABEL:   func.func @k_outer(
// CUT:         memref.subview %arg0[0, 3] [3, 1] [1, 1] : memref<3x4xf32> to memref<3x1xf32, strided<[4, 1], offset: 3>>
// CUT:         memref.subview %arg1[3, 0] [1, 5] [1, 1] : memref<4x5xf32> to memref<1x5xf32, strided<[5, 1], offset: 15>>
// CUT:         systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1
// ASIS-LABEL:  func.func @k_outer(
// ASIS:        affine.for %{{.*}} = 0 to 4 {
// ASIS-NEXT:   affine.for %{{.*}} = 0 to 3 {
// ASIS-NEXT:   affine.for %{{.*}} = 0 to 5 {
// ASIS:        affine.store
// ASIS-NOT:    systolic.matmul_tile
// OFF-LABEL:   func.func @k_outer(
// OFF-NOT:     systolic.matmul_tile
// OFF:         affine.store
func.func @k_outer(%A: memref<3x4xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %k = 0 to 4 {
    affine.for %i = 0 to 3 {
      affine.for %j = 0 to 5 {
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

// 2. K in the middle: for i, k, j.
// REMARK:      remark: affine assignment nest C = A * B: last write wins (docs/AFFINE_GEMM_MAPPING.md section 20.5): loop(s) 1 of the nest, which C does not read, run their last iteration only (the earlier ones write values overwritten later)
// REMARK:      remark: affine GEMM 3x5x1 lowered as 1 exact region(s) (an assignment C = A B: C zeroed before the tiles)
// CHECK-LABEL: func.func @k_middle(
// CHECK:       memref.subview %arg0[0, 3]
// CHECK:       memref.subview %arg1[3, 0]
// CHECK-NOT:   affine.load
// CHECK:       systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1
// CHECK-NEXT:  bufferization.materialize_in_destination
// CUT-LABEL:   func.func @k_middle(
// CUT:         memref.subview %arg0[0, 3] [3, 1] [1, 1] : memref<3x4xf32> to memref<3x1xf32, strided<[4, 1], offset: 3>>
// CUT:         memref.subview %arg1[3, 0] [1, 5] [1, 1] : memref<4x5xf32> to memref<1x5xf32, strided<[5, 1], offset: 15>>
// CUT:         systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1
// ASIS-LABEL:  func.func @k_middle(
// ASIS:        affine.for %{{.*}} = 0 to 3 {
// ASIS-NEXT:   affine.for %{{.*}} = 0 to 4 {
// ASIS-NEXT:   affine.for %{{.*}} = 0 to 5 {
// ASIS:        affine.store
// ASIS-NOT:    systolic.matmul_tile
// OFF-LABEL:   func.func @k_middle(
// OFF-NOT:     systolic.matmul_tile
// OFF:         affine.store
func.func @k_middle(%A: memref<3x4xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %k = 0 to 4 {
      affine.for %j = 0 to 5 {
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

// 3. K innermost: for i, j, k.
// REMARK:      remark: affine assignment nest C = A * B: last write wins (docs/AFFINE_GEMM_MAPPING.md section 20.5): loop(s) 2 of the nest, which C does not read, run their last iteration only (the earlier ones write values overwritten later)
// REMARK:      remark: affine GEMM 3x5x1 lowered as 1 exact region(s) (an assignment C = A B: C zeroed before the tiles)
// CHECK-LABEL: func.func @k_inner(
// CHECK:       memref.subview %arg0[0, 3]
// CHECK:       memref.subview %arg1[3, 0]
// CHECK-NOT:   affine.load
// CHECK:       systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1
// CHECK-NEXT:  bufferization.materialize_in_destination
// CUT-LABEL:   func.func @k_inner(
// CUT:         memref.subview %arg0[0, 3] [3, 1] [1, 1] : memref<3x4xf32> to memref<3x1xf32, strided<[4, 1], offset: 3>>
// CUT:         memref.subview %arg1[3, 0] [1, 5] [1, 1] : memref<4x5xf32> to memref<1x5xf32, strided<[5, 1], offset: 15>>
// CUT:         systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1
// ASIS-LABEL:  func.func @k_inner(
// ASIS:        affine.for %{{.*}} = 0 to 3 {
// ASIS-NEXT:   affine.for %{{.*}} = 0 to 5 {
// ASIS-NEXT:   affine.for %{{.*}} = 0 to 4 {
// ASIS:        affine.store
// ASIS-NOT:    systolic.matmul_tile
// OFF-LABEL:   func.func @k_inner(
// OFF-NOT:     systolic.matmul_tile
// OFF:         affine.store
func.func @k_inner(%A: memref<3x4xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
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

// 4. Two loops C does not read (k at depth 1, l innermost):
//    C[i][j] = A[i][k][l] * B[k][l][j]; both cut, to k = 3 and l = 1.
// REMARK:      remark: affine assignment nest C = A * B: last write wins (docs/AFFINE_GEMM_MAPPING.md section 20.5): loop(s) 1+3 of the nest, which C does not read, run their last iteration only (the earlier ones write values overwritten later)
// REMARK:      remark: affine GEMM 3x5x1 lowered as 1 exact region(s) (an assignment C = A B: C zeroed before the tiles)
// CHECK-LABEL: func.func @two_cut(
// CHECK:       memref.subview %arg0[0, 3, 1]
// CHECK:       memref.subview %arg1[3, 1, 0]
// CHECK-NOT:   affine.load
// CHECK:       systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1
// CHECK-NEXT:  bufferization.materialize_in_destination
// CUT-LABEL:   func.func @two_cut(
// CUT:         memref.subview %arg0[0, 3, 1] [3, 1, 1] [1, 1, 1] : memref<3x4x2xf32> to memref<3x1xf32, strided<[8, 2], offset: 7>>
// CUT:         memref.subview %arg1[3, 1, 0] [1, 1, 5] [1, 1, 1] : memref<4x2x5xf32> to memref<1x5xf32, strided<[10, 1], offset: 35>>
// CUT:         systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1
// ASIS-LABEL:  func.func @two_cut(
// ASIS:        affine.for %{{.*}} = 0 to 3 {
// ASIS-NEXT:   affine.for %{{.*}} = 0 to 4 {
// ASIS-NEXT:   affine.for %{{.*}} = 0 to 5 {
// ASIS-NEXT:   affine.for %{{.*}} = 0 to 2 {
// ASIS:        affine.store
// ASIS-NOT:    systolic.matmul_tile
// OFF-LABEL:   func.func @two_cut(
// OFF-NOT:     systolic.matmul_tile
// OFF:         affine.store
func.func @two_cut(%A: memref<3x4x2xf32> {llvm.noalias}, %B: memref<4x2x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %k = 0 to 4 {
      affine.for %j = 0 to 5 {
        affine.for %l = 0 to 2 {
          %a = affine.load %A[%i, %k, %l] : memref<3x4x2xf32>
          %b = affine.load %B[%k, %l, %j] : memref<4x2x5xf32>
          %p = arith.mulf %a, %b : f32
          affine.store %p, %C[%i, %j] : memref<3x5xf32>
        }
      }
    }
  }
  return
}

// -----

// 5. A stepped loop: k = 1, 3, 5, 7 (1 to 9 step 2); last = 1 + (4 - 1) * 2 = 7,
//    not the upper bound minus one (8): column 7 of A, row 7 of B (the
//    subviews' offset 7). The cut loop keeps its step (one iteration either
//    way).
// REMARK:      remark: affine assignment nest C = A * B: last write wins (docs/AFFINE_GEMM_MAPPING.md section 20.5): loop(s) 2 of the nest, which C does not read, run their last iteration only (the earlier ones write values overwritten later)
// REMARK:      remark: affine GEMM 3x5x1 lowered as 1 exact region(s) (an assignment C = A B: C zeroed before the tiles)
// CHECK-LABEL: func.func @stepped(
// CHECK:       memref.subview %arg0[0, 7]
// CHECK:       memref.subview %arg1[7, 0]
// CHECK-NOT:   affine.load
// CHECK:       systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1
// CHECK-NEXT:  bufferization.materialize_in_destination
// CUT-LABEL:   func.func @stepped(
// CUT:         memref.subview %arg0[0, 7] [3, 1] [1, 1] : memref<3x9xf32> to memref<3x1xf32, strided<[9, 1], offset: 7>>
// CUT:         memref.subview %arg1[7, 0] [1, 5] [1, 1] : memref<9x5xf32> to memref<1x5xf32, strided<[5, 1], offset: 35>>
// CUT:         systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1
// ASIS-LABEL:  func.func @stepped(
// ASIS:        affine.for %{{.*}} = 0 to 3 {
// ASIS-NEXT:   affine.for %{{.*}} = 0 to 5 {
// ASIS-NEXT:   affine.for %{{.*}} = 1 to 9 step 2 {
// ASIS:        affine.store
// ASIS-NOT:    systolic.matmul_tile
// OFF-LABEL:   func.func @stepped(
// OFF-NOT:     systolic.matmul_tile
// OFF:         affine.store
func.func @stepped(%A: memref<3x9xf32> {llvm.noalias}, %B: memref<9x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %k = 1 to 9 step 2 {
        %a = affine.load %A[%i, %k] : memref<3x9xf32>
        %b = affine.load %B[%k, %j] : memref<9x5xf32>
        %p = arith.mulf %a, %b : f32
        affine.store %p, %C[%i, %j] : memref<3x5xf32>
      }
    }
  }
  return
}

// -----

// 6. A nonzero lower bound: k = 2 to 6, last = 5.
// REMARK:      remark: affine assignment nest C = A * B: last write wins (docs/AFFINE_GEMM_MAPPING.md section 20.5): loop(s) 2 of the nest, which C does not read, run their last iteration only (the earlier ones write values overwritten later)
// REMARK:      remark: affine GEMM 3x5x1 lowered as 1 exact region(s) (an assignment C = A B: C zeroed before the tiles)
// CHECK-LABEL: func.func @lower_bound(
// CHECK:       memref.subview %arg0[0, 5]
// CHECK:       memref.subview %arg1[5, 0]
// CHECK-NOT:   affine.load
// CHECK:       systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1
// CHECK-NEXT:  bufferization.materialize_in_destination
// CUT-LABEL:   func.func @lower_bound(
// CUT:         memref.subview %arg0[0, 5] [3, 1] [1, 1] : memref<3x6xf32> to memref<3x1xf32, strided<[6, 1], offset: 5>>
// CUT:         memref.subview %arg1[5, 0] [1, 5] [1, 1] : memref<6x5xf32> to memref<1x5xf32, strided<[5, 1], offset: 25>>
// CUT:         systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1
// ASIS-LABEL:  func.func @lower_bound(
// ASIS:        affine.for %{{.*}} = 0 to 3 {
// ASIS-NEXT:   affine.for %{{.*}} = 0 to 5 {
// ASIS-NEXT:   affine.for %{{.*}} = 2 to 6 {
// ASIS:        affine.store
// ASIS-NOT:    systolic.matmul_tile
// OFF-LABEL:   func.func @lower_bound(
// OFF-NOT:     systolic.matmul_tile
// OFF:         affine.store
func.func @lower_bound(%A: memref<3x6xf32> {llvm.noalias}, %B: memref<6x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %k = 2 to 6 {
        %a = affine.load %A[%i, %k] : memref<3x6xf32>
        %b = affine.load %B[%k, %j] : memref<6x5xf32>
        %p = arith.mulf %a, %b : f32
        affine.store %p, %C[%i, %j] : memref<3x5xf32>
      }
    }
  }
  return
}

// -----

// 7. The T5 shape C[i][j] = A[i][k] * B[l][j] (k read by A only, l by B
//    only): both cut, C = A[:, 3] B[3, :].
// REMARK:      remark: affine assignment nest C = A * B: last write wins (docs/AFFINE_GEMM_MAPPING.md section 20.5): loop(s) 2+3 of the nest, which C does not read, run their last iteration only (the earlier ones write values overwritten later)
// REMARK:      remark: affine GEMM 3x5x1 lowered as 1 exact region(s) (an assignment C = A B: C zeroed before the tiles)
// CHECK-LABEL: func.func @t5(
// CHECK:       memref.subview %arg0[0, 3]
// CHECK:       memref.subview %arg1[3, 0]
// CHECK-NOT:   affine.load
// CHECK:       systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1
// CHECK-NEXT:  bufferization.materialize_in_destination
// CUT-LABEL:   func.func @t5(
// CUT:         memref.subview %arg0[0, 3] [3, 1] [1, 1] : memref<3x4xf32> to memref<3x1xf32, strided<[4, 1], offset: 3>>
// CUT:         memref.subview %arg1[3, 0] [1, 5] [1, 1] : memref<4x5xf32> to memref<1x5xf32, strided<[5, 1], offset: 15>>
// CUT:         systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1
// ASIS-LABEL:  func.func @t5(
// ASIS:        affine.for %{{.*}} = 0 to 3 {
// ASIS-NEXT:   affine.for %{{.*}} = 0 to 5 {
// ASIS-NEXT:   affine.for %{{.*}} = 0 to 4 {
// ASIS-NEXT:   affine.for %{{.*}} = 0 to 4 {
// ASIS:        affine.store
// ASIS-NOT:    systolic.matmul_tile
// OFF-LABEL:   func.func @t5(
// OFF-NOT:     systolic.matmul_tile
// OFF:         affine.store
func.func @t5(%A: memref<3x4xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 4 {
        affine.for %l = 0 to 4 {
          %a = affine.load %A[%i, %k] : memref<3x4xf32>
          %b = affine.load %B[%l, %j] : memref<4x5xf32>
          %p = arith.mulf %a, %b : f32
          affine.store %p, %C[%i, %j] : memref<3x5xf32>
        }
      }
    }
  }
  return
}

// -----

// 8. The T2 shape C[i][j] = A[i][x0 - x1 + 3] * B[x0][j]: x0 and x1 are cut
//    to 3 (the last writer of every element is x0 = x1 = 3), A column
//    3 - 3 + 3 = 3, B row 3.
// REMARK:      remark: affine assignment nest C = A * B: last write wins (docs/AFFINE_GEMM_MAPPING.md section 20.5): loop(s) 2+3 of the nest, which C does not read, run their last iteration only (the earlier ones write values overwritten later)
// REMARK:      remark: affine GEMM 3x5x1 lowered as 1 exact region(s) (an assignment C = A B: C zeroed before the tiles)
// CHECK-LABEL: func.func @t2(
// CHECK:       memref.subview %arg0[0, 3]
// CHECK:       memref.subview %arg1[3, 0]
// CHECK-NOT:   affine.load
// CHECK:       systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1
// CHECK-NEXT:  bufferization.materialize_in_destination
// CUT-LABEL:   func.func @t2(
// CUT:         memref.subview %arg0[0, 3] [3, 1] [1, 1] : memref<3x7xf32> to memref<3x1xf32, strided<[7, 1], offset: 3>>
// CUT:         memref.subview %arg1[3, 0] [1, 5] [1, 1] : memref<4x5xf32> to memref<1x5xf32, strided<[5, 1], offset: 15>>
// CUT:         systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1
// ASIS-LABEL:  func.func @t2(
// ASIS:        affine.for %{{.*}} = 0 to 3 {
// ASIS-NEXT:   affine.for %{{.*}} = 0 to 5 {
// ASIS-NEXT:   affine.for %{{.*}} = 0 to 4 {
// ASIS-NEXT:   affine.for %{{.*}} = 0 to 4 {
// ASIS:        affine.store
// ASIS-NOT:    systolic.matmul_tile
// OFF-LABEL:   func.func @t2(
// OFF-NOT:     systolic.matmul_tile
// OFF:         affine.store
func.func @t2(%A: memref<3x7xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %x0 = 0 to 4 {
        affine.for %x1 = 0 to 4 {
          %a = affine.load %A[%i, %x0 - %x1 + 3] : memref<3x7xf32>
          %b = affine.load %B[%x0, %j] : memref<4x5xf32>
          %p = arith.mulf %a, %b : f32
          affine.store %p, %C[%i, %j] : memref<3x5xf32>
        }
      }
    }
  }
  return
}

// -----

// 9. C not written once over the remaining loops: C[i + j] = A[i][k] * B[k][j].
//    k is cut (the cut is exact whatever C's map: for each element of C its
//    writers range over all of k), then the written-once rule refuses the
//    nest -- (i, j) and (i + 1, j - 1) write the same element -- and it runs
//    on the CPU with the cut loop (k = 3 only), the same final C.
//    WONCE replays the recognizer on the rewritten nest.
// REMARK:      remark: affine assignment nest C = A * B: last write wins (docs/AFFINE_GEMM_MAPPING.md section 20.5): loop(s) 2 of the nest, which C does not read, run their last iteration only (the earlier ones write values overwritten later)
// REMARK-NOT:  affine GEMM
// REMARK-NOT:  last write wins
// WONCE:       reason: C's view is not injective on the (I, J) box: the outputs with ordinals (I, J) and (I + 1, J + -1) write the same element of C: the assignment C = A * B overwrites it
// CHECK-LABEL: func.func @not_written_once(
// CHECK:       affine.for %{{.*}} = 0 to 3 {
// CHECK-NEXT:  affine.for %{{.*}} = 0 to 5 {
// CHECK-NEXT:  affine.for %{{.*}} = 3 to 4 {
// CHECK:       affine.store
// CHECK-NOT:   systolic.matmul_tile
// CUT-LABEL:   func.func @not_written_once(
// CUT:         affine.for %{{.*}} = 0 to 3 {
// CUT-NEXT:    affine.for %{{.*}} = 0 to 5 {
// CUT-NEXT:    affine.for %{{.*}} = 3 to 4 {
// CUT:         affine.store
// CUT-NOT:     systolic.matmul_tile
// ASIS-LABEL:  func.func @not_written_once(
// ASIS:        affine.for %{{.*}} = 0 to 3 {
// ASIS-NEXT:   affine.for %{{.*}} = 0 to 5 {
// ASIS-NEXT:   affine.for %{{.*}} = 0 to 4 {
// ASIS:        affine.store
// ASIS-NOT:    systolic.matmul_tile
func.func @not_written_once(%A: memref<3x4xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<7xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<3x4xf32>
        %b = affine.load %B[%k, %j] : memref<4x5xf32>
        %p = arith.mulf %a, %b : f32
        affine.store %p, %C[%i + %j] : memref<7xf32>
      }
    }
  }
  return
}

// -----

// 10. Aliasing: no llvm.noalias, A or B may overlap C -- a removed write
//     could be read by a later product: no cut (and the nest stays refused),
//     with the permission too.
// CHECK-LABEL: func.func @aliasing(
// CHECK:       affine.for %{{.*}} = 0 to 3 {
// CHECK-NEXT:  affine.for %{{.*}} = 0 to 5 {
// CHECK-NEXT:  affine.for %{{.*}} = 0 to 4 {
// CHECK:       affine.store
// CHECK-NOT:   systolic.matmul_tile
// CUT-LABEL:   func.func @aliasing(
// CUT:         affine.for %{{.*}} = 0 to 3 {
// CUT-NEXT:    affine.for %{{.*}} = 0 to 5 {
// CUT-NEXT:    affine.for %{{.*}} = 0 to 4 {
// CUT:         affine.store
// CUT-NOT:     systolic.matmul_tile
func.func @aliasing(%A: memref<3x4xf32>, %B: memref<4x5xf32>, %C: memref<3x5xf32>) {
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

// 11. A factor that is C itself: C[i][j] = C[i][k] * B[k][j] reads what an
//     earlier k wrote: no cut.
// CHECK-LABEL: func.func @in_place(
// CHECK:       affine.for %{{.*}} = 0 to 3 {
// CHECK-NEXT:  affine.for %{{.*}} = 0 to 5 {
// CHECK-NEXT:  affine.for %{{.*}} = 0 to 4 {
// CHECK:       affine.store
// CHECK-NOT:   systolic.matmul_tile
// CUT-LABEL:   func.func @in_place(
// CUT:         affine.for %{{.*}} = 0 to 3 {
// CUT-NEXT:    affine.for %{{.*}} = 0 to 5 {
// CUT-NEXT:    affine.for %{{.*}} = 0 to 4 {
// CUT:         affine.store
// CUT-NOT:     systolic.matmul_tile
func.func @in_place(%B: memref<5x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 4 {
        %a = affine.load %C[%i, %k] : memref<3x5xf32>
        %b = affine.load %B[%k, %j] : memref<5x5xf32>
        %p = arith.mulf %a, %b : f32
        affine.store %p, %C[%i, %j] : memref<3x5xf32>
      }
    }
  }
  return
}

// -----

// 12. An empty domain (i from 3 to 3): nothing is written, no last writer:
//     no cut (k keeps 0 to 4).
// CHECK-LABEL: func.func @empty_domain(
// CHECK:       affine.for %{{.*}} = 3 to 3 {
// CHECK-NEXT:  affine.for %{{.*}} = 0 to 5 {
// CHECK-NEXT:  affine.for %{{.*}} = 0 to 4 {
// CHECK:       affine.store
// CHECK-NOT:   systolic.matmul_tile
// CUT-LABEL:   func.func @empty_domain(
// CUT:         affine.for %{{.*}} = 3 to 3 {
// CUT-NEXT:    affine.for %{{.*}} = 0 to 5 {
// CUT-NEXT:    affine.for %{{.*}} = 0 to 4 {
// CUT:         affine.store
// CUT-NOT:     systolic.matmul_tile
func.func @empty_domain(%A: memref<3x4xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 3 to 3 {
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

// 13. A loop of one iteration that C does not read (k = 2 to 3): nothing to
//     cut, no remark; the K = 1 GEMM of column 2 of A and row 2 of B as before
//     (also with last-write-wins=false).
// REMARK:      remark: affine GEMM 3x5x1 lowered as 1 exact region(s) (an assignment C = A B: C zeroed before the tiles)
// CHECK-LABEL: func.func @one_iteration(
// CHECK:       memref.subview %arg0[0, 2]
// CHECK:       memref.subview %arg1[2, 0]
// CHECK:       systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1
// CUT-LABEL:   func.func @one_iteration(
// CUT:         memref.subview %arg0[0, 2] [3, 1] [1, 1] : memref<3x4xf32> to memref<3x1xf32, strided<[4, 1], offset: 2>>
// CUT:         memref.subview %arg1[2, 0] [1, 5] [1, 1] : memref<4x5xf32> to memref<1x5xf32, strided<[5, 1], offset: 10>>
// CUT:         systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1
// ASIS-LABEL:  func.func @one_iteration(
// ASIS:        affine.for %{{.*}} = 0 to 3 {
// ASIS-NEXT:   affine.for %{{.*}} = 0 to 5 {
// ASIS-NEXT:   affine.for %{{.*}} = 2 to 3 {
// ASIS-NOT:    systolic.matmul_tile
// OFF-LABEL:   func.func @one_iteration(
// OFF:         systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1
func.func @one_iteration(%A: memref<3x4xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %k = 2 to 3 {
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

// 14. A one-iteration loop l (2 to 3) next to a cut one (k): the remark names
//     k (loop 3) only, l keeps its bounds.
// REMARK:      remark: affine assignment nest C = A * B: last write wins (docs/AFFINE_GEMM_MAPPING.md section 20.5): loop(s) 3 of the nest, which C does not read, run their last iteration only (the earlier ones write values overwritten later)
// REMARK:      remark: affine GEMM 3x5x1 lowered as 1 exact region(s) (an assignment C = A B: C zeroed before the tiles)
// CHECK-LABEL: func.func @one_iteration_and_cut(
// CHECK:       memref.subview %arg0[0, 2, 3]
// CHECK:       memref.subview %arg1[2, 3, 0]
// CHECK-NOT:   affine.load
// CHECK:       systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1
// CHECK-NEXT:  bufferization.materialize_in_destination
// CUT-LABEL:   func.func @one_iteration_and_cut(
// CUT:         memref.subview %arg0[0, 2, 3] [3, 1, 1] [1, 1, 1] : memref<3x4x4xf32> to memref<3x1xf32, strided<[16, 4], offset: 11>>
// CUT:         memref.subview %arg1[2, 3, 0] [1, 1, 5] [1, 1, 1] : memref<4x4x5xf32> to memref<1x5xf32, strided<[20, 1], offset: 55>>
// CUT:         systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1
// ASIS-LABEL:  func.func @one_iteration_and_cut(
// ASIS:        affine.for %{{.*}} = 0 to 3 {
// ASIS-NEXT:   affine.for %{{.*}} = 2 to 3 {
// ASIS-NEXT:   affine.for %{{.*}} = 0 to 5 {
// ASIS-NEXT:   affine.for %{{.*}} = 0 to 4 {
// ASIS:        affine.store
// ASIS-NOT:    systolic.matmul_tile
// OFF-LABEL:   func.func @one_iteration_and_cut(
// OFF-NOT:     systolic.matmul_tile
// OFF:         affine.store
func.func @one_iteration_and_cut(%A: memref<3x4x4xf32> {llvm.noalias}, %B: memref<4x4x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %l = 2 to 3 {
      affine.for %j = 0 to 5 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%i, %l, %k] : memref<3x4x4xf32>
          %b = affine.load %B[%l, %k, %j] : memref<4x4x5xf32>
          %p = arith.mulf %a, %b : f32
          affine.store %p, %C[%i, %j] : memref<3x5xf32>
        }
      }
    }
  }
  return
}

// -----

// 15. An accumulation C += A * B: never cut (every k contributes); the
//     3 x 5 x 4 GEMM as before.
// REMARK-NOT:  last write wins
// REMARK:      remark: affine GEMM 3x5x4 lowered as 1 exact region(s){{$}}
// CHECK-LABEL: func.func @accumulation(
// CHECK:       systolic.matmul_tile {{.*}} m = 3 n = 5 k = 4
// CUT-LABEL:   func.func @accumulation(
// CUT:         systolic.matmul_tile {{.*}} m = 3 n = 5 k = 4
// ASIS-LABEL:  func.func @accumulation(
// ASIS:        affine.for %{{.*}} = 0 to 4 {
// ASIS-NOT:    systolic.matmul_tile
// OFF-LABEL:   func.func @accumulation(
// OFF:         systolic.matmul_tile {{.*}} m = 3 n = 5 k = 4
func.func @accumulation(%A: memref<3x4xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<3x4xf32>
        %b = affine.load %B[%k, %j] : memref<4x5xf32>
        %p = arith.mulf %a, %b : f32
        %c = affine.load %C[%i, %j] : memref<3x5xf32>
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<3x5xf32>
      }
    }
  }
  return
}

// -----

// 16. Case 3 with the product marked reassoc (fastmath<reassoc>, what
//     systolic-cc --fp-reassociation emits): the permission is the flag, so
//     the nest is cut and lowered without allow-fp-reassociation too (STRICT,
//     ASIS); with last-write-wins=false it stays refused.
// REMARK:      remark: affine assignment nest C = A * B: last write wins (docs/AFFINE_GEMM_MAPPING.md section 20.5): loop(s) 2 of the nest, which C does not read, run their last iteration only (the earlier ones write values overwritten later)
// REMARK:      remark: affine GEMM 3x5x1 lowered as 1 exact region(s) (an assignment C = A B: C zeroed before the tiles)
// CHECK-LABEL: func.func @k_inner_reassoc(
// CHECK:       memref.subview %arg0[0, 3]
// CHECK:       memref.subview %arg1[3, 0]
// CHECK-NOT:   affine.load
// CHECK:       systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1
// CHECK-NEXT:  bufferization.materialize_in_destination
// CUT-LABEL:   func.func @k_inner_reassoc(
// CUT:         memref.subview %arg0[0, 3] [3, 1] [1, 1] : memref<3x4xf32> to memref<3x1xf32, strided<[4, 1], offset: 3>>
// CUT:         memref.subview %arg1[3, 0] [1, 5] [1, 1] : memref<4x5xf32> to memref<1x5xf32, strided<[5, 1], offset: 15>>
// CUT:         systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1
// ASIS-LABEL:  func.func @k_inner_reassoc(
// ASIS:        memref.subview %arg0[0, 3] [3, 1] [1, 1] : memref<3x4xf32> to memref<3x1xf32, strided<[4, 1], offset: 3>>
// ASIS:        memref.subview %arg1[3, 0] [1, 5] [1, 1] : memref<4x5xf32> to memref<1x5xf32, strided<[5, 1], offset: 15>>
// ASIS-NOT:    affine.load
// ASIS:        systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1
// OFF-LABEL:   func.func @k_inner_reassoc(
// OFF-NOT:     systolic.matmul_tile
// OFF:         affine.store
func.func @k_inner_reassoc(%A: memref<3x4xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<3x4xf32>
        %b = affine.load %B[%k, %j] : memref<4x5xf32>
        %p = arith.mulf %a, %b fastmath<reassoc> : f32
        affine.store %p, %C[%i, %j] : memref<3x5xf32>
      }
    }
  }
  return
}
