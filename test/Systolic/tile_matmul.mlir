// test/Systolic/tile_matmul_exec.mlir
// RUN: systolic-opt --systolic-tile-matmul="tile-m=4 tile-n=4 tile-k=4" %s | FileCheck %s
module {
  systolic.device @acc_8x8 rows = 8 cols = 8 depth = 8 dataflow = weight_stationary
  systolic.device @acc_4x4 rows = 4 cols = 4 depth = 4 dataflow = weight_stationary

  // 16x16x16 tiled at 4x4x4 decomposes into 4*4*4 = 64 tiles. The K-dimension
  // tiles chain through c_in so the partial sums accumulate in one tensor,
  // and each output tile is written back with a single insert_slice.
  // CHECK-LABEL: func.func @big_matmul
  // CHECK: tensor.extract_slice
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: k = 4 : i64, m = 4 : i64, n = 4 : i64
  // CHECK-COUNT-63: systolic.matmul_tile
  // CHECK-NOT: linalg.matmul
  func.func @big_matmul(%A: tensor<16x16xf32>, %B: tensor<16x16xf32>, %C: tensor<16x16xf32>) -> tensor<16x16xf32> {
    %result = linalg.matmul ins(%A, %B : tensor<16x16xf32>, tensor<16x16xf32>)
                            outs(%C : tensor<16x16xf32>) -> tensor<16x16xf32>
    return %result : tensor<16x16xf32>
  }
}
