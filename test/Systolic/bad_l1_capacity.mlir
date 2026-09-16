// RUN: systolic-opt --systolic-select-device -verify-diagnostics %s

// A tile that fits no declared device is an error, not a silent fallback:
// the remedy is to re-tile it (or to raise l1_bytes), and a schedule that
// placed it anyway would describe an execution the hardware cannot perform.
// 64^3 f32 needs 2 * 64*64 * 4 = 32768 bytes for A and B; the only device
// declares 1024.
module {
  systolic.device @tiny rows = 8 cols = 8 dataflow = output_stationary
      {k_max = 256 : i64, tile_overhead = 95 : i64, l1_bytes = 1024 : i64}

  func.func @too_big(%a: tensor<64x64xf32>, %b: tensor<64x64xf32>,
                     %c: tensor<64x64xf32>) -> tensor<64x64xf32> {
    // expected-error @+1 {{no declared systolic.device can hold this tile's operands (32768 bytes)}}
    %0 = systolic.matmul_tile %a, %b, %c
         {m = 64 : i64, n = 64 : i64, k = 64 : i64}
         : (tensor<64x64xf32>, tensor<64x64xf32>, tensor<64x64xf32>)
           -> tensor<64x64xf32>
    return %0 : tensor<64x64xf32>
  }
}
