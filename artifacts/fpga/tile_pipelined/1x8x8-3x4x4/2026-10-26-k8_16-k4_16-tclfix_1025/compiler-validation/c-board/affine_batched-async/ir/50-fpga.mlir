#map = affine_map<(d0) -> (d0)>
#map1 = affine_map<(d0, d1) -> (d0)>
#map2 = affine_map<(d0, d1) -> (d1)>
#map3 = affine_map<(d0) -> (-d0 + 3)>
#map4 = affine_map<(d0) -> (d0 + 2)>
module {
  llvm.func @fpga_gemm_wait(!llvm.ptr) -> i32
  llvm.func @fpga_tile_submit(!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
  llvm.func @fpga_gemm_begin(i32, i32, i32, i32) -> !llvm.ptr
  func.func @bat__systolic_kernel(%arg0: memref<?x4x4xf32> {llvm.noalias}, %arg1: memref<?x4x4xf32> {llvm.noalias}, %arg2: memref<?x4x4xf32> {llvm.noalias}) {
    %alloca = memref.alloca() : memref<128xf32>
    %alloca_0 = memref.alloca() : memref<128xf32>
    affine.for %arg3 = 0 to 2 {
      %0 = affine.apply #map(%arg3)
      %subview = memref.subview %arg0[%0, 0, 0] [1, 4, 4] [1, 1, 1] : memref<?x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %1 = bufferization.to_tensor %subview restrict : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
      %subview_1 = memref.subview %arg1[%0, 0, 0] [1, 4, 4] [1, 1, 1] : memref<?x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %2 = bufferization.to_tensor %subview_1 restrict : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
      %subview_2 = memref.subview %arg2[%0, 0, 0] [1, 4, 4] [1, 1, 1] : memref<?x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %3 = bufferization.to_tensor %subview_2 restrict writable : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
      %base_buffer, %offset, %sizes:2, %strides:2 = memref.extract_strided_metadata %subview_2 : memref<4x4xf32, strided<[4, 1], offset: ?>> -> memref<f32>, index, index, index, index, index
      %intptr = memref.extract_aligned_pointer_as_index %base_buffer : memref<f32> -> index
      %c4 = arith.constant 4 : index
      %4 = arith.muli %offset, %c4 : index
      %5 = arith.addi %intptr, %4 : index
      %6 = arith.index_cast %5 : index to i64
      %7 = llvm.inttoptr %6 : i64 to !llvm.ptr
      %8 = arith.index_cast %strides#0 : index to i32
      %c4_i32 = arith.constant 4 : i32
      %c4_i32_3 = arith.constant 4 : i32
      %c4_i32_4 = arith.constant 4 : i32
      %c1_i32 = arith.constant 1 : i32
      %9 = llvm.call @fpga_gemm_begin(%c4_i32, %c4_i32_3, %c4_i32_4, %c1_i32) : (i32, i32, i32, i32) -> !llvm.ptr
      %subview_5 = memref.subview %subview[0, 0] [4, 4] [1, 1] : memref<4x4xf32, strided<[4, 1], offset: ?>> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %reinterpret_cast = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<128xf32> to memref<4x4xf32>
      memref.copy %subview_5, %reinterpret_cast : memref<4x4xf32, strided<[4, 1], offset: ?>> to memref<4x4xf32>
      %intptr_6 = memref.extract_aligned_pointer_as_index %reinterpret_cast : memref<4x4xf32> -> index
      %10 = arith.index_cast %intptr_6 : index to i64
      %11 = llvm.inttoptr %10 : i64 to !llvm.ptr
      %subview_7 = memref.subview %subview_1[0, 0] [4, 4] [1, 1] : memref<4x4xf32, strided<[4, 1], offset: ?>> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %reinterpret_cast_8 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<128xf32> to memref<4x4xf32>
      memref.copy %subview_7, %reinterpret_cast_8 : memref<4x4xf32, strided<[4, 1], offset: ?>> to memref<4x4xf32>
      %intptr_9 = memref.extract_aligned_pointer_as_index %reinterpret_cast_8 : memref<4x4xf32> -> index
      %12 = arith.index_cast %intptr_9 : index to i64
      %13 = llvm.inttoptr %12 : i64 to !llvm.ptr
      %c0_i32 = arith.constant 0 : i32
      %c0_i32_10 = arith.constant 0 : i32
      %c1_i32_11 = arith.constant 1 : i32
      %c4_i32_12 = arith.constant 4 : i32
      %c4_i32_13 = arith.constant 4 : i32
      %c4_i32_14 = arith.constant 4 : i32
      %c1_i32_15 = arith.constant 1 : i32
      %c0_i32_16 = arith.constant 0 : i32
      %c105_i32 = arith.constant 105 : i32
      %14 = llvm.call @fpga_tile_submit(%9, %c0_i32, %c0_i32_10, %c1_i32_11, %c4_i32_12, %c4_i32_13, %c4_i32_14, %11, %13, %7, %8, %c1_i32_15, %c0_i32_16, %c105_i32) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
      %15 = bufferization.to_tensor %subview_2 restrict writable : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
      %16 = llvm.call @fpga_gemm_wait(%9) : (!llvm.ptr) -> i32
      bufferization.materialize_in_destination %15 in restrict writable %subview_2 : (tensor<4x4xf32>, memref<4x4xf32, strided<[4, 1], offset: ?>>) -> ()
    }
    return
  }
  func.func @bat_mid__systolic_kernel(%arg0: memref<?x8x8xf32> {llvm.noalias}, %arg1: memref<?x8x8xf32> {llvm.noalias}, %arg2: memref<?x8x8xf32> {llvm.noalias}) {
    %alloca = memref.alloca() : memref<128xf32>
    %alloca_0 = memref.alloca() : memref<128xf32>
    affine.for %arg3 = 0 to 3 {
      %0 = affine.apply #map(%arg3)
      %subview = memref.subview %arg0[%0, 0, 0] [1, 8, 8] [1, 1, 1] : memref<?x8x8xf32> to memref<8x8xf32, strided<[8, 1], offset: ?>>
      %1 = bufferization.to_tensor %subview restrict : memref<8x8xf32, strided<[8, 1], offset: ?>> to tensor<8x8xf32>
      %subview_1 = memref.subview %arg1[%0, 0, 0] [1, 8, 8] [1, 1, 1] : memref<?x8x8xf32> to memref<8x8xf32, strided<[8, 1], offset: ?>>
      %2 = bufferization.to_tensor %subview_1 restrict : memref<8x8xf32, strided<[8, 1], offset: ?>> to tensor<8x8xf32>
      %subview_2 = memref.subview %arg2[%0, 0, 0] [1, 8, 8] [1, 1, 1] : memref<?x8x8xf32> to memref<8x8xf32, strided<[8, 1], offset: ?>>
      %3 = bufferization.to_tensor %subview_2 restrict writable : memref<8x8xf32, strided<[8, 1], offset: ?>> to tensor<8x8xf32>
      %base_buffer, %offset, %sizes:2, %strides:2 = memref.extract_strided_metadata %subview_2 : memref<8x8xf32, strided<[8, 1], offset: ?>> -> memref<f32>, index, index, index, index, index
      %intptr = memref.extract_aligned_pointer_as_index %base_buffer : memref<f32> -> index
      %c4 = arith.constant 4 : index
      %4 = arith.muli %offset, %c4 : index
      %5 = arith.addi %intptr, %4 : index
      %6 = arith.index_cast %5 : index to i64
      %7 = llvm.inttoptr %6 : i64 to !llvm.ptr
      %8 = arith.index_cast %strides#0 : index to i32
      %c8_i32 = arith.constant 8 : i32
      %c8_i32_3 = arith.constant 8 : i32
      %c8_i32_4 = arith.constant 8 : i32
      %c1_i32 = arith.constant 1 : i32
      %9 = llvm.call @fpga_gemm_begin(%c8_i32, %c8_i32_3, %c8_i32_4, %c1_i32) : (i32, i32, i32, i32) -> !llvm.ptr
      %subview_5 = memref.subview %subview[0, 0] [8, 8] [1, 1] : memref<8x8xf32, strided<[8, 1], offset: ?>> to memref<8x8xf32, strided<[8, 1], offset: ?>>
      %reinterpret_cast = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 8], strides: [8, 1] : memref<128xf32> to memref<8x8xf32>
      memref.copy %subview_5, %reinterpret_cast : memref<8x8xf32, strided<[8, 1], offset: ?>> to memref<8x8xf32>
      %intptr_6 = memref.extract_aligned_pointer_as_index %reinterpret_cast : memref<8x8xf32> -> index
      %10 = arith.index_cast %intptr_6 : index to i64
      %11 = llvm.inttoptr %10 : i64 to !llvm.ptr
      %subview_7 = memref.subview %subview_1[0, 0] [8, 8] [1, 1] : memref<8x8xf32, strided<[8, 1], offset: ?>> to memref<8x8xf32, strided<[8, 1], offset: ?>>
      %reinterpret_cast_8 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [8, 8], strides: [8, 1] : memref<128xf32> to memref<8x8xf32>
      memref.copy %subview_7, %reinterpret_cast_8 : memref<8x8xf32, strided<[8, 1], offset: ?>> to memref<8x8xf32>
      %intptr_9 = memref.extract_aligned_pointer_as_index %reinterpret_cast_8 : memref<8x8xf32> -> index
      %12 = arith.index_cast %intptr_9 : index to i64
      %13 = llvm.inttoptr %12 : i64 to !llvm.ptr
      %c0_i32 = arith.constant 0 : i32
      %c0_i32_10 = arith.constant 0 : i32
      %c1_i32_11 = arith.constant 1 : i32
      %c8_i32_12 = arith.constant 8 : i32
      %c8_i32_13 = arith.constant 8 : i32
      %c8_i32_14 = arith.constant 8 : i32
      %c0_i32_15 = arith.constant 0 : i32
      %c0_i32_16 = arith.constant 0 : i32
      %c117_i32 = arith.constant 117 : i32
      %14 = llvm.call @fpga_tile_submit(%9, %c0_i32, %c0_i32_10, %c1_i32_11, %c8_i32_12, %c8_i32_13, %c8_i32_14, %11, %13, %7, %8, %c0_i32_15, %c0_i32_16, %c117_i32) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
      %15 = bufferization.to_tensor %subview_2 restrict writable : memref<8x8xf32, strided<[8, 1], offset: ?>> to tensor<8x8xf32>
      %16 = llvm.call @fpga_gemm_wait(%9) : (!llvm.ptr) -> i32
      bufferization.materialize_in_destination %15 in restrict writable %subview_2 : (tensor<8x8xf32>, memref<8x8xf32, strided<[8, 1], offset: ?>>) -> ()
    }
    return
  }
  func.func @bat_step__systolic_kernel(%arg0: memref<?x4x4xf32> {llvm.noalias}, %arg1: memref<?x4x4xf32> {llvm.noalias}, %arg2: memref<?x4x4xf32> {llvm.noalias}) {
    %alloca = memref.alloca() : memref<128xf32>
    %alloca_0 = memref.alloca() : memref<128xf32>
    affine.for %arg3 = 1 to 7 step 2 {
      %0 = affine.apply #map(%arg3)
      %subview = memref.subview %arg0[%0, 0, 0] [1, 4, 4] [1, 1, 1] : memref<?x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %1 = bufferization.to_tensor %subview restrict : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
      %subview_1 = memref.subview %arg1[%0, 0, 0] [1, 4, 4] [1, 1, 1] : memref<?x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %2 = bufferization.to_tensor %subview_1 restrict : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
      %subview_2 = memref.subview %arg2[%0, 0, 0] [1, 4, 4] [1, 1, 1] : memref<?x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %3 = bufferization.to_tensor %subview_2 restrict writable : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
      %base_buffer, %offset, %sizes:2, %strides:2 = memref.extract_strided_metadata %subview_2 : memref<4x4xf32, strided<[4, 1], offset: ?>> -> memref<f32>, index, index, index, index, index
      %intptr = memref.extract_aligned_pointer_as_index %base_buffer : memref<f32> -> index
      %c4 = arith.constant 4 : index
      %4 = arith.muli %offset, %c4 : index
      %5 = arith.addi %intptr, %4 : index
      %6 = arith.index_cast %5 : index to i64
      %7 = llvm.inttoptr %6 : i64 to !llvm.ptr
      %8 = arith.index_cast %strides#0 : index to i32
      %c4_i32 = arith.constant 4 : i32
      %c4_i32_3 = arith.constant 4 : i32
      %c4_i32_4 = arith.constant 4 : i32
      %c1_i32 = arith.constant 1 : i32
      %9 = llvm.call @fpga_gemm_begin(%c4_i32, %c4_i32_3, %c4_i32_4, %c1_i32) : (i32, i32, i32, i32) -> !llvm.ptr
      %subview_5 = memref.subview %subview[0, 0] [4, 4] [1, 1] : memref<4x4xf32, strided<[4, 1], offset: ?>> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %reinterpret_cast = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<128xf32> to memref<4x4xf32>
      memref.copy %subview_5, %reinterpret_cast : memref<4x4xf32, strided<[4, 1], offset: ?>> to memref<4x4xf32>
      %intptr_6 = memref.extract_aligned_pointer_as_index %reinterpret_cast : memref<4x4xf32> -> index
      %10 = arith.index_cast %intptr_6 : index to i64
      %11 = llvm.inttoptr %10 : i64 to !llvm.ptr
      %subview_7 = memref.subview %subview_1[0, 0] [4, 4] [1, 1] : memref<4x4xf32, strided<[4, 1], offset: ?>> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %reinterpret_cast_8 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<128xf32> to memref<4x4xf32>
      memref.copy %subview_7, %reinterpret_cast_8 : memref<4x4xf32, strided<[4, 1], offset: ?>> to memref<4x4xf32>
      %intptr_9 = memref.extract_aligned_pointer_as_index %reinterpret_cast_8 : memref<4x4xf32> -> index
      %12 = arith.index_cast %intptr_9 : index to i64
      %13 = llvm.inttoptr %12 : i64 to !llvm.ptr
      %c0_i32 = arith.constant 0 : i32
      %c0_i32_10 = arith.constant 0 : i32
      %c1_i32_11 = arith.constant 1 : i32
      %c4_i32_12 = arith.constant 4 : i32
      %c4_i32_13 = arith.constant 4 : i32
      %c4_i32_14 = arith.constant 4 : i32
      %c1_i32_15 = arith.constant 1 : i32
      %c0_i32_16 = arith.constant 0 : i32
      %c105_i32 = arith.constant 105 : i32
      %14 = llvm.call @fpga_tile_submit(%9, %c0_i32, %c0_i32_10, %c1_i32_11, %c4_i32_12, %c4_i32_13, %c4_i32_14, %11, %13, %7, %8, %c1_i32_15, %c0_i32_16, %c105_i32) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
      %15 = bufferization.to_tensor %subview_2 restrict writable : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
      %16 = llvm.call @fpga_gemm_wait(%9) : (!llvm.ptr) -> i32
      bufferization.materialize_in_destination %15 in restrict writable %subview_2 : (tensor<4x4xf32>, memref<4x4xf32, strided<[4, 1], offset: ?>>) -> ()
    }
    return
  }
  func.func @bat_two__systolic_kernel(%arg0: memref<?x3x4x4xf32> {llvm.noalias}, %arg1: memref<?x3x4x4xf32> {llvm.noalias}, %arg2: memref<?x3x4x4xf32> {llvm.noalias}) {
    %alloca = memref.alloca() : memref<128xf32>
    %alloca_0 = memref.alloca() : memref<128xf32>
    affine.for %arg3 = 0 to 2 {
      affine.for %arg4 = 0 to 3 {
        %0 = affine.apply #map1(%arg3, %arg4)
        %1 = affine.apply #map2(%arg3, %arg4)
        %subview = memref.subview %arg0[%0, %1, 0, 0] [1, 1, 4, 4] [1, 1, 1, 1] : memref<?x3x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
        %2 = bufferization.to_tensor %subview restrict : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
        %subview_1 = memref.subview %arg1[%0, %1, 0, 0] [1, 1, 4, 4] [1, 1, 1, 1] : memref<?x3x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
        %3 = bufferization.to_tensor %subview_1 restrict : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
        %subview_2 = memref.subview %arg2[%0, %1, 0, 0] [1, 1, 4, 4] [1, 1, 1, 1] : memref<?x3x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
        %4 = bufferization.to_tensor %subview_2 restrict writable : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
        %base_buffer, %offset, %sizes:2, %strides:2 = memref.extract_strided_metadata %subview_2 : memref<4x4xf32, strided<[4, 1], offset: ?>> -> memref<f32>, index, index, index, index, index
        %intptr = memref.extract_aligned_pointer_as_index %base_buffer : memref<f32> -> index
        %c4 = arith.constant 4 : index
        %5 = arith.muli %offset, %c4 : index
        %6 = arith.addi %intptr, %5 : index
        %7 = arith.index_cast %6 : index to i64
        %8 = llvm.inttoptr %7 : i64 to !llvm.ptr
        %9 = arith.index_cast %strides#0 : index to i32
        %c4_i32 = arith.constant 4 : i32
        %c4_i32_3 = arith.constant 4 : i32
        %c4_i32_4 = arith.constant 4 : i32
        %c1_i32 = arith.constant 1 : i32
        %10 = llvm.call @fpga_gemm_begin(%c4_i32, %c4_i32_3, %c4_i32_4, %c1_i32) : (i32, i32, i32, i32) -> !llvm.ptr
        %subview_5 = memref.subview %subview[0, 0] [4, 4] [1, 1] : memref<4x4xf32, strided<[4, 1], offset: ?>> to memref<4x4xf32, strided<[4, 1], offset: ?>>
        %reinterpret_cast = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<128xf32> to memref<4x4xf32>
        memref.copy %subview_5, %reinterpret_cast : memref<4x4xf32, strided<[4, 1], offset: ?>> to memref<4x4xf32>
        %intptr_6 = memref.extract_aligned_pointer_as_index %reinterpret_cast : memref<4x4xf32> -> index
        %11 = arith.index_cast %intptr_6 : index to i64
        %12 = llvm.inttoptr %11 : i64 to !llvm.ptr
        %subview_7 = memref.subview %subview_1[0, 0] [4, 4] [1, 1] : memref<4x4xf32, strided<[4, 1], offset: ?>> to memref<4x4xf32, strided<[4, 1], offset: ?>>
        %reinterpret_cast_8 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<128xf32> to memref<4x4xf32>
        memref.copy %subview_7, %reinterpret_cast_8 : memref<4x4xf32, strided<[4, 1], offset: ?>> to memref<4x4xf32>
        %intptr_9 = memref.extract_aligned_pointer_as_index %reinterpret_cast_8 : memref<4x4xf32> -> index
        %13 = arith.index_cast %intptr_9 : index to i64
        %14 = llvm.inttoptr %13 : i64 to !llvm.ptr
        %c0_i32 = arith.constant 0 : i32
        %c0_i32_10 = arith.constant 0 : i32
        %c1_i32_11 = arith.constant 1 : i32
        %c4_i32_12 = arith.constant 4 : i32
        %c4_i32_13 = arith.constant 4 : i32
        %c4_i32_14 = arith.constant 4 : i32
        %c1_i32_15 = arith.constant 1 : i32
        %c0_i32_16 = arith.constant 0 : i32
        %c105_i32 = arith.constant 105 : i32
        %15 = llvm.call @fpga_tile_submit(%10, %c0_i32, %c0_i32_10, %c1_i32_11, %c4_i32_12, %c4_i32_13, %c4_i32_14, %12, %14, %8, %9, %c1_i32_15, %c0_i32_16, %c105_i32) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
        %16 = bufferization.to_tensor %subview_2 restrict writable : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
        %17 = llvm.call @fpga_gemm_wait(%10) : (!llvm.ptr) -> i32
        bufferization.materialize_in_destination %16 in restrict writable %subview_2 : (tensor<4x4xf32>, memref<4x4xf32, strided<[4, 1], offset: ?>>) -> ()
      }
    }
    return
  }
  func.func @bat_inner__systolic_kernel(%arg0: memref<?x2x4xf32> {llvm.noalias}, %arg1: memref<?x4x4xf32> {llvm.noalias}, %arg2: memref<?x4x4xf32> {llvm.noalias}) {
    %alloca = memref.alloca() : memref<128xf32>
    %alloca_0 = memref.alloca() : memref<128xf32>
    affine.for %arg3 = 0 to 2 {
      %0 = affine.apply #map(%arg3)
      %subview = memref.subview %arg0[0, %0, 0] [4, 1, 4] [1, 1, 1] : memref<?x2x4xf32> to memref<4x4xf32, strided<[8, 1], offset: ?>>
      %1 = bufferization.to_tensor %subview restrict : memref<4x4xf32, strided<[8, 1], offset: ?>> to tensor<4x4xf32>
      %subview_1 = memref.subview %arg1[%0, 0, 0] [1, 4, 4] [1, 1, 1] : memref<?x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %2 = bufferization.to_tensor %subview_1 restrict : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
      %subview_2 = memref.subview %arg2[%0, 0, 0] [1, 4, 4] [1, 1, 1] : memref<?x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %3 = bufferization.to_tensor %subview_2 restrict writable : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
      %base_buffer, %offset, %sizes:2, %strides:2 = memref.extract_strided_metadata %subview_2 : memref<4x4xf32, strided<[4, 1], offset: ?>> -> memref<f32>, index, index, index, index, index
      %intptr = memref.extract_aligned_pointer_as_index %base_buffer : memref<f32> -> index
      %c4 = arith.constant 4 : index
      %4 = arith.muli %offset, %c4 : index
      %5 = arith.addi %intptr, %4 : index
      %6 = arith.index_cast %5 : index to i64
      %7 = llvm.inttoptr %6 : i64 to !llvm.ptr
      %8 = arith.index_cast %strides#0 : index to i32
      %c4_i32 = arith.constant 4 : i32
      %c4_i32_3 = arith.constant 4 : i32
      %c4_i32_4 = arith.constant 4 : i32
      %c1_i32 = arith.constant 1 : i32
      %9 = llvm.call @fpga_gemm_begin(%c4_i32, %c4_i32_3, %c4_i32_4, %c1_i32) : (i32, i32, i32, i32) -> !llvm.ptr
      %subview_5 = memref.subview %subview[0, 0] [4, 4] [1, 1] : memref<4x4xf32, strided<[8, 1], offset: ?>> to memref<4x4xf32, strided<[8, 1], offset: ?>>
      %reinterpret_cast = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<128xf32> to memref<4x4xf32>
      memref.copy %subview_5, %reinterpret_cast : memref<4x4xf32, strided<[8, 1], offset: ?>> to memref<4x4xf32>
      %intptr_6 = memref.extract_aligned_pointer_as_index %reinterpret_cast : memref<4x4xf32> -> index
      %10 = arith.index_cast %intptr_6 : index to i64
      %11 = llvm.inttoptr %10 : i64 to !llvm.ptr
      %subview_7 = memref.subview %subview_1[0, 0] [4, 4] [1, 1] : memref<4x4xf32, strided<[4, 1], offset: ?>> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %reinterpret_cast_8 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<128xf32> to memref<4x4xf32>
      memref.copy %subview_7, %reinterpret_cast_8 : memref<4x4xf32, strided<[4, 1], offset: ?>> to memref<4x4xf32>
      %intptr_9 = memref.extract_aligned_pointer_as_index %reinterpret_cast_8 : memref<4x4xf32> -> index
      %12 = arith.index_cast %intptr_9 : index to i64
      %13 = llvm.inttoptr %12 : i64 to !llvm.ptr
      %c0_i32 = arith.constant 0 : i32
      %c0_i32_10 = arith.constant 0 : i32
      %c1_i32_11 = arith.constant 1 : i32
      %c4_i32_12 = arith.constant 4 : i32
      %c4_i32_13 = arith.constant 4 : i32
      %c4_i32_14 = arith.constant 4 : i32
      %c1_i32_15 = arith.constant 1 : i32
      %c0_i32_16 = arith.constant 0 : i32
      %c105_i32 = arith.constant 105 : i32
      %14 = llvm.call @fpga_tile_submit(%9, %c0_i32, %c0_i32_10, %c1_i32_11, %c4_i32_12, %c4_i32_13, %c4_i32_14, %11, %13, %7, %8, %c1_i32_15, %c0_i32_16, %c105_i32) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
      %15 = bufferization.to_tensor %subview_2 restrict writable : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
      %16 = llvm.call @fpga_gemm_wait(%9) : (!llvm.ptr) -> i32
      bufferization.materialize_in_destination %15 in restrict writable %subview_2 : (tensor<4x4xf32>, memref<4x4xf32, strided<[4, 1], offset: ?>>) -> ()
    }
    return
  }
  func.func @bat_tr_neg__systolic_kernel(%arg0: memref<?x8x8xf32> {llvm.noalias}, %arg1: memref<?x8x8xf32> {llvm.noalias}, %arg2: memref<?x8x8xf32> {llvm.noalias}) {
    %alloca = memref.alloca() : memref<128xf32>
    %alloca_0 = memref.alloca() : memref<128xf32>
    affine.for %arg3 = 0 to 2 {
      %0 = affine.apply #map(%arg3)
      %subview = memref.subview %arg0[%0, 0, 0] [1, 8, 8] [1, 1, 1] : memref<?x8x8xf32> to memref<8x8xf32, strided<[8, 1], offset: ?>>
      %transpose = memref.transpose %subview (d0, d1) -> (d1, d0) : memref<8x8xf32, strided<[8, 1], offset: ?>> to memref<8x8xf32, strided<[1, 8], offset: ?>>
      %1 = bufferization.to_tensor %transpose restrict : memref<8x8xf32, strided<[1, 8], offset: ?>> to tensor<8x8xf32>
      %subview_1 = memref.subview %arg1[%0, 0, 0] [1, 8, 8] [1, 1, 1] : memref<?x8x8xf32> to memref<8x8xf32, strided<[8, 1], offset: ?>>
      %2 = bufferization.to_tensor %subview_1 restrict : memref<8x8xf32, strided<[8, 1], offset: ?>> to tensor<8x8xf32>
      %subview_2 = memref.subview %arg2[%0, 0, 7] [1, 8, 8] [1, 1, -1] : memref<?x8x8xf32> to memref<8x8xf32, strided<[8, -1], offset: ?>>
      %3 = bufferization.to_tensor %subview_2 restrict writable : memref<8x8xf32, strided<[8, -1], offset: ?>> to tensor<8x8xf32>
      %extracted_slice = tensor.extract_slice %3[0, 0] [8, 8] [1, 1] : tensor<8x8xf32> to tensor<8x8xf32>
      %c8_i32 = arith.constant 8 : i32
      %c8_i32_3 = arith.constant 8 : i32
      %c8_i32_4 = arith.constant 8 : i32
      %c1_i32 = arith.constant 1 : i32
      %4 = llvm.call @fpga_gemm_begin(%c8_i32, %c8_i32_3, %c8_i32_4, %c1_i32) : (i32, i32, i32, i32) -> !llvm.ptr
      %5 = bufferization.to_buffer %extracted_slice : tensor<8x8xf32> to memref<8x8xf32>
      %c8_i32_5 = arith.constant 8 : i32
      %intptr = memref.extract_aligned_pointer_as_index %5 : memref<8x8xf32> -> index
      %6 = arith.index_cast %intptr : index to i64
      %7 = llvm.inttoptr %6 : i64 to !llvm.ptr
      %subview_6 = memref.subview %transpose[0, 0] [8, 8] [1, 1] : memref<8x8xf32, strided<[1, 8], offset: ?>> to memref<8x8xf32, strided<[1, 8], offset: ?>>
      %reinterpret_cast = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 8], strides: [8, 1] : memref<128xf32> to memref<8x8xf32>
      memref.copy %subview_6, %reinterpret_cast : memref<8x8xf32, strided<[1, 8], offset: ?>> to memref<8x8xf32>
      %intptr_7 = memref.extract_aligned_pointer_as_index %reinterpret_cast : memref<8x8xf32> -> index
      %8 = arith.index_cast %intptr_7 : index to i64
      %9 = llvm.inttoptr %8 : i64 to !llvm.ptr
      %subview_8 = memref.subview %subview_1[0, 0] [8, 8] [1, 1] : memref<8x8xf32, strided<[8, 1], offset: ?>> to memref<8x8xf32, strided<[8, 1], offset: ?>>
      %reinterpret_cast_9 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [8, 8], strides: [8, 1] : memref<128xf32> to memref<8x8xf32>
      memref.copy %subview_8, %reinterpret_cast_9 : memref<8x8xf32, strided<[8, 1], offset: ?>> to memref<8x8xf32>
      %intptr_10 = memref.extract_aligned_pointer_as_index %reinterpret_cast_9 : memref<8x8xf32> -> index
      %10 = arith.index_cast %intptr_10 : index to i64
      %11 = llvm.inttoptr %10 : i64 to !llvm.ptr
      %c0_i32 = arith.constant 0 : i32
      %c0_i32_11 = arith.constant 0 : i32
      %c1_i32_12 = arith.constant 1 : i32
      %c8_i32_13 = arith.constant 8 : i32
      %c8_i32_14 = arith.constant 8 : i32
      %c8_i32_15 = arith.constant 8 : i32
      %c0_i32_16 = arith.constant 0 : i32
      %c0_i32_17 = arith.constant 0 : i32
      %c117_i32 = arith.constant 117 : i32
      %12 = llvm.call @fpga_tile_submit(%4, %c0_i32, %c0_i32_11, %c1_i32_12, %c8_i32_13, %c8_i32_14, %c8_i32_15, %9, %11, %7, %c8_i32_5, %c0_i32_16, %c0_i32_17, %c117_i32) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
      %13 = bufferization.to_tensor %5 restrict writable : memref<8x8xf32> to tensor<8x8xf32>
      %14 = llvm.call @fpga_gemm_wait(%4) : (!llvm.ptr) -> i32
      %inserted_slice = tensor.insert_slice %13 into %3[0, 0] [8, 8] [1, 1] : tensor<8x8xf32> into tensor<8x8xf32>
      bufferization.materialize_in_destination %inserted_slice in restrict writable %subview_2 : (tensor<8x8xf32>, memref<8x8xf32, strided<[8, -1], offset: ?>>) -> ()
    }
    return
  }
  func.func @bat_down__systolic_kernel(%arg0: memref<?x4x4xf32> {llvm.noalias}, %arg1: memref<?x4x4xf32> {llvm.noalias}, %arg2: memref<?x4x4xf32> {llvm.noalias}) {
    %alloca = memref.alloca() : memref<128xf32>
    %alloca_0 = memref.alloca() : memref<128xf32>
    affine.for %arg3 = 0 to 4 {
      %0 = affine.apply #map3(%arg3)
      %subview = memref.subview %arg0[%0, 0, 0] [1, 4, 4] [1, 1, 1] : memref<?x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %1 = bufferization.to_tensor %subview restrict : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
      %subview_1 = memref.subview %arg1[%0, 0, 0] [1, 4, 4] [1, 1, 1] : memref<?x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %2 = bufferization.to_tensor %subview_1 restrict : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
      %subview_2 = memref.subview %arg2[%0, 0, 0] [1, 4, 4] [1, 1, 1] : memref<?x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %3 = bufferization.to_tensor %subview_2 restrict writable : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
      %base_buffer, %offset, %sizes:2, %strides:2 = memref.extract_strided_metadata %subview_2 : memref<4x4xf32, strided<[4, 1], offset: ?>> -> memref<f32>, index, index, index, index, index
      %intptr = memref.extract_aligned_pointer_as_index %base_buffer : memref<f32> -> index
      %c4 = arith.constant 4 : index
      %4 = arith.muli %offset, %c4 : index
      %5 = arith.addi %intptr, %4 : index
      %6 = arith.index_cast %5 : index to i64
      %7 = llvm.inttoptr %6 : i64 to !llvm.ptr
      %8 = arith.index_cast %strides#0 : index to i32
      %c4_i32 = arith.constant 4 : i32
      %c4_i32_3 = arith.constant 4 : i32
      %c4_i32_4 = arith.constant 4 : i32
      %c1_i32 = arith.constant 1 : i32
      %9 = llvm.call @fpga_gemm_begin(%c4_i32, %c4_i32_3, %c4_i32_4, %c1_i32) : (i32, i32, i32, i32) -> !llvm.ptr
      %subview_5 = memref.subview %subview[0, 0] [4, 4] [1, 1] : memref<4x4xf32, strided<[4, 1], offset: ?>> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %reinterpret_cast = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<128xf32> to memref<4x4xf32>
      memref.copy %subview_5, %reinterpret_cast : memref<4x4xf32, strided<[4, 1], offset: ?>> to memref<4x4xf32>
      %intptr_6 = memref.extract_aligned_pointer_as_index %reinterpret_cast : memref<4x4xf32> -> index
      %10 = arith.index_cast %intptr_6 : index to i64
      %11 = llvm.inttoptr %10 : i64 to !llvm.ptr
      %subview_7 = memref.subview %subview_1[0, 0] [4, 4] [1, 1] : memref<4x4xf32, strided<[4, 1], offset: ?>> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %reinterpret_cast_8 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<128xf32> to memref<4x4xf32>
      memref.copy %subview_7, %reinterpret_cast_8 : memref<4x4xf32, strided<[4, 1], offset: ?>> to memref<4x4xf32>
      %intptr_9 = memref.extract_aligned_pointer_as_index %reinterpret_cast_8 : memref<4x4xf32> -> index
      %12 = arith.index_cast %intptr_9 : index to i64
      %13 = llvm.inttoptr %12 : i64 to !llvm.ptr
      %c0_i32 = arith.constant 0 : i32
      %c0_i32_10 = arith.constant 0 : i32
      %c1_i32_11 = arith.constant 1 : i32
      %c4_i32_12 = arith.constant 4 : i32
      %c4_i32_13 = arith.constant 4 : i32
      %c4_i32_14 = arith.constant 4 : i32
      %c1_i32_15 = arith.constant 1 : i32
      %c0_i32_16 = arith.constant 0 : i32
      %c105_i32 = arith.constant 105 : i32
      %14 = llvm.call @fpga_tile_submit(%9, %c0_i32, %c0_i32_10, %c1_i32_11, %c4_i32_12, %c4_i32_13, %c4_i32_14, %11, %13, %7, %8, %c1_i32_15, %c0_i32_16, %c105_i32) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
      %15 = bufferization.to_tensor %subview_2 restrict writable : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
      %16 = llvm.call @fpga_gemm_wait(%9) : (!llvm.ptr) -> i32
      bufferization.materialize_in_destination %15 in restrict writable %subview_2 : (tensor<4x4xf32>, memref<4x4xf32, strided<[4, 1], offset: ?>>) -> ()
    }
    return
  }
  func.func @bat_offset__systolic_kernel(%arg0: memref<?x4x4xf32> {llvm.noalias}, %arg1: memref<?x4x4xf32> {llvm.noalias}, %arg2: memref<?x4x4xf32> {llvm.noalias}) {
    %alloca = memref.alloca() : memref<128xf32>
    %alloca_0 = memref.alloca() : memref<128xf32>
    affine.for %arg3 = 0 to 3 {
      %0 = affine.apply #map4(%arg3)
      %subview = memref.subview %arg0[%0, 0, 0] [1, 4, 4] [1, 1, 1] : memref<?x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %1 = bufferization.to_tensor %subview restrict : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
      %subview_1 = memref.subview %arg1[%0, 0, 0] [1, 4, 4] [1, 1, 1] : memref<?x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %2 = bufferization.to_tensor %subview_1 restrict : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
      %subview_2 = memref.subview %arg2[%0, 0, 0] [1, 4, 4] [1, 1, 1] : memref<?x4x4xf32> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %3 = bufferization.to_tensor %subview_2 restrict writable : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
      %base_buffer, %offset, %sizes:2, %strides:2 = memref.extract_strided_metadata %subview_2 : memref<4x4xf32, strided<[4, 1], offset: ?>> -> memref<f32>, index, index, index, index, index
      %intptr = memref.extract_aligned_pointer_as_index %base_buffer : memref<f32> -> index
      %c4 = arith.constant 4 : index
      %4 = arith.muli %offset, %c4 : index
      %5 = arith.addi %intptr, %4 : index
      %6 = arith.index_cast %5 : index to i64
      %7 = llvm.inttoptr %6 : i64 to !llvm.ptr
      %8 = arith.index_cast %strides#0 : index to i32
      %c4_i32 = arith.constant 4 : i32
      %c4_i32_3 = arith.constant 4 : i32
      %c4_i32_4 = arith.constant 4 : i32
      %c1_i32 = arith.constant 1 : i32
      %9 = llvm.call @fpga_gemm_begin(%c4_i32, %c4_i32_3, %c4_i32_4, %c1_i32) : (i32, i32, i32, i32) -> !llvm.ptr
      %subview_5 = memref.subview %subview[0, 0] [4, 4] [1, 1] : memref<4x4xf32, strided<[4, 1], offset: ?>> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %reinterpret_cast = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<128xf32> to memref<4x4xf32>
      memref.copy %subview_5, %reinterpret_cast : memref<4x4xf32, strided<[4, 1], offset: ?>> to memref<4x4xf32>
      %intptr_6 = memref.extract_aligned_pointer_as_index %reinterpret_cast : memref<4x4xf32> -> index
      %10 = arith.index_cast %intptr_6 : index to i64
      %11 = llvm.inttoptr %10 : i64 to !llvm.ptr
      %subview_7 = memref.subview %subview_1[0, 0] [4, 4] [1, 1] : memref<4x4xf32, strided<[4, 1], offset: ?>> to memref<4x4xf32, strided<[4, 1], offset: ?>>
      %reinterpret_cast_8 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<128xf32> to memref<4x4xf32>
      memref.copy %subview_7, %reinterpret_cast_8 : memref<4x4xf32, strided<[4, 1], offset: ?>> to memref<4x4xf32>
      %intptr_9 = memref.extract_aligned_pointer_as_index %reinterpret_cast_8 : memref<4x4xf32> -> index
      %12 = arith.index_cast %intptr_9 : index to i64
      %13 = llvm.inttoptr %12 : i64 to !llvm.ptr
      %c0_i32 = arith.constant 0 : i32
      %c0_i32_10 = arith.constant 0 : i32
      %c1_i32_11 = arith.constant 1 : i32
      %c4_i32_12 = arith.constant 4 : i32
      %c4_i32_13 = arith.constant 4 : i32
      %c4_i32_14 = arith.constant 4 : i32
      %c1_i32_15 = arith.constant 1 : i32
      %c0_i32_16 = arith.constant 0 : i32
      %c105_i32 = arith.constant 105 : i32
      %14 = llvm.call @fpga_tile_submit(%9, %c0_i32, %c0_i32_10, %c1_i32_11, %c4_i32_12, %c4_i32_13, %c4_i32_14, %11, %13, %7, %8, %c1_i32_15, %c0_i32_16, %c105_i32) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
      %15 = bufferization.to_tensor %subview_2 restrict writable : memref<4x4xf32, strided<[4, 1], offset: ?>> to tensor<4x4xf32>
      %16 = llvm.call @fpga_gemm_wait(%9) : (!llvm.ptr) -> i32
      bufferization.materialize_in_destination %15 in restrict writable %subview_2 : (tensor<4x4xf32>, memref<4x4xf32, strided<[4, 1], offset: ?>>) -> ()
    }
    return
  }
  func.func @bat_tiles__systolic_kernel(%arg0: memref<?x16x16xf32> {llvm.noalias}, %arg1: memref<?x16x16xf32> {llvm.noalias}, %arg2: memref<?x16x16xf32> {llvm.noalias}) {
    %alloca = memref.alloca() : memref<128xf32>
    %alloca_0 = memref.alloca() : memref<128xf32>
    affine.for %arg3 = 0 to 2 {
      %0 = affine.apply #map(%arg3)
      %subview = memref.subview %arg0[%0, 0, 0] [1, 16, 16] [1, 1, 1] : memref<?x16x16xf32> to memref<16x16xf32, strided<[16, 1], offset: ?>>
      %1 = bufferization.to_tensor %subview restrict : memref<16x16xf32, strided<[16, 1], offset: ?>> to tensor<16x16xf32>
      %subview_1 = memref.subview %arg1[%0, 0, 0] [1, 16, 16] [1, 1, 1] : memref<?x16x16xf32> to memref<16x16xf32, strided<[16, 1], offset: ?>>
      %2 = bufferization.to_tensor %subview_1 restrict : memref<16x16xf32, strided<[16, 1], offset: ?>> to tensor<16x16xf32>
      %subview_2 = memref.subview %arg2[%0, 0, 0] [1, 16, 16] [1, 1, 1] : memref<?x16x16xf32> to memref<16x16xf32, strided<[16, 1], offset: ?>>
      %3 = bufferization.to_tensor %subview_2 restrict writable : memref<16x16xf32, strided<[16, 1], offset: ?>> to tensor<16x16xf32>
      %base_buffer, %offset, %sizes:2, %strides:2 = memref.extract_strided_metadata %subview_2 : memref<16x16xf32, strided<[16, 1], offset: ?>> -> memref<f32>, index, index, index, index, index
      %intptr = memref.extract_aligned_pointer_as_index %base_buffer : memref<f32> -> index
      %c4 = arith.constant 4 : index
      %4 = arith.muli %offset, %c4 : index
      %5 = arith.addi %intptr, %4 : index
      %6 = arith.index_cast %5 : index to i64
      %7 = llvm.inttoptr %6 : i64 to !llvm.ptr
      %8 = arith.index_cast %strides#0 : index to i32
      %c16_i32 = arith.constant 16 : i32
      %c16_i32_3 = arith.constant 16 : i32
      %c16_i32_4 = arith.constant 16 : i32
      %c7_i32 = arith.constant 7 : i32
      %9 = llvm.call @fpga_gemm_begin(%c16_i32, %c16_i32_3, %c16_i32_4, %c7_i32) : (i32, i32, i32, i32) -> !llvm.ptr
      %subview_5 = memref.subview %subview[0, 0] [8, 16] [1, 1] : memref<16x16xf32, strided<[16, 1], offset: ?>> to memref<8x16xf32, strided<[16, 1], offset: ?>>
      %reinterpret_cast = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 16], strides: [16, 1] : memref<128xf32> to memref<8x16xf32>
      memref.copy %subview_5, %reinterpret_cast : memref<8x16xf32, strided<[16, 1], offset: ?>> to memref<8x16xf32>
      %intptr_6 = memref.extract_aligned_pointer_as_index %reinterpret_cast : memref<8x16xf32> -> index
      %10 = arith.index_cast %intptr_6 : index to i64
      %11 = llvm.inttoptr %10 : i64 to !llvm.ptr
      %subview_7 = memref.subview %subview_1[0, 0] [16, 8] [1, 1] : memref<16x16xf32, strided<[16, 1], offset: ?>> to memref<16x8xf32, strided<[16, 1], offset: ?>>
      %reinterpret_cast_8 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 8], strides: [8, 1] : memref<128xf32> to memref<16x8xf32>
      memref.copy %subview_7, %reinterpret_cast_8 : memref<16x8xf32, strided<[16, 1], offset: ?>> to memref<16x8xf32>
      %intptr_9 = memref.extract_aligned_pointer_as_index %reinterpret_cast_8 : memref<16x8xf32> -> index
      %12 = arith.index_cast %intptr_9 : index to i64
      %13 = llvm.inttoptr %12 : i64 to !llvm.ptr
      %c0_i32 = arith.constant 0 : i32
      %c0_i32_10 = arith.constant 0 : i32
      %c1_i32 = arith.constant 1 : i32
      %c8_i32 = arith.constant 8 : i32
      %c16_i32_11 = arith.constant 16 : i32
      %c8_i32_12 = arith.constant 8 : i32
      %c0_i32_13 = arith.constant 0 : i32
      %c0_i32_14 = arith.constant 0 : i32
      %c125_i32 = arith.constant 125 : i32
      %14 = llvm.call @fpga_tile_submit(%9, %c0_i32, %c0_i32_10, %c1_i32, %c8_i32, %c16_i32_11, %c8_i32_12, %11, %13, %7, %8, %c0_i32_13, %c0_i32_14, %c125_i32) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
      %15 = bufferization.to_tensor %subview_2 restrict writable : memref<16x16xf32, strided<[16, 1], offset: ?>> to tensor<16x16xf32>
      %base_buffer_15, %offset_16, %sizes_17:2, %strides_18:2 = memref.extract_strided_metadata %subview_2 : memref<16x16xf32, strided<[16, 1], offset: ?>> -> memref<f32>, index, index, index, index, index
      %intptr_19 = memref.extract_aligned_pointer_as_index %base_buffer_15 : memref<f32> -> index
      %c8 = arith.constant 8 : index
      %16 = arith.addi %offset_16, %c8 : index
      %c4_20 = arith.constant 4 : index
      %17 = arith.muli %16, %c4_20 : index
      %18 = arith.addi %intptr_19, %17 : index
      %19 = arith.index_cast %18 : index to i64
      %20 = llvm.inttoptr %19 : i64 to !llvm.ptr
      %21 = arith.index_cast %strides_18#0 : index to i32
      %subview_21 = memref.subview %subview[0, 0] [8, 16] [1, 1] : memref<16x16xf32, strided<[16, 1], offset: ?>> to memref<8x16xf32, strided<[16, 1], offset: ?>>
      %reinterpret_cast_22 = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 16], strides: [16, 1] : memref<128xf32> to memref<8x16xf32>
      memref.copy %subview_21, %reinterpret_cast_22 : memref<8x16xf32, strided<[16, 1], offset: ?>> to memref<8x16xf32>
      %intptr_23 = memref.extract_aligned_pointer_as_index %reinterpret_cast_22 : memref<8x16xf32> -> index
      %22 = arith.index_cast %intptr_23 : index to i64
      %23 = llvm.inttoptr %22 : i64 to !llvm.ptr
      %subview_24 = memref.subview %subview_1[0, 8] [16, 8] [1, 1] : memref<16x16xf32, strided<[16, 1], offset: ?>> to memref<16x8xf32, strided<[16, 1], offset: ?>>
      %reinterpret_cast_25 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 8], strides: [8, 1] : memref<128xf32> to memref<16x8xf32>
      memref.copy %subview_24, %reinterpret_cast_25 : memref<16x8xf32, strided<[16, 1], offset: ?>> to memref<16x8xf32>
      %intptr_26 = memref.extract_aligned_pointer_as_index %reinterpret_cast_25 : memref<16x8xf32> -> index
      %24 = arith.index_cast %intptr_26 : index to i64
      %25 = llvm.inttoptr %24 : i64 to !llvm.ptr
      %c1_i32_27 = arith.constant 1 : i32
      %c0_i32_28 = arith.constant 0 : i32
      %c1_i32_29 = arith.constant 1 : i32
      %c8_i32_30 = arith.constant 8 : i32
      %c16_i32_31 = arith.constant 16 : i32
      %c8_i32_32 = arith.constant 8 : i32
      %c0_i32_33 = arith.constant 0 : i32
      %c42_i32 = arith.constant 42 : i32
      %c125_i32_34 = arith.constant 125 : i32
      %26 = llvm.call @fpga_tile_submit(%9, %c1_i32_27, %c0_i32_28, %c1_i32_29, %c8_i32_30, %c16_i32_31, %c8_i32_32, %23, %25, %20, %21, %c0_i32_33, %c42_i32, %c125_i32_34) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
      %27 = bufferization.to_tensor %subview_2 restrict writable : memref<16x16xf32, strided<[16, 1], offset: ?>> to tensor<16x16xf32>
      %base_buffer_35, %offset_36, %sizes_37:2, %strides_38:2 = memref.extract_strided_metadata %subview_2 : memref<16x16xf32, strided<[16, 1], offset: ?>> -> memref<f32>, index, index, index, index, index
      %intptr_39 = memref.extract_aligned_pointer_as_index %base_buffer_35 : memref<f32> -> index
      %c8_40 = arith.constant 8 : index
      %28 = arith.muli %strides_38#0, %c8_40 : index
      %29 = arith.addi %offset_36, %28 : index
      %c4_41 = arith.constant 4 : index
      %30 = arith.muli %29, %c4_41 : index
      %31 = arith.addi %intptr_39, %30 : index
      %32 = arith.index_cast %31 : index to i64
      %33 = llvm.inttoptr %32 : i64 to !llvm.ptr
      %34 = arith.index_cast %strides_38#0 : index to i32
      %subview_42 = memref.subview %subview[8, 0] [8, 16] [1, 1] : memref<16x16xf32, strided<[16, 1], offset: ?>> to memref<8x16xf32, strided<[16, 1], offset: ?>>
      %reinterpret_cast_43 = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 16], strides: [16, 1] : memref<128xf32> to memref<8x16xf32>
      memref.copy %subview_42, %reinterpret_cast_43 : memref<8x16xf32, strided<[16, 1], offset: ?>> to memref<8x16xf32>
      %intptr_44 = memref.extract_aligned_pointer_as_index %reinterpret_cast_43 : memref<8x16xf32> -> index
      %35 = arith.index_cast %intptr_44 : index to i64
      %36 = llvm.inttoptr %35 : i64 to !llvm.ptr
      %subview_45 = memref.subview %subview_1[0, 0] [16, 8] [1, 1] : memref<16x16xf32, strided<[16, 1], offset: ?>> to memref<16x8xf32, strided<[16, 1], offset: ?>>
      %reinterpret_cast_46 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 8], strides: [8, 1] : memref<128xf32> to memref<16x8xf32>
      memref.copy %subview_45, %reinterpret_cast_46 : memref<16x8xf32, strided<[16, 1], offset: ?>> to memref<16x8xf32>
      %intptr_47 = memref.extract_aligned_pointer_as_index %reinterpret_cast_46 : memref<16x8xf32> -> index
      %37 = arith.index_cast %intptr_47 : index to i64
      %38 = llvm.inttoptr %37 : i64 to !llvm.ptr
      %c2_i32 = arith.constant 2 : i32
      %c0_i32_48 = arith.constant 0 : i32
      %c1_i32_49 = arith.constant 1 : i32
      %c8_i32_50 = arith.constant 8 : i32
      %c16_i32_51 = arith.constant 16 : i32
      %c8_i32_52 = arith.constant 8 : i32
      %c0_i32_53 = arith.constant 0 : i32
      %c128_i32 = arith.constant 128 : i32
      %c125_i32_54 = arith.constant 125 : i32
      %39 = llvm.call @fpga_tile_submit(%9, %c2_i32, %c0_i32_48, %c1_i32_49, %c8_i32_50, %c16_i32_51, %c8_i32_52, %36, %38, %33, %34, %c0_i32_53, %c128_i32, %c125_i32_54) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
      %40 = bufferization.to_tensor %subview_2 restrict writable : memref<16x16xf32, strided<[16, 1], offset: ?>> to tensor<16x16xf32>
      %base_buffer_55, %offset_56, %sizes_57:2, %strides_58:2 = memref.extract_strided_metadata %subview_2 : memref<16x16xf32, strided<[16, 1], offset: ?>> -> memref<f32>, index, index, index, index, index
      %intptr_59 = memref.extract_aligned_pointer_as_index %base_buffer_55 : memref<f32> -> index
      %c8_60 = arith.constant 8 : index
      %41 = arith.muli %strides_58#0, %c8_60 : index
      %42 = arith.addi %offset_56, %41 : index
      %c8_61 = arith.constant 8 : index
      %43 = arith.addi %42, %c8_61 : index
      %c4_62 = arith.constant 4 : index
      %44 = arith.muli %43, %c4_62 : index
      %45 = arith.addi %intptr_59, %44 : index
      %46 = arith.index_cast %45 : index to i64
      %47 = llvm.inttoptr %46 : i64 to !llvm.ptr
      %48 = arith.index_cast %strides_58#0 : index to i32
      %subview_63 = memref.subview %subview[8, 0] [4, 16] [1, 1] : memref<16x16xf32, strided<[16, 1], offset: ?>> to memref<4x16xf32, strided<[16, 1], offset: ?>>
      %reinterpret_cast_64 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
      memref.copy %subview_63, %reinterpret_cast_64 : memref<4x16xf32, strided<[16, 1], offset: ?>> to memref<4x16xf32>
      %intptr_65 = memref.extract_aligned_pointer_as_index %reinterpret_cast_64 : memref<4x16xf32> -> index
      %49 = arith.index_cast %intptr_65 : index to i64
      %50 = llvm.inttoptr %49 : i64 to !llvm.ptr
      %subview_66 = memref.subview %subview_1[0, 8] [16, 4] [1, 1] : memref<16x16xf32, strided<[16, 1], offset: ?>> to memref<16x4xf32, strided<[16, 1], offset: ?>>
      %reinterpret_cast_67 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
      memref.copy %subview_66, %reinterpret_cast_67 : memref<16x4xf32, strided<[16, 1], offset: ?>> to memref<16x4xf32>
      %intptr_68 = memref.extract_aligned_pointer_as_index %reinterpret_cast_67 : memref<16x4xf32> -> index
      %51 = arith.index_cast %intptr_68 : index to i64
      %52 = llvm.inttoptr %51 : i64 to !llvm.ptr
      %c3_i32 = arith.constant 3 : i32
      %c0_i32_69 = arith.constant 0 : i32
      %c1_i32_70 = arith.constant 1 : i32
      %c4_i32 = arith.constant 4 : i32
      %c16_i32_71 = arith.constant 16 : i32
      %c4_i32_72 = arith.constant 4 : i32
      %c1_i32_73 = arith.constant 1 : i32
      %c0_i32_74 = arith.constant 0 : i32
      %c117_i32 = arith.constant 117 : i32
      %53 = llvm.call @fpga_tile_submit(%9, %c3_i32, %c0_i32_69, %c1_i32_70, %c4_i32, %c16_i32_71, %c4_i32_72, %50, %52, %47, %48, %c1_i32_73, %c0_i32_74, %c117_i32) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
      %54 = bufferization.to_tensor %subview_2 restrict writable : memref<16x16xf32, strided<[16, 1], offset: ?>> to tensor<16x16xf32>
      %base_buffer_75, %offset_76, %sizes_77:2, %strides_78:2 = memref.extract_strided_metadata %subview_2 : memref<16x16xf32, strided<[16, 1], offset: ?>> -> memref<f32>, index, index, index, index, index
      %intptr_79 = memref.extract_aligned_pointer_as_index %base_buffer_75 : memref<f32> -> index
      %c8_80 = arith.constant 8 : index
      %55 = arith.muli %strides_78#0, %c8_80 : index
      %56 = arith.addi %offset_76, %55 : index
      %c12 = arith.constant 12 : index
      %57 = arith.addi %56, %c12 : index
      %c4_81 = arith.constant 4 : index
      %58 = arith.muli %57, %c4_81 : index
      %59 = arith.addi %intptr_79, %58 : index
      %60 = arith.index_cast %59 : index to i64
      %61 = llvm.inttoptr %60 : i64 to !llvm.ptr
      %62 = arith.index_cast %strides_78#0 : index to i32
      %subview_82 = memref.subview %subview[8, 0] [4, 16] [1, 1] : memref<16x16xf32, strided<[16, 1], offset: ?>> to memref<4x16xf32, strided<[16, 1], offset: ?>>
      %reinterpret_cast_83 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
      memref.copy %subview_82, %reinterpret_cast_83 : memref<4x16xf32, strided<[16, 1], offset: ?>> to memref<4x16xf32>
      %intptr_84 = memref.extract_aligned_pointer_as_index %reinterpret_cast_83 : memref<4x16xf32> -> index
      %63 = arith.index_cast %intptr_84 : index to i64
      %64 = llvm.inttoptr %63 : i64 to !llvm.ptr
      %subview_85 = memref.subview %subview_1[0, 12] [16, 4] [1, 1] : memref<16x16xf32, strided<[16, 1], offset: ?>> to memref<16x4xf32, strided<[16, 1], offset: ?>>
      %reinterpret_cast_86 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
      memref.copy %subview_85, %reinterpret_cast_86 : memref<16x4xf32, strided<[16, 1], offset: ?>> to memref<16x4xf32>
      %intptr_87 = memref.extract_aligned_pointer_as_index %reinterpret_cast_86 : memref<16x4xf32> -> index
      %65 = arith.index_cast %intptr_87 : index to i64
      %66 = llvm.inttoptr %65 : i64 to !llvm.ptr
      %c4_i32_88 = arith.constant 4 : i32
      %c0_i32_89 = arith.constant 0 : i32
      %c1_i32_90 = arith.constant 1 : i32
      %c4_i32_91 = arith.constant 4 : i32
      %c16_i32_92 = arith.constant 16 : i32
      %c4_i32_93 = arith.constant 4 : i32
      %c2_i32_94 = arith.constant 2 : i32
      %c0_i32_95 = arith.constant 0 : i32
      %c117_i32_96 = arith.constant 117 : i32
      %67 = llvm.call @fpga_tile_submit(%9, %c4_i32_88, %c0_i32_89, %c1_i32_90, %c4_i32_91, %c16_i32_92, %c4_i32_93, %64, %66, %61, %62, %c2_i32_94, %c0_i32_95, %c117_i32_96) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
      %68 = bufferization.to_tensor %subview_2 restrict writable : memref<16x16xf32, strided<[16, 1], offset: ?>> to tensor<16x16xf32>
      %base_buffer_97, %offset_98, %sizes_99:2, %strides_100:2 = memref.extract_strided_metadata %subview_2 : memref<16x16xf32, strided<[16, 1], offset: ?>> -> memref<f32>, index, index, index, index, index
      %intptr_101 = memref.extract_aligned_pointer_as_index %base_buffer_97 : memref<f32> -> index
      %c12_102 = arith.constant 12 : index
      %69 = arith.muli %strides_100#0, %c12_102 : index
      %70 = arith.addi %offset_98, %69 : index
      %c8_103 = arith.constant 8 : index
      %71 = arith.addi %70, %c8_103 : index
      %c4_104 = arith.constant 4 : index
      %72 = arith.muli %71, %c4_104 : index
      %73 = arith.addi %intptr_101, %72 : index
      %74 = arith.index_cast %73 : index to i64
      %75 = llvm.inttoptr %74 : i64 to !llvm.ptr
      %76 = arith.index_cast %strides_100#0 : index to i32
      %subview_105 = memref.subview %subview[12, 0] [4, 16] [1, 1] : memref<16x16xf32, strided<[16, 1], offset: ?>> to memref<4x16xf32, strided<[16, 1], offset: ?>>
      %reinterpret_cast_106 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
      memref.copy %subview_105, %reinterpret_cast_106 : memref<4x16xf32, strided<[16, 1], offset: ?>> to memref<4x16xf32>
      %intptr_107 = memref.extract_aligned_pointer_as_index %reinterpret_cast_106 : memref<4x16xf32> -> index
      %77 = arith.index_cast %intptr_107 : index to i64
      %78 = llvm.inttoptr %77 : i64 to !llvm.ptr
      %subview_108 = memref.subview %subview_1[0, 8] [16, 4] [1, 1] : memref<16x16xf32, strided<[16, 1], offset: ?>> to memref<16x4xf32, strided<[16, 1], offset: ?>>
      %reinterpret_cast_109 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
      memref.copy %subview_108, %reinterpret_cast_109 : memref<16x4xf32, strided<[16, 1], offset: ?>> to memref<16x4xf32>
      %intptr_110 = memref.extract_aligned_pointer_as_index %reinterpret_cast_109 : memref<16x4xf32> -> index
      %79 = arith.index_cast %intptr_110 : index to i64
      %80 = llvm.inttoptr %79 : i64 to !llvm.ptr
      %c5_i32 = arith.constant 5 : i32
      %c0_i32_111 = arith.constant 0 : i32
      %c1_i32_112 = arith.constant 1 : i32
      %c4_i32_113 = arith.constant 4 : i32
      %c16_i32_114 = arith.constant 16 : i32
      %c4_i32_115 = arith.constant 4 : i32
      %c3_i32_116 = arith.constant 3 : i32
      %c0_i32_117 = arith.constant 0 : i32
      %c117_i32_118 = arith.constant 117 : i32
      %81 = llvm.call @fpga_tile_submit(%9, %c5_i32, %c0_i32_111, %c1_i32_112, %c4_i32_113, %c16_i32_114, %c4_i32_115, %78, %80, %75, %76, %c3_i32_116, %c0_i32_117, %c117_i32_118) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
      %82 = bufferization.to_tensor %subview_2 restrict writable : memref<16x16xf32, strided<[16, 1], offset: ?>> to tensor<16x16xf32>
      %base_buffer_119, %offset_120, %sizes_121:2, %strides_122:2 = memref.extract_strided_metadata %subview_2 : memref<16x16xf32, strided<[16, 1], offset: ?>> -> memref<f32>, index, index, index, index, index
      %intptr_123 = memref.extract_aligned_pointer_as_index %base_buffer_119 : memref<f32> -> index
      %c12_124 = arith.constant 12 : index
      %83 = arith.muli %strides_122#0, %c12_124 : index
      %84 = arith.addi %offset_120, %83 : index
      %c12_125 = arith.constant 12 : index
      %85 = arith.addi %84, %c12_125 : index
      %c4_126 = arith.constant 4 : index
      %86 = arith.muli %85, %c4_126 : index
      %87 = arith.addi %intptr_123, %86 : index
      %88 = arith.index_cast %87 : index to i64
      %89 = llvm.inttoptr %88 : i64 to !llvm.ptr
      %90 = arith.index_cast %strides_122#0 : index to i32
      %subview_127 = memref.subview %subview[12, 0] [4, 16] [1, 1] : memref<16x16xf32, strided<[16, 1], offset: ?>> to memref<4x16xf32, strided<[16, 1], offset: ?>>
      %reinterpret_cast_128 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
      memref.copy %subview_127, %reinterpret_cast_128 : memref<4x16xf32, strided<[16, 1], offset: ?>> to memref<4x16xf32>
      %intptr_129 = memref.extract_aligned_pointer_as_index %reinterpret_cast_128 : memref<4x16xf32> -> index
      %91 = arith.index_cast %intptr_129 : index to i64
      %92 = llvm.inttoptr %91 : i64 to !llvm.ptr
      %subview_130 = memref.subview %subview_1[0, 12] [16, 4] [1, 1] : memref<16x16xf32, strided<[16, 1], offset: ?>> to memref<16x4xf32, strided<[16, 1], offset: ?>>
      %reinterpret_cast_131 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
      memref.copy %subview_130, %reinterpret_cast_131 : memref<16x4xf32, strided<[16, 1], offset: ?>> to memref<16x4xf32>
      %intptr_132 = memref.extract_aligned_pointer_as_index %reinterpret_cast_131 : memref<16x4xf32> -> index
      %93 = arith.index_cast %intptr_132 : index to i64
      %94 = llvm.inttoptr %93 : i64 to !llvm.ptr
      %c6_i32 = arith.constant 6 : i32
      %c0_i32_133 = arith.constant 0 : i32
      %c1_i32_134 = arith.constant 1 : i32
      %c4_i32_135 = arith.constant 4 : i32
      %c16_i32_136 = arith.constant 16 : i32
      %c4_i32_137 = arith.constant 4 : i32
      %c1_i32_138 = arith.constant 1 : i32
      %c42_i32_139 = arith.constant 42 : i32
      %c117_i32_140 = arith.constant 117 : i32
      %95 = llvm.call @fpga_tile_submit(%9, %c6_i32, %c0_i32_133, %c1_i32_134, %c4_i32_135, %c16_i32_136, %c4_i32_137, %92, %94, %89, %90, %c1_i32_138, %c42_i32_139, %c117_i32_140) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
      %96 = bufferization.to_tensor %subview_2 restrict writable : memref<16x16xf32, strided<[16, 1], offset: ?>> to tensor<16x16xf32>
      %97 = llvm.call @fpga_gemm_wait(%9) : (!llvm.ptr) -> i32
      bufferization.materialize_in_destination %96 in restrict writable %subview_2 : (tensor<16x16xf32>, memref<16x16xf32, strided<[16, 1], offset: ?>>) -> ()
    }
    return
  }
}

