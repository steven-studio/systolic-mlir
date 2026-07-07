// RUN: systolic-opt %s --convert-matmul-to-systolic="rows=8 cols=8" --expand-pe-array-to-mac | FileCheck %s
func.func @matmul_8x8x8(%A: tensor<8x8xf32>, %B: tensor<8x8xf32>, %C: tensor<8x8xf32>)
    -> tensor<8x8xf32> {
  // CHECK-NOT: systolic.pe_array
  // CHECK-NOT: systolic.stream
  // CHECK: scf.for
  // CHECK: scf.for
  // CHECK: scf.for
  // CHECK: systolic.mac
  %result = linalg.matmul ins(%A, %B : tensor<8x8xf32>, tensor<8x8xf32>)
                          outs(%C : tensor<8x8xf32>) -> tensor<8x8xf32>
  return %result : tensor<8x8xf32>
}
