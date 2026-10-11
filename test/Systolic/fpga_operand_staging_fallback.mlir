// RUN: systolic-opt --systolic-binary-search-tile-matmul --lower-systolic-matmul-tile-to-fpga %s | FileCheck %s

// fpga_operand_staging.mlir shows output blocks of C staged at the buffer
// level, where the insert_slice becomes a `to_tensor ... restrict` of C's
// buffer. That assertion is only true when nothing else can reach the
// buffer, so the blocks below are lowered as tensors instead (one
// to_buffer of the C tile, the call, a to_tensor, the insert_slice kept)
// and bufferization sees every alias.

module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95

  // C's root is an insert_slice into a tensor.empty, whose buffer other
  // tensors may share in place.
  // CHECK-LABEL: func.func @root_not_argument
  // CHECK: %[[Z:.*]] = tensor.insert_slice %{{.*}} into %{{.*}}[0, 0] [4, 4] [1, 1]
  // CHECK: %[[CS:.*]] = tensor.extract_slice %[[Z]][0, 0] [4, 4] [1, 1]
  // CHECK: %[[CB:.*]] = bufferization.to_buffer %[[CS]] : tensor<4x4xf32> to memref<4x4xf32>
  // CHECK: llvm.call @fpga_matmul_tiled_auto_scheduled
  // CHECK: %[[CT:.*]] = bufferization.to_tensor %[[CB]] restrict writable
  // CHECK: %[[R:.*]] = tensor.insert_slice %[[CT]] into %[[Z]][0, 0] [4, 4] [1, 1]
  // CHECK: return %[[R]]
  func.func @root_not_argument(%A: tensor<4x4xf32>, %B: tensor<4x4xf32>,
                               %C: tensor<4x4xf32>) -> tensor<4x4xf32> {
    %e = tensor.empty() : tensor<4x4xf32>
    %z = tensor.insert_slice %C into %e[0, 0] [4, 4] [1, 1]
           : tensor<4x4xf32> into tensor<4x4xf32>
    %r = linalg.matmul ins(%A, %B : tensor<4x4xf32>, tensor<4x4xf32>)
                       outs(%z : tensor<4x4xf32>) -> tensor<4x4xf32>
    return %r : tensor<4x4xf32>
  }

  // C is loop-carried: the yield must stay equivalent to the iter_arg.
  // CHECK-LABEL: func.func @loop_carried
  // CHECK: scf.for {{.*}} iter_args(%[[ACC:.*]] = %{{.*}})
  // CHECK: %[[CS:.*]] = tensor.extract_slice %[[ACC]][0, 0] [4, 4] [1, 1]
  // CHECK: %[[CB:.*]] = bufferization.to_buffer %[[CS]] : tensor<4x4xf32> to memref<4x4xf32>
  // CHECK: llvm.call @fpga_matmul_tiled_auto_scheduled
  // CHECK: %[[CT:.*]] = bufferization.to_tensor %[[CB]] restrict writable
  // CHECK: %[[R:.*]] = tensor.insert_slice %[[CT]] into %[[ACC]]
  // CHECK: scf.yield %[[R]]
  func.func @loop_carried(%A: tensor<4x4xf32>, %B: tensor<4x4xf32>,
                          %C: tensor<4x4xf32>) -> tensor<4x4xf32> {
    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index
    %c2 = arith.constant 2 : index
    %r = scf.for %i = %c0 to %c2 step %c1 iter_args(%acc = %C) -> (tensor<4x4xf32>) {
      %m = linalg.matmul ins(%A, %B : tensor<4x4xf32>, tensor<4x4xf32>)
                         outs(%acc : tensor<4x4xf32>) -> tensor<4x4xf32>
      scf.yield %m : tensor<4x4xf32>
    }
    return %r : tensor<4x4xf32>
  }

  // Two blocks whose extract .. insert ranges interleave would accumulate
  // in the same C scratch tile at the same time.
  // CHECK-LABEL: func.func @interleaved
  // CHECK-SAME: (%{{.*}}: tensor<4x4xf32>, %{{.*}}: tensor<4x4xf32>, %[[X:.*]]: tensor<8x8xf32>, %[[Y:.*]]: tensor<8x8xf32>)
  // CHECK: %[[CX:.*]] = tensor.extract_slice %[[X]][0, 0] [4, 4] [1, 1]
  // CHECK: %[[CY:.*]] = tensor.extract_slice %[[Y]][4, 4] [4, 4] [1, 1]
  // CHECK: bufferization.to_buffer %[[CX]] : tensor<4x4xf32> to memref<4x4xf32>
  // CHECK: llvm.call @fpga_matmul_tiled_auto_scheduled
  // CHECK: bufferization.to_buffer %[[CY]] : tensor<4x4xf32> to memref<4x4xf32>
  // CHECK: llvm.call @fpga_matmul_tiled_auto_scheduled
  // CHECK: tensor.insert_slice %{{.*}} into %[[X]][0, 0] [4, 4] [1, 1]
  // CHECK: tensor.insert_slice %{{.*}} into %[[Y]][4, 4] [4, 4] [1, 1]
  func.func @interleaved(%A: tensor<4x4xf32>, %B: tensor<4x4xf32>,
                         %X: tensor<8x8xf32>, %Y: tensor<8x8xf32>)
      -> (tensor<8x8xf32>, tensor<8x8xf32>) {
    %cx = tensor.extract_slice %X[0, 0] [4, 4] [1, 1] : tensor<8x8xf32> to tensor<4x4xf32>
    %cy = tensor.extract_slice %Y[4, 4] [4, 4] [1, 1] : tensor<8x8xf32> to tensor<4x4xf32>
    %tx = systolic.matmul_tile %A, %B, %cx m = 4 n = 4 k = 4 on @acc_4x4_0 est_cycles = 101 start_cycle = 0
          : (tensor<4x4xf32>, tensor<4x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %ty = systolic.matmul_tile %A, %B, %cy m = 4 n = 4 k = 4 on @acc_4x4_1 est_cycles = 101 start_cycle = 0
          : (tensor<4x4xf32>, tensor<4x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    %X1 = tensor.insert_slice %tx into %X[0, 0] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<8x8xf32>
    %Y1 = tensor.insert_slice %ty into %Y[4, 4] [4, 4] [1, 1] : tensor<4x4xf32> into tensor<8x8xf32>
    return %X1, %Y1 : tensor<8x8xf32>, tensor<8x8xf32>
  }
}
