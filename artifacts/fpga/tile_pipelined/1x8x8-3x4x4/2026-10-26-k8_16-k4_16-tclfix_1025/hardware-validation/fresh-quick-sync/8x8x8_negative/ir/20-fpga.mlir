module {
  llvm.func @fpga_matmul_tiled_auto_scheduled(i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
  func.func @systolic_matmul(%arg0: tensor<8x8xf32>, %arg1: tensor<8x8xf32>, %arg2: tensor<8x8xf32>) -> tensor<8x8xf32> attributes {llvm.emit_c_interface} {
    %0 = bufferization.to_buffer %arg1 read_only : tensor<8x8xf32> to memref<8x8xf32>
    %1 = bufferization.to_buffer %arg0 read_only : tensor<8x8xf32> to memref<8x8xf32>
    %alloca = memref.alloca() : memref<128xf32>
    %alloca_0 = memref.alloca() : memref<128xf32>
    %alloca_1 = memref.alloca() : memref<64xf32>
    %2 = bufferization.to_buffer %arg2 : tensor<8x8xf32> to memref<8x8xf32>
    %subview = memref.subview %2[0, 0] [8, 8] [1, 1] : memref<8x8xf32> to memref<8x8xf32, strided<[8, 1]>>
    %reinterpret_cast = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [8, 8], strides: [8, 1] : memref<64xf32> to memref<8x8xf32>
    memref.copy %subview, %reinterpret_cast : memref<8x8xf32, strided<[8, 1]>> to memref<8x8xf32>
    %intptr = memref.extract_aligned_pointer_as_index %reinterpret_cast : memref<8x8xf32> -> index
    %3 = arith.index_cast %intptr : index to i64
    %4 = llvm.inttoptr %3 : i64 to !llvm.ptr
    %subview_2 = memref.subview %1[0, 0] [8, 8] [1, 1] : memref<8x8xf32> to memref<8x8xf32, strided<[8, 1]>>
    %reinterpret_cast_3 = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 8], strides: [8, 1] : memref<128xf32> to memref<8x8xf32>
    memref.copy %subview_2, %reinterpret_cast_3 : memref<8x8xf32, strided<[8, 1]>> to memref<8x8xf32>
    %intptr_4 = memref.extract_aligned_pointer_as_index %reinterpret_cast_3 : memref<8x8xf32> -> index
    %5 = arith.index_cast %intptr_4 : index to i64
    %6 = llvm.inttoptr %5 : i64 to !llvm.ptr
    %subview_5 = memref.subview %0[0, 0] [8, 8] [1, 1] : memref<8x8xf32> to memref<8x8xf32, strided<[8, 1]>>
    %reinterpret_cast_6 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [8, 8], strides: [8, 1] : memref<128xf32> to memref<8x8xf32>
    memref.copy %subview_5, %reinterpret_cast_6 : memref<8x8xf32, strided<[8, 1]>> to memref<8x8xf32>
    %intptr_7 = memref.extract_aligned_pointer_as_index %reinterpret_cast_6 : memref<8x8xf32> -> index
    %7 = arith.index_cast %intptr_7 : index to i64
    %8 = llvm.inttoptr %7 : i64 to !llvm.ptr
    %c8_i32 = arith.constant 8 : i32
    %c8_i32_8 = arith.constant 8 : i32
    %c8_i32_9 = arith.constant 8 : i32
    %c0_i32 = arith.constant 0 : i32
    %c0_i32_10 = arith.constant 0 : i32
    %c117_i32 = arith.constant 117 : i32
    %9 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c8_i32, %c8_i32_8, %c8_i32_9, %6, %8, %4, %c0_i32, %c0_i32_10, %c117_i32) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast, %subview : memref<8x8xf32> to memref<8x8xf32, strided<[8, 1]>>
    %10 = bufferization.to_tensor %2 restrict writable : memref<8x8xf32> to tensor<8x8xf32>
    return %10 : tensor<8x8xf32>
  }
}

