module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  func.func @systolic_matmul(%arg0: tensor<16x31xf32>, %arg1: tensor<31x16xf32>, %arg2: tensor<16x16xf32>) -> tensor<16x16xf32> attributes {llvm.emit_c_interface} {
    %extracted_slice = tensor.extract_slice %arg2[0, 0] [8, 8] [1, 1] : tensor<16x16xf32> to tensor<8x8xf32>
    %extracted_slice_0 = tensor.extract_slice %arg0[0, 0] [8, 16] [1, 1] : tensor<16x31xf32> to tensor<8x16xf32>
    %extracted_slice_1 = tensor.extract_slice %arg1[0, 0] [16, 8] [1, 1] : tensor<31x16xf32> to tensor<16x8xf32>
    %0 = systolic.matmul_tile %extracted_slice_0, %extracted_slice_1, %extracted_slice m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 0 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 0 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %extracted_slice_2 = tensor.extract_slice %arg0[0, 16] [8, 15] [1, 1] : tensor<16x31xf32> to tensor<8x15xf32>
    %extracted_slice_3 = tensor.extract_slice %arg1[16, 0] [15, 8] [1, 1] : tensor<31x16xf32> to tensor<15x8xf32>
    %1 = systolic.matmul_tile %extracted_slice_2, %extracted_slice_3, %0 m = 8 n = 8 k = 15 on @acc_8x8_0 est_cycles = 124 start_cycle = 42 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 0 : i64} : (tensor<8x15xf32>, tensor<15x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice = tensor.insert_slice %1 into %arg2[0, 0] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<16x16xf32>
    %extracted_slice_4 = tensor.extract_slice %inserted_slice[0, 8] [8, 8] [1, 1] : tensor<16x16xf32> to tensor<8x8xf32>
    %extracted_slice_5 = tensor.extract_slice %arg0[0, 0] [8, 16] [1, 1] : tensor<16x31xf32> to tensor<8x16xf32>
    %extracted_slice_6 = tensor.extract_slice %arg1[0, 8] [16, 8] [1, 1] : tensor<31x16xf32> to tensor<16x8xf32>
    %2 = systolic.matmul_tile %extracted_slice_5, %extracted_slice_6, %extracted_slice_4 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 128 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 1 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %extracted_slice_7 = tensor.extract_slice %arg0[0, 16] [8, 15] [1, 1] : tensor<16x31xf32> to tensor<8x15xf32>
    %extracted_slice_8 = tensor.extract_slice %arg1[16, 8] [15, 8] [1, 1] : tensor<31x16xf32> to tensor<15x8xf32>
    %3 = systolic.matmul_tile %extracted_slice_7, %extracted_slice_8, %2 m = 8 n = 8 k = 15 on @acc_8x8_0 est_cycles = 124 start_cycle = 197 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 1 : i64} : (tensor<8x15xf32>, tensor<15x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice_9 = tensor.insert_slice %3 into %inserted_slice[0, 8] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<16x16xf32>
    %extracted_slice_10 = tensor.extract_slice %inserted_slice_9[8, 0] [8, 8] [1, 1] : tensor<16x16xf32> to tensor<8x8xf32>
    %extracted_slice_11 = tensor.extract_slice %arg0[8, 0] [8, 16] [1, 1] : tensor<16x31xf32> to tensor<8x16xf32>
    %extracted_slice_12 = tensor.extract_slice %arg1[0, 0] [16, 8] [1, 1] : tensor<31x16xf32> to tensor<16x8xf32>
    %4 = systolic.matmul_tile %extracted_slice_11, %extracted_slice_12, %extracted_slice_10 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 266 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 2 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %extracted_slice_13 = tensor.extract_slice %arg0[8, 16] [8, 15] [1, 1] : tensor<16x31xf32> to tensor<8x15xf32>
    %extracted_slice_14 = tensor.extract_slice %arg1[16, 0] [15, 8] [1, 1] : tensor<31x16xf32> to tensor<15x8xf32>
    %5 = systolic.matmul_tile %extracted_slice_13, %extracted_slice_14, %4 m = 8 n = 8 k = 15 on @acc_8x8_0 est_cycles = 124 start_cycle = 335 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 2 : i64} : (tensor<8x15xf32>, tensor<15x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice_15 = tensor.insert_slice %5 into %inserted_slice_9[8, 0] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<16x16xf32>
    %extracted_slice_16 = tensor.extract_slice %inserted_slice_15[8, 8] [4, 4] [1, 1] : tensor<16x16xf32> to tensor<4x4xf32>
    %extracted_slice_17 = tensor.extract_slice %arg0[8, 0] [4, 16] [1, 1] : tensor<16x31xf32> to tensor<4x16xf32>
    %extracted_slice_18 = tensor.extract_slice %arg1[0, 8] [16, 4] [1, 1] : tensor<31x16xf32> to tensor<16x4xf32>
    %6 = systolic.matmul_tile %extracted_slice_17, %extracted_slice_18, %extracted_slice_16 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 0 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 3 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_19 = tensor.extract_slice %arg0[8, 16] [4, 15] [1, 1] : tensor<16x31xf32> to tensor<4x15xf32>
    %extracted_slice_20 = tensor.extract_slice %arg1[16, 8] [15, 4] [1, 1] : tensor<31x16xf32> to tensor<15x4xf32>
    %7 = systolic.matmul_tile %extracted_slice_19, %extracted_slice_20, %6 m = 4 n = 4 k = 15 on @acc_4x4_0 est_cycles = 116 start_cycle = 42 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 3 : i64} : (tensor<4x15xf32>, tensor<15x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_21 = tensor.insert_slice %7 into %inserted_slice_15[8, 8] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<16x16xf32>
    %extracted_slice_22 = tensor.extract_slice %inserted_slice_21[8, 12] [4, 4] [1, 1] : tensor<16x16xf32> to tensor<4x4xf32>
    %extracted_slice_23 = tensor.extract_slice %arg0[8, 0] [4, 16] [1, 1] : tensor<16x31xf32> to tensor<4x16xf32>
    %extracted_slice_24 = tensor.extract_slice %arg1[0, 12] [16, 4] [1, 1] : tensor<31x16xf32> to tensor<16x4xf32>
    %8 = systolic.matmul_tile %extracted_slice_23, %extracted_slice_24, %extracted_slice_22 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 0 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 4 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_25 = tensor.extract_slice %arg0[8, 16] [4, 15] [1, 1] : tensor<16x31xf32> to tensor<4x15xf32>
    %extracted_slice_26 = tensor.extract_slice %arg1[16, 12] [15, 4] [1, 1] : tensor<31x16xf32> to tensor<15x4xf32>
    %9 = systolic.matmul_tile %extracted_slice_25, %extracted_slice_26, %8 m = 4 n = 4 k = 15 on @acc_4x4_1 est_cycles = 116 start_cycle = 42 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 4 : i64} : (tensor<4x15xf32>, tensor<15x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_27 = tensor.insert_slice %9 into %inserted_slice_21[8, 12] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<16x16xf32>
    %extracted_slice_28 = tensor.extract_slice %inserted_slice_27[12, 8] [4, 4] [1, 1] : tensor<16x16xf32> to tensor<4x4xf32>
    %extracted_slice_29 = tensor.extract_slice %arg0[12, 0] [4, 16] [1, 1] : tensor<16x31xf32> to tensor<4x16xf32>
    %extracted_slice_30 = tensor.extract_slice %arg1[0, 8] [16, 4] [1, 1] : tensor<31x16xf32> to tensor<16x4xf32>
    %10 = systolic.matmul_tile %extracted_slice_29, %extracted_slice_30, %extracted_slice_28 m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 0 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 5 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_31 = tensor.extract_slice %arg0[12, 16] [4, 15] [1, 1] : tensor<16x31xf32> to tensor<4x15xf32>
    %extracted_slice_32 = tensor.extract_slice %arg1[16, 8] [15, 4] [1, 1] : tensor<31x16xf32> to tensor<15x4xf32>
    %11 = systolic.matmul_tile %extracted_slice_31, %extracted_slice_32, %10 m = 4 n = 4 k = 15 on @acc_4x4_2 est_cycles = 116 start_cycle = 42 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 5 : i64} : (tensor<4x15xf32>, tensor<15x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_33 = tensor.insert_slice %11 into %inserted_slice_27[12, 8] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<16x16xf32>
    %extracted_slice_34 = tensor.extract_slice %inserted_slice_33[12, 12] [4, 4] [1, 1] : tensor<16x16xf32> to tensor<4x4xf32>
    %extracted_slice_35 = tensor.extract_slice %arg0[12, 0] [4, 16] [1, 1] : tensor<16x31xf32> to tensor<4x16xf32>
    %extracted_slice_36 = tensor.extract_slice %arg1[0, 12] [16, 4] [1, 1] : tensor<31x16xf32> to tensor<16x4xf32>
    %12 = systolic.matmul_tile %extracted_slice_35, %extracted_slice_36, %extracted_slice_34 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 120 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 6 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_37 = tensor.extract_slice %arg0[12, 16] [4, 15] [1, 1] : tensor<16x31xf32> to tensor<4x15xf32>
    %extracted_slice_38 = tensor.extract_slice %arg1[16, 12] [15, 4] [1, 1] : tensor<31x16xf32> to tensor<15x4xf32>
    %13 = systolic.matmul_tile %extracted_slice_37, %extracted_slice_38, %12 m = 4 n = 4 k = 15 on @acc_4x4_0 est_cycles = 116 start_cycle = 189 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 6 : i64} : (tensor<4x15xf32>, tensor<15x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_39 = tensor.insert_slice %13 into %inserted_slice_33[12, 12] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<16x16xf32>
    return %inserted_slice_39 : tensor<16x16xf32>
  }
}

