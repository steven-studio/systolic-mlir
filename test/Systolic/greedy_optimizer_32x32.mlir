module {
  systolic.device @acc_16x16
    rows = 16
    cols = 16
    depth = 1
    dataflow = output_stationary
    tile_overhead = 10

  systolic.device @acc_8x8
    rows = 8
    cols = 8
    depth = 1
    dataflow = output_stationary
    tile_overhead = 10

  systolic.device @acc_4x4
    rows = 4
    cols = 4
    depth = 1
    dataflow = output_stationary
    tile_overhead = 10

  func.func @main(
      %A: tensor<32x32xf32>,
      %B: tensor<32x32xf32>,
      %C: tensor<32x32xf32>
  ) -> tensor<32x32xf32> {

    %0 = linalg.matmul
      ins(%A, %B : tensor<32x32xf32>, tensor<32x32xf32>)
      outs(%C : tensor<32x32xf32>)
      -> tensor<32x32xf32>

    return %0 : tensor<32x32xf32>
  }
}
