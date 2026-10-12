// RUN: systolic-opt --systolic-fpga-pipeline="phys-k-max-8x8=256 phys-k-max-4x4=256" --mlir-disable-threading --mlir-print-ir-after=systolic-binary-search-tile-matmul %s 2>&1 >/dev/null | FileCheck %s --check-prefix=SHARED
// RUN: systolic-opt --systolic-fpga-pipeline="phys-k-max-8x8=256 phys-k-max-4x4=256 shared-read-words-per-cycle=0" --mlir-disable-threading --mlir-print-ir-after=systolic-binary-search-tile-matmul %s 2>&1 >/dev/null | FileCheck %s --check-prefix=INDEP
// RUN: systolic-opt --systolic-binary-search-tile-matmul="report=true shared-read-bytes-per-cycle=14.04" %s 2>&1 >/dev/null | FileCheck %s --check-prefix=REPORT-SHARED
// RUN: systolic-opt --systolic-binary-search-tile-matmul="report=true" %s 2>&1 >/dev/null | FileCheck %s --check-prefix=REPORT-INDEP

// The four arrays run at the same time and fill their operand buffers
// through one AXI read port (dma_engine_multi). --systolic-fpga-pipeline,
// like systolic-matmul, hands the binary search that shared limit: by
// default the fleet's reference DDR3 supply, 3.51 words/cycle = 14.04 B/cycle
// (Systolic/FpgaFleet.h; a reference, not measured on this bitstream), and
// shared-read-words-per-cycle=0 for fills that do not limit each other.
//
// The fleet is systolic-matmul's at K_MAX_8X8 = K_MAX_4X4 = 256
// (`systolic-cost fleet --k8 256 --k4 256`). With K = 256 every tile is one
// invocation that reads 2 x 8 lanes x 256 steps x 4 B = 16384 B on either
// geometry, and costs
//   8x8: 4107 read + 365 compute + 91 write = 4563
//   4x4: 4107 read + 357 compute + 31 write = 4495.
//
// Independent fills: 11 8x8 tiles (50193) and 36 4x4 tiles (12 rounds on
// three arrays, 53940) -- makespan 53940. Their 47 tiles read 770048 B,
// 54847 cycles at 14.04 B/cycle: more than 53940, so with the shared port
// that schedule is not feasible. An 8x8 tile reads what a 4x4 tile reads and
// covers four times the output; the search moves work onto the 8x8: 12 8x8
// tiles (54756) and 32 4x4 tiles (11 rounds, 49445) read 720896 B, 51346
// cycles, and the makespan is 54756.

// SHARED: IR Dump After {{.*}}SystolicBinarySearchTileMatmulPass{{.*}}shared-read-bytes-per-cycle=1.404000e+01
// SHARED-COUNT-12: on @acc_8x8_0
// SHARED-NOT: on @acc_8x8_0

// INDEP: IR Dump After {{.*}}SystolicBinarySearchTileMatmulPass{{.*}}shared-read-bytes-per-cycle=0.000000e+00
// INDEP-COUNT-11: on @acc_8x8_0
// INDEP-NOT: on @acc_8x8_0

// REPORT-SHARED: remark: binary-search schedule 32x40x256: makespan = 54756 cycles (read + compute + write), compute-only = 4380, schedule end = 54756, operand bytes read = 720896, shared-read cycles = 51346, tiles = 44
// REPORT-INDEP: remark: binary-search schedule 32x40x256: makespan = 53940 cycles (read + compute + write), compute-only = 4284, schedule end = 53940, operand bytes read = 770048, shared-read cycles = 0, tiles = 47

module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 256 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 256 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 256 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 256 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  func.func @mm_32x40x256(%A: tensor<32x256xf32>, %B: tensor<256x40xf32>, %C: tensor<32x40xf32>) -> tensor<32x40xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<32x256xf32>, tensor<256x40xf32>) outs(%C : tensor<32x40xf32>) -> tensor<32x40xf32>
    return %0 : tensor<32x40xf32>
  }
}
