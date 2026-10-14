// RUN: systolic-opt --test-affine-matmul-discovery %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s
//
// CHECK: unique logical matmul count: 1
// CHECK: recognition result: success
// CHECK: T[0] = [1 0 0]
// CHECK: T[1] = [0 1 0]
// CHECK: T[2] = [0 0 1]
// CHECK: c = [0 0 0]
// CHECK: row extent: static 4
// CHECK: column extent: static 4
// CHECK: reduction extent: static 4
// CHECK: dense logical GEMM: yes, I/J/K on loops 0/1/2
// CHECK: partition count: 1

func.func @affine_gemm_nonzero_origin(
    %A: memref<5x5xf32>,
    %B: memref<5x5xf32>,
    %C: memref<5x5xf32>) {
  affine.for %x = 1 to 5 {
    affine.for %y = 1 to 5 {
      affine.for %z = 1 to 5 {
        %a = affine.load %A[%x, %z] : memref<5x5xf32>
        %b = affine.load %B[%z, %y] : memref<5x5xf32>
        %c = affine.load %C[%x, %y] : memref<5x5xf32>

        %product = arith.mulf %a, %b : f32
        %sum = arith.addf %c, %product : f32

        affine.store %sum, %C[%x, %y] : memref<5x5xf32>
      }
    }
  }

  return
}
