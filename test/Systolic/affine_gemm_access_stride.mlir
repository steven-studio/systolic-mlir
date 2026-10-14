// RUN: systolic-opt --test-affine-matmul-discovery %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s
//
// CHECK: unique logical matmul count: 1
// CHECK: recognition result: success
// CHECK: T[0] = [8 0 0]
// CHECK: T[1] = [0 1 0]
// CHECK: T[2] = [0 0 1]
// CHECK: c = [5 0 0]
// CHECK: row extent: static 20
// CHECK: column extent: static 4
// CHECK: reduction extent: static 4
// CHECK: dense logical GEMM: yes, I/J/K on loops 0/1/2
// CHECK: partition count: 1

func.func @affine_gemm_access_stride(
    %A: memref<165x4xf32>,
    %B: memref<4x4xf32>,
    %C: memref<165x4xf32>) {
  affine.for %q = 0 to 20 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[5 + 8 * %q, %k] : memref<165x4xf32>
        %b = affine.load %B[%k, %j] : memref<4x4xf32>
        %c = affine.load %C[5 + 8 * %q, %j] : memref<165x4xf32>

        %product = arith.mulf %a, %b : f32
        %sum = arith.addf %c, %product : f32

        affine.store %sum, %C[5 + 8 * %q, %j] : memref<165x4xf32>
      }
    }
  }

  return
}
