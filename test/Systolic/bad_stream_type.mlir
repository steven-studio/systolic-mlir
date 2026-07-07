// RUN: systolic-opt %s -verify-diagnostics
func.func @bad_stream_type(%A: tensor<8x8xf32>) -> tensor<4x4xf32> {
  // expected-error @+1 {{data 跟 result 的型别必须一致}}
  %r = systolic.stream %A direction(row) skew(1) : (tensor<8x8xf32>) -> tensor<4x4xf32>
  return %r : tensor<4x4xf32>
}
