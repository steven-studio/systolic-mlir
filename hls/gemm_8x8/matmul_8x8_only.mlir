func.func @matmul_8x8x8(%A: tensor<8x8xf32>, %B: tensor<8x8xf32>, %C: tensor<8x8xf32>)
    -> tensor<8x8xf32> {
  %result = linalg.matmul ins(%A, %B : tensor<8x8xf32>, tensor<8x8xf32>)
                          outs(%C : tensor<8x8xf32>) -> tensor<8x8xf32>
  return %result : tensor<8x8xf32>
}
