module {
  llvm.func @fpga_gemm_wait(!llvm.ptr) -> i32
  llvm.func @fpga_tile_submit(!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
  llvm.func @fpga_gemm_begin(i32, i32, i32, i32) -> !llvm.ptr
  func.func @systolic_matmul(%arg0: tensor<16x4xf32>, %arg1: tensor<4x8xf32>, %arg2: tensor<16x8xf32>) -> tensor<16x8xf32> attributes {llvm.emit_c_interface} {
    %0 = bufferization.to_buffer %arg1 read_only : tensor<4x8xf32> to memref<4x8xf32>
    %1 = bufferization.to_buffer %arg0 read_only : tensor<16x4xf32> to memref<16x4xf32>
    %alloca = memref.alloca() : memref<128xf32>
    %alloca_0 = memref.alloca() : memref<128xf32>
    %2 = bufferization.to_buffer %arg2 : tensor<16x8xf32> to memref<16x8xf32>
    %c8_i32 = arith.constant 8 : i32
    %intptr = memref.extract_aligned_pointer_as_index %2 : memref<16x8xf32> -> index
    %3 = arith.index_cast %intptr : index to i64
    %4 = llvm.inttoptr %3 : i64 to !llvm.ptr
    %c16_i32 = arith.constant 16 : i32
    %c8_i32_1 = arith.constant 8 : i32
    %c4_i32 = arith.constant 4 : i32
    %c2_i32 = arith.constant 2 : i32
    %5 = llvm.call @fpga_gemm_begin(%c16_i32, %c8_i32_1, %c4_i32, %c2_i32) : (i32, i32, i32, i32) -> !llvm.ptr
    %subview = memref.subview %1[0, 0] [8, 4] [1, 1] : memref<16x4xf32> to memref<8x4xf32, strided<[4, 1]>>
    %reinterpret_cast = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 4], strides: [4, 1] : memref<128xf32> to memref<8x4xf32>
    memref.copy %subview, %reinterpret_cast : memref<8x4xf32, strided<[4, 1]>> to memref<8x4xf32>
    %intptr_2 = memref.extract_aligned_pointer_as_index %reinterpret_cast : memref<8x4xf32> -> index
    %6 = arith.index_cast %intptr_2 : index to i64
    %7 = llvm.inttoptr %6 : i64 to !llvm.ptr
    %subview_3 = memref.subview %0[0, 0] [4, 8] [1, 1] : memref<4x8xf32> to memref<4x8xf32, strided<[8, 1]>>
    %reinterpret_cast_4 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [4, 8], strides: [8, 1] : memref<128xf32> to memref<4x8xf32>
    memref.copy %subview_3, %reinterpret_cast_4 : memref<4x8xf32, strided<[8, 1]>> to memref<4x8xf32>
    %intptr_5 = memref.extract_aligned_pointer_as_index %reinterpret_cast_4 : memref<4x8xf32> -> index
    %8 = arith.index_cast %intptr_5 : index to i64
    %9 = llvm.inttoptr %8 : i64 to !llvm.ptr
    %c0_i32 = arith.constant 0 : i32
    %c0_i32_6 = arith.constant 0 : i32
    %c1_i32 = arith.constant 1 : i32
    %c8_i32_7 = arith.constant 8 : i32
    %c4_i32_8 = arith.constant 4 : i32
    %c8_i32_9 = arith.constant 8 : i32
    %c0_i32_10 = arith.constant 0 : i32
    %c0_i32_11 = arith.constant 0 : i32
    %c113_i32 = arith.constant 113 : i32
    %10 = llvm.call @fpga_tile_submit(%5, %c0_i32, %c0_i32_6, %c1_i32, %c8_i32_7, %c4_i32_8, %c8_i32_9, %7, %9, %4, %c8_i32, %c0_i32_10, %c0_i32_11, %c113_i32) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %11 = bufferization.to_tensor %2 restrict writable : memref<16x8xf32> to tensor<16x8xf32>
    %c8_i32_12 = arith.constant 8 : i32
    %intptr_13 = memref.extract_aligned_pointer_as_index %2 : memref<16x8xf32> -> index
    %c256 = arith.constant 256 : index
    %12 = arith.addi %intptr_13, %c256 : index
    %13 = arith.index_cast %12 : index to i64
    %14 = llvm.inttoptr %13 : i64 to !llvm.ptr
    %subview_14 = memref.subview %1[8, 0] [8, 4] [1, 1] : memref<16x4xf32> to memref<8x4xf32, strided<[4, 1], offset: 32>>
    %reinterpret_cast_15 = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 4], strides: [4, 1] : memref<128xf32> to memref<8x4xf32>
    memref.copy %subview_14, %reinterpret_cast_15 : memref<8x4xf32, strided<[4, 1], offset: 32>> to memref<8x4xf32>
    %intptr_16 = memref.extract_aligned_pointer_as_index %reinterpret_cast_15 : memref<8x4xf32> -> index
    %15 = arith.index_cast %intptr_16 : index to i64
    %16 = llvm.inttoptr %15 : i64 to !llvm.ptr
    %subview_17 = memref.subview %0[0, 0] [4, 8] [1, 1] : memref<4x8xf32> to memref<4x8xf32, strided<[8, 1]>>
    %reinterpret_cast_18 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [4, 8], strides: [8, 1] : memref<128xf32> to memref<4x8xf32>
    memref.copy %subview_17, %reinterpret_cast_18 : memref<4x8xf32, strided<[8, 1]>> to memref<4x8xf32>
    %intptr_19 = memref.extract_aligned_pointer_as_index %reinterpret_cast_18 : memref<4x8xf32> -> index
    %17 = arith.index_cast %intptr_19 : index to i64
    %18 = llvm.inttoptr %17 : i64 to !llvm.ptr
    %c1_i32_20 = arith.constant 1 : i32
    %c0_i32_21 = arith.constant 0 : i32
    %c1_i32_22 = arith.constant 1 : i32
    %c8_i32_23 = arith.constant 8 : i32
    %c4_i32_24 = arith.constant 4 : i32
    %c8_i32_25 = arith.constant 8 : i32
    %c0_i32_26 = arith.constant 0 : i32
    %c30_i32 = arith.constant 30 : i32
    %c113_i32_27 = arith.constant 113 : i32
    %19 = llvm.call @fpga_tile_submit(%5, %c1_i32_20, %c0_i32_21, %c1_i32_22, %c8_i32_23, %c4_i32_24, %c8_i32_25, %16, %18, %14, %c8_i32_12, %c0_i32_26, %c30_i32, %c113_i32_27) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %20 = bufferization.to_tensor %2 restrict writable : memref<16x8xf32> to tensor<16x8xf32>
    %21 = llvm.call @fpga_gemm_wait(%5) : (!llvm.ptr) -> i32
    return %20 : tensor<16x8xf32>
  }
}

