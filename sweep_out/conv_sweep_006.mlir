// Auto-generated shape-sweep test case: conv_sweep_006
// N=1 H=10 W=10 Cin=3 Kh=3 Kw=3 Cout=8
// stride=(1,1) dilation=(2,1)
// pad=(top=1,bottom=1,left=1,right=1)
func.func @conv_sweep_006(%input: tensor<1x10x10x3xf32>,
                   %filter: tensor<3x3x3x8xf32>) -> tensor<1x8x10x8xf32> {
  %cst = arith.constant 0.0 : f32
  %padded = tensor.pad %input low[0, 1, 1, 0] high[0, 1, 1, 0] {
  ^bb0(%a: index, %b: index, %c: index, %d: index):
    tensor.yield %cst : f32
  } : tensor<1x10x10x3xf32> to tensor<1x12x12x3xf32>
  %init = tensor.empty() : tensor<1x8x10x8xf32>
  %out = linalg.conv_2d_nhwc_hwcf
      {strides = dense<[1, 1]> : tensor<2xi64>,
        dilations = dense<[2, 1]> : tensor<2xi64>}
      ins(%padded, %filter : tensor<1x12x12x3xf32>, tensor<3x3x3x8xf32>)
      outs(%init : tensor<1x8x10x8xf32>) -> tensor<1x8x10x8xf32>
  return %out : tensor<1x8x10x8xf32>
}
