module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  func.func @systolic_matmul(%arg0: tensor<8x16xf32>, %arg1: tensor<16x4xf32>, %arg2: tensor<8x4xf32>) -> tensor<8x4xf32> attributes {llvm.emit_c_interface} {
    %extracted_slice = tensor.extract_slice %arg2[0, 0] [4, 4] [1, 1] : tensor<8x4xf32> to tensor<4x4xf32>
    %extracted_slice_0 = tensor.extract_slice %arg0[0, 0] [4, 16] [1, 1] : tensor<8x16xf32> to tensor<4x16xf32>
    %extracted_slice_1 = tensor.extract_slice %arg1[0, 0] [16, 4] [1, 1] : tensor<16x4xf32> to tensor<16x4xf32>
    %0 = systolic.matmul_tile %extracted_slice_0, %extracted_slice_1, %extracted_slice m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 0 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 0 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice = tensor.insert_slice %0 into %arg2[0, 0] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<8x4xf32>
    %extracted_slice_2 = tensor.extract_slice %inserted_slice[4, 0] [4, 4] [1, 1] : tensor<8x4xf32> to tensor<4x4xf32>
    %extracted_slice_3 = tensor.extract_slice %arg0[4, 0] [4, 16] [1, 1] : tensor<8x16xf32> to tensor<4x16xf32>
    %extracted_slice_4 = tensor.extract_slice %arg1[0, 0] [16, 4] [1, 1] : tensor<16x4xf32> to tensor<16x4xf32>
    %1 = systolic.matmul_tile %extracted_slice_3, %extracted_slice_4, %extracted_slice_2 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 0 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 1 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_5 = tensor.insert_slice %1 into %inserted_slice[4, 0] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<8x4xf32>
    return %inserted_slice_5 : tensor<8x4xf32>
  }
}

