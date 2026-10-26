module {
  llvm.func @fpga_matmul_tiled_auto_scheduled(i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
  func.func @systolic_matmul(%arg0: tensor<16x31xf32>, %arg1: tensor<31x16xf32>, %arg2: tensor<16x16xf32>) -> tensor<16x16xf32> attributes {llvm.emit_c_interface} {
    %0 = bufferization.to_buffer %arg1 read_only : tensor<31x16xf32> to memref<31x16xf32>
    %1 = bufferization.to_buffer %arg0 read_only : tensor<16x31xf32> to memref<16x31xf32>
    %alloca = memref.alloca() : memref<128xf32>
    %alloca_0 = memref.alloca() : memref<128xf32>
    %alloca_1 = memref.alloca() : memref<64xf32>
    %2 = bufferization.to_buffer %arg2 : tensor<16x16xf32> to memref<16x16xf32>
    %subview = memref.subview %2[0, 0] [8, 8] [1, 1] : memref<16x16xf32> to memref<8x8xf32, strided<[16, 1]>>
    %reinterpret_cast = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [8, 8], strides: [8, 1] : memref<64xf32> to memref<8x8xf32>
    memref.copy %subview, %reinterpret_cast : memref<8x8xf32, strided<[16, 1]>> to memref<8x8xf32>
    %intptr = memref.extract_aligned_pointer_as_index %reinterpret_cast : memref<8x8xf32> -> index
    %3 = arith.index_cast %intptr : index to i64
    %4 = llvm.inttoptr %3 : i64 to !llvm.ptr
    %subview_2 = memref.subview %1[0, 0] [8, 16] [1, 1] : memref<16x31xf32> to memref<8x16xf32, strided<[31, 1]>>
    %reinterpret_cast_3 = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 16], strides: [16, 1] : memref<128xf32> to memref<8x16xf32>
    memref.copy %subview_2, %reinterpret_cast_3 : memref<8x16xf32, strided<[31, 1]>> to memref<8x16xf32>
    %intptr_4 = memref.extract_aligned_pointer_as_index %reinterpret_cast_3 : memref<8x16xf32> -> index
    %5 = arith.index_cast %intptr_4 : index to i64
    %6 = llvm.inttoptr %5 : i64 to !llvm.ptr
    %subview_5 = memref.subview %0[0, 0] [16, 8] [1, 1] : memref<31x16xf32> to memref<16x8xf32, strided<[16, 1]>>
    %reinterpret_cast_6 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 8], strides: [8, 1] : memref<128xf32> to memref<16x8xf32>
    memref.copy %subview_5, %reinterpret_cast_6 : memref<16x8xf32, strided<[16, 1]>> to memref<16x8xf32>
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
    %subview_10 = memref.subview %1[0, 16] [8, 15] [1, 1] : memref<16x31xf32> to memref<8x15xf32, strided<[31, 1], offset: 16>>
    %reinterpret_cast_11 = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 15], strides: [15, 1] : memref<128xf32> to memref<8x15xf32>
    memref.copy %subview_10, %reinterpret_cast_11 : memref<8x15xf32, strided<[31, 1], offset: 16>> to memref<8x15xf32>
    %intptr_12 = memref.extract_aligned_pointer_as_index %reinterpret_cast_11 : memref<8x15xf32> -> index
    %10 = arith.index_cast %intptr_12 : index to i64
    %11 = llvm.inttoptr %10 : i64 to !llvm.ptr
    %subview_13 = memref.subview %0[16, 0] [15, 8] [1, 1] : memref<31x16xf32> to memref<15x8xf32, strided<[16, 1], offset: 256>>
    %reinterpret_cast_14 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [15, 8], strides: [8, 1] : memref<128xf32> to memref<15x8xf32>
    memref.copy %subview_13, %reinterpret_cast_14 : memref<15x8xf32, strided<[16, 1], offset: 256>> to memref<15x8xf32>
    %intptr_15 = memref.extract_aligned_pointer_as_index %reinterpret_cast_14 : memref<15x8xf32> -> index
    %12 = arith.index_cast %intptr_15 : index to i64
    %13 = llvm.inttoptr %12 : i64 to !llvm.ptr
    %c8_i32_16 = arith.constant 8 : i32
    %c15_i32 = arith.constant 15 : i32
    %c8_i32_17 = arith.constant 8 : i32
    %c0_i32_18 = arith.constant 0 : i32
    %c42_i32 = arith.constant 42 : i32
    %c124_i32 = arith.constant 124 : i32
    %14 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c8_i32_16, %c15_i32, %c8_i32_17, %11, %13, %4, %c0_i32_18, %c42_i32, %c124_i32) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast, %subview : memref<8x8xf32> to memref<8x8xf32, strided<[16, 1]>>
    %15 = bufferization.to_tensor %2 restrict writable : memref<16x16xf32> to tensor<16x16xf32>
    %subview_19 = memref.subview %2[0, 8] [8, 8] [1, 1] : memref<16x16xf32> to memref<8x8xf32, strided<[16, 1], offset: 8>>
    %reinterpret_cast_20 = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [8, 8], strides: [8, 1] : memref<64xf32> to memref<8x8xf32>
    memref.copy %subview_19, %reinterpret_cast_20 : memref<8x8xf32, strided<[16, 1], offset: 8>> to memref<8x8xf32>
    %intptr_21 = memref.extract_aligned_pointer_as_index %reinterpret_cast_20 : memref<8x8xf32> -> index
    %16 = arith.index_cast %intptr_21 : index to i64
    %17 = llvm.inttoptr %16 : i64 to !llvm.ptr
    %subview_22 = memref.subview %1[0, 0] [8, 16] [1, 1] : memref<16x31xf32> to memref<8x16xf32, strided<[31, 1]>>
    %reinterpret_cast_23 = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 16], strides: [16, 1] : memref<128xf32> to memref<8x16xf32>
    memref.copy %subview_22, %reinterpret_cast_23 : memref<8x16xf32, strided<[31, 1]>> to memref<8x16xf32>
    %intptr_24 = memref.extract_aligned_pointer_as_index %reinterpret_cast_23 : memref<8x16xf32> -> index
    %18 = arith.index_cast %intptr_24 : index to i64
    %19 = llvm.inttoptr %18 : i64 to !llvm.ptr
    %subview_25 = memref.subview %0[0, 8] [16, 8] [1, 1] : memref<31x16xf32> to memref<16x8xf32, strided<[16, 1], offset: 8>>
    %reinterpret_cast_26 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 8], strides: [8, 1] : memref<128xf32> to memref<16x8xf32>
    memref.copy %subview_25, %reinterpret_cast_26 : memref<16x8xf32, strided<[16, 1], offset: 8>> to memref<16x8xf32>
    %intptr_27 = memref.extract_aligned_pointer_as_index %reinterpret_cast_26 : memref<16x8xf32> -> index
    %20 = arith.index_cast %intptr_27 : index to i64
    %21 = llvm.inttoptr %20 : i64 to !llvm.ptr
    %c8_i32_28 = arith.constant 8 : i32
    %c16_i32_29 = arith.constant 16 : i32
    %c8_i32_30 = arith.constant 8 : i32
    %c0_i32_31 = arith.constant 0 : i32
    %c128_i32 = arith.constant 128 : i32
    %c125_i32_32 = arith.constant 125 : i32
    %22 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c8_i32_28, %c16_i32_29, %c8_i32_30, %19, %21, %17, %c0_i32_31, %c128_i32, %c125_i32_32) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    %subview_33 = memref.subview %1[0, 16] [8, 15] [1, 1] : memref<16x31xf32> to memref<8x15xf32, strided<[31, 1], offset: 16>>
    %reinterpret_cast_34 = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 15], strides: [15, 1] : memref<128xf32> to memref<8x15xf32>
    memref.copy %subview_33, %reinterpret_cast_34 : memref<8x15xf32, strided<[31, 1], offset: 16>> to memref<8x15xf32>
    %intptr_35 = memref.extract_aligned_pointer_as_index %reinterpret_cast_34 : memref<8x15xf32> -> index
    %23 = arith.index_cast %intptr_35 : index to i64
    %24 = llvm.inttoptr %23 : i64 to !llvm.ptr
    %subview_36 = memref.subview %0[16, 8] [15, 8] [1, 1] : memref<31x16xf32> to memref<15x8xf32, strided<[16, 1], offset: 264>>
    %reinterpret_cast_37 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [15, 8], strides: [8, 1] : memref<128xf32> to memref<15x8xf32>
    memref.copy %subview_36, %reinterpret_cast_37 : memref<15x8xf32, strided<[16, 1], offset: 264>> to memref<15x8xf32>
    %intptr_38 = memref.extract_aligned_pointer_as_index %reinterpret_cast_37 : memref<15x8xf32> -> index
    %25 = arith.index_cast %intptr_38 : index to i64
    %26 = llvm.inttoptr %25 : i64 to !llvm.ptr
    %c8_i32_39 = arith.constant 8 : i32
    %c15_i32_40 = arith.constant 15 : i32
    %c8_i32_41 = arith.constant 8 : i32
    %c0_i32_42 = arith.constant 0 : i32
    %c197_i32 = arith.constant 197 : i32
    %c124_i32_43 = arith.constant 124 : i32
    %27 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c8_i32_39, %c15_i32_40, %c8_i32_41, %24, %26, %17, %c0_i32_42, %c197_i32, %c124_i32_43) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast_20, %subview_19 : memref<8x8xf32> to memref<8x8xf32, strided<[16, 1], offset: 8>>
    %28 = bufferization.to_tensor %2 restrict writable : memref<16x16xf32> to tensor<16x16xf32>
    %subview_44 = memref.subview %2[8, 0] [8, 8] [1, 1] : memref<16x16xf32> to memref<8x8xf32, strided<[16, 1], offset: 128>>
    %reinterpret_cast_45 = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [8, 8], strides: [8, 1] : memref<64xf32> to memref<8x8xf32>
    memref.copy %subview_44, %reinterpret_cast_45 : memref<8x8xf32, strided<[16, 1], offset: 128>> to memref<8x8xf32>
    %intptr_46 = memref.extract_aligned_pointer_as_index %reinterpret_cast_45 : memref<8x8xf32> -> index
    %29 = arith.index_cast %intptr_46 : index to i64
    %30 = llvm.inttoptr %29 : i64 to !llvm.ptr
    %subview_47 = memref.subview %1[8, 0] [8, 16] [1, 1] : memref<16x31xf32> to memref<8x16xf32, strided<[31, 1], offset: 248>>
    %reinterpret_cast_48 = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 16], strides: [16, 1] : memref<128xf32> to memref<8x16xf32>
    memref.copy %subview_47, %reinterpret_cast_48 : memref<8x16xf32, strided<[31, 1], offset: 248>> to memref<8x16xf32>
    %intptr_49 = memref.extract_aligned_pointer_as_index %reinterpret_cast_48 : memref<8x16xf32> -> index
    %31 = arith.index_cast %intptr_49 : index to i64
    %32 = llvm.inttoptr %31 : i64 to !llvm.ptr
    %subview_50 = memref.subview %0[0, 0] [16, 8] [1, 1] : memref<31x16xf32> to memref<16x8xf32, strided<[16, 1]>>
    %reinterpret_cast_51 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 8], strides: [8, 1] : memref<128xf32> to memref<16x8xf32>
    memref.copy %subview_50, %reinterpret_cast_51 : memref<16x8xf32, strided<[16, 1]>> to memref<16x8xf32>
    %intptr_52 = memref.extract_aligned_pointer_as_index %reinterpret_cast_51 : memref<16x8xf32> -> index
    %33 = arith.index_cast %intptr_52 : index to i64
    %34 = llvm.inttoptr %33 : i64 to !llvm.ptr
    %c8_i32_53 = arith.constant 8 : i32
    %c16_i32_54 = arith.constant 16 : i32
    %c8_i32_55 = arith.constant 8 : i32
    %c0_i32_56 = arith.constant 0 : i32
    %c266_i32 = arith.constant 266 : i32
    %c125_i32_57 = arith.constant 125 : i32
    %35 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c8_i32_53, %c16_i32_54, %c8_i32_55, %32, %34, %30, %c0_i32_56, %c266_i32, %c125_i32_57) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    %subview_58 = memref.subview %1[8, 16] [8, 15] [1, 1] : memref<16x31xf32> to memref<8x15xf32, strided<[31, 1], offset: 264>>
    %reinterpret_cast_59 = memref.reinterpret_cast %alloca to offset: [0], sizes: [8, 15], strides: [15, 1] : memref<128xf32> to memref<8x15xf32>
    memref.copy %subview_58, %reinterpret_cast_59 : memref<8x15xf32, strided<[31, 1], offset: 264>> to memref<8x15xf32>
    %intptr_60 = memref.extract_aligned_pointer_as_index %reinterpret_cast_59 : memref<8x15xf32> -> index
    %36 = arith.index_cast %intptr_60 : index to i64
    %37 = llvm.inttoptr %36 : i64 to !llvm.ptr
    %subview_61 = memref.subview %0[16, 0] [15, 8] [1, 1] : memref<31x16xf32> to memref<15x8xf32, strided<[16, 1], offset: 256>>
    %reinterpret_cast_62 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [15, 8], strides: [8, 1] : memref<128xf32> to memref<15x8xf32>
    memref.copy %subview_61, %reinterpret_cast_62 : memref<15x8xf32, strided<[16, 1], offset: 256>> to memref<15x8xf32>
    %intptr_63 = memref.extract_aligned_pointer_as_index %reinterpret_cast_62 : memref<15x8xf32> -> index
    %38 = arith.index_cast %intptr_63 : index to i64
    %39 = llvm.inttoptr %38 : i64 to !llvm.ptr
    %c8_i32_64 = arith.constant 8 : i32
    %c15_i32_65 = arith.constant 15 : i32
    %c8_i32_66 = arith.constant 8 : i32
    %c0_i32_67 = arith.constant 0 : i32
    %c335_i32 = arith.constant 335 : i32
    %c124_i32_68 = arith.constant 124 : i32
    %40 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c8_i32_64, %c15_i32_65, %c8_i32_66, %37, %39, %30, %c0_i32_67, %c335_i32, %c124_i32_68) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast_45, %subview_44 : memref<8x8xf32> to memref<8x8xf32, strided<[16, 1], offset: 128>>
    %41 = bufferization.to_tensor %2 restrict writable : memref<16x16xf32> to tensor<16x16xf32>
    %subview_69 = memref.subview %2[8, 8] [4, 4] [1, 1] : memref<16x16xf32> to memref<4x4xf32, strided<[16, 1], offset: 136>>
    %reinterpret_cast_70 = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<64xf32> to memref<4x4xf32>
    memref.copy %subview_69, %reinterpret_cast_70 : memref<4x4xf32, strided<[16, 1], offset: 136>> to memref<4x4xf32>
    %intptr_71 = memref.extract_aligned_pointer_as_index %reinterpret_cast_70 : memref<4x4xf32> -> index
    %42 = arith.index_cast %intptr_71 : index to i64
    %43 = llvm.inttoptr %42 : i64 to !llvm.ptr
    %subview_72 = memref.subview %1[8, 0] [4, 16] [1, 1] : memref<16x31xf32> to memref<4x16xf32, strided<[31, 1], offset: 248>>
    %reinterpret_cast_73 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_72, %reinterpret_cast_73 : memref<4x16xf32, strided<[31, 1], offset: 248>> to memref<4x16xf32>
    %intptr_74 = memref.extract_aligned_pointer_as_index %reinterpret_cast_73 : memref<4x16xf32> -> index
    %44 = arith.index_cast %intptr_74 : index to i64
    %45 = llvm.inttoptr %44 : i64 to !llvm.ptr
    %subview_75 = memref.subview %0[0, 8] [16, 4] [1, 1] : memref<31x16xf32> to memref<16x4xf32, strided<[16, 1], offset: 8>>
    %reinterpret_cast_76 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_75, %reinterpret_cast_76 : memref<16x4xf32, strided<[16, 1], offset: 8>> to memref<16x4xf32>
    %intptr_77 = memref.extract_aligned_pointer_as_index %reinterpret_cast_76 : memref<16x4xf32> -> index
    %46 = arith.index_cast %intptr_77 : index to i64
    %47 = llvm.inttoptr %46 : i64 to !llvm.ptr
    %c4_i32 = arith.constant 4 : i32
    %c16_i32_78 = arith.constant 16 : i32
    %c4_i32_79 = arith.constant 4 : i32
    %c1_i32 = arith.constant 1 : i32
    %c0_i32_80 = arith.constant 0 : i32
    %c117_i32 = arith.constant 117 : i32
    %48 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32, %c16_i32_78, %c4_i32_79, %45, %47, %43, %c1_i32, %c0_i32_80, %c117_i32) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    %subview_81 = memref.subview %1[8, 16] [4, 15] [1, 1] : memref<16x31xf32> to memref<4x15xf32, strided<[31, 1], offset: 264>>
    %reinterpret_cast_82 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 15], strides: [15, 1] : memref<128xf32> to memref<4x15xf32>
    memref.copy %subview_81, %reinterpret_cast_82 : memref<4x15xf32, strided<[31, 1], offset: 264>> to memref<4x15xf32>
    %intptr_83 = memref.extract_aligned_pointer_as_index %reinterpret_cast_82 : memref<4x15xf32> -> index
    %49 = arith.index_cast %intptr_83 : index to i64
    %50 = llvm.inttoptr %49 : i64 to !llvm.ptr
    %subview_84 = memref.subview %0[16, 8] [15, 4] [1, 1] : memref<31x16xf32> to memref<15x4xf32, strided<[16, 1], offset: 264>>
    %reinterpret_cast_85 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [15, 4], strides: [4, 1] : memref<128xf32> to memref<15x4xf32>
    memref.copy %subview_84, %reinterpret_cast_85 : memref<15x4xf32, strided<[16, 1], offset: 264>> to memref<15x4xf32>
    %intptr_86 = memref.extract_aligned_pointer_as_index %reinterpret_cast_85 : memref<15x4xf32> -> index
    %51 = arith.index_cast %intptr_86 : index to i64
    %52 = llvm.inttoptr %51 : i64 to !llvm.ptr
    %c4_i32_87 = arith.constant 4 : i32
    %c15_i32_88 = arith.constant 15 : i32
    %c4_i32_89 = arith.constant 4 : i32
    %c1_i32_90 = arith.constant 1 : i32
    %c42_i32_91 = arith.constant 42 : i32
    %c116_i32 = arith.constant 116 : i32
    %53 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_87, %c15_i32_88, %c4_i32_89, %50, %52, %43, %c1_i32_90, %c42_i32_91, %c116_i32) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast_70, %subview_69 : memref<4x4xf32> to memref<4x4xf32, strided<[16, 1], offset: 136>>
    %54 = bufferization.to_tensor %2 restrict writable : memref<16x16xf32> to tensor<16x16xf32>
    %subview_92 = memref.subview %2[8, 12] [4, 4] [1, 1] : memref<16x16xf32> to memref<4x4xf32, strided<[16, 1], offset: 140>>
    %reinterpret_cast_93 = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<64xf32> to memref<4x4xf32>
    memref.copy %subview_92, %reinterpret_cast_93 : memref<4x4xf32, strided<[16, 1], offset: 140>> to memref<4x4xf32>
    %intptr_94 = memref.extract_aligned_pointer_as_index %reinterpret_cast_93 : memref<4x4xf32> -> index
    %55 = arith.index_cast %intptr_94 : index to i64
    %56 = llvm.inttoptr %55 : i64 to !llvm.ptr
    %subview_95 = memref.subview %1[8, 0] [4, 16] [1, 1] : memref<16x31xf32> to memref<4x16xf32, strided<[31, 1], offset: 248>>
    %reinterpret_cast_96 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_95, %reinterpret_cast_96 : memref<4x16xf32, strided<[31, 1], offset: 248>> to memref<4x16xf32>
    %intptr_97 = memref.extract_aligned_pointer_as_index %reinterpret_cast_96 : memref<4x16xf32> -> index
    %57 = arith.index_cast %intptr_97 : index to i64
    %58 = llvm.inttoptr %57 : i64 to !llvm.ptr
    %subview_98 = memref.subview %0[0, 12] [16, 4] [1, 1] : memref<31x16xf32> to memref<16x4xf32, strided<[16, 1], offset: 12>>
    %reinterpret_cast_99 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_98, %reinterpret_cast_99 : memref<16x4xf32, strided<[16, 1], offset: 12>> to memref<16x4xf32>
    %intptr_100 = memref.extract_aligned_pointer_as_index %reinterpret_cast_99 : memref<16x4xf32> -> index
    %59 = arith.index_cast %intptr_100 : index to i64
    %60 = llvm.inttoptr %59 : i64 to !llvm.ptr
    %c4_i32_101 = arith.constant 4 : i32
    %c16_i32_102 = arith.constant 16 : i32
    %c4_i32_103 = arith.constant 4 : i32
    %c2_i32 = arith.constant 2 : i32
    %c0_i32_104 = arith.constant 0 : i32
    %c117_i32_105 = arith.constant 117 : i32
    %61 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_101, %c16_i32_102, %c4_i32_103, %58, %60, %56, %c2_i32, %c0_i32_104, %c117_i32_105) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    %subview_106 = memref.subview %1[8, 16] [4, 15] [1, 1] : memref<16x31xf32> to memref<4x15xf32, strided<[31, 1], offset: 264>>
    %reinterpret_cast_107 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 15], strides: [15, 1] : memref<128xf32> to memref<4x15xf32>
    memref.copy %subview_106, %reinterpret_cast_107 : memref<4x15xf32, strided<[31, 1], offset: 264>> to memref<4x15xf32>
    %intptr_108 = memref.extract_aligned_pointer_as_index %reinterpret_cast_107 : memref<4x15xf32> -> index
    %62 = arith.index_cast %intptr_108 : index to i64
    %63 = llvm.inttoptr %62 : i64 to !llvm.ptr
    %subview_109 = memref.subview %0[16, 12] [15, 4] [1, 1] : memref<31x16xf32> to memref<15x4xf32, strided<[16, 1], offset: 268>>
    %reinterpret_cast_110 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [15, 4], strides: [4, 1] : memref<128xf32> to memref<15x4xf32>
    memref.copy %subview_109, %reinterpret_cast_110 : memref<15x4xf32, strided<[16, 1], offset: 268>> to memref<15x4xf32>
    %intptr_111 = memref.extract_aligned_pointer_as_index %reinterpret_cast_110 : memref<15x4xf32> -> index
    %64 = arith.index_cast %intptr_111 : index to i64
    %65 = llvm.inttoptr %64 : i64 to !llvm.ptr
    %c4_i32_112 = arith.constant 4 : i32
    %c15_i32_113 = arith.constant 15 : i32
    %c4_i32_114 = arith.constant 4 : i32
    %c2_i32_115 = arith.constant 2 : i32
    %c42_i32_116 = arith.constant 42 : i32
    %c116_i32_117 = arith.constant 116 : i32
    %66 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_112, %c15_i32_113, %c4_i32_114, %63, %65, %56, %c2_i32_115, %c42_i32_116, %c116_i32_117) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast_93, %subview_92 : memref<4x4xf32> to memref<4x4xf32, strided<[16, 1], offset: 140>>
    %67 = bufferization.to_tensor %2 restrict writable : memref<16x16xf32> to tensor<16x16xf32>
    %subview_118 = memref.subview %2[12, 8] [4, 4] [1, 1] : memref<16x16xf32> to memref<4x4xf32, strided<[16, 1], offset: 200>>
    %reinterpret_cast_119 = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<64xf32> to memref<4x4xf32>
    memref.copy %subview_118, %reinterpret_cast_119 : memref<4x4xf32, strided<[16, 1], offset: 200>> to memref<4x4xf32>
    %intptr_120 = memref.extract_aligned_pointer_as_index %reinterpret_cast_119 : memref<4x4xf32> -> index
    %68 = arith.index_cast %intptr_120 : index to i64
    %69 = llvm.inttoptr %68 : i64 to !llvm.ptr
    %subview_121 = memref.subview %1[12, 0] [4, 16] [1, 1] : memref<16x31xf32> to memref<4x16xf32, strided<[31, 1], offset: 372>>
    %reinterpret_cast_122 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_121, %reinterpret_cast_122 : memref<4x16xf32, strided<[31, 1], offset: 372>> to memref<4x16xf32>
    %intptr_123 = memref.extract_aligned_pointer_as_index %reinterpret_cast_122 : memref<4x16xf32> -> index
    %70 = arith.index_cast %intptr_123 : index to i64
    %71 = llvm.inttoptr %70 : i64 to !llvm.ptr
    %subview_124 = memref.subview %0[0, 8] [16, 4] [1, 1] : memref<31x16xf32> to memref<16x4xf32, strided<[16, 1], offset: 8>>
    %reinterpret_cast_125 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_124, %reinterpret_cast_125 : memref<16x4xf32, strided<[16, 1], offset: 8>> to memref<16x4xf32>
    %intptr_126 = memref.extract_aligned_pointer_as_index %reinterpret_cast_125 : memref<16x4xf32> -> index
    %72 = arith.index_cast %intptr_126 : index to i64
    %73 = llvm.inttoptr %72 : i64 to !llvm.ptr
    %c4_i32_127 = arith.constant 4 : i32
    %c16_i32_128 = arith.constant 16 : i32
    %c4_i32_129 = arith.constant 4 : i32
    %c3_i32 = arith.constant 3 : i32
    %c0_i32_130 = arith.constant 0 : i32
    %c117_i32_131 = arith.constant 117 : i32
    %74 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_127, %c16_i32_128, %c4_i32_129, %71, %73, %69, %c3_i32, %c0_i32_130, %c117_i32_131) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    %subview_132 = memref.subview %1[12, 16] [4, 15] [1, 1] : memref<16x31xf32> to memref<4x15xf32, strided<[31, 1], offset: 388>>
    %reinterpret_cast_133 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 15], strides: [15, 1] : memref<128xf32> to memref<4x15xf32>
    memref.copy %subview_132, %reinterpret_cast_133 : memref<4x15xf32, strided<[31, 1], offset: 388>> to memref<4x15xf32>
    %intptr_134 = memref.extract_aligned_pointer_as_index %reinterpret_cast_133 : memref<4x15xf32> -> index
    %75 = arith.index_cast %intptr_134 : index to i64
    %76 = llvm.inttoptr %75 : i64 to !llvm.ptr
    %subview_135 = memref.subview %0[16, 8] [15, 4] [1, 1] : memref<31x16xf32> to memref<15x4xf32, strided<[16, 1], offset: 264>>
    %reinterpret_cast_136 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [15, 4], strides: [4, 1] : memref<128xf32> to memref<15x4xf32>
    memref.copy %subview_135, %reinterpret_cast_136 : memref<15x4xf32, strided<[16, 1], offset: 264>> to memref<15x4xf32>
    %intptr_137 = memref.extract_aligned_pointer_as_index %reinterpret_cast_136 : memref<15x4xf32> -> index
    %77 = arith.index_cast %intptr_137 : index to i64
    %78 = llvm.inttoptr %77 : i64 to !llvm.ptr
    %c4_i32_138 = arith.constant 4 : i32
    %c15_i32_139 = arith.constant 15 : i32
    %c4_i32_140 = arith.constant 4 : i32
    %c3_i32_141 = arith.constant 3 : i32
    %c42_i32_142 = arith.constant 42 : i32
    %c116_i32_143 = arith.constant 116 : i32
    %79 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_138, %c15_i32_139, %c4_i32_140, %76, %78, %69, %c3_i32_141, %c42_i32_142, %c116_i32_143) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast_119, %subview_118 : memref<4x4xf32> to memref<4x4xf32, strided<[16, 1], offset: 200>>
    %80 = bufferization.to_tensor %2 restrict writable : memref<16x16xf32> to tensor<16x16xf32>
    %subview_144 = memref.subview %2[12, 12] [4, 4] [1, 1] : memref<16x16xf32> to memref<4x4xf32, strided<[16, 1], offset: 204>>
    %reinterpret_cast_145 = memref.reinterpret_cast %alloca_1 to offset: [0], sizes: [4, 4], strides: [4, 1] : memref<64xf32> to memref<4x4xf32>
    memref.copy %subview_144, %reinterpret_cast_145 : memref<4x4xf32, strided<[16, 1], offset: 204>> to memref<4x4xf32>
    %intptr_146 = memref.extract_aligned_pointer_as_index %reinterpret_cast_145 : memref<4x4xf32> -> index
    %81 = arith.index_cast %intptr_146 : index to i64
    %82 = llvm.inttoptr %81 : i64 to !llvm.ptr
    %subview_147 = memref.subview %1[12, 0] [4, 16] [1, 1] : memref<16x31xf32> to memref<4x16xf32, strided<[31, 1], offset: 372>>
    %reinterpret_cast_148 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 16], strides: [16, 1] : memref<128xf32> to memref<4x16xf32>
    memref.copy %subview_147, %reinterpret_cast_148 : memref<4x16xf32, strided<[31, 1], offset: 372>> to memref<4x16xf32>
    %intptr_149 = memref.extract_aligned_pointer_as_index %reinterpret_cast_148 : memref<4x16xf32> -> index
    %83 = arith.index_cast %intptr_149 : index to i64
    %84 = llvm.inttoptr %83 : i64 to !llvm.ptr
    %subview_150 = memref.subview %0[0, 12] [16, 4] [1, 1] : memref<31x16xf32> to memref<16x4xf32, strided<[16, 1], offset: 12>>
    %reinterpret_cast_151 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [16, 4], strides: [4, 1] : memref<128xf32> to memref<16x4xf32>
    memref.copy %subview_150, %reinterpret_cast_151 : memref<16x4xf32, strided<[16, 1], offset: 12>> to memref<16x4xf32>
    %intptr_152 = memref.extract_aligned_pointer_as_index %reinterpret_cast_151 : memref<16x4xf32> -> index
    %85 = arith.index_cast %intptr_152 : index to i64
    %86 = llvm.inttoptr %85 : i64 to !llvm.ptr
    %c4_i32_153 = arith.constant 4 : i32
    %c16_i32_154 = arith.constant 16 : i32
    %c4_i32_155 = arith.constant 4 : i32
    %c1_i32_156 = arith.constant 1 : i32
    %c120_i32 = arith.constant 120 : i32
    %c117_i32_157 = arith.constant 117 : i32
    %87 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_153, %c16_i32_154, %c4_i32_155, %84, %86, %82, %c1_i32_156, %c120_i32, %c117_i32_157) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    %subview_158 = memref.subview %1[12, 16] [4, 15] [1, 1] : memref<16x31xf32> to memref<4x15xf32, strided<[31, 1], offset: 388>>
    %reinterpret_cast_159 = memref.reinterpret_cast %alloca to offset: [0], sizes: [4, 15], strides: [15, 1] : memref<128xf32> to memref<4x15xf32>
    memref.copy %subview_158, %reinterpret_cast_159 : memref<4x15xf32, strided<[31, 1], offset: 388>> to memref<4x15xf32>
    %intptr_160 = memref.extract_aligned_pointer_as_index %reinterpret_cast_159 : memref<4x15xf32> -> index
    %88 = arith.index_cast %intptr_160 : index to i64
    %89 = llvm.inttoptr %88 : i64 to !llvm.ptr
    %subview_161 = memref.subview %0[16, 12] [15, 4] [1, 1] : memref<31x16xf32> to memref<15x4xf32, strided<[16, 1], offset: 268>>
    %reinterpret_cast_162 = memref.reinterpret_cast %alloca_0 to offset: [0], sizes: [15, 4], strides: [4, 1] : memref<128xf32> to memref<15x4xf32>
    memref.copy %subview_161, %reinterpret_cast_162 : memref<15x4xf32, strided<[16, 1], offset: 268>> to memref<15x4xf32>
    %intptr_163 = memref.extract_aligned_pointer_as_index %reinterpret_cast_162 : memref<15x4xf32> -> index
    %90 = arith.index_cast %intptr_163 : index to i64
    %91 = llvm.inttoptr %90 : i64 to !llvm.ptr
    %c4_i32_164 = arith.constant 4 : i32
    %c15_i32_165 = arith.constant 15 : i32
    %c4_i32_166 = arith.constant 4 : i32
    %c1_i32_167 = arith.constant 1 : i32
    %c189_i32 = arith.constant 189 : i32
    %c116_i32_168 = arith.constant 116 : i32
    %92 = llvm.call @fpga_matmul_tiled_auto_scheduled(%c4_i32_164, %c15_i32_165, %c4_i32_166, %89, %91, %82, %c1_i32_167, %c189_i32, %c116_i32_168) : (i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
    memref.copy %reinterpret_cast_145, %subview_144 : memref<4x4xf32> to memref<4x4xf32, strided<[16, 1], offset: 204>>
    %93 = bufferization.to_tensor %2 restrict writable : memref<16x16xf32> to tensor<16x16xf32>
    return %93 : tensor<16x16xf32>
  }
}

