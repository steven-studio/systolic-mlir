module {
  llvm.func @fpga_gemm_wait(!llvm.ptr) -> i32
  llvm.func @fpga_tile_submit(!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
  llvm.func @fpga_gemm_begin(i32, i32, i32, i32) -> !llvm.ptr
  func.func @systolic_matmul(%arg0: tensor<15x16xf32>, %arg1: tensor<16x15xf32>, %arg2: tensor<15x15xf32>) -> tensor<15x15xf32> attributes {llvm.emit_c_interface} {
    %0 = bufferization.to_buffer %arg1 read_only : tensor<16x15xf32> to memref<16x15xf32>
    %1 = bufferization.to_buffer %arg0 read_only : tensor<15x16xf32> to memref<15x16xf32>
    %alloca = memref.alloca() : memref<128xf32>
    %alloca_0 = memref.alloca() : memref<128xf32>
    %2 = bufferization.to_buffer %arg2 : tensor<15x15xf32> to memref<15x15xf32>
    %c15_i32 = arith.constant 15 : i32
    %intptr = memref.extract_aligned_pointer_as_index %2 : memref<15x15xf32> -> index
    %3 = arith.index_cast %intptr : index to i64
    %4 = llvm.inttoptr %3 : i64 to !llvm.ptr
    %c15_i32_1 = arith.constant 15 : i32
    %c15_i32_2 = arith.constant 15 : i32
    %c16_i32 = arith.constant 16 : i32
    %c7_i32 = arith.constant 7 : i32
    %5 = llvm.call @fpga_gemm_begin(%c15_i32_1, %c15_i32_2, %c16_i32, %c7_i32) : (i32, i32, i32, i32) -> !llvm.ptr
    %subview = memref.subview %1[0, 0] [8, 16] [1, 1] : memref<15x16xf32> to memref<8x16xf32, strided<[16, 1]>>
    %reinterpret_cast = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 16], strides: [16, 1] : memref<128xf32> to memref<8x16xf32>
    memref.copy %subview, %reinterpret_cast : memref<8x16xf32, strided<[16, 1]>> to memref<8x16xf32>
    %intptr_3 = memref.extract_aligned_pointer_as_index %reinterpret_cast : memref<8x16xf32> -> index
    %6 = arith.index_cast %intptr_3 : index to i64
    %7 = llvm.inttoptr %6 : i64 to !llvm.ptr
    %subview_4 = memref.subview %0[0, 0] [16, 8] [1, 1] : memref<16x15xf32> to memref<16x8xf32, strided<[15, 1]>>
    %reinterpret_cast_5 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 8], strides: [8, 1] : memref<128xf32> to memref<16x8xf32>
    memref.copy %subview_4, %reinterpret_cast_5 : memref<16x8xf32, strided<[15, 1]>> to memref<16x8xf32>
    %intptr_6 = memref.extract_aligned_pointer_as_index %reinterpret_cast_5 : memref<16x8xf32> -> index
    %8 = arith.index_cast %intptr_6 : index to i64
    %9 = llvm.inttoptr %8 : i64 to !llvm.ptr
    %c0_i32 = arith.constant 0 : i32
    %c0_i32_7 = arith.constant 0 : i32
    %c1_i32 = arith.constant 1 : i32
    %c8_i32 = arith.constant 8 : i32
    %c16_i32_8 = arith.constant 16 : i32
    %c8_i32_9 = arith.constant 8 : i32
    %c0_i32_10 = arith.constant 0 : i32
    %c0_i32_11 = arith.constant 0 : i32
    %c125_i32 = arith.constant 125 : i32
    %10 = llvm.call @fpga_tile_submit(%5, %c0_i32, %c0_i32_7, %c1_i32, %c8_i32, %c16_i32_8, %c8_i32_9, %7, %9, %4, %c15_i32, %c0_i32_10, %c0_i32_11, %c125_i32) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %11 = bufferization.to_tensor %2 restrict writable : memref<15x15xf32> to tensor<15x15xf32>
    %c15_i32_12 = arith.constant 15 : i32
    %intptr_13 = memref.extract_aligned_pointer_as_index %2 : memref<15x15xf32> -> index
    %c32 = arith.constant 32 : index
    %12 = arith.addi %intptr_13, %c32 : index
    %13 = arith.index_cast %12 : index to i64
    %14 = llvm.inttoptr %13 : i64 to !llvm.ptr
    %subview_14 = memref.subview %1[0, 0] [8, 16] [1, 1] : memref<15x16xf32> to memref<8x16xf32, strided<[16, 1]>>
    %reinterpret_cast_15 = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 16], strides: [16, 1] : memref<128xf32> to memref<8x16xf32>
    memref.copy %subview_14, %reinterpret_cast_15 : memref<8x16xf32, strided<[16, 1]>> to memref<8x16xf32>
    %intptr_16 = memref.extract_aligned_pointer_as_index %reinterpret_cast_15 : memref<8x16xf32> -> index
    %15 = arith.index_cast %intptr_16 : index to i64
    %16 = llvm.inttoptr %15 : i64 to !llvm.ptr
    %subview_17 = memref.subview %0[0, 8] [16, 7] [1, 1] : memref<16x15xf32> to memref<16x7xf32, strided<[15, 1], offset: 8>>
    %reinterpret_cast_18 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 7], strides: [7, 1] : memref<128xf32> to memref<16x7xf32>
    memref.copy %subview_17, %reinterpret_cast_18 : memref<16x7xf32, strided<[15, 1], offset: 8>> to memref<16x7xf32>
    %intptr_19 = memref.extract_aligned_pointer_as_index %reinterpret_cast_18 : memref<16x7xf32> -> index
    %17 = arith.index_cast %intptr_19 : index to i64
    %18 = llvm.inttoptr %17 : i64 to !llvm.ptr
    %c1_i32_20 = arith.constant 1 : i32
    %c0_i32_21 = arith.constant 0 : i32
    %c1_i32_22 = arith.constant 1 : i32
    %c8_i32_23 = arith.constant 8 : i32
    %c16_i32_24 = arith.constant 16 : i32
    %c7_i32_25 = arith.constant 7 : i32
    %c0_i32_26 = arith.constant 0 : i32
    %c42_i32 = arith.constant 42 : i32
    %c125_i32_27 = arith.constant 125 : i32
    %19 = llvm.call @fpga_tile_submit(%5, %c1_i32_20, %c0_i32_21, %c1_i32_22, %c8_i32_23, %c16_i32_24, %c7_i32_25, %16, %18, %14, %c15_i32_12, %c0_i32_26, %c42_i32, %c125_i32_27) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %20 = bufferization.to_tensor %2 restrict writable : memref<15x15xf32> to tensor<15x15xf32>
    %c15_i32_28 = arith.constant 15 : i32
    %intptr_29 = memref.extract_aligned_pointer_as_index %2 : memref<15x15xf32> -> index
    %c480 = arith.constant 480 : index
    %21 = arith.addi %intptr_29, %c480 : index
    %22 = arith.index_cast %21 : index to i64
    %23 = llvm.inttoptr %22 : i64 to !llvm.ptr
    %subview_30 = memref.subview %1[8, 0] [7, 16] [1, 1] : memref<15x16xf32> to memref<7x16xf32, strided<[16, 1], offset: 128>>
    %reinterpret_cast_31 = memref.reinterpret_cast %alloca to offset: [0], sizes: [7, 16], strides: [16, 1] : memref<128xf32> to memref<7x16xf32>
    memref.copy %subview_30, %reinterpret_cast_31 : memref<7x16xf32, strided<[16, 1], offset: 128>> to memref<7x16xf32>
    %intptr_32 = memref.extract_aligned_pointer_as_index %reinterpret_cast_31 : memref<7x16xf32> -> index
    %24 = arith.index_cast %intptr_32 : index to i64
    %25 = llvm.inttoptr %24 : i64 to !llvm.ptr
    %subview_33 = memref.subview %0[0, 0] [16, 8] [1, 1] : memref<16x15xf32> to memref<16x8xf32, strided<[15, 1]>>
    %reinterpret_cast_34 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 8], strides: [8, 1] : memref<128xf32> to memref<16x8xf32>
    memref.copy %subview_33, %reinterpret_cast_34 : memref<16x8xf32, strided<[15, 1]>> to memref<16x8xf32>
    %intptr_35 = memref.extract_aligned_pointer_as_index %reinterpret_cast_34 : memref<16x8xf32> -> index
    %26 = arith.index_cast %intptr_35 : index to i64
    %27 = llvm.inttoptr %26 : i64 to !llvm.ptr
    %c2_i32 = arith.constant 2 : i32
    %c0_i32_36 = arith.constant 0 : i32
    %c1_i32_37 = arith.constant 1 : i32
    %c7_i32_38 = arith.constant 7 : i32
    %c16_i32_39 = arith.constant 16 : i32
    %c8_i32_40 = arith.constant 8 : i32
    %c0_i32_41 = arith.constant 0 : i32
    %c128_i32 = arith.constant 128 : i32
    %c125_i32_42 = arith.constant 125 : i32
    %28 = llvm.call @fpga_tile_submit(%5, %c2_i32, %c0_i32_36, %c1_i32_37, %c7_i32_38, %c16_i32_39, %c8_i32_40, %25, %27, %23, %c15_i32_28, %c0_i32_41, %c128_i32, %c125_i32_42) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %29 = bufferization.to_tensor %2 restrict writable : memref<15x15xf32> to tensor<15x15xf32>
    %c15_i32_43 = arith.constant 15 : i32
    %intptr_44 = memref.extract_aligned_pointer_as_index %2 : memref<15x15xf32> -> index
    %c512 = arith.constant 512 : index
    %30 = arith.addi %intptr_44, %c512 : index
    %31 = arith.index_cast %30 : index to i64
    %32 = llvm.inttoptr %31 : i64 to !llvm.ptr
    %subview_45 = memref.subview %1[8, 0] [4, 16] [1, 1] : memref<15x16xf32> to memref<4x16xf32, strided<[16, 1], offset: 128>>
    %reinterpret_cast_46 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_45, %reinterpret_cast_46 : memref<4x16xf32, strided<[16, 1], offset: 128>> to memref<4x16xf32>
    %intptr_47 = memref.extract_aligned_pointer_as_index %reinterpret_cast_46 : memref<4x16xf32> -> index
    %33 = arith.index_cast %intptr_47 : index to i64
    %34 = llvm.inttoptr %33 : i64 to !llvm.ptr
    %subview_48 = memref.subview %0[0, 8] [16, 4] [1, 1] : memref<16x15xf32> to memref<16x4xf32, strided<[15, 1], offset: 8>>
    %reinterpret_cast_49 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_48, %reinterpret_cast_49 : memref<16x4xf32, strided<[15, 1], offset: 8>> to memref<16x4xf32>
    %intptr_50 = memref.extract_aligned_pointer_as_index %reinterpret_cast_49 : memref<16x4xf32> -> index
    %35 = arith.index_cast %intptr_50 : index to i64
    %36 = llvm.inttoptr %35 : i64 to !llvm.ptr
    %c3_i32 = arith.constant 3 : i32
    %c0_i32_51 = arith.constant 0 : i32
    %c1_i32_52 = arith.constant 1 : i32
    %c4_i32 = arith.constant 4 : i32
    %c16_i32_53 = arith.constant 16 : i32
    %c4_i32_54 = arith.constant 4 : i32
    %c1_i32_55 = arith.constant 1 : i32
    %c0_i32_56 = arith.constant 0 : i32
    %c117_i32 = arith.constant 117 : i32
    %37 = llvm.call @fpga_tile_submit(%5, %c3_i32, %c0_i32_51, %c1_i32_52, %c4_i32, %c16_i32_53, %c4_i32_54, %34, %36, %32, %c15_i32_43, %c1_i32_55, %c0_i32_56, %c117_i32) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %38 = bufferization.to_tensor %2 restrict writable : memref<15x15xf32> to tensor<15x15xf32>
    %c15_i32_57 = arith.constant 15 : i32
    %intptr_58 = memref.extract_aligned_pointer_as_index %2 : memref<15x15xf32> -> index
    %c528 = arith.constant 528 : index
    %39 = arith.addi %intptr_58, %c528 : index
    %40 = arith.index_cast %39 : index to i64
    %41 = llvm.inttoptr %40 : i64 to !llvm.ptr
    %subview_59 = memref.subview %1[8, 0] [4, 16] [1, 1] : memref<15x16xf32> to memref<4x16xf32, strided<[16, 1], offset: 128>>
    %reinterpret_cast_60 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_59, %reinterpret_cast_60 : memref<4x16xf32, strided<[16, 1], offset: 128>> to memref<4x16xf32>
    %intptr_61 = memref.extract_aligned_pointer_as_index %reinterpret_cast_60 : memref<4x16xf32> -> index
    %42 = arith.index_cast %intptr_61 : index to i64
    %43 = llvm.inttoptr %42 : i64 to !llvm.ptr
    %subview_62 = memref.subview %0[0, 12] [16, 3] [1, 1] : memref<16x15xf32> to memref<16x3xf32, strided<[15, 1], offset: 12>>
    %reinterpret_cast_63 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 3], strides: [3, 1] : memref<128xf32> to memref<16x3xf32>
    memref.copy %subview_62, %reinterpret_cast_63 : memref<16x3xf32, strided<[15, 1], offset: 12>> to memref<16x3xf32>
    %intptr_64 = memref.extract_aligned_pointer_as_index %reinterpret_cast_63 : memref<16x3xf32> -> index
    %44 = arith.index_cast %intptr_64 : index to i64
    %45 = llvm.inttoptr %44 : i64 to !llvm.ptr
    %c4_i32_65 = arith.constant 4 : i32
    %c0_i32_66 = arith.constant 0 : i32
    %c1_i32_67 = arith.constant 1 : i32
    %c4_i32_68 = arith.constant 4 : i32
    %c16_i32_69 = arith.constant 16 : i32
    %c3_i32_70 = arith.constant 3 : i32
    %c2_i32_71 = arith.constant 2 : i32
    %c0_i32_72 = arith.constant 0 : i32
    %c117_i32_73 = arith.constant 117 : i32
    %46 = llvm.call @fpga_tile_submit(%5, %c4_i32_65, %c0_i32_66, %c1_i32_67, %c4_i32_68, %c16_i32_69, %c3_i32_70, %43, %45, %41, %c15_i32_57, %c2_i32_71, %c0_i32_72, %c117_i32_73) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %47 = bufferization.to_tensor %2 restrict writable : memref<15x15xf32> to tensor<15x15xf32>
    %c15_i32_74 = arith.constant 15 : i32
    %intptr_75 = memref.extract_aligned_pointer_as_index %2 : memref<15x15xf32> -> index
    %c752 = arith.constant 752 : index
    %48 = arith.addi %intptr_75, %c752 : index
    %49 = arith.index_cast %48 : index to i64
    %50 = llvm.inttoptr %49 : i64 to !llvm.ptr
    %subview_76 = memref.subview %1[12, 0] [3, 16] [1, 1] : memref<15x16xf32> to memref<3x16xf32, strided<[16, 1], offset: 192>>
    %reinterpret_cast_77 = memref.reinterpret_cast %alloca to offset: [0], sizes: [3, 16], strides: [16, 1] : memref<128xf32> to memref<3x16xf32>
    memref.copy %subview_76, %reinterpret_cast_77 : memref<3x16xf32, strided<[16, 1], offset: 192>> to memref<3x16xf32>
    %intptr_78 = memref.extract_aligned_pointer_as_index %reinterpret_cast_77 : memref<3x16xf32> -> index
    %51 = arith.index_cast %intptr_78 : index to i64
    %52 = llvm.inttoptr %51 : i64 to !llvm.ptr
    %subview_79 = memref.subview %0[0, 8] [16, 4] [1, 1] : memref<16x15xf32> to memref<16x4xf32, strided<[15, 1], offset: 8>>
    %reinterpret_cast_80 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_79, %reinterpret_cast_80 : memref<16x4xf32, strided<[15, 1], offset: 8>> to memref<16x4xf32>
    %intptr_81 = memref.extract_aligned_pointer_as_index %reinterpret_cast_80 : memref<16x4xf32> -> index
    %53 = arith.index_cast %intptr_81 : index to i64
    %54 = llvm.inttoptr %53 : i64 to !llvm.ptr
    %c5_i32 = arith.constant 5 : i32
    %c0_i32_82 = arith.constant 0 : i32
    %c1_i32_83 = arith.constant 1 : i32
    %c3_i32_84 = arith.constant 3 : i32
    %c16_i32_85 = arith.constant 16 : i32
    %c4_i32_86 = arith.constant 4 : i32
    %c3_i32_87 = arith.constant 3 : i32
    %c0_i32_88 = arith.constant 0 : i32
    %c117_i32_89 = arith.constant 117 : i32
    %55 = llvm.call @fpga_tile_submit(%5, %c5_i32, %c0_i32_82, %c1_i32_83, %c3_i32_84, %c16_i32_85, %c4_i32_86, %52, %54, %50, %c15_i32_74, %c3_i32_87, %c0_i32_88, %c117_i32_89) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %56 = bufferization.to_tensor %2 restrict writable : memref<15x15xf32> to tensor<15x15xf32>
    %c15_i32_90 = arith.constant 15 : i32
    %intptr_91 = memref.extract_aligned_pointer_as_index %2 : memref<15x15xf32> -> index
    %c768 = arith.constant 768 : index
    %57 = arith.addi %intptr_91, %c768 : index
    %58 = arith.index_cast %57 : index to i64
    %59 = llvm.inttoptr %58 : i64 to !llvm.ptr
    %subview_92 = memref.subview %1[12, 0] [3, 16] [1, 1] : memref<15x16xf32> to memref<3x16xf32, strided<[16, 1], offset: 192>>
    %reinterpret_cast_93 = memref.reinterpret_cast %alloca to offset: [0], sizes: [3, 16], strides: [16, 1] : memref<128xf32> to memref<3x16xf32>
    memref.copy %subview_92, %reinterpret_cast_93 : memref<3x16xf32, strided<[16, 1], offset: 192>> to memref<3x16xf32>
    %intptr_94 = memref.extract_aligned_pointer_as_index %reinterpret_cast_93 : memref<3x16xf32> -> index
    %60 = arith.index_cast %intptr_94 : index to i64
    %61 = llvm.inttoptr %60 : i64 to !llvm.ptr
    %subview_95 = memref.subview %0[0, 12] [16, 3] [1, 1] : memref<16x15xf32> to memref<16x3xf32, strided<[15, 1], offset: 12>>
    %reinterpret_cast_96 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 3], strides: [3, 1] : memref<128xf32> to memref<16x3xf32>
    memref.copy %subview_95, %reinterpret_cast_96 : memref<16x3xf32, strided<[15, 1], offset: 12>> to memref<16x3xf32>
    %intptr_97 = memref.extract_aligned_pointer_as_index %reinterpret_cast_96 : memref<16x3xf32> -> index
    %62 = arith.index_cast %intptr_97 : index to i64
    %63 = llvm.inttoptr %62 : i64 to !llvm.ptr
    %c6_i32 = arith.constant 6 : i32
    %c0_i32_98 = arith.constant 0 : i32
    %c1_i32_99 = arith.constant 1 : i32
    %c3_i32_100 = arith.constant 3 : i32
    %c16_i32_101 = arith.constant 16 : i32
    %c3_i32_102 = arith.constant 3 : i32
    %c1_i32_103 = arith.constant 1 : i32
    %c42_i32_104 = arith.constant 42 : i32
    %c117_i32_105 = arith.constant 117 : i32
    %64 = llvm.call @fpga_tile_submit(%5, %c6_i32, %c0_i32_98, %c1_i32_99, %c3_i32_100, %c16_i32_101, %c3_i32_102, %61, %63, %59, %c15_i32_90, %c1_i32_103, %c42_i32_104, %c117_i32_105) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %65 = bufferization.to_tensor %2 restrict writable : memref<15x15xf32> to tensor<15x15xf32>
    %66 = llvm.call @fpga_gemm_wait(%5) : (!llvm.ptr) -> i32
    return %65 : tensor<15x15xf32>
  }
}

