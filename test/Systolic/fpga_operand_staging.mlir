// RUN: systolic-opt --systolic-binary-search-tile-matmul --lower-systolic-matmul-tile-to-fpga %s \
// RUN:   | FileCheck %s --implicit-check-not=tensor.extract_slice --implicit-check-not=tensor.insert_slice
// RUN: systolic-opt --systolic-fpga-pipeline %s | FileCheck %s --check-prefix=PIPE

// How --lower-systolic-matmul-tile-to-fpga hands tiles to the runtime, which
// takes contiguous row-major m x k, k x n and m x n tiles. 8x4x20 is two 4x4
// output blocks (devices 1 and 2), each folded into k = 16 and k = 4:
//
// - A and B are each read through one buffer for the whole function; every
//   tile is copied from it into a scratch tile of its own;
// - each output block of C is copied into a scratch tile once, accumulated
//   there by both of its invocations, and copied back; the next block reads
//   the C that the previous one produced.
//
// No tensor slice is left for bufferization, so its cost stays linear in the
// number of invocations instead of growing with the cube of the blocks.

module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95

  // CHECK-LABEL: func.func @mm_8x4x20
  // CHECK-SAME: (%[[A:.*]]: tensor<8x20xf32>, %[[B:.*]]: tensor<20x4xf32>, %[[C:.*]]: tensor<8x4xf32>)
  // CHECK-DAG: %[[BB:.*]] = bufferization.to_buffer %[[B]] read_only
  // CHECK-DAG: %[[AB:.*]] = bufferization.to_buffer %[[A]] read_only
  // CHECK-DAG: %[[SA:.*]] = memref.alloca() : memref<128xf32>
  // CHECK-DAG: %[[SB:.*]] = memref.alloca() : memref<128xf32>
  // CHECK-DAG: %[[SC:.*]] = memref.alloca() : memref<64xf32>

  // Block C[0:4, 0:4] on device 1.
  // CHECK: %[[CM:.*]] = bufferization.to_buffer %[[C]] :
  // CHECK: %[[CV:.*]] = memref.subview %[[CM]][0, 0] [4, 4] [1, 1]
  // CHECK: %[[CT:.*]] = memref.reinterpret_cast %[[SC]] to offset: [0], sizes: [4, 4], strides: [4, 1]
  // CHECK: memref.copy %[[CV]], %[[CT]]
  // CHECK: %[[AV:.*]] = memref.subview %[[AB]][0, 0] [4, 16] [1, 1]
  // CHECK: %[[AT:.*]] = memref.reinterpret_cast %[[SA]] to offset: [0], sizes: [4, 16], strides: [16, 1]
  // CHECK: memref.copy %[[AV]], %[[AT]]
  // CHECK: %[[BV:.*]] = memref.subview %[[BB]][0, 0] [16, 4] [1, 1]
  // CHECK: %[[BT:.*]] = memref.reinterpret_cast %[[SB]] to offset: [0], sizes: [16, 4], strides: [4, 1]
  // CHECK: memref.copy %[[BV]], %[[BT]]
  // CHECK: llvm.call @fpga_matmul_tiled_auto_scheduled
  // CHECK: memref.subview %[[AB]][0, 16] [4, 4] [1, 1]
  // CHECK: memref.subview %[[BB]][16, 0] [4, 4] [1, 1]
  // CHECK: llvm.call @fpga_matmul_tiled_auto_scheduled
  // CHECK: memref.copy %[[CT]], %[[CV]]
  // CHECK: %[[R1:.*]] = bufferization.to_tensor %[[CM]] restrict writable

  // Block C[4:8, 0:4] on device 2, on top of the first block's result.
  // CHECK: %[[CM2:.*]] = bufferization.to_buffer %[[R1]] :
  // CHECK: memref.subview %[[CM2]][4, 0] [4, 4] [1, 1]
  // CHECK-COUNT-2: llvm.call @fpga_matmul_tiled_auto_scheduled
  // CHECK: %[[R2:.*]] = bufferization.to_tensor %[[CM2]] restrict writable
  // CHECK: return %[[R2]]
  func.func @mm_8x4x20(%A: tensor<8x20xf32>, %B: tensor<20x4xf32>,
                       %C: tensor<8x4xf32>) -> tensor<8x4xf32>
      attributes {llvm.emit_c_interface} {
    %0 = linalg.matmul ins(%A, %B : tensor<8x20xf32>, tensor<20x4xf32>)
                       outs(%C : tensor<8x4xf32>) -> tensor<8x4xf32>
    return %0 : tensor<8x4xf32>
  }
}

// After bufferization the invocations allocate nothing on the heap: the one
// malloc is the result the function returns.
// PIPE-COUNT-4: llvm.call @fpga_matmul_tiled_auto_scheduled
// PIPE-COUNT-1: llvm.call @malloc
// PIPE-NOT: llvm.call @malloc
