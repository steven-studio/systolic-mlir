// RUN: systolic-opt --systolic-select-device --split-input-file %s | FileCheck %s

// Device selection charges every candidate its own k_max:
//   cycles = folds * (k + ceil(k / k_max) * (rows + cols - 2 + H)).
// The same 4x4x16 tile goes to the 8x8 when the 8x8's capacity is the
// deeper one and to a 4x4 when the 4x4's is.

// K_MAX_8X8 = 17, K_MAX_4X4 = 7:
//   acc_8x8  16 + 1 * 109 = 125
//   acc_4x4  16 + 3 * 101 = 319
// CHECK-LABEL: func.func @deep_8x8
// CHECK: on @acc_8x8
// CHECK-SAME: est_cycles = 125
module {
  systolic.device @acc_8x8 rows = 8 cols = 8 dataflow = output_stationary k_max = 17 tile_overhead = 95
  systolic.device @acc_4x4 rows = 4 cols = 4 dataflow = output_stationary k_max = 7 tile_overhead = 95
  func.func @deep_8x8(%a: tensor<4x16xf32>, %b: tensor<16x4xf32>, %c: tensor<4x4xf32>) -> tensor<4x4xf32> {
    %0 = systolic.matmul_tile %a, %b, %c m = 4 n = 4 k = 16
         : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    return %0 : tensor<4x4xf32>
  }
}

// -----

// K_MAX_8X8 = 8, K_MAX_4X4 = 16:
//   acc_8x8  16 + 2 * 109 = 234
//   acc_4x4  16 + 1 * 101 = 117
// CHECK-LABEL: func.func @deep_4x4
// CHECK: on @acc_4x4
// CHECK-SAME: est_cycles = 117
module {
  systolic.device @acc_8x8 rows = 8 cols = 8 dataflow = output_stationary k_max = 8 tile_overhead = 95
  systolic.device @acc_4x4 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  func.func @deep_4x4(%a: tensor<4x16xf32>, %b: tensor<16x4xf32>, %c: tensor<4x4xf32>) -> tensor<4x4xf32> {
    %0 = systolic.matmul_tile %a, %b, %c m = 4 n = 4 k = 16
         : (tensor<4x16xf32>, tensor<16x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    return %0 : tensor<4x4xf32>
  }
}
