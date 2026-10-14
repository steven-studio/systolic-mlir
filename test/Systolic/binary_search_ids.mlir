// RUN: systolic-opt --systolic-binary-search-tile-matmul %s | FileCheck %s

// Every matmul_tile the binary-search pass emits names its place in its
// GEMM, for the asynchronous FPGA lowering (fpga_lowering_async.mlir):
// systolic.gemm (which linalg.matmul of the module), systolic.tile (which
// output block of it), systolic.k_index / systolic.k_count (which K fold of
// that block, of how many). 8x4x20 is two 4x4 output blocks (tiles 0 and 1),
// each folded into k = 16 and k = 4 (folds 0 and 1 of 2); the second
// function's GEMM is gemm 1, one block, one fold.

module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95

  // CHECK-LABEL: func.func @mm_8x4x20
  // CHECK: systolic.matmul_tile {{.*}} m = 4 n = 4 k = 16 on @acc_4x4_0 {{.*}} {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 0 : i64}
  // CHECK: systolic.matmul_tile {{.*}} m = 4 n = 4 k = 4 on @acc_4x4_0 {{.*}} {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 0 : i64}
  // CHECK: systolic.matmul_tile {{.*}} m = 4 n = 4 k = 16 on @acc_4x4_1 {{.*}} {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 0 : i64, systolic.tile = 1 : i64}
  // CHECK: systolic.matmul_tile {{.*}} m = 4 n = 4 k = 4 on @acc_4x4_1 {{.*}} {systolic.gemm = 0 : i64, systolic.k_count = 2 : i64, systolic.k_index = 1 : i64, systolic.tile = 1 : i64}
  func.func @mm_8x4x20(%A: tensor<8x20xf32>, %B: tensor<20x4xf32>,
                       %C: tensor<8x4xf32>) -> tensor<8x4xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<8x20xf32>, tensor<20x4xf32>)
                       outs(%C : tensor<8x4xf32>) -> tensor<8x4xf32>
    return %0 : tensor<8x4xf32>
  }

  // CHECK-LABEL: func.func @mm_4x4x8
  // CHECK: systolic.matmul_tile {{.*}} m = 4 n = 4 k = 8 on @acc_{{.*}} {systolic.gemm = 1 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 0 : i64}
  func.func @mm_4x4x8(%A: tensor<4x8xf32>, %B: tensor<8x4xf32>,
                      %C: tensor<4x4xf32>) -> tensor<4x4xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<4x8xf32>, tensor<8x4xf32>)
                       outs(%C : tensor<4x4xf32>) -> tensor<4x4xf32>
    return %0 : tensor<4x4xf32>
  }
}
