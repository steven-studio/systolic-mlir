// RUN: systolic-opt --systolic-tile-to-fpga %s | FileCheck %s

module {
  systolic.device @acc_4x4 rows = 4 cols = 4 dataflow = weight_stationary
      {k_max = 4 : i64, tile_overhead = 6 : i64}

  // A scheduled 4x4 tile should lower completely into the dispatch-runtime
  // ABI. No systolic.matmul_tile should remain after lowering.
  //
  // CHECK: llvm.func @systolic_dispatch_matmul4x4_dev
  // CHECK: llvm.func @systolic_dispatch_open
  // CHECK-LABEL: func.func @matmul
  // CHECK: llvm.call @systolic_dispatch_open()
  // CHECK: llvm.call @systolic_dispatch_matmul4x4_dev
  // CHECK-NOT: systolic.matmul_tile
  func.func @matmul(
      %a: tensor<4x4xf32>,
      %b: tensor<4x4xf32>,
      %c: tensor<4x4xf32>) -> tensor<4x4xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @acc_4x4
         {m = 4 : i64, n = 4 : i64, k = 4 : i64}
         : (tensor<4x4xf32>, tensor<4x4xf32>, tensor<4x4xf32>)
           -> tensor<4x4xf32>
    return %0 : tensor<4x4xf32>
  }
}
