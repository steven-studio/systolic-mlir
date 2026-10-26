module {
  llvm.func @fpga_gemm_wait(!llvm.ptr) -> i32
  llvm.func @fpga_tile_submit(!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
  llvm.func @fpga_gemm_begin(i32, i32, i32, i32) -> !llvm.ptr
  func.func @systolic_matmul(%arg0: tensor<4x4xf32>, %arg1: tensor<4x4xf32>, %arg2: tensor<4x4xf32>) -> tensor<4x4xf32> attributes {llvm.emit_c_interface} {
    %0 = bufferization.to_buffer %arg1 read_only : tensor<4x4xf32> to memref<4x4xf32>
    %1 = bufferization.to_buffer %arg0 read_only : tensor<4x4xf32> to memref<4x4xf32>
    %alloca = memref.alloca() : memref<128xf32>
    %alloca_0 = memref.alloca() : memref<128xf32>
    %2 = bufferization.to_buffer %arg2 : tensor<4x4xf32> to memref<4x4xf32>
    %c4_i32 = arith.constant 4 : i32
    %intptr = memref.extract_aligned_pointer_as_index %2 : memref<4x4xf32> -> index
    %3 = arith.index_cast %intptr : index to i64
    %4 = llvm.inttoptr %3 : i64 to !llvm.ptr
    %c4_i32_1 = arith.constant 4 : i32
    %c4_i32_2 = arith.constant 4 : i32
    %c4_i32_3 = arith.constant 4 : i32
    %c1_i32 = arith.constant 1 : i32
    %5 = llvm.call @fpga_gemm_begin(%c4_i32_1, %c4_i32_2, %c4_i32_3, %c1_i32) : (i32, i32, i32, i32) -> !llvm.ptr
    %subview = memref.subview %1[0, 0] [4, 4] [1, 1] : memref<4x4xf32> to memref<4x4xf32, strided<[4, 1]>>
    %reinterpret_cast = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<128xf32> to memref<4x4xf32>
    memref.copy %subview, %reinterpret_cast : memref<4x4xf32, strided<[4, 1]>> to memref<4x4xf32>
    %intptr_4 = memref.extract_aligned_pointer_as_index %reinterpret_cast : memref<4x4xf32> -> index
    %6 = arith.index_cast %intptr_4 : index to i64
    %7 = llvm.inttoptr %6 : i64 to !llvm.ptr
    %subview_5 = memref.subview %0[0, 0] [4, 4] [1, 1] : memref<4x4xf32> to memref<4x4xf32, strided<[4, 1]>>
    %reinterpret_cast_6 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<128xf32> to memref<4x4xf32>
    memref.copy %subview_5, %reinterpret_cast_6 : memref<4x4xf32, strided<[4, 1]>> to memref<4x4xf32>
    %intptr_7 = memref.extract_aligned_pointer_as_index %reinterpret_cast_6 : memref<4x4xf32> -> index
    %8 = arith.index_cast %intptr_7 : index to i64
    %9 = llvm.inttoptr %8 : i64 to !llvm.ptr
    %c0_i32 = arith.constant 0 : i32
    %c0_i32_8 = arith.constant 0 : i32
    %c1_i32_9 = arith.constant 1 : i32
    %c4_i32_10 = arith.constant 4 : i32
    %c4_i32_11 = arith.constant 4 : i32
    %c4_i32_12 = arith.constant 4 : i32
    %c1_i32_13 = arith.constant 1 : i32
    %c0_i32_14 = arith.constant 0 : i32
    %c105_i32 = arith.constant 105 : i32
    %10 = llvm.call @fpga_tile_submit(%5, %c0_i32, %c0_i32_8, %c1_i32_9, %c4_i32_10, %c4_i32_11, %c4_i32_12, %7, %9, %4, %c4_i32, %c1_i32_13, %c0_i32_14, %c105_i32) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %11 = bufferization.to_tensor %2 restrict writable : memref<4x4xf32> to tensor<4x4xf32>
    %12 = llvm.call @fpga_gemm_wait(%5) : (!llvm.ptr) -> i32
    return %11 : tensor<4x4xf32>
  }
}

