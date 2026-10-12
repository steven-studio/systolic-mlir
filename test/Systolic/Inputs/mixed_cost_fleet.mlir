// Input of bad_device_cost_attrs.mlir (no RUN line of its own): two 4x4s
// that agree on k_max but not on the read term. The binary-search
// decomposition bills one cost per geometry, so it refuses the fleet.

// expected-error @+1 {{systolic.device @acc_4x4_1 and @acc_4x4_0 of the same 4x4 geometry declare different cost parameters}}
module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.0
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 8.0
  func.func @mm(%A: tensor<8x8xf32>, %B: tensor<8x8xf32>, %C: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<8x8xf32>, tensor<8x8xf32>) outs(%C : tensor<8x8xf32>) -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }
}
