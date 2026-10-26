module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  func.func @systolic_matmul(%arg0: tensor<12x35xf32>, %arg1: tensor<35x12xf32>, %arg2: tensor<12x12xf32>) -> tensor<12x12xf32> attributes {llvm.emit_c_interface} {
    %extracted_slice = tensor.extract_slice %arg2[0, 0] [8, 8] [1, 1] : tensor<12x12xf32> to tensor<8x8xf32>
    %extracted_slice_0 = tensor.extract_slice %arg0[0, 0] [8, 16] [1, 1] : tensor<12x35xf32> to tensor<8x16xf32>
    %extracted_slice_1 = tensor.extract_slice %arg1[0, 0] [16, 8] [1, 1] : tensor<35x12xf32> to tensor<16x8xf32>
    %0 = systolic.matmul_tile %extracted_slice_0, %extracted_slice_1, %extracted_slice m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 0 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 0 : i64, systolic.tile = 0 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %extracted_slice_2 = tensor.extract_slice %arg0[0, 16] [8, 16] [1, 1] : tensor<12x35xf32> to tensor<8x16xf32>
    %extracted_slice_3 = tensor.extract_slice %arg1[16, 0] [16, 8] [1, 1] : tensor<35x12xf32> to tensor<16x8xf32>
    %1 = systolic.matmul_tile %extracted_slice_2, %extracted_slice_3, %0 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 42 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 1 : i64, systolic.tile = 0 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %extracted_slice_4 = tensor.extract_slice %arg0[0, 32] [8, 3] [1, 1] : tensor<12x35xf32> to tensor<8x3xf32>
    %extracted_slice_5 = tensor.extract_slice %arg1[32, 0] [3, 8] [1, 1] : tensor<35x12xf32> to tensor<3x8xf32>
    %2 = systolic.matmul_tile %extracted_slice_4, %extracted_slice_5, %1 m = 8 n = 8 k = 3 on @acc_8x8_0 est_cycles = 112 start_cycle = 128 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 2 : i64, systolic.tile = 0 : i64} : (tensor<8x3xf32>, tensor<3x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice = tensor.insert_slice %2 into %arg2[0, 0] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<12x12xf32>
    %extracted_slice_6 = tensor.extract_slice %inserted_slice[0, 8] [4, 4] [1, 1] : tensor<12x12xf32> to tensor<4x4xf32>
    %extracted_slice_7 = tensor.extract_slice %arg0[0, 0] [4, 16] [1, 1] : tensor<12x35xf32> to tensor<4x16xf32>
    %extracted_slice_8 = tensor.extract_slice %arg1[0, 8] [16, 4] [1, 1] : tensor<35x12xf32> to tensor<16x4xf32>
    %3 = systolic.matmul_tile %extracted_slice_7, %extracted_slice_8, %extracted_slice_6 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 0 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 0 : i64, systolic.tile = 1 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_9 = tensor.extract_slice %arg0[0, 16] [4, 16] [1, 1] : tensor<12x35xf32> to tensor<4x16xf32>
    %extracted_slice_10 = tensor.extract_slice %arg1[16, 8] [16, 4] [1, 1] : tensor<35x12xf32> to tensor<16x4xf32>
    %4 = systolic.matmul_tile %extracted_slice_9, %extracted_slice_10, %3 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 42 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 1 : i64, systolic.tile = 1 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_11 = tensor.extract_slice %arg0[0, 32] [4, 3] [1, 1] : tensor<12x35xf32> to tensor<4x3xf32>
    %extracted_slice_12 = tensor.extract_slice %arg1[32, 8] [3, 4] [1, 1] : tensor<35x12xf32> to tensor<3x4xf32>
    %5 = systolic.matmul_tile %extracted_slice_11, %extracted_slice_12, %4 m = 4 n = 4 k = 3 on @acc_4x4_0 est_cycles = 104 start_cycle = 120 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 2 : i64, systolic.tile = 1 : i64} : (tensor<4x3xf32>, tensor<3x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_13 = tensor.insert_slice %5 into %inserted_slice[0, 8] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<12x12xf32>
    %extracted_slice_14 = tensor.extract_slice %inserted_slice_13[4, 8] [4, 4] [1, 1] : tensor<12x12xf32> to tensor<4x4xf32>
    %extracted_slice_15 = tensor.extract_slice %arg0[4, 0] [4, 16] [1, 1] : tensor<12x35xf32> to tensor<4x16xf32>
    %extracted_slice_16 = tensor.extract_slice %arg1[0, 8] [16, 4] [1, 1] : tensor<35x12xf32> to tensor<16x4xf32>
    %6 = systolic.matmul_tile %extracted_slice_15, %extracted_slice_16, %extracted_slice_14 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 0 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 0 : i64, systolic.tile = 2 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_17 = tensor.extract_slice %arg0[4, 16] [4, 16] [1, 1] : tensor<12x35xf32> to tensor<4x16xf32>
    %extracted_slice_18 = tensor.extract_slice %arg1[16, 8] [16, 4] [1, 1] : tensor<35x12xf32> to tensor<16x4xf32>
    %7 = systolic.matmul_tile %extracted_slice_17, %extracted_slice_18, %6 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 42 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 1 : i64, systolic.tile = 2 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_19 = tensor.extract_slice %arg0[4, 32] [4, 3] [1, 1] : tensor<12x35xf32> to tensor<4x3xf32>
    %extracted_slice_20 = tensor.extract_slice %arg1[32, 8] [3, 4] [1, 1] : tensor<35x12xf32> to tensor<3x4xf32>
    %8 = systolic.matmul_tile %extracted_slice_19, %extracted_slice_20, %7 m = 4 n = 4 k = 3 on @acc_4x4_1 est_cycles = 104 start_cycle = 120 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 2 : i64, systolic.tile = 2 : i64} : (tensor<4x3xf32>, tensor<3x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_21 = tensor.insert_slice %8 into %inserted_slice_13[4, 8] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<12x12xf32>
    %extracted_slice_22 = tensor.extract_slice %inserted_slice_21[8, 0] [4, 4] [1, 1] : tensor<12x12xf32> to tensor<4x4xf32>
    %extracted_slice_23 = tensor.extract_slice %arg0[8, 0] [4, 16] [1, 1] : tensor<12x35xf32> to tensor<4x16xf32>
    %extracted_slice_24 = tensor.extract_slice %arg1[0, 0] [16, 4] [1, 1] : tensor<35x12xf32> to tensor<16x4xf32>
    %9 = systolic.matmul_tile %extracted_slice_23, %extracted_slice_24, %extracted_slice_22 m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 0 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 0 : i64, systolic.tile = 3 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_25 = tensor.extract_slice %arg0[8, 16] [4, 16] [1, 1] : tensor<12x35xf32> to tensor<4x16xf32>
    %extracted_slice_26 = tensor.extract_slice %arg1[16, 0] [16, 4] [1, 1] : tensor<35x12xf32> to tensor<16x4xf32>
    %10 = systolic.matmul_tile %extracted_slice_25, %extracted_slice_26, %9 m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 42 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 1 : i64, systolic.tile = 3 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_27 = tensor.extract_slice %arg0[8, 32] [4, 3] [1, 1] : tensor<12x35xf32> to tensor<4x3xf32>
    %extracted_slice_28 = tensor.extract_slice %arg1[32, 0] [3, 4] [1, 1] : tensor<35x12xf32> to tensor<3x4xf32>
    %11 = systolic.matmul_tile %extracted_slice_27, %extracted_slice_28, %10 m = 4 n = 4 k = 3 on @acc_4x4_2 est_cycles = 104 start_cycle = 120 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 2 : i64, systolic.tile = 3 : i64} : (tensor<4x3xf32>, tensor<3x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_29 = tensor.insert_slice %11 into %inserted_slice_21[8, 0] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<12x12xf32>
    %extracted_slice_30 = tensor.extract_slice %inserted_slice_29[8, 4] [4, 4] [1, 1] : tensor<12x12xf32> to tensor<4x4xf32>
    %extracted_slice_31 = tensor.extract_slice %arg0[8, 0] [4, 16] [1, 1] : tensor<12x35xf32> to tensor<4x16xf32>
    %extracted_slice_32 = tensor.extract_slice %arg1[0, 4] [16, 4] [1, 1] : tensor<35x12xf32> to tensor<16x4xf32>
    %12 = systolic.matmul_tile %extracted_slice_31, %extracted_slice_32, %extracted_slice_30 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 189 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 0 : i64, systolic.tile = 4 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_33 = tensor.extract_slice %arg0[8, 16] [4, 16] [1, 1] : tensor<12x35xf32> to tensor<4x16xf32>
    %extracted_slice_34 = tensor.extract_slice %arg1[16, 4] [16, 4] [1, 1] : tensor<35x12xf32> to tensor<16x4xf32>
    %13 = systolic.matmul_tile %extracted_slice_33, %extracted_slice_34, %12 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 258 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 1 : i64, systolic.tile = 4 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_35 = tensor.extract_slice %arg0[8, 32] [4, 3] [1, 1] : tensor<12x35xf32> to tensor<4x3xf32>
    %extracted_slice_36 = tensor.extract_slice %arg1[32, 4] [3, 4] [1, 1] : tensor<35x12xf32> to tensor<3x4xf32>
    %14 = systolic.matmul_tile %extracted_slice_35, %extracted_slice_36, %13 m = 4 n = 4 k = 3 on @acc_4x4_0 est_cycles = 104 start_cycle = 327 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 2 : i64, systolic.tile = 4 : i64} : (tensor<4x3xf32>, tensor<3x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_37 = tensor.insert_slice %14 into %inserted_slice_29[8, 4] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<12x12xf32>
    %extracted_slice_38 = tensor.extract_slice %inserted_slice_37[8, 8] [4, 4] [1, 1] : tensor<12x12xf32> to tensor<4x4xf32>
    %extracted_slice_39 = tensor.extract_slice %arg0[8, 0] [4, 16] [1, 1] : tensor<12x35xf32> to tensor<4x16xf32>
    %extracted_slice_40 = tensor.extract_slice %arg1[0, 8] [16, 4] [1, 1] : tensor<35x12xf32> to tensor<16x4xf32>
    %15 = systolic.matmul_tile %extracted_slice_39, %extracted_slice_40, %extracted_slice_38 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 189 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 0 : i64, systolic.tile = 5 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_41 = tensor.extract_slice %arg0[8, 16] [4, 16] [1, 1] : tensor<12x35xf32> to tensor<4x16xf32>
    %extracted_slice_42 = tensor.extract_slice %arg1[16, 8] [16, 4] [1, 1] : tensor<35x12xf32> to tensor<16x4xf32>
    %16 = systolic.matmul_tile %extracted_slice_41, %extracted_slice_42, %15 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 258 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 1 : i64, systolic.tile = 5 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_43 = tensor.extract_slice %arg0[8, 32] [4, 3] [1, 1] : tensor<12x35xf32> to tensor<4x3xf32>
    %extracted_slice_44 = tensor.extract_slice %arg1[32, 8] [3, 4] [1, 1] : tensor<35x12xf32> to tensor<3x4xf32>
    %17 = systolic.matmul_tile %extracted_slice_43, %extracted_slice_44, %16 m = 4 n = 4 k = 3 on @acc_4x4_1 est_cycles = 104 start_cycle = 327 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 2 : i64, systolic.tile = 5 : i64} : (tensor<4x3xf32>, tensor<3x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_45 = tensor.insert_slice %17 into %inserted_slice_37[8, 8] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<12x12xf32>
    return %inserted_slice_45 : tensor<12x12xf32>
  }
}

