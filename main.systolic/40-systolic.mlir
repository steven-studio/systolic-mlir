module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  func.func @gemm__systolic_kernel(%arg0: memref<?x16xf32> {llvm.noalias}, %arg1: memref<?x16xf32> {llvm.noalias}, %arg2: memref<?x16xf32> {llvm.noalias}) {
    %subview = memref.subview %arg0[0, 0] [16, 16] [1, 1] : memref<?x16xf32> to memref<16x16xf32, strided<[16, 1]>>
    %0 = bufferization.to_tensor %subview restrict : memref<16x16xf32, strided<[16, 1]>> to tensor<16x16xf32>
    %subview_0 = memref.subview %arg1[0, 0] [16, 16] [1, 1] : memref<?x16xf32> to memref<16x16xf32, strided<[16, 1]>>
    %1 = bufferization.to_tensor %subview_0 restrict : memref<16x16xf32, strided<[16, 1]>> to tensor<16x16xf32>
    %subview_1 = memref.subview %arg2[0, 0] [16, 16] [1, 1] : memref<?x16xf32> to memref<16x16xf32, strided<[16, 1]>>
    %2 = bufferization.to_tensor %subview_1 restrict writable : memref<16x16xf32, strided<[16, 1]>> to tensor<16x16xf32>
    %extracted_slice = tensor.extract_slice %2[0, 0] [8, 8] [1, 1] : tensor<16x16xf32> to tensor<8x8xf32>
    %extracted_slice_2 = tensor.extract_slice %0[0, 0] [8, 16] [1, 1] : tensor<16x16xf32> to tensor<8x16xf32>
    %extracted_slice_3 = tensor.extract_slice %1[0, 0] [16, 8] [1, 1] : tensor<16x16xf32> to tensor<16x8xf32>
    %3 = systolic.matmul_tile %extracted_slice_2, %extracted_slice_3, %extracted_slice m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 0 : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice = tensor.insert_slice %3 into %2[0, 0] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<16x16xf32>
    %extracted_slice_4 = tensor.extract_slice %inserted_slice[0, 8] [8, 8] [1, 1] : tensor<16x16xf32> to tensor<8x8xf32>
    %extracted_slice_5 = tensor.extract_slice %0[0, 0] [8, 16] [1, 1] : tensor<16x16xf32> to tensor<8x16xf32>
    %extracted_slice_6 = tensor.extract_slice %1[0, 8] [16, 8] [1, 1] : tensor<16x16xf32> to tensor<16x8xf32>
    %4 = systolic.matmul_tile %extracted_slice_5, %extracted_slice_6, %extracted_slice_4 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 125 : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice_7 = tensor.insert_slice %4 into %inserted_slice[0, 8] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<16x16xf32>
    %extracted_slice_8 = tensor.extract_slice %inserted_slice_7[8, 0] [4, 4] [1, 1] : tensor<16x16xf32> to tensor<4x4xf32>
    %extracted_slice_9 = tensor.extract_slice %0[8, 0] [4, 16] [1, 1] : tensor<16x16xf32> to tensor<4x16xf32>
    %extracted_slice_10 = tensor.extract_slice %1[0, 0] [16, 4] [1, 1] : tensor<16x16xf32> to tensor<16x4xf32>
    %5 = systolic.matmul_tile %extracted_slice_9, %extracted_slice_10, %extracted_slice_8 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 0 : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_11 = tensor.insert_slice %5 into %inserted_slice_7[8, 0] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<16x16xf32>
    %extracted_slice_12 = tensor.extract_slice %inserted_slice_11[8, 4] [4, 4] [1, 1] : tensor<16x16xf32> to tensor<4x4xf32>
    %extracted_slice_13 = tensor.extract_slice %0[8, 0] [4, 16] [1, 1] : tensor<16x16xf32> to tensor<4x16xf32>
    %extracted_slice_14 = tensor.extract_slice %1[0, 4] [16, 4] [1, 1] : tensor<16x16xf32> to tensor<16x4xf32>
    %6 = systolic.matmul_tile %extracted_slice_13, %extracted_slice_14, %extracted_slice_12 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 0 : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_15 = tensor.insert_slice %6 into %inserted_slice_11[8, 4] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<16x16xf32>
    %extracted_slice_16 = tensor.extract_slice %inserted_slice_15[8, 8] [4, 4] [1, 1] : tensor<16x16xf32> to tensor<4x4xf32>
    %extracted_slice_17 = tensor.extract_slice %0[8, 0] [4, 16] [1, 1] : tensor<16x16xf32> to tensor<4x16xf32>
    %extracted_slice_18 = tensor.extract_slice %1[0, 8] [16, 4] [1, 1] : tensor<16x16xf32> to tensor<16x4xf32>
    %7 = systolic.matmul_tile %extracted_slice_17, %extracted_slice_18, %extracted_slice_16 m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 0 : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_19 = tensor.insert_slice %7 into %inserted_slice_15[8, 8] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<16x16xf32>
    %extracted_slice_20 = tensor.extract_slice %inserted_slice_19[8, 12] [4, 4] [1, 1] : tensor<16x16xf32> to tensor<4x4xf32>
    %extracted_slice_21 = tensor.extract_slice %0[8, 0] [4, 16] [1, 1] : tensor<16x16xf32> to tensor<4x16xf32>
    %extracted_slice_22 = tensor.extract_slice %1[0, 12] [16, 4] [1, 1] : tensor<16x16xf32> to tensor<16x4xf32>
    %8 = systolic.matmul_tile %extracted_slice_21, %extracted_slice_22, %extracted_slice_20 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 117 : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_23 = tensor.insert_slice %8 into %inserted_slice_19[8, 12] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<16x16xf32>
    %extracted_slice_24 = tensor.extract_slice %inserted_slice_23[12, 0] [4, 4] [1, 1] : tensor<16x16xf32> to tensor<4x4xf32>
    %extracted_slice_25 = tensor.extract_slice %0[12, 0] [4, 16] [1, 1] : tensor<16x16xf32> to tensor<4x16xf32>
    %extracted_slice_26 = tensor.extract_slice %1[0, 0] [16, 4] [1, 1] : tensor<16x16xf32> to tensor<16x4xf32>
    %9 = systolic.matmul_tile %extracted_slice_25, %extracted_slice_26, %extracted_slice_24 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 117 : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_27 = tensor.insert_slice %9 into %inserted_slice_23[12, 0] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<16x16xf32>
    %extracted_slice_28 = tensor.extract_slice %inserted_slice_27[12, 4] [4, 4] [1, 1] : tensor<16x16xf32> to tensor<4x4xf32>
    %extracted_slice_29 = tensor.extract_slice %0[12, 0] [4, 16] [1, 1] : tensor<16x16xf32> to tensor<4x16xf32>
    %extracted_slice_30 = tensor.extract_slice %1[0, 4] [16, 4] [1, 1] : tensor<16x16xf32> to tensor<16x4xf32>
    %10 = systolic.matmul_tile %extracted_slice_29, %extracted_slice_30, %extracted_slice_28 m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 117 : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_31 = tensor.insert_slice %10 into %inserted_slice_27[12, 4] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<16x16xf32>
    %extracted_slice_32 = tensor.extract_slice %inserted_slice_31[12, 8] [4, 4] [1, 1] : tensor<16x16xf32> to tensor<4x4xf32>
    %extracted_slice_33 = tensor.extract_slice %0[12, 0] [4, 16] [1, 1] : tensor<16x16xf32> to tensor<4x16xf32>
    %extracted_slice_34 = tensor.extract_slice %1[0, 8] [16, 4] [1, 1] : tensor<16x16xf32> to tensor<16x4xf32>
    %11 = systolic.matmul_tile %extracted_slice_33, %extracted_slice_34, %extracted_slice_32 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 234 : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_35 = tensor.insert_slice %11 into %inserted_slice_31[12, 8] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<16x16xf32>
    %extracted_slice_36 = tensor.extract_slice %inserted_slice_35[12, 12] [4, 4] [1, 1] : tensor<16x16xf32> to tensor<4x4xf32>
    %extracted_slice_37 = tensor.extract_slice %0[12, 0] [4, 16] [1, 1] : tensor<16x16xf32> to tensor<4x16xf32>
    %extracted_slice_38 = tensor.extract_slice %1[0, 12] [16, 4] [1, 1] : tensor<16x16xf32> to tensor<16x4xf32>
    %12 = systolic.matmul_tile %extracted_slice_37, %extracted_slice_38, %extracted_slice_36 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 234 : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice_39 = tensor.insert_slice %12 into %inserted_slice_35[12, 12] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<16x16xf32>
    bufferization.materialize_in_destination %inserted_slice_39 in restrict writable %subview_1 : (tensor<16x16xf32>, memref<16x16xf32, strided<[16, 1]>>) -> ()
    return
  }
}

