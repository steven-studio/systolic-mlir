module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  func.func @gemm__systolic_kernel(%arg0: memref<?x40xf32> {llvm.noalias}, %arg1: memref<?x20xf32> {llvm.noalias}, %arg2: memref<?x20xf32> {llvm.noalias}) {
    affine.for %arg3 = 0 to 24 {
      affine.for %arg4 = 0 to 20 {
        %cst = arith.constant 0.000000e+00 : f32
        affine.store %cst, %arg2[%arg3, %arg4] : memref<?x20xf32>
      }
    }
    %subview = memref.subview %arg0[0, 0] [24, 40] [1, 1] : memref<?x40xf32> to memref<24x40xf32, strided<[40, 1]>>
    %0 = bufferization.to_tensor %subview restrict : memref<24x40xf32, strided<[40, 1]>> to tensor<24x40xf32>
    %subview_0 = memref.subview %arg1[0, 0] [40, 20] [1, 1] : memref<?x20xf32> to memref<40x20xf32, strided<[20, 1]>>
    %1 = bufferization.to_tensor %subview_0 restrict : memref<40x20xf32, strided<[20, 1]>> to tensor<40x20xf32>
    %subview_1 = memref.subview %arg2[0, 0] [24, 20] [1, 1] : memref<?x20xf32> to memref<24x20xf32, strided<[20, 1]>>
    %2 = bufferization.to_tensor %subview_1 restrict writable : memref<24x20xf32, strided<[20, 1]>> to tensor<24x20xf32>
    %extracted_slice = tensor.extract_slice %2[0, 0] [8, 8] [1, 1] : tensor<24x20xf32> to tensor<8x8xf32>
    %extracted_slice_2 = tensor.extract_slice %0[0, 0] [8, 16] [1, 1] : tensor<24x40xf32> to tensor<8x16xf32>
    %extracted_slice_3 = tensor.extract_slice %1[0, 0] [16, 8] [1, 1] : tensor<40x20xf32> to tensor<16x8xf32>
    %3 = systolic.matmul_tile %extracted_slice_2, %extracted_slice_3, %extracted_slice m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 0 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 0 : i64, systolic.tile = 0 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %extracted_slice_4 = tensor.extract_slice %0[0, 16] [8, 16] [1, 1] : tensor<24x40xf32> to tensor<8x16xf32>
    %extracted_slice_5 = tensor.extract_slice %1[16, 0] [16, 8] [1, 1] : tensor<40x20xf32> to tensor<16x8xf32>
    %4 = systolic.matmul_tile %extracted_slice_4, %extracted_slice_5, %3 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 125 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 1 : i64, systolic.tile = 0 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %extracted_slice_6 = tensor.extract_slice %0[0, 32] [8, 8] [1, 1] : tensor<24x40xf32> to tensor<8x8xf32>
    %extracted_slice_7 = tensor.extract_slice %1[32, 0] [8, 8] [1, 1] : tensor<40x20xf32> to tensor<8x8xf32>
    %5 = systolic.matmul_tile %extracted_slice_6, %extracted_slice_7, %4 m = 8 n = 8 k = 8 on @acc_8x8_0 est_cycles = 117 start_cycle = 250 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 2 : i64, systolic.tile = 0 : i64} : (tensor<8x8xf32>, tensor<8x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice = tensor.insert_slice %5 into %2[0, 0] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<24x20xf32>
    %extracted_slice_8 = tensor.extract_slice %inserted_slice[0, 8] [8, 8] [1, 1] : tensor<24x20xf32> to tensor<8x8xf32>
    %extracted_slice_9 = tensor.extract_slice %0[0, 0] [8, 16] [1, 1] : tensor<24x40xf32> to tensor<8x16xf32>
    %extracted_slice_10 = tensor.extract_slice %1[0, 8] [16, 8] [1, 1] : tensor<40x20xf32> to tensor<16x8xf32>
    %6 = systolic.matmul_tile %extracted_slice_9, %extracted_slice_10, %extracted_slice_8 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 367 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 0 : i64, systolic.tile = 1 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %extracted_slice_11 = tensor.extract_slice %0[0, 16] [8, 16] [1, 1] : tensor<24x40xf32> to tensor<8x16xf32>
    %extracted_slice_12 = tensor.extract_slice %1[16, 8] [16, 8] [1, 1] : tensor<40x20xf32> to tensor<16x8xf32>
    %7 = systolic.matmul_tile %extracted_slice_11, %extracted_slice_12, %6 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 492 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 1 : i64, systolic.tile = 1 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %extracted_slice_13 = tensor.extract_slice %0[0, 32] [8, 8] [1, 1] : tensor<24x40xf32> to tensor<8x8xf32>
    %extracted_slice_14 = tensor.extract_slice %1[32, 8] [8, 8] [1, 1] : tensor<40x20xf32> to tensor<8x8xf32>
    %8 = systolic.matmul_tile %extracted_slice_13, %extracted_slice_14, %7 m = 8 n = 8 k = 8 on @acc_8x8_0 est_cycles = 117 start_cycle = 617 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 2 : i64, systolic.tile = 1 : i64} : (tensor<8x8xf32>, tensor<8x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice_15 = tensor.insert_slice %8 into %inserted_slice[0, 8] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<24x20xf32>
    %extracted_slice_16 = tensor.extract_slice %inserted_slice_15[0, 16] [4, 4] [1, 1] : tensor<24x20xf32> to tensor<4x4xf32>
    %extracted_slice_17 = tensor.extract_slice %0[0, 0] [4, 16] [1, 1] : tensor<24x40xf32> to tensor<4x16xf32>
    %extracted_slice_18 = tensor.extract_slice %1[0, 16] [16, 4] [1, 1] : tensor<40x20xf32> to tensor<16x4xf32>
    %9 = systolic.matmul_tile %extracted_slice_17, %extracted_slice_18, %extracted_slice_16 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 0 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 0 : i64, systolic.tile = 2 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_19 = tensor.extract_slice %0[0, 16] [4, 16] [1, 1] : tensor<24x40xf32> to tensor<4x16xf32>
    %extracted_slice_20 = tensor.extract_slice %1[16, 16] [16, 4] [1, 1] : tensor<40x20xf32> to tensor<16x4xf32>
    %10 = systolic.matmul_tile %extracted_slice_19, %extracted_slice_20, %9 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 117 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 1 : i64, systolic.tile = 2 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_21 = tensor.extract_slice %0[0, 32] [4, 8] [1, 1] : tensor<24x40xf32> to tensor<4x8xf32>
    %extracted_slice_22 = tensor.extract_slice %1[32, 16] [8, 4] [1, 1] : tensor<40x20xf32> to tensor<8x4xf32>
    %11 = systolic.matmul_tile %extracted_slice_21, %extracted_slice_22, %10 m = 4 n = 4 k = 8 on @acc_4x4_0 est_cycles = 109 start_cycle = 234 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 2 : i64, systolic.tile = 2 : i64} : (tensor<4x8xf32>, tensor<8x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_23 = tensor.insert_slice %11 into %inserted_slice_15[0, 16] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<24x20xf32>
    %extracted_slice_24 = tensor.extract_slice %inserted_slice_23[4, 16] [4, 4] [1, 1] : tensor<24x20xf32> to tensor<4x4xf32>
    %extracted_slice_25 = tensor.extract_slice %0[4, 0] [4, 16] [1, 1] : tensor<24x40xf32> to tensor<4x16xf32>
    %extracted_slice_26 = tensor.extract_slice %1[0, 16] [16, 4] [1, 1] : tensor<40x20xf32> to tensor<16x4xf32>
    %12 = systolic.matmul_tile %extracted_slice_25, %extracted_slice_26, %extracted_slice_24 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 0 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 0 : i64, systolic.tile = 3 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_27 = tensor.extract_slice %0[4, 16] [4, 16] [1, 1] : tensor<24x40xf32> to tensor<4x16xf32>
    %extracted_slice_28 = tensor.extract_slice %1[16, 16] [16, 4] [1, 1] : tensor<40x20xf32> to tensor<16x4xf32>
    %13 = systolic.matmul_tile %extracted_slice_27, %extracted_slice_28, %12 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 117 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 1 : i64, systolic.tile = 3 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_29 = tensor.extract_slice %0[4, 32] [4, 8] [1, 1] : tensor<24x40xf32> to tensor<4x8xf32>
    %extracted_slice_30 = tensor.extract_slice %1[32, 16] [8, 4] [1, 1] : tensor<40x20xf32> to tensor<8x4xf32>
    %14 = systolic.matmul_tile %extracted_slice_29, %extracted_slice_30, %13 m = 4 n = 4 k = 8 on @acc_4x4_1 est_cycles = 109 start_cycle = 234 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 2 : i64, systolic.tile = 3 : i64} : (tensor<4x8xf32>, tensor<8x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_31 = tensor.insert_slice %14 into %inserted_slice_23[4, 16] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<24x20xf32>
    %extracted_slice_32 = tensor.extract_slice %inserted_slice_31[8, 0] [8, 8] [1, 1] : tensor<24x20xf32> to tensor<8x8xf32>
    %extracted_slice_33 = tensor.extract_slice %0[8, 0] [8, 16] [1, 1] : tensor<24x40xf32> to tensor<8x16xf32>
    %extracted_slice_34 = tensor.extract_slice %1[0, 0] [16, 8] [1, 1] : tensor<40x20xf32> to tensor<16x8xf32>
    %15 = systolic.matmul_tile %extracted_slice_33, %extracted_slice_34, %extracted_slice_32 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 734 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 0 : i64, systolic.tile = 4 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %extracted_slice_35 = tensor.extract_slice %0[8, 16] [8, 16] [1, 1] : tensor<24x40xf32> to tensor<8x16xf32>
    %extracted_slice_36 = tensor.extract_slice %1[16, 0] [16, 8] [1, 1] : tensor<40x20xf32> to tensor<16x8xf32>
    %16 = systolic.matmul_tile %extracted_slice_35, %extracted_slice_36, %15 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 859 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 1 : i64, systolic.tile = 4 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %extracted_slice_37 = tensor.extract_slice %0[8, 32] [8, 8] [1, 1] : tensor<24x40xf32> to tensor<8x8xf32>
    %extracted_slice_38 = tensor.extract_slice %1[32, 0] [8, 8] [1, 1] : tensor<40x20xf32> to tensor<8x8xf32>
    %17 = systolic.matmul_tile %extracted_slice_37, %extracted_slice_38, %16 m = 8 n = 8 k = 8 on @acc_8x8_0 est_cycles = 117 start_cycle = 984 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 2 : i64, systolic.tile = 4 : i64} : (tensor<8x8xf32>, tensor<8x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice_39 = tensor.insert_slice %17 into %inserted_slice_31[8, 0] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<24x20xf32>
    %extracted_slice_40 = tensor.extract_slice %inserted_slice_39[8, 8] [8, 8] [1, 1] : tensor<24x20xf32> to tensor<8x8xf32>
    %extracted_slice_41 = tensor.extract_slice %0[8, 0] [8, 16] [1, 1] : tensor<24x40xf32> to tensor<8x16xf32>
    %extracted_slice_42 = tensor.extract_slice %1[0, 8] [16, 8] [1, 1] : tensor<40x20xf32> to tensor<16x8xf32>
    %18 = systolic.matmul_tile %extracted_slice_41, %extracted_slice_42, %extracted_slice_40 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 1101 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 0 : i64, systolic.tile = 5 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %extracted_slice_43 = tensor.extract_slice %0[8, 16] [8, 16] [1, 1] : tensor<24x40xf32> to tensor<8x16xf32>
    %extracted_slice_44 = tensor.extract_slice %1[16, 8] [16, 8] [1, 1] : tensor<40x20xf32> to tensor<16x8xf32>
    %19 = systolic.matmul_tile %extracted_slice_43, %extracted_slice_44, %18 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 1226 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 1 : i64, systolic.tile = 5 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %extracted_slice_45 = tensor.extract_slice %0[8, 32] [8, 8] [1, 1] : tensor<24x40xf32> to tensor<8x8xf32>
    %extracted_slice_46 = tensor.extract_slice %1[32, 8] [8, 8] [1, 1] : tensor<40x20xf32> to tensor<8x8xf32>
    %20 = systolic.matmul_tile %extracted_slice_45, %extracted_slice_46, %19 m = 8 n = 8 k = 8 on @acc_8x8_0 est_cycles = 117 start_cycle = 1351 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 2 : i64, systolic.tile = 5 : i64} : (tensor<8x8xf32>, tensor<8x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice_47 = tensor.insert_slice %20 into %inserted_slice_39[8, 8] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<24x20xf32>
    %extracted_slice_48 = tensor.extract_slice %inserted_slice_47[8, 16] [4, 4] [1, 1] : tensor<24x20xf32> to tensor<4x4xf32>
    %extracted_slice_49 = tensor.extract_slice %0[8, 0] [4, 16] [1, 1] : tensor<24x40xf32> to tensor<4x16xf32>
    %extracted_slice_50 = tensor.extract_slice %1[0, 16] [16, 4] [1, 1] : tensor<40x20xf32> to tensor<16x4xf32>
    %21 = systolic.matmul_tile %extracted_slice_49, %extracted_slice_50, %extracted_slice_48 m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 0 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 0 : i64, systolic.tile = 6 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_51 = tensor.extract_slice %0[8, 16] [4, 16] [1, 1] : tensor<24x40xf32> to tensor<4x16xf32>
    %extracted_slice_52 = tensor.extract_slice %1[16, 16] [16, 4] [1, 1] : tensor<40x20xf32> to tensor<16x4xf32>
    %22 = systolic.matmul_tile %extracted_slice_51, %extracted_slice_52, %21 m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 117 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 1 : i64, systolic.tile = 6 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_53 = tensor.extract_slice %0[8, 32] [4, 8] [1, 1] : tensor<24x40xf32> to tensor<4x8xf32>
    %extracted_slice_54 = tensor.extract_slice %1[32, 16] [8, 4] [1, 1] : tensor<40x20xf32> to tensor<8x4xf32>
    %23 = systolic.matmul_tile %extracted_slice_53, %extracted_slice_54, %22 m = 4 n = 4 k = 8 on @acc_4x4_2 est_cycles = 109 start_cycle = 234 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 2 : i64, systolic.tile = 6 : i64} : (tensor<4x8xf32>, tensor<8x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_55 = tensor.insert_slice %23 into %inserted_slice_47[8, 16] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<24x20xf32>
    %extracted_slice_56 = tensor.extract_slice %inserted_slice_55[12, 16] [4, 4] [1, 1] : tensor<24x20xf32> to tensor<4x4xf32>
    %extracted_slice_57 = tensor.extract_slice %0[12, 0] [4, 16] [1, 1] : tensor<24x40xf32> to tensor<4x16xf32>
    %extracted_slice_58 = tensor.extract_slice %1[0, 16] [16, 4] [1, 1] : tensor<40x20xf32> to tensor<16x4xf32>
    %24 = systolic.matmul_tile %extracted_slice_57, %extracted_slice_58, %extracted_slice_56 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 343 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 0 : i64, systolic.tile = 7 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_59 = tensor.extract_slice %0[12, 16] [4, 16] [1, 1] : tensor<24x40xf32> to tensor<4x16xf32>
    %extracted_slice_60 = tensor.extract_slice %1[16, 16] [16, 4] [1, 1] : tensor<40x20xf32> to tensor<16x4xf32>
    %25 = systolic.matmul_tile %extracted_slice_59, %extracted_slice_60, %24 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 460 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 1 : i64, systolic.tile = 7 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_61 = tensor.extract_slice %0[12, 32] [4, 8] [1, 1] : tensor<24x40xf32> to tensor<4x8xf32>
    %extracted_slice_62 = tensor.extract_slice %1[32, 16] [8, 4] [1, 1] : tensor<40x20xf32> to tensor<8x4xf32>
    %26 = systolic.matmul_tile %extracted_slice_61, %extracted_slice_62, %25 m = 4 n = 4 k = 8 on @acc_4x4_0 est_cycles = 109 start_cycle = 577 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 2 : i64, systolic.tile = 7 : i64} : (tensor<4x8xf32>, tensor<8x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_63 = tensor.insert_slice %26 into %inserted_slice_55[12, 16] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<24x20xf32>
    %extracted_slice_64 = tensor.extract_slice %inserted_slice_63[16, 0] [8, 8] [1, 1] : tensor<24x20xf32> to tensor<8x8xf32>
    %extracted_slice_65 = tensor.extract_slice %0[16, 0] [8, 16] [1, 1] : tensor<24x40xf32> to tensor<8x16xf32>
    %extracted_slice_66 = tensor.extract_slice %1[0, 0] [16, 8] [1, 1] : tensor<40x20xf32> to tensor<16x8xf32>
    %27 = systolic.matmul_tile %extracted_slice_65, %extracted_slice_66, %extracted_slice_64 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 1468 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 0 : i64, systolic.tile = 8 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %extracted_slice_67 = tensor.extract_slice %0[16, 16] [8, 16] [1, 1] : tensor<24x40xf32> to tensor<8x16xf32>
    %extracted_slice_68 = tensor.extract_slice %1[16, 0] [16, 8] [1, 1] : tensor<40x20xf32> to tensor<16x8xf32>
    %28 = systolic.matmul_tile %extracted_slice_67, %extracted_slice_68, %27 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 1593 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 1 : i64, systolic.tile = 8 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %extracted_slice_69 = tensor.extract_slice %0[16, 32] [8, 8] [1, 1] : tensor<24x40xf32> to tensor<8x8xf32>
    %extracted_slice_70 = tensor.extract_slice %1[32, 0] [8, 8] [1, 1] : tensor<40x20xf32> to tensor<8x8xf32>
    %29 = systolic.matmul_tile %extracted_slice_69, %extracted_slice_70, %28 m = 8 n = 8 k = 8 on @acc_8x8_0 est_cycles = 117 start_cycle = 1718 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 2 : i64, systolic.tile = 8 : i64} : (tensor<8x8xf32>, tensor<8x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice_71 = tensor.insert_slice %29 into %inserted_slice_63[16, 0] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<24x20xf32>
    %extracted_slice_72 = tensor.extract_slice %inserted_slice_71[16, 8] [4, 4] [1, 1] : tensor<24x20xf32> to tensor<4x4xf32>
    %extracted_slice_73 = tensor.extract_slice %0[16, 0] [4, 16] [1, 1] : tensor<24x40xf32> to tensor<4x16xf32>
    %extracted_slice_74 = tensor.extract_slice %1[0, 8] [16, 4] [1, 1] : tensor<40x20xf32> to tensor<16x4xf32>
    %30 = systolic.matmul_tile %extracted_slice_73, %extracted_slice_74, %extracted_slice_72 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 343 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 0 : i64, systolic.tile = 9 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_75 = tensor.extract_slice %0[16, 16] [4, 16] [1, 1] : tensor<24x40xf32> to tensor<4x16xf32>
    %extracted_slice_76 = tensor.extract_slice %1[16, 8] [16, 4] [1, 1] : tensor<40x20xf32> to tensor<16x4xf32>
    %31 = systolic.matmul_tile %extracted_slice_75, %extracted_slice_76, %30 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 460 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 1 : i64, systolic.tile = 9 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_77 = tensor.extract_slice %0[16, 32] [4, 8] [1, 1] : tensor<24x40xf32> to tensor<4x8xf32>
    %extracted_slice_78 = tensor.extract_slice %1[32, 8] [8, 4] [1, 1] : tensor<40x20xf32> to tensor<8x4xf32>
    %32 = systolic.matmul_tile %extracted_slice_77, %extracted_slice_78, %31 m = 4 n = 4 k = 8 on @acc_4x4_1 est_cycles = 109 start_cycle = 577 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 2 : i64, systolic.tile = 9 : i64} : (tensor<4x8xf32>, tensor<8x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_79 = tensor.insert_slice %32 into %inserted_slice_71[16, 8] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<24x20xf32>
    %extracted_slice_80 = tensor.extract_slice %inserted_slice_79[16, 12] [4, 4] [1, 1] : tensor<24x20xf32> to tensor<4x4xf32>
    %extracted_slice_81 = tensor.extract_slice %0[16, 0] [4, 16] [1, 1] : tensor<24x40xf32> to tensor<4x16xf32>
    %extracted_slice_82 = tensor.extract_slice %1[0, 12] [16, 4] [1, 1] : tensor<40x20xf32> to tensor<16x4xf32>
    %33 = systolic.matmul_tile %extracted_slice_81, %extracted_slice_82, %extracted_slice_80 m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 343 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 0 : i64, systolic.tile = 10 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_83 = tensor.extract_slice %0[16, 16] [4, 16] [1, 1] : tensor<24x40xf32> to tensor<4x16xf32>
    %extracted_slice_84 = tensor.extract_slice %1[16, 12] [16, 4] [1, 1] : tensor<40x20xf32> to tensor<16x4xf32>
    %34 = systolic.matmul_tile %extracted_slice_83, %extracted_slice_84, %33 m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 460 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 1 : i64, systolic.tile = 10 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_85 = tensor.extract_slice %0[16, 32] [4, 8] [1, 1] : tensor<24x40xf32> to tensor<4x8xf32>
    %extracted_slice_86 = tensor.extract_slice %1[32, 12] [8, 4] [1, 1] : tensor<40x20xf32> to tensor<8x4xf32>
    %35 = systolic.matmul_tile %extracted_slice_85, %extracted_slice_86, %34 m = 4 n = 4 k = 8 on @acc_4x4_2 est_cycles = 109 start_cycle = 577 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 2 : i64, systolic.tile = 10 : i64} : (tensor<4x8xf32>, tensor<8x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_87 = tensor.insert_slice %35 into %inserted_slice_79[16, 12] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<24x20xf32>
    %extracted_slice_88 = tensor.extract_slice %inserted_slice_87[16, 16] [4, 4] [1, 1] : tensor<24x20xf32> to tensor<4x4xf32>
    %extracted_slice_89 = tensor.extract_slice %0[16, 0] [4, 16] [1, 1] : tensor<24x40xf32> to tensor<4x16xf32>
    %extracted_slice_90 = tensor.extract_slice %1[0, 16] [16, 4] [1, 1] : tensor<40x20xf32> to tensor<16x4xf32>
    %36 = systolic.matmul_tile %extracted_slice_89, %extracted_slice_90, %extracted_slice_88 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 686 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 0 : i64, systolic.tile = 11 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_91 = tensor.extract_slice %0[16, 16] [4, 16] [1, 1] : tensor<24x40xf32> to tensor<4x16xf32>
    %extracted_slice_92 = tensor.extract_slice %1[16, 16] [16, 4] [1, 1] : tensor<40x20xf32> to tensor<16x4xf32>
    %37 = systolic.matmul_tile %extracted_slice_91, %extracted_slice_92, %36 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 803 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 1 : i64, systolic.tile = 11 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_93 = tensor.extract_slice %0[16, 32] [4, 8] [1, 1] : tensor<24x40xf32> to tensor<4x8xf32>
    %extracted_slice_94 = tensor.extract_slice %1[32, 16] [8, 4] [1, 1] : tensor<40x20xf32> to tensor<8x4xf32>
    %38 = systolic.matmul_tile %extracted_slice_93, %extracted_slice_94, %37 m = 4 n = 4 k = 8 on @acc_4x4_0 est_cycles = 109 start_cycle = 920 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 2 : i64, systolic.tile = 11 : i64} : (tensor<4x8xf32>, tensor<8x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_95 = tensor.insert_slice %38 into %inserted_slice_87[16, 16] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<24x20xf32>
    %extracted_slice_96 = tensor.extract_slice %inserted_slice_95[20, 8] [4, 4] [1, 1] : tensor<24x20xf32> to tensor<4x4xf32>
    %extracted_slice_97 = tensor.extract_slice %0[20, 0] [4, 16] [1, 1] : tensor<24x40xf32> to tensor<4x16xf32>
    %extracted_slice_98 = tensor.extract_slice %1[0, 8] [16, 4] [1, 1] : tensor<40x20xf32> to tensor<16x4xf32>
    %39 = systolic.matmul_tile %extracted_slice_97, %extracted_slice_98, %extracted_slice_96 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 686 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 0 : i64, systolic.tile = 12 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_99 = tensor.extract_slice %0[20, 16] [4, 16] [1, 1] : tensor<24x40xf32> to tensor<4x16xf32>
    %extracted_slice_100 = tensor.extract_slice %1[16, 8] [16, 4] [1, 1] : tensor<40x20xf32> to tensor<16x4xf32>
    %40 = systolic.matmul_tile %extracted_slice_99, %extracted_slice_100, %39 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 803 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 1 : i64, systolic.tile = 12 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_101 = tensor.extract_slice %0[20, 32] [4, 8] [1, 1] : tensor<24x40xf32> to tensor<4x8xf32>
    %extracted_slice_102 = tensor.extract_slice %1[32, 8] [8, 4] [1, 1] : tensor<40x20xf32> to tensor<8x4xf32>
    %41 = systolic.matmul_tile %extracted_slice_101, %extracted_slice_102, %40 m = 4 n = 4 k = 8 on @acc_4x4_1 est_cycles = 109 start_cycle = 920 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 2 : i64, systolic.tile = 12 : i64} : (tensor<4x8xf32>, tensor<8x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_103 = tensor.insert_slice %41 into %inserted_slice_95[20, 8] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<24x20xf32>
    %extracted_slice_104 = tensor.extract_slice %inserted_slice_103[20, 12] [4, 4] [1, 1] : tensor<24x20xf32> to tensor<4x4xf32>
    %extracted_slice_105 = tensor.extract_slice %0[20, 0] [4, 16] [1, 1] : tensor<24x40xf32> to tensor<4x16xf32>
    %extracted_slice_106 = tensor.extract_slice %1[0, 12] [16, 4] [1, 1] : tensor<40x20xf32> to tensor<16x4xf32>
    %42 = systolic.matmul_tile %extracted_slice_105, %extracted_slice_106, %extracted_slice_104 m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 686 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 0 : i64, systolic.tile = 13 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_107 = tensor.extract_slice %0[20, 16] [4, 16] [1, 1] : tensor<24x40xf32> to tensor<4x16xf32>
    %extracted_slice_108 = tensor.extract_slice %1[16, 12] [16, 4] [1, 1] : tensor<40x20xf32> to tensor<16x4xf32>
    %43 = systolic.matmul_tile %extracted_slice_107, %extracted_slice_108, %42 m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 803 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 1 : i64, systolic.tile = 13 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_109 = tensor.extract_slice %0[20, 32] [4, 8] [1, 1] : tensor<24x40xf32> to tensor<4x8xf32>
    %extracted_slice_110 = tensor.extract_slice %1[32, 12] [8, 4] [1, 1] : tensor<40x20xf32> to tensor<8x4xf32>
    %44 = systolic.matmul_tile %extracted_slice_109, %extracted_slice_110, %43 m = 4 n = 4 k = 8 on @acc_4x4_2 est_cycles = 109 start_cycle = 920 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 2 : i64, systolic.tile = 13 : i64} : (tensor<4x8xf32>, tensor<8x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_111 = tensor.insert_slice %44 into %inserted_slice_103[20, 12] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<24x20xf32>
    %extracted_slice_112 = tensor.extract_slice %inserted_slice_111[20, 16] [4, 4] [1, 1] : tensor<24x20xf32> to tensor<4x4xf32>
    %extracted_slice_113 = tensor.extract_slice %0[20, 0] [4, 16] [1, 1] : tensor<24x40xf32> to tensor<4x16xf32>
    %extracted_slice_114 = tensor.extract_slice %1[0, 16] [16, 4] [1, 1] : tensor<40x20xf32> to tensor<16x4xf32>
    %45 = systolic.matmul_tile %extracted_slice_113, %extracted_slice_114, %extracted_slice_112 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 1029 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 0 : i64, systolic.tile = 14 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_115 = tensor.extract_slice %0[20, 16] [4, 16] [1, 1] : tensor<24x40xf32> to tensor<4x16xf32>
    %extracted_slice_116 = tensor.extract_slice %1[16, 16] [16, 4] [1, 1] : tensor<40x20xf32> to tensor<16x4xf32>
    %46 = systolic.matmul_tile %extracted_slice_115, %extracted_slice_116, %45 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 1146 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 1 : i64, systolic.tile = 14 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %extracted_slice_117 = tensor.extract_slice %0[20, 32] [4, 8] [1, 1] : tensor<24x40xf32> to tensor<4x8xf32>
    %extracted_slice_118 = tensor.extract_slice %1[32, 16] [8, 4] [1, 1] : tensor<40x20xf32> to tensor<8x4xf32>
    %47 = systolic.matmul_tile %extracted_slice_117, %extracted_slice_118, %46 m = 4 n = 4 k = 8 on @acc_4x4_0 est_cycles = 109 start_cycle = 1263 {systolic.gemm = 0 : i64, systolic.k_count = 3 : i64, systolic.k_index = 2 : i64, systolic.tile = 14 : i64} : (tensor<4x8xf32>, tensor<8x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_119 = tensor.insert_slice %47 into %inserted_slice_111[20, 16] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<24x20xf32>
    bufferization.materialize_in_destination %inserted_slice_119 in restrict writable %subview_1 : (tensor<24x20xf32>, memref<24x20xf32, strided<[20, 1]>>) -> ()
    return
  }
}

