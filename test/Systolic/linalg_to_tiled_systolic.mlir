module {
  func.func @matmul_16x16(
      %A: tensor<16x16xf32>,
      %B: tensor<16x16xf32>,
      %C: tensor<16x16xf32>)
      -> tensor<16x16xf32> {

    %0 = linalg.matmul
      ins(%A, %B : tensor<16x16xf32>, tensor<16x16xf32>)
      outs(%C : tensor<16x16xf32>)
      -> tensor<16x16xf32>

    return %0 : tensor<16x16xf32>
  }
}
