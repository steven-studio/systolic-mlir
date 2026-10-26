module {
  llvm.func @fpga_gemm_wait(!llvm.ptr) -> i32
  llvm.func @fpga_tile_submit(!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
  llvm.func @fpga_gemm_begin(i32, i32, i32, i32) -> !llvm.ptr
  func.func @systolic_matmul(%arg0: tensor<12x16xf32>, %arg1: tensor<16x12xf32>, %arg2: tensor<12x12xf32>) -> tensor<12x12xf32> attributes {llvm.emit_c_interface} {
    %0 = bufferization.to_buffer %arg1 read_only : tensor<16x12xf32> to memref<16x12xf32>
    %1 = bufferization.to_buffer %arg0 read_only : tensor<12x16xf32> to memref<12x16xf32>
    %alloca = memref.alloca() : memref<128xf32>
    %alloca_0 = memref.alloca() : memref<128xf32>
    %2 = bufferization.to_buffer %arg2 : tensor<12x12xf32> to memref<12x12xf32>
    %c12_i32 = arith.constant 12 : i32
    %intptr = memref.extract_aligned_pointer_as_index %2 : memref<12x12xf32> -> index
    %3 = arith.index_cast %intptr : index to i64
    %4 = llvm.inttoptr %3 : i64 to !llvm.ptr
    %c12_i32_1 = arith.constant 12 : i32
    %c12_i32_2 = arith.constant 12 : i32
    %c16_i32 = arith.constant 16 : i32
    %c6_i32 = arith.constant 6 : i32
    %5 = llvm.call @fpga_gemm_begin(%c12_i32_1, %c12_i32_2, %c16_i32, %c6_i32) : (i32, i32, i32, i32) -> !llvm.ptr
    %subview = memref.subview %1[0, 0] [8, 16] [1, 1] : memref<12x16xf32> to memref<8x16xf32, strided<[16, 1]>>
    %reinterpret_cast = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 16], strides: [16, 1] : memref<128xf32> to memref<8x16xf32>
    memref.copy %subview, %reinterpret_cast : memref<8x16xf32, strided<[16, 1]>> to memref<8x16xf32>
    %intptr_3 = memref.extract_aligned_pointer_as_index %reinterpret_cast : memref<8x16xf32> -> index
    %6 = arith.index_cast %intptr_3 : index to i64
    %7 = llvm.inttoptr %6 : i64 to !llvm.ptr
    %subview_4 = memref.subview %0[0, 0] [16, 8] [1, 1] : memref<16x12xf32> to memref<16x8xf32, strided<[12, 1]>>
    %reinterpret_cast_5 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 8], strides: [8, 1] : memref<128xf32> to memref<16x8xf32>
    memref.copy %subview_4, %reinterpret_cast_5 : memref<16x8xf32, strided<[12, 1]>> to memref<16x8xf32>
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
    %10 = llvm.call @fpga_tile_submit(%5, %c0_i32, %c0_i32_7, %c1_i32, %c8_i32, %c16_i32_8, %c8_i32_9, %7, %9, %4, %c12_i32, %c0_i32_10, %c0_i32_11, %c125_i32) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %11 = bufferization.to_tensor %2 restrict writable : memref<12x12xf32> to tensor<12x12xf32>
    %c12_i32_12 = arith.constant 12 : i32
    %intptr_13 = memref.extract_aligned_pointer_as_index %2 : memref<12x12xf32> -> index
    %c32 = arith.constant 32 : index
    %12 = arith.addi %intptr_13, %c32 : index
    %13 = arith.index_cast %12 : index to i64
    %14 = llvm.inttoptr %13 : i64 to !llvm.ptr
    %subview_14 = memref.subview %1[0, 0] [4, 16] [1, 1] : memref<12x16xf32> to memref<4x16xf32, strided<[16, 1]>>
    %reinterpret_cast_15 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_14, %reinterpret_cast_15 : memref<4x16xf32, strided<[16, 1]>> to memref<4x16xf32>
    %intptr_16 = memref.extract_aligned_pointer_as_index %reinterpret_cast_15 : memref<4x16xf32> -> index
    %15 = arith.index_cast %intptr_16 : index to i64
    %16 = llvm.inttoptr %15 : i64 to !llvm.ptr
    %subview_17 = memref.subview %0[0, 8] [16, 4] [1, 1] : memref<16x12xf32> to memref<16x4xf32, strided<[12, 1], offset: 8>>
    %reinterpret_cast_18 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_17, %reinterpret_cast_18 : memref<16x4xf32, strided<[12, 1], offset: 8>> to memref<16x4xf32>
    %intptr_19 = memref.extract_aligned_pointer_as_index %reinterpret_cast_18 : memref<16x4xf32> -> index
    %17 = arith.index_cast %intptr_19 : index to i64
    %18 = llvm.inttoptr %17 : i64 to !llvm.ptr
    %c1_i32_20 = arith.constant 1 : i32
    %c0_i32_21 = arith.constant 0 : i32
    %c1_i32_22 = arith.constant 1 : i32
    %c4_i32 = arith.constant 4 : i32
    %c16_i32_23 = arith.constant 16 : i32
    %c4_i32_24 = arith.constant 4 : i32
    %c1_i32_25 = arith.constant 1 : i32
    %c0_i32_26 = arith.constant 0 : i32
    %c117_i32 = arith.constant 117 : i32
    %19 = llvm.call @fpga_tile_submit(%5, %c1_i32_20, %c0_i32_21, %c1_i32_22, %c4_i32, %c16_i32_23, %c4_i32_24, %16, %18, %14, %c12_i32_12, %c1_i32_25, %c0_i32_26, %c117_i32) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %20 = bufferization.to_tensor %2 restrict writable : memref<12x12xf32> to tensor<12x12xf32>
    %c12_i32_27 = arith.constant 12 : i32
    %intptr_28 = memref.extract_aligned_pointer_as_index %2 : memref<12x12xf32> -> index
    %c224 = arith.constant 224 : index
    %21 = arith.addi %intptr_28, %c224 : index
    %22 = arith.index_cast %21 : index to i64
    %23 = llvm.inttoptr %22 : i64 to !llvm.ptr
    %subview_29 = memref.subview %1[4, 0] [4, 16] [1, 1] : memref<12x16xf32> to memref<4x16xf32, strided<[16, 1], offset: 64>>
    %reinterpret_cast_30 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_29, %reinterpret_cast_30 : memref<4x16xf32, strided<[16, 1], offset: 64>> to memref<4x16xf32>
    %intptr_31 = memref.extract_aligned_pointer_as_index %reinterpret_cast_30 : memref<4x16xf32> -> index
    %24 = arith.index_cast %intptr_31 : index to i64
    %25 = llvm.inttoptr %24 : i64 to !llvm.ptr
    %subview_32 = memref.subview %0[0, 8] [16, 4] [1, 1] : memref<16x12xf32> to memref<16x4xf32, strided<[12, 1], offset: 8>>
    %reinterpret_cast_33 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_32, %reinterpret_cast_33 : memref<16x4xf32, strided<[12, 1], offset: 8>> to memref<16x4xf32>
    %intptr_34 = memref.extract_aligned_pointer_as_index %reinterpret_cast_33 : memref<16x4xf32> -> index
    %26 = arith.index_cast %intptr_34 : index to i64
    %27 = llvm.inttoptr %26 : i64 to !llvm.ptr
    %c2_i32 = arith.constant 2 : i32
    %c0_i32_35 = arith.constant 0 : i32
    %c1_i32_36 = arith.constant 1 : i32
    %c4_i32_37 = arith.constant 4 : i32
    %c16_i32_38 = arith.constant 16 : i32
    %c4_i32_39 = arith.constant 4 : i32
    %c2_i32_40 = arith.constant 2 : i32
    %c0_i32_41 = arith.constant 0 : i32
    %c117_i32_42 = arith.constant 117 : i32
    %28 = llvm.call @fpga_tile_submit(%5, %c2_i32, %c0_i32_35, %c1_i32_36, %c4_i32_37, %c16_i32_38, %c4_i32_39, %25, %27, %23, %c12_i32_27, %c2_i32_40, %c0_i32_41, %c117_i32_42) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %29 = bufferization.to_tensor %2 restrict writable : memref<12x12xf32> to tensor<12x12xf32>
    %c12_i32_43 = arith.constant 12 : i32
    %intptr_44 = memref.extract_aligned_pointer_as_index %2 : memref<12x12xf32> -> index
    %c384 = arith.constant 384 : index
    %30 = arith.addi %intptr_44, %c384 : index
    %31 = arith.index_cast %30 : index to i64
    %32 = llvm.inttoptr %31 : i64 to !llvm.ptr
    %subview_45 = memref.subview %1[8, 0] [4, 16] [1, 1] : memref<12x16xf32> to memref<4x16xf32, strided<[16, 1], offset: 128>>
    %reinterpret_cast_46 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_45, %reinterpret_cast_46 : memref<4x16xf32, strided<[16, 1], offset: 128>> to memref<4x16xf32>
    %intptr_47 = memref.extract_aligned_pointer_as_index %reinterpret_cast_46 : memref<4x16xf32> -> index
    %33 = arith.index_cast %intptr_47 : index to i64
    %34 = llvm.inttoptr %33 : i64 to !llvm.ptr
    %subview_48 = memref.subview %0[0, 0] [16, 4] [1, 1] : memref<16x12xf32> to memref<16x4xf32, strided<[12, 1]>>
    %reinterpret_cast_49 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_48, %reinterpret_cast_49 : memref<16x4xf32, strided<[12, 1]>> to memref<16x4xf32>
    %intptr_50 = memref.extract_aligned_pointer_as_index %reinterpret_cast_49 : memref<16x4xf32> -> index
    %35 = arith.index_cast %intptr_50 : index to i64
    %36 = llvm.inttoptr %35 : i64 to !llvm.ptr
    %c3_i32 = arith.constant 3 : i32
    %c0_i32_51 = arith.constant 0 : i32
    %c1_i32_52 = arith.constant 1 : i32
    %c4_i32_53 = arith.constant 4 : i32
    %c16_i32_54 = arith.constant 16 : i32
    %c4_i32_55 = arith.constant 4 : i32
    %c3_i32_56 = arith.constant 3 : i32
    %c0_i32_57 = arith.constant 0 : i32
    %c117_i32_58 = arith.constant 117 : i32
    %37 = llvm.call @fpga_tile_submit(%5, %c3_i32, %c0_i32_51, %c1_i32_52, %c4_i32_53, %c16_i32_54, %c4_i32_55, %34, %36, %32, %c12_i32_43, %c3_i32_56, %c0_i32_57, %c117_i32_58) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %38 = bufferization.to_tensor %2 restrict writable : memref<12x12xf32> to tensor<12x12xf32>
    %c12_i32_59 = arith.constant 12 : i32
    %intptr_60 = memref.extract_aligned_pointer_as_index %2 : memref<12x12xf32> -> index
    %c400 = arith.constant 400 : index
    %39 = arith.addi %intptr_60, %c400 : index
    %40 = arith.index_cast %39 : index to i64
    %41 = llvm.inttoptr %40 : i64 to !llvm.ptr
    %subview_61 = memref.subview %1[8, 0] [4, 16] [1, 1] : memref<12x16xf32> to memref<4x16xf32, strided<[16, 1], offset: 128>>
    %reinterpret_cast_62 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_61, %reinterpret_cast_62 : memref<4x16xf32, strided<[16, 1], offset: 128>> to memref<4x16xf32>
    %intptr_63 = memref.extract_aligned_pointer_as_index %reinterpret_cast_62 : memref<4x16xf32> -> index
    %42 = arith.index_cast %intptr_63 : index to i64
    %43 = llvm.inttoptr %42 : i64 to !llvm.ptr
    %subview_64 = memref.subview %0[0, 4] [16, 4] [1, 1] : memref<16x12xf32> to memref<16x4xf32, strided<[12, 1], offset: 4>>
    %reinterpret_cast_65 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_64, %reinterpret_cast_65 : memref<16x4xf32, strided<[12, 1], offset: 4>> to memref<16x4xf32>
    %intptr_66 = memref.extract_aligned_pointer_as_index %reinterpret_cast_65 : memref<16x4xf32> -> index
    %44 = arith.index_cast %intptr_66 : index to i64
    %45 = llvm.inttoptr %44 : i64 to !llvm.ptr
    %c4_i32_67 = arith.constant 4 : i32
    %c0_i32_68 = arith.constant 0 : i32
    %c1_i32_69 = arith.constant 1 : i32
    %c4_i32_70 = arith.constant 4 : i32
    %c16_i32_71 = arith.constant 16 : i32
    %c4_i32_72 = arith.constant 4 : i32
    %c1_i32_73 = arith.constant 1 : i32
    %c42_i32 = arith.constant 42 : i32
    %c117_i32_74 = arith.constant 117 : i32
    %46 = llvm.call @fpga_tile_submit(%5, %c4_i32_67, %c0_i32_68, %c1_i32_69, %c4_i32_70, %c16_i32_71, %c4_i32_72, %43, %45, %41, %c12_i32_59, %c1_i32_73, %c42_i32, %c117_i32_74) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %47 = bufferization.to_tensor %2 restrict writable : memref<12x12xf32> to tensor<12x12xf32>
    %c12_i32_75 = arith.constant 12 : i32
    %intptr_76 = memref.extract_aligned_pointer_as_index %2 : memref<12x12xf32> -> index
    %c416 = arith.constant 416 : index
    %48 = arith.addi %intptr_76, %c416 : index
    %49 = arith.index_cast %48 : index to i64
    %50 = llvm.inttoptr %49 : i64 to !llvm.ptr
    %subview_77 = memref.subview %1[8, 0] [4, 16] [1, 1] : memref<12x16xf32> to memref<4x16xf32, strided<[16, 1], offset: 128>>
    %reinterpret_cast_78 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_77, %reinterpret_cast_78 : memref<4x16xf32, strided<[16, 1], offset: 128>> to memref<4x16xf32>
    %intptr_79 = memref.extract_aligned_pointer_as_index %reinterpret_cast_78 : memref<4x16xf32> -> index
    %51 = arith.index_cast %intptr_79 : index to i64
    %52 = llvm.inttoptr %51 : i64 to !llvm.ptr
    %subview_80 = memref.subview %0[0, 8] [16, 4] [1, 1] : memref<16x12xf32> to memref<16x4xf32, strided<[12, 1], offset: 8>>
    %reinterpret_cast_81 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_80, %reinterpret_cast_81 : memref<16x4xf32, strided<[12, 1], offset: 8>> to memref<16x4xf32>
    %intptr_82 = memref.extract_aligned_pointer_as_index %reinterpret_cast_81 : memref<16x4xf32> -> index
    %53 = arith.index_cast %intptr_82 : index to i64
    %54 = llvm.inttoptr %53 : i64 to !llvm.ptr
    %c5_i32 = arith.constant 5 : i32
    %c0_i32_83 = arith.constant 0 : i32
    %c1_i32_84 = arith.constant 1 : i32
    %c4_i32_85 = arith.constant 4 : i32
    %c16_i32_86 = arith.constant 16 : i32
    %c4_i32_87 = arith.constant 4 : i32
    %c2_i32_88 = arith.constant 2 : i32
    %c42_i32_89 = arith.constant 42 : i32
    %c117_i32_90 = arith.constant 117 : i32
    %55 = llvm.call @fpga_tile_submit(%5, %c5_i32, %c0_i32_83, %c1_i32_84, %c4_i32_85, %c16_i32_86, %c4_i32_87, %52, %54, %50, %c12_i32_75, %c2_i32_88, %c42_i32_89, %c117_i32_90) : (!llvm.ptr, i32, i32, i32, i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32, i32) -> i32
    %56 = bufferization.to_tensor %2 restrict writable : memref<12x12xf32> to tensor<12x12xf32>
    %57 = llvm.call @fpga_gemm_wait(%5) : (!llvm.ptr) -> i32
    return %56 : tensor<12x12xf32>
  }
}

