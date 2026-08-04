// Auto-generated shape-sweep test case: conv_sweep_034
// N=1 H=10 W=10 Cin=8 Kh=1 Kw=1 Cout=16
// stride=(1,2) dilation=(1,1)
// pad=(top=1,bottom=1,left=1,right=1)
func.func @conv_sweep_034(%input: tensor<1x10x10x8xf32>,
                   %filter: tensor<1x1x8x16xf32>) -> tensor<1x12x6x16xf32> {
  %cst = arith.constant 0.0 : f32
  %padded = tensor.pad %input low[0, 1, 1, 0] high[0, 1, 1, 0] {
  ^bb0(%a: index, %b: index, %c: index, %d: index):
    tensor.yield %cst : f32
  } : tensor<1x10x10x8xf32> to tensor<1x12x12x8xf32>
  %init = tensor.empty() : tensor<1x12x6x16xf32>
  %out = linalg.conv_2d_nhwc_hwcf
      {strides = dense<[1, 2]> : tensor<2xi64>,
        dilations = dense<[1, 1]> : tensor<2xi64>}
      ins(%padded, %filter : tensor<1x12x12x8xf32>, tensor<1x1x8x16xf32>)
      outs(%init : tensor<1x12x6x16xf32>) -> tensor<1x12x6x16xf32>
  return %out : tensor<1x12x6x16xf32>
}
