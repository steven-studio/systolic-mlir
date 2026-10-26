module {
  llvm.func @fpga_matmul_tiled_auto_scheduled(i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
  func.func @systolic_matmul(%arg0: tensor<12x17xf32>, %arg1: tensor<17x12xf32>, %arg2: tensor<12x12xf32>) -> tensor<12x12xf32> attributes {llvm.emit_c_interface} {
    %0 = bufferization.to_buffer %arg1 read_only : tensor<17x12xf32> to memref<17x12xf32>
    %1 = bufferization.to_buffer %arg0 read_only : tensor<12x17xf32> to memref<12x17xf32>
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
    %subview_2 = memref.subview %1[0, 0] [8, 16] [1, 1] : memref<12x17xf32> to memref<8x16xf32, strided<[17, 1]>>
    %reinterpret_cast_3 = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 16], strides: [16, 1] : memref<128xf32> to memref<8x16xf32>
    memref.copy %subview_2, %reinterpret_cast_3 : memref<8x16xf32, strided<[17, 1]>> to memref<8x16xf32>
    %intptr_4 = memref.extract_aligned_pointer_as_index %reinterpret_cast_3 : memref<8x16xf32> -> index
    %5 = arith.index_cast %intptr_4 : index to i64
    %6 = llvm.inttoptr %5 : i64 to !llvm.ptr
    %subview_5 = memref.subview %0[0, 0] [16, 8] [1, 1] : memref<17x12xf32> to memref<16x8xf32, strided<[12, 1]>>
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
    %subview_10 = memref.subview %1[0, 16] [8, 1] [1, 1] : memref<12x17xf32> to memref<8x1xf32, strided<[17, 1], offset: 16>>
    %reinterpret_cast_11 = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 1], strides: [1, 1] : memref<128xf32> to memref<8x1xf32>
    memref.copy %subview_10, %reinterpret_cast_11 : memref<8x1xf32, strided<[17, 1], offset: 16>> to memref<8x1xf32>
    %intptr_12 = memref.extract_aligned_pointer_as_index %reinterpret_cast_11 : memref<8x1xf32> -> index
    %10 = arith.index_cast %intptr_12 : index to i64
    %11 = llvm.inttoptr %10 : i64 to !llvm.ptr
    %subview_13 = memref.subview %0[16, 0] [1, 8] [1, 1] : memref<17x12xf32> to memref<1x8xf32, strided<[12, 1], offset: 192>>
    %reinterpret_cast_14 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [1, 8], strides: [8, 1] : memref<128xf32> to memref<1x8xf32>
    memref.copy %subview_13, %reinterpret_cast_14 : memref<1x8xf32, strided<[12, 1], offset: 192>> to memref<1x8xf32>
    %intptr_15 = memref.extract_aligned_pointer_as_index %reinterpret_cast_14 : memref<1x8xf32> -> index
    %12 = arith.index_cast %intptr_15 : index to i64
    %13 = llvm.inttoptr %12 : i64 to !llvm.ptr
    %c8_i32_16 = arith.constant 8 : i32
    %c1_i32 = arith.constant 1 : i32
    %c8_i32_17 = arith.constant 8 : i32
    %c0_i32_18 = arith.constant 0 : i32
    %c42_i32 = arith.constant 42 : i32
    %c110_i32 = arith.constant 110 : i32
    %14 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c8_i32_16, %c1_i32, %c8_i32_17, %11, %13, %4, %c0_i32_18, %c42_i32, %c110_i32) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast, %subview : memref<8x8xf32> to memref<8x8xf32, strided<[12, 1]>>
    %15 = bufferization.to_tensor %2 restrict writable : memref<12x12xf32> to tensor<12x12xf32>
    %subview_19 = memref.subview %2[0, 8] [4, 4] [1, 1] : memref<12x12xf32> to memref<4x4xf32, strided<[12, 1], offset: 8>>
    %reinterpret_cast_20 = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<64xf32> to memref<4x4xf32>
    memref.copy %subview_19, %reinterpret_cast_20 : memref<4x4xf32, strided<[12, 1], offset: 8>> to memref<4x4xf32>
    %intptr_21 = memref.extract_aligned_pointer_as_index %reinterpret_cast_20 : memref<4x4xf32> -> index
    %16 = arith.index_cast %intptr_21 : index to i64
    %17 = llvm.inttoptr %16 : i64 to !llvm.ptr
    %subview_22 = memref.subview %1[0, 0] [4, 16] [1, 1] : memref<12x17xf32> to memref<4x16xf32, strided<[17, 1]>>
    %reinterpret_cast_23 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_22, %reinterpret_cast_23 : memref<4x16xf32, strided<[17, 1]>> to memref<4x16xf32>
    %intptr_24 = memref.extract_aligned_pointer_as_index %reinterpret_cast_23 : memref<4x16xf32> -> index
    %18 = arith.index_cast %intptr_24 : index to i64
    %19 = llvm.inttoptr %18 : i64 to !llvm.ptr
    %subview_25 = memref.subview %0[0, 8] [16, 4] [1, 1] : memref<17x12xf32> to memref<16x4xf32, strided<[12, 1], offset: 8>>
    %reinterpret_cast_26 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_25, %reinterpret_cast_26 : memref<16x4xf32, strided<[12, 1], offset: 8>> to memref<16x4xf32>
    %intptr_27 = memref.extract_aligned_pointer_as_index %reinterpret_cast_26 : memref<16x4xf32> -> index
    %20 = arith.index_cast %intptr_27 : index to i64
    %21 = llvm.inttoptr %20 : i64 to !llvm.ptr
    %c4_i32 = arith.constant 4 : i32
    %c16_i32_28 = arith.constant 16 : i32
    %c4_i32_29 = arith.constant 4 : i32
    %c1_i32_30 = arith.constant 1 : i32
    %c0_i32_31 = arith.constant 0 : i32
    %c117_i32 = arith.constant 117 : i32
    %22 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32, %c16_i32_28, %c4_i32_29, %19, %21, %17, %c1_i32_30, %c0_i32_31, %c117_i32) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    %subview_32 = memref.subview %1[0, 16] [4, 1] [1, 1] : memref<12x17xf32> to memref<4x1xf32, strided<[17, 1], offset: 16>>
    %reinterpret_cast_33 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 1], strides: [1, 1] : memref<128xf32> to memref<4x1xf32>
    memref.copy %subview_32, %reinterpret_cast_33 : memref<4x1xf32, strided<[17, 1], offset: 16>> to memref<4x1xf32>
    %intptr_34 = memref.extract_aligned_pointer_as_index %reinterpret_cast_33 : memref<4x1xf32> -> index
    %23 = arith.index_cast %intptr_34 : index to i64
    %24 = llvm.inttoptr %23 : i64 to !llvm.ptr
    %subview_35 = memref.subview %0[16, 8] [1, 4] [1, 1] : memref<17x12xf32> to memref<1x4xf32, strided<[12, 1], offset: 200>>
    %reinterpret_cast_36 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [1, 4], strides: [4, 1] : memref<128xf32> to memref<1x4xf32>
    memref.copy %subview_35, %reinterpret_cast_36 : memref<1x4xf32, strided<[12, 1], offset: 200>> to memref<1x4xf32>
    %intptr_37 = memref.extract_aligned_pointer_as_index %reinterpret_cast_36 : memref<1x4xf32> -> index
    %25 = arith.index_cast %intptr_37 : index to i64
    %26 = llvm.inttoptr %25 : i64 to !llvm.ptr
    %c4_i32_38 = arith.constant 4 : i32
    %c1_i32_39 = arith.constant 1 : i32
    %c4_i32_40 = arith.constant 4 : i32
    %c1_i32_41 = arith.constant 1 : i32
    %c42_i32_42 = arith.constant 42 : i32
    %c102_i32 = arith.constant 102 : i32
    %27 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_38, %c1_i32_39, %c4_i32_40, %24, %26, %17, %c1_i32_41, %c42_i32_42, %c102_i32) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast_20, %subview_19 : memref<4x4xf32> to memref<4x4xf32, strided<[12, 1], offset: 8>>
    %28 = bufferization.to_tensor %2 restrict writable : memref<12x12xf32> to tensor<12x12xf32>
    %subview_43 = memref.subview %2[4, 8] [4, 4] [1, 1] : memref<12x12xf32> to memref<4x4xf32, strided<[12, 1], offset: 56>>
    %reinterpret_cast_44 = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<64xf32> to memref<4x4xf32>
    memref.copy %subview_43, %reinterpret_cast_44 : memref<4x4xf32, strided<[12, 1], offset: 56>> to memref<4x4xf32>
    %intptr_45 = memref.extract_aligned_pointer_as_index %reinterpret_cast_44 : memref<4x4xf32> -> index
    %29 = arith.index_cast %intptr_45 : index to i64
    %30 = llvm.inttoptr %29 : i64 to !llvm.ptr
    %subview_46 = memref.subview %1[4, 0] [4, 16] [1, 1] : memref<12x17xf32> to memref<4x16xf32, strided<[17, 1], offset: 68>>
    %reinterpret_cast_47 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_46, %reinterpret_cast_47 : memref<4x16xf32, strided<[17, 1], offset: 68>> to memref<4x16xf32>
    %intptr_48 = memref.extract_aligned_pointer_as_index %reinterpret_cast_47 : memref<4x16xf32> -> index
    %31 = arith.index_cast %intptr_48 : index to i64
    %32 = llvm.inttoptr %31 : i64 to !llvm.ptr
    %subview_49 = memref.subview %0[0, 8] [16, 4] [1, 1] : memref<17x12xf32> to memref<16x4xf32, strided<[12, 1], offset: 8>>
    %reinterpret_cast_50 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_49, %reinterpret_cast_50 : memref<16x4xf32, strided<[12, 1], offset: 8>> to memref<16x4xf32>
    %intptr_51 = memref.extract_aligned_pointer_as_index %reinterpret_cast_50 : memref<16x4xf32> -> index
    %33 = arith.index_cast %intptr_51 : index to i64
    %34 = llvm.inttoptr %33 : i64 to !llvm.ptr
    %c4_i32_52 = arith.constant 4 : i32
    %c16_i32_53 = arith.constant 16 : i32
    %c4_i32_54 = arith.constant 4 : i32
    %c2_i32 = arith.constant 2 : i32
    %c0_i32_55 = arith.constant 0 : i32
    %c117_i32_56 = arith.constant 117 : i32
    %35 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_52, %c16_i32_53, %c4_i32_54, %32, %34, %30, %c2_i32, %c0_i32_55, %c117_i32_56) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    %subview_57 = memref.subview %1[4, 16] [4, 1] [1, 1] : memref<12x17xf32> to memref<4x1xf32, strided<[17, 1], offset: 84>>
    %reinterpret_cast_58 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 1], strides: [1, 1] : memref<128xf32> to memref<4x1xf32>
    memref.copy %subview_57, %reinterpret_cast_58 : memref<4x1xf32, strided<[17, 1], offset: 84>> to memref<4x1xf32>
    %intptr_59 = memref.extract_aligned_pointer_as_index %reinterpret_cast_58 : memref<4x1xf32> -> index
    %36 = arith.index_cast %intptr_59 : index to i64
    %37 = llvm.inttoptr %36 : i64 to !llvm.ptr
    %subview_60 = memref.subview %0[16, 8] [1, 4] [1, 1] : memref<17x12xf32> to memref<1x4xf32, strided<[12, 1], offset: 200>>
    %reinterpret_cast_61 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [1, 4], strides: [4, 1] : memref<128xf32> to memref<1x4xf32>
    memref.copy %subview_60, %reinterpret_cast_61 : memref<1x4xf32, strided<[12, 1], offset: 200>> to memref<1x4xf32>
    %intptr_62 = memref.extract_aligned_pointer_as_index %reinterpret_cast_61 : memref<1x4xf32> -> index
    %38 = arith.index_cast %intptr_62 : index to i64
    %39 = llvm.inttoptr %38 : i64 to !llvm.ptr
    %c4_i32_63 = arith.constant 4 : i32
    %c1_i32_64 = arith.constant 1 : i32
    %c4_i32_65 = arith.constant 4 : i32
    %c2_i32_66 = arith.constant 2 : i32
    %c42_i32_67 = arith.constant 42 : i32
    %c102_i32_68 = arith.constant 102 : i32
    %40 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_63, %c1_i32_64, %c4_i32_65, %37, %39, %30, %c2_i32_66, %c42_i32_67, %c102_i32_68) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast_44, %subview_43 : memref<4x4xf32> to memref<4x4xf32, strided<[12, 1], offset: 56>>
    %41 = bufferization.to_tensor %2 restrict writable : memref<12x12xf32> to tensor<12x12xf32>
    %subview_69 = memref.subview %2[8, 0] [4, 4] [1, 1] : memref<12x12xf32> to memref<4x4xf32, strided<[12, 1], offset: 96>>
    %reinterpret_cast_70 = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<64xf32> to memref<4x4xf32>
    memref.copy %subview_69, %reinterpret_cast_70 : memref<4x4xf32, strided<[12, 1], offset: 96>> to memref<4x4xf32>
    %intptr_71 = memref.extract_aligned_pointer_as_index %reinterpret_cast_70 : memref<4x4xf32> -> index
    %42 = arith.index_cast %intptr_71 : index to i64
    %43 = llvm.inttoptr %42 : i64 to !llvm.ptr
    %subview_72 = memref.subview %1[8, 0] [4, 16] [1, 1] : memref<12x17xf32> to memref<4x16xf32, strided<[17, 1], offset: 136>>
    %reinterpret_cast_73 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_72, %reinterpret_cast_73 : memref<4x16xf32, strided<[17, 1], offset: 136>> to memref<4x16xf32>
    %intptr_74 = memref.extract_aligned_pointer_as_index %reinterpret_cast_73 : memref<4x16xf32> -> index
    %44 = arith.index_cast %intptr_74 : index to i64
    %45 = llvm.inttoptr %44 : i64 to !llvm.ptr
    %subview_75 = memref.subview %0[0, 0] [16, 4] [1, 1] : memref<17x12xf32> to memref<16x4xf32, strided<[12, 1]>>
    %reinterpret_cast_76 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_75, %reinterpret_cast_76 : memref<16x4xf32, strided<[12, 1]>> to memref<16x4xf32>
    %intptr_77 = memref.extract_aligned_pointer_as_index %reinterpret_cast_76 : memref<16x4xf32> -> index
    %46 = arith.index_cast %intptr_77 : index to i64
    %47 = llvm.inttoptr %46 : i64 to !llvm.ptr
    %c4_i32_78 = arith.constant 4 : i32
    %c16_i32_79 = arith.constant 16 : i32
    %c4_i32_80 = arith.constant 4 : i32
    %c3_i32 = arith.constant 3 : i32
    %c0_i32_81 = arith.constant 0 : i32
    %c117_i32_82 = arith.constant 117 : i32
    %48 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_78, %c16_i32_79, %c4_i32_80, %45, %47, %43, %c3_i32, %c0_i32_81, %c117_i32_82) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    %subview_83 = memref.subview %1[8, 16] [4, 1] [1, 1] : memref<12x17xf32> to memref<4x1xf32, strided<[17, 1], offset: 152>>
    %reinterpret_cast_84 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 1], strides: [1, 1] : memref<128xf32> to memref<4x1xf32>
    memref.copy %subview_83, %reinterpret_cast_84 : memref<4x1xf32, strided<[17, 1], offset: 152>> to memref<4x1xf32>
    %intptr_85 = memref.extract_aligned_pointer_as_index %reinterpret_cast_84 : memref<4x1xf32> -> index
    %49 = arith.index_cast %intptr_85 : index to i64
    %50 = llvm.inttoptr %49 : i64 to !llvm.ptr
    %subview_86 = memref.subview %0[16, 0] [1, 4] [1, 1] : memref<17x12xf32> to memref<1x4xf32, strided<[12, 1], offset: 192>>
    %reinterpret_cast_87 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [1, 4], strides: [4, 1] : memref<128xf32> to memref<1x4xf32>
    memref.copy %subview_86, %reinterpret_cast_87 : memref<1x4xf32, strided<[12, 1], offset: 192>> to memref<1x4xf32>
    %intptr_88 = memref.extract_aligned_pointer_as_index %reinterpret_cast_87 : memref<1x4xf32> -> index
    %51 = arith.index_cast %intptr_88 : index to i64
    %52 = llvm.inttoptr %51 : i64 to !llvm.ptr
    %c4_i32_89 = arith.constant 4 : i32
    %c1_i32_90 = arith.constant 1 : i32
    %c4_i32_91 = arith.constant 4 : i32
    %c3_i32_92 = arith.constant 3 : i32
    %c42_i32_93 = arith.constant 42 : i32
    %c102_i32_94 = arith.constant 102 : i32
    %53 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_89, %c1_i32_90, %c4_i32_91, %50, %52, %43, %c3_i32_92, %c42_i32_93, %c102_i32_94) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast_70, %subview_69 : memref<4x4xf32> to memref<4x4xf32, strided<[12, 1], offset: 96>>
    %54 = bufferization.to_tensor %2 restrict writable : memref<12x12xf32> to tensor<12x12xf32>
    %subview_95 = memref.subview %2[8, 4] [4, 4] [1, 1] : memref<12x12xf32> to memref<4x4xf32, strided<[12, 1], offset: 100>>
    %reinterpret_cast_96 = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<64xf32> to memref<4x4xf32>
    memref.copy %subview_95, %reinterpret_cast_96 : memref<4x4xf32, strided<[12, 1], offset: 100>> to memref<4x4xf32>
    %intptr_97 = memref.extract_aligned_pointer_as_index %reinterpret_cast_96 : memref<4x4xf32> -> index
    %55 = arith.index_cast %intptr_97 : index to i64
    %56 = llvm.inttoptr %55 : i64 to !llvm.ptr
    %subview_98 = memref.subview %1[8, 0] [4, 16] [1, 1] : memref<12x17xf32> to memref<4x16xf32, strided<[17, 1], offset: 136>>
    %reinterpret_cast_99 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_98, %reinterpret_cast_99 : memref<4x16xf32, strided<[17, 1], offset: 136>> to memref<4x16xf32>
    %intptr_100 = memref.extract_aligned_pointer_as_index %reinterpret_cast_99 : memref<4x16xf32> -> index
    %57 = arith.index_cast %intptr_100 : index to i64
    %58 = llvm.inttoptr %57 : i64 to !llvm.ptr
    %subview_101 = memref.subview %0[0, 4] [16, 4] [1, 1] : memref<17x12xf32> to memref<16x4xf32, strided<[12, 1], offset: 4>>
    %reinterpret_cast_102 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_101, %reinterpret_cast_102 : memref<16x4xf32, strided<[12, 1], offset: 4>> to memref<16x4xf32>
    %intptr_103 = memref.extract_aligned_pointer_as_index %reinterpret_cast_102 : memref<16x4xf32> -> index
    %59 = arith.index_cast %intptr_103 : index to i64
    %60 = llvm.inttoptr %59 : i64 to !llvm.ptr
    %c4_i32_104 = arith.constant 4 : i32
    %c16_i32_105 = arith.constant 16 : i32
    %c4_i32_106 = arith.constant 4 : i32
    %c1_i32_107 = arith.constant 1 : i32
    %c120_i32 = arith.constant 120 : i32
    %c117_i32_108 = arith.constant 117 : i32
    %61 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_104, %c16_i32_105, %c4_i32_106, %58, %60, %56, %c1_i32_107, %c120_i32, %c117_i32_108) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    %subview_109 = memref.subview %1[8, 16] [4, 1] [1, 1] : memref<12x17xf32> to memref<4x1xf32, strided<[17, 1], offset: 152>>
    %reinterpret_cast_110 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 1], strides: [1, 1] : memref<128xf32> to memref<4x1xf32>
    memref.copy %subview_109, %reinterpret_cast_110 : memref<4x1xf32, strided<[17, 1], offset: 152>> to memref<4x1xf32>
    %intptr_111 = memref.extract_aligned_pointer_as_index %reinterpret_cast_110 : memref<4x1xf32> -> index
    %62 = arith.index_cast %intptr_111 : index to i64
    %63 = llvm.inttoptr %62 : i64 to !llvm.ptr
    %subview_112 = memref.subview %0[16, 4] [1, 4] [1, 1] : memref<17x12xf32> to memref<1x4xf32, strided<[12, 1], offset: 196>>
    %reinterpret_cast_113 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [1, 4], strides: [4, 1] : memref<128xf32> to memref<1x4xf32>
    memref.copy %subview_112, %reinterpret_cast_113 : memref<1x4xf32, strided<[12, 1], offset: 196>> to memref<1x4xf32>
    %intptr_114 = memref.extract_aligned_pointer_as_index %reinterpret_cast_113 : memref<1x4xf32> -> index
    %64 = arith.index_cast %intptr_114 : index to i64
    %65 = llvm.inttoptr %64 : i64 to !llvm.ptr
    %c4_i32_115 = arith.constant 4 : i32
    %c1_i32_116 = arith.constant 1 : i32
    %c4_i32_117 = arith.constant 4 : i32
    %c1_i32_118 = arith.constant 1 : i32
    %c189_i32 = arith.constant 189 : i32
    %c102_i32_119 = arith.constant 102 : i32
    %66 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_115, %c1_i32_116, %c4_i32_117, %63, %65, %56, %c1_i32_118, %c189_i32, %c102_i32_119) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast_96, %subview_95 : memref<4x4xf32> to memref<4x4xf32, strided<[12, 1], offset: 100>>
    %67 = bufferization.to_tensor %2 restrict writable : memref<12x12xf32> to tensor<12x12xf32>
    %subview_120 = memref.subview %2[8, 8] [4, 4] [1, 1] : memref<12x12xf32> to memref<4x4xf32, strided<[12, 1], offset: 104>>
    %reinterpret_cast_121 = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<64xf32> to memref<4x4xf32>
    memref.copy %subview_120, %reinterpret_cast_121 : memref<4x4xf32, strided<[12, 1], offset: 104>> to memref<4x4xf32>
    %intptr_122 = memref.extract_aligned_pointer_as_index %reinterpret_cast_121 : memref<4x4xf32> -> index
    %68 = arith.index_cast %intptr_122 : index to i64
    %69 = llvm.inttoptr %68 : i64 to !llvm.ptr
    %subview_123 = memref.subview %1[8, 0] [4, 16] [1, 1] : memref<12x17xf32> to memref<4x16xf32, strided<[17, 1], offset: 136>>
    %reinterpret_cast_124 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_123, %reinterpret_cast_124 : memref<4x16xf32, strided<[17, 1], offset: 136>> to memref<4x16xf32>
    %intptr_125 = memref.extract_aligned_pointer_as_index %reinterpret_cast_124 : memref<4x16xf32> -> index
    %70 = arith.index_cast %intptr_125 : index to i64
    %71 = llvm.inttoptr %70 : i64 to !llvm.ptr
    %subview_126 = memref.subview %0[0, 8] [16, 4] [1, 1] : memref<17x12xf32> to memref<16x4xf32, strided<[12, 1], offset: 8>>
    %reinterpret_cast_127 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_126, %reinterpret_cast_127 : memref<16x4xf32, strided<[12, 1], offset: 8>> to memref<16x4xf32>
    %intptr_128 = memref.extract_aligned_pointer_as_index %reinterpret_cast_127 : memref<16x4xf32> -> index
    %72 = arith.index_cast %intptr_128 : index to i64
    %73 = llvm.inttoptr %72 : i64 to !llvm.ptr
    %c4_i32_129 = arith.constant 4 : i32
    %c16_i32_130 = arith.constant 16 : i32
    %c4_i32_131 = arith.constant 4 : i32
    %c2_i32_132 = arith.constant 2 : i32
    %c120_i32_133 = arith.constant 120 : i32
    %c117_i32_134 = arith.constant 117 : i32
    %74 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_129, %c16_i32_130, %c4_i32_131, %71, %73, %69, %c2_i32_132, %c120_i32_133, %c117_i32_134) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    %subview_135 = memref.subview %1[8, 16] [4, 1] [1, 1] : memref<12x17xf32> to memref<4x1xf32, strided<[17, 1], offset: 152>>
    %reinterpret_cast_136 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 1], strides: [1, 1] : memref<128xf32> to memref<4x1xf32>
    memref.copy %subview_135, %reinterpret_cast_136 : memref<4x1xf32, strided<[17, 1], offset: 152>> to memref<4x1xf32>
    %intptr_137 = memref.extract_aligned_pointer_as_index %reinterpret_cast_136 : memref<4x1xf32> -> index
    %75 = arith.index_cast %intptr_137 : index to i64
    %76 = llvm.inttoptr %75 : i64 to !llvm.ptr
    %subview_138 = memref.subview %0[16, 8] [1, 4] [1, 1] : memref<17x12xf32> to memref<1x4xf32, strided<[12, 1], offset: 200>>
    %reinterpret_cast_139 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [1, 4], strides: [4, 1] : memref<128xf32> to memref<1x4xf32>
    memref.copy %subview_138, %reinterpret_cast_139 : memref<1x4xf32, strided<[12, 1], offset: 200>> to memref<1x4xf32>
    %intptr_140 = memref.extract_aligned_pointer_as_index %reinterpret_cast_139 : memref<1x4xf32> -> index
    %77 = arith.index_cast %intptr_140 : index to i64
    %78 = llvm.inttoptr %77 : i64 to !llvm.ptr
    %c4_i32_141 = arith.constant 4 : i32
    %c1_i32_142 = arith.constant 1 : i32
    %c4_i32_143 = arith.constant 4 : i32
    %c2_i32_144 = arith.constant 2 : i32
    %c189_i32_145 = arith.constant 189 : i32
    %c102_i32_146 = arith.constant 102 : i32
    %79 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_141, %c1_i32_142, %c4_i32_143, %76, %78, %69, %c2_i32_144, %c189_i32_145, %c102_i32_146) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast_121, %subview_120 : memref<4x4xf32> to memref<4x4xf32, strided<[12, 1], offset: 104>>
    %80 = bufferization.to_tensor %2 restrict writable : memref<12x12xf32> to tensor<12x12xf32>
    return %80 : tensor<12x12xf32>
  }
}

