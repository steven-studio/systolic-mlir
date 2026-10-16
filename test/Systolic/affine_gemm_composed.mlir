// RUN: systolic-opt --test-affine-matmul-discovery %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s
//
// C[x + y][y] += A[x + y][z] * B[z][y]. RE-DECIDED 2026-10-16 (docs/AFFINE_GEMM_MAPPING.md
// section 19.3): y is read by all three operands, a batch loop (the row of P*), and C and
// A may carry it in their I dimension; per y the GEMV C'(I) = C[I + y][y] +=
// A[I + y][K] * B[K][y] (I = x, K = z, J absent). Before: the positional reading I = x + y,
// J = y, refused by the dense partition ("loop 1 drives I and J").
//
// CHECK: unique logical matmul count: 1
// CHECK: recognition result: success
// CHECK: T[0] = [0 1 0]
// CHECK: T[1] = [1 0 0]
// CHECK: T[2] = [0 0 0]
// CHECK: T[3] = [0 0 1]
// CHECK: c = [0 0 0 0]
// CHECK: batch terms C: dimension 0 += 1*x1, dimension 1 = 0 + 1*x1
// CHECK: row extent: static 4
// CHECK: column extent: unknown
// CHECK: reduction extent: static 4
// CHECK: dense logical GEMM: yes, I/J/K on loops 0//2, batched over loop(s) 1 (4 batch iteration(s))
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
