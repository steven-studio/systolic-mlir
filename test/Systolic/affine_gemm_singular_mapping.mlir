// RUN: systolic-opt --test-affine-matmul-discovery %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s
//
// A singular mapping: the positional reading I = x + z, J = y + z, K = x + y + 2z has a
// singular T ([[1,0,1],[0,1,1],[1,1,2]]). Every loop is read by A, B and C, so every loop
// is a batch loop (docs/AFFINE_GEMM_MAPPING.md section 19.2): no row, column or reduction
// coordinate -- an element-wise product, refused at recognition since 2026-10-16 (a
// coupled positional reading is never lowerable, so the readers' reason is the one given;
// before, the dense partition refused the positional reading: "loop 0 drives I and K").
//
// CHECK: recognition result: failure
// CHECK: reason: 1 coordinate(s) are read by all three operands and none by two: an element-wise product
// CHECK: class: PROVEN_INVALID
// CHECK: unique logical matmul count: 0

func.func @affine_gemm_singular_mapping(
    %A: memref<7x13xf32>,
    %B: memref<13x7xf32>,
    %C: memref<7x7xf32>) {
  affine.for %x = 0 to 4 {
    affine.for %y = 0 to 4 {
      affine.for %z = 0 to 4 {
        %a = affine.load %A[%x + %z, %x + %y + 2 * %z]
            : memref<7x13xf32>
        %b = affine.load %B[%x + %y + 2 * %z, %y + %z]
            : memref<13x7xf32>
        %c = affine.load %C[%x + %z, %y + %z]
            : memref<7x7xf32>

        %product = arith.mulf %a, %b : f32
        %sum = arith.addf %c, %product : f32

        affine.store %sum, %C[%x + %z, %y + %z]
            : memref<7x7xf32>
      }
    }
  }

  return
}
