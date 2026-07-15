func.func @matmul_4x4x4(%A: tensor<4x4xf32>, %B: tensor<4x4xf32>, %C: tensor<4x4xf32>)
    -> tensor<4x4xf32> {
  %result = linalg.matmul ins(%A, %B : tensor<4x4xf32>, tensor<4x4xf32>)
                          outs(%C : tensor<4x4xf32>) -> tensor<4x4xf32>
  return %result : tensor<4x4xf32>
}
