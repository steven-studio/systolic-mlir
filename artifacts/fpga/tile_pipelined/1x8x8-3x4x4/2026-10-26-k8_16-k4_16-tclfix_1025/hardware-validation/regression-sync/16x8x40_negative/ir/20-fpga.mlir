module {
  llvm.func @fpga_matmul_tiled_auto_scheduled(i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
  func.func @systolic_matmul(%arg0: tensor<16x40xf32>, %arg1: tensor<40x8xf32>, %arg2: tensor<16x8xf32>) -> tensor<16x8xf32> attributes {llvm.emit_c_interface} {
    %0 = bufferization.to_buffer %arg1 read_only : tensor<40x8xf32> to memref<40x8xf32>
    %1 = bufferization.to_buffer %arg0 read_only : tensor<16x40xf32> to memref<16x40xf32>
    %alloca = memref.alloca() : memref<128xf32>
    %alloca_0 = memref.alloca() : memref<128xf32>
    %alloca_1 = memref.alloca() : memref<64xf32>
    %2 = bufferization.to_buffer %arg2 : tensor<16x8xf32> to memref<16x8xf32>
    %subview = memref.subview %2[0, 0] [8, 8] [1, 1] : memref<16x8xf32> to memref<8x8xf32, strided<[8, 1]>>
    %reinterpret_cast = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [8, 8], strides: [8, 1] : memref<64xf32> to memref<8x8xf32>
    memref.copy %subview, %reinterpret_cast : memref<8x8xf32, strided<[8, 1]>> to memref<8x8xf32>
    %intptr = memref.extract_aligned_pointer_as_index %reinterpret_cast : memref<8x8xf32> -> index
    %3 = arith.index_cast %intptr : index to i64
    %4 = llvm.inttoptr %3 : i64 to !llvm.ptr
    %subview_2 = memref.subview %1[0, 0] [8, 16] [1, 1] : memref<16x40xf32> to memref<8x16xf32, strided<[40, 1]>>
    %reinterpret_cast_3 = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 16], strides: [16, 1] : memref<128xf32> to memref<8x16xf32>
    memref.copy %subview_2, %reinterpret_cast_3 : memref<8x16xf32, strided<[40, 1]>> to memref<8x16xf32>
    %intptr_4 = memref.extract_aligned_pointer_as_index %reinterpret_cast_3 : memref<8x16xf32> -> index
    %5 = arith.index_cast %intptr_4 : index to i64
    %6 = llvm.inttoptr %5 : i64 to !llvm.ptr
    %subview_5 = memref.subview %0[0, 0] [16, 8] [1, 1] : memref<40x8xf32> to memref<16x8xf32, strided<[8, 1]>>
    %reinterpret_cast_6 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 8], strides: [8, 1] : memref<128xf32> to memref<16x8xf32>
    memref.copy %subview_5, %reinterpret_cast_6 : memref<16x8xf32, strided<[8, 1]>> to memref<16x8xf32>
    %intptr_7 = memref.extract_aligned_pointer_as_index %reinterpret_cast_6 : memref<16x8xf32> -> index
    %7 = arith.index_cast %intptr_7 : index to i64
    %8 = llvm.inttoptr %7 : i64 to !llvm.ptr
    %c8_i32 = arith.constant 8 : i32
    %c16_i32 = arith.constant 16 : i32
    %c8_i32_8 = arith.constant 8 : i32
    %c0_i32 = arith.constant 0 : i32
    %c0_i32_9 = arith.constant 0 : i32
    %c125_i32 = arith.constant 125 : i32
    %9 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c8_i32, %c16_i32, %c8_i32_8, %6, %8, %4, %c0_i32, %c0_i32_9, %c125_i32) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    %subview_10 = memref.subview %1[0, 16] [8, 16] [1, 1] : memref<16x40xf32> to memref<8x16xf32, strided<[40, 1], offset: 16>>
    %reinterpret_cast_11 = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 16], strides: [16, 1] : memref<128xf32> to memref<8x16xf32>
    memref.copy %subview_10, %reinterpret_cast_11 : memref<8x16xf32, strided<[40, 1], offset: 16>> to memref<8x16xf32>
    %intptr_12 = memref.extract_aligned_pointer_as_index %reinterpret_cast_11 : memref<8x16xf32> -> index
    %10 = arith.index_cast %intptr_12 : index to i64
    %11 = llvm.inttoptr %10 : i64 to !llvm.ptr
    %subview_13 = memref.subview %0[16, 0] [16, 8] [1, 1] : memref<40x8xf32> to memref<16x8xf32, strided<[8, 1], offset: 128>>
    %reinterpret_cast_14 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 8], strides: [8, 1] : memref<128xf32> to memref<16x8xf32>
    memref.copy %subview_13, %reinterpret_cast_14 : memref<16x8xf32, strided<[8, 1], offset: 128>> to memref<16x8xf32>
    %intptr_15 = memref.extract_aligned_pointer_as_index %reinterpret_cast_14 : memref<16x8xf32> -> index
    %12 = arith.index_cast %intptr_15 : index to i64
    %13 = llvm.inttoptr %12 : i64 to !llvm.ptr
    %c8_i32_16 = arith.constant 8 : i32
    %c16_i32_17 = arith.constant 16 : i32
    %c8_i32_18 = arith.constant 8 : i32
    %c0_i32_19 = arith.constant 0 : i32
    %c42_i32 = arith.constant 42 : i32
    %c125_i32_20 = arith.constant 125 : i32
    %14 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c8_i32_16, %c16_i32_17, %c8_i32_18, %11, %13, %4, %c0_i32_19, %c42_i32, %c125_i32_20) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    %subview_21 = memref.subview %1[0, 32] [8, 8] [1, 1] : memref<16x40xf32> to memref<8x8xf32, strided<[40, 1], offset: 32>>
    %reinterpret_cast_22 = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 8], strides: [8, 1] : memref<128xf32> to memref<8x8xf32>
    memref.copy %subview_21, %reinterpret_cast_22 : memref<8x8xf32, strided<[40, 1], offset: 32>> to memref<8x8xf32>
    %intptr_23 = memref.extract_aligned_pointer_as_index %reinterpret_cast_22 : memref<8x8xf32> -> index
    %15 = arith.index_cast %intptr_23 : index to i64
    %16 = llvm.inttoptr %15 : i64 to !llvm.ptr
    %subview_24 = memref.subview %0[32, 0] [8, 8] [1, 1] : memref<40x8xf32> to memref<8x8xf32, strided<[8, 1], offset: 256>>
    %reinterpret_cast_25 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [8, 8], strides: [8, 1] : memref<128xf32> to memref<8x8xf32>
    memref.copy %subview_24, %reinterpret_cast_25 : memref<8x8xf32, strided<[8, 1], offset: 256>> to memref<8x8xf32>
    %intptr_26 = memref.extract_aligned_pointer_as_index %reinterpret_cast_25 : memref<8x8xf32> -> index
    %17 = arith.index_cast %intptr_26 : index to i64
    %18 = llvm.inttoptr %17 : i64 to !llvm.ptr
    %c8_i32_27 = arith.constant 8 : i32
    %c8_i32_28 = arith.constant 8 : i32
    %c8_i32_29 = arith.constant 8 : i32
    %c0_i32_30 = arith.constant 0 : i32
    %c128_i32 = arith.constant 128 : i32
    %c117_i32 = arith.constant 117 : i32
    %19 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c8_i32_27, %c8_i32_28, %c8_i32_29, %16, %18, %4, %c0_i32_30, %c128_i32, %c117_i32) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast, %subview : memref<8x8xf32> to memref<8x8xf32, strided<[8, 1]>>
    %20 = bufferization.to_tensor %2 restrict writable : memref<16x8xf32> to tensor<16x8xf32>
    %subview_31 = memref.subview %2[8, 0] [8, 8] [1, 1] : memref<16x8xf32> to memref<8x8xf32, strided<[8, 1], offset: 64>>
    %reinterpret_cast_32 = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [8, 8], strides: [8, 1] : memref<64xf32> to memref<8x8xf32>
    memref.copy %subview_31, %reinterpret_cast_32 : memref<8x8xf32, strided<[8, 1], offset: 64>> to memref<8x8xf32>
    %intptr_33 = memref.extract_aligned_pointer_as_index %reinterpret_cast_32 : memref<8x8xf32> -> index
    %21 = arith.index_cast %intptr_33 : index to i64
    %22 = llvm.inttoptr %21 : i64 to !llvm.ptr
    %subview_34 = memref.subview %1[8, 0] [8, 16] [1, 1] : memref<16x40xf32> to memref<8x16xf32, strided<[40, 1], offset: 320>>
    %reinterpret_cast_35 = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 16], strides: [16, 1] : memref<128xf32> to memref<8x16xf32>
    memref.copy %subview_34, %reinterpret_cast_35 : memref<8x16xf32, strided<[40, 1], offset: 320>> to memref<8x16xf32>
    %intptr_36 = memref.extract_aligned_pointer_as_index %reinterpret_cast_35 : memref<8x16xf32> -> index
    %23 = arith.index_cast %intptr_36 : index to i64
    %24 = llvm.inttoptr %23 : i64 to !llvm.ptr
    %subview_37 = memref.subview %0[0, 0] [16, 8] [1, 1] : memref<40x8xf32> to memref<16x8xf32, strided<[8, 1]>>
    %reinterpret_cast_38 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 8], strides: [8, 1] : memref<128xf32> to memref<16x8xf32>
    memref.copy %subview_37, %reinterpret_cast_38 : memref<16x8xf32, strided<[8, 1]>> to memref<16x8xf32>
    %intptr_39 = memref.extract_aligned_pointer_as_index %reinterpret_cast_38 : memref<16x8xf32> -> index
    %25 = arith.index_cast %intptr_39 : index to i64
    %26 = llvm.inttoptr %25 : i64 to !llvm.ptr
    %c8_i32_40 = arith.constant 8 : i32
    %c16_i32_41 = arith.constant 16 : i32
    %c8_i32_42 = arith.constant 8 : i32
    %c0_i32_43 = arith.constant 0 : i32
    %c197_i32 = arith.constant 197 : i32
    %c125_i32_44 = arith.constant 125 : i32
    %27 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c8_i32_40, %c16_i32_41, %c8_i32_42, %24, %26, %22, %c0_i32_43, %c197_i32, %c125_i32_44) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    %subview_45 = memref.subview %1[8, 16] [8, 16] [1, 1] : memref<16x40xf32> to memref<8x16xf32, strided<[40, 1], offset: 336>>
    %reinterpret_cast_46 = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 16], strides: [16, 1] : memref<128xf32> to memref<8x16xf32>
    memref.copy %subview_45, %reinterpret_cast_46 : memref<8x16xf32, strided<[40, 1], offset: 336>> to memref<8x16xf32>
    %intptr_47 = memref.extract_aligned_pointer_as_index %reinterpret_cast_46 : memref<8x16xf32> -> index
    %28 = arith.index_cast %intptr_47 : index to i64
    %29 = llvm.inttoptr %28 : i64 to !llvm.ptr
    %subview_48 = memref.subview %0[16, 0] [16, 8] [1, 1] : memref<40x8xf32> to memref<16x8xf32, strided<[8, 1], offset: 128>>
    %reinterpret_cast_49 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 8], strides: [8, 1] : memref<128xf32> to memref<16x8xf32>
    memref.copy %subview_48, %reinterpret_cast_49 : memref<16x8xf32, strided<[8, 1], offset: 128>> to memref<16x8xf32>
    %intptr_50 = memref.extract_aligned_pointer_as_index %reinterpret_cast_49 : memref<16x8xf32> -> index
    %30 = arith.index_cast %intptr_50 : index to i64
    %31 = llvm.inttoptr %30 : i64 to !llvm.ptr
    %c8_i32_51 = arith.constant 8 : i32
    %c16_i32_52 = arith.constant 16 : i32
    %c8_i32_53 = arith.constant 8 : i32
    %c0_i32_54 = arith.constant 0 : i32
    %c266_i32 = arith.constant 266 : i32
    %c125_i32_55 = arith.constant 125 : i32
    %32 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c8_i32_51, %c16_i32_52, %c8_i32_53, %29, %31, %22, %c0_i32_54, %c266_i32, %c125_i32_55) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    %subview_56 = memref.subview %1[8, 32] [8, 8] [1, 1] : memref<16x40xf32> to memref<8x8xf32, strided<[40, 1], offset: 352>>
    %reinterpret_cast_57 = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 8], strides: [8, 1] : memref<128xf32> to memref<8x8xf32>
    memref.copy %subview_56, %reinterpret_cast_57 : memref<8x8xf32, strided<[40, 1], offset: 352>> to memref<8x8xf32>
    %intptr_58 = memref.extract_aligned_pointer_as_index %reinterpret_cast_57 : memref<8x8xf32> -> index
    %33 = arith.index_cast %intptr_58 : index to i64
    %34 = llvm.inttoptr %33 : i64 to !llvm.ptr
    %subview_59 = memref.subview %0[32, 0] [8, 8] [1, 1] : memref<40x8xf32> to memref<8x8xf32, strided<[8, 1], offset: 256>>
    %reinterpret_cast_60 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [8, 8], strides: [8, 1] : memref<128xf32> to memref<8x8xf32>
    memref.copy %subview_59, %reinterpret_cast_60 : memref<8x8xf32, strided<[8, 1], offset: 256>> to memref<8x8xf32>
    %intptr_61 = memref.extract_aligned_pointer_as_index %reinterpret_cast_60 : memref<8x8xf32> -> index
    %35 = arith.index_cast %intptr_61 : index to i64
    %36 = llvm.inttoptr %35 : i64 to !llvm.ptr
    %c8_i32_62 = arith.constant 8 : i32
    %c8_i32_63 = arith.constant 8 : i32
    %c8_i32_64 = arith.constant 8 : i32
    %c0_i32_65 = arith.constant 0 : i32
    %c335_i32 = arith.constant 335 : i32
    %c117_i32_66 = arith.constant 117 : i32
    %37 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c8_i32_62, %c8_i32_63, %c8_i32_64, %34, %36, %22, %c0_i32_65, %c335_i32, %c117_i32_66) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast_32, %subview_31 : memref<8x8xf32> to memref<8x8xf32, strided<[8, 1], offset: 64>>
    %38 = bufferization.to_tensor %2 restrict writable : memref<16x8xf32> to tensor<16x8xf32>
    return %38 : tensor<16x8xf32>
  }
}

