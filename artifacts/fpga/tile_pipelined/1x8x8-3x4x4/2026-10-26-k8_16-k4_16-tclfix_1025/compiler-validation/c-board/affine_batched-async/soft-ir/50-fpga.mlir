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
      %c10_i32 = arith.constant 10 : i32
      %9 = llvm.call @fpga_gemm_begin(%c16_i32, %c16_i32_3, %c16_i32_4, %c10_i32) : (i32, i32, i32, i32) -> !llvm.ptr
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
      %c125_i32_34 = arith.constant 125 : i32
      %c125_i32_35 = arith.constant 125 : i32
      %26 = llvm.call @fpga_tile_submit(%9, %c1_i32_27, %c0_i32_28, %c1_i32_29, %c8_i32_30, %c16_i32_31, %c8_i32_32, %23, %25, %20, %21, %c0_i32_33, %c125_i32_34, %c125_i32_35) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
      %27 = bufferization.to_tensor %subview_2 restrict writable : memref<16x16xf32, strided<[16, 1], offset: ?>> to tensor<16x16xf32>
      %base_buffer_36, %offset_37, %sizes_38:2, %strides_39:2 = memref.extract_strided_metadata %subview_2 : memref<16x16xf32, strided<[16, 1], offset: ?>> -> memref<f32>, index, index, index, index, index
      %intptr_40 = memref.extract_aligned_pointer_as_index %base_buffer_36 : memref<f32> -> index
      %c8_41 = arith.constant 8 : index
      %28 = arith.muli %strides_39#0, %c8_41 : index
      %29 = arith.addi %offset_37, %28 : index
      %c4_42 = arith.constant 4 : index
      %30 = arith.muli %29, %c4_42 : index
      %31 = arith.addi %intptr_40, %30 : index
      %32 = arith.index_cast %31 : index to i64
      %33 = llvm.inttoptr %32 : i64 to !llvm.ptr
      %34 = arith.index_cast %strides_39#0 : index to i32
      %subview_43 = memref.subview %subview[8, 0] [4, 16] [1, 1] : memref<16x16xf32, strided<[16, 1], offset: ?>> to memref<4x16xf32, strided<[16, 1], offset: ?>>
      %reinterpret_cast_44 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
      memref.copy %subview_43, %reinterpret_cast_44 : memref<4x16xf32, strided<[16, 1], offset: ?>> to memref<4x16xf32>
      %intptr_45 = memref.extract_aligned_pointer_as_index %reinterpret_cast_44 : memref<4x16xf32> -> index
      %35 = arith.index_cast %intptr_45 : index to i64
      %36 = llvm.inttoptr %35 : i64 to !llvm.ptr
      %subview_46 = memref.subview %subview_1[0, 0] [16, 4] [1, 1] : memref<16x16xf32, strided<[16, 1], offset: ?>> to memref<16x4xf32, strided<[16, 1], offset: ?>>
      %reinterpret_cast_47 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
      memref.copy %subview_46, %reinterpret_cast_47 : memref<16x4xf32, strided<[16, 1], offset: ?>> to memref<16x4xf32>
      %intptr_48 = memref.extract_aligned_pointer_as_index %reinterpret_cast_47 : memref<16x4xf32> -> index
      %37 = arith.index_cast %intptr_48 : index to i64
      %38 = llvm.inttoptr %37 : i64 to !llvm.ptr
      %c2_i32 = arith.constant 2 : i32
      %c0_i32_49 = arith.constant 0 : i32
      %c1_i32_50 = arith.constant 1 : i32
      %c4_i32 = arith.constant 4 : i32
      %c16_i32_51 = arith.constant 16 : i32
      %c4_i32_52 = arith.constant 4 : i32
      %c1_i32_53 = arith.constant 1 : i32
      %c0_i32_54 = arith.constant 0 : i32
      %c117_i32 = arith.constant 117 : i32
      %39 = llvm.call @fpga_tile_submit(%9, %c2_i32, %c0_i32_49, %c1_i32_50, %c4_i32, %c16_i32_51, %c4_i32_52, %36, %38, %33, %34, %c1_i32_53, %c0_i32_54, %c117_i32) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
      %40 = bufferization.to_tensor %subview_2 restrict writable : memref<16x16xf32, strided<[16, 1], offset: ?>> to tensor<16x16xf32>
      %base_buffer_55, %offset_56, %sizes_57:2, %strides_58:2 = memref.extract_strided_metadata %subview_2 : memref<16x16xf32, strided<[16, 1], offset: ?>> -> memref<f32>, index, index, index, index, index
      %intptr_59 = memref.extract_aligned_pointer_as_index %base_buffer_55 : memref<f32> -> index
      %c8_60 = arith.constant 8 : index
      %41 = arith.muli %strides_58#0, %c8_60 : index
      %42 = arith.addi %offset_56, %41 : index
      %c4_61 = arith.constant 4 : index
      %43 = arith.addi %42, %c4_61 : index
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
      %subview_66 = memref.subview %subview_1[0, 4] [16, 4] [1, 1] : memref<16x16xf32, strided<[16, 1], offset: ?>> to memref<16x4xf32, strided<[16, 1], offset: ?>>
      %reinterpret_cast_67 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
      memref.copy %subview_66, %reinterpret_cast_67 : memref<16x4xf32, strided<[16, 1], offset: ?>> to memref<16x4xf32>
      %intptr_68 = memref.extract_aligned_pointer_as_index %reinterpret_cast_67 : memref<16x4xf32> -> index
      %51 = arith.index_cast %intptr_68 : index to i64
      %52 = llvm.inttoptr %51 : i64 to !llvm.ptr
      %c3_i32 = arith.constant 3 : i32
      %c0_i32_69 = arith.constant 0 : i32
      %c1_i32_70 = arith.constant 1 : i32
      %c4_i32_71 = arith.constant 4 : i32
      %c16_i32_72 = arith.constant 16 : i32
      %c4_i32_73 = arith.constant 4 : i32
      %c2_i32_74 = arith.constant 2 : i32
      %c0_i32_75 = arith.constant 0 : i32
      %c117_i32_76 = arith.constant 117 : i32
      %53 = llvm.call @fpga_tile_submit(%9, %c3_i32, %c0_i32_69, %c1_i32_70, %c4_i32_71, %c16_i32_72, %c4_i32_73, %50, %52, %47, %48, %c2_i32_74, %c0_i32_75, %c117_i32_76) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
      %54 = bufferization.to_tensor %subview_2 restrict writable : memref<16x16xf32, strided<[16, 1], offset: ?>> to tensor<16x16xf32>
      %base_buffer_77, %offset_78, %sizes_79:2, %strides_80:2 = memref.extract_strided_metadata %subview_2 : memref<16x16xf32, strided<[16, 1], offset: ?>> -> memref<f32>, index, index, index, index, index
      %intptr_81 = memref.extract_aligned_pointer_as_index %base_buffer_77 : memref<f32> -> index
      %c8_82 = arith.constant 8 : index
      %55 = arith.muli %strides_80#0, %c8_82 : index
      %56 = arith.addi %offset_78, %55 : index
      %c8_83 = arith.constant 8 : index
      %57 = arith.addi %56, %c8_83 : index
      %c4_84 = arith.constant 4 : index
      %58 = arith.muli %57, %c4_84 : index
      %59 = arith.addi %intptr_81, %58 : index
      %60 = arith.index_cast %59 : index to i64
      %61 = llvm.inttoptr %60 : i64 to !llvm.ptr
      %62 = arith.index_cast %strides_80#0 : index to i32
      %subview_85 = memref.subview %subview[8, 0] [4, 16] [1, 1] : memref<16x16xf32, strided<[16, 1], offset: ?>> to memref<4x16xf32, strided<[16, 1], offset: ?>>
      %reinterpret_cast_86 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
      memref.copy %subview_85, %reinterpret_cast_86 : memref<4x16xf32, strided<[16, 1], offset: ?>> to memref<4x16xf32>
      %intptr_87 = memref.extract_aligned_pointer_as_index %reinterpret_cast_86 : memref<4x16xf32> -> index
      %63 = arith.index_cast %intptr_87 : index to i64
      %64 = llvm.inttoptr %63 : i64 to !llvm.ptr
      %subview_88 = memref.subview %subview_1[0, 8] [16, 4] [1, 1] : memref<16x16xf32, strided<[16, 1], offset: ?>> to memref<16x4xf32, strided<[16, 1], offset: ?>>
      %reinterpret_cast_89 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
      memref.copy %subview_88, %reinterpret_cast_89 : memref<16x4xf32, strided<[16, 1], offset: ?>> to memref<16x4xf32>
      %intptr_90 = memref.extract_aligned_pointer_as_index %reinterpret_cast_89 : memref<16x4xf32> -> index
      %65 = arith.index_cast %intptr_90 : index to i64
      %66 = llvm.inttoptr %65 : i64 to !llvm.ptr
      %c4_i32_91 = arith.constant 4 : i32
      %c0_i32_92 = arith.constant 0 : i32
      %c1_i32_93 = arith.constant 1 : i32
      %c4_i32_94 = arith.constant 4 : i32
      %c16_i32_95 = arith.constant 16 : i32
      %c4_i32_96 = arith.constant 4 : i32
      %c3_i32_97 = arith.constant 3 : i32
      %c0_i32_98 = arith.constant 0 : i32
      %c117_i32_99 = arith.constant 117 : i32
      %67 = llvm.call @fpga_tile_submit(%9, %c4_i32_91, %c0_i32_92, %c1_i32_93, %c4_i32_94, %c16_i32_95, %c4_i32_96, %64, %66, %61, %62, %c3_i32_97, %c0_i32_98, %c117_i32_99) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
      %68 = bufferization.to_tensor %subview_2 restrict writable : memref<16x16xf32, strided<[16, 1], offset: ?>> to tensor<16x16xf32>
      %base_buffer_100, %offset_101, %sizes_102:2, %strides_103:2 = memref.extract_strided_metadata %subview_2 : memref<16x16xf32, strided<[16, 1], offset: ?>> -> memref<f32>, index, index, index, index, index
      %intptr_104 = memref.extract_aligned_pointer_as_index %base_buffer_100 : memref<f32> -> index
      %c8_105 = arith.constant 8 : index
      %69 = arith.muli %strides_103#0, %c8_105 : index
      %70 = arith.addi %offset_101, %69 : index
      %c12 = arith.constant 12 : index
      %71 = arith.addi %70, %c12 : index
      %c4_106 = arith.constant 4 : index
      %72 = arith.muli %71, %c4_106 : index
      %73 = arith.addi %intptr_104, %72 : index
      %74 = arith.index_cast %73 : index to i64
      %75 = llvm.inttoptr %74 : i64 to !llvm.ptr
      %76 = arith.index_cast %strides_103#0 : index to i32
      %subview_107 = memref.subview %subview[8, 0] [4, 16] [1, 1] : memref<16x16xf32, strided<[16, 1], offset: ?>> to memref<4x16xf32, strided<[16, 1], offset: ?>>
      %reinterpret_cast_108 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
      memref.copy %subview_107, %reinterpret_cast_108 : memref<4x16xf32, strided<[16, 1], offset: ?>> to memref<4x16xf32>
      %intptr_109 = memref.extract_aligned_pointer_as_index %reinterpret_cast_108 : memref<4x16xf32> -> index
      %77 = arith.index_cast %intptr_109 : index to i64
      %78 = llvm.inttoptr %77 : i64 to !llvm.ptr
      %subview_110 = memref.subview %subview_1[0, 12] [16, 4] [1, 1] : memref<16x16xf32, strided<[16, 1], offset: ?>> to memref<16x4xf32, strided<[16, 1], offset: ?>>
      %reinterpret_cast_111 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
      memref.copy %subview_110, %reinterpret_cast_111 : memref<16x4xf32, strided<[16, 1], offset: ?>> to memref<16x4xf32>
      %intptr_112 = memref.extract_aligned_pointer_as_index %reinterpret_cast_111 : memref<16x4xf32> -> index
      %79 = arith.index_cast %intptr_112 : index to i64
      %80 = llvm.inttoptr %79 : i64 to !llvm.ptr
      %c5_i32 = arith.constant 5 : i32
      %c0_i32_113 = arith.constant 0 : i32
      %c1_i32_114 = arith.constant 1 : i32
      %c4_i32_115 = arith.constant 4 : i32
      %c16_i32_116 = arith.constant 16 : i32
      %c4_i32_117 = arith.constant 4 : i32
      %c1_i32_118 = arith.constant 1 : i32
      %c117_i32_119 = arith.constant 117 : i32
      %c117_i32_120 = arith.constant 117 : i32
      %81 = llvm.call @fpga_tile_submit(%9, %c5_i32, %c0_i32_113, %c1_i32_114, %c4_i32_115, %c16_i32_116, %c4_i32_117, %78, %80, %75, %76, %c1_i32_118, %c117_i32_119, %c117_i32_120) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
      %82 = bufferization.to_tensor %subview_2 restrict writable : memref<16x16xf32, strided<[16, 1], offset: ?>> to tensor<16x16xf32>
      %base_buffer_121, %offset_122, %sizes_123:2, %strides_124:2 = memref.extract_strided_metadata %subview_2 : memref<16x16xf32, strided<[16, 1], offset: ?>> -> memref<f32>, index, index, index, index, index
      %intptr_125 = memref.extract_aligned_pointer_as_index %base_buffer_121 : memref<f32> -> index
      %c12_126 = arith.constant 12 : index
      %83 = arith.muli %strides_124#0, %c12_126 : index
      %84 = arith.addi %offset_122, %83 : index
      %c4_127 = arith.constant 4 : index
      %85 = arith.muli %84, %c4_127 : index
      %86 = arith.addi %intptr_125, %85 : index
      %87 = arith.index_cast %86 : index to i64
      %88 = llvm.inttoptr %87 : i64 to !llvm.ptr
      %89 = arith.index_cast %strides_124#0 : index to i32
      %subview_128 = memref.subview %subview[12, 0] [4, 16] [1, 1] : memref<16x16xf32, strided<[16, 1], offset: ?>> to memref<4x16xf32, strided<[16, 1], offset: ?>>
      %reinterpret_cast_129 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
      memref.copy %subview_128, %reinterpret_cast_129 : memref<4x16xf32, strided<[16, 1], offset: ?>> to memref<4x16xf32>
      %intptr_130 = memref.extract_aligned_pointer_as_index %reinterpret_cast_129 : memref<4x16xf32> -> index
      %90 = arith.index_cast %intptr_130 : index to i64
      %91 = llvm.inttoptr %90 : i64 to !llvm.ptr
      %subview_131 = memref.subview %subview_1[0, 0] [16, 4] [1, 1] : memref<16x16xf32, strided<[16, 1], offset: ?>> to memref<16x4xf32, strided<[16, 1], offset: ?>>
      %reinterpret_cast_132 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
      memref.copy %subview_131, %reinterpret_cast_132 : memref<16x4xf32, strided<[16, 1], offset: ?>> to memref<16x4xf32>
      %intptr_133 = memref.extract_aligned_pointer_as_index %reinterpret_cast_132 : memref<16x4xf32> -> index
      %92 = arith.index_cast %intptr_133 : index to i64
      %93 = llvm.inttoptr %92 : i64 to !llvm.ptr
      %c6_i32 = arith.constant 6 : i32
      %c0_i32_134 = arith.constant 0 : i32
      %c1_i32_135 = arith.constant 1 : i32
      %c4_i32_136 = arith.constant 4 : i32
      %c16_i32_137 = arith.constant 16 : i32
      %c4_i32_138 = arith.constant 4 : i32
      %c2_i32_139 = arith.constant 2 : i32
      %c117_i32_140 = arith.constant 117 : i32
      %c117_i32_141 = arith.constant 117 : i32
      %94 = llvm.call @fpga_tile_submit(%9, %c6_i32, %c0_i32_134, %c1_i32_135, %c4_i32_136, %c16_i32_137, %c4_i32_138, %91, %93, %88, %89, %c2_i32_139, %c117_i32_140, %c117_i32_141) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
      %95 = bufferization.to_tensor %subview_2 restrict writable : memref<16x16xf32, strided<[16, 1], offset: ?>> to tensor<16x16xf32>
      %base_buffer_142, %offset_143, %sizes_144:2, %strides_145:2 = memref.extract_strided_metadata %subview_2 : memref<16x16xf32, strided<[16, 1], offset: ?>> -> memref<f32>, index, index, index, index, index
      %intptr_146 = memref.extract_aligned_pointer_as_index %base_buffer_142 : memref<f32> -> index
      %c12_147 = arith.constant 12 : index
      %96 = arith.muli %strides_145#0, %c12_147 : index
      %97 = arith.addi %offset_143, %96 : index
      %c4_148 = arith.constant 4 : index
      %98 = arith.addi %97, %c4_148 : index
      %c4_149 = arith.constant 4 : index
      %99 = arith.muli %98, %c4_149 : index
      %100 = arith.addi %intptr_146, %99 : index
      %101 = arith.index_cast %100 : index to i64
      %102 = llvm.inttoptr %101 : i64 to !llvm.ptr
      %103 = arith.index_cast %strides_145#0 : index to i32
      %subview_150 = memref.subview %subview[12, 0] [4, 16] [1, 1] : memref<16x16xf32, strided<[16, 1], offset: ?>> to memref<4x16xf32, strided<[16, 1], offset: ?>>
      %reinterpret_cast_151 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
      memref.copy %subview_150, %reinterpret_cast_151 : memref<4x16xf32, strided<[16, 1], offset: ?>> to memref<4x16xf32>
      %intptr_152 = memref.extract_aligned_pointer_as_index %reinterpret_cast_151 : memref<4x16xf32> -> index
      %104 = arith.index_cast %intptr_152 : index to i64
      %105 = llvm.inttoptr %104 : i64 to !llvm.ptr
      %subview_153 = memref.subview %subview_1[0, 4] [16, 4] [1, 1] : memref<16x16xf32, strided<[16, 1], offset: ?>> to memref<16x4xf32, strided<[16, 1], offset: ?>>
      %reinterpret_cast_154 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
      memref.copy %subview_153, %reinterpret_cast_154 : memref<16x4xf32, strided<[16, 1], offset: ?>> to memref<16x4xf32>
      %intptr_155 = memref.extract_aligned_pointer_as_index %reinterpret_cast_154 : memref<16x4xf32> -> index
      %106 = arith.index_cast %intptr_155 : index to i64
      %107 = llvm.inttoptr %106 : i64 to !llvm.ptr
      %c7_i32 = arith.constant 7 : i32
      %c0_i32_156 = arith.constant 0 : i32
      %c1_i32_157 = arith.constant 1 : i32
      %c4_i32_158 = arith.constant 4 : i32
      %c16_i32_159 = arith.constant 16 : i32
      %c4_i32_160 = arith.constant 4 : i32
      %c3_i32_161 = arith.constant 3 : i32
      %c117_i32_162 = arith.constant 117 : i32
      %c117_i32_163 = arith.constant 117 : i32
      %108 = llvm.call @fpga_tile_submit(%9, %c7_i32, %c0_i32_156, %c1_i32_157, %c4_i32_158, %c16_i32_159, %c4_i32_160, %105, %107, %102, %103, %c3_i32_161, %c117_i32_162, %c117_i32_163) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
      %109 = bufferization.to_tensor %subview_2 restrict writable : memref<16x16xf32, strided<[16, 1], offset: ?>> to tensor<16x16xf32>
      %base_buffer_164, %offset_165, %sizes_166:2, %strides_167:2 = memref.extract_strided_metadata %subview_2 : memref<16x16xf32, strided<[16, 1], offset: ?>> -> memref<f32>, index, index, index, index, index
      %intptr_168 = memref.extract_aligned_pointer_as_index %base_buffer_164 : memref<f32> -> index
      %c12_169 = arith.constant 12 : index
      %110 = arith.muli %strides_167#0, %c12_169 : index
      %111 = arith.addi %offset_165, %110 : index
      %c8_170 = arith.constant 8 : index
      %112 = arith.addi %111, %c8_170 : index
      %c4_171 = arith.constant 4 : index
      %113 = arith.muli %112, %c4_171 : index
      %114 = arith.addi %intptr_168, %113 : index
      %115 = arith.index_cast %114 : index to i64
      %116 = llvm.inttoptr %115 : i64 to !llvm.ptr
      %117 = arith.index_cast %strides_167#0 : index to i32
      %subview_172 = memref.subview %subview[12, 0] [4, 16] [1, 1] : memref<16x16xf32, strided<[16, 1], offset: ?>> to memref<4x16xf32, strided<[16, 1], offset: ?>>
      %reinterpret_cast_173 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
      memref.copy %subview_172, %reinterpret_cast_173 : memref<4x16xf32, strided<[16, 1], offset: ?>> to memref<4x16xf32>
      %intptr_174 = memref.extract_aligned_pointer_as_index %reinterpret_cast_173 : memref<4x16xf32> -> index
      %118 = arith.index_cast %intptr_174 : index to i64
      %119 = llvm.inttoptr %118 : i64 to !llvm.ptr
      %subview_175 = memref.subview %subview_1[0, 8] [16, 4] [1, 1] : memref<16x16xf32, strided<[16, 1], offset: ?>> to memref<16x4xf32, strided<[16, 1], offset: ?>>
      %reinterpret_cast_176 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
      memref.copy %subview_175, %reinterpret_cast_176 : memref<16x4xf32, strided<[16, 1], offset: ?>> to memref<16x4xf32>
      %intptr_177 = memref.extract_aligned_pointer_as_index %reinterpret_cast_176 : memref<16x4xf32> -> index
      %120 = arith.index_cast %intptr_177 : index to i64
      %121 = llvm.inttoptr %120 : i64 to !llvm.ptr
      %c8_i32_178 = arith.constant 8 : i32
      %c0_i32_179 = arith.constant 0 : i32
      %c1_i32_180 = arith.constant 1 : i32
      %c4_i32_181 = arith.constant 4 : i32
      %c16_i32_182 = arith.constant 16 : i32
      %c4_i32_183 = arith.constant 4 : i32
      %c1_i32_184 = arith.constant 1 : i32
      %c234_i32 = arith.constant 234 : i32
      %c117_i32_185 = arith.constant 117 : i32
      %122 = llvm.call @fpga_tile_submit(%9, %c8_i32_178, %c0_i32_179, %c1_i32_180, %c4_i32_181, %c16_i32_182, %c4_i32_183, %119, %121, %116, %117, %c1_i32_184, %c234_i32, %c117_i32_185) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
      %123 = bufferization.to_tensor %subview_2 restrict writable : memref<16x16xf32, strided<[16, 1], offset: ?>> to tensor<16x16xf32>
      %base_buffer_186, %offset_187, %sizes_188:2, %strides_189:2 = memref.extract_strided_metadata %subview_2 : memref<16x16xf32, strided<[16, 1], offset: ?>> -> memref<f32>, index, index, index, index, index
      %intptr_190 = memref.extract_aligned_pointer_as_index %base_buffer_186 : memref<f32> -> index
      %c12_191 = arith.constant 12 : index
      %124 = arith.muli %strides_189#0, %c12_191 : index
      %125 = arith.addi %offset_187, %124 : index
      %c12_192 = arith.constant 12 : index
      %126 = arith.addi %125, %c12_192 : index
      %c4_193 = arith.constant 4 : index
      %127 = arith.muli %126, %c4_193 : index
      %128 = arith.addi %intptr_190, %127 : index
      %129 = arith.index_cast %128 : index to i64
      %130 = llvm.inttoptr %129 : i64 to !llvm.ptr
      %131 = arith.index_cast %strides_189#0 : index to i32
      %subview_194 = memref.subview %subview[12, 0] [4, 16] [1, 1] : memref<16x16xf32, strided<[16, 1], offset: ?>> to memref<4x16xf32, strided<[16, 1], offset: ?>>
      %reinterpret_cast_195 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
      memref.copy %subview_194, %reinterpret_cast_195 : memref<4x16xf32, strided<[16, 1], offset: ?>> to memref<4x16xf32>
      %intptr_196 = memref.extract_aligned_pointer_as_index %reinterpret_cast_195 : memref<4x16xf32> -> index
      %132 = arith.index_cast %intptr_196 : index to i64
      %133 = llvm.inttoptr %132 : i64 to !llvm.ptr
      %subview_197 = memref.subview %subview_1[0, 12] [16, 4] [1, 1] : memref<16x16xf32, strided<[16, 1], offset: ?>> to memref<16x4xf32, strided<[16, 1], offset: ?>>
      %reinterpret_cast_198 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
      memref.copy %subview_197, %reinterpret_cast_198 : memref<16x4xf32, strided<[16, 1], offset: ?>> to memref<16x4xf32>
      %intptr_199 = memref.extract_aligned_pointer_as_index %reinterpret_cast_198 : memref<16x4xf32> -> index
      %134 = arith.index_cast %intptr_199 : index to i64
      %135 = llvm.inttoptr %134 : i64 to !llvm.ptr
      %c9_i32 = arith.constant 9 : i32
      %c0_i32_200 = arith.constant 0 : i32
      %c1_i32_201 = arith.constant 1 : i32
      %c4_i32_202 = arith.constant 4 : i32
      %c16_i32_203 = arith.constant 16 : i32
      %c4_i32_204 = arith.constant 4 : i32
      %c2_i32_205 = arith.constant 2 : i32
      %c234_i32_206 = arith.constant 234 : i32
      %c117_i32_207 = arith.constant 117 : i32
      %136 = llvm.call @fpga_tile_submit(%9, %c9_i32, %c0_i32_200, %c1_i32_201, %c4_i32_202, %c16_i32_203, %c4_i32_204, %133, %135, %130, %131, %c2_i32_205, %c234_i32_206, %c117_i32_207) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
      %137 = bufferization.to_tensor %subview_2 restrict writable : memref<16x16xf32, strided<[16, 1], offset: ?>> to tensor<16x16xf32>
      %138 = llvm.call @fpga_gemm_wait(%9) : (!llvm.ptr) -> i32
      bufferization.materialize_in_destination %137 in restrict writable %subview_2 : (tensor<16x16xf32>, memref<16x16xf32, strided<[16, 1], offset: ?>>) -> ()
    }
    return
  }
}

