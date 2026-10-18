// RUN: systolic-opt --test-affine-matmul-discovery %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s
//
// Two candidates share the MAC: the nest anchored at %l is rejected (no
// access uses %l, so it repeats the GEMM rather than being one of its
// dimensions), the one anchored at %k is the GEMM.
//
// CHECK: recognition result: failure
// CHECK: unique logical matmul count: 1
// CHECK: recognition result: success
// CHECK: T[0] = [0 1 0]
// CHECK: T[1] = [0 0 1]
// CHECK: T[2] = [1 0 0]
// CHECK: c = [0 0 0]
// CHECK: row extent: static 16
// CHECK: column extent: static 16
// CHECK: reduction extent: static 16
// CHECK: dense logical GEMM: yes, I/J/K on loops 1/2/0
// CHECK: partition count: 1

func.func @affine_gemm_nested(
    %A: memref<16x16xf32>,
    %B: memref<16x16xf32>,
    %C: memref<16x16xf32>) {

  // Extra enclosing loop, analogous to Livermore Kernel 21's `l`.
  affine.for %l = 0 to 2 {
    affine.for %k = 0 to 16 {
      affine.for %i = 0 to 16 {
        affine.for %j = 0 to 16 {
          %a = affine.load %A[%i, %k] : memref<16x16xf32>
          %b = affine.load %B[%k, %j] : memref<16x16xf32>
          %c = affine.load %C[%i, %j] : memref<16x16xf32>

          %product = arith.mulf %a, %b : f32
          %sum = arith.addf %c, %product : f32

          affine.store %sum, %C[%i, %j] : memref<16x16xf32>
        }
      }
    }
  }

  return
}
