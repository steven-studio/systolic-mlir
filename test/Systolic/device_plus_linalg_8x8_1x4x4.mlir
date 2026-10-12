// RUN: systolic-opt --systolic-binary-search-tile-matmul %s | FileCheck %s

// The 16x16 output is materialized as:
//   3 x 8x8 tiles
//   4 x 4x4 tiles on 1 physical 4x4 accelerator instance
//
// The test checks physical device identity, not merely geometry.
//
// A tile costs its compute term, K + 2g - 2 + 95: 125 on the 8x8, 117 on
// the 4x4. Three 8x8 tiles (375) and four 4x4 tiles (468) give makespan
// 468; four 8x8 tiles would end at 500.

// CHECK: systolic.matmul_tile{{.*}} m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 0
// CHECK: systolic.matmul_tile{{.*}} m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 125
// CHECK: systolic.matmul_tile{{.*}} m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 250
// CHECK: systolic.matmul_tile{{.*}} m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 0
// CHECK: systolic.matmul_tile{{.*}} m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 117
// CHECK: systolic.matmul_tile{{.*}} m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 234
// CHECK: systolic.matmul_tile{{.*}} m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 351

module {
systolic.device @acc_8x8_0
    rows = 8
    cols = 8
    dataflow = output_stationary
    tile_overhead = 95

  systolic.device @acc_4x4_0
    rows = 4
    cols = 4
    dataflow = output_stationary
    tile_overhead = 95

func.func @matmul_16x16(
      %A: tensor<16x16xf32>,
      %B: tensor<16x16xf32>,
      %C: tensor<16x16xf32>)
      -> tensor<16x16xf32> {

    %0 = linalg.matmul
      ins(%A, %B : tensor<16x16xf32>, tensor<16x16xf32>)
      outs(%C : tensor<16x16xf32>)
      -> tensor<16x16xf32>

    return %0 : tensor<16x16xf32>
  }
}
