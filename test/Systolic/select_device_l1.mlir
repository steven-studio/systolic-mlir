// RUN: systolic-opt --systolic-select-device %s | FileCheck %s --check-prefix=ON
// RUN: systolic-opt --systolic-select-device="enforce-l1-capacity=false" %s | FileCheck %s --check-prefix=OFF

// The optional l1_bytes admissibility filter of --systolic-select-device.
// A device that declares l1_bytes is excluded for a tile whose A and B
// operands, (m*k + k*n) elements at the tile's element width, do not fit;
// a device that does not declare it is unconstrained. The filter only
// removes candidates, so with it on the greedy scheduler can land on a
// slower device, and with it off it schedules exactly as before.
//
// Costs use the paper-hw-v1 calibration, H = 95, k_max = 256 on both
// devices, cycles = folds * (k + ceil(k/k_max) * (rows + cols - 2 + 95)):
//
//   64^3 f32: A + B = 2 * 64*64 * 4 = 32768 bytes  > 16384: acc_8x8 excluded
//     acc_8x8:  64 folds * (64 + 109) = 11072
//     acc_4x4: 256 folds * (64 + 101) = 42240
//   32^3 f32: A + B = 2 * 32*32 * 4 =  8192 bytes <= 16384: both admissible
//     acc_8x8:  16 folds * (32 + 109) = 16 * 141 = 2256
//     acc_4x4:  64 folds * (32 + 101)      = 64 * 133 = 8512
//
// Tiles are processed largest-first, so 64^3 is placed before 32^3.

module {
  systolic.device @acc_8x8 rows = 8 cols = 8 dataflow = output_stationary
      {k_max = 256 : i64, tile_overhead = 95 : i64, l1_bytes = 16384 : i64}
  systolic.device @acc_4x4 rows = 4 cols = 4 dataflow = output_stationary
      {k_max = 256 : i64, tile_overhead = 95 : i64}

  // With the check on, the fast device cannot hold this tile's operands and
  // the tile goes to the slow one at 3.8x the cost. With the check off it
  // goes where the cost model alone would put it.
  // ON-LABEL: func.func @big
  // ON: systolic.matmul_tile
  // ON-SAME: on @acc_4x4
  // ON-SAME: est_cycles = 42240
  // OFF-LABEL: func.func @big
  // OFF: systolic.matmul_tile
  // OFF-SAME: on @acc_8x8
  // OFF-SAME: est_cycles = 11072
  func.func @big(%a: tensor<64x64xf32>, %b: tensor<64x64xf32>,
                 %c: tensor<64x64xf32>) -> tensor<64x64xf32> {
    %0 = systolic.matmul_tile %a, %b, %c
         {m = 64 : i64, n = 64 : i64, k = 64 : i64}
         : (tensor<64x64xf32>, tensor<64x64xf32>, tensor<64x64xf32>)
           -> tensor<64x64xf32>
    return %0 : tensor<64x64xf32>
  }

  // This tile fits both devices, so the filter never touches it directly;
  // it still lands somewhere else because the load the big tile left behind
  // moved. Check on: acc_8x8 is idle (0 + 2256) against acc_4x4 carrying
  // 42240. Check off: acc_8x8 carries 11072 (11072 + 2256 = 13328) against
  // an idle acc_4x4 (0 + 8512).
  // ON-LABEL: func.func @small
  // ON: systolic.matmul_tile
  // ON-SAME: on @acc_8x8
  // ON-SAME: est_cycles = 2256
  // OFF-LABEL: func.func @small
  // OFF: systolic.matmul_tile
  // OFF-SAME: on @acc_4x4
  // OFF-SAME: est_cycles = 8512
  func.func @small(%a: tensor<32x32xf32>, %b: tensor<32x32xf32>,
                   %c: tensor<32x32xf32>) -> tensor<32x32xf32> {
    %0 = systolic.matmul_tile %a, %b, %c
         {m = 32 : i64, n = 32 : i64, k = 32 : i64}
         : (tensor<32x32xf32>, tensor<32x32xf32>, tensor<32x32xf32>)
           -> tensor<32x32xf32>
    return %0 : tensor<32x32xf32>
  }
}
