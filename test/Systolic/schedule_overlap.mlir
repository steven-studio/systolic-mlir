// RUN: systolic-opt --systolic-tile-matmul="tile-m=8 tile-n=8 tile-k=8" --systolic-select-device --systolic-schedule-overlap %s | FileCheck %s
module {
  systolic.device @acc_8x8 rows = 8 cols = 8 depth = 8 dataflow = weight_stationary {dma_bytes_per_cycle = 32.0 : f64}

  // Overlap scheduling needs tiles that have already been assigned to a
  // device, so this runs the whole pipeline rather than the overlap pass
  // alone -- on bare linalg.matmul the pass has nothing to schedule.
  //
  // Each 8x8x8 tile costs 28 cycles (the co-simulated figure), so start
  // times advance by exactly that, offset by the initial DMA.
  // CHECK-LABEL: func.func @big_matmul
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: est_cycles = 28 : i64
  // CHECK-SAME: start_cycle = 16 : i64
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: start_cycle = 44 : i64
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: start_cycle = 72 : i64
  // CHECK-NOT: linalg.matmul
  func.func @big_matmul(%A: tensor<16x16xf32>, %B: tensor<16x16xf32>, %C: tensor<16x16xf32>) -> tensor<16x16xf32> {
    %result = linalg.matmul ins(%A, %B : tensor<16x16xf32>, tensor<16x16xf32>)
                            outs(%C : tensor<16x16xf32>) -> tensor<16x16xf32>
    return %result : tensor<16x16xf32>
  }
}
