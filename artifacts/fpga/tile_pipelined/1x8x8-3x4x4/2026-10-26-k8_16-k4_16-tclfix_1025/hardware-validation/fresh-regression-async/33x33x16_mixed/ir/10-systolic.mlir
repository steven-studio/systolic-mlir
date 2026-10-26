module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  func.func @systolic_matmul(%arg0: tensor<33x16xf32>, %arg1: tensor<16x33xf32>, %arg2: tensor<33x33xf32>) -> tensor<33x33xf32> attributes {llvm.emit_c_interface} {
    %extracted_slice = tensor.extract_slice %arg2[0, 0] [8, 8] [1, 1] : tensor<33x33xf32> to tensor<8x8xf32>
    %extracted_slice_0 = tensor.extract_slice %arg0[0, 0] [8, 16] [1, 1] : tensor<33x16xf32> to tensor<8x16xf32>
    %extracted_slice_1 = tensor.extract_slice %arg1[0, 0] [16, 8] [1, 1] : tensor<16x33xf32> to tensor<16x8xf32>
    %0 = systolic.matmul_tile %extracted_slice_0, %extracted_slice_1, %extracted_slice m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 0 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 0 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice = tensor.insert_slice %0 into %arg2[0, 0] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<33x33xf32>
    %extracted_slice_2 = tensor.extract_slice %inserted_slice[0, 8] [8, 8] [1, 1] : tensor<33x33xf32> to tensor<8x8xf32>
    %extracted_slice_3 = tensor.extract_slice %arg0[0, 0] [8, 16] [1, 1] : tensor<33x16xf32> to tensor<8x16xf32>
    %extracted_slice_4 = tensor.extract_slice %arg1[0, 8] [16, 8] [1, 1] : tensor<16x33xf32> to tensor<16x8xf32>
    %1 = systolic.matmul_tile %extracted_slice_3, %extracted_slice_4, %extracted_slice_2 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 42 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 1 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice_5 = tensor.insert_slice %1 into %inserted_slice[0, 8] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<33x33xf32>
    %extracted_slice_6 = tensor.extract_slice %inserted_slice_5[0, 16] [8, 8] [1, 1] : tensor<33x33xf32> to tensor<8x8xf32>
    %extracted_slice_7 = tensor.extract_slice %arg0[0, 0] [8, 16] [1, 1] : tensor<33x16xf32> to tensor<8x16xf32>
    %extracted_slice_8 = tensor.extract_slice %arg1[0, 16] [16, 8] [1, 1] : tensor<16x33xf32> to tensor<16x8xf32>
    %2 = systolic.matmul_tile %extracted_slice_7, %extracted_slice_8, %extracted_slice_6 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 128 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 2 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice_9 = tensor.insert_slice %2 into %inserted_slice_5[0, 16] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<33x33xf32>
    %extracted_slice_10 = tensor.extract_slice %inserted_slice_9[0, 24] [8, 8] [1, 1] : tensor<33x33xf32> to tensor<8x8xf32>
    %extracted_slice_11 = tensor.extract_slice %arg0[0, 0] [8, 16] [1, 1] : tensor<33x16xf32> to tensor<8x16xf32>
    %extracted_slice_12 = tensor.extract_slice %arg1[0, 24] [16, 8] [1, 1] : tensor<16x33xf32> to tensor<16x8xf32>
    %3 = systolic.matmul_tile %extracted_slice_11, %extracted_slice_12, %extracted_slice_10 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 197 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 3 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice_13 = tensor.insert_slice %3 into %inserted_slice_9[0, 24] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<33x33xf32>
    %extracted_slice_14 = tensor.extract_slice %inserted_slice_13[0, 32] [4, 1] [1, 1] : tensor<33x33xf32> to tensor<4x1xf32>
    %extracted_slice_15 = tensor.extract_slice %arg0[0, 0] [4, 16] [1, 1] : tensor<33x16xf32> to tensor<4x16xf32>
    %extracted_slice_16 = tensor.extract_slice %arg1[0, 32] [16, 1] [1, 1] : tensor<16x33xf32> to tensor<16x1xf32>
    %4 = systolic.matmul_tile %extracted_slice_15, %extracted_slice_16, %extracted_slice_14 m = 4 n = 1 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 0 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 4 : i64} : (tensor<4x16xf32>, tensor<16x1xf32>, tensor<4x1xf32>) -> tensor<4x1xf32>
    %inserted_slice_17 = tensor.insert_slice %4 into %inserted_slice_13[0, 32] [4, 1] [1, 1] : tensor<4x1xf32> into tensor<33x33xf32>
    %extracted_slice_18 = tensor.extract_slice %inserted_slice_17[4, 32] [4, 1] [1, 1] : tensor<33x33xf32> to tensor<4x1xf32>
    %extracted_slice_19 = tensor.extract_slice %arg0[4, 0] [4, 16] [1, 1] : tensor<33x16xf32> to tensor<4x16xf32>
    %extracted_slice_20 = tensor.extract_slice %arg1[0, 32] [16, 1] [1, 1] : tensor<16x33xf32> to tensor<16x1xf32>
    %5 = systolic.matmul_tile %extracted_slice_19, %extracted_slice_20, %extracted_slice_18 m = 4 n = 1 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 0 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 5 : i64} : (tensor<4x16xf32>, tensor<16x1xf32>, tensor<4x1xf32>) -> tensor<4x1xf32>
    %inserted_slice_21 = tensor.insert_slice %5 into %inserted_slice_17[4, 32] [4, 1] [1, 1] : tensor<4x1xf32> into tensor<33x33xf32>
    %extracted_slice_22 = tensor.extract_slice %inserted_slice_21[8, 0] [8, 8] [1, 1] : tensor<33x33xf32> to tensor<8x8xf32>
    %extracted_slice_23 = tensor.extract_slice %arg0[8, 0] [8, 16] [1, 1] : tensor<33x16xf32> to tensor<8x16xf32>
    %extracted_slice_24 = tensor.extract_slice %arg1[0, 0] [16, 8] [1, 1] : tensor<16x33xf32> to tensor<16x8xf32>
    %6 = systolic.matmul_tile %extracted_slice_23, %extracted_slice_24, %extracted_slice_22 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 266 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 6 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice_25 = tensor.insert_slice %6 into %inserted_slice_21[8, 0] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<33x33xf32>
    %extracted_slice_26 = tensor.extract_slice %inserted_slice_25[8, 8] [8, 8] [1, 1] : tensor<33x33xf32> to tensor<8x8xf32>
    %extracted_slice_27 = tensor.extract_slice %arg0[8, 0] [8, 16] [1, 1] : tensor<33x16xf32> to tensor<8x16xf32>
    %extracted_slice_28 = tensor.extract_slice %arg1[0, 8] [16, 8] [1, 1] : tensor<16x33xf32> to tensor<16x8xf32>
    %7 = systolic.matmul_tile %extracted_slice_27, %extracted_slice_28, %extracted_slice_26 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 335 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 7 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice_29 = tensor.insert_slice %7 into %inserted_slice_25[8, 8] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<33x33xf32>
    %extracted_slice_30 = tensor.extract_slice %inserted_slice_29[8, 16] [8, 8] [1, 1] : tensor<33x33xf32> to tensor<8x8xf32>
    %extracted_slice_31 = tensor.extract_slice %arg0[8, 0] [8, 16] [1, 1] : tensor<33x16xf32> to tensor<8x16xf32>
    %extracted_slice_32 = tensor.extract_slice %arg1[0, 16] [16, 8] [1, 1] : tensor<16x33xf32> to tensor<16x8xf32>
    %8 = systolic.matmul_tile %extracted_slice_31, %extracted_slice_32, %extracted_slice_30 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 404 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 8 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice_33 = tensor.insert_slice %8 into %inserted_slice_29[8, 16] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<33x33xf32>
    %extracted_slice_34 = tensor.extract_slice %inserted_slice_33[8, 24] [8, 8] [1, 1] : tensor<33x33xf32> to tensor<8x8xf32>
    %extracted_slice_35 = tensor.extract_slice %arg0[8, 0] [8, 16] [1, 1] : tensor<33x16xf32> to tensor<8x16xf32>
    %extracted_slice_36 = tensor.extract_slice %arg1[0, 24] [16, 8] [1, 1] : tensor<16x33xf32> to tensor<16x8xf32>
    %9 = systolic.matmul_tile %extracted_slice_35, %extracted_slice_36, %extracted_slice_34 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 473 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 9 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice_37 = tensor.insert_slice %9 into %inserted_slice_33[8, 24] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<33x33xf32>
    %extracted_slice_38 = tensor.extract_slice %inserted_slice_37[8, 32] [4, 1] [1, 1] : tensor<33x33xf32> to tensor<4x1xf32>
    %extracted_slice_39 = tensor.extract_slice %arg0[8, 0] [4, 16] [1, 1] : tensor<33x16xf32> to tensor<4x16xf32>
    %extracted_slice_40 = tensor.extract_slice %arg1[0, 32] [16, 1] [1, 1] : tensor<16x33xf32> to tensor<16x1xf32>
    %10 = systolic.matmul_tile %extracted_slice_39, %extracted_slice_40, %extracted_slice_38 m = 4 n = 1 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 0 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 10 : i64} : (tensor<4x16xf32>, tensor<16x1xf32>, tensor<4x1xf32>) -> tensor<4x1xf32>
    %inserted_slice_41 = tensor.insert_slice %10 into %inserted_slice_37[8, 32] [4, 1] [1, 1] : tensor<4x1xf32> into tensor<33x33xf32>
    %extracted_slice_42 = tensor.extract_slice %inserted_slice_41[12, 32] [4, 1] [1, 1] : tensor<33x33xf32> to tensor<4x1xf32>
    %extracted_slice_43 = tensor.extract_slice %arg0[12, 0] [4, 16] [1, 1] : tensor<33x16xf32> to tensor<4x16xf32>
    %extracted_slice_44 = tensor.extract_slice %arg1[0, 32] [16, 1] [1, 1] : tensor<16x33xf32> to tensor<16x1xf32>
    %11 = systolic.matmul_tile %extracted_slice_43, %extracted_slice_44, %extracted_slice_42 m = 4 n = 1 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 42 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 11 : i64} : (tensor<4x16xf32>, tensor<16x1xf32>, tensor<4x1xf32>) -> tensor<4x1xf32>
    %inserted_slice_45 = tensor.insert_slice %11 into %inserted_slice_41[12, 32] [4, 1] [1, 1] : tensor<4x1xf32> into tensor<33x33xf32>
    %extracted_slice_46 = tensor.extract_slice %inserted_slice_45[16, 0] [8, 8] [1, 1] : tensor<33x33xf32> to tensor<8x8xf32>
    %extracted_slice_47 = tensor.extract_slice %arg0[16, 0] [8, 16] [1, 1] : tensor<33x16xf32> to tensor<8x16xf32>
    %extracted_slice_48 = tensor.extract_slice %arg1[0, 0] [16, 8] [1, 1] : tensor<16x33xf32> to tensor<16x8xf32>
    %12 = systolic.matmul_tile %extracted_slice_47, %extracted_slice_48, %extracted_slice_46 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 542 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 12 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice_49 = tensor.insert_slice %12 into %inserted_slice_45[16, 0] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<33x33xf32>
    %extracted_slice_50 = tensor.extract_slice %inserted_slice_49[16, 8] [8, 8] [1, 1] : tensor<33x33xf32> to tensor<8x8xf32>
    %extracted_slice_51 = tensor.extract_slice %arg0[16, 0] [8, 16] [1, 1] : tensor<33x16xf32> to tensor<8x16xf32>
    %extracted_slice_52 = tensor.extract_slice %arg1[0, 8] [16, 8] [1, 1] : tensor<16x33xf32> to tensor<16x8xf32>
    %13 = systolic.matmul_tile %extracted_slice_51, %extracted_slice_52, %extracted_slice_50 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 611 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 13 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice_53 = tensor.insert_slice %13 into %inserted_slice_49[16, 8] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<33x33xf32>
    %extracted_slice_54 = tensor.extract_slice %inserted_slice_53[16, 16] [8, 8] [1, 1] : tensor<33x33xf32> to tensor<8x8xf32>
    %extracted_slice_55 = tensor.extract_slice %arg0[16, 0] [8, 16] [1, 1] : tensor<33x16xf32> to tensor<8x16xf32>
    %extracted_slice_56 = tensor.extract_slice %arg1[0, 16] [16, 8] [1, 1] : tensor<16x33xf32> to tensor<16x8xf32>
    %14 = systolic.matmul_tile %extracted_slice_55, %extracted_slice_56, %extracted_slice_54 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 680 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 14 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice_57 = tensor.insert_slice %14 into %inserted_slice_53[16, 16] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<33x33xf32>
    %extracted_slice_58 = tensor.extract_slice %inserted_slice_57[16, 24] [8, 8] [1, 1] : tensor<33x33xf32> to tensor<8x8xf32>
    %extracted_slice_59 = tensor.extract_slice %arg0[16, 0] [8, 16] [1, 1] : tensor<33x16xf32> to tensor<8x16xf32>
    %extracted_slice_60 = tensor.extract_slice %arg1[0, 24] [16, 8] [1, 1] : tensor<16x33xf32> to tensor<16x8xf32>
    %15 = systolic.matmul_tile %extracted_slice_59, %extracted_slice_60, %extracted_slice_58 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 749 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 15 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice_61 = tensor.insert_slice %15 into %inserted_slice_57[16, 24] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<33x33xf32>
    %extracted_slice_62 = tensor.extract_slice %inserted_slice_61[16, 32] [4, 1] [1, 1] : tensor<33x33xf32> to tensor<4x1xf32>
    %extracted_slice_63 = tensor.extract_slice %arg0[16, 0] [4, 16] [1, 1] : tensor<33x16xf32> to tensor<4x16xf32>
    %extracted_slice_64 = tensor.extract_slice %arg1[0, 32] [16, 1] [1, 1] : tensor<16x33xf32> to tensor<16x1xf32>
    %16 = systolic.matmul_tile %extracted_slice_63, %extracted_slice_64, %extracted_slice_62 m = 4 n = 1 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 42 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 16 : i64} : (tensor<4x16xf32>, tensor<16x1xf32>, tensor<4x1xf32>) -> tensor<4x1xf32>
    %inserted_slice_65 = tensor.insert_slice %16 into %inserted_slice_61[16, 32] [4, 1] [1, 1] : tensor<4x1xf32> into tensor<33x33xf32>
    %extracted_slice_66 = tensor.extract_slice %inserted_slice_65[20, 32] [4, 1] [1, 1] : tensor<33x33xf32> to tensor<4x1xf32>
    %extracted_slice_67 = tensor.extract_slice %arg0[20, 0] [4, 16] [1, 1] : tensor<33x16xf32> to tensor<4x16xf32>
    %extracted_slice_68 = tensor.extract_slice %arg1[0, 32] [16, 1] [1, 1] : tensor<16x33xf32> to tensor<16x1xf32>
    %17 = systolic.matmul_tile %extracted_slice_67, %extracted_slice_68, %extracted_slice_66 m = 4 n = 1 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 42 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 17 : i64} : (tensor<4x16xf32>, tensor<16x1xf32>, tensor<4x1xf32>) -> tensor<4x1xf32>
    %inserted_slice_69 = tensor.insert_slice %17 into %inserted_slice_65[20, 32] [4, 1] [1, 1] : tensor<4x1xf32> into tensor<33x33xf32>
    %extracted_slice_70 = tensor.extract_slice %inserted_slice_69[24, 0] [4, 4] [1, 1] : tensor<33x33xf32> to tensor<4x4xf32>
    %extracted_slice_71 = tensor.extract_slice %arg0[24, 0] [4, 16] [1, 1] : tensor<33x16xf32> to tensor<4x16xf32>
    %extracted_slice_72 = tensor.extract_slice %arg1[0, 0] [16, 4] [1, 1] : tensor<16x33xf32> to tensor<16x4xf32>
    %18 = systolic.matmul_tile %extracted_slice_71, %extracted_slice_72, %extracted_slice_70 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 120 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 18 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_73 = tensor.insert_slice %18 into %inserted_slice_69[24, 0] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<33x33xf32>
    %extracted_slice_74 = tensor.extract_slice %inserted_slice_73[24, 4] [4, 4] [1, 1] : tensor<33x33xf32> to tensor<4x4xf32>
    %extracted_slice_75 = tensor.extract_slice %arg0[24, 0] [4, 16] [1, 1] : tensor<33x16xf32> to tensor<4x16xf32>
    %extracted_slice_76 = tensor.extract_slice %arg1[0, 4] [16, 4] [1, 1] : tensor<16x33xf32> to tensor<16x4xf32>
    %19 = systolic.matmul_tile %extracted_slice_75, %extracted_slice_76, %extracted_slice_74 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 120 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 19 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_77 = tensor.insert_slice %19 into %inserted_slice_73[24, 4] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<33x33xf32>
    %extracted_slice_78 = tensor.extract_slice %inserted_slice_77[24, 8] [4, 4] [1, 1] : tensor<33x33xf32> to tensor<4x4xf32>
    %extracted_slice_79 = tensor.extract_slice %arg0[24, 0] [4, 16] [1, 1] : tensor<33x16xf32> to tensor<4x16xf32>
    %extracted_slice_80 = tensor.extract_slice %arg1[0, 8] [16, 4] [1, 1] : tensor<16x33xf32> to tensor<16x4xf32>
    %20 = systolic.matmul_tile %extracted_slice_79, %extracted_slice_80, %extracted_slice_78 m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 120 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 20 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_81 = tensor.insert_slice %20 into %inserted_slice_77[24, 8] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<33x33xf32>
    %extracted_slice_82 = tensor.extract_slice %inserted_slice_81[24, 12] [4, 4] [1, 1] : tensor<33x33xf32> to tensor<4x4xf32>
    %extracted_slice_83 = tensor.extract_slice %arg0[24, 0] [4, 16] [1, 1] : tensor<33x16xf32> to tensor<4x16xf32>
    %extracted_slice_84 = tensor.extract_slice %arg1[0, 12] [16, 4] [1, 1] : tensor<16x33xf32> to tensor<16x4xf32>
    %21 = systolic.matmul_tile %extracted_slice_83, %extracted_slice_84, %extracted_slice_82 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 189 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 21 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_85 = tensor.insert_slice %21 into %inserted_slice_81[24, 12] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<33x33xf32>
    %extracted_slice_86 = tensor.extract_slice %inserted_slice_85[24, 16] [4, 4] [1, 1] : tensor<33x33xf32> to tensor<4x4xf32>
    %extracted_slice_87 = tensor.extract_slice %arg0[24, 0] [4, 16] [1, 1] : tensor<33x16xf32> to tensor<4x16xf32>
    %extracted_slice_88 = tensor.extract_slice %arg1[0, 16] [16, 4] [1, 1] : tensor<16x33xf32> to tensor<16x4xf32>
    %22 = systolic.matmul_tile %extracted_slice_87, %extracted_slice_88, %extracted_slice_86 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 189 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 22 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_89 = tensor.insert_slice %22 into %inserted_slice_85[24, 16] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<33x33xf32>
    %extracted_slice_90 = tensor.extract_slice %inserted_slice_89[24, 20] [4, 4] [1, 1] : tensor<33x33xf32> to tensor<4x4xf32>
    %extracted_slice_91 = tensor.extract_slice %arg0[24, 0] [4, 16] [1, 1] : tensor<33x16xf32> to tensor<4x16xf32>
    %extracted_slice_92 = tensor.extract_slice %arg1[0, 20] [16, 4] [1, 1] : tensor<16x33xf32> to tensor<16x4xf32>
    %23 = systolic.matmul_tile %extracted_slice_91, %extracted_slice_92, %extracted_slice_90 m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 189 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 23 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_93 = tensor.insert_slice %23 into %inserted_slice_89[24, 20] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<33x33xf32>
    %extracted_slice_94 = tensor.extract_slice %inserted_slice_93[24, 24] [4, 4] [1, 1] : tensor<33x33xf32> to tensor<4x4xf32>
    %extracted_slice_95 = tensor.extract_slice %arg0[24, 0] [4, 16] [1, 1] : tensor<33x16xf32> to tensor<4x16xf32>
    %extracted_slice_96 = tensor.extract_slice %arg1[0, 24] [16, 4] [1, 1] : tensor<16x33xf32> to tensor<16x4xf32>
    %24 = systolic.matmul_tile %extracted_slice_95, %extracted_slice_96, %extracted_slice_94 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 258 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 24 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_97 = tensor.insert_slice %24 into %inserted_slice_93[24, 24] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<33x33xf32>
    %extracted_slice_98 = tensor.extract_slice %inserted_slice_97[24, 28] [4, 4] [1, 1] : tensor<33x33xf32> to tensor<4x4xf32>
    %extracted_slice_99 = tensor.extract_slice %arg0[24, 0] [4, 16] [1, 1] : tensor<33x16xf32> to tensor<4x16xf32>
    %extracted_slice_100 = tensor.extract_slice %arg1[0, 28] [16, 4] [1, 1] : tensor<16x33xf32> to tensor<16x4xf32>
    %25 = systolic.matmul_tile %extracted_slice_99, %extracted_slice_100, %extracted_slice_98 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 258 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 25 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_101 = tensor.insert_slice %25 into %inserted_slice_97[24, 28] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<33x33xf32>
    %extracted_slice_102 = tensor.extract_slice %inserted_slice_101[24, 32] [4, 1] [1, 1] : tensor<33x33xf32> to tensor<4x1xf32>
    %extracted_slice_103 = tensor.extract_slice %arg0[24, 0] [4, 16] [1, 1] : tensor<33x16xf32> to tensor<4x16xf32>
    %extracted_slice_104 = tensor.extract_slice %arg1[0, 32] [16, 1] [1, 1] : tensor<16x33xf32> to tensor<16x1xf32>
    %26 = systolic.matmul_tile %extracted_slice_103, %extracted_slice_104, %extracted_slice_102 m = 4 n = 1 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 258 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 26 : i64} : (tensor<4x16xf32>, tensor<16x1xf32>, tensor<4x1xf32>) -> tensor<4x1xf32>
    %inserted_slice_105 = tensor.insert_slice %26 into %inserted_slice_101[24, 32] [4, 1] [1, 1] : tensor<4x1xf32> into tensor<33x33xf32>
    %extracted_slice_106 = tensor.extract_slice %inserted_slice_105[28, 0] [4, 4] [1, 1] : tensor<33x33xf32> to tensor<4x4xf32>
    %extracted_slice_107 = tensor.extract_slice %arg0[28, 0] [4, 16] [1, 1] : tensor<33x16xf32> to tensor<4x16xf32>
    %extracted_slice_108 = tensor.extract_slice %arg1[0, 0] [16, 4] [1, 1] : tensor<16x33xf32> to tensor<16x4xf32>
    %27 = systolic.matmul_tile %extracted_slice_107, %extracted_slice_108, %extracted_slice_106 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 327 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 27 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_109 = tensor.insert_slice %27 into %inserted_slice_105[28, 0] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<33x33xf32>
    %extracted_slice_110 = tensor.extract_slice %inserted_slice_109[28, 4] [4, 4] [1, 1] : tensor<33x33xf32> to tensor<4x4xf32>
    %extracted_slice_111 = tensor.extract_slice %arg0[28, 0] [4, 16] [1, 1] : tensor<33x16xf32> to tensor<4x16xf32>
    %extracted_slice_112 = tensor.extract_slice %arg1[0, 4] [16, 4] [1, 1] : tensor<16x33xf32> to tensor<16x4xf32>
    %28 = systolic.matmul_tile %extracted_slice_111, %extracted_slice_112, %extracted_slice_110 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 327 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 28 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_113 = tensor.insert_slice %28 into %inserted_slice_109[28, 4] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<33x33xf32>
    %extracted_slice_114 = tensor.extract_slice %inserted_slice_113[28, 8] [4, 4] [1, 1] : tensor<33x33xf32> to tensor<4x4xf32>
    %extracted_slice_115 = tensor.extract_slice %arg0[28, 0] [4, 16] [1, 1] : tensor<33x16xf32> to tensor<4x16xf32>
    %extracted_slice_116 = tensor.extract_slice %arg1[0, 8] [16, 4] [1, 1] : tensor<16x33xf32> to tensor<16x4xf32>
    %29 = systolic.matmul_tile %extracted_slice_115, %extracted_slice_116, %extracted_slice_114 m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 327 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 29 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_117 = tensor.insert_slice %29 into %inserted_slice_113[28, 8] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<33x33xf32>
    %extracted_slice_118 = tensor.extract_slice %inserted_slice_117[28, 12] [4, 4] [1, 1] : tensor<33x33xf32> to tensor<4x4xf32>
    %extracted_slice_119 = tensor.extract_slice %arg0[28, 0] [4, 16] [1, 1] : tensor<33x16xf32> to tensor<4x16xf32>
    %extracted_slice_120 = tensor.extract_slice %arg1[0, 12] [16, 4] [1, 1] : tensor<16x33xf32> to tensor<16x4xf32>
    %30 = systolic.matmul_tile %extracted_slice_119, %extracted_slice_120, %extracted_slice_118 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 396 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 30 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_121 = tensor.insert_slice %30 into %inserted_slice_117[28, 12] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<33x33xf32>
    %extracted_slice_122 = tensor.extract_slice %inserted_slice_121[28, 16] [4, 4] [1, 1] : tensor<33x33xf32> to tensor<4x4xf32>
    %extracted_slice_123 = tensor.extract_slice %arg0[28, 0] [4, 16] [1, 1] : tensor<33x16xf32> to tensor<4x16xf32>
    %extracted_slice_124 = tensor.extract_slice %arg1[0, 16] [16, 4] [1, 1] : tensor<16x33xf32> to tensor<16x4xf32>
    %31 = systolic.matmul_tile %extracted_slice_123, %extracted_slice_124, %extracted_slice_122 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 396 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 31 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_125 = tensor.insert_slice %31 into %inserted_slice_121[28, 16] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<33x33xf32>
    %extracted_slice_126 = tensor.extract_slice %inserted_slice_125[28, 20] [4, 4] [1, 1] : tensor<33x33xf32> to tensor<4x4xf32>
    %extracted_slice_127 = tensor.extract_slice %arg0[28, 0] [4, 16] [1, 1] : tensor<33x16xf32> to tensor<4x16xf32>
    %extracted_slice_128 = tensor.extract_slice %arg1[0, 20] [16, 4] [1, 1] : tensor<16x33xf32> to tensor<16x4xf32>
    %32 = systolic.matmul_tile %extracted_slice_127, %extracted_slice_128, %extracted_slice_126 m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 396 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 32 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_129 = tensor.insert_slice %32 into %inserted_slice_125[28, 20] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<33x33xf32>
    %extracted_slice_130 = tensor.extract_slice %inserted_slice_129[28, 24] [4, 4] [1, 1] : tensor<33x33xf32> to tensor<4x4xf32>
    %extracted_slice_131 = tensor.extract_slice %arg0[28, 0] [4, 16] [1, 1] : tensor<33x16xf32> to tensor<4x16xf32>
    %extracted_slice_132 = tensor.extract_slice %arg1[0, 24] [16, 4] [1, 1] : tensor<16x33xf32> to tensor<16x4xf32>
    %33 = systolic.matmul_tile %extracted_slice_131, %extracted_slice_132, %extracted_slice_130 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 465 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 33 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_133 = tensor.insert_slice %33 into %inserted_slice_129[28, 24] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<33x33xf32>
    %extracted_slice_134 = tensor.extract_slice %inserted_slice_133[28, 28] [4, 4] [1, 1] : tensor<33x33xf32> to tensor<4x4xf32>
    %extracted_slice_135 = tensor.extract_slice %arg0[28, 0] [4, 16] [1, 1] : tensor<33x16xf32> to tensor<4x16xf32>
    %extracted_slice_136 = tensor.extract_slice %arg1[0, 28] [16, 4] [1, 1] : tensor<16x33xf32> to tensor<16x4xf32>
    %34 = systolic.matmul_tile %extracted_slice_135, %extracted_slice_136, %extracted_slice_134 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 465 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 34 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_137 = tensor.insert_slice %34 into %inserted_slice_133[28, 28] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<33x33xf32>
    %extracted_slice_138 = tensor.extract_slice %inserted_slice_137[28, 32] [4, 1] [1, 1] : tensor<33x33xf32> to tensor<4x1xf32>
    %extracted_slice_139 = tensor.extract_slice %arg0[28, 0] [4, 16] [1, 1] : tensor<33x16xf32> to tensor<4x16xf32>
    %extracted_slice_140 = tensor.extract_slice %arg1[0, 32] [16, 1] [1, 1] : tensor<16x33xf32> to tensor<16x1xf32>
    %35 = systolic.matmul_tile %extracted_slice_139, %extracted_slice_140, %extracted_slice_138 m = 4 n = 1 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 465 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 35 : i64} : (tensor<4x16xf32>, tensor<16x1xf32>, tensor<4x1xf32>) -> tensor<4x1xf32>
    %inserted_slice_141 = tensor.insert_slice %35 into %inserted_slice_137[28, 32] [4, 1] [1, 1] : tensor<4x1xf32> into tensor<33x33xf32>
    %extracted_slice_142 = tensor.extract_slice %inserted_slice_141[32, 0] [1, 4] [1, 1] : tensor<33x33xf32> to tensor<1x4xf32>
    %extracted_slice_143 = tensor.extract_slice %arg0[32, 0] [1, 16] [1, 1] : tensor<33x16xf32> to tensor<1x16xf32>
    %extracted_slice_144 = tensor.extract_slice %arg1[0, 0] [16, 4] [1, 1] : tensor<16x33xf32> to tensor<16x4xf32>
    %36 = systolic.matmul_tile %extracted_slice_143, %extracted_slice_144, %extracted_slice_142 m = 1 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 534 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 36 : i64} : (tensor<1x16xf32>, tensor<16x4xf32>, tensor<1x4xf32>) -> tensor<1x4xf32>
    %inserted_slice_145 = tensor.insert_slice %36 into %inserted_slice_141[32, 0] [1, 4] [1, 1] : tensor<1x4xf32> into tensor<33x33xf32>
    %extracted_slice_146 = tensor.extract_slice %inserted_slice_145[32, 4] [1, 4] [1, 1] : tensor<33x33xf32> to tensor<1x4xf32>
    %extracted_slice_147 = tensor.extract_slice %arg0[32, 0] [1, 16] [1, 1] : tensor<33x16xf32> to tensor<1x16xf32>
    %extracted_slice_148 = tensor.extract_slice %arg1[0, 4] [16, 4] [1, 1] : tensor<16x33xf32> to tensor<16x4xf32>
    %37 = systolic.matmul_tile %extracted_slice_147, %extracted_slice_148, %extracted_slice_146 m = 1 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 534 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 37 : i64} : (tensor<1x16xf32>, tensor<16x4xf32>, tensor<1x4xf32>) -> tensor<1x4xf32>
    %inserted_slice_149 = tensor.insert_slice %37 into %inserted_slice_145[32, 4] [1, 4] [1, 1] : tensor<1x4xf32> into tensor<33x33xf32>
    %extracted_slice_150 = tensor.extract_slice %inserted_slice_149[32, 8] [1, 4] [1, 1] : tensor<33x33xf32> to tensor<1x4xf32>
    %extracted_slice_151 = tensor.extract_slice %arg0[32, 0] [1, 16] [1, 1] : tensor<33x16xf32> to tensor<1x16xf32>
    %extracted_slice_152 = tensor.extract_slice %arg1[0, 8] [16, 4] [1, 1] : tensor<16x33xf32> to tensor<16x4xf32>
    %38 = systolic.matmul_tile %extracted_slice_151, %extracted_slice_152, %extracted_slice_150 m = 1 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 534 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 38 : i64} : (tensor<1x16xf32>, tensor<16x4xf32>, tensor<1x4xf32>) -> tensor<1x4xf32>
    %inserted_slice_153 = tensor.insert_slice %38 into %inserted_slice_149[32, 8] [1, 4] [1, 1] : tensor<1x4xf32> into tensor<33x33xf32>
    %extracted_slice_154 = tensor.extract_slice %inserted_slice_153[32, 12] [1, 4] [1, 1] : tensor<33x33xf32> to tensor<1x4xf32>
    %extracted_slice_155 = tensor.extract_slice %arg0[32, 0] [1, 16] [1, 1] : tensor<33x16xf32> to tensor<1x16xf32>
    %extracted_slice_156 = tensor.extract_slice %arg1[0, 12] [16, 4] [1, 1] : tensor<16x33xf32> to tensor<16x4xf32>
    %39 = systolic.matmul_tile %extracted_slice_155, %extracted_slice_156, %extracted_slice_154 m = 1 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 603 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 39 : i64} : (tensor<1x16xf32>, tensor<16x4xf32>, tensor<1x4xf32>) -> tensor<1x4xf32>
    %inserted_slice_157 = tensor.insert_slice %39 into %inserted_slice_153[32, 12] [1, 4] [1, 1] : tensor<1x4xf32> into tensor<33x33xf32>
    %extracted_slice_158 = tensor.extract_slice %inserted_slice_157[32, 16] [1, 4] [1, 1] : tensor<33x33xf32> to tensor<1x4xf32>
    %extracted_slice_159 = tensor.extract_slice %arg0[32, 0] [1, 16] [1, 1] : tensor<33x16xf32> to tensor<1x16xf32>
    %extracted_slice_160 = tensor.extract_slice %arg1[0, 16] [16, 4] [1, 1] : tensor<16x33xf32> to tensor<16x4xf32>
    %40 = systolic.matmul_tile %extracted_slice_159, %extracted_slice_160, %extracted_slice_158 m = 1 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 603 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 40 : i64} : (tensor<1x16xf32>, tensor<16x4xf32>, tensor<1x4xf32>) -> tensor<1x4xf32>
    %inserted_slice_161 = tensor.insert_slice %40 into %inserted_slice_157[32, 16] [1, 4] [1, 1] : tensor<1x4xf32> into tensor<33x33xf32>
    %extracted_slice_162 = tensor.extract_slice %inserted_slice_161[32, 20] [1, 4] [1, 1] : tensor<33x33xf32> to tensor<1x4xf32>
    %extracted_slice_163 = tensor.extract_slice %arg0[32, 0] [1, 16] [1, 1] : tensor<33x16xf32> to tensor<1x16xf32>
    %extracted_slice_164 = tensor.extract_slice %arg1[0, 20] [16, 4] [1, 1] : tensor<16x33xf32> to tensor<16x4xf32>
    %41 = systolic.matmul_tile %extracted_slice_163, %extracted_slice_164, %extracted_slice_162 m = 1 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 603 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 41 : i64} : (tensor<1x16xf32>, tensor<16x4xf32>, tensor<1x4xf32>) -> tensor<1x4xf32>
    %inserted_slice_165 = tensor.insert_slice %41 into %inserted_slice_161[32, 20] [1, 4] [1, 1] : tensor<1x4xf32> into tensor<33x33xf32>
    %extracted_slice_166 = tensor.extract_slice %inserted_slice_165[32, 24] [1, 4] [1, 1] : tensor<33x33xf32> to tensor<1x4xf32>
    %extracted_slice_167 = tensor.extract_slice %arg0[32, 0] [1, 16] [1, 1] : tensor<33x16xf32> to tensor<1x16xf32>
    %extracted_slice_168 = tensor.extract_slice %arg1[0, 24] [16, 4] [1, 1] : tensor<16x33xf32> to tensor<16x4xf32>
    %42 = systolic.matmul_tile %extracted_slice_167, %extracted_slice_168, %extracted_slice_166 m = 1 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 672 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 42 : i64} : (tensor<1x16xf32>, tensor<16x4xf32>, tensor<1x4xf32>) -> tensor<1x4xf32>
    %inserted_slice_169 = tensor.insert_slice %42 into %inserted_slice_165[32, 24] [1, 4] [1, 1] : tensor<1x4xf32> into tensor<33x33xf32>
    %extracted_slice_170 = tensor.extract_slice %inserted_slice_169[32, 28] [1, 4] [1, 1] : tensor<33x33xf32> to tensor<1x4xf32>
    %extracted_slice_171 = tensor.extract_slice %arg0[32, 0] [1, 16] [1, 1] : tensor<33x16xf32> to tensor<1x16xf32>
    %extracted_slice_172 = tensor.extract_slice %arg1[0, 28] [16, 4] [1, 1] : tensor<16x33xf32> to tensor<16x4xf32>
    %43 = systolic.matmul_tile %extracted_slice_171, %extracted_slice_172, %extracted_slice_170 m = 1 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 672 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 43 : i64} : (tensor<1x16xf32>, tensor<16x4xf32>, tensor<1x4xf32>) -> tensor<1x4xf32>
    %inserted_slice_173 = tensor.insert_slice %43 into %inserted_slice_169[32, 28] [1, 4] [1, 1] : tensor<1x4xf32> into tensor<33x33xf32>
    %extracted_slice_174 = tensor.extract_slice %inserted_slice_173[32, 32] [1, 1] [1, 1] : tensor<33x33xf32> to tensor<1x1xf32>
    %extracted_slice_175 = tensor.extract_slice %arg0[32, 0] [1, 16] [1, 1] : tensor<33x16xf32> to tensor<1x16xf32>
    %extracted_slice_176 = tensor.extract_slice %arg1[0, 32] [16, 1] [1, 1] : tensor<16x33xf32> to tensor<16x1xf32>
    %44 = systolic.matmul_tile %extracted_slice_175, %extracted_slice_176, %extracted_slice_174 m = 1 n = 1 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 672 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 44 : i64} : (tensor<1x16xf32>, tensor<16x1xf32>, tensor<1x1xf32>) -> tensor<1x1xf32>
    %inserted_slice_177 = tensor.insert_slice %44 into %inserted_slice_173[32, 32] [1, 1] [1, 1] : tensor<1x1xf32> into tensor<33x33xf32>
    return %inserted_slice_177 : tensor<33x33xf32>
  }
}

