module {
  llvm.func @fpga_gemm_wait(!llvm.ptr) -> i32
  llvm.func @fpga_tile_submit(!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
  llvm.func @fpga_gemm_begin(i32, i32, i32, i32) -> !llvm.ptr
  func.func @systolic_matmul(%arg0: tensor<16x40xf32>, %arg1: tensor<40x8xf32>, %arg2: tensor<16x8xf32>) -> tensor<16x8xf32> attributes {llvm.emit_c_interface} {
    %0 = bufferization.to_buffer %arg1 read_only : tensor<40x8xf32> to memref<40x8xf32>
    %1 = bufferization.to_buffer %arg0 read_only : tensor<16x40xf32> to memref<16x40xf32>
    %alloca = memref.alloca() : memref<128xf32>
    %alloca_0 = memref.alloca() : memref<128xf32>
    %2 = bufferization.to_buffer %arg2 : tensor<16x8xf32> to memref<16x8xf32>
    %c8_i32 = arith.constant 8 : i32
    %intptr = memref.extract_aligned_pointer_as_index %2 : memref<16x8xf32> -> index
    %3 = arith.index_cast %intptr : index to i64
    %4 = llvm.inttoptr %3 : i64 to !llvm.ptr
    %c16_i32 = arith.constant 16 : i32
    %c8_i32_1 = arith.constant 8 : i32
    %c40_i32 = arith.constant 40 : i32
    %c6_i32 = arith.constant 6 : i32
    %5 = llvm.call @fpga_gemm_begin(%c16_i32, %c8_i32_1, %c40_i32, %c6_i32) : (i32, i32, i32, i32) -> !llvm.ptr
    %subview = memref.subview %1[0, 0] [8, 16] [1, 1] : memref<16x40xf32> to memref<8x16xf32, strided<[40, 1]>>
    %reinterpret_cast = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 16], strides: [16, 1] : memref<128xf32> to memref<8x16xf32>
    memref.copy %subview, %reinterpret_cast : memref<8x16xf32, strided<[40, 1]>> to memref<8x16xf32>
    %intptr_2 = memref.extract_aligned_pointer_as_index %reinterpret_cast : memref<8x16xf32> -> index
    %6 = arith.index_cast %intptr_2 : index to i64
    %7 = llvm.inttoptr %6 : i64 to !llvm.ptr
    %subview_3 = memref.subview %0[0, 0] [16, 8] [1, 1] : memref<40x8xf32> to memref<16x8xf32, strided<[8, 1]>>
    %reinterpret_cast_4 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 8], strides: [8, 1] : memref<128xf32> to memref<16x8xf32>
    memref.copy %subview_3, %reinterpret_cast_4 : memref<16x8xf32, strided<[8, 1]>> to memref<16x8xf32>
    %intptr_5 = memref.extract_aligned_pointer_as_index %reinterpret_cast_4 : memref<16x8xf32> -> index
    %8 = arith.index_cast %intptr_5 : index to i64
    %9 = llvm.inttoptr %8 : i64 to !llvm.ptr
    %c0_i32 = arith.constant 0 : i32
    %c0_i32_6 = arith.constant 0 : i32
    %c3_i32 = arith.constant 3 : i32
    %c8_i32_7 = arith.constant 8 : i32
    %c16_i32_8 = arith.constant 16 : i32
    %c8_i32_9 = arith.constant 8 : i32
    %c0_i32_10 = arith.constant 0 : i32
    %c0_i32_11 = arith.constant 0 : i32
    %c125_i32 = arith.constant 125 : i32
    %10 = llvm.call @fpga_tile_submit(%5, %c0_i32, %c0_i32_6, %c3_i32, %c8_i32_7, %c16_i32_8, %c8_i32_9, %7, %9, %4, %c8_i32, %c0_i32_10, %c0_i32_11, %c125_i32) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %subview_12 = memref.subview %1[0, 16] [8, 16] [1, 1] : memref<16x40xf32> to memref<8x16xf32, strided<[40, 1], offset: 16>>
    %reinterpret_cast_13 = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 16], strides: [16, 1] : memref<128xf32> to memref<8x16xf32>
    memref.copy %subview_12, %reinterpret_cast_13 : memref<8x16xf32, strided<[40, 1], offset: 16>> to memref<8x16xf32>
    %intptr_14 = memref.extract_aligned_pointer_as_index %reinterpret_cast_13 : memref<8x16xf32> -> index
    %11 = arith.index_cast %intptr_14 : index to i64
    %12 = llvm.inttoptr %11 : i64 to !llvm.ptr
    %subview_15 = memref.subview %0[16, 0] [16, 8] [1, 1] : memref<40x8xf32> to memref<16x8xf32, strided<[8, 1], offset: 128>>
    %reinterpret_cast_16 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 8], strides: [8, 1] : memref<128xf32> to memref<16x8xf32>
    memref.copy %subview_15, %reinterpret_cast_16 : memref<16x8xf32, strided<[8, 1], offset: 128>> to memref<16x8xf32>
    %intptr_17 = memref.extract_aligned_pointer_as_index %reinterpret_cast_16 : memref<16x8xf32> -> index
    %13 = arith.index_cast %intptr_17 : index to i64
    %14 = llvm.inttoptr %13 : i64 to !llvm.ptr
    %c0_i32_18 = arith.constant 0 : i32
    %c1_i32 = arith.constant 1 : i32
    %c3_i32_19 = arith.constant 3 : i32
    %c8_i32_20 = arith.constant 8 : i32
    %c16_i32_21 = arith.constant 16 : i32
    %c8_i32_22 = arith.constant 8 : i32
    %c0_i32_23 = arith.constant 0 : i32
    %c42_i32 = arith.constant 42 : i32
    %c125_i32_24 = arith.constant 125 : i32
    %15 = llvm.call @fpga_tile_submit(%5, %c0_i32_18, %c1_i32, %c3_i32_19, %c8_i32_20, %c16_i32_21, %c8_i32_22, %12, %14, %4, %c8_i32, %c0_i32_23, %c42_i32, %c125_i32_24) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %subview_25 = memref.subview %1[0, 32] [8, 8] [1, 1] : memref<16x40xf32> to memref<8x8xf32, strided<[40, 1], offset: 32>>
    %reinterpret_cast_26 = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 8], strides: [8, 1] : memref<128xf32> to memref<8x8xf32>
    memref.copy %subview_25, %reinterpret_cast_26 : memref<8x8xf32, strided<[40, 1], offset: 32>> to memref<8x8xf32>
    %intptr_27 = memref.extract_aligned_pointer_as_index %reinterpret_cast_26 : memref<8x8xf32> -> index
    %16 = arith.index_cast %intptr_27 : index to i64
    %17 = llvm.inttoptr %16 : i64 to !llvm.ptr
    %subview_28 = memref.subview %0[32, 0] [8, 8] [1, 1] : memref<40x8xf32> to memref<8x8xf32, strided<[8, 1], offset: 256>>
    %reinterpret_cast_29 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [8, 8], strides: [8, 1] : memref<128xf32> to memref<8x8xf32>
    memref.copy %subview_28, %reinterpret_cast_29 : memref<8x8xf32, strided<[8, 1], offset: 256>> to memref<8x8xf32>
    %intptr_30 = memref.extract_aligned_pointer_as_index %reinterpret_cast_29 : memref<8x8xf32> -> index
    %18 = arith.index_cast %intptr_30 : index to i64
    %19 = llvm.inttoptr %18 : i64 to !llvm.ptr
    %c0_i32_31 = arith.constant 0 : i32
    %c2_i32 = arith.constant 2 : i32
    %c3_i32_32 = arith.constant 3 : i32
    %c8_i32_33 = arith.constant 8 : i32
    %c8_i32_34 = arith.constant 8 : i32
    %c8_i32_35 = arith.constant 8 : i32
    %c0_i32_36 = arith.constant 0 : i32
    %c128_i32 = arith.constant 128 : i32
    %c117_i32 = arith.constant 117 : i32
    %20 = llvm.call @fpga_tile_submit(%5, %c0_i32_31, %c2_i32, %c3_i32_32, %c8_i32_33, %c8_i32_34, %c8_i32_35, %17, %19, %4, %c8_i32, %c0_i32_36, %c128_i32, %c117_i32) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %21 = bufferization.to_tensor %2 restrict writable : memref<16x8xf32> to tensor<16x8xf32>
    %c8_i32_37 = arith.constant 8 : i32
    %intptr_38 = memref.extract_aligned_pointer_as_index %2 : memref<16x8xf32> -> index
    %c256 = arith.constant 256 : index
    %22 = arith.addi %intptr_38, %c256 : index
    %23 = arith.index_cast %22 : index to i64
    %24 = llvm.inttoptr %23 : i64 to !llvm.ptr
    %subview_39 = memref.subview %1[8, 0] [8, 16] [1, 1] : memref<16x40xf32> to memref<8x16xf32, strided<[40, 1], offset: 320>>
    %reinterpret_cast_40 = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 16], strides: [16, 1] : memref<128xf32> to memref<8x16xf32>
    memref.copy %subview_39, %reinterpret_cast_40 : memref<8x16xf32, strided<[40, 1], offset: 320>> to memref<8x16xf32>
    %intptr_41 = memref.extract_aligned_pointer_as_index %reinterpret_cast_40 : memref<8x16xf32> -> index
    %25 = arith.index_cast %intptr_41 : index to i64
    %26 = llvm.inttoptr %25 : i64 to !llvm.ptr
    %subview_42 = memref.subview %0[0, 0] [16, 8] [1, 1] : memref<40x8xf32> to memref<16x8xf32, strided<[8, 1]>>
    %reinterpret_cast_43 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 8], strides: [8, 1] : memref<128xf32> to memref<16x8xf32>
    memref.copy %subview_42, %reinterpret_cast_43 : memref<16x8xf32, strided<[8, 1]>> to memref<16x8xf32>
    %intptr_44 = memref.extract_aligned_pointer_as_index %reinterpret_cast_43 : memref<16x8xf32> -> index
    %27 = arith.index_cast %intptr_44 : index to i64
    %28 = llvm.inttoptr %27 : i64 to !llvm.ptr
    %c1_i32_45 = arith.constant 1 : i32
    %c0_i32_46 = arith.constant 0 : i32
    %c3_i32_47 = arith.constant 3 : i32
    %c8_i32_48 = arith.constant 8 : i32
    %c16_i32_49 = arith.constant 16 : i32
    %c8_i32_50 = arith.constant 8 : i32
    %c0_i32_51 = arith.constant 0 : i32
    %c197_i32 = arith.constant 197 : i32
    %c125_i32_52 = arith.constant 125 : i32
    %29 = llvm.call @fpga_tile_submit(%5, %c1_i32_45, %c0_i32_46, %c3_i32_47, %c8_i32_48, %c16_i32_49, %c8_i32_50, %26, %28, %24, %c8_i32_37, %c0_i32_51, %c197_i32, %c125_i32_52) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %subview_53 = memref.subview %1[8, 16] [8, 16] [1, 1] : memref<16x40xf32> to memref<8x16xf32, strided<[40, 1], offset: 336>>
    %reinterpret_cast_54 = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 16], strides: [16, 1] : memref<128xf32> to memref<8x16xf32>
    memref.copy %subview_53, %reinterpret_cast_54 : memref<8x16xf32, strided<[40, 1], offset: 336>> to memref<8x16xf32>
    %intptr_55 = memref.extract_aligned_pointer_as_index %reinterpret_cast_54 : memref<8x16xf32> -> index
    %30 = arith.index_cast %intptr_55 : index to i64
    %31 = llvm.inttoptr %30 : i64 to !llvm.ptr
    %subview_56 = memref.subview %0[16, 0] [16, 8] [1, 1] : memref<40x8xf32> to memref<16x8xf32, strided<[8, 1], offset: 128>>
    %reinterpret_cast_57 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 8], strides: [8, 1] : memref<128xf32> to memref<16x8xf32>
    memref.copy %subview_56, %reinterpret_cast_57 : memref<16x8xf32, strided<[8, 1], offset: 128>> to memref<16x8xf32>
    %intptr_58 = memref.extract_aligned_pointer_as_index %reinterpret_cast_57 : memref<16x8xf32> -> index
    %32 = arith.index_cast %intptr_58 : index to i64
    %33 = llvm.inttoptr %32 : i64 to !llvm.ptr
    %c1_i32_59 = arith.constant 1 : i32
    %c1_i32_60 = arith.constant 1 : i32
    %c3_i32_61 = arith.constant 3 : i32
    %c8_i32_62 = arith.constant 8 : i32
    %c16_i32_63 = arith.constant 16 : i32
    %c8_i32_64 = arith.constant 8 : i32
    %c0_i32_65 = arith.constant 0 : i32
    %c266_i32 = arith.constant 266 : i32
    %c125_i32_66 = arith.constant 125 : i32
    %34 = llvm.call @fpga_tile_submit(%5, %c1_i32_59, %c1_i32_60, %c3_i32_61, %c8_i32_62, %c16_i32_63, %c8_i32_64, %31, %33, %24, %c8_i32_37, %c0_i32_65, %c266_i32, %c125_i32_66) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %subview_67 = memref.subview %1[8, 32] [8, 8] [1, 1] : memref<16x40xf32> to memref<8x8xf32, strided<[40, 1], offset: 352>>
    %reinterpret_cast_68 = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 8], strides: [8, 1] : memref<128xf32> to memref<8x8xf32>
    memref.copy %subview_67, %reinterpret_cast_68 : memref<8x8xf32, strided<[40, 1], offset: 352>> to memref<8x8xf32>
    %intptr_69 = memref.extract_aligned_pointer_as_index %reinterpret_cast_68 : memref<8x8xf32> -> index
    %35 = arith.index_cast %intptr_69 : index to i64
    %36 = llvm.inttoptr %35 : i64 to !llvm.ptr
    %subview_70 = memref.subview %0[32, 0] [8, 8] [1, 1] : memref<40x8xf32> to memref<8x8xf32, strided<[8, 1], offset: 256>>
    %reinterpret_cast_71 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [8, 8], strides: [8, 1] : memref<128xf32> to memref<8x8xf32>
    memref.copy %subview_70, %reinterpret_cast_71 : memref<8x8xf32, strided<[8, 1], offset: 256>> to memref<8x8xf32>
    %intptr_72 = memref.extract_aligned_pointer_as_index %reinterpret_cast_71 : memref<8x8xf32> -> index
    %37 = arith.index_cast %intptr_72 : index to i64
    %38 = llvm.inttoptr %37 : i64 to !llvm.ptr
    %c1_i32_73 = arith.constant 1 : i32
    %c2_i32_74 = arith.constant 2 : i32
    %c3_i32_75 = arith.constant 3 : i32
    %c8_i32_76 = arith.constant 8 : i32
    %c8_i32_77 = arith.constant 8 : i32
    %c8_i32_78 = arith.constant 8 : i32
    %c0_i32_79 = arith.constant 0 : i32
    %c335_i32 = arith.constant 335 : i32
    %c117_i32_80 = arith.constant 117 : i32
    %39 = llvm.call @fpga_tile_submit(%5, %c1_i32_73, %c2_i32_74, %c3_i32_75, %c8_i32_76, %c8_i32_77, %c8_i32_78, %36, %38, %24, %c8_i32_37, %c0_i32_79, %c335_i32, %c117_i32_80) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %40 = bufferization.to_tensor %2 restrict writable : memref<16x8xf32> to tensor<16x8xf32>
    %41 = llvm.call @fpga_gemm_wait(%5) : (!llvm.ptr) -> i32
    return %40 : tensor<16x8xf32>
  }
}

