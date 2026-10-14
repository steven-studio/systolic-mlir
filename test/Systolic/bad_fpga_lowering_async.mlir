// RUN: systolic-opt --lower-systolic-matmul-tile-to-fpga="async=true" --split-input-file --verify-diagnostics %s

// The asynchronous lowering groups tiles into GEMMs by the attributes the
// binary-search pass sets; a tile without them, or a GEMM whose tiles are
// spread over several blocks, is refused and the module left unlowered.

module {
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  func.func @no_ids(%a: tensor<4x4xf32>, %b: tensor<4x4xf32>, %c: tensor<4x4xf32>) -> tensor<4x4xf32> {
    // expected-error @+1 {{the asynchronous FPGA lowering needs the systolic.gemm, systolic.tile, systolic.k_index and systolic.k_count attributes}}
    %0 = systolic.matmul_tile %a, %b, %c m = 4 n = 4 k = 4 on @acc_4x4_0 est_cycles = 101 start_cycle = 0
         : (tensor<4x4xf32>, tensor<4x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    return %0 : tensor<4x4xf32>
  }
}

// -----

module {
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  func.func @bad_fold(%a: tensor<4x4xf32>, %b: tensor<4x4xf32>, %c: tensor<4x4xf32>) -> tensor<4x4xf32> {
    // expected-error @+1 {{systolic.gemm/tile/k_index/k_count must be non-negative with 0 <= k_index < k_count}}
    %0 = systolic.matmul_tile %a, %b, %c m = 4 n = 4 k = 4 on @acc_4x4_0 est_cycles = 101 start_cycle = 0
         {systolic.gemm = 0 : i64, systolic.tile = 0 : i64, systolic.k_index = 2 : i64, systolic.k_count = 2 : i64}
         : (tensor<4x4xf32>, tensor<4x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    return %0 : tensor<4x4xf32>
  }
}

// -----

module {
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  func.func @two_blocks(%a: tensor<4x4xf32>, %b: tensor<4x4xf32>, %c: tensor<4x4xf32>, %p: i1) -> tensor<4x4xf32> {
    %0 = systolic.matmul_tile %a, %b, %c m = 4 n = 4 k = 4 on @acc_4x4_0 est_cycles = 101 start_cycle = 0
         {systolic.gemm = 0 : i64, systolic.tile = 0 : i64, systolic.k_index = 0 : i64, systolic.k_count = 2 : i64}
         : (tensor<4x4xf32>, tensor<4x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %1 = scf.if %p -> tensor<4x4xf32> {
      // expected-error @+1 {{the asynchronous FPGA lowering needs every matmul_tile of a GEMM (systolic.gemm = 0) in one block}}
      %2 = systolic.matmul_tile %a, %b, %0 m = 4 n = 4 k = 4 on @acc_4x4_0 est_cycles = 101 start_cycle = 101
           {systolic.gemm = 0 : i64, systolic.tile = 0 : i64, systolic.k_index = 1 : i64, systolic.k_count = 2 : i64}
           : (tensor<4x4xf32>, tensor<4x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
      scf.yield %2 : tensor<4x4xf32>
    } else {
      scf.yield %0 : tensor<4x4xf32>
    }
    return %1 : tensor<4x4xf32>
  }
}
