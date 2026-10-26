module {
  llvm.func @fpga_matmul_tiled_auto_scheduled(i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
  func.func @systolic_matmul(%arg0: tensor<12x35xf32>, %arg1: tensor<35x12xf32>, %arg2: tensor<12x12xf32>) -> tensor<12x12xf32> attributes {llvm.emit_c_interface} {
    %0 = bufferization.to_buffer %arg1 read_only : tensor<35x12xf32> to memref<35x12xf32>
    %1 = bufferization.to_buffer %arg0 read_only : tensor<12x35xf32> to memref<12x35xf32>
    %alloca = memref.alloca() : memref<128xf32>
    %alloca_0 = memref.alloca() : memref<128xf32>
    %alloca_1 = memref.alloca() : memref<64xf32>
    %2 = bufferization.to_buffer %arg2 : tensor<12x12xf32> to memref<12x12xf32>
    %subview = memref.subview %2[0, 0] [8, 8] [1, 1] : memref<12x12xf32> to memref<8x8xf32, strided<[12, 1]>>
    %reinterpret_cast = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [8, 8], strides: [8, 1] : memref<64xf32> to memref<8x8xf32>
    memref.copy %subview, %reinterpret_cast : memref<8x8xf32, strided<[12, 1]>> to memref<8x8xf32>
    %intptr = memref.extract_aligned_pointer_as_index %reinterpret_cast : memref<8x8xf32> -> index
    %3 = arith.index_cast %intptr : index to i64
    %4 = llvm.inttoptr %3 : i64 to !llvm.ptr
    %subview_2 = memref.subview %1[0, 0] [8, 16] [1, 1] : memref<12x35xf32> to memref<8x16xf32, strided<[35, 1]>>
    %reinterpret_cast_3 = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 16], strides: [16, 1] : memref<128xf32> to memref<8x16xf32>
    memref.copy %subview_2, %reinterpret_cast_3 : memref<8x16xf32, strided<[35, 1]>> to memref<8x16xf32>
    %intptr_4 = memref.extract_aligned_pointer_as_index %reinterpret_cast_3 : memref<8x16xf32> -> index
    %5 = arith.index_cast %intptr_4 : index to i64
    %6 = llvm.inttoptr %5 : i64 to !llvm.ptr
    %subview_5 = memref.subview %0[0, 0] [16, 8] [1, 1] : memref<35x12xf32> to memref<16x8xf32, strided<[12, 1]>>
    %reinterpret_cast_6 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 8], strides: [8, 1] : memref<128xf32> to memref<16x8xf32>
    memref.copy %subview_5, %reinterpret_cast_6 : memref<16x8xf32, strided<[12, 1]>> to memref<16x8xf32>
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
    %subview_10 = memref.subview %1[0, 16] [8, 16] [1, 1] : memref<12x35xf32> to memref<8x16xf32, strided<[35, 1], offset: 16>>
    %reinterpret_cast_11 = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 16], strides: [16, 1] : memref<128xf32> to memref<8x16xf32>
    memref.copy %subview_10, %reinterpret_cast_11 : memref<8x16xf32, strided<[35, 1], offset: 16>> to memref<8x16xf32>
    %intptr_12 = memref.extract_aligned_pointer_as_index %reinterpret_cast_11 : memref<8x16xf32> -> index
    %10 = arith.index_cast %intptr_12 : index to i64
    %11 = llvm.inttoptr %10 : i64 to !llvm.ptr
    %subview_13 = memref.subview %0[16, 0] [16, 8] [1, 1] : memref<35x12xf32> to memref<16x8xf32, strided<[12, 1], offset: 192>>
    %reinterpret_cast_14 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 8], strides: [8, 1] : memref<128xf32> to memref<16x8xf32>
    memref.copy %subview_13, %reinterpret_cast_14 : memref<16x8xf32, strided<[12, 1], offset: 192>> to memref<16x8xf32>
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
    %subview_21 = memref.subview %1[0, 32] [8, 3] [1, 1] : memref<12x35xf32> to memref<8x3xf32, strided<[35, 1], offset: 32>>
    %reinterpret_cast_22 = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 3], strides: [3, 1] : memref<128xf32> to memref<8x3xf32>
    memref.copy %subview_21, %reinterpret_cast_22 : memref<8x3xf32, strided<[35, 1], offset: 32>> to memref<8x3xf32>
    %intptr_23 = memref.extract_aligned_pointer_as_index %reinterpret_cast_22 : memref<8x3xf32> -> index
    %15 = arith.index_cast %intptr_23 : index to i64
    %16 = llvm.inttoptr %15 : i64 to !llvm.ptr
    %subview_24 = memref.subview %0[32, 0] [3, 8] [1, 1] : memref<35x12xf32> to memref<3x8xf32, strided<[12, 1], offset: 384>>
    %reinterpret_cast_25 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [3, 8], strides: [8, 1] : memref<128xf32> to memref<3x8xf32>
    memref.copy %subview_24, %reinterpret_cast_25 : memref<3x8xf32, strided<[12, 1], offset: 384>> to memref<3x8xf32>
    %intptr_26 = memref.extract_aligned_pointer_as_index %reinterpret_cast_25 : memref<3x8xf32> -> index
    %17 = arith.index_cast %intptr_26 : index to i64
    %18 = llvm.inttoptr %17 : i64 to !llvm.ptr
    %c8_i32_27 = arith.constant 8 : i32
    %c3_i32 = arith.constant 3 : i32
    %c8_i32_28 = arith.constant 8 : i32
    %c0_i32_29 = arith.constant 0 : i32
    %c128_i32 = arith.constant 128 : i32
    %c112_i32 = arith.constant 112 : i32
    %19 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c8_i32_27, %c3_i32, %c8_i32_28, %16, %18, %4, %c0_i32_29, %c128_i32, %c112_i32) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast, %subview : memref<8x8xf32> to memref<8x8xf32, strided<[12, 1]>>
    %20 = bufferization.to_tensor %2 restrict writable : memref<12x12xf32> to tensor<12x12xf32>
    %subview_30 = memref.subview %2[0, 8] [4, 4] [1, 1] : memref<12x12xf32> to memref<4x4xf32, strided<[12, 1], offset: 8>>
    %reinterpret_cast_31 = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<64xf32> to memref<4x4xf32>
    memref.copy %subview_30, %reinterpret_cast_31 : memref<4x4xf32, strided<[12, 1], offset: 8>> to memref<4x4xf32>
    %intptr_32 = memref.extract_aligned_pointer_as_index %reinterpret_cast_31 : memref<4x4xf32> -> index
    %21 = arith.index_cast %intptr_32 : index to i64
    %22 = llvm.inttoptr %21 : i64 to !llvm.ptr
    %subview_33 = memref.subview %1[0, 0] [4, 16] [1, 1] : memref<12x35xf32> to memref<4x16xf32, strided<[35, 1]>>
    %reinterpret_cast_34 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_33, %reinterpret_cast_34 : memref<4x16xf32, strided<[35, 1]>> to memref<4x16xf32>
    %intptr_35 = memref.extract_aligned_pointer_as_index %reinterpret_cast_34 : memref<4x16xf32> -> index
    %23 = arith.index_cast %intptr_35 : index to i64
    %24 = llvm.inttoptr %23 : i64 to !llvm.ptr
    %subview_36 = memref.subview %0[0, 8] [16, 4] [1, 1] : memref<35x12xf32> to memref<16x4xf32, strided<[12, 1], offset: 8>>
    %reinterpret_cast_37 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_36, %reinterpret_cast_37 : memref<16x4xf32, strided<[12, 1], offset: 8>> to memref<16x4xf32>
    %intptr_38 = memref.extract_aligned_pointer_as_index %reinterpret_cast_37 : memref<16x4xf32> -> index
    %25 = arith.index_cast %intptr_38 : index to i64
    %26 = llvm.inttoptr %25 : i64 to !llvm.ptr
    %c4_i32 = arith.constant 4 : i32
    %c16_i32_39 = arith.constant 16 : i32
    %c4_i32_40 = arith.constant 4 : i32
    %c1_i32 = arith.constant 1 : i32
    %c0_i32_41 = arith.constant 0 : i32
    %c117_i32 = arith.constant 117 : i32
    %27 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32, %c16_i32_39, %c4_i32_40, %24, %26, %22, %c1_i32, %c0_i32_41, %c117_i32) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    %subview_42 = memref.subview %1[0, 16] [4, 16] [1, 1] : memref<12x35xf32> to memref<4x16xf32, strided<[35, 1], offset: 16>>
    %reinterpret_cast_43 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_42, %reinterpret_cast_43 : memref<4x16xf32, strided<[35, 1], offset: 16>> to memref<4x16xf32>
    %intptr_44 = memref.extract_aligned_pointer_as_index %reinterpret_cast_43 : memref<4x16xf32> -> index
    %28 = arith.index_cast %intptr_44 : index to i64
    %29 = llvm.inttoptr %28 : i64 to !llvm.ptr
    %subview_45 = memref.subview %0[16, 8] [16, 4] [1, 1] : memref<35x12xf32> to memref<16x4xf32, strided<[12, 1], offset: 200>>
    %reinterpret_cast_46 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_45, %reinterpret_cast_46 : memref<16x4xf32, strided<[12, 1], offset: 200>> to memref<16x4xf32>
    %intptr_47 = memref.extract_aligned_pointer_as_index %reinterpret_cast_46 : memref<16x4xf32> -> index
    %30 = arith.index_cast %intptr_47 : index to i64
    %31 = llvm.inttoptr %30 : i64 to !llvm.ptr
    %c4_i32_48 = arith.constant 4 : i32
    %c16_i32_49 = arith.constant 16 : i32
    %c4_i32_50 = arith.constant 4 : i32
    %c1_i32_51 = arith.constant 1 : i32
    %c42_i32_52 = arith.constant 42 : i32
    %c117_i32_53 = arith.constant 117 : i32
    %32 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_48, %c16_i32_49, %c4_i32_50, %29, %31, %22, %c1_i32_51, %c42_i32_52, %c117_i32_53) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    %subview_54 = memref.subview %1[0, 32] [4, 3] [1, 1] : memref<12x35xf32> to memref<4x3xf32, strided<[35, 1], offset: 32>>
    %reinterpret_cast_55 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 3], strides: [3, 1] : memref<128xf32> to memref<4x3xf32>
    memref.copy %subview_54, %reinterpret_cast_55 : memref<4x3xf32, strided<[35, 1], offset: 32>> to memref<4x3xf32>
    %intptr_56 = memref.extract_aligned_pointer_as_index %reinterpret_cast_55 : memref<4x3xf32> -> index
    %33 = arith.index_cast %intptr_56 : index to i64
    %34 = llvm.inttoptr %33 : i64 to !llvm.ptr
    %subview_57 = memref.subview %0[32, 8] [3, 4] [1, 1] : memref<35x12xf32> to memref<3x4xf32, strided<[12, 1], offset: 392>>
    %reinterpret_cast_58 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [3, 4], strides: [4, 1] : memref<128xf32> to memref<3x4xf32>
    memref.copy %subview_57, %reinterpret_cast_58 : memref<3x4xf32, strided<[12, 1], offset: 392>> to memref<3x4xf32>
    %intptr_59 = memref.extract_aligned_pointer_as_index %reinterpret_cast_58 : memref<3x4xf32> -> index
    %35 = arith.index_cast %intptr_59 : index to i64
    %36 = llvm.inttoptr %35 : i64 to !llvm.ptr
    %c4_i32_60 = arith.constant 4 : i32
    %c3_i32_61 = arith.constant 3 : i32
    %c4_i32_62 = arith.constant 4 : i32
    %c1_i32_63 = arith.constant 1 : i32
    %c120_i32 = arith.constant 120 : i32
    %c104_i32 = arith.constant 104 : i32
    %37 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_60, %c3_i32_61, %c4_i32_62, %34, %36, %22, %c1_i32_63, %c120_i32, %c104_i32) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast_31, %subview_30 : memref<4x4xf32> to memref<4x4xf32, strided<[12, 1], offset: 8>>
    %38 = bufferization.to_tensor %2 restrict writable : memref<12x12xf32> to tensor<12x12xf32>
    %subview_64 = memref.subview %2[4, 8] [4, 4] [1, 1] : memref<12x12xf32> to memref<4x4xf32, strided<[12, 1], offset: 56>>
    %reinterpret_cast_65 = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<64xf32> to memref<4x4xf32>
    memref.copy %subview_64, %reinterpret_cast_65 : memref<4x4xf32, strided<[12, 1], offset: 56>> to memref<4x4xf32>
    %intptr_66 = memref.extract_aligned_pointer_as_index %reinterpret_cast_65 : memref<4x4xf32> -> index
    %39 = arith.index_cast %intptr_66 : index to i64
    %40 = llvm.inttoptr %39 : i64 to !llvm.ptr
    %subview_67 = memref.subview %1[4, 0] [4, 16] [1, 1] : memref<12x35xf32> to memref<4x16xf32, strided<[35, 1], offset: 140>>
    %reinterpret_cast_68 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_67, %reinterpret_cast_68 : memref<4x16xf32, strided<[35, 1], offset: 140>> to memref<4x16xf32>
    %intptr_69 = memref.extract_aligned_pointer_as_index %reinterpret_cast_68 : memref<4x16xf32> -> index
    %41 = arith.index_cast %intptr_69 : index to i64
    %42 = llvm.inttoptr %41 : i64 to !llvm.ptr
    %subview_70 = memref.subview %0[0, 8] [16, 4] [1, 1] : memref<35x12xf32> to memref<16x4xf32, strided<[12, 1], offset: 8>>
    %reinterpret_cast_71 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_70, %reinterpret_cast_71 : memref<16x4xf32, strided<[12, 1], offset: 8>> to memref<16x4xf32>
    %intptr_72 = memref.extract_aligned_pointer_as_index %reinterpret_cast_71 : memref<16x4xf32> -> index
    %43 = arith.index_cast %intptr_72 : index to i64
    %44 = llvm.inttoptr %43 : i64 to !llvm.ptr
    %c4_i32_73 = arith.constant 4 : i32
    %c16_i32_74 = arith.constant 16 : i32
    %c4_i32_75 = arith.constant 4 : i32
    %c2_i32 = arith.constant 2 : i32
    %c0_i32_76 = arith.constant 0 : i32
    %c117_i32_77 = arith.constant 117 : i32
    %45 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_73, %c16_i32_74, %c4_i32_75, %42, %44, %40, %c2_i32, %c0_i32_76, %c117_i32_77) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    %subview_78 = memref.subview %1[4, 16] [4, 16] [1, 1] : memref<12x35xf32> to memref<4x16xf32, strided<[35, 1], offset: 156>>
    %reinterpret_cast_79 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_78, %reinterpret_cast_79 : memref<4x16xf32, strided<[35, 1], offset: 156>> to memref<4x16xf32>
    %intptr_80 = memref.extract_aligned_pointer_as_index %reinterpret_cast_79 : memref<4x16xf32> -> index
    %46 = arith.index_cast %intptr_80 : index to i64
    %47 = llvm.inttoptr %46 : i64 to !llvm.ptr
    %subview_81 = memref.subview %0[16, 8] [16, 4] [1, 1] : memref<35x12xf32> to memref<16x4xf32, strided<[12, 1], offset: 200>>
    %reinterpret_cast_82 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_81, %reinterpret_cast_82 : memref<16x4xf32, strided<[12, 1], offset: 200>> to memref<16x4xf32>
    %intptr_83 = memref.extract_aligned_pointer_as_index %reinterpret_cast_82 : memref<16x4xf32> -> index
    %48 = arith.index_cast %intptr_83 : index to i64
    %49 = llvm.inttoptr %48 : i64 to !llvm.ptr
    %c4_i32_84 = arith.constant 4 : i32
    %c16_i32_85 = arith.constant 16 : i32
    %c4_i32_86 = arith.constant 4 : i32
    %c2_i32_87 = arith.constant 2 : i32
    %c42_i32_88 = arith.constant 42 : i32
    %c117_i32_89 = arith.constant 117 : i32
    %50 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_84, %c16_i32_85, %c4_i32_86, %47, %49, %40, %c2_i32_87, %c42_i32_88, %c117_i32_89) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    %subview_90 = memref.subview %1[4, 32] [4, 3] [1, 1] : memref<12x35xf32> to memref<4x3xf32, strided<[35, 1], offset: 172>>
    %reinterpret_cast_91 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 3], strides: [3, 1] : memref<128xf32> to memref<4x3xf32>
    memref.copy %subview_90, %reinterpret_cast_91 : memref<4x3xf32, strided<[35, 1], offset: 172>> to memref<4x3xf32>
    %intptr_92 = memref.extract_aligned_pointer_as_index %reinterpret_cast_91 : memref<4x3xf32> -> index
    %51 = arith.index_cast %intptr_92 : index to i64
    %52 = llvm.inttoptr %51 : i64 to !llvm.ptr
    %subview_93 = memref.subview %0[32, 8] [3, 4] [1, 1] : memref<35x12xf32> to memref<3x4xf32, strided<[12, 1], offset: 392>>
    %reinterpret_cast_94 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [3, 4], strides: [4, 1] : memref<128xf32> to memref<3x4xf32>
    memref.copy %subview_93, %reinterpret_cast_94 : memref<3x4xf32, strided<[12, 1], offset: 392>> to memref<3x4xf32>
    %intptr_95 = memref.extract_aligned_pointer_as_index %reinterpret_cast_94 : memref<3x4xf32> -> index
    %53 = arith.index_cast %intptr_95 : index to i64
    %54 = llvm.inttoptr %53 : i64 to !llvm.ptr
    %c4_i32_96 = arith.constant 4 : i32
    %c3_i32_97 = arith.constant 3 : i32
    %c4_i32_98 = arith.constant 4 : i32
    %c2_i32_99 = arith.constant 2 : i32
    %c120_i32_100 = arith.constant 120 : i32
    %c104_i32_101 = arith.constant 104 : i32
    %55 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_96, %c3_i32_97, %c4_i32_98, %52, %54, %40, %c2_i32_99, %c120_i32_100, %c104_i32_101) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast_65, %subview_64 : memref<4x4xf32> to memref<4x4xf32, strided<[12, 1], offset: 56>>
    %56 = bufferization.to_tensor %2 restrict writable : memref<12x12xf32> to tensor<12x12xf32>
    %subview_102 = memref.subview %2[8, 0] [4, 4] [1, 1] : memref<12x12xf32> to memref<4x4xf32, strided<[12, 1], offset: 96>>
    %reinterpret_cast_103 = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<64xf32> to memref<4x4xf32>
    memref.copy %subview_102, %reinterpret_cast_103 : memref<4x4xf32, strided<[12, 1], offset: 96>> to memref<4x4xf32>
    %intptr_104 = memref.extract_aligned_pointer_as_index %reinterpret_cast_103 : memref<4x4xf32> -> index
    %57 = arith.index_cast %intptr_104 : index to i64
    %58 = llvm.inttoptr %57 : i64 to !llvm.ptr
    %subview_105 = memref.subview %1[8, 0] [4, 16] [1, 1] : memref<12x35xf32> to memref<4x16xf32, strided<[35, 1], offset: 280>>
    %reinterpret_cast_106 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_105, %reinterpret_cast_106 : memref<4x16xf32, strided<[35, 1], offset: 280>> to memref<4x16xf32>
    %intptr_107 = memref.extract_aligned_pointer_as_index %reinterpret_cast_106 : memref<4x16xf32> -> index
    %59 = arith.index_cast %intptr_107 : index to i64
    %60 = llvm.inttoptr %59 : i64 to !llvm.ptr
    %subview_108 = memref.subview %0[0, 0] [16, 4] [1, 1] : memref<35x12xf32> to memref<16x4xf32, strided<[12, 1]>>
    %reinterpret_cast_109 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_108, %reinterpret_cast_109 : memref<16x4xf32, strided<[12, 1]>> to memref<16x4xf32>
    %intptr_110 = memref.extract_aligned_pointer_as_index %reinterpret_cast_109 : memref<16x4xf32> -> index
    %61 = arith.index_cast %intptr_110 : index to i64
    %62 = llvm.inttoptr %61 : i64 to !llvm.ptr
    %c4_i32_111 = arith.constant 4 : i32
    %c16_i32_112 = arith.constant 16 : i32
    %c4_i32_113 = arith.constant 4 : i32
    %c3_i32_114 = arith.constant 3 : i32
    %c0_i32_115 = arith.constant 0 : i32
    %c117_i32_116 = arith.constant 117 : i32
    %63 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_111, %c16_i32_112, %c4_i32_113, %60, %62, %58, %c3_i32_114, %c0_i32_115, %c117_i32_116) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    %subview_117 = memref.subview %1[8, 16] [4, 16] [1, 1] : memref<12x35xf32> to memref<4x16xf32, strided<[35, 1], offset: 296>>
    %reinterpret_cast_118 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_117, %reinterpret_cast_118 : memref<4x16xf32, strided<[35, 1], offset: 296>> to memref<4x16xf32>
    %intptr_119 = memref.extract_aligned_pointer_as_index %reinterpret_cast_118 : memref<4x16xf32> -> index
    %64 = arith.index_cast %intptr_119 : index to i64
    %65 = llvm.inttoptr %64 : i64 to !llvm.ptr
    %subview_120 = memref.subview %0[16, 0] [16, 4] [1, 1] : memref<35x12xf32> to memref<16x4xf32, strided<[12, 1], offset: 192>>
    %reinterpret_cast_121 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_120, %reinterpret_cast_121 : memref<16x4xf32, strided<[12, 1], offset: 192>> to memref<16x4xf32>
    %intptr_122 = memref.extract_aligned_pointer_as_index %reinterpret_cast_121 : memref<16x4xf32> -> index
    %66 = arith.index_cast %intptr_122 : index to i64
    %67 = llvm.inttoptr %66 : i64 to !llvm.ptr
    %c4_i32_123 = arith.constant 4 : i32
    %c16_i32_124 = arith.constant 16 : i32
    %c4_i32_125 = arith.constant 4 : i32
    %c3_i32_126 = arith.constant 3 : i32
    %c42_i32_127 = arith.constant 42 : i32
    %c117_i32_128 = arith.constant 117 : i32
    %68 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_123, %c16_i32_124, %c4_i32_125, %65, %67, %58, %c3_i32_126, %c42_i32_127, %c117_i32_128) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    %subview_129 = memref.subview %1[8, 32] [4, 3] [1, 1] : memref<12x35xf32> to memref<4x3xf32, strided<[35, 1], offset: 312>>
    %reinterpret_cast_130 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 3], strides: [3, 1] : memref<128xf32> to memref<4x3xf32>
    memref.copy %subview_129, %reinterpret_cast_130 : memref<4x3xf32, strided<[35, 1], offset: 312>> to memref<4x3xf32>
    %intptr_131 = memref.extract_aligned_pointer_as_index %reinterpret_cast_130 : memref<4x3xf32> -> index
    %69 = arith.index_cast %intptr_131 : index to i64
    %70 = llvm.inttoptr %69 : i64 to !llvm.ptr
    %subview_132 = memref.subview %0[32, 0] [3, 4] [1, 1] : memref<35x12xf32> to memref<3x4xf32, strided<[12, 1], offset: 384>>
    %reinterpret_cast_133 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [3, 4], strides: [4, 1] : memref<128xf32> to memref<3x4xf32>
    memref.copy %subview_132, %reinterpret_cast_133 : memref<3x4xf32, strided<[12, 1], offset: 384>> to memref<3x4xf32>
    %intptr_134 = memref.extract_aligned_pointer_as_index %reinterpret_cast_133 : memref<3x4xf32> -> index
    %71 = arith.index_cast %intptr_134 : index to i64
    %72 = llvm.inttoptr %71 : i64 to !llvm.ptr
    %c4_i32_135 = arith.constant 4 : i32
    %c3_i32_136 = arith.constant 3 : i32
    %c4_i32_137 = arith.constant 4 : i32
    %c3_i32_138 = arith.constant 3 : i32
    %c120_i32_139 = arith.constant 120 : i32
    %c104_i32_140 = arith.constant 104 : i32
    %73 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_135, %c3_i32_136, %c4_i32_137, %70, %72, %58, %c3_i32_138, %c120_i32_139, %c104_i32_140) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast_103, %subview_102 : memref<4x4xf32> to memref<4x4xf32, strided<[12, 1], offset: 96>>
    %74 = bufferization.to_tensor %2 restrict writable : memref<12x12xf32> to tensor<12x12xf32>
    %subview_141 = memref.subview %2[8, 4] [4, 4] [1, 1] : memref<12x12xf32> to memref<4x4xf32, strided<[12, 1], offset: 100>>
    %reinterpret_cast_142 = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<64xf32> to memref<4x4xf32>
    memref.copy %subview_141, %reinterpret_cast_142 : memref<4x4xf32, strided<[12, 1], offset: 100>> to memref<4x4xf32>
    %intptr_143 = memref.extract_aligned_pointer_as_index %reinterpret_cast_142 : memref<4x4xf32> -> index
    %75 = arith.index_cast %intptr_143 : index to i64
    %76 = llvm.inttoptr %75 : i64 to !llvm.ptr
    %subview_144 = memref.subview %1[8, 0] [4, 16] [1, 1] : memref<12x35xf32> to memref<4x16xf32, strided<[35, 1], offset: 280>>
    %reinterpret_cast_145 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_144, %reinterpret_cast_145 : memref<4x16xf32, strided<[35, 1], offset: 280>> to memref<4x16xf32>
    %intptr_146 = memref.extract_aligned_pointer_as_index %reinterpret_cast_145 : memref<4x16xf32> -> index
    %77 = arith.index_cast %intptr_146 : index to i64
    %78 = llvm.inttoptr %77 : i64 to !llvm.ptr
    %subview_147 = memref.subview %0[0, 4] [16, 4] [1, 1] : memref<35x12xf32> to memref<16x4xf32, strided<[12, 1], offset: 4>>
    %reinterpret_cast_148 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_147, %reinterpret_cast_148 : memref<16x4xf32, strided<[12, 1], offset: 4>> to memref<16x4xf32>
    %intptr_149 = memref.extract_aligned_pointer_as_index %reinterpret_cast_148 : memref<16x4xf32> -> index
    %79 = arith.index_cast %intptr_149 : index to i64
    %80 = llvm.inttoptr %79 : i64 to !llvm.ptr
    %c4_i32_150 = arith.constant 4 : i32
    %c16_i32_151 = arith.constant 16 : i32
    %c4_i32_152 = arith.constant 4 : i32
    %c1_i32_153 = arith.constant 1 : i32
    %c189_i32 = arith.constant 189 : i32
    %c117_i32_154 = arith.constant 117 : i32
    %81 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_150, %c16_i32_151, %c4_i32_152, %78, %80, %76, %c1_i32_153, %c189_i32, %c117_i32_154) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    %subview_155 = memref.subview %1[8, 16] [4, 16] [1, 1] : memref<12x35xf32> to memref<4x16xf32, strided<[35, 1], offset: 296>>
    %reinterpret_cast_156 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_155, %reinterpret_cast_156 : memref<4x16xf32, strided<[35, 1], offset: 296>> to memref<4x16xf32>
    %intptr_157 = memref.extract_aligned_pointer_as_index %reinterpret_cast_156 : memref<4x16xf32> -> index
    %82 = arith.index_cast %intptr_157 : index to i64
    %83 = llvm.inttoptr %82 : i64 to !llvm.ptr
    %subview_158 = memref.subview %0[16, 4] [16, 4] [1, 1] : memref<35x12xf32> to memref<16x4xf32, strided<[12, 1], offset: 196>>
    %reinterpret_cast_159 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_158, %reinterpret_cast_159 : memref<16x4xf32, strided<[12, 1], offset: 196>> to memref<16x4xf32>
    %intptr_160 = memref.extract_aligned_pointer_as_index %reinterpret_cast_159 : memref<16x4xf32> -> index
    %84 = arith.index_cast %intptr_160 : index to i64
    %85 = llvm.inttoptr %84 : i64 to !llvm.ptr
    %c4_i32_161 = arith.constant 4 : i32
    %c16_i32_162 = arith.constant 16 : i32
    %c4_i32_163 = arith.constant 4 : i32
    %c1_i32_164 = arith.constant 1 : i32
    %c258_i32 = arith.constant 258 : i32
    %c117_i32_165 = arith.constant 117 : i32
    %86 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_161, %c16_i32_162, %c4_i32_163, %83, %85, %76, %c1_i32_164, %c258_i32, %c117_i32_165) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    %subview_166 = memref.subview %1[8, 32] [4, 3] [1, 1] : memref<12x35xf32> to memref<4x3xf32, strided<[35, 1], offset: 312>>
    %reinterpret_cast_167 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 3], strides: [3, 1] : memref<128xf32> to memref<4x3xf32>
    memref.copy %subview_166, %reinterpret_cast_167 : memref<4x3xf32, strided<[35, 1], offset: 312>> to memref<4x3xf32>
    %intptr_168 = memref.extract_aligned_pointer_as_index %reinterpret_cast_167 : memref<4x3xf32> -> index
    %87 = arith.index_cast %intptr_168 : index to i64
    %88 = llvm.inttoptr %87 : i64 to !llvm.ptr
    %subview_169 = memref.subview %0[32, 4] [3, 4] [1, 1] : memref<35x12xf32> to memref<3x4xf32, strided<[12, 1], offset: 388>>
    %reinterpret_cast_170 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [3, 4], strides: [4, 1] : memref<128xf32> to memref<3x4xf32>
    memref.copy %subview_169, %reinterpret_cast_170 : memref<3x4xf32, strided<[12, 1], offset: 388>> to memref<3x4xf32>
    %intptr_171 = memref.extract_aligned_pointer_as_index %reinterpret_cast_170 : memref<3x4xf32> -> index
    %89 = arith.index_cast %intptr_171 : index to i64
    %90 = llvm.inttoptr %89 : i64 to !llvm.ptr
    %c4_i32_172 = arith.constant 4 : i32
    %c3_i32_173 = arith.constant 3 : i32
    %c4_i32_174 = arith.constant 4 : i32
    %c1_i32_175 = arith.constant 1 : i32
    %c327_i32 = arith.constant 327 : i32
    %c104_i32_176 = arith.constant 104 : i32
    %91 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_172, %c3_i32_173, %c4_i32_174, %88, %90, %76, %c1_i32_175, %c327_i32, %c104_i32_176) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast_142, %subview_141 : memref<4x4xf32> to memref<4x4xf32, strided<[12, 1], offset: 100>>
    %92 = bufferization.to_tensor %2 restrict writable : memref<12x12xf32> to tensor<12x12xf32>
    %subview_177 = memref.subview %2[8, 8] [4, 4] [1, 1] : memref<12x12xf32> to memref<4x4xf32, strided<[12, 1], offset: 104>>
    %reinterpret_cast_178 = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<64xf32> to memref<4x4xf32>
    memref.copy %subview_177, %reinterpret_cast_178 : memref<4x4xf32, strided<[12, 1], offset: 104>> to memref<4x4xf32>
    %intptr_179 = memref.extract_aligned_pointer_as_index %reinterpret_cast_178 : memref<4x4xf32> -> index
    %93 = arith.index_cast %intptr_179 : index to i64
    %94 = llvm.inttoptr %93 : i64 to !llvm.ptr
    %subview_180 = memref.subview %1[8, 0] [4, 16] [1, 1] : memref<12x35xf32> to memref<4x16xf32, strided<[35, 1], offset: 280>>
    %reinterpret_cast_181 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_180, %reinterpret_cast_181 : memref<4x16xf32, strided<[35, 1], offset: 280>> to memref<4x16xf32>
    %intptr_182 = memref.extract_aligned_pointer_as_index %reinterpret_cast_181 : memref<4x16xf32> -> index
    %95 = arith.index_cast %intptr_182 : index to i64
    %96 = llvm.inttoptr %95 : i64 to !llvm.ptr
    %subview_183 = memref.subview %0[0, 8] [16, 4] [1, 1] : memref<35x12xf32> to memref<16x4xf32, strided<[12, 1], offset: 8>>
    %reinterpret_cast_184 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_183, %reinterpret_cast_184 : memref<16x4xf32, strided<[12, 1], offset: 8>> to memref<16x4xf32>
    %intptr_185 = memref.extract_aligned_pointer_as_index %reinterpret_cast_184 : memref<16x4xf32> -> index
    %97 = arith.index_cast %intptr_185 : index to i64
    %98 = llvm.inttoptr %97 : i64 to !llvm.ptr
    %c4_i32_186 = arith.constant 4 : i32
    %c16_i32_187 = arith.constant 16 : i32
    %c4_i32_188 = arith.constant 4 : i32
    %c2_i32_189 = arith.constant 2 : i32
    %c189_i32_190 = arith.constant 189 : i32
    %c117_i32_191 = arith.constant 117 : i32
    %99 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_186, %c16_i32_187, %c4_i32_188, %96, %98, %94, %c2_i32_189, %c189_i32_190, %c117_i32_191) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    %subview_192 = memref.subview %1[8, 16] [4, 16] [1, 1] : memref<12x35xf32> to memref<4x16xf32, strided<[35, 1], offset: 296>>
    %reinterpret_cast_193 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_192, %reinterpret_cast_193 : memref<4x16xf32, strided<[35, 1], offset: 296>> to memref<4x16xf32>
    %intptr_194 = memref.extract_aligned_pointer_as_index %reinterpret_cast_193 : memref<4x16xf32> -> index
    %100 = arith.index_cast %intptr_194 : index to i64
    %101 = llvm.inttoptr %100 : i64 to !llvm.ptr
    %subview_195 = memref.subview %0[16, 8] [16, 4] [1, 1] : memref<35x12xf32> to memref<16x4xf32, strided<[12, 1], offset: 200>>
    %reinterpret_cast_196 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_195, %reinterpret_cast_196 : memref<16x4xf32, strided<[12, 1], offset: 200>> to memref<16x4xf32>
    %intptr_197 = memref.extract_aligned_pointer_as_index %reinterpret_cast_196 : memref<16x4xf32> -> index
    %102 = arith.index_cast %intptr_197 : index to i64
    %103 = llvm.inttoptr %102 : i64 to !llvm.ptr
    %c4_i32_198 = arith.constant 4 : i32
    %c16_i32_199 = arith.constant 16 : i32
    %c4_i32_200 = arith.constant 4 : i32
    %c2_i32_201 = arith.constant 2 : i32
    %c258_i32_202 = arith.constant 258 : i32
    %c117_i32_203 = arith.constant 117 : i32
    %104 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_198, %c16_i32_199, %c4_i32_200, %101, %103, %94, %c2_i32_201, %c258_i32_202, %c117_i32_203) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    %subview_204 = memref.subview %1[8, 32] [4, 3] [1, 1] : memref<12x35xf32> to memref<4x3xf32, strided<[35, 1], offset: 312>>
    %reinterpret_cast_205 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 3], strides: [3, 1] : memref<128xf32> to memref<4x3xf32>
    memref.copy %subview_204, %reinterpret_cast_205 : memref<4x3xf32, strided<[35, 1], offset: 312>> to memref<4x3xf32>
    %intptr_206 = memref.extract_aligned_pointer_as_index %reinterpret_cast_205 : memref<4x3xf32> -> index
    %105 = arith.index_cast %intptr_206 : index to i64
    %106 = llvm.inttoptr %105 : i64 to !llvm.ptr
    %subview_207 = memref.subview %0[32, 8] [3, 4] [1, 1] : memref<35x12xf32> to memref<3x4xf32, strided<[12, 1], offset: 392>>
    %reinterpret_cast_208 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [3, 4], strides: [4, 1] : memref<128xf32> to memref<3x4xf32>
    memref.copy %subview_207, %reinterpret_cast_208 : memref<3x4xf32, strided<[12, 1], offset: 392>> to memref<3x4xf32>
    %intptr_209 = memref.extract_aligned_pointer_as_index %reinterpret_cast_208 : memref<3x4xf32> -> index
    %107 = arith.index_cast %intptr_209 : index to i64
    %108 = llvm.inttoptr %107 : i64 to !llvm.ptr
    %c4_i32_210 = arith.constant 4 : i32
    %c3_i32_211 = arith.constant 3 : i32
    %c4_i32_212 = arith.constant 4 : i32
    %c2_i32_213 = arith.constant 2 : i32
    %c327_i32_214 = arith.constant 327 : i32
    %c104_i32_215 = arith.constant 104 : i32
    %109 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_210, %c3_i32_211, %c4_i32_212, %106, %108, %94, %c2_i32_213, %c327_i32_214, %c104_i32_215) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast_178, %subview_177 : memref<4x4xf32> to memref<4x4xf32, strided<[12, 1], offset: 104>>
    %110 = bufferization.to_tensor %2 restrict writable : memref<12x12xf32> to tensor<12x12xf32>
    return %110 : tensor<12x12xf32>
  }
}

