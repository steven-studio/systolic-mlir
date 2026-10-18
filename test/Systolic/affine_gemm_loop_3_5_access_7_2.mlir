// RUN: systolic-opt --test-affine-matmul-discovery %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s
//
// CHECK: unique logical matmul count: 1
// CHECK: recognition result: success
// CHECK: T[0] = [2 0 0]
// CHECK: T[1] = [0 1 0]
// CHECK: T[2] = [0 0 1]
// CHECK: c = [7 0 0]
// CHECK: row extent: static 20
// CHECK: column extent: static 4
// CHECK: reduction extent: static 4
// CHECK: dense logical GEMM: yes, I/J/K on loops 0/1/2
// CHECK: partition count: 1

func.func @affine_gemm_loop_3_5_access_7_2(
    %A: memref<204x4xf32>,
    %B: memref<4x4xf32>,
    %C: memref<204x4xf32>) {
  affine.for %x = 3 to 103 step 5 {
    affine.for %y = 0 to 4 {
      affine.for %z = 0 to 4 {
        %a = affine.load %A[7 + 2 * %x, %z] : memref<204x4xf32>
        %b = affine.load %B[%z, %y] : memref<4x4xf32>
        %c = affine.load %C[7 + 2 * %x, %y] : memref<204x4xf32>

        %product = arith.mulf %a, %b : f32
        %sum = arith.addf %c, %product : f32

        affine.store %sum, %C[7 + 2 * %x, %y] : memref<204x4xf32>
      }
    }
  }

  return
}
