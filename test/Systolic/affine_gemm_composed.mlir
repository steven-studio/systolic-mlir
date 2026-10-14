// RUN: systolic-opt --test-affine-matmul-discovery %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s
//
// CHECK: unique logical matmul count: 1
// CHECK: recognition result: success
// CHECK: T[0] = [1 1 0]
// CHECK: T[1] = [0 1 0]
// CHECK: T[2] = [0 0 1]
// CHECK: c = [0 0 0]
// CHECK: row extent: unknown
// CHECK: column extent: static 4
// CHECK: reduction extent: static 4
// CHECK: dense logical GEMM: no (logical coordinate I depends on 2 loops; only one loop per coordinate is supported)
// CHECK: partition count: 1

func.func @affine_gemm_composed(
    %A: memref<8x4xf32>,
    %B: memref<4x4xf32>,
    %C: memref<8x4xf32>) {
  affine.for %x = 0 to 4 {
    affine.for %y = 0 to 4 {
      affine.for %z = 0 to 4 {
        %a = affine.load %A[%x + %y, %z] : memref<8x4xf32>
        %b = affine.load %B[%z, %y] : memref<4x4xf32>
        %c = affine.load %C[%x + %y, %y] : memref<8x4xf32>

        %product = arith.mulf %a, %b : f32
        %sum = arith.addf %c, %product : f32

        affine.store %sum, %C[%x + %y, %y] : memref<8x4xf32>
      }
    }
  }

  return
}
