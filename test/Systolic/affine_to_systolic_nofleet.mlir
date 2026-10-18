// RUN: systolic-opt --affine-matmul-to-systolic -split-input-file %s \
// RUN:   | FileCheck %s

// Without a systolic.device fleet the whole iteration domain is one region:
// a single unassigned matmul_tile of the full logical shape, for device
// selection to place later.

// Distinct allocations cannot overlap, with or without llvm.noalias.
// CHECK-LABEL: func.func @allocations
// CHECK-NOT:   affine.for
// CHECK:       %[[C:.*]] = memref.alloc() : memref<6x5xf32>
// CHECK:       systolic.matmul_tile {{.*}} m = 6 n = 5 k = 3 : (tensor<6x3xf32>, tensor<3x5xf32>, tensor<6x5xf32>) -> tensor<6x5xf32>
// CHECK-NOT:   on @
// CHECK:       bufferization.materialize_in_destination %{{.*}} in restrict writable %[[C]]
func.func @allocations() -> memref<6x5xf32> {
  %A = memref.alloc() : memref<6x3xf32>
  %B = memref.alloc() : memref<3x5xf32>
  %C = memref.alloc() : memref<6x5xf32>
  affine.for %i = 0 to 6 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 3 {
        %a = affine.load %A[%i, %k] : memref<6x3xf32>
        %b = affine.load %B[%k, %j] : memref<3x5xf32>
        %c = affine.load %C[%i, %j] : memref<6x5xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<6x5xf32>
      }
    }
  }
  memref.dealloc %A : memref<6x3xf32>
  memref.dealloc %B : memref<3x5xf32>
  return %C : memref<6x5xf32>
}

// -----

// C += A x A: one read-only tensor serves as both operands.
// CHECK-LABEL: func.func @square
// CHECK:       %[[A:.*]] = bufferization.to_tensor %arg0 restrict : memref<4x4xf32> to tensor<4x4xf32>
// CHECK-NOT:   bufferization.to_tensor %arg0
// CHECK:       systolic.matmul_tile %[[A]], %[[A]], %{{.*}} m = 4 n = 4 k = 4
func.func @square(%A: memref<4x4xf32>, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<4x4xf32>
        %b = affine.load %A[%k, %j] : memref<4x4xf32>
        %c = affine.load %C[%i, %j] : memref<4x4xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<4x4xf32>
      }
    }
  }
  return
}
