// RUN: systolic-opt --systolic-binary-search-tile-matmul="report=true" --split-input-file -verify-diagnostics %s | FileCheck %s

// The binary search on the FPGA fleet whose contexts pipeline consecutive
// invocations (pipeline_depth / reduce_cycles / feed_gap, the 2026-10-18 RTL:
// Systolic/FpgaFleet.h). Every tile holds its accelerator for its interval on
// the per-accelerator timeline (CostModel.h, AcceleratorTimeline), so
// consecutive tiles of one accelerator overlap: the next tile's fill starts
// at the previous tile's fold, its fold while the previous tile is still
// being reduced. est_cycles keeps its meaning (the invocation's compute
// term); start_cycle is the fold's start on the compute-only timeline.
//
// The same shapes on the serialized fleet are binary_search_rcw.mlir and
// kfold_variable_k.mlir: 16x16x16 took 1245 there, 8x8x65 on K_MAX 32
// started its invocations at 0 / 141 / 282.

// 16x16x16 at K_MAX 16: the second 8x8 tile's fill starts at the first one's
// fold (267) and the tile ends at 750 instead of 966; each 4x4 runs its
// tiles 267 apart instead of 415. Compute-only: 42-cycle folds (k + the
// 26-cycle gap) with the first result at 125, the third 4x4 tile's fold at
// 120 = the first one's publish (117) + 3.
// CHECK-LABEL: func.func @mm_16x16x16
// CHECK: m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 0
// CHECK: m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 42
// CHECK: m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 0
// CHECK: m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 0
// CHECK: m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 0
// CHECK: m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 42
// CHECK: m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 42
// CHECK: m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 42
// CHECK: m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 120
// CHECK: m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 120
// CHECK-NOT: systolic.matmul_tile
module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  func.func @mm_16x16x16(%A: tensor<16x16xf32>, %B: tensor<16x16xf32>, %C: tensor<16x16xf32>) -> tensor<16x16xf32> {
    // expected-remark @+1 {{binary-search schedule 16x16x16: makespan = 949 cycles (read + compute + write), compute-only = 255, schedule end = 949, operand bytes read = 10240, shared-read cycles = 0, tiles = 10}}
    %0 = linalg.matmul ins(%A, %B : tensor<16x16xf32>, tensor<16x16xf32>) outs(%C : tensor<16x16xf32>) -> tensor<16x16xf32>
    return %0 : tensor<16x16xf32>
  }
}

// -----

// A 65-step reduction on the 8x8 at K_MAX 16: five chained invocations
// (16, 16, 16, 16, 1) of one output block. Compute-only, by hand: fold 1
// at 0 + 16 + 26 = 42; result 0 at 125; fold 2 at 125 + 3 = 128 (tile 0
// published, its accumulator set free); result 1 at max(42 + 125, 125 + 69)
// = 194; fold 3 at 197; result 2 at max(253, 263) = 263; fold 4 at 266;
// results 3, 4 at 332 and max(266 + 110, 332 + 69) = 401 -- the reduction
// tree, 69 cycles per invocation, is what the chain waits on. Serialized the
// same chain took 0 / 125 / 250 / 375 / 500 and 610 cycles.
// CHECK-LABEL: func.func @mm_8x8x65
// CHECK: m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 0
// CHECK: m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 42
// CHECK: m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 128
// CHECK: m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 197
// CHECK: m = 8 n = 8 k = 1 on @acc_8x8_0 est_cycles = 110 start_cycle = 266
// CHECK-NOT: systolic.matmul_tile
module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  func.func @mm_8x8x65(%A: tensor<8x65xf32>, %B: tensor<65x8xf32>, %C: tensor<8x8xf32>) -> tensor<8x8xf32> {
    // expected-remark @+1 {{binary-search schedule 8x8x65: makespan = 1408 cycles (read + compute + write), compute-only = 401, schedule end = 1408, operand bytes read = 4608, shared-read cycles = 0, tiles = 1}}
    %0 = linalg.matmul ins(%A, %B : tensor<8x65xf32>, tensor<65x8xf32>) outs(%C : tensor<8x8xf32>) -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }
}

// -----

// Depth 1 declared explicitly is the serialized fleet to the cycle: the
// numbers of binary_search_rcw.mlir's first case.
// CHECK-LABEL: func.func @mm_16x16x16_depth1
// CHECK: m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 0
// CHECK: m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 125
// CHECK: m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 0
module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 1 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 1 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 1 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 1 reduce_cycles = 69 feed_gap = 26
  func.func @mm_16x16x16_depth1(%A: tensor<16x16xf32>, %B: tensor<16x16xf32>, %C: tensor<16x16xf32>) -> tensor<16x16xf32> {
    // expected-remark @+1 {{binary-search schedule 16x16x16: makespan = 1245 cycles (read + compute + write), compute-only = 351, schedule end = 1245, operand bytes read = 10240, shared-read cycles = 0, tiles = 10}}
    %0 = linalg.matmul ins(%A, %B : tensor<16x16xf32>, tensor<16x16xf32>) outs(%C : tensor<16x16xf32>) -> tensor<16x16xf32>
    return %0 : tensor<16x16xf32>
  }
}
