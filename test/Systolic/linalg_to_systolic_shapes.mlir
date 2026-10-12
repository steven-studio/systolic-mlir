// RUN: systolic-opt --systolic-binary-search-tile-matmul %s | FileCheck %s

// Linalg -> Systolic on the board fleet for the shapes the E2E regression
// runs: rectangular M x N (the pass used to reject M != N although the
// optimizer covers rows x columns separately), tiles clipped at the matrix
// edge, and K folded by k_max = 16 into invocations chained through c_in.
//
// Costs come from the cost model (CostModel.h). These devices declare no
// read or write term and the default logical depth, so one invocation of
// depth k costs its compute term alone, k + 2g - 2 + 95: k + 109 on the 8x8,
// k + 101 on a 4x4. est_cycles is that term; start_cycle the sum of the
// terms before it on the same device.

module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95

  // A 1x1 output is one 4x4 tile clipped to 1x1: 1 + 101 = 102.
  // CHECK-LABEL: func.func @mm_1x1x1
  // CHECK: systolic.matmul_tile {{.*}} m = 1 n = 1 k = 1 on @acc_4x4_0 est_cycles = 102 start_cycle = 0 :
  // CHECK-NOT: systolic.matmul_tile
  // CHECK: return
  func.func @mm_1x1x1(%A: tensor<1x1xf32>, %B: tensor<1x1xf32>,
                      %C: tensor<1x1xf32>) -> tensor<1x1xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<1x1xf32>, tensor<1x1xf32>)
                       outs(%C : tensor<1x1xf32>) -> tensor<1x1xf32>
    return %0 : tensor<1x1xf32>
  }

  // M = 2, N = 3: one 4x4 tile clipped to 2x3: 4 + 101 = 105.
  // CHECK-LABEL: func.func @mm_2x3x4
  // CHECK: tensor.extract_slice %{{.*}}[0, 0] [2, 4] [1, 1]
  // CHECK: tensor.extract_slice %{{.*}}[0, 0] [4, 3] [1, 1]
  // CHECK: systolic.matmul_tile {{.*}} m = 2 n = 3 k = 4 on @acc_4x4_0 est_cycles = 105 start_cycle = 0 :
  // CHECK-NOT: systolic.matmul_tile
  // CHECK: return
  func.func @mm_2x3x4(%A: tensor<2x4xf32>, %B: tensor<4x3xf32>,
                      %C: tensor<2x3xf32>) -> tensor<2x3xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<2x4xf32>, tensor<4x3xf32>)
                       outs(%C : tensor<2x3xf32>) -> tensor<2x3xf32>
    return %0 : tensor<2x3xf32>
  }

  // M = 8, N = 4: two 4x4 arrays in parallel (makespan 16 + 101 = 117) beat
  // the 8x8 alone (16 + 109 = 125).
  // CHECK-LABEL: func.func @mm_8x4x16
  // CHECK: systolic.matmul_tile {{.*}} m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 0 :
  // CHECK: systolic.matmul_tile {{.*}} m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 0 :
  // CHECK-NOT: systolic.matmul_tile
  // CHECK: return
  func.func @mm_8x4x16(%A: tensor<8x16xf32>, %B: tensor<16x4xf32>,
                       %C: tensor<8x4xf32>) -> tensor<8x4xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<8x16xf32>, tensor<16x4xf32>)
                       outs(%C : tensor<8x4xf32>) -> tensor<8x4xf32>
    return %0 : tensor<8x4xf32>
  }

  // M = 16, N = 8: the 8x8 takes the top half (4 + 109 = 113); the bottom
  // half goes to the three 4x4 arrays in two rounds (makespan 2 x 105 = 210
  // < 2 x 113).
  // CHECK-LABEL: func.func @mm_16x8x4
  // CHECK: systolic.matmul_tile {{.*}} m = 8 n = 8 k = 4 on @acc_8x8_0 est_cycles = 113 start_cycle = 0 :
  // CHECK: tensor.extract_slice %{{.*}}[8, 0] [4, 4] [1, 1]
  // CHECK: systolic.matmul_tile {{.*}} m = 4 n = 4 k = 4 on @acc_4x4_0 est_cycles = 105 start_cycle = 0 :
  // CHECK: systolic.matmul_tile {{.*}} m = 4 n = 4 k = 4 on @acc_4x4_1 est_cycles = 105 start_cycle = 0 :
  // CHECK: systolic.matmul_tile {{.*}} m = 4 n = 4 k = 4 on @acc_4x4_2 est_cycles = 105 start_cycle = 0 :
  // CHECK: tensor.extract_slice %{{.*}}[12, 4] [4, 4] [1, 1]
  // CHECK: systolic.matmul_tile {{.*}} m = 4 n = 4 k = 4 on @acc_4x4_0 est_cycles = 105 start_cycle = 105 :
  // CHECK-NOT: systolic.matmul_tile
  // CHECK: return
  func.func @mm_16x8x4(%A: tensor<16x4xf32>, %B: tensor<4x8xf32>,
                       %C: tensor<16x8xf32>) -> tensor<16x8xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<16x4xf32>, tensor<4x8xf32>)
                       outs(%C : tensor<16x8xf32>) -> tensor<16x8xf32>
    return %0 : tensor<16x8xf32>
  }

  // K = 40 > k_max = 16: three invocations of depth 16, 16 and 8 on the same
  // device, each one's c_in the previous one's result. They cost 117, 117 and
  // 109 -- the last one is shorter, so the tile is not split into equal
  // parts: they start at 0, 117 and 234. One insert_slice writes the tile
  // back.
  // CHECK-LABEL: func.func @mm_4x4x40
  // CHECK-SAME: (%[[A:.*]]: tensor<4x40xf32>, %[[B:.*]]: tensor<40x4xf32>, %[[C:.*]]: tensor<4x4xf32>)
  // CHECK: %[[C0:.*]] = tensor.extract_slice %[[C]][0, 0] [4, 4] [1, 1]
  // CHECK: %[[A0:.*]] = tensor.extract_slice %[[A]][0, 0] [4, 16] [1, 1]
  // CHECK: %[[B0:.*]] = tensor.extract_slice %[[B]][0, 0] [16, 4] [1, 1]
  // CHECK: %[[T0:.*]] = systolic.matmul_tile %[[A0]], %[[B0]], %[[C0]] m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 0 :
  // CHECK: %[[A1:.*]] = tensor.extract_slice %[[A]][0, 16] [4, 16] [1, 1]
  // CHECK: %[[B1:.*]] = tensor.extract_slice %[[B]][16, 0] [16, 4] [1, 1]
  // CHECK: %[[T1:.*]] = systolic.matmul_tile %[[A1]], %[[B1]], %[[T0]] m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 117 :
  // CHECK: %[[A2:.*]] = tensor.extract_slice %[[A]][0, 32] [4, 8] [1, 1]
  // CHECK: %[[B2:.*]] = tensor.extract_slice %[[B]][32, 0] [8, 4] [1, 1]
  // CHECK: %[[T2:.*]] = systolic.matmul_tile %[[A2]], %[[B2]], %[[T1]] m = 4 n = 4 k = 8 on @acc_4x4_0 est_cycles = 109 start_cycle = 234 :
  // CHECK: %[[R:.*]] = tensor.insert_slice %[[T2]] into %[[C]][0, 0] [4, 4] [1, 1]
  // CHECK: return %[[R]]
  func.func @mm_4x4x40(%A: tensor<4x40xf32>, %B: tensor<40x4xf32>,
                       %C: tensor<4x4xf32>) -> tensor<4x4xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<4x40xf32>, tensor<40x4xf32>)
                       outs(%C : tensor<4x4xf32>) -> tensor<4x4xf32>
    return %0 : tensor<4x4xf32>
  }
}
