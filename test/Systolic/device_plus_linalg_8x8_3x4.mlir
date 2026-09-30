// RUN: %systolic_opt --systolic-binary-search-tile-matmul %s | FileCheck %s

// The 16x16 output is materialized as:
//   2 x 8x8 tiles
//   3 x 4x4 physical accelerator instances
//
// The test checks physical device identity, not merely geometry.

// CHECK: systolic.matmul_tile{{.*}} m = 8 n = 8 k = 16 on @acc_8x8 est_cycles = 24 start_cycle = 0
// CHECK: systolic.matmul_tile{{.*}} m = 8 n = 8 k = 16 on @acc_8x8 est_cycles = 24 start_cycle = 24
// CHECK: systolic.matmul_tile{{.*}} m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 16 start_cycle = 0
// CHECK: systolic.matmul_tile{{.*}} m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 16 start_cycle = 0
// CHECK: systolic.matmul_tile{{.*}} m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 16 start_cycle = 0
// CHECK: systolic.matmul_tile{{.*}} m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 16 start_cycle = 16
// CHECK: systolic.matmul_tile{{.*}} m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 16 start_cycle = 16
// CHECK: systolic.matmul_tile{{.*}} m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 16 start_cycle = 16
// CHECK: systolic.matmul_tile{{.*}} m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 16 start_cycle = 32
// CHECK: systolic.matmul_tile{{.*}} m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 16 start_cycle = 32

module {
systolic.device @acc_8x8
    rows = 8
    cols = 8
    depth = 1
    dataflow = output_stationary
    tile_overhead = 10

  systolic.device @acc_4x4_0
    rows = 4
    cols = 4
    depth = 1
    dataflow = output_stationary
    tile_overhead = 10

  systolic.device @acc_4x4_1
    rows = 4
    cols = 4
    depth = 1
    dataflow = output_stationary
    tile_overhead = 10

  systolic.device @acc_4x4_2
    rows = 4
    cols = 4
    depth = 1
    dataflow = output_stationary
    tile_overhead = 10

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
