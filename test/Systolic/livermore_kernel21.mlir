// RUN: systolic-opt --test-affine-matmul-discovery %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s
//
// CHECK: recognition result: failure
// CHECK: unique logical matmul count: 1
// CHECK: recognition result: success
// CHECK: T[0] = [0 0 1]
// CHECK: T[1] = [0 1 0]
// CHECK: T[2] = [1 0 0]
// CHECK: c = [0 0 0]
// CHECK: row extent: symbolic <block argument> of type 'index' at index: 3
// CHECK: column extent: static 25
// CHECK: reduction extent: static 25
// CHECK: dense logical GEMM: no (the loop bounds are not static, so the iteration domain has no static normalized extent)
// CHECK: partition count: 1

func.func @livermore_kernel21(
    %px: memref<101x25xf32>,
    %vy: memref<25x101xf32>,
    %cx: memref<101x25xf32>,
    %n: index) {

  // Extracted from Livermore Kernel 21:
  //
  // for (l = 1; l <= loop; l++)
  //   for (k = 0; k < 25; k++)
  //     for (i = 0; i < 25; i++)
  //       for (j = 0; j < n; j++)
  //         px[j][i] += vy[k][i] * cx[j][k];

  affine.for %l = 0 to 2 {
    affine.for %k = 0 to 25 {
      affine.for %i = 0 to 25 {
        affine.for %j = 0 to %n {
          %a = affine.load %vy[%k, %i]
              : memref<25x101xf32>
          %b = affine.load %cx[%j, %k]
              : memref<101x25xf32>
          %c = affine.load %px[%j, %i]
              : memref<101x25xf32>

          %product = arith.mulf %a, %b : f32
          %sum = arith.addf %c, %product : f32

          affine.store %sum, %px[%j, %i]
              : memref<101x25xf32>
        }
      }
    }
  }

  return
}
