// RUN: systolic-opt --test-affine-matmul-discovery %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s
//
// A tiled GEMM: I = 8 i0 + i1, J = 4 j0 + j1, K = 8 k0 + k1. Each logical
// coordinate is driven by two loops at different depths (0 and 3, 1 and 4,
// 2 and 5), which number it densely.
//
// CHECK: unique logical matmul count: 1
// CHECK: recognition result: success
// CHECK: T[0] = [8 0 0 1 0 0]
// CHECK: T[1] = [0 4 0 0 1 0]
// CHECK: T[2] = [0 0 8 0 0 1]
// CHECK: dense logical GEMM: yes, I/J/K on loops 0+3/1+4/2+5

func.func @tiled(%A: memref<16x24xf32>, %B: memref<24x16xf32>,
                 %C: memref<16x16xf32>) {
  affine.for %i0 = 0 to 2 {
    affine.for %j0 = 0 to 4 {
      affine.for %k0 = 0 to 3 {
        affine.for %i1 = 0 to 8 {
          affine.for %j1 = 0 to 4 {
            affine.for %k1 = 0 to 8 {
              %a = affine.load %A[8 * %i0 + %i1, 8 * %k0 + %k1] : memref<16x24xf32>
              %b = affine.load %B[8 * %k0 + %k1, 4 * %j0 + %j1] : memref<24x16xf32>
              %c = affine.load %C[8 * %i0 + %i1, 4 * %j0 + %j1] : memref<16x16xf32>
              %p = arith.mulf %a, %b : f32
              %s = arith.addf %c, %p : f32
              affine.store %s, %C[8 * %i0 + %i1, 4 * %j0 + %j1] : memref<16x16xf32>
            }
          }
        }
      }
    }
  }
  return
}
