// RUN: systolic-opt --test-affine-matmul-discovery %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s
//
// CHECK: recognition result: failure
// CHECK: unique logical matmul count: 1
// CHECK: recognition result: success
// CHECK: T[0] = [1 0 0 0]
// CHECK: T[1] = [0 1 0 0]
// CHECK: T[2] = [0 0 1 0]
// CHECK: T[3] = [0 0 0 1]
// CHECK: c = [0 0 0 0]
// CHECK: row extent: static 4
// CHECK: column extent: static 4
// CHECK: reduction extent: static 4
// CHECK: dense logical GEMM: yes, I/J/K on loops 1/2/3, batched over loop(s) 0 (2 batch iteration(s))
// CHECK: partition count: 1

func.func @affine_gemm_batched(
    %A: memref<2x4x4xf32>,
    %B: memref<2x4x4xf32>,
    %C: memref<2x4x4xf32>) {
  affine.for %b = 0 to 2 {
    affine.for %i = 0 to 4 {
      affine.for %j = 0 to 4 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%b, %i, %k] : memref<2x4x4xf32>
          %bb = affine.load %B[%b, %k, %j] : memref<2x4x4xf32>
          %c = affine.load %C[%b, %i, %j] : memref<2x4x4xf32>
          %mul = arith.mulf %a, %bb : f32
          %add = arith.addf %c, %mul : f32
          affine.store %add, %C[%b, %i, %j] : memref<2x4x4xf32>
        }
      }
    }
  }
  return
}
