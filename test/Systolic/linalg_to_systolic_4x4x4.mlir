// RUN: systolic-opt --systolic-binary-search-tile-matmul %s | FileCheck %s

// Minimal Linalg -> Systolic step of the E2E pipeline, on the fleet the
// board's bitstream implements (tag tile-pipelined-1x8x8-3x4x4-20261009):
// one 8x8 and three 4x4 arrays, k_max = 16, H = 95.
//
// A 4x4x4 GEMM is one spatial tile. These devices declare no read or write
// term, so the cost model prices the tile at its compute term: the 4x4
// array finishes it in k + 2*4 - 2 + 95 = 105 cycles against
// 4 + 2*8 - 2 + 95 = 113 on the 8x8, so the binary search puts it on the
// first 4x4. K = 4 <= k_max: one invocation.

module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95

  // CHECK-LABEL: func.func @matmul_4x4x4
  // CHECK-SAME: (%[[A:.*]]: tensor<4x4xf32>, %[[B:.*]]: tensor<4x4xf32>, %[[C:.*]]: tensor<4x4xf32>)
  // CHECK: %[[CT:.*]] = tensor.extract_slice %[[C]][0, 0] [4, 4] [1, 1]
  // CHECK: %[[AT:.*]] = tensor.extract_slice %[[A]][0, 0] [4, 4] [1, 1]
  // CHECK: %[[BT:.*]] = tensor.extract_slice %[[B]][0, 0] [4, 4] [1, 1]
  // CHECK: %[[T:.*]] = systolic.matmul_tile %[[AT]], %[[BT]], %[[CT]] m = 4 n = 4 k = 4 on @acc_4x4_0 est_cycles = 105 start_cycle = 0 :
  // CHECK: %[[R:.*]] = tensor.insert_slice %[[T]] into %[[C]][0, 0] [4, 4] [1, 1]
  // CHECK: return %[[R]]
  // CHECK-NOT: linalg.matmul
  func.func @matmul_4x4x4(%A: tensor<4x4xf32>, %B: tensor<4x4xf32>,
                          %C: tensor<4x4xf32>) -> tensor<4x4xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<4x4xf32>, tensor<4x4xf32>)
                       outs(%C : tensor<4x4xf32>) -> tensor<4x4xf32>
    return %0 : tensor<4x4xf32>
  }
}
