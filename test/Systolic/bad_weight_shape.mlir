// RUN: systolic-opt %s -verify-diagnostics
func.func @bad_weight_shape(%B: tensor<4x4xf32>, %A: tensor<8x5xf32>, %C: tensor<8x4xf32>)
    -> tensor<8x4xf32> {
  // expected-error @+1 {{化约维度 (K) 对不上}}
  %r = systolic.pe_array<8 x 4> stationary(weight) %B, %A, %C
       : (tensor<4x4xf32>, tensor<8x5xf32>, tensor<8x4xf32>) -> tensor<8x4xf32>
  return %r : tensor<8x4xf32>
}
