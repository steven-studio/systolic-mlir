module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  func.func @systolic_matmul(%arg0: tensor<16x40xf32>, %arg1: tensor<40x8xf32>, %arg2: tensor<16x8xf32>) -> tensor<16x8xf32> attributes {llvm.emit_c_interface} {
    %extracted_slice = tensor.extract_slice %arg2[0, 0] [8, 8] [1, 1] : tensor<16x8xf32> to tensor<8x8xf32>
    %extracted_slice_0 = tensor.extract_slice %arg0[0, 0] [8, 16] [1, 1] : tensor<16x40xf32> to tensor<8x16xf32>
    %extracted_slice_1 = tensor.extract_slice %arg1[0, 0] [16, 8] [1, 1] : tensor<40x8xf32> to tensor<16x8xf32>
    %0 = systolic.matmul_tile %extracted_slice_0, %extracted_slice_1, %extracted_slice m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 0 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 0 : i64, systolic.tile = 0 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %extracted_slice_2 = tensor.extract_slice %arg0[0, 16] [8, 16] [1, 1] : tensor<16x40xf32> to tensor<8x16xf32>
    %extracted_slice_3 = tensor.extract_slice %arg1[16, 0] [16, 8] [1, 1] : tensor<40x8xf32> to tensor<16x8xf32>
    %1 = systolic.matmul_tile %extracted_slice_2, %extracted_slice_3, %0 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 42 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 1 : i64, systolic.tile = 0 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %extracted_slice_4 = tensor.extract_slice %arg0[0, 32] [8, 8] [1, 1] : tensor<16x40xf32> to tensor<8x8xf32>
    %extracted_slice_5 = tensor.extract_slice %arg1[32, 0] [8, 8] [1, 1] : tensor<40x8xf32> to tensor<8x8xf32>
    %2 = systolic.matmul_tile %extracted_slice_4, %extracted_slice_5, %1 m = 8 n = 8 k = 8 on @acc_8x8_0 est_cycles = 117 start_cycle = 128 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 2 : i64, systolic.tile = 0 : i64} : (tensor<8x8xf32>, tensor<8x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice = tensor.insert_slice %2 into %arg2[0, 0] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<16x8xf32>
    %extracted_slice_6 = tensor.extract_slice %inserted_slice[8, 0] [8, 8] [1, 1] : tensor<16x8xf32> to tensor<8x8xf32>
    %extracted_slice_7 = tensor.extract_slice %arg0[8, 0] [8, 16] [1, 1] : tensor<16x40xf32> to tensor<8x16xf32>
    %extracted_slice_8 = tensor.extract_slice %arg1[0, 0] [16, 8] [1, 1] : tensor<40x8xf32> to tensor<16x8xf32>
    %3 = systolic.matmul_tile %extracted_slice_7, %extracted_slice_8, %extracted_slice_6 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 197 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 0 : i64, systolic.tile = 1 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %extracted_slice_9 = tensor.extract_slice %arg0[8, 16] [8, 16] [1, 1] : tensor<16x40xf32> to tensor<8x16xf32>
    %extracted_slice_10 = tensor.extract_slice %arg1[16, 0] [16, 8] [1, 1] : tensor<40x8xf32> to tensor<16x8xf32>
    %4 = systolic.matmul_tile %extracted_slice_9, %extracted_slice_10, %3 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 266 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 1 : i64, systolic.tile = 1 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %extracted_slice_11 = tensor.extract_slice %arg0[8, 32] [8, 8] [1, 1] : tensor<16x40xf32> to tensor<8x8xf32>
    %extracted_slice_12 = tensor.extract_slice %arg1[32, 0] [8, 8] [1, 1] : tensor<40x8xf32> to tensor<8x8xf32>
    %5 = systolic.matmul_tile %extracted_slice_11, %extracted_slice_12, %4 m = 8 n = 8 k = 8 on @acc_8x8_0 est_cycles = 117 start_cycle = 335 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 2 : i64, systolic.tile = 1 : i64} : (tensor<8x8xf32>, tensor<8x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice_13 = tensor.insert_slice %5 into %inserted_slice[8, 0] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<16x8xf32>
    return %inserted_slice_13 : tensor<16x8xf32>
  }
}

