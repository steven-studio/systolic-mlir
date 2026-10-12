// RUN: systolic-opt --systolic-binary-search-tile-matmul="report=true" --split-input-file -verify-diagnostics %s | FileCheck %s
// RUN: systolic-opt %s --split-input-file | systolic-opt --split-input-file | FileCheck %s --check-prefix=ROUND

// The binary search on the FPGA fleet as systolic-matmul declares it
// (Systolic/FpgaFleet.h), with the read and write terms of the cost model
// and variable-K execution: K_MAX is the operand buffer's capacity, and
// invocation i of a K-step reduction runs exactly k_i = min(K_MAX,
// K - i * K_MAX) steps (the runtime sends job.k = k_i). One invocation costs
// read + compute + write (CostModel.h):
//
//   read    = 512 B * ceil(k_i / 8) / 4.0 B/cycle + 11 = 128 * ceil(k_i / 8) + 11
//   compute = k_i + 2g - 2 + 95
//   write   = g * g * 4 B / 3.2 B/cycle + 11           (91 on the 8x8, 31 on a 4x4)
//
// The decomposition balances those. The IR carries what the hardware
// scheduler is given: est_cycles = the invocation's compute term,
// start_cycle = on each device, the compute terms before it.

// ROUND: systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = 3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8

// 16x16x16 at K_MAX 16/16: one invocation of 16 steps per tile,
// 267 + 125 + 91 = 483 on the 8x8, 267 + 117 + 31 = 415 on a 4x4. Two 8x8
// tiles (966) and eight 4x4 tiles in three rounds (3 x 415 = 1245) --
// makespan 1245; three 8x8 tiles would end at 1449. Compute-only, the same
// schedule ends at max(2 x 125, 3 x 117) = 351.
// CHECK-LABEL: func.func @mm_16x16x16
// CHECK: m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 0
// CHECK: m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 125
// CHECK: m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 0
// CHECK: m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 0
// CHECK: m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 0
// CHECK: m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 117
// CHECK: m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 117
// CHECK: m = 4 n = 4 k = 16 on @acc_4x4_2 est_cycles = 117 start_cycle = 117
// CHECK: m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 234
// CHECK: m = 4 n = 4 k = 16 on @acc_4x4_1 est_cycles = 117 start_cycle = 234
// CHECK-NOT: systolic.matmul_tile

module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  func.func @mm_16x16x16(%A: tensor<16x16xf32>, %B: tensor<16x16xf32>, %C: tensor<16x16xf32>) -> tensor<16x16xf32> {
    // expected-remark @+1 {{binary-search schedule 16x16x16: makespan = 1245 cycles (read + compute + write), compute-only = 351, schedule end = 1245, operand bytes read = 10240, shared-read cycles = 0, tiles = 10}}
    %0 = linalg.matmul ins(%A, %B : tensor<16x16xf32>, tensor<16x16xf32>) outs(%C : tensor<16x16xf32>) -> tensor<16x16xf32>
    return %0 : tensor<16x16xf32>
  }
}

// -----

// K_MAX_8X8 = 16, K_MAX_4X4 = 7, 8x8x17. The 8x8 runs 16 + 1: 483, then
// one step in one window, 139 + (1 + 109) + 91 = 340 -- 823 for the tile,
// and the two invocations are NOT an equal split: est_cycles 125 then 110,
// starting at 0 and 125. A 4x4 needs k = 7, 7, 3 (278 + 278 + 274 = 830 per
// tile); four of them on three arrays take 1660. The 8x8 takes the GEMM.
// CHECK-LABEL: func.func @mm_8x8x17
// CHECK: m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 0
// CHECK: m = 8 n = 8 k = 1 on @acc_8x8_0 est_cycles = 110 start_cycle = 125
// CHECK-NOT: systolic.matmul_tile
module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 7 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 7 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 7 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  func.func @mm_8x8x17(%A: tensor<8x17xf32>, %B: tensor<17x8xf32>, %C: tensor<8x8xf32>) -> tensor<8x8xf32> {
    // expected-remark @+1 {{binary-search schedule 8x8x17: makespan = 823 cycles (read + compute + write), compute-only = 235, schedule end = 823, operand bytes read = 1536, shared-read cycles = 0, tiles = 1}}
    %0 = linalg.matmul ins(%A, %B : tensor<8x17xf32>, tensor<17x8xf32>) outs(%C : tensor<8x8xf32>) -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }
}

// -----

// K_MAX = 256 on every array, K = 37: one invocation of 37 steps, not 256.
// 8x8: five windows (2560 B) -> 640 + 11, 37 + 109 = 146, 91 -- 888. A 4x4
// tile costs 651 + (37 + 101) + 31 = 820, four of them on three arrays
// 1640. est_cycles is 146, the compute term of a 37-step invocation.
// CHECK-LABEL: func.func @mm_8x8x37
// CHECK: m = 8 n = 8 k = 37 on @acc_8x8_0 est_cycles = 146 start_cycle = 0
// CHECK-NOT: systolic.matmul_tile
module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 256 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 256 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 256 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 256 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  func.func @mm_8x8x37(%A: tensor<8x37xf32>, %B: tensor<37x8xf32>, %C: tensor<8x8xf32>) -> tensor<8x8xf32> {
    // expected-remark @+1 {{binary-search schedule 8x8x37: makespan = 888 cycles (read + compute + write), compute-only = 146, schedule end = 888, operand bytes read = 2560, shared-read cycles = 0, tiles = 1}}
    %0 = linalg.matmul ins(%A, %B : tensor<8x37xf32>, tensor<37x8xf32>) outs(%C : tensor<8x8xf32>) -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }
}

// -----

// K_MAX = 256, K = 300: invocation 0 runs 256 steps
// (4107 + 365 + 91 = 4563), invocation 1 runs 44 (six windows, 779;
// 44 + 109 = 153; 91 -- 1023). The tile is 5586; est_cycles 365 then 153,
// the second invocation starting at 365. On a 4x4 the tile is
// (4107 + 357 + 31) + (779 + 145 + 31) = 5450, four of them 10900.
// CHECK-LABEL: func.func @mm_8x8x300
// CHECK: m = 8 n = 8 k = 256 on @acc_8x8_0 est_cycles = 365 start_cycle = 0
// CHECK: m = 8 n = 8 k = 44 on @acc_8x8_0 est_cycles = 153 start_cycle = 365
// CHECK-NOT: systolic.matmul_tile
module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 256 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 256 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 256 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 256 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  func.func @mm_8x8x300(%A: tensor<8x300xf32>, %B: tensor<300x8xf32>, %C: tensor<8x8xf32>) -> tensor<8x8xf32> {
    // expected-remark @+1 {{binary-search schedule 8x8x300: makespan = 5586 cycles (read + compute + write), compute-only = 518, schedule end = 5586, operand bytes read = 19456, shared-read cycles = 0, tiles = 1}}
    %0 = linalg.matmul ins(%A, %B : tensor<8x300xf32>, tensor<300x8xf32>) outs(%C : tensor<8x8xf32>) -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }
}
