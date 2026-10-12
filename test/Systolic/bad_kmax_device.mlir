// RUN: systolic-opt %s -split-input-file -verify-diagnostics

// k_max is a reduction capacity, any positive integer. Zero or a negative
// value is refused by the op itself, so no pass ever divides by it (the cost
// model used to cost such a device at zero cycles).

module {
  // expected-error @+1 {{'systolic.device' op k_max must be a positive integer, got 0}}
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 0 tile_overhead = 95
}

// -----

module {
  // expected-error @+1 {{'systolic.device' op k_max must be a positive integer, got -3}}
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = -3 tile_overhead = 95
}

// -----

// 1 and non-powers of two are capacities like any other.
module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 1 tile_overhead = 95
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 17 tile_overhead = 95
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 17 tile_overhead = 95
}
