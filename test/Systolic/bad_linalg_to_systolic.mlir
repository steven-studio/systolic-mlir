// RUN: systolic-opt --systolic-binary-search-tile-matmul --split-input-file --verify-diagnostics %s

// Linalg -> Systolic slices A as M x K and B as K x N. A linalg.matmul that
// means something else is refused with a diagnostic instead of being lowered
// as if it were C(m, n) += A(m, k) * B(k, n).

#map_a = affine_map<(m, n, k) -> (m, k)>
#map_bt = affine_map<(m, n, k) -> (n, k)>
#map_c = affine_map<(m, n, k) -> (m, n)>
module {
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  func.func @transposed_b(%A: tensor<4x4xf32>, %B: tensor<4x4xf32>, %C: tensor<4x4xf32>) -> tensor<4x4xf32> {
    // expected-error @+1 {{linalg.matmul with user-defined indexing_maps is not supported}}
    %0 = linalg.matmul indexing_maps = [#map_a, #map_bt, #map_c]
           ins(%A, %B : tensor<4x4xf32>, tensor<4x4xf32>) outs(%C : tensor<4x4xf32>) -> tensor<4x4xf32>
    return %0 : tensor<4x4xf32>
  }
}

// -----

#map_at = affine_map<(m, n, k) -> (k, m)>
#map_b = affine_map<(m, n, k) -> (k, n)>
#map_c = affine_map<(m, n, k) -> (m, n)>
module {
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  func.func @transposed_a(%A: tensor<8x4xf32>, %B: tensor<8x4xf32>, %C: tensor<4x4xf32>) -> tensor<4x4xf32> {
    // expected-error @+1 {{linalg.matmul with user-defined indexing_maps is not supported}}
    %0 = linalg.matmul indexing_maps = [#map_at, #map_b, #map_c]
           ins(%A, %B : tensor<8x4xf32>, tensor<8x4xf32>) outs(%C : tensor<4x4xf32>) -> tensor<4x4xf32>
    return %0 : tensor<4x4xf32>
  }
}

// -----

module {
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  func.func @empty_k(%A: tensor<4x0xf32>, %B: tensor<0x4xf32>, %C: tensor<4x4xf32>) -> tensor<4x4xf32> {
    // expected-error @+1 {{requires non-empty M, N and K, got 4x4x0}}
    %0 = linalg.matmul ins(%A, %B : tensor<4x0xf32>, tensor<0x4xf32>) outs(%C : tensor<4x4xf32>) -> tensor<4x4xf32>
    return %0 : tensor<4x4xf32>
  }
}

// -----

// Without k_max the reduction used to be folded in steps of K itself, a
// division by zero for K = 0.
module {
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary tile_overhead = 95
  func.func @empty_k_no_k_max(%A: tensor<4x0xf32>, %B: tensor<0x4xf32>, %C: tensor<4x4xf32>) -> tensor<4x4xf32> {
    // expected-error @+1 {{requires non-empty M, N and K, got 4x4x0}}
    %0 = linalg.matmul ins(%A, %B : tensor<4x0xf32>, tensor<0x4xf32>) outs(%C : tensor<4x4xf32>) -> tensor<4x4xf32>
    return %0 : tensor<4x4xf32>
  }
}

// -----

module {
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  func.func @empty_m(%A: tensor<0x4xf32>, %B: tensor<4x4xf32>, %C: tensor<0x4xf32>) -> tensor<0x4xf32> {
    // expected-error @+1 {{requires non-empty M, N and K, got 0x4x4}}
    %0 = linalg.matmul ins(%A, %B : tensor<0x4xf32>, tensor<4x4xf32>) outs(%C : tensor<0x4xf32>) -> tensor<0x4xf32>
    return %0 : tensor<0x4xf32>
  }
}

// -----

// Spelling out the default maps is still the default matmul: accepted, no
// diagnostic.
#map_a = affine_map<(m, n, k) -> (m, k)>
#map_b = affine_map<(m, n, k) -> (k, n)>
#map_c = affine_map<(m, n, k) -> (m, n)>
module {
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  func.func @explicit_default_maps(%A: tensor<4x4xf32>, %B: tensor<4x4xf32>, %C: tensor<4x4xf32>) -> tensor<4x4xf32> {
    %0 = linalg.matmul indexing_maps = [#map_a, #map_b, #map_c]
           ins(%A, %B : tensor<4x4xf32>, tensor<4x4xf32>) outs(%C : tensor<4x4xf32>) -> tensor<4x4xf32>
    return %0 : tensor<4x4xf32>
  }
}
