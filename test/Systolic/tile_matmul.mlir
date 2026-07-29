module {
  systolic.device @acc_8x8 rows = 8 cols = 8 depth = 1 dataflow = weight_stationary
  systolic.device @acc_4x4 rows = 4 cols = 4 depth = 1 dataflow = weight_stationary

  func.func @big_matmul(%A: tensor<16x16xf32>, %B: tensor<16x16xf32>, %C: tensor<16x16xf32>) -> tensor<16x16xf32> {
    %result = linalg.matmul ins(%A, %B : tensor<16x16xf32>, tensor<16x16xf32>)
                            outs(%C : tensor<16x16xf32>) -> tensor<16x16xf32>
    return %result : tensor<16x16xf32>
  }
}
