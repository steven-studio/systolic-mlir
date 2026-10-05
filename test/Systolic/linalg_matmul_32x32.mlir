module {
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
