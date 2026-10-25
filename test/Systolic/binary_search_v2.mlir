// RUN: systolic-opt --systolic-binary-search-tile-matmul="report=true" --split-input-file %s 2>&1 >/dev/null | FileCheck %s --check-prefix=V2
// RUN: systolic-opt --systolic-binary-search-tile-matmul="report=true cost-model=v2" --split-input-file %s 2>&1 >/dev/null | FileCheck %s --check-prefix=V2
// RUN: systolic-opt --systolic-binary-search-tile-matmul="report=true cost-model=v1" --split-input-file %s 2>&1 >/dev/null | FileCheck %s --check-prefix=V1
// RUN: systolic-opt --systolic-binary-search-tile-matmul --split-input-file %s | FileCheck %s --check-prefix=IR
// RUN: not systolic-opt --systolic-binary-search-tile-matmul="cost-model=v3" --split-input-file %s 2>&1 | FileCheck %s --check-prefix=BAD
// RUN: not systolic-opt --systolic-binary-search-tile-matmul="operand-path=v9" --split-input-file %s 2>&1 | FileCheck %s --check-prefix=BADPATH

// Cost Model V2 is the binary search's default (docs/COSTMODEL_V2.md); V1
// stays selectable with cost-model=v1 and prices exactly as before
// (binary_search_rcw.mlir, binary_search_pipelined.mlir).
//
// Revision 2 timings; the pipelined schedule is replayed in the RTL
// differential validation (run_validation.py credit).
// The numbers of the V2 remarks are the C++ fleet simulator's
// (CostModelV2.cpp). The simulator reproduces the RTL bench to the cycle on
// every validated job list (cost_model_v2_rtl.mlir, eval/cost_model_v2);
// these particular schedules are checked against the RTL bench by
// eval/cost_model_v2/run_validation.py (the `gemm` family).
//
// "planning makespan" is the search's deadline T*: every geometry's tile
// stream measured with the whole fleet busy (saturated). "predicted device
// makespan" is the chosen schedule itself on the fleet simulator, the tiles
// offered as the asynchronous runtime offers them. The two are different
// scenarios; neither is a general bound on the other. The second case
// below is 2489 planned vs 2473 predicted on Revision 2.

// 16x16x16, K_MAX 16/16, the serialized fleet. The same decomposition as
// V1 (two 8x8 tiles, eight 4x4 tiles), priced 1915 instead of 1245: the
// fills contend for the one read port and every result is read back through
// the DPTI output path.
// V2: remark: binary-search schedule 16x16x16: makespan = 1915 cycles (read + compute + write), compute-only = 351, schedule end = 1915, operand bytes read = 10240, shared-read cycles = 0, tiles = 10
// V2: remark: cost model v2: planning makespan = 1915 cycles (saturated fleet), predicted device makespan = 1902, readback end = 3970, AXI read requests = 626, operand bytes = 10240, contention cycles = 2168
// V1: remark: binary-search schedule 16x16x16: makespan = 1245 cycles (read + compute + write), compute-only = 351, schedule end = 1245, operand bytes read = 10240, shared-read cycles = 0, tiles = 10
// V1-NOT: cost model
// IR-LABEL: func.func @mm_16x16x16
// IR: m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 0
// IR: m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 125
// IR: m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 0
// IR: m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 0
// IR: m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 0
// IR: m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 117
// IR: m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 117
// IR: m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 117
// IR: m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 234
// IR: m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 234
// IR-NOT: systolic.matmul_tile
// BAD: error: cost-model must be v1 or v2, got 'v3'
// BADPATH: error: operand-path must be v1 or v2, got 'v9'

module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  func.func @mm_16x16x16(%A: tensor<16x16xf32>, %B: tensor<16x16xf32>, %C: tensor<16x16xf32>) -> tensor<16x16xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<16x16xf32>, tensor<16x16xf32>) outs(%C : tensor<16x16xf32>) -> tensor<16x16xf32>
    return %0 : tensor<16x16xf32>
  }
}

// -----

// 16x16x33, K_MAX 16/16, the pipelined fleet (job queue of 4): K folds into
// 16 + 16 + 1. V2 runs three 8x8 tiles and four 4x4 tiles; start_cycle is
// still the compute-only timeline (the hardware scheduler holds folds on it,
// so its meaning does not change with the cost model).
// V2: remark: binary-search schedule 16x16x33: makespan = 2489 cycles (read + compute + write), compute-only = 677, schedule end = 2489, operand bytes read = 17920, shared-read cycles = 0, tiles = 7
// V2: remark: cost model v2: planning makespan = 2489 cycles (saturated fleet), predicted device makespan = 2473, readback end = 11383, AXI read requests = 1049, operand bytes = 17920, contention cycles = 1229
// V1: remark: binary-search schedule 16x16x33:
// V1-NOT: cost model
// IR-LABEL: func.func @mm_16x16x33
// IR: m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 0
// IR: m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 42
// IR: m = 8 n = 8 k = 1 on @acc_8x8_0 est_cycles = 110 start_cycle = 128

module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8 pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
  func.func @mm_16x16x33(%A: tensor<16x33xf32>, %B: tensor<33x16xf32>, %C: tensor<16x16xf32>) -> tensor<16x16xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<16x33xf32>, tensor<33x16xf32>) outs(%C : tensor<16x16xf32>) -> tensor<16x16xf32>
    return %0 : tensor<16x16xf32>
  }
}

// -----

// A fleet without the FPGA transport declaration (compute-only devices):
// nothing for the fleet simulator to model. V2 asked for, V1 prices it and
// the remark says why; asked for V1, no extra remark.
// V2: remark: binary-search schedule 16x16x16: makespan = 468 cycles
// V2: remark: cost model v1 (v2 cannot model this fleet: an accelerator without the FPGA transport declaration (read_bytes_per_cycle / write_bytes_per_cycle))
// V1: remark: binary-search schedule 16x16x16: makespan = 468 cycles
// V1-NOT: cost model

module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  func.func @mm_compute_only(%A: tensor<16x16xf32>, %B: tensor<16x16xf32>, %C: tensor<16x16xf32>) -> tensor<16x16xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<16x16xf32>, tensor<16x16xf32>) outs(%C : tensor<16x16xf32>) -> tensor<16x16xf32>
    return %0 : tensor<16x16xf32>
  }
}
