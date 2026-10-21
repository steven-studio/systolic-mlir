// RUN: systolic-opt --systolic-binary-search-tile-matmul="report=true cost-model=v1 shared-read-bytes-per-cycle=4.0" -verify-diagnostics %s | FileCheck %s
// RUN: systolic-opt --systolic-binary-search-tile-matmul="report=true cost-model=v1 shared-read-bytes-per-cycle=16.0" %s 2>&1 >/dev/null | FileCheck %s --check-prefix=WIDE

// The FPGA fills every array's operand buffer through ONE AXI read port
// (dma_engine_multi): the fills of all four arrays together cannot move more
// than the port does, however fast each array's own writer is. With
// shared-read-bytes-per-cycle the search also requires the operand bytes of
// the whole schedule to fit that bandwidth by the deadline; without it every
// array fills as if it had a port of its own (binary_search_rcw.mlir).
//
// The fleet is systolic-matmul's at K_MAX_8X8 = K_MAX_4X4 = 16: one
// invocation reads 1024 B on either geometry (the slab is 8 lanes wide for
// the 4x4s too) and costs 483 on the 8x8, 415 on a 4x4.
//
// 16x16x16 at 4 B/cycle shared. Independently, two 8x8 tiles and eight 4x4
// tiles finish at 1245 -- but ten tiles read 10240 B, 2560 cycles through
// the shared port. An 8x8 tile reads what a 4x4 tile reads and covers four
// times the output: with three 8x8 tiles and four 4x4 tiles the schedule
// reads 7168 B, 1792 cycles, and that is the makespan. The arrays
// themselves are done at 1449 (schedule end).
// CHECK-LABEL: func.func @mm_16x16x16
// CHECK: m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 0
// CHECK: m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 125
// CHECK: m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 250
// CHECK: m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 0
// CHECK: m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 0
// CHECK: m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 0
// CHECK: m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 117
// CHECK-NOT: systolic.matmul_tile
//
// At 16 B/cycle -- four fills at their full rate at once -- the shared port
// carries the independent schedule's 10240 B in 640 cycles, well inside
// 1245: the decomposition and the makespan are the independent ones.
// WIDE: remark: binary-search schedule 16x16x16: makespan = 1245 cycles (read + compute + write), compute-only = 351, schedule end = 1245, operand bytes read = 10240, shared-read cycles = 640, tiles = 10
module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  func.func @mm_16x16x16(%A: tensor<16x16xf32>, %B: tensor<16x16xf32>, %C: tensor<16x16xf32>) -> tensor<16x16xf32> {
    // expected-remark @+1 {{binary-search schedule 16x16x16: makespan = 1792 cycles (read + compute + write), compute-only = 375, schedule end = 1449, operand bytes read = 7168, shared-read cycles = 1792, tiles = 7}}
    %0 = linalg.matmul ins(%A, %B : tensor<16x16xf32>, tensor<16x16xf32>) outs(%C : tensor<16x16xf32>) -> tensor<16x16xf32>
    return %0 : tensor<16x16xf32>
  }
}
