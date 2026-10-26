module {
  llvm.func @fpga_gemm_wait(!llvm.ptr) -> i32
  llvm.func @fpga_tile_submit(!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
  llvm.func @fpga_gemm_begin(i32, i32, i32, i32) -> !llvm.ptr
  func.func @systolic_matmul(%arg0: tensor<8x16xf32>, %arg1: tensor<16x8xf32>, %arg2: tensor<8x8xf32>) -> tensor<8x8xf32> attributes {llvm.emit_c_interface} {
    %0 = bufferization.to_buffer %arg1 read_only : tensor<16x8xf32> to memref<16x8xf32>
    %1 = bufferization.to_buffer %arg0 read_only : tensor<8x16xf32> to memref<8x16xf32>
    %alloca = memref.alloca() : memref<128xf32>
    %alloca_0 = memref.alloca() : memref<128xf32>
    %2 = bufferization.to_buffer %arg2 : tensor<8x8xf32> to memref<8x8xf32>
    %c8_i32 = arith.constant 8 : i32
    %intptr = memref.extract_aligned_pointer_as_index %2 : memref<8x8xf32> -> index
    %3 = arith.index_cast %intptr : index to i64
    %4 = llvm.inttoptr %3 : i64 to !llvm.ptr
    %c8_i32_1 = arith.constant 8 : i32
    %c8_i32_2 = arith.constant 8 : i32
    %c16_i32 = arith.constant 16 : i32
    %c1_i32 = arith.constant 1 : i32
    %5 = llvm.call @fpga_gemm_begin(%c8_i32_1, %c8_i32_2, %c16_i32, %c1_i32) : (i32, i32, i32, i32) -> !llvm.ptr
    %subview = memref.subview %1[0, 0] [8, 16] [1, 1] : memref<8x16xf32> to memref<8x16xf32, strided<[16, 1]>>
    %reinterpret_cast = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 16], strides: [16, 1] : memref<128xf32> to memref<8x16xf32>
    memref.copy %subview, %reinterpret_cast : memref<8x16xf32, strided<[16, 1]>> to memref<8x16xf32>
    %intptr_3 = memref.extract_aligned_pointer_as_index %reinterpret_cast : memref<8x16xf32> -> index
    %6 = arith.index_cast %intptr_3 : index to i64
    %7 = llvm.inttoptr %6 : i64 to !llvm.ptr
    %subview_4 = memref.subview %0[0, 0] [16, 8] [1, 1] : memref<16x8xf32> to memref<16x8xf32, strided<[8, 1]>>
    %reinterpret_cast_5 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 8], strides: [8, 1] : memref<128xf32> to memref<16x8xf32>
    memref.copy %subview_4, %reinterpret_cast_5 : memref<16x8xf32, strided<[8, 1]>> to memref<16x8xf32>
    %intptr_6 = memref.extract_aligned_pointer_as_index %reinterpret_cast_5 : memref<16x8xf32> -> index
    %8 = arith.index_cast %intptr_6 : index to i64
    %9 = llvm.inttoptr %8 : i64 to !llvm.ptr
    %c0_i32 = arith.constant 0 : i32
    %c0_i32_7 = arith.constant 0 : i32
    %c1_i32_8 = arith.constant 1 : i32
    %c8_i32_9 = arith.constant 8 : i32
    %c16_i32_10 = arith.constant 16 : i32
    %c8_i32_11 = arith.constant 8 : i32
    %c0_i32_12 = arith.constant 0 : i32
    %c0_i32_13 = arith.constant 0 : i32
    %c125_i32 = arith.constant 125 : i32
    %10 = llvm.call @fpga_tile_submit(%5, %c0_i32, %c0_i32_7, %c1_i32_8, %c8_i32_9, %c16_i32_10, %c8_i32_11, %7, %9, %4, %c8_i32, %c0_i32_12, %c0_i32_13, %c125_i32) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %11 = bufferization.to_tensor %2 restrict writable : memref<8x8xf32> to tensor<8x8xf32>
    %12 = llvm.call @fpga_gemm_wait(%5) : (!llvm.ptr) -> i32
    return %11 : tensor<8x8xf32>
  }
}

