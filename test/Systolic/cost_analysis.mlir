// RUN: systolic-opt --systolic-cost-analysis %s | FileCheck %s

// Pins the cost model to its calibration data. Each single-tile case below
// is a configuration that was measured by C/RTL co-simulation, so the
// est_cycles the pass computes must equal the number the RTL actually took.
// If someone changes the formula or the fitted constants, this test fails
// rather than the model silently drifting away from the hardware.
//
//   cycles_tile = II * (depth + rows + cols - 2) + fixedOverhead
//   with II = 1 and fixedOverhead = 6.
//
// The device `depth` attribute is the array's K-tile depth -- the same
// quantity as K_DIM in hls/gemm_4x4/design.h. It must match the synthesised
// kernel or the estimate describes hardware that was never built.

module {
  systolic.device @acc_4x4  rows = 4  cols = 4 depth = 4 dataflow = weight_stationary
  systolic.device @acc_8x8  rows = 8  cols = 8 depth = 8 dataflow = weight_stationary
  systolic.device @acc_32x2 rows = 32 cols = 2 depth = 8 dataflow = weight_stationary

  // 4x4x4, one tile: 1 * (4 + 4 + 4 - 2) + 6 = 16. Cosim measured 16.
  // CHECK-LABEL: func.func @tile_4x4x4
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: est_cycles = 16
  func.func @tile_4x4x4(%a: tensor<4x4xf32>, %b: tensor<4x4xf32>,
                        %c: tensor<4x4xf32>) -> tensor<4x4xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @acc_4x4
         {m = 4 : i64, n = 4 : i64, k = 4 : i64}
         : (tensor<4x4xf32>, tensor<4x4xf32>, tensor<4x4xf32>)
           -> tensor<4x4xf32>
    return %0 : tensor<4x4xf32>
  }

  // 8x8x8, one tile: 1 * (8 + 8 + 8 - 2) + 6 = 28. Cosim measured 28.
  // CHECK-LABEL: func.func @tile_8x8x8
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: est_cycles = 28
  func.func @tile_8x8x8(%a: tensor<8x8xf32>, %b: tensor<8x8xf32>,
                        %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @acc_8x8
         {m = 8 : i64, n = 8 : i64, k = 8 : i64}
         : (tensor<8x8xf32>, tensor<8x8xf32>, tensor<8x8xf32>)
           -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }

  // 32x2x8, one tile: 1 * (8 + 32 + 2 - 2) + 6 = 46. Cosim measured 46.
  // A tall, narrow array: same 64 PEs as 8x8 but rows+cols is 34 instead
  // of 16, so it costs far more per tile. That separation is exactly what
  // the model has to represent, and what a tiles-only estimate cannot.
  // CHECK-LABEL: func.func @tile_32x2x8
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: est_cycles = 46
  func.func @tile_32x2x8(%a: tensor<32x8xf32>, %b: tensor<8x2xf32>,
                         %c: tensor<32x2xf32>) -> tensor<32x2xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @acc_32x2
         {m = 32 : i64, n = 2 : i64, k = 8 : i64}
         : (tensor<32x8xf32>, tensor<8x2xf32>, tensor<32x2xf32>)
           -> tensor<32x2xf32>
    return %0 : tensor<32x2xf32>
  }

  // Multi-tile: 8x8x8 on the 4x4x4 array decomposes into
  // ceil(8/4) * ceil(8/4) * ceil(8/4) = 8 tiles, each costing 16 -> 128.
  // Every tile pays its own fill/drain; charging it once for the whole
  // GEMM would give 8 + 6 = 14 and make a larger array look free.
  // CHECK-LABEL: func.func @multi_tile
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: est_cycles = 128
  func.func @multi_tile(%a: tensor<8x8xf32>, %b: tensor<8x8xf32>,
                        %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @acc_4x4
         {m = 8 : i64, n = 8 : i64, k = 8 : i64}
         : (tensor<8x8xf32>, tensor<8x8xf32>, tensor<8x8xf32>)
           -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }

  // Non-divisible: 5x5x5 on 4x4x4 rounds up to 2*2*2 = 8 tiles -> 128.
  // The padding waste is real and the model must not smooth it away.
  // CHECK-LABEL: func.func @ragged
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: est_cycles = 128
  func.func @ragged(%a: tensor<5x5xf32>, %b: tensor<5x5xf32>,
                    %c: tensor<5x5xf32>) -> tensor<5x5xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @acc_4x4
         {m = 5 : i64, n = 5 : i64, k = 5 : i64}
         : (tensor<5x5xf32>, tensor<5x5xf32>, tensor<5x5xf32>)
           -> tensor<5x5xf32>
    return %0 : tensor<5x5xf32>
  }
}
