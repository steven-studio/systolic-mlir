// RUN: systolic-opt --systolic-binary-search-tile-matmul %s -split-input-file -verify-diagnostics

// K_MAX is per geometry in the hardware (K_MAX_8X8, K_MAX_4X4), and the
// binary-search decomposition bills one cost per geometry: accelerators of
// one geometry must declare the same k_max. Different geometries may differ
// freely (kmax_heterogeneous.mlir).

// expected-error @+1 {{systolic.device @acc_4x4_1 declares k_max = 8 but @acc_4x4_0 of the same 4x4 geometry declares k_max = 7}}
module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 7 tile_overhead = 95
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 8 tile_overhead = 95
  func.func @mm(%A: tensor<8x8xf32>, %B: tensor<8x8xf32>, %C: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<8x8xf32>, tensor<8x8xf32>) outs(%C : tensor<8x8xf32>) -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }
}

// -----

// Declaring k_max on one 4x4 and not on another is the same disagreement.
// expected-error @+1 {{systolic.device @acc_4x4_1 declares k_max = none but @acc_4x4_0 of the same 4x4 geometry declares k_max = 12}}
module {
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 12 tile_overhead = 95
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary tile_overhead = 95
  func.func @mm(%A: tensor<8x8xf32>, %B: tensor<8x8xf32>, %C: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<8x8xf32>, tensor<8x8xf32>) outs(%C : tensor<8x8xf32>) -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }
}
