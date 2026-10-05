module {
  func.func @main(
      %A: tensor<1x512xf32>,
      %B: tensor<512x1000xf32>,
      %C: tensor<1x1000xf32>
  ) -> tensor<1x1000xf32> {

    %0 = linalg.matmul
      ins(%A, %B : tensor<1x512xf32>, tensor<512x1000xf32>)
      outs(%C : tensor<1x1000xf32>)
      -> tensor<1x1000xf32>

    return %0 : tensor<1x1000xf32>
  }
}
