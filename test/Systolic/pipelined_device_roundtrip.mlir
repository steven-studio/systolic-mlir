// RUN: systolic-opt %s | FileCheck %s
// RUN: systolic-opt %s | systolic-opt | FileCheck %s

// The pipelining attributes of systolic.device round-trip through the
// printer and the parser: the pipelined RTL's accelerators as systolic-matmul
// declares them from a bitstream record with PIPELINE_DEPTH, and depth 1 --
// the serialized hardware -- written out.
// CHECK: systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
// CHECK: systolic.device @serial rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 pipeline_depth = 1 reduce_cycles = 0 feed_gap = 0
systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
systolic.device @serial rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 pipeline_depth = 1 reduce_cycles = 0 feed_gap = 0
