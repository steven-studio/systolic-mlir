module {
  systolic.device @acc_4x4_1
      rows = 4 cols = 4 depth = 1
      dataflow = weight_stationary
      k_max = 16

  func.func @main(
      %A: tensor<16x16xf32>,
      %B: tensor<16x16xf32>,
      %C: tensor<16x16xf32>
  ) -> tensor<16x16xf32> {

    %r = linalg.matmul
        ins(%A, %B : tensor<16x16xf32>, tensor<16x16xf32>)
        outs(%C : tensor<16x16xf32>)
        -> tensor<16x16xf32>

    return %r : tensor<16x16xf32>
  }
}
