module {
  llvm.func @fpga_matmul_tiled_auto_scheduled(i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
  func.func @systolic_matmul(%arg0: tensor<15x16xf32>, %arg1: tensor<16x15xf32>, %arg2: tensor<15x15xf32>) -> tensor<15x15xf32> attributes {llvm.emit_c_interface} {
    %0 = bufferization.to_buffer %arg1 read_only : tensor<16x15xf32> to memref<16x15xf32>
    %1 = bufferization.to_buffer %arg0 read_only : tensor<15x16xf32> to memref<15x16xf32>
    %alloca = memref.alloca() : memref<128xf32>
    %alloca_0 = memref.alloca() : memref<128xf32>
    %alloca_1 = memref.alloca() : memref<64xf32>
    %2 = bufferization.to_buffer %arg2 : tensor<15x15xf32> to memref<15x15xf32>
    %subview = memref.subview %2[0, 0] [8, 8] [1, 1] : memref<15x15xf32> to memref<8x8xf32, strided<[15, 1]>>
    %reinterpret_cast = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [8, 8], strides: [8, 1] : memref<64xf32> to memref<8x8xf32>
    memref.copy %subview, %reinterpret_cast : memref<8x8xf32, strided<[15, 1]>> to memref<8x8xf32>
    %intptr = memref.extract_aligned_pointer_as_index %reinterpret_cast : memref<8x8xf32> -> index
    %3 = arith.index_cast %intptr : index to i64
    %4 = llvm.inttoptr %3 : i64 to !llvm.ptr
    %subview_2 = memref.subview %1[0, 0] [8, 16] [1, 1] : memref<15x16xf32> to memref<8x16xf32, strided<[16, 1]>>
    %reinterpret_cast_3 = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 16], strides: [16, 1] : memref<128xf32> to memref<8x16xf32>
    memref.copy %subview_2, %reinterpret_cast_3 : memref<8x16xf32, strided<[16, 1]>> to memref<8x16xf32>
    %intptr_4 = memref.extract_aligned_pointer_as_index %reinterpret_cast_3 : memref<8x16xf32> -> index
    %5 = arith.index_cast %intptr_4 : index to i64
    %6 = llvm.inttoptr %5 : i64 to !llvm.ptr
    %subview_5 = memref.subview %0[0, 0] [16, 8] [1, 1] : memref<16x15xf32> to memref<16x8xf32, strided<[15, 1]>>
    %reinterpret_cast_6 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 8], strides: [8, 1] : memref<128xf32> to memref<16x8xf32>
    memref.copy %subview_5, %reinterpret_cast_6 : memref<16x8xf32, strided<[15, 1]>> to memref<16x8xf32>
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
    memref.copy %reinterpret_cast, %subview : memref<8x8xf32> to memref<8x8xf32, strided<[15, 1]>>
    %10 = bufferization.to_tensor %2 restrict writable : memref<15x15xf32> to tensor<15x15xf32>
    %subview_10 = memref.subview %2[0, 8] [8, 7] [1, 1] : memref<15x15xf32> to memref<8x7xf32, strided<[15, 1], offset: 8>>
    %reinterpret_cast_11 = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [8, 7], strides: [7, 1] : memref<64xf32> to memref<8x7xf32>
    memref.copy %subview_10, %reinterpret_cast_11 : memref<8x7xf32, strided<[15, 1], offset: 8>> to memref<8x7xf32>
    %intptr_12 = memref.extract_aligned_pointer_as_index %reinterpret_cast_11 : memref<8x7xf32> -> index
    %11 = arith.index_cast %intptr_12 : index to i64
    %12 = llvm.inttoptr %11 : i64 to !llvm.ptr
    %subview_13 = memref.subview %1[0, 0] [8, 16] [1, 1] : memref<15x16xf32> to memref<8x16xf32, strided<[16, 1]>>
    %reinterpret_cast_14 = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 16], strides: [16, 1] : memref<128xf32> to memref<8x16xf32>
    memref.copy %subview_13, %reinterpret_cast_14 : memref<8x16xf32, strided<[16, 1]>> to memref<8x16xf32>
    %intptr_15 = memref.extract_aligned_pointer_as_index %reinterpret_cast_14 : memref<8x16xf32> -> index
    %13 = arith.index_cast %intptr_15 : index to i64
    %14 = llvm.inttoptr %13 : i64 to !llvm.ptr
    %subview_16 = memref.subview %0[0, 8] [16, 7] [1, 1] : memref<16x15xf32> to memref<16x7xf32, strided<[15, 1], offset: 8>>
    %reinterpret_cast_17 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 7], strides: [7, 1] : memref<128xf32> to memref<16x7xf32>
    memref.copy %subview_16, %reinterpret_cast_17 : memref<16x7xf32, strided<[15, 1], offset: 8>> to memref<16x7xf32>
    %intptr_18 = memref.extract_aligned_pointer_as_index %reinterpret_cast_17 : memref<16x7xf32> -> index
    %15 = arith.index_cast %intptr_18 : index to i64
    %16 = llvm.inttoptr %15 : i64 to !llvm.ptr
    %c8_i32_19 = arith.constant 8 : i32
    %c16_i32_20 = arith.constant 16 : i32
    %c7_i32 = arith.constant 7 : i32
    %c0_i32_21 = arith.constant 0 : i32
    %c42_i32 = arith.constant 42 : i32
    %c125_i32_22 = arith.constant 125 : i32
    %17 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c8_i32_19, %c16_i32_20, %c7_i32, %14, %16, %12, %c0_i32_21, %c42_i32, %c125_i32_22) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast_11, %subview_10 : memref<8x7xf32> to memref<8x7xf32, strided<[15, 1], offset: 8>>
    %18 = bufferization.to_tensor %2 restrict writable : memref<15x15xf32> to tensor<15x15xf32>
    %subview_23 = memref.subview %2[8, 0] [7, 8] [1, 1] : memref<15x15xf32> to memref<7x8xf32, strided<[15, 1], offset: 120>>
    %reinterpret_cast_24 = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [7, 8], strides: [8, 1] : memref<64xf32> to memref<7x8xf32>
    memref.copy %subview_23, %reinterpret_cast_24 : memref<7x8xf32, strided<[15, 1], offset: 120>> to memref<7x8xf32>
    %intptr_25 = memref.extract_aligned_pointer_as_index %reinterpret_cast_24 : memref<7x8xf32> -> index
    %19 = arith.index_cast %intptr_25 : index to i64
    %20 = llvm.inttoptr %19 : i64 to !llvm.ptr
    %subview_26 = memref.subview %1[8, 0] [7, 16] [1, 1] : memref<15x16xf32> to memref<7x16xf32, strided<[16, 1], offset: 128>>
    %reinterpret_cast_27 = memref.reinterpret_cast %alloca to offset: [0], sizes: [7, 16], strides: [16, 1] : memref<128xf32> to memref<7x16xf32>
    memref.copy %subview_26, %reinterpret_cast_27 : memref<7x16xf32, strided<[16, 1], offset: 128>> to memref<7x16xf32>
    %intptr_28 = memref.extract_aligned_pointer_as_index %reinterpret_cast_27 : memref<7x16xf32> -> index
    %21 = arith.index_cast %intptr_28 : index to i64
    %22 = llvm.inttoptr %21 : i64 to !llvm.ptr
    %subview_29 = memref.subview %0[0, 0] [16, 8] [1, 1] : memref<16x15xf32> to memref<16x8xf32, strided<[15, 1]>>
    %reinterpret_cast_30 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 8], strides: [8, 1] : memref<128xf32> to memref<16x8xf32>
    memref.copy %subview_29, %reinterpret_cast_30 : memref<16x8xf32, strided<[15, 1]>> to memref<16x8xf32>
    %intptr_31 = memref.extract_aligned_pointer_as_index %reinterpret_cast_30 : memref<16x8xf32> -> index
    %23 = arith.index_cast %intptr_31 : index to i64
    %24 = llvm.inttoptr %23 : i64 to !llvm.ptr
    %c7_i32_32 = arith.constant 7 : i32
    %c16_i32_33 = arith.constant 16 : i32
    %c8_i32_34 = arith.constant 8 : i32
    %c0_i32_35 = arith.constant 0 : i32
    %c128_i32 = arith.constant 128 : i32
    %c125_i32_36 = arith.constant 125 : i32
    %25 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c7_i32_32, %c16_i32_33, %c8_i32_34, %22, %24, %20, %c0_i32_35, %c128_i32, %c125_i32_36) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast_24, %subview_23 : memref<7x8xf32> to memref<7x8xf32, strided<[15, 1], offset: 120>>
    %26 = bufferization.to_tensor %2 restrict writable : memref<15x15xf32> to tensor<15x15xf32>
    %subview_37 = memref.subview %2[8, 8] [4, 4] [1, 1] : memref<15x15xf32> to memref<4x4xf32, strided<[15, 1], offset: 128>>
    %reinterpret_cast_38 = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<64xf32> to memref<4x4xf32>
    memref.copy %subview_37, %reinterpret_cast_38 : memref<4x4xf32, strided<[15, 1], offset: 128>> to memref<4x4xf32>
    %intptr_39 = memref.extract_aligned_pointer_as_index %reinterpret_cast_38 : memref<4x4xf32> -> index
    %27 = arith.index_cast %intptr_39 : index to i64
    %28 = llvm.inttoptr %27 : i64 to !llvm.ptr
    %subview_40 = memref.subview %1[8, 0] [4, 16] [1, 1] : memref<15x16xf32> to memref<4x16xf32, strided<[16, 1], offset: 128>>
    %reinterpret_cast_41 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_40, %reinterpret_cast_41 : memref<4x16xf32, strided<[16, 1], offset: 128>> to memref<4x16xf32>
    %intptr_42 = memref.extract_aligned_pointer_as_index %reinterpret_cast_41 : memref<4x16xf32> -> index
    %29 = arith.index_cast %intptr_42 : index to i64
    %30 = llvm.inttoptr %29 : i64 to !llvm.ptr
    %subview_43 = memref.subview %0[0, 8] [16, 4] [1, 1] : memref<16x15xf32> to memref<16x4xf32, strided<[15, 1], offset: 8>>
    %reinterpret_cast_44 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_43, %reinterpret_cast_44 : memref<16x4xf32, strided<[15, 1], offset: 8>> to memref<16x4xf32>
    %intptr_45 = memref.extract_aligned_pointer_as_index %reinterpret_cast_44 : memref<16x4xf32> -> index
    %31 = arith.index_cast %intptr_45 : index to i64
    %32 = llvm.inttoptr %31 : i64 to !llvm.ptr
    %c4_i32 = arith.constant 4 : i32
    %c16_i32_46 = arith.constant 16 : i32
    %c4_i32_47 = arith.constant 4 : i32
    %c1_i32 = arith.constant 1 : i32
    %c0_i32_48 = arith.constant 0 : i32
    %c117_i32 = arith.constant 117 : i32
    %33 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32, %c16_i32_46, %c4_i32_47, %30, %32, %28, %c1_i32, %c0_i32_48, %c117_i32) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast_38, %subview_37 : memref<4x4xf32> to memref<4x4xf32, strided<[15, 1], offset: 128>>
    %34 = bufferization.to_tensor %2 restrict writable : memref<15x15xf32> to tensor<15x15xf32>
    %subview_49 = memref.subview %2[8, 12] [4, 3] [1, 1] : memref<15x15xf32> to memref<4x3xf32, strided<[15, 1], offset: 132>>
    %reinterpret_cast_50 = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [4, 3], strides: [3, 1] : memref<64xf32> to memref<4x3xf32>
    memref.copy %subview_49, %reinterpret_cast_50 : memref<4x3xf32, strided<[15, 1], offset: 132>> to memref<4x3xf32>
    %intptr_51 = memref.extract_aligned_pointer_as_index %reinterpret_cast_50 : memref<4x3xf32> -> index
    %35 = arith.index_cast %intptr_51 : index to i64
    %36 = llvm.inttoptr %35 : i64 to !llvm.ptr
    %subview_52 = memref.subview %1[8, 0] [4, 16] [1, 1] : memref<15x16xf32> to memref<4x16xf32, strided<[16, 1], offset: 128>>
    %reinterpret_cast_53 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_52, %reinterpret_cast_53 : memref<4x16xf32, strided<[16, 1], offset: 128>> to memref<4x16xf32>
    %intptr_54 = memref.extract_aligned_pointer_as_index %reinterpret_cast_53 : memref<4x16xf32> -> index
    %37 = arith.index_cast %intptr_54 : index to i64
    %38 = llvm.inttoptr %37 : i64 to !llvm.ptr
    %subview_55 = memref.subview %0[0, 12] [16, 3] [1, 1] : memref<16x15xf32> to memref<16x3xf32, strided<[15, 1], offset: 12>>
    %reinterpret_cast_56 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 3], strides: [3, 1] : memref<128xf32> to memref<16x3xf32>
    memref.copy %subview_55, %reinterpret_cast_56 : memref<16x3xf32, strided<[15, 1], offset: 12>> to memref<16x3xf32>
    %intptr_57 = memref.extract_aligned_pointer_as_index %reinterpret_cast_56 : memref<16x3xf32> -> index
    %39 = arith.index_cast %intptr_57 : index to i64
    %40 = llvm.inttoptr %39 : i64 to !llvm.ptr
    %c4_i32_58 = arith.constant 4 : i32
    %c16_i32_59 = arith.constant 16 : i32
    %c3_i32 = arith.constant 3 : i32
    %c2_i32 = arith.constant 2 : i32
    %c0_i32_60 = arith.constant 0 : i32
    %c117_i32_61 = arith.constant 117 : i32
    %41 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_58, %c16_i32_59, %c3_i32, %38, %40, %36, %c2_i32, %c0_i32_60, %c117_i32_61) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast_50, %subview_49 : memref<4x3xf32> to memref<4x3xf32, strided<[15, 1], offset: 132>>
    %42 = bufferization.to_tensor %2 restrict writable : memref<15x15xf32> to tensor<15x15xf32>
    %subview_62 = memref.subview %2[12, 8] [3, 4] [1, 1] : memref<15x15xf32> to memref<3x4xf32, strided<[15, 1], offset: 188>>
    %reinterpret_cast_63 = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [3, 4], strides: [4, 1] : memref<64xf32> to memref<3x4xf32>
    memref.copy %subview_62, %reinterpret_cast_63 : memref<3x4xf32, strided<[15, 1], offset: 188>> to memref<3x4xf32>
    %intptr_64 = memref.extract_aligned_pointer_as_index %reinterpret_cast_63 : memref<3x4xf32> -> index
    %43 = arith.index_cast %intptr_64 : index to i64
    %44 = llvm.inttoptr %43 : i64 to !llvm.ptr
    %subview_65 = memref.subview %1[12, 0] [3, 16] [1, 1] : memref<15x16xf32> to memref<3x16xf32, strided<[16, 1], offset: 192>>
    %reinterpret_cast_66 = memref.reinterpret_cast %alloca to offset: [0], sizes: [3, 16], strides: [16, 1] : memref<128xf32> to memref<3x16xf32>
    memref.copy %subview_65, %reinterpret_cast_66 : memref<3x16xf32, strided<[16, 1], offset: 192>> to memref<3x16xf32>
    %intptr_67 = memref.extract_aligned_pointer_as_index %reinterpret_cast_66 : memref<3x16xf32> -> index
    %45 = arith.index_cast %intptr_67 : index to i64
    %46 = llvm.inttoptr %45 : i64 to !llvm.ptr
    %subview_68 = memref.subview %0[0, 8] [16, 4] [1, 1] : memref<16x15xf32> to memref<16x4xf32, strided<[15, 1], offset: 8>>
    %reinterpret_cast_69 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_68, %reinterpret_cast_69 : memref<16x4xf32, strided<[15, 1], offset: 8>> to memref<16x4xf32>
    %intptr_70 = memref.extract_aligned_pointer_as_index %reinterpret_cast_69 : memref<16x4xf32> -> index
    %47 = arith.index_cast %intptr_70 : index to i64
    %48 = llvm.inttoptr %47 : i64 to !llvm.ptr
    %c3_i32_71 = arith.constant 3 : i32
    %c16_i32_72 = arith.constant 16 : i32
    %c4_i32_73 = arith.constant 4 : i32
    %c3_i32_74 = arith.constant 3 : i32
    %c0_i32_75 = arith.constant 0 : i32
    %c117_i32_76 = arith.constant 117 : i32
    %49 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c3_i32_71, %c16_i32_72, %c4_i32_73, %46, %48, %44, %c3_i32_74, %c0_i32_75, %c117_i32_76) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast_63, %subview_62 : memref<3x4xf32> to memref<3x4xf32, strided<[15, 1], offset: 188>>
    %50 = bufferization.to_tensor %2 restrict writable : memref<15x15xf32> to tensor<15x15xf32>
    %subview_77 = memref.subview %2[12, 12] [3, 3] [1, 1] : memref<15x15xf32> to memref<3x3xf32, strided<[15, 1], offset: 192>>
    %reinterpret_cast_78 = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [3, 3], strides: [3, 1] : memref<64xf32> to memref<3x3xf32>
    memref.copy %subview_77, %reinterpret_cast_78 : memref<3x3xf32, strided<[15, 1], offset: 192>> to memref<3x3xf32>
    %intptr_79 = memref.extract_aligned_pointer_as_index %reinterpret_cast_78 : memref<3x3xf32> -> index
    %51 = arith.index_cast %intptr_79 : index to i64
    %52 = llvm.inttoptr %51 : i64 to !llvm.ptr
    %subview_80 = memref.subview %1[12, 0] [3, 16] [1, 1] : memref<15x16xf32> to memref<3x16xf32, strided<[16, 1], offset: 192>>
    %reinterpret_cast_81 = memref.reinterpret_cast %alloca to offset: [0], sizes: [3, 16], strides: [16, 1] : memref<128xf32> to memref<3x16xf32>
    memref.copy %subview_80, %reinterpret_cast_81 : memref<3x16xf32, strided<[16, 1], offset: 192>> to memref<3x16xf32>
    %intptr_82 = memref.extract_aligned_pointer_as_index %reinterpret_cast_81 : memref<3x16xf32> -> index
    %53 = arith.index_cast %intptr_82 : index to i64
    %54 = llvm.inttoptr %53 : i64 to !llvm.ptr
    %subview_83 = memref.subview %0[0, 12] [16, 3] [1, 1] : memref<16x15xf32> to memref<16x3xf32, strided<[15, 1], offset: 12>>
    %reinterpret_cast_84 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 3], strides: [3, 1] : memref<128xf32> to memref<16x3xf32>
    memref.copy %subview_83, %reinterpret_cast_84 : memref<16x3xf32, strided<[15, 1], offset: 12>> to memref<16x3xf32>
    %intptr_85 = memref.extract_aligned_pointer_as_index %reinterpret_cast_84 : memref<16x3xf32> -> index
    %55 = arith.index_cast %intptr_85 : index to i64
    %56 = llvm.inttoptr %55 : i64 to !llvm.ptr
    %c3_i32_86 = arith.constant 3 : i32
    %c16_i32_87 = arith.constant 16 : i32
    %c3_i32_88 = arith.constant 3 : i32
    %c1_i32_89 = arith.constant 1 : i32
    %c42_i32_90 = arith.constant 42 : i32
    %c117_i32_91 = arith.constant 117 : i32
    %57 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c3_i32_86, %c16_i32_87, %c3_i32_88, %54, %56, %52, %c1_i32_89, %c42_i32_90, %c117_i32_91) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast_78, %subview_77 : memref<3x3xf32> to memref<3x3xf32, strided<[15, 1], offset: 192>>
    %58 = bufferization.to_tensor %2 restrict writable : memref<15x15xf32> to tensor<15x15xf32>
    return %58 : tensor<15x15xf32>
  }
}

