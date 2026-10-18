// RUN: systolic-opt %s -split-input-file -verify-diagnostics
// RUN: systolic-opt --systolic-binary-search-tile-matmul %S/Inputs/mixed_pipeline_fleet.mlir -verify-diagnostics

// The pipelining attributes of systolic.device (CostModel.h,
// AcceleratorTimeline): how one context runs consecutive invocations. The
// three describe one piece of hardware -- the job queue, the serial part of
// tile_overhead, the PE's boundary gap -- and come together or not at all; a
// device without them is costed as the serialized hardware.

// expected-error @+1 {{pipeline_depth, reduce_cycles and feed_gap describe one pipelined accelerator and must be declared together}}
systolic.device @a rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95 pipeline_depth = 4

// -----

// expected-error @+1 {{pipeline_depth, reduce_cycles and feed_gap describe one pipelined accelerator and must be declared together}}
systolic.device @a rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95 reduce_cycles = 69 feed_gap = 26

// -----

// expected-error @+1 {{pipeline_depth must be a positive integer, got 0}}
systolic.device @a rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95 pipeline_depth = 0 reduce_cycles = 69 feed_gap = 26

// -----

// expected-error @+1 {{reduce_cycles must be non-negative, got -1}}
systolic.device @a rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95 pipeline_depth = 4 reduce_cycles = -1 feed_gap = 26

// -----

// expected-error @+1 {{feed_gap must be non-negative, got -26}}
systolic.device @a rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95 pipeline_depth = 4 reduce_cycles = 69 feed_gap = -26

// -----

// The serial reduction is part of H: it cannot exceed it.
// expected-error @+1 {{reduce_cycles is the serial part of tile_overhead and cannot exceed it: 96 > 95}}
systolic.device @a rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95 pipeline_depth = 4 reduce_cycles = 96 feed_gap = 26

// -----

// Without tile_overhead H = 0, so only reduce_cycles = 0 fits.
// expected-error @+1 {{reduce_cycles is the serial part of tile_overhead and cannot exceed it: 1 > 0}}
systolic.device @a rows = 8 cols = 8 dataflow = output_stationary k_max = 16 pipeline_depth = 2 reduce_cycles = 1 feed_gap = 0
