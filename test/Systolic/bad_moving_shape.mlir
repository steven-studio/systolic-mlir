// RUN: systolic-opt %s -verify-diagnostics
func.func @bad_moving_shape(%B: tensor<4x4xf32>, %A: tensor<6x4xf32>, %C: tensor<8x4xf32>)
    -> tensor<8x4xf32> {
  // expected-error @+1 {{第 0 维 (M) 应该等于 rows}}
  %r = systolic.pe_array<8 x 4> stationary(weight) %B, %A, %C
       : (tensor<4x4xf32>, tensor<6x4xf32>, tensor<8x4xf32>) -> tensor<8x4xf32>
  return %r : tensor<8x4xf32>
}
