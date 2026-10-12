// RUN: systolic-opt --lower-systolic-matmul-tile-to-fpga="phys-k-max-8x8=17 phys-k-max-4x4=7" %s -split-input-file -verify-diagnostics
// RUN: not systolic-opt --lower-systolic-matmul-tile-to-fpga="phys-k-max-8x8=0 phys-k-max-4x4=7" %s -split-input-file 2>&1 | FileCheck %s --check-prefix=OPTION
// RUN: not systolic-opt --lower-systolic-matmul-tile-to-fpga="phys-k-max-8x8=17 phys-k-max-4x4=16377" %s -split-input-file 2>&1 | FileCheck %s --check-prefix=OPTION-HI

// Three different K's meet in the FPGA lowering:
//   IR-declared k_max   the systolic.device attribute
//   physical K_MAX      the loaded bitstream's, per geometry
//                       (phys-k-max-8x8 = K_MAX_8X8, phys-k-max-4x4 = K_MAX_4X4)
//   invocation k        the matmul_tile's k
// The declaration must equal the hardware, and every invocation must fit it.

// The physical K_MAX is a positive integer no deeper than the DMA protocol
// can move (docs/KMAX.md, section 4).
// OPTION: phys-k-max-8x8 and phys-k-max-4x4 must be integers in 1..16376 (the DMA descriptor counts an operand slab's beats in 16 bits), got 0 and 7
// OPTION-HI: phys-k-max-8x8 and phys-k-max-4x4 must be integers in 1..16376 (the DMA descriptor counts an operand slab's beats in 16 bits), got 17 and 16377

// The bitstream's 8x8 holds 17; a fleet that says 16 describes another one.
module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95
  func.func @declared_below_hardware(%a: tensor<8x16xf32>, %b: tensor<16x8xf32>, %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    // expected-error @+1 {{systolic.device @acc_8x8_0 declares k_max = 16, but the 8x8 accelerators of the loaded bitstream hold K_MAX = 17 reduction steps per invocation (phys-k-max-8x8)}}
    %0 = systolic.matmul_tile %a, %b, %c m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 109 start_cycle = 0
         : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }
}

// -----

// The 4x4s hold 7; declaring 8 would let an invocation of 8 through.
module {
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 8 tile_overhead = 95
  func.func @declared_above_hardware(%a: tensor<4x8xf32>, %b: tensor<8x4xf32>, %c: tensor<4x4xf32>) -> tensor<4x4xf32> {
    // expected-error @+1 {{systolic.device @acc_4x4_0 declares k_max = 8, but the 4x4 accelerators of the loaded bitstream hold K_MAX = 7 reduction steps per invocation (phys-k-max-4x4)}}
    %0 = systolic.matmul_tile %a, %b, %c m = 4 n = 4 k = 8 on @acc_4x4_0 est_cycles = 101 start_cycle = 0
         : (tensor<4x8xf32>, tensor<8x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    return %0 : tensor<4x4xf32>
  }
}

// -----

// Declaration and hardware agree (7), the invocation does not fit them.
module {
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 7 tile_overhead = 95
  func.func @invocation_too_deep(%a: tensor<4x8xf32>, %b: tensor<8x4xf32>, %c: tensor<4x4xf32>) -> tensor<4x4xf32> {
    // expected-error @+1 {{tile reduction depth k = 8 exceeds k_max = 7 of @acc_4x4_2}}
    %0 = systolic.matmul_tile %a, %b, %c m = 4 n = 4 k = 8 on @acc_4x4_2 est_cycles = 101 start_cycle = 0
         : (tensor<4x8xf32>, tensor<8x4xf32>, tensor<4x4xf32>) -> tensor<4x4xf32>
    return %0 : tensor<4x4xf32>
  }
}

// -----

// Only the 8x8 runs a tile, but the decomposition also costed the 4x4s with
// their declared k_max: a declaration that is not the bitstream's is refused
// on a device that ends up with no tile as well.
module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 17 tile_overhead = 95
  // expected-error @+1 {{systolic.device @acc_4x4_0 declares k_max = 1, but the 4x4 accelerators of the loaded bitstream hold K_MAX = 7 reduction steps per invocation (phys-k-max-4x4)}}
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 1 tile_overhead = 95
  // expected-error @+1 {{FPGA lowering needs the operand-buffer capacity of @acc_4x4_1; declare k_max on the systolic.device}}
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary tile_overhead = 95
  // Not one of the bitstream's arrays, and nothing runs on it: not checked.
  systolic.device @acc_16x16 rows = 16 cols = 16 dataflow = output_stationary k_max = 3 tile_overhead = 95
  func.func @unused_devices(%a: tensor<8x17xf32>, %b: tensor<17x8xf32>, %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = systolic.matmul_tile %a, %b, %c m = 8 n = 8 k = 17 on @acc_8x8_0 est_cycles = 109 start_cycle = 0
         : (tensor<8x17xf32>, tensor<17x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }
}
