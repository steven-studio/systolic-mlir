module {
  llvm.func @fpga_matmul_tiled_auto_scheduled(i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
  func.func @systolic_matmul(%arg0: tensor<12x16xf32>, %arg1: tensor<16x12xf32>, %arg2: tensor<12x12xf32>) -> tensor<12x12xf32> attributes {llvm.emit_c_interface} {
    %0 = bufferization.to_buffer %arg1 read_only : tensor<16x12xf32> to memref<16x12xf32>
    %1 = bufferization.to_buffer %arg0 read_only : tensor<12x16xf32> to memref<12x16xf32>
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
    %subview_2 = memref.subview %1[0, 0] [8, 16] [1, 1] : memref<12x16xf32> to memref<8x16xf32, strided<[16, 1]>>
    %reinterpret_cast_3 = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 16], strides: [16, 1] : memref<128xf32> to memref<8x16xf32>
    memref.copy %subview_2, %reinterpret_cast_3 : memref<8x16xf32, strided<[16, 1]>> to memref<8x16xf32>
    %intptr_4 = memref.extract_aligned_pointer_as_index %reinterpret_cast_3 : memref<8x16xf32> -> index
    %5 = arith.index_cast %intptr_4 : index to i64
    %6 = llvm.inttoptr %5 : i64 to !llvm.ptr
    %subview_5 = memref.subview %0[0, 0] [16, 8] [1, 1] : memref<16x12xf32> to memref<16x8xf32, strided<[12, 1]>>
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
    memref.copy %reinterpret_cast, %subview : memref<8x8xf32> to memref<8x8xf32, strided<[12, 1]>>
    %10 = bufferization.to_tensor %2 restrict writable : memref<12x12xf32> to tensor<12x12xf32>
    %subview_10 = memref.subview %2[0, 8] [4, 4] [1, 1] : memref<12x12xf32> to memref<4x4xf32, strided<[12, 1], offset: 8>>
    %reinterpret_cast_11 = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<64xf32> to memref<4x4xf32>
    memref.copy %subview_10, %reinterpret_cast_11 : memref<4x4xf32, strided<[12, 1], offset: 8>> to memref<4x4xf32>
    %intptr_12 = memref.extract_aligned_pointer_as_index %reinterpret_cast_11 : memref<4x4xf32> -> index
    %11 = arith.index_cast %intptr_12 : index to i64
    %12 = llvm.inttoptr %11 : i64 to !llvm.ptr
    %subview_13 = memref.subview %1[0, 0] [4, 16] [1, 1] : memref<12x16xf32> to memref<4x16xf32, strided<[16, 1]>>
    %reinterpret_cast_14 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_13, %reinterpret_cast_14 : memref<4x16xf32, strided<[16, 1]>> to memref<4x16xf32>
    %intptr_15 = memref.extract_aligned_pointer_as_index %reinterpret_cast_14 : memref<4x16xf32> -> index
    %13 = arith.index_cast %intptr_15 : index to i64
    %14 = llvm.inttoptr %13 : i64 to !llvm.ptr
    %subview_16 = memref.subview %0[0, 8] [16, 4] [1, 1] : memref<16x12xf32> to memref<16x4xf32, strided<[12, 1], offset: 8>>
    %reinterpret_cast_17 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_16, %reinterpret_cast_17 : memref<16x4xf32, strided<[12, 1], offset: 8>> to memref<16x4xf32>
    %intptr_18 = memref.extract_aligned_pointer_as_index %reinterpret_cast_17 : memref<16x4xf32> -> index
    %15 = arith.index_cast %intptr_18 : index to i64
    %16 = llvm.inttoptr %15 : i64 to !llvm.ptr
    %c4_i32 = arith.constant 4 : i32
    %c16_i32_19 = arith.constant 16 : i32
    %c4_i32_20 = arith.constant 4 : i32
    %c1_i32 = arith.constant 1 : i32
    %c0_i32_21 = arith.constant 0 : i32
    %c117_i32 = arith.constant 117 : i32
    %17 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32, %c16_i32_19, %c4_i32_20, %14, %16, %12, %c1_i32, %c0_i32_21, %c117_i32) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast_11, %subview_10 : memref<4x4xf32> to memref<4x4xf32, strided<[12, 1], offset: 8>>
    %18 = bufferization.to_tensor %2 restrict writable : memref<12x12xf32> to tensor<12x12xf32>
    %subview_22 = memref.subview %2[4, 8] [4, 4] [1, 1] : memref<12x12xf32> to memref<4x4xf32, strided<[12, 1], offset: 56>>
    %reinterpret_cast_23 = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<64xf32> to memref<4x4xf32>
    memref.copy %subview_22, %reinterpret_cast_23 : memref<4x4xf32, strided<[12, 1], offset: 56>> to memref<4x4xf32>
    %intptr_24 = memref.extract_aligned_pointer_as_index %reinterpret_cast_23 : memref<4x4xf32> -> index
    %19 = arith.index_cast %intptr_24 : index to i64
    %20 = llvm.inttoptr %19 : i64 to !llvm.ptr
    %subview_25 = memref.subview %1[4, 0] [4, 16] [1, 1] : memref<12x16xf32> to memref<4x16xf32, strided<[16, 1], offset: 64>>
    %reinterpret_cast_26 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_25, %reinterpret_cast_26 : memref<4x16xf32, strided<[16, 1], offset: 64>> to memref<4x16xf32>
    %intptr_27 = memref.extract_aligned_pointer_as_index %reinterpret_cast_26 : memref<4x16xf32> -> index
    %21 = arith.index_cast %intptr_27 : index to i64
    %22 = llvm.inttoptr %21 : i64 to !llvm.ptr
    %subview_28 = memref.subview %0[0, 8] [16, 4] [1, 1] : memref<16x12xf32> to memref<16x4xf32, strided<[12, 1], offset: 8>>
    %reinterpret_cast_29 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_28, %reinterpret_cast_29 : memref<16x4xf32, strided<[12, 1], offset: 8>> to memref<16x4xf32>
    %intptr_30 = memref.extract_aligned_pointer_as_index %reinterpret_cast_29 : memref<16x4xf32> -> index
    %23 = arith.index_cast %intptr_30 : index to i64
    %24 = llvm.inttoptr %23 : i64 to !llvm.ptr
    %c4_i32_31 = arith.constant 4 : i32
    %c16_i32_32 = arith.constant 16 : i32
    %c4_i32_33 = arith.constant 4 : i32
    %c2_i32 = arith.constant 2 : i32
    %c0_i32_34 = arith.constant 0 : i32
    %c117_i32_35 = arith.constant 117 : i32
    %25 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_31, %c16_i32_32, %c4_i32_33, %22, %24, %20, %c2_i32, %c0_i32_34, %c117_i32_35) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast_23, %subview_22 : memref<4x4xf32> to memref<4x4xf32, strided<[12, 1], offset: 56>>
    %26 = bufferization.to_tensor %2 restrict writable : memref<12x12xf32> to tensor<12x12xf32>
    %subview_36 = memref.subview %2[8, 0] [4, 4] [1, 1] : memref<12x12xf32> to memref<4x4xf32, strided<[12, 1], offset: 96>>
    %reinterpret_cast_37 = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<64xf32> to memref<4x4xf32>
    memref.copy %subview_36, %reinterpret_cast_37 : memref<4x4xf32, strided<[12, 1], offset: 96>> to memref<4x4xf32>
    %intptr_38 = memref.extract_aligned_pointer_as_index %reinterpret_cast_37 : memref<4x4xf32> -> index
    %27 = arith.index_cast %intptr_38 : index to i64
    %28 = llvm.inttoptr %27 : i64 to !llvm.ptr
    %subview_39 = memref.subview %1[8, 0] [4, 16] [1, 1] : memref<12x16xf32> to memref<4x16xf32, strided<[16, 1], offset: 128>>
    %reinterpret_cast_40 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_39, %reinterpret_cast_40 : memref<4x16xf32, strided<[16, 1], offset: 128>> to memref<4x16xf32>
    %intptr_41 = memref.extract_aligned_pointer_as_index %reinterpret_cast_40 : memref<4x16xf32> -> index
    %29 = arith.index_cast %intptr_41 : index to i64
    %30 = llvm.inttoptr %29 : i64 to !llvm.ptr
    %subview_42 = memref.subview %0[0, 0] [16, 4] [1, 1] : memref<16x12xf32> to memref<16x4xf32, strided<[12, 1]>>
    %reinterpret_cast_43 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_42, %reinterpret_cast_43 : memref<16x4xf32, strided<[12, 1]>> to memref<16x4xf32>
    %intptr_44 = memref.extract_aligned_pointer_as_index %reinterpret_cast_43 : memref<16x4xf32> -> index
    %31 = arith.index_cast %intptr_44 : index to i64
    %32 = llvm.inttoptr %31 : i64 to !llvm.ptr
    %c4_i32_45 = arith.constant 4 : i32
    %c16_i32_46 = arith.constant 16 : i32
    %c4_i32_47 = arith.constant 4 : i32
    %c3_i32 = arith.constant 3 : i32
    %c0_i32_48 = arith.constant 0 : i32
    %c117_i32_49 = arith.constant 117 : i32
    %33 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_45, %c16_i32_46, %c4_i32_47, %30, %32, %28, %c3_i32, %c0_i32_48, %c117_i32_49) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast_37, %subview_36 : memref<4x4xf32> to memref<4x4xf32, strided<[12, 1], offset: 96>>
    %34 = bufferization.to_tensor %2 restrict writable : memref<12x12xf32> to tensor<12x12xf32>
    %subview_50 = memref.subview %2[8, 4] [4, 4] [1, 1] : memref<12x12xf32> to memref<4x4xf32, strided<[12, 1], offset: 100>>
    %reinterpret_cast_51 = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<64xf32> to memref<4x4xf32>
    memref.copy %subview_50, %reinterpret_cast_51 : memref<4x4xf32, strided<[12, 1], offset: 100>> to memref<4x4xf32>
    %intptr_52 = memref.extract_aligned_pointer_as_index %reinterpret_cast_51 : memref<4x4xf32> -> index
    %35 = arith.index_cast %intptr_52 : index to i64
    %36 = llvm.inttoptr %35 : i64 to !llvm.ptr
    %subview_53 = memref.subview %1[8, 0] [4, 16] [1, 1] : memref<12x16xf32> to memref<4x16xf32, strided<[16, 1], offset: 128>>
    %reinterpret_cast_54 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_53, %reinterpret_cast_54 : memref<4x16xf32, strided<[16, 1], offset: 128>> to memref<4x16xf32>
    %intptr_55 = memref.extract_aligned_pointer_as_index %reinterpret_cast_54 : memref<4x16xf32> -> index
    %37 = arith.index_cast %intptr_55 : index to i64
    %38 = llvm.inttoptr %37 : i64 to !llvm.ptr
    %subview_56 = memref.subview %0[0, 4] [16, 4] [1, 1] : memref<16x12xf32> to memref<16x4xf32, strided<[12, 1], offset: 4>>
    %reinterpret_cast_57 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_56, %reinterpret_cast_57 : memref<16x4xf32, strided<[12, 1], offset: 4>> to memref<16x4xf32>
    %intptr_58 = memref.extract_aligned_pointer_as_index %reinterpret_cast_57 : memref<16x4xf32> -> index
    %39 = arith.index_cast %intptr_58 : index to i64
    %40 = llvm.inttoptr %39 : i64 to !llvm.ptr
    %c4_i32_59 = arith.constant 4 : i32
    %c16_i32_60 = arith.constant 16 : i32
    %c4_i32_61 = arith.constant 4 : i32
    %c1_i32_62 = arith.constant 1 : i32
    %c42_i32 = arith.constant 42 : i32
    %c117_i32_63 = arith.constant 117 : i32
    %41 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_59, %c16_i32_60, %c4_i32_61, %38, %40, %36, %c1_i32_62, %c42_i32, %c117_i32_63) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast_51, %subview_50 : memref<4x4xf32> to memref<4x4xf32, strided<[12, 1], offset: 100>>
    %42 = bufferization.to_tensor %2 restrict writable : memref<12x12xf32> to tensor<12x12xf32>
    %subview_64 = memref.subview %2[8, 8] [4, 4] [1, 1] : memref<12x12xf32> to memref<4x4xf32, strided<[12, 1], offset: 104>>
    %reinterpret_cast_65 = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<64xf32> to memref<4x4xf32>
    memref.copy %subview_64, %reinterpret_cast_65 : memref<4x4xf32, strided<[12, 1], offset: 104>> to memref<4x4xf32>
    %intptr_66 = memref.extract_aligned_pointer_as_index %reinterpret_cast_65 : memref<4x4xf32> -> index
    %43 = arith.index_cast %intptr_66 : index to i64
    %44 = llvm.inttoptr %43 : i64 to !llvm.ptr
    %subview_67 = memref.subview %1[8, 0] [4, 16] [1, 1] : memref<12x16xf32> to memref<4x16xf32, strided<[16, 1], offset: 128>>
    %reinterpret_cast_68 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_67, %reinterpret_cast_68 : memref<4x16xf32, strided<[16, 1], offset: 128>> to memref<4x16xf32>
    %intptr_69 = memref.extract_aligned_pointer_as_index %reinterpret_cast_68 : memref<4x16xf32> -> index
    %45 = arith.index_cast %intptr_69 : index to i64
    %46 = llvm.inttoptr %45 : i64 to !llvm.ptr
    %subview_70 = memref.subview %0[0, 8] [16, 4] [1, 1] : memref<16x12xf32> to memref<16x4xf32, strided<[12, 1], offset: 8>>
    %reinterpret_cast_71 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_70, %reinterpret_cast_71 : memref<16x4xf32, strided<[12, 1], offset: 8>> to memref<16x4xf32>
    %intptr_72 = memref.extract_aligned_pointer_as_index %reinterpret_cast_71 : memref<16x4xf32> -> index
    %47 = arith.index_cast %intptr_72 : index to i64
    %48 = llvm.inttoptr %47 : i64 to !llvm.ptr
    %c4_i32_73 = arith.constant 4 : i32
    %c16_i32_74 = arith.constant 16 : i32
    %c4_i32_75 = arith.constant 4 : i32
    %c2_i32_76 = arith.constant 2 : i32
    %c42_i32_77 = arith.constant 42 : i32
    %c117_i32_78 = arith.constant 117 : i32
    %49 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_73, %c16_i32_74, %c4_i32_75, %46, %48, %44, %c2_i32_76, %c42_i32_77, %c117_i32_78) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast_65, %subview_64 : memref<4x4xf32> to memref<4x4xf32, strided<[12, 1], offset: 104>>
    %50 = bufferization.to_tensor %2 restrict writable : memref<12x12xf32> to tensor<12x12xf32>
    return %50 : tensor<12x12xf32>
  }
}

