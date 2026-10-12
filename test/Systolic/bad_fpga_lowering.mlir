// RUN: systolic-opt --lower-systolic-matmul-tile-to-fpga --split-input-file --verify-diagnostics %s

// A matmul_tile reaches the FPGA only as one legal invocation of a real
// device of the bitstream. Each case below is refused with a diagnostic and
// the module is left unlowered.

module {
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  func.func @unassigned(%a: tensor<4x4xf32>, %b: tensor<4x4xf32>, %c: tensor<4x4xf32>) -> tensor<4x4xf32> {
    // expected-error @+1 {{FPGA lowering requires an assigned accelerator}}
    %0 = systolic.matmul_tile %a, %b, %c m = 4 n = 4 k = 4
         : (tensor<4x4xf32>, tensor<4x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    return %0 : tensor<4x4xf32>
  }
}

// -----

module {
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  func.func @unscheduled(%a: tensor<4x4xf32>, %b: tensor<4x4xf32>, %c: tensor<4x4xf32>) -> tensor<4x4xf32> {
    // expected-error @+1 {{FPGA lowering requires a scheduled systolic.matmul_tile}}
    %0 = systolic.matmul_tile %a, %b, %c m = 4 n = 4 k = 4 on @acc_4x4_0
         : (tensor<4x4xf32>, tensor<4x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    return %0 : tensor<4x4xf32>
  }
}

// -----

module {
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  func.func @too_deep(%a: tensor<4x32xf32>, %b: tensor<32x4xf32>, %c: tensor<4x4xf32>) -> tensor<4x4xf32> {
    // expected-error @+1 {{tile reduction depth k = 32 exceeds k_max = 16 of @acc_4x4_0}}
    %0 = systolic.matmul_tile %a, %b, %c m = 4 n = 4 k = 32 on @acc_4x4_0 est_cycles = 101 start_cycle = 0
         : (tensor<4x32xf32>, tensor<32x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    return %0 : tensor<4x4xf32>
  }
}

// -----

module {
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  func.func @too_wide(%a: tensor<8x16xf32>, %b: tensor<16x8xf32>, %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    // expected-error @+1 {{tile 8x8 does not fit the assigned 4x4 accelerator @acc_4x4_0}}
    %0 = systolic.matmul_tile %a, %b, %c m = 8 n = 8 k = 16 on @acc_4x4_0 est_cycles = 101 start_cycle = 0
         : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }
}

// -----

module {
  systolic.device @acc_16x16 rows = 16 cols = 16 dataflow = output_stationary k_max = 16 tile_overhead = 95
  func.func @no_such_instance(%a: tensor<16x16xf32>, %b: tensor<16x16xf32>, %c: tensor<16x16xf32>) -> tensor<16x16xf32> {
    // expected-error @+1 {{systolic.device @acc_16x16 (16x16) has no physical FPGA device ID}}
    %0 = systolic.matmul_tile %a, %b, %c m = 16 n = 16 k = 16 on @acc_16x16 est_cycles = 125 start_cycle = 0
         : (tensor<16x16xf32>, tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    return %0 : tensor<16x16xf32>
  }
}

// -----

module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary tile_overhead = 95
  func.func @no_k_max(%a: tensor<8x16xf32>, %b: tensor<16x8xf32>, %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    // expected-error @+1 {{declare k_max on the systolic.device}}
    %0 = systolic.matmul_tile %a, %b, %c m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 109 start_cycle = 0
         : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }
}

// -----

// k_max is the IR's declaration; one invocation of the bitstream's arrays
// holds K_MAX reduction steps (16 by default, phys-k-max-8x8 otherwise)
// whatever the declaration says, and a declaration that differs from the
// hardware is refused.
module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 32 tile_overhead = 95
  func.func @deeper_than_bitstream(%a: tensor<8x32xf32>, %b: tensor<32x8xf32>, %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    // expected-error @+1 {{systolic.device @acc_8x8_0 declares k_max = 32, but the 8x8 accelerators of the loaded bitstream hold K_MAX = 16 reduction steps per invocation (phys-k-max-8x8)}}
    %0 = systolic.matmul_tile %a, %b, %c m = 8 n = 8 k = 32 on @acc_8x8_0 est_cycles = 109 start_cycle = 0
         : (tensor<8x32xf32>, tensor<32x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }
}
