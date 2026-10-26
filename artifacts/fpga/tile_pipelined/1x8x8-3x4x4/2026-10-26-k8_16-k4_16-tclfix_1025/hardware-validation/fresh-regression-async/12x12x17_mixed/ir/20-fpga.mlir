module {
  llvm.func @fpga_gemm_wait(!llvm.ptr) -> i32
  llvm.func @fpga_tile_submit(!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
  llvm.func @fpga_gemm_begin(i32, i32, i32, i32) -> !llvm.ptr
  func.func @systolic_matmul(%arg0: tensor<12x17xf32>, %arg1: tensor<17x12xf32>, %arg2: tensor<12x12xf32>) -> tensor<12x12xf32> attributes {llvm.emit_c_interface} {
    %0 = bufferization.to_buffer %arg1 read_only : tensor<17x12xf32> to memref<17x12xf32>
    %1 = bufferization.to_buffer %arg0 read_only : tensor<12x17xf32> to memref<12x17xf32>
    %alloca = memref.alloca() : memref<128xf32>
    %alloca_0 = memref.alloca() : memref<128xf32>
    %2 = bufferization.to_buffer %arg2 : tensor<12x12xf32> to memref<12x12xf32>
    %c12_i32 = arith.constant 12 : i32
    %intptr = memref.extract_aligned_pointer_as_index %2 : memref<12x12xf32> -> index
    %3 = arith.index_cast %intptr : index to i64
    %4 = llvm.inttoptr %3 : i64 to !llvm.ptr
    %c12_i32_1 = arith.constant 12 : i32
    %c12_i32_2 = arith.constant 12 : i32
    %c17_i32 = arith.constant 17 : i32
    %c12_i32_3 = arith.constant 12 : i32
    %5 = llvm.call @fpga_gemm_begin(%c12_i32_1, %c12_i32_2, %c17_i32, %c12_i32_3) : (i32, i32, i32, i32) -> !llvm.ptr
    %subview = memref.subview %1[0, 0] [8, 16] [1, 1] : memref<12x17xf32> to memref<8x16xf32, strided<[17, 1]>>
    %reinterpret_cast = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 16], strides: [16, 1] : memref<128xf32> to memref<8x16xf32>
    memref.copy %subview, %reinterpret_cast : memref<8x16xf32, strided<[17, 1]>> to memref<8x16xf32>
    %intptr_4 = memref.extract_aligned_pointer_as_index %reinterpret_cast : memref<8x16xf32> -> index
    %6 = arith.index_cast %intptr_4 : index to i64
    %7 = llvm.inttoptr %6 : i64 to !llvm.ptr
    %subview_5 = memref.subview %0[0, 0] [16, 8] [1, 1] : memref<17x12xf32> to memref<16x8xf32, strided<[12, 1]>>
    %reinterpret_cast_6 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 8], strides: [8, 1] : memref<128xf32> to memref<16x8xf32>
    memref.copy %subview_5, %reinterpret_cast_6 : memref<16x8xf32, strided<[12, 1]>> to memref<16x8xf32>
    %intptr_7 = memref.extract_aligned_pointer_as_index %reinterpret_cast_6 : memref<16x8xf32> -> index
    %8 = arith.index_cast %intptr_7 : index to i64
    %9 = llvm.inttoptr %8 : i64 to !llvm.ptr
    %c0_i32 = arith.constant 0 : i32
    %c0_i32_8 = arith.constant 0 : i32
    %c2_i32 = arith.constant 2 : i32
    %c8_i32 = arith.constant 8 : i32
    %c16_i32 = arith.constant 16 : i32
    %c8_i32_9 = arith.constant 8 : i32
    %c0_i32_10 = arith.constant 0 : i32
    %c0_i32_11 = arith.constant 0 : i32
    %c125_i32 = arith.constant 125 : i32
    %10 = llvm.call @fpga_tile_submit(%5, %c0_i32, %c0_i32_8, %c2_i32, %c8_i32, %c16_i32, %c8_i32_9, %7, %9, %4, %c12_i32, %c0_i32_10, %c0_i32_11, %c125_i32) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %subview_12 = memref.subview %1[0, 16] [8, 1] [1, 1] : memref<12x17xf32> to memref<8x1xf32, strided<[17, 1], offset: 16>>
    %reinterpret_cast_13 = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 1], strides: [1, 1] : memref<128xf32> to memref<8x1xf32>
    memref.copy %subview_12, %reinterpret_cast_13 : memref<8x1xf32, strided<[17, 1], offset: 16>> to memref<8x1xf32>
    %intptr_14 = memref.extract_aligned_pointer_as_index %reinterpret_cast_13 : memref<8x1xf32> -> index
    %11 = arith.index_cast %intptr_14 : index to i64
    %12 = llvm.inttoptr %11 : i64 to !llvm.ptr
    %subview_15 = memref.subview %0[16, 0] [1, 8] [1, 1] : memref<17x12xf32> to memref<1x8xf32, strided<[12, 1], offset: 192>>
    %reinterpret_cast_16 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [1, 8], strides: [8, 1] : memref<128xf32> to memref<1x8xf32>
    memref.copy %subview_15, %reinterpret_cast_16 : memref<1x8xf32, strided<[12, 1], offset: 192>> to memref<1x8xf32>
    %intptr_17 = memref.extract_aligned_pointer_as_index %reinterpret_cast_16 : memref<1x8xf32> -> index
    %13 = arith.index_cast %intptr_17 : index to i64
    %14 = llvm.inttoptr %13 : i64 to !llvm.ptr
    %c0_i32_18 = arith.constant 0 : i32
    %c1_i32 = arith.constant 1 : i32
    %c2_i32_19 = arith.constant 2 : i32
    %c8_i32_20 = arith.constant 8 : i32
    %c1_i32_21 = arith.constant 1 : i32
    %c8_i32_22 = arith.constant 8 : i32
    %c0_i32_23 = arith.constant 0 : i32
    %c42_i32 = arith.constant 42 : i32
    %c110_i32 = arith.constant 110 : i32
    %15 = llvm.call @fpga_tile_submit(%5, %c0_i32_18, %c1_i32, %c2_i32_19, %c8_i32_20, %c1_i32_21, %c8_i32_22, %12, %14, %4, %c12_i32, %c0_i32_23, %c42_i32, %c110_i32) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %16 = bufferization.to_tensor %2 restrict writable : memref<12x12xf32> to tensor<12x12xf32>
    %c12_i32_24 = arith.constant 12 : i32
    %intptr_25 = memref.extract_aligned_pointer_as_index %2 : memref<12x12xf32> -> index
    %c32 = arith.constant 32 : index
    %17 = arith.addi %intptr_25, %c32 : index
    %18 = arith.index_cast %17 : index to i64
    %19 = llvm.inttoptr %18 : i64 to !llvm.ptr
    %subview_26 = memref.subview %1[0, 0] [4, 16] [1, 1] : memref<12x17xf32> to memref<4x16xf32, strided<[17, 1]>>
    %reinterpret_cast_27 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_26, %reinterpret_cast_27 : memref<4x16xf32, strided<[17, 1]>> to memref<4x16xf32>
    %intptr_28 = memref.extract_aligned_pointer_as_index %reinterpret_cast_27 : memref<4x16xf32> -> index
    %20 = arith.index_cast %intptr_28 : index to i64
    %21 = llvm.inttoptr %20 : i64 to !llvm.ptr
    %subview_29 = memref.subview %0[0, 8] [16, 4] [1, 1] : memref<17x12xf32> to memref<16x4xf32, strided<[12, 1], offset: 8>>
    %reinterpret_cast_30 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_29, %reinterpret_cast_30 : memref<16x4xf32, strided<[12, 1], offset: 8>> to memref<16x4xf32>
    %intptr_31 = memref.extract_aligned_pointer_as_index %reinterpret_cast_30 : memref<16x4xf32> -> index
    %22 = arith.index_cast %intptr_31 : index to i64
    %23 = llvm.inttoptr %22 : i64 to !llvm.ptr
    %c1_i32_32 = arith.constant 1 : i32
    %c0_i32_33 = arith.constant 0 : i32
    %c2_i32_34 = arith.constant 2 : i32
    %c4_i32 = arith.constant 4 : i32
    %c16_i32_35 = arith.constant 16 : i32
    %c4_i32_36 = arith.constant 4 : i32
    %c1_i32_37 = arith.constant 1 : i32
    %c0_i32_38 = arith.constant 0 : i32
    %c117_i32 = arith.constant 117 : i32
    %24 = llvm.call @fpga_tile_submit(%5, %c1_i32_32, %c0_i32_33, %c2_i32_34, %c4_i32, %c16_i32_35, %c4_i32_36, %21, %23, %19, %c12_i32_24, %c1_i32_37, %c0_i32_38, %c117_i32) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %subview_39 = memref.subview %1[0, 16] [4, 1] [1, 1] : memref<12x17xf32> to memref<4x1xf32, strided<[17, 1], offset: 16>>
    %reinterpret_cast_40 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 1], strides: [1, 1] : memref<128xf32> to memref<4x1xf32>
    memref.copy %subview_39, %reinterpret_cast_40 : memref<4x1xf32, strided<[17, 1], offset: 16>> to memref<4x1xf32>
    %intptr_41 = memref.extract_aligned_pointer_as_index %reinterpret_cast_40 : memref<4x1xf32> -> index
    %25 = arith.index_cast %intptr_41 : index to i64
    %26 = llvm.inttoptr %25 : i64 to !llvm.ptr
    %subview_42 = memref.subview %0[16, 8] [1, 4] [1, 1] : memref<17x12xf32> to memref<1x4xf32, strided<[12, 1], offset: 200>>
    %reinterpret_cast_43 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [1, 4], strides: [4, 1] : memref<128xf32> to memref<1x4xf32>
    memref.copy %subview_42, %reinterpret_cast_43 : memref<1x4xf32, strided<[12, 1], offset: 200>> to memref<1x4xf32>
    %intptr_44 = memref.extract_aligned_pointer_as_index %reinterpret_cast_43 : memref<1x4xf32> -> index
    %27 = arith.index_cast %intptr_44 : index to i64
    %28 = llvm.inttoptr %27 : i64 to !llvm.ptr
    %c1_i32_45 = arith.constant 1 : i32
    %c1_i32_46 = arith.constant 1 : i32
    %c2_i32_47 = arith.constant 2 : i32
    %c4_i32_48 = arith.constant 4 : i32
    %c1_i32_49 = arith.constant 1 : i32
    %c4_i32_50 = arith.constant 4 : i32
    %c1_i32_51 = arith.constant 1 : i32
    %c42_i32_52 = arith.constant 42 : i32
    %c102_i32 = arith.constant 102 : i32
    %29 = llvm.call @fpga_tile_submit(%5, %c1_i32_45, %c1_i32_46, %c2_i32_47, %c4_i32_48, %c1_i32_49, %c4_i32_50, %26, %28, %19, %c12_i32_24, %c1_i32_51, %c42_i32_52, %c102_i32) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %30 = bufferization.to_tensor %2 restrict writable : memref<12x12xf32> to tensor<12x12xf32>
    %c12_i32_53 = arith.constant 12 : i32
    %intptr_54 = memref.extract_aligned_pointer_as_index %2 : memref<12x12xf32> -> index
    %c224 = arith.constant 224 : index
    %31 = arith.addi %intptr_54, %c224 : index
    %32 = arith.index_cast %31 : index to i64
    %33 = llvm.inttoptr %32 : i64 to !llvm.ptr
    %subview_55 = memref.subview %1[4, 0] [4, 16] [1, 1] : memref<12x17xf32> to memref<4x16xf32, strided<[17, 1], offset: 68>>
    %reinterpret_cast_56 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_55, %reinterpret_cast_56 : memref<4x16xf32, strided<[17, 1], offset: 68>> to memref<4x16xf32>
    %intptr_57 = memref.extract_aligned_pointer_as_index %reinterpret_cast_56 : memref<4x16xf32> -> index
    %34 = arith.index_cast %intptr_57 : index to i64
    %35 = llvm.inttoptr %34 : i64 to !llvm.ptr
    %subview_58 = memref.subview %0[0, 8] [16, 4] [1, 1] : memref<17x12xf32> to memref<16x4xf32, strided<[12, 1], offset: 8>>
    %reinterpret_cast_59 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_58, %reinterpret_cast_59 : memref<16x4xf32, strided<[12, 1], offset: 8>> to memref<16x4xf32>
    %intptr_60 = memref.extract_aligned_pointer_as_index %reinterpret_cast_59 : memref<16x4xf32> -> index
    %36 = arith.index_cast %intptr_60 : index to i64
    %37 = llvm.inttoptr %36 : i64 to !llvm.ptr
    %c2_i32_61 = arith.constant 2 : i32
    %c0_i32_62 = arith.constant 0 : i32
    %c2_i32_63 = arith.constant 2 : i32
    %c4_i32_64 = arith.constant 4 : i32
    %c16_i32_65 = arith.constant 16 : i32
    %c4_i32_66 = arith.constant 4 : i32
    %c2_i32_67 = arith.constant 2 : i32
    %c0_i32_68 = arith.constant 0 : i32
    %c117_i32_69 = arith.constant 117 : i32
    %38 = llvm.call @fpga_tile_submit(%5, %c2_i32_61, %c0_i32_62, %c2_i32_63, %c4_i32_64, %c16_i32_65, %c4_i32_66, %35, %37, %33, %c12_i32_53, %c2_i32_67, %c0_i32_68, %c117_i32_69) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %subview_70 = memref.subview %1[4, 16] [4, 1] [1, 1] : memref<12x17xf32> to memref<4x1xf32, strided<[17, 1], offset: 84>>
    %reinterpret_cast_71 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 1], strides: [1, 1] : memref<128xf32> to memref<4x1xf32>
    memref.copy %subview_70, %reinterpret_cast_71 : memref<4x1xf32, strided<[17, 1], offset: 84>> to memref<4x1xf32>
    %intptr_72 = memref.extract_aligned_pointer_as_index %reinterpret_cast_71 : memref<4x1xf32> -> index
    %39 = arith.index_cast %intptr_72 : index to i64
    %40 = llvm.inttoptr %39 : i64 to !llvm.ptr
    %subview_73 = memref.subview %0[16, 8] [1, 4] [1, 1] : memref<17x12xf32> to memref<1x4xf32, strided<[12, 1], offset: 200>>
    %reinterpret_cast_74 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [1, 4], strides: [4, 1] : memref<128xf32> to memref<1x4xf32>
    memref.copy %subview_73, %reinterpret_cast_74 : memref<1x4xf32, strided<[12, 1], offset: 200>> to memref<1x4xf32>
    %intptr_75 = memref.extract_aligned_pointer_as_index %reinterpret_cast_74 : memref<1x4xf32> -> index
    %41 = arith.index_cast %intptr_75 : index to i64
    %42 = llvm.inttoptr %41 : i64 to !llvm.ptr
    %c2_i32_76 = arith.constant 2 : i32
    %c1_i32_77 = arith.constant 1 : i32
    %c2_i32_78 = arith.constant 2 : i32
    %c4_i32_79 = arith.constant 4 : i32
    %c1_i32_80 = arith.constant 1 : i32
    %c4_i32_81 = arith.constant 4 : i32
    %c2_i32_82 = arith.constant 2 : i32
    %c42_i32_83 = arith.constant 42 : i32
    %c102_i32_84 = arith.constant 102 : i32
    %43 = llvm.call @fpga_tile_submit(%5, %c2_i32_76, %c1_i32_77, %c2_i32_78, %c4_i32_79, %c1_i32_80, %c4_i32_81, %40, %42, %33, %c12_i32_53, %c2_i32_82, %c42_i32_83, %c102_i32_84) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %44 = bufferization.to_tensor %2 restrict writable : memref<12x12xf32> to tensor<12x12xf32>
    %c12_i32_85 = arith.constant 12 : i32
    %intptr_86 = memref.extract_aligned_pointer_as_index %2 : memref<12x12xf32> -> index
    %c384 = arith.constant 384 : index
    %45 = arith.addi %intptr_86, %c384 : index
    %46 = arith.index_cast %45 : index to i64
    %47 = llvm.inttoptr %46 : i64 to !llvm.ptr
    %subview_87 = memref.subview %1[8, 0] [4, 16] [1, 1] : memref<12x17xf32> to memref<4x16xf32, strided<[17, 1], offset: 136>>
    %reinterpret_cast_88 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_87, %reinterpret_cast_88 : memref<4x16xf32, strided<[17, 1], offset: 136>> to memref<4x16xf32>
    %intptr_89 = memref.extract_aligned_pointer_as_index %reinterpret_cast_88 : memref<4x16xf32> -> index
    %48 = arith.index_cast %intptr_89 : index to i64
    %49 = llvm.inttoptr %48 : i64 to !llvm.ptr
    %subview_90 = memref.subview %0[0, 0] [16, 4] [1, 1] : memref<17x12xf32> to memref<16x4xf32, strided<[12, 1]>>
    %reinterpret_cast_91 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_90, %reinterpret_cast_91 : memref<16x4xf32, strided<[12, 1]>> to memref<16x4xf32>
    %intptr_92 = memref.extract_aligned_pointer_as_index %reinterpret_cast_91 : memref<16x4xf32> -> index
    %50 = arith.index_cast %intptr_92 : index to i64
    %51 = llvm.inttoptr %50 : i64 to !llvm.ptr
    %c3_i32 = arith.constant 3 : i32
    %c0_i32_93 = arith.constant 0 : i32
    %c2_i32_94 = arith.constant 2 : i32
    %c4_i32_95 = arith.constant 4 : i32
    %c16_i32_96 = arith.constant 16 : i32
    %c4_i32_97 = arith.constant 4 : i32
    %c3_i32_98 = arith.constant 3 : i32
    %c0_i32_99 = arith.constant 0 : i32
    %c117_i32_100 = arith.constant 117 : i32
    %52 = llvm.call @fpga_tile_submit(%5, %c3_i32, %c0_i32_93, %c2_i32_94, %c4_i32_95, %c16_i32_96, %c4_i32_97, %49, %51, %47, %c12_i32_85, %c3_i32_98, %c0_i32_99, %c117_i32_100) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %subview_101 = memref.subview %1[8, 16] [4, 1] [1, 1] : memref<12x17xf32> to memref<4x1xf32, strided<[17, 1], offset: 152>>
    %reinterpret_cast_102 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 1], strides: [1, 1] : memref<128xf32> to memref<4x1xf32>
    memref.copy %subview_101, %reinterpret_cast_102 : memref<4x1xf32, strided<[17, 1], offset: 152>> to memref<4x1xf32>
    %intptr_103 = memref.extract_aligned_pointer_as_index %reinterpret_cast_102 : memref<4x1xf32> -> index
    %53 = arith.index_cast %intptr_103 : index to i64
    %54 = llvm.inttoptr %53 : i64 to !llvm.ptr
    %subview_104 = memref.subview %0[16, 0] [1, 4] [1, 1] : memref<17x12xf32> to memref<1x4xf32, strided<[12, 1], offset: 192>>
    %reinterpret_cast_105 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [1, 4], strides: [4, 1] : memref<128xf32> to memref<1x4xf32>
    memref.copy %subview_104, %reinterpret_cast_105 : memref<1x4xf32, strided<[12, 1], offset: 192>> to memref<1x4xf32>
    %intptr_106 = memref.extract_aligned_pointer_as_index %reinterpret_cast_105 : memref<1x4xf32> -> index
    %55 = arith.index_cast %intptr_106 : index to i64
    %56 = llvm.inttoptr %55 : i64 to !llvm.ptr
    %c3_i32_107 = arith.constant 3 : i32
    %c1_i32_108 = arith.constant 1 : i32
    %c2_i32_109 = arith.constant 2 : i32
    %c4_i32_110 = arith.constant 4 : i32
    %c1_i32_111 = arith.constant 1 : i32
    %c4_i32_112 = arith.constant 4 : i32
    %c3_i32_113 = arith.constant 3 : i32
    %c42_i32_114 = arith.constant 42 : i32
    %c102_i32_115 = arith.constant 102 : i32
    %57 = llvm.call @fpga_tile_submit(%5, %c3_i32_107, %c1_i32_108, %c2_i32_109, %c4_i32_110, %c1_i32_111, %c4_i32_112, %54, %56, %47, %c12_i32_85, %c3_i32_113, %c42_i32_114, %c102_i32_115) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %58 = bufferization.to_tensor %2 restrict writable : memref<12x12xf32> to tensor<12x12xf32>
    %c12_i32_116 = arith.constant 12 : i32
    %intptr_117 = memref.extract_aligned_pointer_as_index %2 : memref<12x12xf32> -> index
    %c400 = arith.constant 400 : index
    %59 = arith.addi %intptr_117, %c400 : index
    %60 = arith.index_cast %59 : index to i64
    %61 = llvm.inttoptr %60 : i64 to !llvm.ptr
    %subview_118 = memref.subview %1[8, 0] [4, 16] [1, 1] : memref<12x17xf32> to memref<4x16xf32, strided<[17, 1], offset: 136>>
    %reinterpret_cast_119 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_118, %reinterpret_cast_119 : memref<4x16xf32, strided<[17, 1], offset: 136>> to memref<4x16xf32>
    %intptr_120 = memref.extract_aligned_pointer_as_index %reinterpret_cast_119 : memref<4x16xf32> -> index
    %62 = arith.index_cast %intptr_120 : index to i64
    %63 = llvm.inttoptr %62 : i64 to !llvm.ptr
    %subview_121 = memref.subview %0[0, 4] [16, 4] [1, 1] : memref<17x12xf32> to memref<16x4xf32, strided<[12, 1], offset: 4>>
    %reinterpret_cast_122 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_121, %reinterpret_cast_122 : memref<16x4xf32, strided<[12, 1], offset: 4>> to memref<16x4xf32>
    %intptr_123 = memref.extract_aligned_pointer_as_index %reinterpret_cast_122 : memref<16x4xf32> -> index
    %64 = arith.index_cast %intptr_123 : index to i64
    %65 = llvm.inttoptr %64 : i64 to !llvm.ptr
    %c4_i32_124 = arith.constant 4 : i32
    %c0_i32_125 = arith.constant 0 : i32
    %c2_i32_126 = arith.constant 2 : i32
    %c4_i32_127 = arith.constant 4 : i32
    %c16_i32_128 = arith.constant 16 : i32
    %c4_i32_129 = arith.constant 4 : i32
    %c1_i32_130 = arith.constant 1 : i32
    %c120_i32 = arith.constant 120 : i32
    %c117_i32_131 = arith.constant 117 : i32
    %66 = llvm.call @fpga_tile_submit(%5, %c4_i32_124, %c0_i32_125, %c2_i32_126, %c4_i32_127, %c16_i32_128, %c4_i32_129, %63, %65, %61, %c12_i32_116, %c1_i32_130, %c120_i32, %c117_i32_131) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %subview_132 = memref.subview %1[8, 16] [4, 1] [1, 1] : memref<12x17xf32> to memref<4x1xf32, strided<[17, 1], offset: 152>>
    %reinterpret_cast_133 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 1], strides: [1, 1] : memref<128xf32> to memref<4x1xf32>
    memref.copy %subview_132, %reinterpret_cast_133 : memref<4x1xf32, strided<[17, 1], offset: 152>> to memref<4x1xf32>
    %intptr_134 = memref.extract_aligned_pointer_as_index %reinterpret_cast_133 : memref<4x1xf32> -> index
    %67 = arith.index_cast %intptr_134 : index to i64
    %68 = llvm.inttoptr %67 : i64 to !llvm.ptr
    %subview_135 = memref.subview %0[16, 4] [1, 4] [1, 1] : memref<17x12xf32> to memref<1x4xf32, strided<[12, 1], offset: 196>>
    %reinterpret_cast_136 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [1, 4], strides: [4, 1] : memref<128xf32> to memref<1x4xf32>
    memref.copy %subview_135, %reinterpret_cast_136 : memref<1x4xf32, strided<[12, 1], offset: 196>> to memref<1x4xf32>
    %intptr_137 = memref.extract_aligned_pointer_as_index %reinterpret_cast_136 : memref<1x4xf32> -> index
    %69 = arith.index_cast %intptr_137 : index to i64
    %70 = llvm.inttoptr %69 : i64 to !llvm.ptr
    %c4_i32_138 = arith.constant 4 : i32
    %c1_i32_139 = arith.constant 1 : i32
    %c2_i32_140 = arith.constant 2 : i32
    %c4_i32_141 = arith.constant 4 : i32
    %c1_i32_142 = arith.constant 1 : i32
    %c4_i32_143 = arith.constant 4 : i32
    %c1_i32_144 = arith.constant 1 : i32
    %c189_i32 = arith.constant 189 : i32
    %c102_i32_145 = arith.constant 102 : i32
    %71 = llvm.call @fpga_tile_submit(%5, %c4_i32_138, %c1_i32_139, %c2_i32_140, %c4_i32_141, %c1_i32_142, %c4_i32_143, %68, %70, %61, %c12_i32_116, %c1_i32_144, %c189_i32, %c102_i32_145) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %72 = bufferization.to_tensor %2 restrict writable : memref<12x12xf32> to tensor<12x12xf32>
    %c12_i32_146 = arith.constant 12 : i32
    %intptr_147 = memref.extract_aligned_pointer_as_index %2 : memref<12x12xf32> -> index
    %c416 = arith.constant 416 : index
    %73 = arith.addi %intptr_147, %c416 : index
    %74 = arith.index_cast %73 : index to i64
    %75 = llvm.inttoptr %74 : i64 to !llvm.ptr
    %subview_148 = memref.subview %1[8, 0] [4, 16] [1, 1] : memref<12x17xf32> to memref<4x16xf32, strided<[17, 1], offset: 136>>
    %reinterpret_cast_149 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_148, %reinterpret_cast_149 : memref<4x16xf32, strided<[17, 1], offset: 136>> to memref<4x16xf32>
    %intptr_150 = memref.extract_aligned_pointer_as_index %reinterpret_cast_149 : memref<4x16xf32> -> index
    %76 = arith.index_cast %intptr_150 : index to i64
    %77 = llvm.inttoptr %76 : i64 to !llvm.ptr
    %subview_151 = memref.subview %0[0, 8] [16, 4] [1, 1] : memref<17x12xf32> to memref<16x4xf32, strided<[12, 1], offset: 8>>
    %reinterpret_cast_152 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_151, %reinterpret_cast_152 : memref<16x4xf32, strided<[12, 1], offset: 8>> to memref<16x4xf32>
    %intptr_153 = memref.extract_aligned_pointer_as_index %reinterpret_cast_152 : memref<16x4xf32> -> index
    %78 = arith.index_cast %intptr_153 : index to i64
    %79 = llvm.inttoptr %78 : i64 to !llvm.ptr
    %c5_i32 = arith.constant 5 : i32
    %c0_i32_154 = arith.constant 0 : i32
    %c2_i32_155 = arith.constant 2 : i32
    %c4_i32_156 = arith.constant 4 : i32
    %c16_i32_157 = arith.constant 16 : i32
    %c4_i32_158 = arith.constant 4 : i32
    %c2_i32_159 = arith.constant 2 : i32
    %c120_i32_160 = arith.constant 120 : i32
    %c117_i32_161 = arith.constant 117 : i32
    %80 = llvm.call @fpga_tile_submit(%5, %c5_i32, %c0_i32_154, %c2_i32_155, %c4_i32_156, %c16_i32_157, %c4_i32_158, %77, %79, %75, %c12_i32_146, %c2_i32_159, %c120_i32_160, %c117_i32_161) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %subview_162 = memref.subview %1[8, 16] [4, 1] [1, 1] : memref<12x17xf32> to memref<4x1xf32, strided<[17, 1], offset: 152>>
    %reinterpret_cast_163 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 1], strides: [1, 1] : memref<128xf32> to memref<4x1xf32>
    memref.copy %subview_162, %reinterpret_cast_163 : memref<4x1xf32, strided<[17, 1], offset: 152>> to memref<4x1xf32>
    %intptr_164 = memref.extract_aligned_pointer_as_index %reinterpret_cast_163 : memref<4x1xf32> -> index
    %81 = arith.index_cast %intptr_164 : index to i64
    %82 = llvm.inttoptr %81 : i64 to !llvm.ptr
    %subview_165 = memref.subview %0[16, 8] [1, 4] [1, 1] : memref<17x12xf32> to memref<1x4xf32, strided<[12, 1], offset: 200>>
    %reinterpret_cast_166 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [1, 4], strides: [4, 1] : memref<128xf32> to memref<1x4xf32>
    memref.copy %subview_165, %reinterpret_cast_166 : memref<1x4xf32, strided<[12, 1], offset: 200>> to memref<1x4xf32>
    %intptr_167 = memref.extract_aligned_pointer_as_index %reinterpret_cast_166 : memref<1x4xf32> -> index
    %83 = arith.index_cast %intptr_167 : index to i64
    %84 = llvm.inttoptr %83 : i64 to !llvm.ptr
    %c5_i32_168 = arith.constant 5 : i32
    %c1_i32_169 = arith.constant 1 : i32
    %c2_i32_170 = arith.constant 2 : i32
    %c4_i32_171 = arith.constant 4 : i32
    %c1_i32_172 = arith.constant 1 : i32
    %c4_i32_173 = arith.constant 4 : i32
    %c2_i32_174 = arith.constant 2 : i32
    %c189_i32_175 = arith.constant 189 : i32
    %c102_i32_176 = arith.constant 102 : i32
    %85 = llvm.call @fpga_tile_submit(%5, %c5_i32_168, %c1_i32_169, %c2_i32_170, %c4_i32_171, %c1_i32_172, %c4_i32_173, %82, %84, %75, %c12_i32_146, %c2_i32_174, %c189_i32_175, %c102_i32_176) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %86 = bufferization.to_tensor %2 restrict writable : memref<12x12xf32> to tensor<12x12xf32>
    %87 = llvm.call @fpga_gemm_wait(%5) : (!llvm.ptr) -> i32
    return %86 : tensor<12x12xf32>
  }
}

