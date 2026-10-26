module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  func.func @systolic_matmul(%arg0: tensor<32x32xf32>, %arg1: tensor<32x32xf32>, %arg2: tensor<32x32xf32>) -> tensor<32x32xf32> attributes {llvm.emit_c_interface} {
    %extracted_slice = tensor.extract_slice %arg2[0, 0] [8, 8] [1, 1] : tensor<32x32xf32> to tensor<8x8xf32>
    %extracted_slice_0 = tensor.extract_slice %arg0[0, 0] [8, 16] [1, 1] : tensor<32x32xf32> to tensor<8x16xf32>
    %extracted_slice_1 = tensor.extract_slice %arg1[0, 0] [16, 8] [1, 1] : tensor<32x32xf32> to tensor<16x8xf32>
    %0 = systolic.matmul_tile %extracted_slice_0, %extracted_slice_1, %extracted_slice m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 0 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 0 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %extracted_slice_2 = tensor.extract_slice %arg0[0, 16] [8, 16] [1, 1] : tensor<32x32xf32> to tensor<8x16xf32>
    %extracted_slice_3 = tensor.extract_slice %arg1[16, 0] [16, 8] [1, 1] : tensor<32x32xf32> to tensor<16x8xf32>
    %1 = systolic.matmul_tile %extracted_slice_2, %extracted_slice_3, %0 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 42 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 0 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice = tensor.insert_slice %1 into %arg2[0, 0] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<32x32xf32>
    %extracted_slice_4 = tensor.extract_slice %inserted_slice[0, 8] [8, 8] [1, 1] : tensor<32x32xf32> to tensor<8x8xf32>
    %extracted_slice_5 = tensor.extract_slice %arg0[0, 0] [8, 16] [1, 1] : tensor<32x32xf32> to tensor<8x16xf32>
    %extracted_slice_6 = tensor.extract_slice %arg1[0, 8] [16, 8] [1, 1] : tensor<32x32xf32> to tensor<16x8xf32>
    %2 = systolic.matmul_tile %extracted_slice_5, %extracted_slice_6, %extracted_slice_4 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 128 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 1 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %extracted_slice_7 = tensor.extract_slice %arg0[0, 16] [8, 16] [1, 1] : tensor<32x32xf32> to tensor<8x16xf32>
    %extracted_slice_8 = tensor.extract_slice %arg1[16, 8] [16, 8] [1, 1] : tensor<32x32xf32> to tensor<16x8xf32>
    %3 = systolic.matmul_tile %extracted_slice_7, %extracted_slice_8, %2 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 197 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 1 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice_9 = tensor.insert_slice %3 into %inserted_slice[0, 8] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<32x32xf32>
    %extracted_slice_10 = tensor.extract_slice %inserted_slice_9[0, 16] [8, 8] [1, 1] : tensor<32x32xf32> to tensor<8x8xf32>
    %extracted_slice_11 = tensor.extract_slice %arg0[0, 0] [8, 16] [1, 1] : tensor<32x32xf32> to tensor<8x16xf32>
    %extracted_slice_12 = tensor.extract_slice %arg1[0, 16] [16, 8] [1, 1] : tensor<32x32xf32> to tensor<16x8xf32>
    %4 = systolic.matmul_tile %extracted_slice_11, %extracted_slice_12, %extracted_slice_10 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 266 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 2 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %extracted_slice_13 = tensor.extract_slice %arg0[0, 16] [8, 16] [1, 1] : tensor<32x32xf32> to tensor<8x16xf32>
    %extracted_slice_14 = tensor.extract_slice %arg1[16, 16] [16, 8] [1, 1] : tensor<32x32xf32> to tensor<16x8xf32>
    %5 = systolic.matmul_tile %extracted_slice_13, %extracted_slice_14, %4 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 335 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 2 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice_15 = tensor.insert_slice %5 into %inserted_slice_9[0, 16] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<32x32xf32>
    %extracted_slice_16 = tensor.extract_slice %inserted_slice_15[0, 24] [8, 8] [1, 1] : tensor<32x32xf32> to tensor<8x8xf32>
    %extracted_slice_17 = tensor.extract_slice %arg0[0, 0] [8, 16] [1, 1] : tensor<32x32xf32> to tensor<8x16xf32>
    %extracted_slice_18 = tensor.extract_slice %arg1[0, 24] [16, 8] [1, 1] : tensor<32x32xf32> to tensor<16x8xf32>
    %6 = systolic.matmul_tile %extracted_slice_17, %extracted_slice_18, %extracted_slice_16 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 404 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 3 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %extracted_slice_19 = tensor.extract_slice %arg0[0, 16] [8, 16] [1, 1] : tensor<32x32xf32> to tensor<8x16xf32>
    %extracted_slice_20 = tensor.extract_slice %arg1[16, 24] [16, 8] [1, 1] : tensor<32x32xf32> to tensor<16x8xf32>
    %7 = systolic.matmul_tile %extracted_slice_19, %extracted_slice_20, %6 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 473 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 3 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice_21 = tensor.insert_slice %7 into %inserted_slice_15[0, 24] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<32x32xf32>
    %extracted_slice_22 = tensor.extract_slice %inserted_slice_21[8, 0] [8, 8] [1, 1] : tensor<32x32xf32> to tensor<8x8xf32>
    %extracted_slice_23 = tensor.extract_slice %arg0[8, 0] [8, 16] [1, 1] : tensor<32x32xf32> to tensor<8x16xf32>
    %extracted_slice_24 = tensor.extract_slice %arg1[0, 0] [16, 8] [1, 1] : tensor<32x32xf32> to tensor<16x8xf32>
    %8 = systolic.matmul_tile %extracted_slice_23, %extracted_slice_24, %extracted_slice_22 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 542 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 4 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %extracted_slice_25 = tensor.extract_slice %arg0[8, 16] [8, 16] [1, 1] : tensor<32x32xf32> to tensor<8x16xf32>
    %extracted_slice_26 = tensor.extract_slice %arg1[16, 0] [16, 8] [1, 1] : tensor<32x32xf32> to tensor<16x8xf32>
    %9 = systolic.matmul_tile %extracted_slice_25, %extracted_slice_26, %8 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 611 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 4 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice_27 = tensor.insert_slice %9 into %inserted_slice_21[8, 0] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<32x32xf32>
    %extracted_slice_28 = tensor.extract_slice %inserted_slice_27[8, 8] [8, 8] [1, 1] : tensor<32x32xf32> to tensor<8x8xf32>
    %extracted_slice_29 = tensor.extract_slice %arg0[8, 0] [8, 16] [1, 1] : tensor<32x32xf32> to tensor<8x16xf32>
    %extracted_slice_30 = tensor.extract_slice %arg1[0, 8] [16, 8] [1, 1] : tensor<32x32xf32> to tensor<16x8xf32>
    %10 = systolic.matmul_tile %extracted_slice_29, %extracted_slice_30, %extracted_slice_28 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 680 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 5 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %extracted_slice_31 = tensor.extract_slice %arg0[8, 16] [8, 16] [1, 1] : tensor<32x32xf32> to tensor<8x16xf32>
    %extracted_slice_32 = tensor.extract_slice %arg1[16, 8] [16, 8] [1, 1] : tensor<32x32xf32> to tensor<16x8xf32>
    %11 = systolic.matmul_tile %extracted_slice_31, %extracted_slice_32, %10 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 749 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 5 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice_33 = tensor.insert_slice %11 into %inserted_slice_27[8, 8] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<32x32xf32>
    %extracted_slice_34 = tensor.extract_slice %inserted_slice_33[8, 16] [8, 8] [1, 1] : tensor<32x32xf32> to tensor<8x8xf32>
    %extracted_slice_35 = tensor.extract_slice %arg0[8, 0] [8, 16] [1, 1] : tensor<32x32xf32> to tensor<8x16xf32>
    %extracted_slice_36 = tensor.extract_slice %arg1[0, 16] [16, 8] [1, 1] : tensor<32x32xf32> to tensor<16x8xf32>
    %12 = systolic.matmul_tile %extracted_slice_35, %extracted_slice_36, %extracted_slice_34 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 818 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 6 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %extracted_slice_37 = tensor.extract_slice %arg0[8, 16] [8, 16] [1, 1] : tensor<32x32xf32> to tensor<8x16xf32>
    %extracted_slice_38 = tensor.extract_slice %arg1[16, 16] [16, 8] [1, 1] : tensor<32x32xf32> to tensor<16x8xf32>
    %13 = systolic.matmul_tile %extracted_slice_37, %extracted_slice_38, %12 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 887 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 6 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice_39 = tensor.insert_slice %13 into %inserted_slice_33[8, 16] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<32x32xf32>
    %extracted_slice_40 = tensor.extract_slice %inserted_slice_39[8, 24] [8, 8] [1, 1] : tensor<32x32xf32> to tensor<8x8xf32>
    %extracted_slice_41 = tensor.extract_slice %arg0[8, 0] [8, 16] [1, 1] : tensor<32x32xf32> to tensor<8x16xf32>
    %extracted_slice_42 = tensor.extract_slice %arg1[0, 24] [16, 8] [1, 1] : tensor<32x32xf32> to tensor<16x8xf32>
    %14 = systolic.matmul_tile %extracted_slice_41, %extracted_slice_42, %extracted_slice_40 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 956 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 7 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %extracted_slice_43 = tensor.extract_slice %arg0[8, 16] [8, 16] [1, 1] : tensor<32x32xf32> to tensor<8x16xf32>
    %extracted_slice_44 = tensor.extract_slice %arg1[16, 24] [16, 8] [1, 1] : tensor<32x32xf32> to tensor<16x8xf32>
    %15 = systolic.matmul_tile %extracted_slice_43, %extracted_slice_44, %14 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 1025 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 7 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice_45 = tensor.insert_slice %15 into %inserted_slice_39[8, 24] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<32x32xf32>
    %extracted_slice_46 = tensor.extract_slice %inserted_slice_45[16, 0] [8, 8] [1, 1] : tensor<32x32xf32> to tensor<8x8xf32>
    %extracted_slice_47 = tensor.extract_slice %arg0[16, 0] [8, 16] [1, 1] : tensor<32x32xf32> to tensor<8x16xf32>
    %extracted_slice_48 = tensor.extract_slice %arg1[0, 0] [16, 8] [1, 1] : tensor<32x32xf32> to tensor<16x8xf32>
    %16 = systolic.matmul_tile %extracted_slice_47, %extracted_slice_48, %extracted_slice_46 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 1094 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 8 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %extracted_slice_49 = tensor.extract_slice %arg0[16, 16] [8, 16] [1, 1] : tensor<32x32xf32> to tensor<8x16xf32>
    %extracted_slice_50 = tensor.extract_slice %arg1[16, 0] [16, 8] [1, 1] : tensor<32x32xf32> to tensor<16x8xf32>
    %17 = systolic.matmul_tile %extracted_slice_49, %extracted_slice_50, %16 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 1163 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 8 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice_51 = tensor.insert_slice %17 into %inserted_slice_45[16, 0] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<32x32xf32>
    %extracted_slice_52 = tensor.extract_slice %inserted_slice_51[16, 8] [8, 8] [1, 1] : tensor<32x32xf32> to tensor<8x8xf32>
    %extracted_slice_53 = tensor.extract_slice %arg0[16, 0] [8, 16] [1, 1] : tensor<32x32xf32> to tensor<8x16xf32>
    %extracted_slice_54 = tensor.extract_slice %arg1[0, 8] [16, 8] [1, 1] : tensor<32x32xf32> to tensor<16x8xf32>
    %18 = systolic.matmul_tile %extracted_slice_53, %extracted_slice_54, %extracted_slice_52 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 1232 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 9 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %extracted_slice_55 = tensor.extract_slice %arg0[16, 16] [8, 16] [1, 1] : tensor<32x32xf32> to tensor<8x16xf32>
    %extracted_slice_56 = tensor.extract_slice %arg1[16, 8] [16, 8] [1, 1] : tensor<32x32xf32> to tensor<16x8xf32>
    %19 = systolic.matmul_tile %extracted_slice_55, %extracted_slice_56, %18 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 1301 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 9 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice_57 = tensor.insert_slice %19 into %inserted_slice_51[16, 8] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<32x32xf32>
    %extracted_slice_58 = tensor.extract_slice %inserted_slice_57[16, 16] [4, 4] [1, 1] : tensor<32x32xf32> to tensor<4x4xf32>
    %extracted_slice_59 = tensor.extract_slice %arg0[16, 0] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_60 = tensor.extract_slice %arg1[0, 16] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %20 = systolic.matmul_tile %extracted_slice_59, %extracted_slice_60, %extracted_slice_58 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 0 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 10 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_61 = tensor.extract_slice %arg0[16, 16] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_62 = tensor.extract_slice %arg1[16, 16] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %21 = systolic.matmul_tile %extracted_slice_61, %extracted_slice_62, %20 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 42 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 10 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_63 = tensor.insert_slice %21 into %inserted_slice_57[16, 16] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<32x32xf32>
    %extracted_slice_64 = tensor.extract_slice %inserted_slice_63[16, 20] [4, 4] [1, 1] : tensor<32x32xf32> to tensor<4x4xf32>
    %extracted_slice_65 = tensor.extract_slice %arg0[16, 0] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_66 = tensor.extract_slice %arg1[0, 20] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %22 = systolic.matmul_tile %extracted_slice_65, %extracted_slice_66, %extracted_slice_64 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 0 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 11 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_67 = tensor.extract_slice %arg0[16, 16] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_68 = tensor.extract_slice %arg1[16, 20] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %23 = systolic.matmul_tile %extracted_slice_67, %extracted_slice_68, %22 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 42 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 11 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_69 = tensor.insert_slice %23 into %inserted_slice_63[16, 20] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<32x32xf32>
    %extracted_slice_70 = tensor.extract_slice %inserted_slice_69[16, 24] [4, 4] [1, 1] : tensor<32x32xf32> to tensor<4x4xf32>
    %extracted_slice_71 = tensor.extract_slice %arg0[16, 0] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_72 = tensor.extract_slice %arg1[0, 24] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %24 = systolic.matmul_tile %extracted_slice_71, %extracted_slice_72, %extracted_slice_70 m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 0 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 12 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_73 = tensor.extract_slice %arg0[16, 16] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_74 = tensor.extract_slice %arg1[16, 24] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %25 = systolic.matmul_tile %extracted_slice_73, %extracted_slice_74, %24 m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 42 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 12 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_75 = tensor.insert_slice %25 into %inserted_slice_69[16, 24] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<32x32xf32>
    %extracted_slice_76 = tensor.extract_slice %inserted_slice_75[16, 28] [4, 4] [1, 1] : tensor<32x32xf32> to tensor<4x4xf32>
    %extracted_slice_77 = tensor.extract_slice %arg0[16, 0] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_78 = tensor.extract_slice %arg1[0, 28] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %26 = systolic.matmul_tile %extracted_slice_77, %extracted_slice_78, %extracted_slice_76 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 120 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 13 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_79 = tensor.extract_slice %arg0[16, 16] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_80 = tensor.extract_slice %arg1[16, 28] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %27 = systolic.matmul_tile %extracted_slice_79, %extracted_slice_80, %26 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 189 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 13 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_81 = tensor.insert_slice %27 into %inserted_slice_75[16, 28] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<32x32xf32>
    %extracted_slice_82 = tensor.extract_slice %inserted_slice_81[20, 16] [4, 4] [1, 1] : tensor<32x32xf32> to tensor<4x4xf32>
    %extracted_slice_83 = tensor.extract_slice %arg0[20, 0] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_84 = tensor.extract_slice %arg1[0, 16] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %28 = systolic.matmul_tile %extracted_slice_83, %extracted_slice_84, %extracted_slice_82 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 120 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 14 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_85 = tensor.extract_slice %arg0[20, 16] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_86 = tensor.extract_slice %arg1[16, 16] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %29 = systolic.matmul_tile %extracted_slice_85, %extracted_slice_86, %28 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 189 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 14 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_87 = tensor.insert_slice %29 into %inserted_slice_81[20, 16] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<32x32xf32>
    %extracted_slice_88 = tensor.extract_slice %inserted_slice_87[20, 20] [4, 4] [1, 1] : tensor<32x32xf32> to tensor<4x4xf32>
    %extracted_slice_89 = tensor.extract_slice %arg0[20, 0] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_90 = tensor.extract_slice %arg1[0, 20] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %30 = systolic.matmul_tile %extracted_slice_89, %extracted_slice_90, %extracted_slice_88 m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 120 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 15 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_91 = tensor.extract_slice %arg0[20, 16] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_92 = tensor.extract_slice %arg1[16, 20] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %31 = systolic.matmul_tile %extracted_slice_91, %extracted_slice_92, %30 m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 189 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 15 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_93 = tensor.insert_slice %31 into %inserted_slice_87[20, 20] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<32x32xf32>
    %extracted_slice_94 = tensor.extract_slice %inserted_slice_93[20, 24] [4, 4] [1, 1] : tensor<32x32xf32> to tensor<4x4xf32>
    %extracted_slice_95 = tensor.extract_slice %arg0[20, 0] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_96 = tensor.extract_slice %arg1[0, 24] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %32 = systolic.matmul_tile %extracted_slice_95, %extracted_slice_96, %extracted_slice_94 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 258 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 16 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_97 = tensor.extract_slice %arg0[20, 16] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_98 = tensor.extract_slice %arg1[16, 24] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %33 = systolic.matmul_tile %extracted_slice_97, %extracted_slice_98, %32 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 327 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 16 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_99 = tensor.insert_slice %33 into %inserted_slice_93[20, 24] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<32x32xf32>
    %extracted_slice_100 = tensor.extract_slice %inserted_slice_99[20, 28] [4, 4] [1, 1] : tensor<32x32xf32> to tensor<4x4xf32>
    %extracted_slice_101 = tensor.extract_slice %arg0[20, 0] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_102 = tensor.extract_slice %arg1[0, 28] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %34 = systolic.matmul_tile %extracted_slice_101, %extracted_slice_102, %extracted_slice_100 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 258 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 17 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_103 = tensor.extract_slice %arg0[20, 16] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_104 = tensor.extract_slice %arg1[16, 28] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %35 = systolic.matmul_tile %extracted_slice_103, %extracted_slice_104, %34 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 327 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 17 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_105 = tensor.insert_slice %35 into %inserted_slice_99[20, 28] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<32x32xf32>
    %extracted_slice_106 = tensor.extract_slice %inserted_slice_105[24, 0] [4, 4] [1, 1] : tensor<32x32xf32> to tensor<4x4xf32>
    %extracted_slice_107 = tensor.extract_slice %arg0[24, 0] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_108 = tensor.extract_slice %arg1[0, 0] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %36 = systolic.matmul_tile %extracted_slice_107, %extracted_slice_108, %extracted_slice_106 m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 258 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 18 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_109 = tensor.extract_slice %arg0[24, 16] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_110 = tensor.extract_slice %arg1[16, 0] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %37 = systolic.matmul_tile %extracted_slice_109, %extracted_slice_110, %36 m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 327 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 18 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_111 = tensor.insert_slice %37 into %inserted_slice_105[24, 0] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<32x32xf32>
    %extracted_slice_112 = tensor.extract_slice %inserted_slice_111[24, 4] [4, 4] [1, 1] : tensor<32x32xf32> to tensor<4x4xf32>
    %extracted_slice_113 = tensor.extract_slice %arg0[24, 0] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_114 = tensor.extract_slice %arg1[0, 4] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %38 = systolic.matmul_tile %extracted_slice_113, %extracted_slice_114, %extracted_slice_112 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 396 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 19 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_115 = tensor.extract_slice %arg0[24, 16] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_116 = tensor.extract_slice %arg1[16, 4] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %39 = systolic.matmul_tile %extracted_slice_115, %extracted_slice_116, %38 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 465 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 19 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_117 = tensor.insert_slice %39 into %inserted_slice_111[24, 4] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<32x32xf32>
    %extracted_slice_118 = tensor.extract_slice %inserted_slice_117[24, 8] [4, 4] [1, 1] : tensor<32x32xf32> to tensor<4x4xf32>
    %extracted_slice_119 = tensor.extract_slice %arg0[24, 0] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_120 = tensor.extract_slice %arg1[0, 8] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %40 = systolic.matmul_tile %extracted_slice_119, %extracted_slice_120, %extracted_slice_118 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 396 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 20 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_121 = tensor.extract_slice %arg0[24, 16] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_122 = tensor.extract_slice %arg1[16, 8] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %41 = systolic.matmul_tile %extracted_slice_121, %extracted_slice_122, %40 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 465 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 20 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_123 = tensor.insert_slice %41 into %inserted_slice_117[24, 8] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<32x32xf32>
    %extracted_slice_124 = tensor.extract_slice %inserted_slice_123[24, 12] [4, 4] [1, 1] : tensor<32x32xf32> to tensor<4x4xf32>
    %extracted_slice_125 = tensor.extract_slice %arg0[24, 0] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_126 = tensor.extract_slice %arg1[0, 12] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %42 = systolic.matmul_tile %extracted_slice_125, %extracted_slice_126, %extracted_slice_124 m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 396 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 21 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_127 = tensor.extract_slice %arg0[24, 16] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_128 = tensor.extract_slice %arg1[16, 12] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %43 = systolic.matmul_tile %extracted_slice_127, %extracted_slice_128, %42 m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 465 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 21 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_129 = tensor.insert_slice %43 into %inserted_slice_123[24, 12] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<32x32xf32>
    %extracted_slice_130 = tensor.extract_slice %inserted_slice_129[24, 16] [4, 4] [1, 1] : tensor<32x32xf32> to tensor<4x4xf32>
    %extracted_slice_131 = tensor.extract_slice %arg0[24, 0] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_132 = tensor.extract_slice %arg1[0, 16] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %44 = systolic.matmul_tile %extracted_slice_131, %extracted_slice_132, %extracted_slice_130 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 534 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 22 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_133 = tensor.extract_slice %arg0[24, 16] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_134 = tensor.extract_slice %arg1[16, 16] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %45 = systolic.matmul_tile %extracted_slice_133, %extracted_slice_134, %44 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 603 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 22 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_135 = tensor.insert_slice %45 into %inserted_slice_129[24, 16] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<32x32xf32>
    %extracted_slice_136 = tensor.extract_slice %inserted_slice_135[24, 20] [4, 4] [1, 1] : tensor<32x32xf32> to tensor<4x4xf32>
    %extracted_slice_137 = tensor.extract_slice %arg0[24, 0] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_138 = tensor.extract_slice %arg1[0, 20] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %46 = systolic.matmul_tile %extracted_slice_137, %extracted_slice_138, %extracted_slice_136 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 534 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 23 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_139 = tensor.extract_slice %arg0[24, 16] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_140 = tensor.extract_slice %arg1[16, 20] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %47 = systolic.matmul_tile %extracted_slice_139, %extracted_slice_140, %46 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 603 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 23 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_141 = tensor.insert_slice %47 into %inserted_slice_135[24, 20] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<32x32xf32>
    %extracted_slice_142 = tensor.extract_slice %inserted_slice_141[24, 24] [4, 4] [1, 1] : tensor<32x32xf32> to tensor<4x4xf32>
    %extracted_slice_143 = tensor.extract_slice %arg0[24, 0] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_144 = tensor.extract_slice %arg1[0, 24] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %48 = systolic.matmul_tile %extracted_slice_143, %extracted_slice_144, %extracted_slice_142 m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 534 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 24 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_145 = tensor.extract_slice %arg0[24, 16] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_146 = tensor.extract_slice %arg1[16, 24] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %49 = systolic.matmul_tile %extracted_slice_145, %extracted_slice_146, %48 m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 603 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 24 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_147 = tensor.insert_slice %49 into %inserted_slice_141[24, 24] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<32x32xf32>
    %extracted_slice_148 = tensor.extract_slice %inserted_slice_147[24, 28] [4, 4] [1, 1] : tensor<32x32xf32> to tensor<4x4xf32>
    %extracted_slice_149 = tensor.extract_slice %arg0[24, 0] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_150 = tensor.extract_slice %arg1[0, 28] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %50 = systolic.matmul_tile %extracted_slice_149, %extracted_slice_150, %extracted_slice_148 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 672 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 25 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_151 = tensor.extract_slice %arg0[24, 16] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_152 = tensor.extract_slice %arg1[16, 28] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %51 = systolic.matmul_tile %extracted_slice_151, %extracted_slice_152, %50 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 741 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 25 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_153 = tensor.insert_slice %51 into %inserted_slice_147[24, 28] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<32x32xf32>
    %extracted_slice_154 = tensor.extract_slice %inserted_slice_153[28, 0] [4, 4] [1, 1] : tensor<32x32xf32> to tensor<4x4xf32>
    %extracted_slice_155 = tensor.extract_slice %arg0[28, 0] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_156 = tensor.extract_slice %arg1[0, 0] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %52 = systolic.matmul_tile %extracted_slice_155, %extracted_slice_156, %extracted_slice_154 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 672 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 26 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_157 = tensor.extract_slice %arg0[28, 16] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_158 = tensor.extract_slice %arg1[16, 0] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %53 = systolic.matmul_tile %extracted_slice_157, %extracted_slice_158, %52 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 741 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 26 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_159 = tensor.insert_slice %53 into %inserted_slice_153[28, 0] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<32x32xf32>
    %extracted_slice_160 = tensor.extract_slice %inserted_slice_159[28, 4] [4, 4] [1, 1] : tensor<32x32xf32> to tensor<4x4xf32>
    %extracted_slice_161 = tensor.extract_slice %arg0[28, 0] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_162 = tensor.extract_slice %arg1[0, 4] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %54 = systolic.matmul_tile %extracted_slice_161, %extracted_slice_162, %extracted_slice_160 m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 672 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 27 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_163 = tensor.extract_slice %arg0[28, 16] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_164 = tensor.extract_slice %arg1[16, 4] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %55 = systolic.matmul_tile %extracted_slice_163, %extracted_slice_164, %54 m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 741 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 27 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_165 = tensor.insert_slice %55 into %inserted_slice_159[28, 4] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<32x32xf32>
    %extracted_slice_166 = tensor.extract_slice %inserted_slice_165[28, 8] [4, 4] [1, 1] : tensor<32x32xf32> to tensor<4x4xf32>
    %extracted_slice_167 = tensor.extract_slice %arg0[28, 0] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_168 = tensor.extract_slice %arg1[0, 8] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %56 = systolic.matmul_tile %extracted_slice_167, %extracted_slice_168, %extracted_slice_166 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 810 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 28 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_169 = tensor.extract_slice %arg0[28, 16] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_170 = tensor.extract_slice %arg1[16, 8] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %57 = systolic.matmul_tile %extracted_slice_169, %extracted_slice_170, %56 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 879 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 28 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_171 = tensor.insert_slice %57 into %inserted_slice_165[28, 8] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<32x32xf32>
    %extracted_slice_172 = tensor.extract_slice %inserted_slice_171[28, 12] [4, 4] [1, 1] : tensor<32x32xf32> to tensor<4x4xf32>
    %extracted_slice_173 = tensor.extract_slice %arg0[28, 0] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_174 = tensor.extract_slice %arg1[0, 12] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %58 = systolic.matmul_tile %extracted_slice_173, %extracted_slice_174, %extracted_slice_172 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 810 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 29 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_175 = tensor.extract_slice %arg0[28, 16] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_176 = tensor.extract_slice %arg1[16, 12] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %59 = systolic.matmul_tile %extracted_slice_175, %extracted_slice_176, %58 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 879 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 29 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_177 = tensor.insert_slice %59 into %inserted_slice_171[28, 12] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<32x32xf32>
    %extracted_slice_178 = tensor.extract_slice %inserted_slice_177[28, 16] [4, 4] [1, 1] : tensor<32x32xf32> to tensor<4x4xf32>
    %extracted_slice_179 = tensor.extract_slice %arg0[28, 0] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_180 = tensor.extract_slice %arg1[0, 16] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %60 = systolic.matmul_tile %extracted_slice_179, %extracted_slice_180, %extracted_slice_178 m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 810 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 30 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_181 = tensor.extract_slice %arg0[28, 16] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_182 = tensor.extract_slice %arg1[16, 16] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %61 = systolic.matmul_tile %extracted_slice_181, %extracted_slice_182, %60 m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 879 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 30 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_183 = tensor.insert_slice %61 into %inserted_slice_177[28, 16] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<32x32xf32>
    %extracted_slice_184 = tensor.extract_slice %inserted_slice_183[28, 20] [4, 4] [1, 1] : tensor<32x32xf32> to tensor<4x4xf32>
    %extracted_slice_185 = tensor.extract_slice %arg0[28, 0] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_186 = tensor.extract_slice %arg1[0, 20] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %62 = systolic.matmul_tile %extracted_slice_185, %extracted_slice_186, %extracted_slice_184 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 948 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 31 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_187 = tensor.extract_slice %arg0[28, 16] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_188 = tensor.extract_slice %arg1[16, 20] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %63 = systolic.matmul_tile %extracted_slice_187, %extracted_slice_188, %62 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 1017 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 31 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_189 = tensor.insert_slice %63 into %inserted_slice_183[28, 20] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<32x32xf32>
    %extracted_slice_190 = tensor.extract_slice %inserted_slice_189[28, 24] [4, 4] [1, 1] : tensor<32x32xf32> to tensor<4x4xf32>
    %extracted_slice_191 = tensor.extract_slice %arg0[28, 0] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_192 = tensor.extract_slice %arg1[0, 24] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %64 = systolic.matmul_tile %extracted_slice_191, %extracted_slice_192, %extracted_slice_190 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 948 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 32 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_193 = tensor.extract_slice %arg0[28, 16] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_194 = tensor.extract_slice %arg1[16, 24] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %65 = systolic.matmul_tile %extracted_slice_193, %extracted_slice_194, %64 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 1017 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 32 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_195 = tensor.insert_slice %65 into %inserted_slice_189[28, 24] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<32x32xf32>
    %extracted_slice_196 = tensor.extract_slice %inserted_slice_195[28, 28] [4, 4] [1, 1] : tensor<32x32xf32> to tensor<4x4xf32>
    %extracted_slice_197 = tensor.extract_slice %arg0[28, 0] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_198 = tensor.extract_slice %arg1[0, 28] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %66 = systolic.matmul_tile %extracted_slice_197, %extracted_slice_198, %extracted_slice_196 m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 948 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 33 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_199 = tensor.extract_slice %arg0[28, 16] [4, 16] [1, 1] : tensor<32x32xf32> to tensor<4x16xf32>
    %extracted_slice_200 = tensor.extract_slice %arg1[16, 28] [16, 4] [1, 1] : tensor<32x32xf32> to tensor<16x4xf32>
    %67 = systolic.matmul_tile %extracted_slice_199, %extracted_slice_200, %66 m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 1017 {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 33 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_201 = tensor.insert_slice %67 into %inserted_slice_195[28, 28] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<32x32xf32>
    return %inserted_slice_201 : tensor<32x32xf32>
  }
}

