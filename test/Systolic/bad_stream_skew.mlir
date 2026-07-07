// RUN: systolic-opt %s -verify-diagnostics
func.func @bad_stream_skew(%A: tensor<8x8xf32>) -> tensor<8x8xf32> {
  // expected-error @+1 {{skew 不能是负数}}
  %r = systolic.stream %A direction(row) skew(-1) : (tensor<8x8xf32>) -> tensor<8x8xf32>
  return %r : tensor<8x8xf32>
}
