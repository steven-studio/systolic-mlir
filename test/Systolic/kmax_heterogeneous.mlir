// RUN: systolic-opt --systolic-binary-search-tile-matmul --split-input-file %s | FileCheck %s

// K_MAX per accelerator, on the 1x8x8 + 3x4x4 fleet with H = 95:
// K_MAX_8X8 (the 8x8's k_max) and K_MAX_4X4 (every 4x4's) are independent,
// any positive integer, no power of two required.
//
// For accelerator d, a spatial tile's reduction K runs as
//   I(d) = ceil(K / k_max(d)) invocations, each 1 <= k <= k_max(d),
// invocation j covering [j * k_max(d), min(K, (j + 1) * k_max(d))) and
// chained through c_in, so the partial sums accumulate in order. The cost
// model prices each invocation (CostModel.h); these devices declare no read
// or write term and the default logical depth, so invocation j of depth k_j
// costs its compute term k_j + 2g - 2 + 95 (k_j + 109 on the 8x8, k_j + 101
// on a 4x4) and the last, shorter invocation costs less than the others. A
// tile costs the sum over its invocations, and that is what the
// decomposition balances. A tile's invocations run back to back:
// est_cycles is invocation j's own cost, start_cycle the sum of the costs
// before it on the same device -- never an equal split of the tile.

// K_MAX_8X8 = 24, K_MAX_4X4 = 12, K = 51 (divisible by neither).
// 12x12 is one 8x8 tile and five 4x4 tiles. The 8x8 folds 51 = 24 + 24 + 3
// (133 + 133 + 112 = 378 cycles), every 4x4 51 = 12 + 12 + 12 + 12 + 3
// (4 * 113 + 104 = 556), so the two 4x4 tiles of round 1 start at 556 and
// the makespan is 2 * 556 = 1112.
// CHECK-LABEL: func.func @kmax_24_12_k51
// CHECK: %[[A0:[^ ]+]] = systolic.matmul_tile %{{[^ ]+}}, %{{[^ ]+}}, %{{[^ ]+}} m = 8 n = 8 k = 24 on @acc_8x8_0 est_cycles = 133 start_cycle = 0
// CHECK: %[[A1:[^ ]+]] = systolic.matmul_tile %{{[^ ]+}}, %{{[^ ]+}}, %[[A0]] m = 8 n = 8 k = 24 on @acc_8x8_0 est_cycles = 133 start_cycle = 133
// CHECK: %[[A2:[^ ]+]] = systolic.matmul_tile %{{[^ ]+}}, %{{[^ ]+}}, %[[A1]] m = 8 n = 8 k = 3 on @acc_8x8_0 est_cycles = 112 start_cycle = 266
// CHECK: tensor.insert_slice %[[A2]] into
// CHECK: %[[B0:[^ ]+]] = systolic.matmul_tile {{.*}} m = 4 n = 4 k = 12 on @acc_4x4_0 est_cycles = 113 start_cycle = 0
// CHECK: %[[B1:[^ ]+]] = systolic.matmul_tile %{{[^ ]+}}, %{{[^ ]+}}, %[[B0]] m = 4 n = 4 k = 12 on @acc_4x4_0 est_cycles = 113 start_cycle = 113
// CHECK: %[[B2:[^ ]+]] = systolic.matmul_tile %{{[^ ]+}}, %{{[^ ]+}}, %[[B1]] m = 4 n = 4 k = 12 on @acc_4x4_0 est_cycles = 113 start_cycle = 226
// CHECK: %[[B3:[^ ]+]] = systolic.matmul_tile %{{[^ ]+}}, %{{[^ ]+}}, %[[B2]] m = 4 n = 4 k = 12 on @acc_4x4_0 est_cycles = 113 start_cycle = 339
// CHECK: %[[B4:[^ ]+]] = systolic.matmul_tile %{{[^ ]+}}, %{{[^ ]+}}, %[[B3]] m = 4 n = 4 k = 3 on @acc_4x4_0 est_cycles = 104 start_cycle = 452
// CHECK: tensor.insert_slice %[[B4]] into
// CHECK: k = 12 on @acc_4x4_1 est_cycles = 113 start_cycle = 0
// CHECK: k = 3 on @acc_4x4_1 est_cycles = 104 start_cycle = 452
// CHECK: k = 12 on @acc_4x4_2 est_cycles = 113 start_cycle = 0
// CHECK: k = 3 on @acc_4x4_2 est_cycles = 104 start_cycle = 452
// CHECK: k = 12 on @acc_4x4_0 est_cycles = 113 start_cycle = 556
// CHECK: k = 3 on @acc_4x4_0 est_cycles = 104 start_cycle = 1008
// CHECK: k = 12 on @acc_4x4_1 est_cycles = 113 start_cycle = 556
// CHECK: k = 3 on @acc_4x4_1 est_cycles = 104 start_cycle = 1008
// CHECK-NOT: systolic.matmul_tile
// CHECK-NOT: linalg.matmul
module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 24 tile_overhead = 95
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 12 tile_overhead = 95
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 12 tile_overhead = 95
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 12 tile_overhead = 95
  func.func @kmax_24_12_k51(%A: tensor<12x51xf32>, %B: tensor<51x12xf32>, %C: tensor<12x12xf32>) -> tensor<12x12xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<12x51xf32>, tensor<51x12xf32>) outs(%C : tensor<12x12xf32>) -> tensor<12x12xf32>
    return %0 : tensor<12x12xf32>
  }
}

// -----

// K_MAX_8X8 = 17, K_MAX_4X4 = 7 (both prime), K = 18: the 8x8 folds
// 17 + 1 (126 + 110 = 236), every 4x4 7 + 7 + 4 (108 + 108 + 105 = 321);
// five 4x4 tiles in two rounds give makespan 642.
// CHECK-LABEL: func.func @kmax_17_7_k18
// CHECK: %[[C0:[^ ]+]] = systolic.matmul_tile {{.*}} m = 8 n = 8 k = 17 on @acc_8x8_0 est_cycles = 126 start_cycle = 0
// CHECK: %[[C1:[^ ]+]] = systolic.matmul_tile %{{[^ ]+}}, %{{[^ ]+}}, %[[C0]] m = 8 n = 8 k = 1 on @acc_8x8_0 est_cycles = 110 start_cycle = 126
// CHECK: tensor.insert_slice %[[C1]] into
// CHECK: %[[D0:[^ ]+]] = systolic.matmul_tile {{.*}} m = 4 n = 4 k = 7 on @acc_4x4_0 est_cycles = 108 start_cycle = 0
// CHECK: %[[D1:[^ ]+]] = systolic.matmul_tile %{{[^ ]+}}, %{{[^ ]+}}, %[[D0]] m = 4 n = 4 k = 7 on @acc_4x4_0 est_cycles = 108 start_cycle = 108
// CHECK: %[[D2:[^ ]+]] = systolic.matmul_tile %{{[^ ]+}}, %{{[^ ]+}}, %[[D1]] m = 4 n = 4 k = 4 on @acc_4x4_0 est_cycles = 105 start_cycle = 216
// CHECK: tensor.insert_slice %[[D2]] into
// CHECK: k = 7 on @acc_4x4_1 est_cycles = 108 start_cycle = 0
// CHECK: k = 4 on @acc_4x4_1 est_cycles = 105 start_cycle = 216
// CHECK: k = 7 on @acc_4x4_2 est_cycles = 108 start_cycle = 0
// CHECK: k = 4 on @acc_4x4_2 est_cycles = 105 start_cycle = 216
// CHECK: k = 7 on @acc_4x4_0 est_cycles = 108 start_cycle = 321
// CHECK: k = 4 on @acc_4x4_0 est_cycles = 105 start_cycle = 537
// CHECK: k = 7 on @acc_4x4_1 est_cycles = 108 start_cycle = 321
// CHECK: k = 4 on @acc_4x4_1 est_cycles = 105 start_cycle = 537
// CHECK-NOT: systolic.matmul_tile
module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 17 tile_overhead = 95
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 7 tile_overhead = 95
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 7 tile_overhead = 95
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 7 tile_overhead = 95
  func.func @kmax_17_7_k18(%A: tensor<12x18xf32>, %B: tensor<18x12xf32>, %C: tensor<12x12xf32>) -> tensor<12x12xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<12x18xf32>, tensor<18x12xf32>) outs(%C : tensor<12x12xf32>) -> tensor<12x12xf32>
    return %0 : tensor<12x12xf32>
  }
}

// -----

// Heterogeneous scheduling follows K_MAX. K_MAX_8X8 = 17, K_MAX_4X4 = 7,
// 8x16x17: on the 8x8 a tile is one invocation (17 + 109 = 126), on a 4x4
// three (108 + 108 + 104 = 320). Two 8x8 tiles back to back finish at 252;
// one 8x8 tile plus four 4x4 tiles would finish at 2 * 320 = 640. The
// decomposition takes the 8x8 twice. (With equal capacities -- next case --
// the same output is the 8x8 plus four 4x4 tiles.)
// CHECK-LABEL: func.func @kmax_17_7_prefers_8x8
// CHECK: m = 8 n = 8 k = 17 on @acc_8x8_0 est_cycles = 126 start_cycle = 0
// CHECK: m = 8 n = 8 k = 17 on @acc_8x8_0 est_cycles = 126 start_cycle = 126
// CHECK-NOT: systolic.matmul_tile
module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 17 tile_overhead = 95
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 7 tile_overhead = 95
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 7 tile_overhead = 95
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 7 tile_overhead = 95
  func.func @kmax_17_7_prefers_8x8(%A: tensor<8x17xf32>, %B: tensor<17x16xf32>, %C: tensor<8x16xf32>) -> tensor<8x16xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<8x17xf32>, tensor<17x16xf32>) outs(%C : tensor<8x16xf32>) -> tensor<8x16xf32>
    return %0 : tensor<8x16xf32>
  }
}

// -----

// The same 8x16x17 at K_MAX_8X8 = K_MAX_4X4 = 16: every device folds
// 16 + 1, a tile costs 125 + 110 = 235 on the 8x8 and 117 + 102 = 219 on a
// 4x4. One 8x8 tile and four 4x4 tiles in two rounds (438) beat two 8x8
// tiles (470) -- the decomposition this pass produced before per-device
// folding existed.
// CHECK-LABEL: func.func @kmax_16_16_mixed
// CHECK: m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 0
// CHECK: m = 8 n = 8 k = 1 on @acc_8x8_0 est_cycles = 110 start_cycle = 125
// CHECK: m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 0
// CHECK: m = 4 n = 4 k = 1 on @acc_4x4_0 est_cycles = 102 start_cycle = 117
// CHECK: m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 0
// CHECK: m = 4 n = 4 k = 1 on @acc_4x4_1 est_cycles = 102 start_cycle = 117
// CHECK: m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 0
// CHECK: m = 4 n = 4 k = 1 on @acc_4x4_2 est_cycles = 102 start_cycle = 117
// CHECK: m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 219
// CHECK: m = 4 n = 4 k = 1 on @acc_4x4_0 est_cycles = 102 start_cycle = 336
// CHECK-NOT: systolic.matmul_tile
module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  func.func @kmax_16_16_mixed(%A: tensor<8x17xf32>, %B: tensor<17x16xf32>, %C: tensor<8x16xf32>) -> tensor<8x16xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<8x17xf32>, tensor<17x16xf32>) outs(%C : tensor<8x16xf32>) -> tensor<8x16xf32>
    return %0 : tensor<8x16xf32>
  }
}

// -----

// K_MAX_8X8 = 8 < K_MAX_4X4 = 16, 8x8x16. The 8x8 needs two invocations,
// but their depths add up to K, so it pays the per-invocation term once more
// and nothing else: 2 * (8 + 109) = 234. Four 4x4 tiles of one invocation
// each take two rounds, 2 * (16 + 101) = 234 as well. On a tie the greedy
// decomposition takes the larger geometry, which it tries first.
// CHECK-LABEL: func.func @kmax_8_16_tie
// CHECK: m = 8 n = 8 k = 8 on @acc_8x8_0 est_cycles = 117 start_cycle = 0
// CHECK: m = 8 n = 8 k = 8 on @acc_8x8_0 est_cycles = 117 start_cycle = 117
// CHECK-NOT: systolic.matmul_tile
module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 8 tile_overhead = 95
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  func.func @kmax_8_16_tie(%A: tensor<8x16xf32>, %B: tensor<16x8xf32>, %C: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<8x16xf32>, tensor<16x8xf32>) outs(%C : tensor<8x8xf32>) -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }
}

// -----

// K_MAX_8X8 = 4 < K_MAX_4X4 = 16, 8x8x16: the 8x8 would need four
// invocations, 4 * (4 + 109) = 452; four 4x4 tiles of one invocation each
// take two rounds (234). The 8x8 is left idle.
// CHECK-LABEL: func.func @kmax_4_16_prefers_4x4
// CHECK: m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 0
// CHECK: m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 0
// CHECK: m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 0
// CHECK: m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 117
// CHECK-NOT: systolic.matmul_tile
module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 4 tile_overhead = 95
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  func.func @kmax_4_16_prefers_4x4(%A: tensor<8x16xf32>, %B: tensor<16x8xf32>, %C: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<8x16xf32>, tensor<16x8xf32>) outs(%C : tensor<8x8xf32>) -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }
}

// -----

// K_MAX_8X8 = 16, K_MAX_4X4 = 8, K = 20: the 8x8 folds 16 + 4
// (125 + 113 = 238), every 4x4 8 + 8 + 4 (109 + 109 + 105 = 323).
// CHECK-LABEL: func.func @kmax_16_8_k20
// CHECK: m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 0
// CHECK: m = 8 n = 8 k = 4 on @acc_8x8_0 est_cycles = 113 start_cycle = 125
// CHECK: m = 4 n = 4 k = 8 on @acc_4x4_0 est_cycles = 109 start_cycle = 0
// CHECK: m = 4 n = 4 k = 8 on @acc_4x4_0 est_cycles = 109 start_cycle = 109
// CHECK: m = 4 n = 4 k = 4 on @acc_4x4_0 est_cycles = 105 start_cycle = 218
// CHECK-NOT: k = 16 on @acc_4x4
// CHECK-NOT: linalg.matmul
module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 8 tile_overhead = 95
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 8 tile_overhead = 95
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 8 tile_overhead = 95
  func.func @kmax_16_8_k20(%A: tensor<12x20xf32>, %B: tensor<20x12xf32>, %C: tensor<12x12xf32>) -> tensor<12x12xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<12x20xf32>, tensor<20x12xf32>) outs(%C : tensor<12x12xf32>) -> tensor<12x12xf32>
    return %0 : tensor<12x12xf32>
  }
}

// -----

// K_MAX = 1 on every device, K = 3: three invocations of one reduction step
// per tile, chained through c_in (3 * 110 = 330 on the 8x8, 3 * 102 = 306 on
// a 4x4).
// CHECK-LABEL: func.func @kmax_1_1_k3
// CHECK: %[[E0:[^ ]+]] = systolic.matmul_tile {{.*}} m = 8 n = 8 k = 1 on @acc_8x8_0 est_cycles = 110 start_cycle = 0
// CHECK: %[[E1:[^ ]+]] = systolic.matmul_tile %{{[^ ]+}}, %{{[^ ]+}}, %[[E0]] m = 8 n = 8 k = 1 on @acc_8x8_0 est_cycles = 110 start_cycle = 110
// CHECK: %[[E2:[^ ]+]] = systolic.matmul_tile %{{[^ ]+}}, %{{[^ ]+}}, %[[E1]] m = 8 n = 8 k = 1 on @acc_8x8_0 est_cycles = 110 start_cycle = 220
// CHECK: tensor.insert_slice %[[E2]] into
// CHECK: m = 4 n = 4 k = 1 on @acc_4x4_0 est_cycles = 102 start_cycle = 0
// CHECK: m = 4 n = 4 k = 1 on @acc_4x4_0 est_cycles = 102 start_cycle = 102
// CHECK: m = 4 n = 4 k = 1 on @acc_4x4_0 est_cycles = 102 start_cycle = 204
// CHECK-NOT: k = 2
// CHECK-NOT: k = 3
module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 1 tile_overhead = 95
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 1 tile_overhead = 95
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 1 tile_overhead = 95
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 1 tile_overhead = 95
  func.func @kmax_1_1_k3(%A: tensor<12x3xf32>, %B: tensor<3x12xf32>, %C: tensor<12x12xf32>) -> tensor<12x12xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<12x3xf32>, tensor<3x12xf32>) outs(%C : tensor<12x12xf32>) -> tensor<12x12xf32>
    return %0 : tensor<12x12xf32>
  }
}

// -----

// K below, at and past K_MAX on one device: K_MAX_8X8 = 16, K_MAX_4X4 = 8,
// an 8x8 output (one 8x8 tile). Below: 9 + 109 = 118; at: 16 + 109 = 125;
// past (K = 33 = 16 + 16 + 1): 125 + 125 + 110, starting at 0, 125, 250.
// CHECK-LABEL: func.func @k_below_kmax
// CHECK: m = 8 n = 8 k = 9 on @acc_8x8_0 est_cycles = 118 start_cycle = 0
// CHECK-NOT: systolic.matmul_tile
// CHECK-LABEL: func.func @k_at_kmax
// CHECK: m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 0
// CHECK-NOT: systolic.matmul_tile
// CHECK-LABEL: func.func @k_past_kmax
// CHECK: m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 0
// CHECK: m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 125
// CHECK: m = 8 n = 8 k = 1 on @acc_8x8_0 est_cycles = 110 start_cycle = 250
// CHECK-NOT: systolic.matmul_tile
module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 8 tile_overhead = 95
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 8 tile_overhead = 95
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 8 tile_overhead = 95
  func.func @k_below_kmax(%A: tensor<8x9xf32>, %B: tensor<9x8xf32>, %C: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<8x9xf32>, tensor<9x8xf32>) outs(%C : tensor<8x8xf32>) -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }
  func.func @k_at_kmax(%A: tensor<8x16xf32>, %B: tensor<16x8xf32>, %C: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<8x16xf32>, tensor<16x8xf32>) outs(%C : tensor<8x8xf32>) -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }
  func.func @k_past_kmax(%A: tensor<8x33xf32>, %B: tensor<33x8xf32>, %C: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<8x33xf32>, tensor<33x8xf32>) outs(%C : tensor<8x8xf32>) -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }
}
