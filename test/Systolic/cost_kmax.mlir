// RUN: systolic-opt --systolic-cost-analysis %s | FileCheck %s

// The cost model with each device's own k_max:
//
//   cycles = folds * (k + ceil(k / k_max) * (rows + cols - 2 + H))
//
// k_max is any positive integer; a device that declares none runs the whole
// reduction in one invocation (ceil = 1), as the binary-search decomposition
// does -- it used to be costed as k_max = 1, one invocation per step.

module {
  systolic.device @acc_8x8_k17 rows = 8 cols = 8 dataflow = output_stationary k_max = 17 tile_overhead = 95
  systolic.device @acc_4x4_k7  rows = 4 cols = 4 dataflow = output_stationary k_max = 7 tile_overhead = 95
  systolic.device @acc_4x4_k1  rows = 4 cols = 4 dataflow = output_stationary k_max = 1 tile_overhead = 95
  systolic.device @acc_8x8_none rows = 8 cols = 8 dataflow = output_stationary tile_overhead = 95

  // 8x8x40 on k_max 17: 1 fold, ceil(40/17) = 3: 40 + 3 * (14 + 95) = 367.
  // CHECK-LABEL: func.func @on_8x8_k17
  // CHECK: est_cycles = 367
  func.func @on_8x8_k17(%a: tensor<8x40xf32>, %b: tensor<40x8xf32>, %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = systolic.matmul_tile %a, %b, %c m = 8 n = 8 k = 40 on @acc_8x8_k17
         : (tensor<8x40xf32>, tensor<40x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }

  // 4x4x40 on k_max 7: ceil(40/7) = 6: 40 + 6 * (6 + 95) = 646.
  // CHECK-LABEL: func.func @on_4x4_k7
  // CHECK: est_cycles = 646
  func.func @on_4x4_k7(%a: tensor<4x40xf32>, %b: tensor<40x4xf32>, %c: tensor<4x4xf32>) -> tensor<4x4xf32> {
    %0 = systolic.matmul_tile %a, %b, %c m = 4 n = 4 k = 40 on @acc_4x4_k7
         : (tensor<4x40xf32>, tensor<40x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    return %0 : tensor<4x4xf32>
  }

  // 4x4x3 on k_max 1: three invocations of one step: 3 + 3 * 101 = 306.
  // CHECK-LABEL: func.func @on_4x4_k1
  // CHECK: est_cycles = 306
  func.func @on_4x4_k1(%a: tensor<4x3xf32>, %b: tensor<3x4xf32>, %c: tensor<4x4xf32>) -> tensor<4x4xf32> {
    %0 = systolic.matmul_tile %a, %b, %c m = 4 n = 4 k = 3 on @acc_4x4_k1
         : (tensor<4x3xf32>, tensor<3x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    return %0 : tensor<4x4xf32>
  }

  // No k_max: one invocation, 40 + 109 = 149 (was 40 + 40 * 109 = 4400).
  // CHECK-LABEL: func.func @on_8x8_undeclared
  // CHECK: est_cycles = 149
  func.func @on_8x8_undeclared(%a: tensor<8x40xf32>, %b: tensor<40x8xf32>, %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = systolic.matmul_tile %a, %b, %c m = 8 n = 8 k = 40 on @acc_8x8_none
         : (tensor<8x40xf32>, tensor<40x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }
}
