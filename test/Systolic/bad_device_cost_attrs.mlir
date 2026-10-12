// RUN: systolic-opt %s -split-input-file -verify-diagnostics
// RUN: systolic-opt --systolic-binary-search-tile-matmul %S/Inputs/mixed_cost_fleet.mlir -verify-diagnostics

// The read / write terms of systolic.device (CostModel.h) are checked once,
// by the op's verifier: a bandwidth is a divisor, a lane count or window a
// multiplier of the bytes moved, and an overhead below zero would make a
// transfer cheaper than free.

// expected-error @+1 {{read_bytes_per_cycle must be positive and finite, got 0}}
systolic.device @a rows = 8 cols = 8 dataflow = output_stationary k_max = 16 read_bytes_per_cycle = 0.0

// -----

// expected-error @+1 {{write_bytes_per_cycle must be positive and finite, got -3.2}}
systolic.device @a rows = 8 cols = 8 dataflow = output_stationary k_max = 16 write_bytes_per_cycle = -3.2

// -----

// expected-error @+1 {{read_overhead must be non-negative, got -1}}
systolic.device @a rows = 8 cols = 8 dataflow = output_stationary k_max = 16 read_overhead = -1

// -----

// expected-error @+1 {{write_overhead must be non-negative, got -11}}
systolic.device @a rows = 8 cols = 8 dataflow = output_stationary k_max = 16 write_overhead = -11

// -----

// expected-error @+1 {{k_window must be a positive integer, got 0}}
systolic.device @a rows = 8 cols = 8 dataflow = output_stationary k_max = 16 k_window = 0

// -----

// The wire must carry at least the array's lanes per reduction step.
// expected-error @+1 {{operand_lanes must cover the array's 8 lanes, got 4}}
systolic.device @a rows = 8 cols = 8 dataflow = output_stationary k_max = 16 operand_lanes = 4

// -----

// A 4x4 with 8 lanes on the wire (the FPGA's slab) is fine.
systolic.device @a rows = 4 cols = 4 dataflow = output_stationary k_max = 7 operand_lanes = 8 k_window = 8
