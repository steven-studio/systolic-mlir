// Auto-generated shape-sweep test case: conv_sweep_043
// N=1 H=10 W=10 Cin=3 Kh=5 Kw=5 Cout=8
// stride=(2,2) dilation=(2,1)
// pad=(top=0,bottom=0,left=0,right=0)
func.func @conv_sweep_043(%input: tensor<1x10x10x3xf32>,
                   %filter: tensor<5x5x3x8xf32>) -> tensor<1x1x3x8xf32> {
  %cst = arith.constant 0.0 : f32
  %padded = tensor.pad %input low[0, 0, 0, 0] high[0, 0, 0, 0] {
  ^bb0(%a: index, %b: index, %c: index, %d: index):
    tensor.yield %cst : f32
  } : tensor<1x10x10x3xf32> to tensor<1x10x10x3xf32>
  %init = tensor.empty() : tensor<1x1x3x8xf32>
  %out = linalg.conv_2d_nhwc_hwcf
      {strides = dense<[2, 2]> : tensor<2xi64>,
        dilations = dense<[2, 1]> : tensor<2xi64>}
      ins(%padded, %filter : tensor<1x10x10x3xf32>, tensor<5x5x3x8xf32>)
      outs(%init : tensor<1x1x3x8xf32>) -> tensor<1x1x3x8xf32>
  return %out : tensor<1x1x3x8xf32>
}
