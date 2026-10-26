#map = affine_map<(d0) -> (d0)>
#map1 = affine_map<(d0, d1) -> (d0)>
#map2 = affine_map<(d0, d1) -> (d1)>
#map3 = affine_map<(d0) -> (-d0 + 3)>
#map4 = affine_map<(d0) -> (d0 + 2)>
module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  func.func @bat__systolic_kernel(%arg0: memref<?x4x4xf32> {llvm.noalias}, %arg1: memref<?x4x4xf32> {llvm.noalias}, %arg2: memref<?x4x4xf32> {llvm.noalias}) {
    affine.for %arg3 = 0 to 2 {
      %0 = affine.apply #map(%arg3)
      %subview = memref.subview %arg0[%0, 0, 0] [1, 4, 4] [1, 1, 1] : memref<?x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %1 = bufferization.to_tensor %subview restrict : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
      %subview_0 = memref.subview %arg1[%0, 0, 0] [1, 4, 4] [1, 1, 1] : memref<?x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %2 = bufferization.to_tensor %subview_0 restrict : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
      %subview_1 = memref.subview %arg2[%0, 0, 0] [1, 4, 4] [1, 1, 1] : memref<?x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %3 = bufferization.to_tensor %subview_1 restrict writable : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
      %extracted_slice = tensor.extract_slice %3[0, 0] [4, 4] [1, 1] : tensor<4x4xf32> to tensor<4x4xf32>
      %extracted_slice_2 = tensor.extract_slice %1[0, 0] [4, 4] [1, 1] : tensor<4x4xf32> to tensor<4x4xf32>
      %extracted_slice_3 = tensor.extract_slice %2[0, 0] [4, 4] [1, 1] : tensor<4x4xf32> to tensor<4x4xf32>
      %4 = systolic.matmul_tile %extracted_slice_2, %extracted_slice_3, %extracted_slice m = 4 n = 4 k = 4 on @acc_4x4_0 est_cycles = 105 start_cycle = 0 {systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 0 : i64} : (tensor<4x4xf32>, tensor<4x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
      %inserted_slice = tensor.insert_slice %4 into %3[0, 0] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<4x4xf32>
      bufferization.materialize_in_destination %inserted_slice in restrict writable %subview_1 : (tensor<4x4xf32>, memref<4x4xf32, strided<[4, 1], offset: ?>>) -> ()
    }
    return
  }
  func.func @bat_mid__systolic_kernel(%arg0: memref<?x8x8xf32> {llvm.noalias}, %arg1: memref<?x8x8xf32> {llvm.noalias}, %arg2: memref<?x8x8xf32> {llvm.noalias}) {
    affine.for %arg3 = 0 to 3 {
      %0 = affine.apply #map(%arg3)
      %subview = memref.subview %arg0[%0, 0, 0] [1, 8, 8] [1, 1, 1] : memref<?x8x8xf32> to memref<8x8xf32, strided<[8, 1], offset: ?>>
      %1 = bufferization.to_tensor %subview restrict : memref<8x8xf32, strided<[8, 1], offset: ?>> to tensor<8x8xf32>
      %subview_0 = memref.subview %arg1[%0, 0, 0] [1, 8, 8] [1, 1, 1] : memref<?x8x8xf32> to memref<8x8xf32, strided<[8, 1], offset: ?>>
      %2 = bufferization.to_tensor %subview_0 restrict : memref<8x8xf32, strided<[8, 1], offset: ?>> to tensor<8x8xf32>
      %subview_1 = memref.subview %arg2[%0, 0, 0] [1, 8, 8] [1, 1, 1] : memref<?x8x8xf32> to memref<8x8xf32, strided<[8, 1], offset: ?>>
      %3 = bufferization.to_tensor %subview_1 restrict writable : memref<8x8xf32, strided<[8, 1], offset: ?>> to tensor<8x8xf32>
      %extracted_slice = tensor.extract_slice %3[0, 0] [8, 8] [1, 1] : tensor<8x8xf32> to tensor<8x8xf32>
      %extracted_slice_2 = tensor.extract_slice %1[0, 0] [8, 8] [1, 1] : tensor<8x8xf32> to tensor<8x8xf32>
      %extracted_slice_3 = tensor.extract_slice %2[0, 0] [8, 8] [1, 1] : tensor<8x8xf32> to tensor<8x8xf32>
      %4 = systolic.matmul_tile %extracted_slice_2, %extracted_slice_3, %extracted_slice m = 8 n = 8 k = 8 on @acc_8x8_0 est_cycles = 117 start_cycle = 0 {systolic.gemm = 1 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 0 : i64} : (tensor<8x8xf32>, tensor<8x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
      %inserted_slice = tensor.insert_slice %4 into %3[0, 0] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<8x8xf32>
      bufferization.materialize_in_destination %inserted_slice in restrict writable %subview_1 : (tensor<8x8xf32>, memref<8x8xf32, strided<[8, 1], offset: ?>>) -> ()
    }
    return
  }
  func.func @bat_step__systolic_kernel(%arg0: memref<?x4x4xf32> {llvm.noalias}, %arg1: memref<?x4x4xf32> {llvm.noalias}, %arg2: memref<?x4x4xf32> {llvm.noalias}) {
    affine.for %arg3 = 1 to 7 step 2 {
      %0 = affine.apply #map(%arg3)
      %subview = memref.subview %arg0[%0, 0, 0] [1, 4, 4] [1, 1, 1] : memref<?x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %1 = bufferization.to_tensor %subview restrict : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
      %subview_0 = memref.subview %arg1[%0, 0, 0] [1, 4, 4] [1, 1, 1] : memref<?x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %2 = bufferization.to_tensor %subview_0 restrict : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
      %subview_1 = memref.subview %arg2[%0, 0, 0] [1, 4, 4] [1, 1, 1] : memref<?x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %3 = bufferization.to_tensor %subview_1 restrict writable : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
      %extracted_slice = tensor.extract_slice %3[0, 0] [4, 4] [1, 1] : tensor<4x4xf32> to tensor<4x4xf32>
      %extracted_slice_2 = tensor.extract_slice %1[0, 0] [4, 4] [1, 1] : tensor<4x4xf32> to tensor<4x4xf32>
      %extracted_slice_3 = tensor.extract_slice %2[0, 0] [4, 4] [1, 1] : tensor<4x4xf32> to tensor<4x4xf32>
      %4 = systolic.matmul_tile %extracted_slice_2, %extracted_slice_3, %extracted_slice m = 4 n = 4 k = 4 on @acc_4x4_0 est_cycles = 105 start_cycle = 0 {systolic.gemm = 2 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 0 : i64} : (tensor<4x4xf32>, tensor<4x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
      %inserted_slice = tensor.insert_slice %4 into %3[0, 0] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<4x4xf32>
      bufferization.materialize_in_destination %inserted_slice in restrict writable %subview_1 : (tensor<4x4xf32>, memref<4x4xf32, strided<[4, 1], offset: ?>>) -> ()
    }
    return
  }
  func.func @bat_two__systolic_kernel(%arg0: memref<?x3x4x4xf32> {llvm.noalias}, %arg1: memref<?x3x4x4xf32> {llvm.noalias}, %arg2: memref<?x3x4x4xf32> {llvm.noalias}) {
    affine.for %arg3 = 0 to 2 {
      affine.for %arg4 = 0 to 3 {
        %0 = affine.apply #map1(%arg3, %arg4)
        %1 = affine.apply #map2(%arg3, %arg4)
        %subview = memref.subview %arg0[%0, %1, 0, 0] [1, 1, 4, 4] [1, 1, 1, 1] : memref<?x3x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
        %2 = bufferization.to_tensor %subview restrict : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
        %subview_0 = memref.subview %arg1[%0, %1, 0, 0] [1, 1, 4, 4] [1, 1, 1, 1] : memref<?x3x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
        %3 = bufferization.to_tensor %subview_0 restrict : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
        %subview_1 = memref.subview %arg2[%0, %1, 0, 0] [1, 1, 4, 4] [1, 1, 1, 1] : memref<?x3x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
        %4 = bufferization.to_tensor %subview_1 restrict writable : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
        %extracted_slice = tensor.extract_slice %4[0, 0] [4, 4] [1, 1] : tensor<4x4xf32> to tensor<4x4xf32>
        %extracted_slice_2 = tensor.extract_slice %2[0, 0] [4, 4] [1, 1] : tensor<4x4xf32> to tensor<4x4xf32>
        %extracted_slice_3 = tensor.extract_slice %3[0, 0] [4, 4] [1, 1] : tensor<4x4xf32> to tensor<4x4xf32>
        %5 = systolic.matmul_tile %extracted_slice_2, %extracted_slice_3, %extracted_slice m = 4 n = 4 k = 4 on @acc_4x4_0 est_cycles = 105 start_cycle = 0 {systolic.gemm = 3 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 0 : i64} : (tensor<4x4xf32>, tensor<4x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
        %inserted_slice = tensor.insert_slice %5 into %4[0, 0] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<4x4xf32>
        bufferization.materialize_in_destination %inserted_slice in restrict writable %subview_1 : (tensor<4x4xf32>, memref<4x4xf32, strided<[4, 1], offset: ?>>) -> ()
      }
    }
    return
  }
  func.func @bat_inner__systolic_kernel(%arg0: memref<?x2x4xf32> {llvm.noalias}, %arg1: memref<?x4x4xf32> {llvm.noalias}, %arg2: memref<?x4x4xf32> {llvm.noalias}) {
    affine.for %arg3 = 0 to 2 {
      %0 = affine.apply #map(%arg3)
      %subview = memref.subview %arg0[0, %0, 0] [4, 1, 4] [1, 1, 1] : memref<?x2x4xf32> to memref<4x4xf32, strided<[8, 1], offset: ?>>
      %1 = bufferization.to_tensor %subview restrict : memref<4x4xf32, strided<[8, 1], offset: ?>> to tensor<4x4xf32>
      %subview_0 = memref.subview %arg1[%0, 0, 0] [1, 4, 4] [1, 1, 1] : memref<?x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %2 = bufferization.to_tensor %subview_0 restrict : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
      %subview_1 = memref.subview %arg2[%0, 0, 0] [1, 4, 4] [1, 1, 1] : memref<?x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %3 = bufferization.to_tensor %subview_1 restrict writable : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
      %extracted_slice = tensor.extract_slice %3[0, 0] [4, 4] [1, 1] : tensor<4x4xf32> to tensor<4x4xf32>
      %extracted_slice_2 = tensor.extract_slice %1[0, 0] [4, 4] [1, 1] : tensor<4x4xf32> to tensor<4x4xf32>
      %extracted_slice_3 = tensor.extract_slice %2[0, 0] [4, 4] [1, 1] : tensor<4x4xf32> to tensor<4x4xf32>
      %4 = systolic.matmul_tile %extracted_slice_2, %extracted_slice_3, %extracted_slice m = 4 n = 4 k = 4 on @acc_4x4_0 est_cycles = 105 start_cycle = 0 {systolic.gemm = 4 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 0 : i64} : (tensor<4x4xf32>, tensor<4x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
      %inserted_slice = tensor.insert_slice %4 into %3[0, 0] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<4x4xf32>
      bufferization.materialize_in_destination %inserted_slice in restrict writable %subview_1 : (tensor<4x4xf32>, memref<4x4xf32, strided<[4, 1], offset: ?>>) -> ()
    }
    return
  }
  func.func @bat_tr_neg__systolic_kernel(%arg0: memref<?x8x8xf32> {llvm.noalias}, %arg1: memref<?x8x8xf32> {llvm.noalias}, %arg2: memref<?x8x8xf32> {llvm.noalias}) {
    affine.for %arg3 = 0 to 2 {
      %0 = affine.apply #map(%arg3)
      %subview = memref.subview %arg0[%0, 0, 0] [1, 8, 8] [1, 1, 1] : memref<?x8x8xf32> to memref<8x8xf32, strided<[8, 1], offset: ?>>
      %transpose = memref.transpose %subview (d0, d1) -> (d1, d0) : memref<8x8xf32, strided<[8, 1], offset: ?>> to memref<8x8xf32, strided<[1, 8], offset: ?>>
      %1 = bufferization.to_tensor %transpose restrict : memref<8x8xf32, strided<[1, 8], offset: ?>> to tensor<8x8xf32>
      %subview_0 = memref.subview %arg1[%0, 0, 0] [1, 8, 8] [1, 1, 1] : memref<?x8x8xf32> to memref<8x8xf32, strided<[8, 1], offset: ?>>
      %2 = bufferization.to_tensor %subview_0 restrict : memref<8x8xf32, strided<[8, 1], offset: ?>> to tensor<8x8xf32>
      %subview_1 = memref.subview %arg2[%0, 0, 7] [1, 8, 8] [1, 1, -1] : memref<?x8x8xf32> to memref<8x8xf32, strided<[8, -1], offset: ?>>
      %3 = bufferization.to_tensor %subview_1 restrict writable : memref<8x8xf32, strided<[8, -1], offset: ?>> to tensor<8x8xf32>
      %extracted_slice = tensor.extract_slice %3[0, 0] [8, 8] [1, 1] : tensor<8x8xf32> to tensor<8x8xf32>
      %extracted_slice_2 = tensor.extract_slice %1[0, 0] [8, 8] [1, 1] : tensor<8x8xf32> to tensor<8x8xf32>
      %extracted_slice_3 = tensor.extract_slice %2[0, 0] [8, 8] [1, 1] : tensor<8x8xf32> to tensor<8x8xf32>
      %4 = systolic.matmul_tile %extracted_slice_2, %extracted_slice_3, %extracted_slice m = 8 n = 8 k = 8 on @acc_8x8_0 est_cycles = 117 start_cycle = 0 {systolic.gemm = 5 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 0 : i64} : (tensor<8x8xf32>, tensor<8x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
      %inserted_slice = tensor.insert_slice %4 into %3[0, 0] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<8x8xf32>
      bufferization.materialize_in_destination %inserted_slice in restrict writable %subview_1 : (tensor<8x8xf32>, memref<8x8xf32, strided<[8, -1], offset: ?>>) -> ()
    }
    return
  }
  func.func @bat_down__systolic_kernel(%arg0: memref<?x4x4xf32> {llvm.noalias}, %arg1: memref<?x4x4xf32> {llvm.noalias}, %arg2: memref<?x4x4xf32> {llvm.noalias}) {
    affine.for %arg3 = 0 to 4 {
      %0 = affine.apply #map3(%arg3)
      %subview = memref.subview %arg0[%0, 0, 0] [1, 4, 4] [1, 1, 1] : memref<?x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %1 = bufferization.to_tensor %subview restrict : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
      %subview_0 = memref.subview %arg1[%0, 0, 0] [1, 4, 4] [1, 1, 1] : memref<?x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %2 = bufferization.to_tensor %subview_0 restrict : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
      %subview_1 = memref.subview %arg2[%0, 0, 0] [1, 4, 4] [1, 1, 1] : memref<?x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %3 = bufferization.to_tensor %subview_1 restrict writable : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
      %extracted_slice = tensor.extract_slice %3[0, 0] [4, 4] [1, 1] : tensor<4x4xf32> to tensor<4x4xf32>
      %extracted_slice_2 = tensor.extract_slice %1[0, 0] [4, 4] [1, 1] : tensor<4x4xf32> to tensor<4x4xf32>
      %extracted_slice_3 = tensor.extract_slice %2[0, 0] [4, 4] [1, 1] : tensor<4x4xf32> to tensor<4x4xf32>
      %4 = systolic.matmul_tile %extracted_slice_2, %extracted_slice_3, %extracted_slice m = 4 n = 4 k = 4 on @acc_4x4_0 est_cycles = 105 start_cycle = 0 {systolic.gemm = 6 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 0 : i64} : (tensor<4x4xf32>, tensor<4x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
      %inserted_slice = tensor.insert_slice %4 into %3[0, 0] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<4x4xf32>
      bufferization.materialize_in_destination %inserted_slice in restrict writable %subview_1 : (tensor<4x4xf32>, memref<4x4xf32, strided<[4, 1], offset: ?>>) -> ()
    }
    return
  }
  func.func @bat_offset__systolic_kernel(%arg0: memref<?x4x4xf32> {llvm.noalias}, %arg1: memref<?x4x4xf32> {llvm.noalias}, %arg2: memref<?x4x4xf32> {llvm.noalias}) {
    affine.for %arg3 = 0 to 3 {
      %0 = affine.apply #map4(%arg3)
      %subview = memref.subview %arg0[%0, 0, 0] [1, 4, 4] [1, 1, 1] : memref<?x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %1 = bufferization.to_tensor %subview restrict : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
      %subview_0 = memref.subview %arg1[%0, 0, 0] [1, 4, 4] [1, 1, 1] : memref<?x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %2 = bufferization.to_tensor %subview_0 restrict : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
      %subview_1 = memref.subview %arg2[%0, 0, 0] [1, 4, 4] [1, 1, 1] : memref<?x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %3 = bufferization.to_tensor %subview_1 restrict writable : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
      %extracted_slice = tensor.extract_slice %3[0, 0] [4, 4] [1, 1] : tensor<4x4xf32> to tensor<4x4xf32>
      %extracted_slice_2 = tensor.extract_slice %1[0, 0] [4, 4] [1, 1] : tensor<4x4xf32> to tensor<4x4xf32>
      %extracted_slice_3 = tensor.extract_slice %2[0, 0] [4, 4] [1, 1] : tensor<4x4xf32> to tensor<4x4xf32>
      %4 = systolic.matmul_tile %extracted_slice_2, %extracted_slice_3, %extracted_slice m = 4 n = 4 k = 4 on @acc_4x4_0 est_cycles = 105 start_cycle = 0 {systolic.gemm = 7 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 0 : i64} : (tensor<4x4xf32>, tensor<4x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
      %inserted_slice = tensor.insert_slice %4 into %3[0, 0] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<4x4xf32>
      bufferization.materialize_in_destination %inserted_slice in restrict writable %subview_1 : (tensor<4x4xf32>, memref<4x4xf32, strided<[4, 1], offset: ?>>) -> ()
    }
    return
  }
  func.func @bat_tiles__systolic_kernel(%arg0: memref<?x16x16xf32> {llvm.noalias}, %arg1: memref<?x16x16xf32> {llvm.noalias}, %arg2: memref<?x16x16xf32> {llvm.noalias}) {
    affine.for %arg3 = 0 to 2 {
      %0 = affine.apply #map(%arg3)
      %subview = memref.subview %arg0[%0, 0, 0] [1, 16, 16] [1, 1, 1] : memref<?x16x16xf32> to memref<16x16xf32, strided<[16, 1], offset: ?>>
      %1 = bufferization.to_tensor %subview restrict : memref<16x16xf32, strided<[16, 1], offset: ?>> to tensor<16x16xf32>
      %subview_0 = memref.subview %arg1[%0, 0, 0] [1, 16, 16] [1, 1, 1] : memref<?x16x16xf32> to memref<16x16xf32, strided<[16, 1], offset: ?>>
      %2 = bufferization.to_tensor %subview_0 restrict : memref<16x16xf32, strided<[16, 1], offset: ?>> to tensor<16x16xf32>
      %subview_1 = memref.subview %arg2[%0, 0, 0] [1, 16, 16] [1, 1, 1] : memref<?x16x16xf32> to memref<16x16xf32, strided<[16, 1], offset: ?>>
      %3 = bufferization.to_tensor %subview_1 restrict writable : memref<16x16xf32, strided<[16, 1], offset: ?>> to tensor<16x16xf32>
      %extracted_slice = tensor.extract_slice %3[0, 0] [8, 8] [1, 1] : tensor<16x16xf32> to tensor<8x8xf32>
      %extracted_slice_2 = tensor.extract_slice %1[0, 0] [8, 16] [1, 1] : tensor<16x16xf32> to tensor<8x16xf32>
      %extracted_slice_3 = tensor.extract_slice %2[0, 0] [16, 8] [1, 1] : tensor<16x16xf32> to tensor<16x8xf32>
      %4 = systolic.matmul_tile %extracted_slice_2, %extracted_slice_3, %extracted_slice m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 0 {systolic.gemm = 8 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 0 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
      %inserted_slice = tensor.insert_slice %4 into %3[0, 0] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<16x16xf32>
      %extracted_slice_4 = tensor.extract_slice %inserted_slice[0, 8] [8, 8] [1, 1] : tensor<16x16xf32> to tensor<8x8xf32>
      %extracted_slice_5 = tensor.extract_slice %1[0, 0] [8, 16] [1, 1] : tensor<16x16xf32> to tensor<8x16xf32>
      %extracted_slice_6 = tensor.extract_slice %2[0, 8] [16, 8] [1, 1] : tensor<16x16xf32> to tensor<16x8xf32>
      %5 = systolic.matmul_tile %extracted_slice_5, %extracted_slice_6, %extracted_slice_4 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 42 {systolic.gemm = 8 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 1 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
      %inserted_slice_7 = tensor.insert_slice %5 into %inserted_slice[0, 8] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<16x16xf32>
      %extracted_slice_8 = tensor.extract_slice %inserted_slice_7[8, 0] [8, 8] [1, 1] : tensor<16x16xf32> to tensor<8x8xf32>
      %extracted_slice_9 = tensor.extract_slice %1[8, 0] [8, 16] [1, 1] : tensor<16x16xf32> to tensor<8x16xf32>
      %extracted_slice_10 = tensor.extract_slice %2[0, 0] [16, 8] [1, 1] : tensor<16x16xf32> to tensor<16x8xf32>
      %6 = systolic.matmul_tile %extracted_slice_9, %extracted_slice_10, %extracted_slice_8 m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 128 {systolic.gemm = 8 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 2 : i64} : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
      %inserted_slice_11 = tensor.insert_slice %6 into %inserted_slice_7[8, 0] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<16x16xf32>
      %extracted_slice_12 = tensor.extract_slice %inserted_slice_11[8, 8] [4, 4] [1, 1] : tensor<16x16xf32> to tensor<4x4xf32>
      %extracted_slice_13 = tensor.extract_slice %1[8, 0] [4, 16] [1, 1] : tensor<16x16xf32> to tensor<4x16xf32>
      %extracted_slice_14 = tensor.extract_slice %2[0, 8] [16, 4] [1, 1] : tensor<16x16xf32> to tensor<16x4xf32>
      %7 = systolic.matmul_tile %extracted_slice_13, %extracted_slice_14, %extracted_slice_12 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 0 {systolic.gemm = 8 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 3 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
      %inserted_slice_15 = tensor.insert_slice %7 into %inserted_slice_11[8, 8] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<16x16xf32>
      %extracted_slice_16 = tensor.extract_slice %inserted_slice_15[8, 12] [4, 4] [1, 1] : tensor<16x16xf32> to tensor<4x4xf32>
      %extracted_slice_17 = tensor.extract_slice %1[8, 0] [4, 16] [1, 1] : tensor<16x16xf32> to tensor<4x16xf32>
      %extracted_slice_18 = tensor.extract_slice %2[0, 12] [16, 4] [1, 1] : tensor<16x16xf32> to tensor<16x4xf32>
      %8 = systolic.matmul_tile %extracted_slice_17, %extracted_slice_18, %extracted_slice_16 m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 0 {systolic.gemm = 8 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 4 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
      %inserted_slice_19 = tensor.insert_slice %8 into %inserted_slice_15[8, 12] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<16x16xf32>
      %extracted_slice_20 = tensor.extract_slice %inserted_slice_19[12, 8] [4, 4] [1, 1] : tensor<16x16xf32> to tensor<4x4xf32>
      %extracted_slice_21 = tensor.extract_slice %1[12, 0] [4, 16] [1, 1] : tensor<16x16xf32> to tensor<4x16xf32>
      %extracted_slice_22 = tensor.extract_slice %2[0, 8] [16, 4] [1, 1] : tensor<16x16xf32> to tensor<16x4xf32>
      %9 = systolic.matmul_tile %extracted_slice_21, %extracted_slice_22, %extracted_slice_20 m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 0 {systolic.gemm = 8 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 5 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
      %inserted_slice_23 = tensor.insert_slice %9 into %inserted_slice_19[12, 8] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<16x16xf32>
      %extracted_slice_24 = tensor.extract_slice %inserted_slice_23[12, 12] [4, 4] [1, 1] : tensor<16x16xf32> to tensor<4x4xf32>
      %extracted_slice_25 = tensor.extract_slice %1[12, 0] [4, 16] [1, 1] : tensor<16x16xf32> to tensor<4x16xf32>
      %extracted_slice_26 = tensor.extract_slice %2[0, 12] [16, 4] [1, 1] : tensor<16x16xf32> to tensor<16x4xf32>
      %10 = systolic.matmul_tile %extracted_slice_25, %extracted_slice_26, %extracted_slice_24 m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 42 {systolic.gemm = 8 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 6 : i64} : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
      %inserted_slice_27 = tensor.insert_slice %10 into %inserted_slice_23[12, 12] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<16x16xf32>
      bufferization.materialize_in_destination %inserted_slice_27 in restrict writable %subview_1 : (tensor<16x16xf32>, memref<16x16xf32, strided<[16, 1], offset: ?>>) -> ()
    }
    return
  }
}

