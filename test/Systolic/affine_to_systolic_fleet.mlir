// RUN: systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic \
// RUN:   -split-input-file %s | FileCheck %s

// An affine GEMM on the 1x8x8 + 3x4x4 fleet: the recognized nest is
// partitioned by the same binary-search plan --systolic-binary-search-tile-
// matmul uses for a 16x16x16 linalg.matmul (device_plus_linalg_8x8_3x4x4),
// proven an exact partition of the iteration domain, and replaced by one
// scheduled matmul_tile per region on views of the memrefs.

// CHECK-LABEL: func.func @gemm16
// CHECK-NOT:   affine.for
// CHECK:       %[[A:.*]] = bufferization.to_tensor %arg0 restrict : memref<16x16xf32> to tensor<16x16xf32>
// CHECK:       %[[B:.*]] = bufferization.to_tensor %arg1 restrict : memref<16x16xf32> to tensor<16x16xf32>
// CHECK:       %[[C:.*]] = bufferization.to_tensor %arg2 restrict writable : memref<16x16xf32> to tensor<16x16xf32>
// CHECK:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 0
// CHECK:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 125
// CHECK:       systolic.matmul_tile {{.*}} m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 0
// CHECK:       systolic.matmul_tile {{.*}} m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 0
// CHECK:       systolic.matmul_tile {{.*}} m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 0
// CHECK:       systolic.matmul_tile {{.*}} m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 117
// CHECK:       systolic.matmul_tile {{.*}} m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 117
// CHECK:       systolic.matmul_tile {{.*}} m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 117
// CHECK:       systolic.matmul_tile {{.*}} m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 234
// CHECK:       systolic.matmul_tile {{.*}} m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 234
// CHECK-NOT:   systolic.matmul_tile
// CHECK:       bufferization.materialize_in_destination %{{.*}} in restrict writable %arg2
// CHECK-NOT:   affine.for
func.func @gemm16(%A: memref<16x16xf32> {llvm.noalias},
                  %B: memref<16x16xf32> {llvm.noalias},
                  %C: memref<16x16xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 16 {
      affine.for %k = 0 to 16 {
        %a = affine.load %A[%i, %k] : memref<16x16xf32>
        %b = affine.load %B[%k, %j] : memref<16x16xf32>
        %c = affine.load %C[%i, %j] : memref<16x16xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<16x16xf32>
      }
    }
  }
  return
}

// -----

// Edge tiles and K-folding: K = 40 > k_max = 16 folds every output block
// into invocations of 16, 16 and 8 reduction steps, each one's c_in the
// previous one's result, in increasing K.

// CHECK-LABEL: func.func @gemm20x12x40
// CHECK:       %[[C0:.*]] = tensor.extract_slice %{{.*}}[0, 0] [8, 8] [1, 1]
// CHECK:       tensor.extract_slice %{{.*}}[0, 0] [8, 16] [1, 1]
// CHECK:       tensor.extract_slice %{{.*}}[0, 0] [16, 8] [1, 1]
// CHECK:       %[[T0:.*]] = systolic.matmul_tile %{{.*}}, %{{.*}}, %[[C0]] m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 0
// CHECK:       tensor.extract_slice %{{.*}}[0, 16] [8, 16] [1, 1]
// CHECK:       tensor.extract_slice %{{.*}}[16, 0] [16, 8] [1, 1]
// CHECK:       %[[T1:.*]] = systolic.matmul_tile %{{.*}}, %{{.*}}, %[[T0]] m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 125
// CHECK:       tensor.extract_slice %{{.*}}[0, 32] [8, 8] [1, 1]
// CHECK:       tensor.extract_slice %{{.*}}[32, 0] [8, 8] [1, 1]
// CHECK:       %[[T2:.*]] = systolic.matmul_tile %{{.*}}, %{{.*}}, %[[T1]] m = 8 n = 8 k = 8 on @acc_8x8_0 est_cycles = 117 start_cycle = 250
// CHECK:       tensor.insert_slice %[[T2]] into %{{.*}}[0, 0] [8, 8] [1, 1]
// CHECK-COUNT-24: systolic.matmul_tile
// CHECK-NOT:   systolic.matmul_tile
// CHECK:       bufferization.materialize_in_destination
func.func @gemm20x12x40(%A: memref<20x40xf32> {llvm.noalias},
                        %B: memref<40x12xf32> {llvm.noalias},
                        %C: memref<20x12xf32> {llvm.noalias}) {
  affine.for %i = 0 to 20 {
    affine.for %j = 0 to 12 {
      affine.for %k = 0 to 40 {
        %a = affine.load %A[%i, %k] : memref<20x40xf32>
        %b = affine.load %B[%k, %j] : memref<40x12xf32>
        %c = affine.load %C[%i, %j] : memref<20x12xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<20x12xf32>
      }
    }
  }
  return
}

// -----

// Loops in k, i, j order, operands at offsets and strides inside larger
// memrefs: A and C rows at 3 + 2 * i, B and C columns at 1 + j. The logical
// matrices are strided views; the tiles are slices of them. (The previous
// lowering assumed loops in I, J, K order and gathered A[0, i] for A[i, k].)

// CHECK-LABEL: func.func @permuted_strided
// CHECK:       %[[AV:.*]] = memref.subview %arg0[3, 0] [6, 4] [2, 1] : memref<20x4xf32> to memref<6x4xf32, strided<[8, 1], offset: 12>>
// CHECK:       bufferization.to_tensor %[[AV]] restrict : {{.*}} to tensor<6x4xf32>
// CHECK:       %[[BV:.*]] = memref.subview %arg1[0, 1] [4, 4] [1, 1] : memref<4x5xf32> to memref<4x4xf32, strided<[5, 1], offset: 1>>
// CHECK:       bufferization.to_tensor %[[BV]] restrict : {{.*}} to tensor<4x4xf32>
// CHECK:       %[[CV:.*]] = memref.subview %arg2[3, 1] [6, 4] [2, 1] : memref<20x5xf32> to memref<6x4xf32, strided<[10, 1], offset: 16>>
// CHECK:       bufferization.to_tensor %[[CV]] restrict writable : {{.*}} to tensor<6x4xf32>
// CHECK:       systolic.matmul_tile {{.*}} m = 4 n = 4 k = 4 on @acc_4x4_0
// CHECK:       systolic.matmul_tile {{.*}} m = 2 n = 4 k = 4 on @acc_4x4_1
// CHECK:       bufferization.materialize_in_destination %{{.*}} in restrict writable %[[CV]]
func.func @permuted_strided(%A: memref<20x4xf32> {llvm.noalias},
                            %B: memref<4x5xf32> {llvm.noalias},
                            %C: memref<20x5xf32> {llvm.noalias}) {
  affine.for %k = 0 to 4 {
    affine.for %i = 0 to 6 {
      affine.for %j = 0 to 4 {
        %a = affine.load %A[3 + 2 * %i, %k] : memref<20x4xf32>
        %b = affine.load %B[%k, %j + 1] : memref<4x5xf32>
        %c = affine.load %C[3 + 2 * %i, %j + 1] : memref<20x5xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[3 + 2 * %i, %j + 1] : memref<20x5xf32>
      }
    }
  }
  return
}

// -----

// An enclosing loop whose IV no access uses repeats the accumulation: the
// GEMM runs twice. The nest anchored at %l is not a GEMM over (l, k, i, j);
// the one anchored at %k is, so the tiles replace it inside the %l loop.
// (Previously the %l nest was recognized and replaced, running it once.)

// CHECK-LABEL: func.func @repeated
// CHECK:       affine.for %{{.*}} = 0 to 2 {
// CHECK-NOT:     affine.for
// CHECK:         systolic.matmul_tile {{.*}} on @acc_8x8_0
// CHECK:         bufferization.materialize_in_destination
// CHECK:       }
// CHECK-NEXT:  return
func.func @repeated(%A: memref<8x8xf32> {llvm.noalias},
                    %B: memref<8x8xf32> {llvm.noalias},
                    %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %l = 0 to 2 {
    affine.for %k = 0 to 8 {
      affine.for %i = 0 to 8 {
        affine.for %j = 0 to 8 {
          %a = affine.load %A[%i, %k] : memref<8x8xf32>
          %b = affine.load %B[%k, %j] : memref<8x8xf32>
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

// The leading extent of a C array parameter is not part of its type
// (float A[][16] is float (*)[16]), so a C front end gives memref<?x16xf32>.
// The views hold exactly the elements the nest reads and writes, so they are
// in bounds whenever the nest is; a static extent the accesses provably
// exceed is still refused (affine_to_systolic_refused.mlir, @out_of_bounds).

// CHECK-LABEL: func.func @dynamic_rows
// CHECK:       %[[AV:.*]] = memref.subview %arg0[0, 0] [16, 16] [1, 1] : memref<?x16xf32> to memref<16x16xf32, strided<[16, 1]>>
// CHECK:       bufferization.to_tensor %[[AV]] restrict : memref<16x16xf32, strided<[16, 1]>> to tensor<16x16xf32>
// CHECK-COUNT-10: systolic.matmul_tile {{.*}} on @acc_
// CHECK:       bufferization.materialize_in_destination
func.func @dynamic_rows(%A: memref<?x16xf32> {llvm.noalias},
                        %B: memref<?x16xf32> {llvm.noalias},
                        %C: memref<?x16xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 16 {
      affine.for %k = 0 to 16 {
        %a = affine.load %A[%i, %k] : memref<?x16xf32>
        %b = affine.load %B[%k, %j] : memref<?x16xf32>
        %c = affine.load %C[%i, %j] : memref<?x16xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<?x16xf32>
      }
    }
  }
  return
}
