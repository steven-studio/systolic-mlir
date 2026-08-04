// Auto-generated shape-sweep test case: conv_sweep_040
// N=2 H=8 W=8 Cin=3 Kh=3 Kw=5 Cout=8
// stride=(1,1) dilation=(1,1)
// pad=(top=0,bottom=0,left=0,right=0)
func.func @conv_sweep_040(%input: tensor<2x8x8x3xf32>,
                   %filter: tensor<3x5x3x8xf32>) -> tensor<2x6x4x8xf32> {
  %cst = arith.constant 0.0 : f32
  %padded = tensor.pad %input low[0, 0, 0, 0] high[0, 0, 0, 0] {
  ^bb0(%a: index, %b: index, %c: index, %d: index):
    tensor.yield %cst : f32
  } : tensor<2x8x8x3xf32> to tensor<2x8x8x3xf32>
  %init = tensor.empty() : tensor<2x6x4x8xf32>
  %out = linalg.conv_2d_nhwc_hwcf
      {strides = dense<[1, 1]> : tensor<2xi64>,
        dilations = dense<[1, 1]> : tensor<2xi64>}
      ins(%padded, %filter : tensor<2x8x8x3xf32>, tensor<3x5x3x8xf32>)
      outs(%init : tensor<2x6x4x8xf32>) -> tensor<2x6x4x8xf32>
  return %out : tensor<2x6x4x8xf32>
}
