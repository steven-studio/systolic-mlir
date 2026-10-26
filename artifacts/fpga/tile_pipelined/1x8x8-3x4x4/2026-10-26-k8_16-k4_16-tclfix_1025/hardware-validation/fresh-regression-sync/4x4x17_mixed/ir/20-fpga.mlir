module {
  llvm.func @fpga_matmul_tiled_auto_scheduled(i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
  func.func @systolic_matmul(%arg0: tensor<4x17xf32>, %arg1: tensor<17x4xf32>, %arg2: tensor<4x4xf32>) -> tensor<4x4xf32> attributes {llvm.emit_c_interface} {
    %0 = bufferization.to_buffer %arg1 read_only : tensor<17x4xf32> to memref<17x4xf32>
    %1 = bufferization.to_buffer %arg0 read_only : tensor<4x17xf32> to memref<4x17xf32>
    %alloca = memref.alloca() : memref<128xf32>
    %alloca_0 = memref.alloca() : memref<128xf32>
    %alloca_1 = memref.alloca() : memref<64xf32>
    %2 = bufferization.to_buffer %arg2 : tensor<4x4xf32> to memref<4x4xf32>
    %subview = memref.subview %2[0, 0] [4, 4] [1, 1] : memref<4x4xf32> to memref<4x4xf32, strided<[4, 1]>>
    %reinterpret_cast = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<64xf32> to memref<4x4xf32>
    memref.copy %subview, %reinterpret_cast : memref<4x4xf32, strided<[4, 1]>> to memref<4x4xf32>
    %intptr = memref.extract_aligned_pointer_as_index %reinterpret_cast : memref<4x4xf32> -> index
    %3 = arith.index_cast %intptr : index to i64
    %4 = llvm.inttoptr %3 : i64 to !llvm.ptr
    %subview_2 = memref.subview %1[0, 0] [4, 16] [1, 1] : memref<4x17xf32> to memref<4x16xf32, strided<[17, 1]>>
    %reinterpret_cast_3 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_2, %reinterpret_cast_3 : memref<4x16xf32, strided<[17, 1]>> to memref<4x16xf32>
    %intptr_4 = memref.extract_aligned_pointer_as_index %reinterpret_cast_3 : memref<4x16xf32> -> index
    %5 = arith.index_cast %intptr_4 : index to i64
    %6 = llvm.inttoptr %5 : i64 to !llvm.ptr
    %subview_5 = memref.subview %0[0, 0] [16, 4] [1, 1] : memref<17x4xf32> to memref<16x4xf32, strided<[4, 1]>>
    %reinterpret_cast_6 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_5, %reinterpret_cast_6 : memref<16x4xf32, strided<[4, 1]>> to memref<16x4xf32>
    %intptr_7 = memref.extract_aligned_pointer_as_index %reinterpret_cast_6 : memref<16x4xf32> -> index
    %7 = arith.index_cast %intptr_7 : index to i64
    %8 = llvm.inttoptr %7 : i64 to !llvm.ptr
    %c4_i32 = arith.constant 4 : i32
    %c16_i32 = arith.constant 16 : i32
    %c4_i32_8 = arith.constant 4 : i32
    %c1_i32 = arith.constant 1 : i32
    %c0_i32 = arith.constant 0 : i32
    %c117_i32 = arith.constant 117 : i32
    %9 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32, %c16_i32, %c4_i32_8, %6, %8, %4, %c1_i32, %c0_i32, %c117_i32) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    %subview_9 = memref.subview %1[0, 16] [4, 1] [1, 1] : memref<4x17xf32> to memref<4x1xf32, strided<[17, 1], offset: 16>>
    %reinterpret_cast_10 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 1], strides: [1, 1] : memref<128xf32> to memref<4x1xf32>
    memref.copy %subview_9, %reinterpret_cast_10 : memref<4x1xf32, strided<[17, 1], offset: 16>> to memref<4x1xf32>
    %intptr_11 = memref.extract_aligned_pointer_as_index %reinterpret_cast_10 : memref<4x1xf32> -> index
    %10 = arith.index_cast %intptr_11 : index to i64
    %11 = llvm.inttoptr %10 : i64 to !llvm.ptr
    %subview_12 = memref.subview %0[16, 0] [1, 4] [1, 1] : memref<17x4xf32> to memref<1x4xf32, strided<[4, 1], offset: 64>>
    %reinterpret_cast_13 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [1, 4], strides: [4, 1] : memref<128xf32> to memref<1x4xf32>
    memref.copy %subview_12, %reinterpret_cast_13 : memref<1x4xf32, strided<[4, 1], offset: 64>> to memref<1x4xf32>
    %intptr_14 = memref.extract_aligned_pointer_as_index %reinterpret_cast_13 : memref<1x4xf32> -> index
    %12 = arith.index_cast %intptr_14 : index to i64
    %13 = llvm.inttoptr %12 : i64 to !llvm.ptr
    %c4_i32_15 = arith.constant 4 : i32
    %c1_i32_16 = arith.constant 1 : i32
    %c4_i32_17 = arith.constant 4 : i32
    %c1_i32_18 = arith.constant 1 : i32
    %c42_i32 = arith.constant 42 : i32
    %c102_i32 = arith.constant 102 : i32
    %14 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_15, %c1_i32_16, %c4_i32_17, %11, %13, %4, %c1_i32_18, %c42_i32, %c102_i32) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast, %subview : memref<4x4xf32> to memref<4x4xf32, strided<[4, 1]>>
    %15 = bufferization.to_tensor %2 restrict writable : memref<4x4xf32> to tensor<4x4xf32>
    return %15 : tensor<4x4xf32>
  }
}

