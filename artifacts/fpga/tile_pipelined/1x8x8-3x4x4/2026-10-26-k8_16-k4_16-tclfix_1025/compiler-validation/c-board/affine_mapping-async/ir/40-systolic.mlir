module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  func.func @expressions__systolic_kernel(%arg0: memref<?x10xf32> {llvm.noalias}, %arg1: memref<?x4xf32> {llvm.noalias}, %arg2: memref<?x8xf32> {llvm.noalias}) {
    %subview = memref.subview %arg0[1, 0] [4, 4] [2, 3] : memref<?x10xf32> to memref<4x4xf32, strided<[20, 3], offset: 10>>
    %0 = bufferization.to_tensor %subview restrict : memref<4x4xf32, strided<[20, 3], offset: 10>> to tensor<4x4xf32>
    %subview_0 = memref.subview %arg1[2, 0] [4, 4] [6, 1] : memref<?x4xf32> to memref<4x4xf32, strided<[24, 1], offset: 8>>
    %1 = bufferization.to_tensor %subview_0 restrict : memref<4x4xf32, strided<[24, 1], offset: 8>> to tensor<4x4xf32>
    %subview_1 = memref.subview %arg2[0, 1] [4, 4] [4, 2] : memref<?x8xf32> to memref<4x4xf32, strided<[32, 2], offset: 1>>
    %2 = bufferization.to_tensor %subview_1 restrict writable : memref<4x4xf32, strided<[32, 2], offset: 1>> to tensor<4x4xf32>
    %extracted_slice = tensor.extract_slice %2[0, 0] [4, 4] [1, 1] : tensor<4x4xf32> to tensor<4x4xf32>
    %extracted_slice_2 = tensor.extract_slice %0[0, 0] [4, 4] [1, 1] : tensor<4x4xf32> to tensor<4x4xf32>
    %extracted_slice_3 = tensor.extract_slice %1[0, 0] [4, 4] [1, 1] : tensor<4x4xf32> to tensor<4x4xf32>
    %3 = systolic.matmul_tile %extracted_slice_2, %extracted_slice_3, %extracted_slice m = 4 n = 4 k = 4 on @acc_4x4_0 est_cycles = 105 start_cycle = 0 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 0 : i64} : (tensor<4x4xf32>, tensor<4x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %inserted_slice = tensor.insert_slice %3 into %2[0, 0] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<4x4xf32>
    bufferization.materialize_in_destination %inserted_slice in restrict writable %subview_1 : (tensor<4x4xf32>, memref<4x4xf32, strided<[32, 2], offset: 1>>) -> ()
    return
  }
  func.func @runs_once__systolic_kernel(%arg0: memref<?x8xf32> {llvm.noalias}, %arg1: memref<?x10xf32> {llvm.noalias}, %arg2: memref<?x11xf32> {llvm.noalias}) {
    %subview = memref.subview %arg0[6, 0] [8, 8] [1, 1] : memref<?x8xf32> to memref<8x8xf32, strided<[8, 1], offset: 48>>
    %0 = bufferization.to_tensor %subview restrict : memref<8x8xf32, strided<[8, 1], offset: 48>> to tensor<8x8xf32>
    %subview_0 = memref.subview %arg1[0, 2] [8, 8] [1, 1] : memref<?x10xf32> to memref<8x8xf32, strided<[10, 1], offset: 2>>
    %1 = bufferization.to_tensor %subview_0 restrict : memref<8x8xf32, strided<[10, 1], offset: 2>> to tensor<8x8xf32>
    %subview_1 = memref.subview %arg2[3, 3] [8, 8] [1, 1] : memref<?x11xf32> to memref<8x8xf32, strided<[11, 1], offset: 36>>
    %2 = bufferization.to_tensor %subview_1 restrict writable : memref<8x8xf32, strided<[11, 1], offset: 36>> to tensor<8x8xf32>
    %extracted_slice = tensor.extract_slice %2[0, 0] [8, 8] [1, 1] : tensor<8x8xf32> to tensor<8x8xf32>
    %extracted_slice_2 = tensor.extract_slice %0[0, 0] [8, 8] [1, 1] : tensor<8x8xf32> to tensor<8x8xf32>
    %extracted_slice_3 = tensor.extract_slice %1[0, 0] [8, 8] [1, 1] : tensor<8x8xf32> to tensor<8x8xf32>
    %3 = systolic.matmul_tile %extracted_slice_2, %extracted_slice_3, %extracted_slice m = 8 n = 8 k = 8 on @acc_8x8_0 est_cycles = 117 start_cycle = 0 {systolic.gemm = 1 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 0 : i64} : (tensor<8x8xf32>, tensor<8x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice = tensor.insert_slice %3 into %2[0, 0] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<8x8xf32>
    bufferization.materialize_in_destination %inserted_slice in restrict writable %subview_1 : (tensor<8x8xf32>, memref<8x8xf32, strided<[11, 1], offset: 36>>) -> ()
    return
  }
  func.func @steps__systolic_kernel(%arg0: memref<?x15xf32> {llvm.noalias}, %arg1: memref<?x11xf32> {llvm.noalias}, %arg2: memref<?x11xf32> {llvm.noalias}) {
    %subview = memref.subview %arg0[1, 0] [3, 8] [2, 2] : memref<?x15xf32> to memref<3x8xf32, strided<[30, 2], offset: 15>>
    %0 = bufferization.to_tensor %subview restrict : memref<3x8xf32, strided<[30, 2], offset: 15>> to tensor<3x8xf32>
    %subview_0 = memref.subview %arg1[0, 0] [8, 11] [2, 1] : memref<?x11xf32> to memref<8x11xf32, strided<[22, 1]>>
    %1 = bufferization.to_tensor %subview_0 restrict : memref<8x11xf32, strided<[22, 1]>> to tensor<8x11xf32>
    %subview_1 = memref.subview %arg2[1, 0] [3, 11] [2, 1] : memref<?x11xf32> to memref<3x11xf32, strided<[22, 1], offset: 11>>
    %2 = bufferization.to_tensor %subview_1 restrict writable : memref<3x11xf32, strided<[22, 1], offset: 11>> to tensor<3x11xf32>
    %extracted_slice = tensor.extract_slice %2[0, 0] [3, 4] [1, 1] : tensor<3x11xf32> to tensor<3x4xf32>
    %extracted_slice_2 = tensor.extract_slice %0[0, 0] [3, 8] [1, 1] : tensor<3x8xf32> to tensor<3x8xf32>
    %extracted_slice_3 = tensor.extract_slice %1[0, 0] [8, 4] [1, 1] : tensor<8x11xf32> to tensor<8x4xf32>
    %3 = systolic.matmul_tile %extracted_slice_2, %extracted_slice_3, %extracted_slice m = 3 n = 4 k = 8 on @acc_4x4_0 est_cycles = 109 start_cycle = 0 {systolic.gemm = 2 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 0 : i64} : (tensor<3x8xf32>, tensor<8x4xf32>, tensor<3x4xf32>) -> tensor<3x4xf32>
    %inserted_slice = tensor.insert_slice %3 into %2[0, 0] [3, 4] [1, 1] : tensor<3x4xf32> into tensor<3x11xf32>
    %extracted_slice_4 = tensor.extract_slice %inserted_slice[0, 4] [3, 4] [1, 1] : tensor<3x11xf32> to tensor<3x4xf32>
    %extracted_slice_5 = tensor.extract_slice %0[0, 0] [3, 8] [1, 1] : tensor<3x8xf32> to tensor<3x8xf32>
    %extracted_slice_6 = tensor.extract_slice %1[0, 4] [8, 4] [1, 1] : tensor<8x11xf32> to tensor<8x4xf32>
    %4 = systolic.matmul_tile %extracted_slice_5, %extracted_slice_6, %extracted_slice_4 m = 3 n = 4 k = 8 on @acc_4x4_1 est_cycles = 109 start_cycle = 0 {systolic.gemm = 2 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 1 : i64} : (tensor<3x8xf32>, tensor<8x4xf32>, tensor<3x4xf32>) -> tensor<3x4xf32>
    %inserted_slice_7 = tensor.insert_slice %4 into %inserted_slice[0, 4] [3, 4] [1, 1] : tensor<3x4xf32> into tensor<3x11xf32>
    %extracted_slice_8 = tensor.extract_slice %inserted_slice_7[0, 8] [3, 3] [1, 1] : tensor<3x11xf32> to tensor<3x3xf32>
    %extracted_slice_9 = tensor.extract_slice %0[0, 0] [3, 8] [1, 1] : tensor<3x8xf32> to tensor<3x8xf32>
    %extracted_slice_10 = tensor.extract_slice %1[0, 8] [8, 3] [1, 1] : tensor<8x11xf32> to tensor<8x3xf32>
    %5 = systolic.matmul_tile %extracted_slice_9, %extracted_slice_10, %extracted_slice_8 m = 3 n = 3 k = 8 on @acc_4x4_2 est_cycles = 109 start_cycle = 0 {systolic.gemm = 2 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 2 : i64} : (tensor<3x8xf32>, tensor<8x3xf32>, tensor<3x3xf32>) -> tensor<3x3xf32>
    %inserted_slice_11 = tensor.insert_slice %5 into %inserted_slice_7[0, 8] [3, 3] [1, 1] : tensor<3x3xf32> into tensor<3x11xf32>
    bufferization.materialize_in_destination %inserted_slice_11 in restrict writable %subview_1 : (tensor<3x11xf32>, memref<3x11xf32, strided<[22, 1], offset: 11>>) -> ()
    return
  }
  func.func @split__systolic_kernel(%arg0: memref<?x8xf32> {llvm.noalias}, %arg1: memref<?x8xf32> {llvm.noalias}, %arg2: memref<?x8xf32> {llvm.noalias}) {
    %subview = memref.subview %arg0[1, 0] [8, 8] [2, 1] : memref<?x8xf32> to memref<8x8xf32, strided<[16, 1], offset: 8>>
    %0 = bufferization.to_tensor %subview restrict : memref<8x8xf32, strided<[16, 1], offset: 8>> to tensor<8x8xf32>
    %subview_0 = memref.subview %arg1[0, 0] [8, 8] [1, 1] : memref<?x8xf32> to memref<8x8xf32, strided<[8, 1]>>
    %1 = bufferization.to_tensor %subview_0 restrict : memref<8x8xf32, strided<[8, 1]>> to tensor<8x8xf32>
    %subview_1 = memref.subview %arg2[5, 0] [8, 8] [4, 1] : memref<?x8xf32> to memref<8x8xf32, strided<[32, 1], offset: 40>>
    %2 = bufferization.to_tensor %subview_1 restrict writable : memref<8x8xf32, strided<[32, 1], offset: 40>> to tensor<8x8xf32>
    %extracted_slice = tensor.extract_slice %2[0, 0] [8, 8] [1, 1] : tensor<8x8xf32> to tensor<8x8xf32>
    %extracted_slice_2 = tensor.extract_slice %0[0, 0] [8, 8] [1, 1] : tensor<8x8xf32> to tensor<8x8xf32>
    %extracted_slice_3 = tensor.extract_slice %1[0, 0] [8, 8] [1, 1] : tensor<8x8xf32> to tensor<8x8xf32>
    %3 = systolic.matmul_tile %extracted_slice_2, %extracted_slice_3, %extracted_slice m = 8 n = 8 k = 8 on @acc_8x8_0 est_cycles = 117 start_cycle = 0 {systolic.gemm = 3 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 0 : i64} : (tensor<8x8xf32>, tensor<8x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    %inserted_slice = tensor.insert_slice %3 into %2[0, 0] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<8x8xf32>
    bufferization.materialize_in_destination %inserted_slice in restrict writable %subview_1 : (tensor<8x8xf32>, memref<8x8xf32, strided<[32, 1], offset: 40>>) -> ()
    return
  }
  func.func @near_max__systolic_kernel(%arg0: memref<?x8xf32> {llvm.noalias}, %arg1: memref<?x8xf32> {llvm.noalias}, %arg2: memref<?x8xf32> {llvm.noalias}) {
    %subview = memref.subview %arg0[0, 0] [7, 8] [1, 1] : memref<?x8xf32> to memref<7x8xf32, strided<[8, 1]>>
    %0 = bufferization.to_tensor %subview restrict : memref<7x8xf32, strided<[8, 1]>> to tensor<7x8xf32>
    %subview_0 = memref.subview %arg1[0, 0] [8, 8] [1, 1] : memref<?x8xf32> to memref<8x8xf32, strided<[8, 1]>>
    %1 = bufferization.to_tensor %subview_0 restrict : memref<8x8xf32, strided<[8, 1]>> to tensor<8x8xf32>
    %subview_1 = memref.subview %arg2[0, 0] [7, 8] [1, 1] : memref<?x8xf32> to memref<7x8xf32, strided<[8, 1]>>
    %2 = bufferization.to_tensor %subview_1 restrict writable : memref<7x8xf32, strided<[8, 1]>> to tensor<7x8xf32>
    %extracted_slice = tensor.extract_slice %2[0, 0] [7, 8] [1, 1] : tensor<7x8xf32> to tensor<7x8xf32>
    %extracted_slice_2 = tensor.extract_slice %0[0, 0] [7, 8] [1, 1] : tensor<7x8xf32> to tensor<7x8xf32>
    %extracted_slice_3 = tensor.extract_slice %1[0, 0] [8, 8] [1, 1] : tensor<8x8xf32> to tensor<8x8xf32>
    %3 = systolic.matmul_tile %extracted_slice_2, %extracted_slice_3, %extracted_slice m = 7 n = 8 k = 8 on @acc_8x8_0 est_cycles = 117 start_cycle = 0 {systolic.gemm = 4 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 0 : i64} : (tensor<7x8xf32>, tensor<8x8xf32>, tensor<7x8xf32>) -> tensor<7x8xf32>
    %inserted_slice = tensor.insert_slice %3 into %2[0, 0] [7, 8] [1, 1] : tensor<7x8xf32> into tensor<7x8xf32>
    bufferization.materialize_in_destination %inserted_slice in restrict writable %subview_1 : (tensor<7x8xf32>, memref<7x8xf32, strided<[8, 1]>>) -> ()
    return
  }
}

