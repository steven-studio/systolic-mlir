// RUN: systolic-opt --systolic-cost-analysis %s | FileCheck %s

// Pins the cost model to the hand-written 8x8 fold RTL, whose calibration
// differs from the HLS pipeline in cost_analysis.mlir by more than 17x.
//
//   cycles_tile = II * (depth + rows + cols - 2) + fixedOverhead
//   II = 1, fixedOverhead = 104, rows = cols = 8
//             => depth + 118
//
// Why 104 and not the 118 the hardware counter reports: the measured
// number is end-to-end, and the geometric term already accounts for
// rows + cols - 2 = 14. Two independent silicon points agree:
//
//   k_dim = 16 -> measured 134, geometric 30 -> 104
//   k_dim = 64 -> measured 182, geometric 78 -> 104
//
// The 104 is the datapath, not the array: roughly 74 cycles of tree
// reduction (log2(ACC_BANKS) levels, each waiting the FP adder latency),
// 20 cycles of multiplier plus adder pipeline, and the output handshake.
// None of those terms depends on rows, cols or depth, which is why a
// single constant is the right shape for them.
//
// `depth` is the K-tile depth the hardware is configured for. On this
// design it is a runtime value (k_dim in the request header), so each
// device below is the same silicon under a different configuration --
// not six different accelerators.
//
// Provenance: k_dim 8/16/24/48/64 measured in xsim via tb_array_fold_kmax
// (drain = 111 at every point, errors = 0); k_dim 16 and 64 additionally
// measured on the board through the hardware cycle counter. k_dim = 32 is
// held out -- it is predicted here and has never been measured.

module {
  systolic.device @fold_k8  rows = 8 cols = 8 depth = 8
      dataflow = output_stationary {fixed_overhead = 104 : i64}
  systolic.device @fold_k16 rows = 8 cols = 8 depth = 16
      dataflow = output_stationary {fixed_overhead = 104 : i64}
  systolic.device @fold_k32 rows = 8 cols = 8 depth = 32
      dataflow = output_stationary {fixed_overhead = 104 : i64}
  systolic.device @fold_k64 rows = 8 cols = 8 depth = 64
      dataflow = output_stationary {fixed_overhead = 104 : i64}

  // xsim: 126. 1 * (8 + 8 + 8 - 2) + 104 = 126.
  // CHECK-LABEL: func.func @fold_8
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: est_cycles = 126
  func.func @fold_8(%a: tensor<8x8xf32>, %b: tensor<8x8xf32>,
                    %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @fold_k8
         {m = 8 : i64, n = 8 : i64, k = 8 : i64}
         : (tensor<8x8xf32>, tensor<8x8xf32>, tensor<8x8xf32>)
           -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }

  // Silicon: the hardware cycle counter reported 134.
  // CHECK-LABEL: func.func @fold_16
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: est_cycles = 134
  func.func @fold_16(%a: tensor<8x16xf32>, %b: tensor<16x8xf32>,
                     %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @fold_k16
         {m = 8 : i64, n = 8 : i64, k = 16 : i64}
         : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>)
           -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }

  // Held out: never measured, in simulation or on the board. If the model
  // is right this is 150.
  // CHECK-LABEL: func.func @fold_32
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: est_cycles = 150
  func.func @fold_32(%a: tensor<8x32xf32>, %b: tensor<32x8xf32>,
                     %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @fold_k32
         {m = 8 : i64, n = 8 : i64, k = 32 : i64}
         : (tensor<8x32xf32>, tensor<32x8xf32>, tensor<8x8xf32>)
           -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }

  // Silicon: the hardware cycle counter reported 182.
  // CHECK-LABEL: func.func @fold_64
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: est_cycles = 182
  func.func @fold_64(%a: tensor<8x64xf32>, %b: tensor<64x8xf32>,
                     %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @fold_k64
         {m = 8 : i64, n = 8 : i64, k = 64 : i64}
         : (tensor<8x64xf32>, tensor<64x8xf32>, tensor<8x8xf32>)
           -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }

  // Two K tiles on the k=32 configuration: ceil(64/32) = 2 tiles, each
  // 150 -> 300. The fold design does pay the reduction drain once per
  // invocation, so charging fixedOverhead per tile is right for it. A
  // design that pipelined one tile's drain under the next tile's fill
  // would not be described correctly by this model -- that is a stated
  // limitation, not an accident of the constants.
  // CHECK-LABEL: func.func @fold_two_tiles
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: est_cycles = 300
  func.func @fold_two_tiles(%a: tensor<8x64xf32>, %b: tensor<64x8xf32>,
                            %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @fold_k32
         {m = 8 : i64, n = 8 : i64, k = 64 : i64}
         : (tensor<8x64xf32>, tensor<64x8xf32>, tensor<8x8xf32>)
           -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }

  // Same geometry, no calibration attribute: falls back to the HLS
  // default of 6 and gives 8 + 14 + 6 = 28. This is the whole point --
  // identical rows, cols and depth, 4.8x apart in cost, and the model can
  // only tell them apart because the constant is on the device.
  systolic.device @hls_k8 rows = 8 cols = 8 depth = 8
      dataflow = weight_stationary

  // CHECK-LABEL: func.func @hls_8
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: est_cycles = 28
  func.func @hls_8(%a: tensor<8x8xf32>, %b: tensor<8x8xf32>,
                   %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @hls_k8
         {m = 8 : i64, n = 8 : i64, k = 8 : i64}
         : (tensor<8x8xf32>, tensor<8x8xf32>, tensor<8x8xf32>)
           -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }
}
