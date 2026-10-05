module {
  systolic.device @acc_4x4_1
      rows = 4 cols = 4 depth = 1
      dataflow = weight_stationary

  func.func @main(
      %A: tensor<4x4xf32>,
      %B: tensor<4x4xf32>,
      %C: tensor<4x4xf32>
  ) -> tensor<4x4xf32> {

    %r = linalg.matmul
        ins(%A, %B : tensor<4x4xf32>, tensor<4x4xf32>)
        outs(%C : tensor<4x4xf32>)
        -> tensor<4x4xf32>

    return %r : tensor<4x4xf32>
  }
}
