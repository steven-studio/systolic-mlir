// RUN: systolic-opt --systolic-binary-search-tile-matmul --lower-systolic-matmul-tile-to-fpga="async=true" %s \
// RUN:   | FileCheck %s --implicit-check-not=tensor.extract_slice --implicit-check-not=tensor.insert_slice --implicit-check-not=fpga_matmul_tiled_auto_scheduled
// RUN: systolic-opt --systolic-fpga-pipeline="async=true" %s | FileCheck %s --check-prefix=PIPE

// --lower-systolic-matmul-tile-to-fpga with async=true: the asynchronous
// runtime (runtime/fpga_async_runtime.h, docs/ASYNC.md). One
// fpga_gemm_begin per GEMM before its first tile, one fpga_tile_submit per
// tile with the tile's place in the GEMM (systolic.tile / k_index / k_count
// from the binary-search pass) and a pointer straight into the output
// buffer with its row stride -- C is never staged -- and one fpga_gemm_wait
// before the first op that reads the GEMM's result, so that
//   - a function with two independent GEMMs submits both before waiting for
//     either (they overlap on the FPGA), and
//   - a GEMM that reads another GEMM's result waits for it first.
// A and B are staged into scratch tiles as in the synchronous lowering: the
// runtime packs them before fpga_tile_submit returns.

module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95

  // 8x4x20: two 4x4 output blocks on devices 1 and 2, each k = 16 then 4.
  // CHECK-LABEL: func.func @mm_8x4x20
  // CHECK-SAME: (%[[A:.*]]: tensor<8x20xf32>, %[[B:.*]]: tensor<20x4xf32>, %[[C:.*]]: tensor<8x4xf32>)
  // CHECK-DAG: bufferization.to_buffer %[[B]] read_only
  // CHECK-DAG: bufferization.to_buffer %[[A]] read_only
  // CHECK-DAG: memref.alloca() : memref<128xf32>
  // CHECK-DAG: memref.alloca() : memref<128xf32>
  // CHECK-NOT: memref.alloca() : memref<64xf32>

  // Block C[0:4, 0:4]: a pointer to element 0 of C's buffer, the GEMM
  // opened, two submits with ldc = 4 on device 1.
  // CHECK: %[[CM:.*]] = bufferization.to_buffer %[[C]] :
  // CHECK: %[[P0:.*]] = memref.extract_aligned_pointer_as_index %[[CM]]
  // CHECK: %[[I0:.*]] = arith.index_cast %[[P0]] : index to i64
  // CHECK: %[[CP0:.*]] = llvm.inttoptr %[[I0]]
  // CHECK: %[[GM:.*]] = arith.constant 8 : i32
  // CHECK-NEXT: %[[GN:.*]] = arith.constant 4 : i32
  // CHECK-NEXT: %[[GK:.*]] = arith.constant 20 : i32
  // CHECK-NEXT: %[[GT:.*]] = arith.constant 4 : i32
  // CHECK-NEXT: %[[G:.*]] = llvm.call @fpga_gemm_begin(%[[GM]], %[[GN]], %[[GK]], %[[GT]])
  // CHECK: memref.copy
  // CHECK: memref.copy
  // CHECK: %[[T0:.*]] = arith.constant 0 : i32
  // CHECK-NEXT: %[[KI0:.*]] = arith.constant 0 : i32
  // CHECK-NEXT: %[[KC0:.*]] = arith.constant 2 : i32
  // CHECK-NEXT: %[[M0:.*]] = arith.constant 4 : i32
  // CHECK-NEXT: %[[K0:.*]] = arith.constant 16 : i32
  // CHECK-NEXT: %[[N0:.*]] = arith.constant 4 : i32
  // CHECK-NEXT: %[[LD0:.*]] = arith.constant 4 : i32
  // CHECK-NEXT: %[[D0:.*]] = arith.constant 1 : i32
  // CHECK-NEXT: %[[S0:.*]] = arith.constant {{[0-9]+}} : i32
  // CHECK-NEXT: %[[E0:.*]] = arith.constant {{[0-9]+}} : i32
  // CHECK-NEXT: llvm.call @fpga_tile_submit(%[[G]], %[[T0]], %[[KI0]], %[[KC0]], %[[M0]], %[[K0]], %[[N0]], %{{.*}}, %{{.*}}, %[[CP0]], %[[LD0]], %[[D0]], %[[S0]], %[[E0]])
  // CHECK: arith.constant 0 : i32
  // CHECK-NEXT: %[[KI1:.*]] = arith.constant 1 : i32
  // CHECK-NEXT: arith.constant 2 : i32
  // CHECK-NEXT: arith.constant 4 : i32
  // CHECK-NEXT: %[[K1:.*]] = arith.constant 4 : i32
  // CHECK: llvm.call @fpga_tile_submit(%[[G]], %{{.*}}, %[[KI1]], %{{.*}}, %{{.*}}, %[[K1]], %{{.*}}, %{{.*}}, %{{.*}}, %[[CP0]],
  // CHECK: %[[R1:.*]] = bufferization.to_tensor %[[CM]] restrict writable

  // Block C[4:8, 0:4]: element 16 of the same buffer (64 bytes in; the
  // previous block's to_tensor stands for it, so no to_buffer of that),
  // device 2; no copy of C anywhere.
  // CHECK: %[[P2:.*]] = memref.extract_aligned_pointer_as_index %[[CM]]
  // CHECK-NEXT: %[[OFF2:.*]] = arith.constant 64 : index
  // CHECK-NEXT: %[[PA2:.*]] = arith.addi %[[P2]], %[[OFF2]]
  // CHECK-NEXT: %[[I2:.*]] = arith.index_cast %[[PA2]] : index to i64
  // CHECK-NEXT: %[[CP2:.*]] = llvm.inttoptr %[[I2]]
  // CHECK: llvm.call @fpga_tile_submit(%[[G]], %{{.*}}, %{{.*}}, %{{.*}}, %{{.*}}, %{{.*}}, %{{.*}}, %{{.*}}, %{{.*}}, %[[CP2]], %{{.*}}, %{{.*}},
  // CHECK: llvm.call @fpga_tile_submit(%[[G]], %{{.*}}, %{{.*}}, %{{.*}}, %{{.*}}, %{{.*}}, %{{.*}}, %{{.*}}, %{{.*}}, %[[CP2]], %{{.*}}, %{{.*}},
  // CHECK: %[[R2:.*]] = bufferization.to_tensor %[[CM]] restrict writable
  // CHECK-NEXT: llvm.call @fpga_gemm_wait(%[[G]])
  // CHECK-NEXT: return %[[R2]]
  func.func @mm_8x4x20(%A: tensor<8x20xf32>, %B: tensor<20x4xf32>,
                       %C: tensor<8x4xf32>) -> tensor<8x4xf32>
      attributes {llvm.emit_c_interface} {
    %0 = linalg.matmul ins(%A, %B : tensor<8x20xf32>, tensor<20x4xf32>)
                       outs(%C : tensor<8x4xf32>) -> tensor<8x4xf32>
    return %0 : tensor<8x4xf32>
  }

  // Two independent GEMMs: every submit of both precedes both waits.
  // CHECK-LABEL: func.func @two_independent
  // CHECK: %[[G0:.*]] = llvm.call @fpga_gemm_begin
  // CHECK: llvm.call @fpga_tile_submit(%[[G0]],
  // CHECK: %[[G1:.*]] = llvm.call @fpga_gemm_begin
  // CHECK: llvm.call @fpga_tile_submit(%[[G1]],
  // CHECK-NOT: fpga_gemm_wait
  // CHECK: llvm.call @fpga_gemm_wait(%[[G0]])
  // CHECK-NEXT: llvm.call @fpga_gemm_wait(%[[G1]])
  // CHECK-NEXT: return
  func.func @two_independent(%A: tensor<4x8xf32>, %B: tensor<8x4xf32>,
                             %C: tensor<4x4xf32>, %D: tensor<4x8xf32>,
                             %E: tensor<8x4xf32>, %F: tensor<4x4xf32>)
      -> (tensor<4x4xf32>, tensor<4x4xf32>) attributes {llvm.emit_c_interface} {
    %0 = linalg.matmul ins(%A, %B : tensor<4x8xf32>, tensor<8x4xf32>)
                       outs(%C : tensor<4x4xf32>) -> tensor<4x4xf32>
    %1 = linalg.matmul ins(%D, %E : tensor<4x8xf32>, tensor<8x4xf32>)
                       outs(%F : tensor<4x4xf32>) -> tensor<4x4xf32>
    return %0, %1 : tensor<4x4xf32>, tensor<4x4xf32>
  }

  // The second GEMM reads the first's result as its A: the first is waited
  // for before anything of the second reads it -- the wait lands right
  // before the buffer the second's A tile is staged from (after the
  // second GEMM has been opened, which reads nothing).
  // CHECK-LABEL: func.func @dependent
  // CHECK: %[[H0:.*]] = llvm.call @fpga_gemm_begin
  // CHECK: llvm.call @fpga_tile_submit(%[[H0]],
  // CHECK: %[[Q0:.*]] = bufferization.to_tensor %{{.*}} restrict writable
  // CHECK-NOT: fpga_gemm_wait
  // CHECK: %[[H1:.*]] = llvm.call @fpga_gemm_begin
  // CHECK-NEXT: llvm.call @fpga_gemm_wait(%[[H0]])
  // CHECK-NEXT: bufferization.to_buffer %[[Q0]] read_only
  // CHECK: llvm.call @fpga_tile_submit(%[[H1]],
  // CHECK: llvm.call @fpga_gemm_wait(%[[H1]])
  // CHECK-NEXT: return
  func.func @dependent(%A: tensor<4x8xf32>, %B: tensor<8x4xf32>,
                       %C: tensor<4x4xf32>, %E: tensor<4x4xf32>,
                       %F: tensor<4x4xf32>) -> tensor<4x4xf32>
      attributes {llvm.emit_c_interface} {
    %0 = linalg.matmul ins(%A, %B : tensor<4x8xf32>, tensor<8x4xf32>)
                       outs(%C : tensor<4x4xf32>) -> tensor<4x4xf32>
    %1 = linalg.matmul ins(%0, %E : tensor<4x4xf32>, tensor<4x4xf32>)
                       outs(%F : tensor<4x4xf32>) -> tensor<4x4xf32>
    return %1 : tensor<4x4xf32>
  }
}

// Through the whole pipeline the calls survive bufferization and LLVM
// lowering, and nothing but the returned result is allocated on the heap
// (one malloc in @mm_8x4x20, after the wait; the next function bounds the
// check).
// PIPE-LABEL: llvm.func @mm_8x4x20
// PIPE: llvm.call @fpga_gemm_begin
// PIPE-COUNT-4: llvm.call @fpga_tile_submit
// PIPE: llvm.call @fpga_gemm_wait
// PIPE-COUNT-1: llvm.call @malloc
// PIPE-NOT: llvm.call @malloc
// PIPE-LABEL: llvm.func @two_independent
