module {
  llvm.func @fpga_gemm_wait(!llvm.ptr) -> i32
  llvm.func @fpga_tile_submit(!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
  llvm.func @fpga_gemm_begin(i32, i32, i32, i32) -> !llvm.ptr
  func.func @systolic_matmul(%arg0: tensor<1x1xf32>, %arg1: tensor<1x1xf32>, %arg2: tensor<1x1xf32>) -> tensor<1x1xf32> attributes {llvm.emit_c_interface} {
    %0 = bufferization.to_buffer %arg1 read_only : tensor<1x1xf32> to memref<1x1xf32>
    %1 = bufferization.to_buffer %arg0 read_only : tensor<1x1xf32> to memref<1x1xf32>
    %alloca = memref.alloca() : memref<128xf32>
    %alloca_0 = memref.alloca() : memref<128xf32>
    %2 = bufferization.to_buffer %arg2 : tensor<1x1xf32> to memref<1x1xf32>
    %c1_i32 = arith.constant 1 : i32
    %intptr = memref.extract_aligned_pointer_as_index %2 : memref<1x1xf32> -> index
    %3 = arith.index_cast %intptr : index to i64
    %4 = llvm.inttoptr %3 : i64 to !llvm.ptr
    %c1_i32_1 = arith.constant 1 : i32
    %c1_i32_2 = arith.constant 1 : i32
    %c1_i32_3 = arith.constant 1 : i32
    %c1_i32_4 = arith.constant 1 : i32
    %5 = llvm.call @fpga_gemm_begin(%c1_i32_1, %c1_i32_2, %c1_i32_3, %c1_i32_4) : (i32, i32, i32, i32) -> !llvm.ptr
    %subview = memref.subview %1[0, 0] [1, 1] [1, 1] : memref<1x1xf32> to memref<1x1xf32, strided<[1, 1]>>
    %reinterpret_cast = memref.reinterpret_cast %alloca to offset: [0], sizes: [1, 1], strides: [1, 1] : memref<128xf32> to memref<1x1xf32>
    memref.copy %subview, %reinterpret_cast : memref<1x1xf32, strided<[1, 1]>> to memref<1x1xf32>
    %intptr_5 = memref.extract_aligned_pointer_as_index %reinterpret_cast : memref<1x1xf32> -> index
    %6 = arith.index_cast %intptr_5 : index to i64
    %7 = llvm.inttoptr %6 : i64 to !llvm.ptr
    %subview_6 = memref.subview %0[0, 0] [1, 1] [1, 1] : memref<1x1xf32> to memref<1x1xf32, strided<[1, 1]>>
    %reinterpret_cast_7 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [1, 1], strides: [1, 1] : memref<128xf32> to memref<1x1xf32>
    memref.copy %subview_6, %reinterpret_cast_7 : memref<1x1xf32, strided<[1, 1]>> to memref<1x1xf32>
    %intptr_8 = memref.extract_aligned_pointer_as_index %reinterpret_cast_7 : memref<1x1xf32> -> index
    %8 = arith.index_cast %intptr_8 : index to i64
    %9 = llvm.inttoptr %8 : i64 to !llvm.ptr
    %c0_i32 = arith.constant 0 : i32
    %c0_i32_9 = arith.constant 0 : i32
    %c1_i32_10 = arith.constant 1 : i32
    %c1_i32_11 = arith.constant 1 : i32
    %c1_i32_12 = arith.constant 1 : i32
    %c1_i32_13 = arith.constant 1 : i32
    %c1_i32_14 = arith.constant 1 : i32
    %c0_i32_15 = arith.constant 0 : i32
    %c102_i32 = arith.constant 102 : i32
    %10 = llvm.call @fpga_tile_submit(%5, %c0_i32, %c0_i32_9, %c1_i32_10, %c1_i32_11, %c1_i32_12, %c1_i32_13, %7, %9, %4, %c1_i32, %c1_i32_14, %c0_i32_15, %c102_i32) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %11 = bufferization.to_tensor %2 restrict writable : memref<1x1xf32> to tensor<1x1xf32>
    %12 = llvm.call @fpga_gemm_wait(%5) : (!llvm.ptr) -> i32
    return %11 : tensor<1x1xf32>
  }
}

