// RUN: systolic-opt --systolic-cost-analysis %s | FileCheck %s

// Pins the cost model to the hand-written 8x8 fold RTL at tag paper-hw-v1
// (BRAM accumulator, two adders per PE), whose calibration differs from
// the HLS pipeline in cost_analysis.mlir by more than 15x.
//
//   cycles = k + ceil(k / k_max) * (rows + cols - 2 + tile_overhead)
//   tile_overhead = H = 95, rows = cols = 8   =>  k + 109 per invocation
//
// Why 95 and not the 109 the hardware counter reports per invocation: the
// measured number is end-to-end, and the geometric term already accounts
// for rows + cols - 2 = 14. H was calibrated on the board at k = 16, 64
// and 128 (125, 173, 237 cycles, residual zero); everything else below is
// a held-out point that was measured afterwards and matched to the cycle.
//
// The 95 is the datapath, not the array: 22 cycles of pipeline drain, 67
// for the four-level bank-reduction tree (8, 4, 2, 1 additions through one
// pipelined adder, each level paying a block-RAM read, the 11-cycle adder
// and one barrier cycle) and 6 of result hand-off. None of those terms
// depends on rows, cols or k_max -- the same 95 holds on the N = 4 build --
// which is why a single constant is the right shape for them. The split is
// measured in rtl/multi_200t/fold_pipelined/tb/tb_array_h_decomp.sv.
//
// `k_max` is the reduction capacity the bitstream is synthesized for
// (K_MAX of uart/build_kmax.tcl). A fold deeper than k_max is issued as
// ceil(k / k_max) invocations, and the last one runs at whatever depth is
// left, so the arithmetic term is exactly k however the fold is split.
//
// Provenance: every board figure below comes from the on-chip cycle counter
// through tools/measure_fold.py on the Nexys Video (xc7a200t), one
// bitstream per k_max. @hls_8 and @geom_8 are not board points.

module {
  systolic.device @fold_k256 rows = 8 cols = 8
      dataflow = output_stationary {k_max = 256 : i64, tile_overhead = 95 : i64}
  systolic.device @fold_k512 rows = 8 cols = 8
      dataflow = output_stationary {k_max = 512 : i64, tile_overhead = 95 : i64}
  systolic.device @fold_k1024 rows = 8 cols = 8
      dataflow = output_stationary {k_max = 1024 : i64, tile_overhead = 95 : i64}
  systolic.device @fold_k2048 rows = 8 cols = 8
      dataflow = output_stationary {k_max = 2048 : i64, tile_overhead = 95 : i64}

  // ---- single invocation, k <= k_max ----------------------------------

  // Board: 117 (held out). 8 + 14 + 95.
  // CHECK-LABEL: func.func @fold_8
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: est_cycles = 117
  func.func @fold_8(%a: tensor<8x8xf32>, %b: tensor<8x8xf32>,
                    %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @fold_k256
         {m = 8 : i64, n = 8 : i64, k = 8 : i64}
         : (tensor<8x8xf32>, tensor<8x8xf32>, tensor<8x8xf32>)
           -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }

  // Board: 125 (calibration point).
  // CHECK-LABEL: func.func @fold_16
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: est_cycles = 125
  func.func @fold_16(%a: tensor<8x16xf32>, %b: tensor<16x8xf32>,
                     %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @fold_k256
         {m = 8 : i64, n = 8 : i64, k = 16 : i64}
         : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>)
           -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }

  // Board: 141 (held out).
  // CHECK-LABEL: func.func @fold_32
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: est_cycles = 141
  func.func @fold_32(%a: tensor<8x32xf32>, %b: tensor<32x8xf32>,
                     %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @fold_k256
         {m = 8 : i64, n = 8 : i64, k = 32 : i64}
         : (tensor<8x32xf32>, tensor<32x8xf32>, tensor<8x8xf32>)
           -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }

  // Board: 173 (calibration point).
  // CHECK-LABEL: func.func @fold_64
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: est_cycles = 173
  func.func @fold_64(%a: tensor<8x64xf32>, %b: tensor<64x8xf32>,
                     %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @fold_k256
         {m = 8 : i64, n = 8 : i64, k = 64 : i64}
         : (tensor<8x64xf32>, tensor<64x8xf32>, tensor<8x8xf32>)
           -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }

  // Board: 205 (held out).
  // CHECK-LABEL: func.func @fold_96
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: est_cycles = 205
  func.func @fold_96(%a: tensor<8x96xf32>, %b: tensor<96x8xf32>,
                     %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @fold_k256
         {m = 8 : i64, n = 8 : i64, k = 96 : i64}
         : (tensor<8x96xf32>, tensor<96x8xf32>, tensor<8x8xf32>)
           -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }

  // Board: 237 (calibration point).
  // CHECK-LABEL: func.func @fold_128
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: est_cycles = 237
  func.func @fold_128(%a: tensor<8x128xf32>, %b: tensor<128x8xf32>,
                      %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @fold_k256
         {m = 8 : i64, n = 8 : i64, k = 128 : i64}
         : (tensor<8x128xf32>, tensor<128x8xf32>, tensor<8x8xf32>)
           -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }

  // Board: 301 (held out).
  // CHECK-LABEL: func.func @fold_192
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: est_cycles = 301
  func.func @fold_192(%a: tensor<8x192xf32>, %b: tensor<192x8xf32>,
                      %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @fold_k256
         {m = 8 : i64, n = 8 : i64, k = 192 : i64}
         : (tensor<8x192xf32>, tensor<192x8xf32>, tensor<8x8xf32>)
           -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }

  // Board: 365 (held out; the buffer exactly full).
  // CHECK-LABEL: func.func @fold_256
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: est_cycles = 365
  func.func @fold_256(%a: tensor<8x256xf32>, %b: tensor<256x8xf32>,
                      %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @fold_k256
         {m = 8 : i64, n = 8 : i64, k = 256 : i64}
         : (tensor<8x256xf32>, tensor<256x8xf32>, tensor<8x8xf32>)
           -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }

  // Board: 621 on the k_max = 512 bitstream. Same H on a different
  // capacity: 512 + 109.
  // CHECK-LABEL: func.func @fold_512
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: est_cycles = 621
  func.func @fold_512(%a: tensor<8x512xf32>, %b: tensor<512x8xf32>,
                      %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @fold_k512
         {m = 8 : i64, n = 8 : i64, k = 512 : i64}
         : (tensor<8x512xf32>, tensor<512x8xf32>, tensor<8x8xf32>)
           -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }

  // Board: 1133 on the k_max = 1024 bitstream. 1024 + 109.
  // CHECK-LABEL: func.func @fold_1024
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: est_cycles = 1133
  func.func @fold_1024(%a: tensor<8x1024xf32>, %b: tensor<1024x8xf32>,
                       %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @fold_k1024
         {m = 8 : i64, n = 8 : i64, k = 1024 : i64}
         : (tensor<8x1024xf32>, tensor<1024x8xf32>, tensor<8x8xf32>)
           -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }

  // ---- several invocations, k > k_max ---------------------------------
  //
  // The fold design pays the per-invocation cost once per invocation, and
  // the last invocation runs at its true depth: K = 576 on k_max = 256 is
  // 256 + 256 + 64, so 365 + 365 + 173 = 903, not 3 x 365. Charging every
  // invocation at k_max would give 1095 here and 2555 below, and the board
  // says 903 and 2491. These two depths are ResNet-18 layer1.0.conv1 and
  // VGG-16 features.6 after im2col, the survey rows of the thesis.

  // Board: 903 = 576 + 3 * 109.
  // CHECK-LABEL: func.func @fold_576_k256
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: est_cycles = 903
  func.func @fold_576_k256(%a: tensor<8x576xf32>, %b: tensor<576x8xf32>,
                           %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @fold_k256
         {m = 8 : i64, n = 8 : i64, k = 576 : i64}
         : (tensor<8x576xf32>, tensor<576x8xf32>, tensor<8x8xf32>)
           -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }

  // Board: 685 = 576 + 109 -- the same depth in one invocation on the
  // k_max = 1024 bitstream. The 218-cycle difference between this line and
  // the one above is the whole argument for buying capacity.
  // CHECK-LABEL: func.func @fold_576_k1024
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: est_cycles = 685
  func.func @fold_576_k1024(%a: tensor<8x576xf32>, %b: tensor<576x8xf32>,
                            %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @fold_k1024
         {m = 8 : i64, n = 8 : i64, k = 576 : i64}
         : (tensor<8x576xf32>, tensor<576x8xf32>, tensor<8x8xf32>)
           -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }

  // Board: 2491 = 1728 + 7 * 109 (six full invocations and one of 192).
  // CHECK-LABEL: func.func @fold_1728_k256
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: est_cycles = 2491
  func.func @fold_1728_k256(%a: tensor<8x1728xf32>, %b: tensor<1728x8xf32>,
                            %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @fold_k256
         {m = 8 : i64, n = 8 : i64, k = 1728 : i64}
         : (tensor<8x1728xf32>, tensor<1728x8xf32>, tensor<8x8xf32>)
           -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }

  // Board: 1837 = 1728 + 109 on the k_max = 2048 bitstream.
  // CHECK-LABEL: func.func @fold_1728_k2048
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: est_cycles = 1837
  func.func @fold_1728_k2048(%a: tensor<8x1728xf32>, %b: tensor<1728x8xf32>,
                             %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @fold_k2048
         {m = 8 : i64, n = 8 : i64, k = 1728 : i64}
         : (tensor<8x1728xf32>, tensor<1728x8xf32>, tensor<8x8xf32>)
           -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }

  // ---- the same geometry under other maps -----------------------------

  // The HLS calibration instead of the fold one: 8 + 14 + 6 = 28. This is
  // the whole point -- identical rows, cols and k_max, 4x apart in cost,
  // and the model can only tell them apart because the constant is on the
  // device.
  systolic.device @hls_k8 rows = 8 cols = 8
      dataflow = weight_stationary {k_max = 8 : i64, tile_overhead = 6 : i64}

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

  // No tile_overhead at all: H = 0, which is exactly the geometric model,
  // 8 + 14 + 0 = 22. The difference between the calibrated map and the
  // geometric one is this one attribute and nothing else.
  systolic.device @geom_k8 rows = 8 cols = 8
      dataflow = weight_stationary {k_max = 8 : i64}

  // CHECK-LABEL: func.func @geom_8
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: est_cycles = 22
  func.func @geom_8(%a: tensor<8x8xf32>, %b: tensor<8x8xf32>,
                    %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @geom_k8
         {m = 8 : i64, n = 8 : i64, k = 8 : i64}
         : (tensor<8x8xf32>, tensor<8x8xf32>, tensor<8x8xf32>)
           -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }
}
