module {
  llvm.func @fpga_gemm_wait(!llvm.ptr) -> i32
  llvm.func @fpga_tile_submit(!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
  llvm.func @fpga_gemm_begin(i32, i32, i32, i32) -> !llvm.ptr
  func.func @expressions__systolic_kernel(%arg0: memref<?x10xf32> {llvm.noalias}, %arg1: memref<?x4xf32> {llvm.noalias}, %arg2: memref<?x8xf32> {llvm.noalias}) {
    %alloca = memref.alloca() : memref<128xf32>
    %alloca_0 = memref.alloca() : memref<128xf32>
    %subview = memref.subview %arg0[1, 0] [4, 4] [2, 3] : memref<?x10xf32> to memref<4x4xf32, strided<[20, 3], offset: 10>>
    %0 = bufferization.to_tensor %subview restrict : memref<4x4xf32, strided<[20, 3], offset: 10>> to tensor<4x4xf32>
    %subview_1 = memref.subview %arg1[2, 0] [4, 4] [6, 1] : memref<?x4xf32> to memref<4x4xf32, strided<[24, 1], offset: 8>>
    %1 = bufferization.to_tensor %subview_1 restrict : memref<4x4xf32, strided<[24, 1], offset: 8>> to tensor<4x4xf32>
    %subview_2 = memref.subview %arg2[0, 1] [4, 4] [4, 2] : memref<?x8xf32> to memref<4x4xf32, strided<[32, 2], offset: 1>>
    %2 = bufferization.to_tensor %subview_2 restrict writable : memref<4x4xf32, strided<[32, 2], offset: 1>> to tensor<4x4xf32>
    %extracted_slice = tensor.extract_slice %2[0, 0] [4, 4] [1, 1] : tensor<4x4xf32> to tensor<4x4xf32>
    %c4_i32 = arith.constant 4 : i32
    %c4_i32_3 = arith.constant 4 : i32
    %c4_i32_4 = arith.constant 4 : i32
    %c1_i32 = arith.constant 1 : i32
    %3 = llvm.call @fpga_gemm_begin(%c4_i32, %c4_i32_3, %c4_i32_4, %c1_i32) : (i32, i32, i32, i32) -> !llvm.ptr
    %4 = bufferization.to_buffer %extracted_slice : tensor<4x4xf32> to memref<4x4xf32>
    %c4_i32_5 = arith.constant 4 : i32
    %intptr = memref.extract_aligned_pointer_as_index %4 : memref<4x4xf32> -> index
    %5 = arith.index_cast %intptr : index to i64
    %6 = llvm.inttoptr %5 : i64 to !llvm.ptr
    %subview_6 = memref.subview %subview[0, 0] [4, 4] [1, 1] : memref<4x4xf32, strided<[20, 3], offset: 10>> to memref<4x4xf32, strided<[20, 3], offset: 10>>
    %reinterpret_cast = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<128xf32> to memref<4x4xf32>
    memref.copy %subview_6, %reinterpret_cast : memref<4x4xf32, strided<[20, 3], offset: 10>> to memref<4x4xf32>
    %intptr_7 = memref.extract_aligned_pointer_as_index %reinterpret_cast : memref<4x4xf32> -> index
    %7 = arith.index_cast %intptr_7 : index to i64
    %8 = llvm.inttoptr %7 : i64 to !llvm.ptr
    %subview_8 = memref.subview %subview_1[0, 0] [4, 4] [1, 1] : memref<4x4xf32, strided<[24, 1], offset: 8>> to memref<4x4xf32, strided<[24, 1], offset: 8>>
    %reinterpret_cast_9 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<128xf32> to memref<4x4xf32>
    memref.copy %subview_8, %reinterpret_cast_9 : memref<4x4xf32, strided<[24, 1], offset: 8>> to memref<4x4xf32>
    %intptr_10 = memref.extract_aligned_pointer_as_index %reinterpret_cast_9 : memref<4x4xf32> -> index
    %9 = arith.index_cast %intptr_10 : index to i64
    %10 = llvm.inttoptr %9 : i64 to !llvm.ptr
    %c0_i32 = arith.constant 0 : i32
    %c0_i32_11 = arith.constant 0 : i32
    %c1_i32_12 = arith.constant 1 : i32
    %c4_i32_13 = arith.constant 4 : i32
    %c4_i32_14 = arith.constant 4 : i32
    %c4_i32_15 = arith.constant 4 : i32
    %c1_i32_16 = arith.constant 1 : i32
    %c0_i32_17 = arith.constant 0 : i32
    %c105_i32 = arith.constant 105 : i32
    %11 = llvm.call @fpga_tile_submit(%3, %c0_i32, %c0_i32_11, %c1_i32_12, %c4_i32_13, %c4_i32_14, %c4_i32_15, %8, %10, %6, %c4_i32_5, %c1_i32_16, %c0_i32_17, %c105_i32) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %12 = bufferization.to_tensor %4 restrict writable : memref<4x4xf32> to tensor<4x4xf32>
    %13 = llvm.call @fpga_gemm_wait(%3) : (!llvm.ptr) -> i32
    %inserted_slice = tensor.insert_slice %12 into %2[0, 0] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<4x4xf32>
    bufferization.materialize_in_destination %inserted_slice in restrict writable %subview_2 : (tensor<4x4xf32>, memref<4x4xf32, strided<[32, 2], offset: 1>>) -> ()
    return
  }
  func.func @runs_once__systolic_kernel(%arg0: memref<?x8xf32> {llvm.noalias}, %arg1: memref<?x10xf32> {llvm.noalias}, %arg2: memref<?x11xf32> {llvm.noalias}) {
    %alloca = memref.alloca() : memref<128xf32>
    %alloca_0 = memref.alloca() : memref<128xf32>
    %subview = memref.subview %arg0[6, 0] [8, 8] [1, 1] : memref<?x8xf32> to memref<8x8xf32, strided<[8, 1], offset: 48>>
    %0 = bufferization.to_tensor %subview restrict : memref<8x8xf32, strided<[8, 1], offset: 48>> to tensor<8x8xf32>
    %subview_1 = memref.subview %arg1[0, 2] [8, 8] [1, 1] : memref<?x10xf32> to memref<8x8xf32, strided<[10, 1], offset: 2>>
    %1 = bufferization.to_tensor %subview_1 restrict : memref<8x8xf32, strided<[10, 1], offset: 2>> to tensor<8x8xf32>
    %subview_2 = memref.subview %arg2[3, 3] [8, 8] [1, 1] : memref<?x11xf32> to memref<8x8xf32, strided<[11, 1], offset: 36>>
    %2 = bufferization.to_tensor %subview_2 restrict writable : memref<8x8xf32, strided<[11, 1], offset: 36>> to tensor<8x8xf32>
    %c11_i32 = arith.constant 11 : i32
    %intptr = memref.extract_aligned_pointer_as_index %subview_2 : memref<8x8xf32, strided<[11, 1], offset: 36>> -> index
    %c144 = arith.constant 144 : index
    %3 = arith.addi %intptr, %c144 : index
    %4 = arith.index_cast %3 : index to i64
    %5 = llvm.inttoptr %4 : i64 to !llvm.ptr
    %c8_i32 = arith.constant 8 : i32
    %c8_i32_3 = arith.constant 8 : i32
    %c8_i32_4 = arith.constant 8 : i32
    %c1_i32 = arith.constant 1 : i32
    %6 = llvm.call @fpga_gemm_begin(%c8_i32, %c8_i32_3, %c8_i32_4, %c1_i32) : (i32, i32, i32, i32) -> !llvm.ptr
    %subview_5 = memref.subview %subview[0, 0] [8, 8] [1, 1] : memref<8x8xf32, strided<[8, 1], offset: 48>> to memref<8x8xf32, strided<[8, 1], offset: 48>>
    %reinterpret_cast = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 8], strides: [8, 1] : memref<128xf32> to memref<8x8xf32>
    memref.copy %subview_5, %reinterpret_cast : memref<8x8xf32, strided<[8, 1], offset: 48>> to memref<8x8xf32>
    %intptr_6 = memref.extract_aligned_pointer_as_index %reinterpret_cast : memref<8x8xf32> -> index
    %7 = arith.index_cast %intptr_6 : index to i64
    %8 = llvm.inttoptr %7 : i64 to !llvm.ptr
    %subview_7 = memref.subview %subview_1[0, 0] [8, 8] [1, 1] : memref<8x8xf32, strided<[10, 1], offset: 2>> to memref<8x8xf32, strided<[10, 1], offset: 2>>
    %reinterpret_cast_8 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [8, 8], strides: [8, 1] : memref<128xf32> to memref<8x8xf32>
    memref.copy %subview_7, %reinterpret_cast_8 : memref<8x8xf32, strided<[10, 1], offset: 2>> to memref<8x8xf32>
    %intptr_9 = memref.extract_aligned_pointer_as_index %reinterpret_cast_8 : memref<8x8xf32> -> index
    %9 = arith.index_cast %intptr_9 : index to i64
    %10 = llvm.inttoptr %9 : i64 to !llvm.ptr
    %c0_i32 = arith.constant 0 : i32
    %c0_i32_10 = arith.constant 0 : i32
    %c1_i32_11 = arith.constant 1 : i32
    %c8_i32_12 = arith.constant 8 : i32
    %c8_i32_13 = arith.constant 8 : i32
    %c8_i32_14 = arith.constant 8 : i32
    %c0_i32_15 = arith.constant 0 : i32
    %c0_i32_16 = arith.constant 0 : i32
    %c117_i32 = arith.constant 117 : i32
    %11 = llvm.call @fpga_tile_submit(%6, %c0_i32, %c0_i32_10, %c1_i32_11, %c8_i32_12, %c8_i32_13, %c8_i32_14, %8, %10, %5, %c11_i32, %c0_i32_15, %c0_i32_16, %c117_i32) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %12 = bufferization.to_tensor %subview_2 restrict writable : memref<8x8xf32, strided<[11, 1], offset: 36>> to tensor<8x8xf32>
    %13 = llvm.call @fpga_gemm_wait(%6) : (!llvm.ptr) -> i32
    bufferization.materialize_in_destination %12 in restrict writable %subview_2 : (tensor<8x8xf32>, memref<8x8xf32, strided<[11, 1], offset: 36>>) -> ()
    return
  }
  func.func @steps__systolic_kernel(%arg0: memref<?x15xf32> {llvm.noalias}, %arg1: memref<?x11xf32> {llvm.noalias}, %arg2: memref<?x11xf32> {llvm.noalias}) {
    %alloca = memref.alloca() : memref<128xf32>
    %alloca_0 = memref.alloca() : memref<128xf32>
    %subview = memref.subview %arg0[1, 0] [3, 8] [2, 2] : memref<?x15xf32> to memref<3x8xf32, strided<[30, 2], offset: 15>>
    %0 = bufferization.to_tensor %subview restrict : memref<3x8xf32, strided<[30, 2], offset: 15>> to tensor<3x8xf32>
    %subview_1 = memref.subview %arg1[0, 0] [8, 11] [2, 1] : memref<?x11xf32> to memref<8x11xf32, strided<[22, 1]>>
    %1 = bufferization.to_tensor %subview_1 restrict : memref<8x11xf32, strided<[22, 1]>> to tensor<8x11xf32>
    %subview_2 = memref.subview %arg2[1, 0] [3, 11] [2, 1] : memref<?x11xf32> to memref<3x11xf32, strided<[22, 1], offset: 11>>
    %2 = bufferization.to_tensor %subview_2 restrict writable : memref<3x11xf32, strided<[22, 1], offset: 11>> to tensor<3x11xf32>
    %c22_i32 = arith.constant 22 : i32
    %intptr = memref.extract_aligned_pointer_as_index %subview_2 : memref<3x11xf32, strided<[22, 1], offset: 11>> -> index
    %c44 = arith.constant 44 : index
    %3 = arith.addi %intptr, %c44 : index
    %4 = arith.index_cast %3 : index to i64
    %5 = llvm.inttoptr %4 : i64 to !llvm.ptr
    %c3_i32 = arith.constant 3 : i32
    %c11_i32 = arith.constant 11 : i32
    %c8_i32 = arith.constant 8 : i32
    %c3_i32_3 = arith.constant 3 : i32
    %6 = llvm.call @fpga_gemm_begin(%c3_i32, %c11_i32, %c8_i32, %c3_i32_3) : (i32, i32, i32, i32) -> !llvm.ptr
    %subview_4 = memref.subview %subview[0, 0] [3, 8] [1, 1] : memref<3x8xf32, strided<[30, 2], offset: 15>> to memref<3x8xf32, strided<[30, 2], offset: 15>>
    %reinterpret_cast = memref.reinterpret_cast %alloca to offset: [0], sizes: [3, 8], strides: [8, 1] : memref<128xf32> to memref<3x8xf32>
    memref.copy %subview_4, %reinterpret_cast : memref<3x8xf32, strided<[30, 2], offset: 15>> to memref<3x8xf32>
    %intptr_5 = memref.extract_aligned_pointer_as_index %reinterpret_cast : memref<3x8xf32> -> index
    %7 = arith.index_cast %intptr_5 : index to i64
    %8 = llvm.inttoptr %7 : i64 to !llvm.ptr
    %subview_6 = memref.subview %subview_1[0, 0] [8, 4] [1, 1] : memref<8x11xf32, strided<[22, 1]>> to memref<8x4xf32, strided<[22, 1]>>
    %reinterpret_cast_7 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [8, 4], strides: [4, 1] : memref<128xf32> to memref<8x4xf32>
    memref.copy %subview_6, %reinterpret_cast_7 : memref<8x4xf32, strided<[22, 1]>> to memref<8x4xf32>
    %intptr_8 = memref.extract_aligned_pointer_as_index %reinterpret_cast_7 : memref<8x4xf32> -> index
    %9 = arith.index_cast %intptr_8 : index to i64
    %10 = llvm.inttoptr %9 : i64 to !llvm.ptr
    %c0_i32 = arith.constant 0 : i32
    %c0_i32_9 = arith.constant 0 : i32
    %c1_i32 = arith.constant 1 : i32
    %c3_i32_10 = arith.constant 3 : i32
    %c8_i32_11 = arith.constant 8 : i32
    %c4_i32 = arith.constant 4 : i32
    %c1_i32_12 = arith.constant 1 : i32
    %c0_i32_13 = arith.constant 0 : i32
    %c109_i32 = arith.constant 109 : i32
    %11 = llvm.call @fpga_tile_submit(%6, %c0_i32, %c0_i32_9, %c1_i32, %c3_i32_10, %c8_i32_11, %c4_i32, %8, %10, %5, %c22_i32, %c1_i32_12, %c0_i32_13, %c109_i32) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %12 = bufferization.to_tensor %subview_2 restrict writable : memref<3x11xf32, strided<[22, 1], offset: 11>> to tensor<3x11xf32>
    %c22_i32_14 = arith.constant 22 : i32
    %intptr_15 = memref.extract_aligned_pointer_as_index %subview_2 : memref<3x11xf32, strided<[22, 1], offset: 11>> -> index
    %c60 = arith.constant 60 : index
    %13 = arith.addi %intptr_15, %c60 : index
    %14 = arith.index_cast %13 : index to i64
    %15 = llvm.inttoptr %14 : i64 to !llvm.ptr
    %subview_16 = memref.subview %subview[0, 0] [3, 8] [1, 1] : memref<3x8xf32, strided<[30, 2], offset: 15>> to memref<3x8xf32, strided<[30, 2], offset: 15>>
    %reinterpret_cast_17 = memref.reinterpret_cast %alloca to offset: [0], sizes: [3, 8], strides: [8, 1] : memref<128xf32> to memref<3x8xf32>
    memref.copy %subview_16, %reinterpret_cast_17 : memref<3x8xf32, strided<[30, 2], offset: 15>> to memref<3x8xf32>
    %intptr_18 = memref.extract_aligned_pointer_as_index %reinterpret_cast_17 : memref<3x8xf32> -> index
    %16 = arith.index_cast %intptr_18 : index to i64
    %17 = llvm.inttoptr %16 : i64 to !llvm.ptr
    %subview_19 = memref.subview %subview_1[0, 4] [8, 4] [1, 1] : memref<8x11xf32, strided<[22, 1]>> to memref<8x4xf32, strided<[22, 1], offset: 4>>
    %reinterpret_cast_20 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [8, 4], strides: [4, 1] : memref<128xf32> to memref<8x4xf32>
    memref.copy %subview_19, %reinterpret_cast_20 : memref<8x4xf32, strided<[22, 1], offset: 4>> to memref<8x4xf32>
    %intptr_21 = memref.extract_aligned_pointer_as_index %reinterpret_cast_20 : memref<8x4xf32> -> index
    %18 = arith.index_cast %intptr_21 : index to i64
    %19 = llvm.inttoptr %18 : i64 to !llvm.ptr
    %c1_i32_22 = arith.constant 1 : i32
    %c0_i32_23 = arith.constant 0 : i32
    %c1_i32_24 = arith.constant 1 : i32
    %c3_i32_25 = arith.constant 3 : i32
    %c8_i32_26 = arith.constant 8 : i32
    %c4_i32_27 = arith.constant 4 : i32
    %c2_i32 = arith.constant 2 : i32
    %c0_i32_28 = arith.constant 0 : i32
    %c109_i32_29 = arith.constant 109 : i32
    %20 = llvm.call @fpga_tile_submit(%6, %c1_i32_22, %c0_i32_23, %c1_i32_24, %c3_i32_25, %c8_i32_26, %c4_i32_27, %17, %19, %15, %c22_i32_14, %c2_i32, %c0_i32_28, %c109_i32_29) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %21 = bufferization.to_tensor %subview_2 restrict writable : memref<3x11xf32, strided<[22, 1], offset: 11>> to tensor<3x11xf32>
    %c22_i32_30 = arith.constant 22 : i32
    %intptr_31 = memref.extract_aligned_pointer_as_index %subview_2 : memref<3x11xf32, strided<[22, 1], offset: 11>> -> index
    %c76 = arith.constant 76 : index
    %22 = arith.addi %intptr_31, %c76 : index
    %23 = arith.index_cast %22 : index to i64
    %24 = llvm.inttoptr %23 : i64 to !llvm.ptr
    %subview_32 = memref.subview %subview[0, 0] [3, 8] [1, 1] : memref<3x8xf32, strided<[30, 2], offset: 15>> to memref<3x8xf32, strided<[30, 2], offset: 15>>
    %reinterpret_cast_33 = memref.reinterpret_cast %alloca to offset: [0], sizes: [3, 8], strides: [8, 1] : memref<128xf32> to memref<3x8xf32>
    memref.copy %subview_32, %reinterpret_cast_33 : memref<3x8xf32, strided<[30, 2], offset: 15>> to memref<3x8xf32>
    %intptr_34 = memref.extract_aligned_pointer_as_index %reinterpret_cast_33 : memref<3x8xf32> -> index
    %25 = arith.index_cast %intptr_34 : index to i64
    %26 = llvm.inttoptr %25 : i64 to !llvm.ptr
    %subview_35 = memref.subview %subview_1[0, 8] [8, 3] [1, 1] : memref<8x11xf32, strided<[22, 1]>> to memref<8x3xf32, strided<[22, 1], offset: 8>>
    %reinterpret_cast_36 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [8, 3], strides: [3, 1] : memref<128xf32> to memref<8x3xf32>
    memref.copy %subview_35, %reinterpret_cast_36 : memref<8x3xf32, strided<[22, 1], offset: 8>> to memref<8x3xf32>
    %intptr_37 = memref.extract_aligned_pointer_as_index %reinterpret_cast_36 : memref<8x3xf32> -> index
    %27 = arith.index_cast %intptr_37 : index to i64
    %28 = llvm.inttoptr %27 : i64 to !llvm.ptr
    %c2_i32_38 = arith.constant 2 : i32
    %c0_i32_39 = arith.constant 0 : i32
    %c1_i32_40 = arith.constant 1 : i32
    %c3_i32_41 = arith.constant 3 : i32
    %c8_i32_42 = arith.constant 8 : i32
    %c3_i32_43 = arith.constant 3 : i32
    %c3_i32_44 = arith.constant 3 : i32
    %c0_i32_45 = arith.constant 0 : i32
    %c109_i32_46 = arith.constant 109 : i32
    %29 = llvm.call @fpga_tile_submit(%6, %c2_i32_38, %c0_i32_39, %c1_i32_40, %c3_i32_41, %c8_i32_42, %c3_i32_43, %26, %28, %24, %c22_i32_30, %c3_i32_44, %c0_i32_45, %c109_i32_46) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %30 = bufferization.to_tensor %subview_2 restrict writable : memref<3x11xf32, strided<[22, 1], offset: 11>> to tensor<3x11xf32>
    %31 = llvm.call @fpga_gemm_wait(%6) : (!llvm.ptr) -> i32
    bufferization.materialize_in_destination %30 in restrict writable %subview_2 : (tensor<3x11xf32>, memref<3x11xf32, strided<[22, 1], offset: 11>>) -> ()
    return
  }
  func.func @split__systolic_kernel(%arg0: memref<?x8xf32> {llvm.noalias}, %arg1: memref<?x8xf32> {llvm.noalias}, %arg2: memref<?x8xf32> {llvm.noalias}) {
    %alloca = memref.alloca() : memref<128xf32>
    %alloca_0 = memref.alloca() : memref<128xf32>
    %subview = memref.subview %arg0[1, 0] [8, 8] [2, 1] : memref<?x8xf32> to memref<8x8xf32, strided<[16, 1], offset: 8>>
    %0 = bufferization.to_tensor %subview restrict : memref<8x8xf32, strided<[16, 1], offset: 8>> to tensor<8x8xf32>
    %subview_1 = memref.subview %arg1[0, 0] [8, 8] [1, 1] : memref<?x8xf32> to memref<8x8xf32, strided<[8, 1]>>
    %1 = bufferization.to_tensor %subview_1 restrict : memref<8x8xf32, strided<[8, 1]>> to tensor<8x8xf32>
    %subview_2 = memref.subview %arg2[5, 0] [8, 8] [4, 1] : memref<?x8xf32> to memref<8x8xf32, strided<[32, 1], offset: 40>>
    %2 = bufferization.to_tensor %subview_2 restrict writable : memref<8x8xf32, strided<[32, 1], offset: 40>> to tensor<8x8xf32>
    %c32_i32 = arith.constant 32 : i32
    %intptr = memref.extract_aligned_pointer_as_index %subview_2 : memref<8x8xf32, strided<[32, 1], offset: 40>> -> index
    %c160 = arith.constant 160 : index
    %3 = arith.addi %intptr, %c160 : index
    %4 = arith.index_cast %3 : index to i64
    %5 = llvm.inttoptr %4 : i64 to !llvm.ptr
    %c8_i32 = arith.constant 8 : i32
    %c8_i32_3 = arith.constant 8 : i32
    %c8_i32_4 = arith.constant 8 : i32
    %c1_i32 = arith.constant 1 : i32
    %6 = llvm.call @fpga_gemm_begin(%c8_i32, %c8_i32_3, %c8_i32_4, %c1_i32) : (i32, i32, i32, i32) -> !llvm.ptr
    %subview_5 = memref.subview %subview[0, 0] [8, 8] [1, 1] : memref<8x8xf32, strided<[16, 1], offset: 8>> to memref<8x8xf32, strided<[16, 1], offset: 8>>
    %reinterpret_cast = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 8], strides: [8, 1] : memref<128xf32> to memref<8x8xf32>
    memref.copy %subview_5, %reinterpret_cast : memref<8x8xf32, strided<[16, 1], offset: 8>> to memref<8x8xf32>
    %intptr_6 = memref.extract_aligned_pointer_as_index %reinterpret_cast : memref<8x8xf32> -> index
    %7 = arith.index_cast %intptr_6 : index to i64
    %8 = llvm.inttoptr %7 : i64 to !llvm.ptr
    %subview_7 = memref.subview %subview_1[0, 0] [8, 8] [1, 1] : memref<8x8xf32, strided<[8, 1]>> to memref<8x8xf32, strided<[8, 1]>>
    %reinterpret_cast_8 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [8, 8], strides: [8, 1] : memref<128xf32> to memref<8x8xf32>
    memref.copy %subview_7, %reinterpret_cast_8 : memref<8x8xf32, strided<[8, 1]>> to memref<8x8xf32>
    %intptr_9 = memref.extract_aligned_pointer_as_index %reinterpret_cast_8 : memref<8x8xf32> -> index
    %9 = arith.index_cast %intptr_9 : index to i64
    %10 = llvm.inttoptr %9 : i64 to !llvm.ptr
    %c0_i32 = arith.constant 0 : i32
    %c0_i32_10 = arith.constant 0 : i32
    %c1_i32_11 = arith.constant 1 : i32
    %c8_i32_12 = arith.constant 8 : i32
    %c8_i32_13 = arith.constant 8 : i32
    %c8_i32_14 = arith.constant 8 : i32
    %c0_i32_15 = arith.constant 0 : i32
    %c0_i32_16 = arith.constant 0 : i32
    %c117_i32 = arith.constant 117 : i32
    %11 = llvm.call @fpga_tile_submit(%6, %c0_i32, %c0_i32_10, %c1_i32_11, %c8_i32_12, %c8_i32_13, %c8_i32_14, %8, %10, %5, %c32_i32, %c0_i32_15, %c0_i32_16, %c117_i32) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %12 = bufferization.to_tensor %subview_2 restrict writable : memref<8x8xf32, strided<[32, 1], offset: 40>> to tensor<8x8xf32>
    %13 = llvm.call @fpga_gemm_wait(%6) : (!llvm.ptr) -> i32
    bufferization.materialize_in_destination %12 in restrict writable %subview_2 : (tensor<8x8xf32>, memref<8x8xf32, strided<[32, 1], offset: 40>>) -> ()
    return
  }
  func.func @near_max__systolic_kernel(%arg0: memref<?x8xf32> {llvm.noalias}, %arg1: memref<?x8xf32> {llvm.noalias}, %arg2: memref<?x8xf32> {llvm.noalias}) {
    %alloca = memref.alloca() : memref<128xf32>
    %alloca_0 = memref.alloca() : memref<128xf32>
    %subview = memref.subview %arg0[0, 0] [7, 8] [1, 1] : memref<?x8xf32> to memref<7x8xf32, strided<[8, 1]>>
    %0 = bufferization.to_tensor %subview restrict : memref<7x8xf32, strided<[8, 1]>> to tensor<7x8xf32>
    %subview_1 = memref.subview %arg1[0, 0] [8, 8] [1, 1] : memref<?x8xf32> to memref<8x8xf32, strided<[8, 1]>>
    %1 = bufferization.to_tensor %subview_1 restrict : memref<8x8xf32, strided<[8, 1]>> to tensor<8x8xf32>
    %subview_2 = memref.subview %arg2[0, 0] [7, 8] [1, 1] : memref<?x8xf32> to memref<7x8xf32, strided<[8, 1]>>
    %2 = bufferization.to_tensor %subview_2 restrict writable : memref<7x8xf32, strided<[8, 1]>> to tensor<7x8xf32>
    %c8_i32 = arith.constant 8 : i32
    %intptr = memref.extract_aligned_pointer_as_index %subview_2 : memref<7x8xf32, strided<[8, 1]>> -> index
    %3 = arith.index_cast %intptr : index to i64
    %4 = llvm.inttoptr %3 : i64 to !llvm.ptr
    %c7_i32 = arith.constant 7 : i32
    %c8_i32_3 = arith.constant 8 : i32
    %c8_i32_4 = arith.constant 8 : i32
    %c1_i32 = arith.constant 1 : i32
    %5 = llvm.call @fpga_gemm_begin(%c7_i32, %c8_i32_3, %c8_i32_4, %c1_i32) : (i32, i32, i32, i32) -> !llvm.ptr
    %subview_5 = memref.subview %subview[0, 0] [7, 8] [1, 1] : memref<7x8xf32, strided<[8, 1]>> to memref<7x8xf32, strided<[8, 1]>>
    %reinterpret_cast = memref.reinterpret_cast %alloca to offset: [0], sizes: [7, 8], strides: [8, 1] : memref<128xf32> to memref<7x8xf32>
    memref.copy %subview_5, %reinterpret_cast : memref<7x8xf32, strided<[8, 1]>> to memref<7x8xf32>
    %intptr_6 = memref.extract_aligned_pointer_as_index %reinterpret_cast : memref<7x8xf32> -> index
    %6 = arith.index_cast %intptr_6 : index to i64
    %7 = llvm.inttoptr %6 : i64 to !llvm.ptr
    %subview_7 = memref.subview %subview_1[0, 0] [8, 8] [1, 1] : memref<8x8xf32, strided<[8, 1]>> to memref<8x8xf32, strided<[8, 1]>>
    %reinterpret_cast_8 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [8, 8], strides: [8, 1] : memref<128xf32> to memref<8x8xf32>
    memref.copy %subview_7, %reinterpret_cast_8 : memref<8x8xf32, strided<[8, 1]>> to memref<8x8xf32>
    %intptr_9 = memref.extract_aligned_pointer_as_index %reinterpret_cast_8 : memref<8x8xf32> -> index
    %8 = arith.index_cast %intptr_9 : index to i64
    %9 = llvm.inttoptr %8 : i64 to !llvm.ptr
    %c0_i32 = arith.constant 0 : i32
    %c0_i32_10 = arith.constant 0 : i32
    %c1_i32_11 = arith.constant 1 : i32
    %c7_i32_12 = arith.constant 7 : i32
    %c8_i32_13 = arith.constant 8 : i32
    %c8_i32_14 = arith.constant 8 : i32
    %c0_i32_15 = arith.constant 0 : i32
    %c0_i32_16 = arith.constant 0 : i32
    %c117_i32 = arith.constant 117 : i32
    %10 = llvm.call @fpga_tile_submit(%5, %c0_i32, %c0_i32_10, %c1_i32_11, %c7_i32_12, %c8_i32_13, %c8_i32_14, %7, %9, %4, %c8_i32, %c0_i32_15, %c0_i32_16, %c117_i32) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %11 = bufferization.to_tensor %subview_2 restrict writable : memref<7x8xf32, strided<[8, 1]>> to tensor<7x8xf32>
    %12 = llvm.call @fpga_gemm_wait(%5) : (!llvm.ptr) -> i32
    bufferization.materialize_in_destination %11 in restrict writable %subview_2 : (tensor<7x8xf32>, memref<7x8xf32, strided<[8, 1]>>) -> ()
    return
  }
}

