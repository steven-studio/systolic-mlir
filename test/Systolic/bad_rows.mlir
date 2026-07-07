// RUN: systolic-opt %s -verify-diagnostics

// rows 是 0,应该触发 PEArrayOp::verify() 里第一条检查:
// "pe_array 的 rows/cols 必须是正数"
func.func @bad_rows(%B: tensor<4x4xf32>, %A: tensor<8x4xf32>, %C: tensor<8x4xf32>)
    -> tensor<8x4xf32> {
  // expected-error @+1 {{pe_array 的 rows/cols 必须是正数}}
  %r = systolic.pe_array<0 x 4> stationary(weight) %B, %A, %C
       : (tensor<4x4xf32>, tensor<8x4xf32>, tensor<8x4xf32>) -> tensor<8x4xf32>
  return %r : tensor<8x4xf32>
}
