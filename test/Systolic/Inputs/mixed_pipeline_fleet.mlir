// Two 4x4s of one fleet, one pipelined and one not: the binary-search
// decomposition bills one cost per geometry and must refuse.
// expected-error @+1 {{systolic.device @acc_4x4_1 and @acc_4x4_0 of the same 4x4 geometry declare different cost parameters}}
module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  func.func @mm(%A: tensor<16x16xf32>, %B: tensor<16x16xf32>, %C: tensor<16x16xf32>) -> tensor<16x16xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<16x16xf32>, tensor<16x16xf32>) outs(%C : tensor<16x16xf32>) -> tensor<16x16xf32>
    return %0 : tensor<16x16xf32>
  }
}
