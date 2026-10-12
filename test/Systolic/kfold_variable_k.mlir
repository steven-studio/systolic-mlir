// RUN: systolic-opt --systolic-binary-search-tile-matmul %s | FileCheck %s
// RUN: systolic-opt --systolic-binary-search-tile-matmul --lower-systolic-matmul-tile-to-fpga="phys-k-max-8x8=32 phys-k-max-4x4=32" %s | FileCheck %s --check-prefix=LOWER

// Variable-K folding on the FPGA fleet (Systolic/FpgaFleet.h) built with
// K_MAX_8X8 = K_MAX_4X4 = 32: K = 1, K_MAX - 1, K_MAX, K_MAX + 1 and three
// deeper reductions, on one 8x8 tile and on one 4x4 tile. K_MAX is the
// operand buffer's capacity: invocation i runs k_i = min(32, K - 32 i)
// steps, the runtime sends job.k = k_i, and est_cycles is that invocation's
// compute term k_i + 2g - 2 + 95 (k_i + 109 on the 8x8, k_i + 101 on a
// 4x4). start_cycle accumulates the terms before it -- the last, shorter
// invocation is not an equal share of the tile.
//
// The 8x8 tile goes to the 8x8 in every case: e.g. K = 65 costs it
// 755 + 755 + 340 = 1850, against 687 + 687 + 272 = 1646 for each of the
// four 4x4 tiles, two rounds on three arrays (3292).

// CHECK-LABEL: func.func @mm8_k1
// CHECK: m = 8 n = 8 k = 1 on @acc_8x8_0 est_cycles = 110 start_cycle = 0
// CHECK-NOT: systolic.matmul_tile
// CHECK-LABEL: func.func @mm8_k31
// CHECK: m = 8 n = 8 k = 31 on @acc_8x8_0 est_cycles = 140 start_cycle = 0
// CHECK-NOT: systolic.matmul_tile
// CHECK-LABEL: func.func @mm8_k32
// CHECK: m = 8 n = 8 k = 32 on @acc_8x8_0 est_cycles = 141 start_cycle = 0
// CHECK-NOT: systolic.matmul_tile
// CHECK-LABEL: func.func @mm8_k33
// CHECK: m = 8 n = 8 k = 32 on @acc_8x8_0 est_cycles = 141 start_cycle = 0
// CHECK: m = 8 n = 8 k = 1 on @acc_8x8_0 est_cycles = 110 start_cycle = 141
// CHECK-NOT: systolic.matmul_tile
// CHECK-LABEL: func.func @mm8_k47
// CHECK: m = 8 n = 8 k = 32 on @acc_8x8_0 est_cycles = 141 start_cycle = 0
// CHECK: m = 8 n = 8 k = 15 on @acc_8x8_0 est_cycles = 124 start_cycle = 141
// CHECK-NOT: systolic.matmul_tile
// CHECK-LABEL: func.func @mm8_k64
// CHECK: m = 8 n = 8 k = 32 on @acc_8x8_0 est_cycles = 141 start_cycle = 0
// CHECK: m = 8 n = 8 k = 32 on @acc_8x8_0 est_cycles = 141 start_cycle = 141
// CHECK-NOT: systolic.matmul_tile
// CHECK-LABEL: func.func @mm8_k65
// CHECK: m = 8 n = 8 k = 32 on @acc_8x8_0 est_cycles = 141 start_cycle = 0
// CHECK: m = 8 n = 8 k = 32 on @acc_8x8_0 est_cycles = 141 start_cycle = 141
// CHECK: m = 8 n = 8 k = 1 on @acc_8x8_0 est_cycles = 110 start_cycle = 282
// CHECK-NOT: systolic.matmul_tile
// CHECK-LABEL: func.func @mm4_k1
// CHECK: m = 4 n = 4 k = 1 on @acc_4x4_0 est_cycles = 102 start_cycle = 0
// CHECK-NOT: systolic.matmul_tile
// CHECK-LABEL: func.func @mm4_k31
// CHECK: m = 4 n = 4 k = 31 on @acc_4x4_0 est_cycles = 132 start_cycle = 0
// CHECK-NOT: systolic.matmul_tile
// CHECK-LABEL: func.func @mm4_k32
// CHECK: m = 4 n = 4 k = 32 on @acc_4x4_0 est_cycles = 133 start_cycle = 0
// CHECK-NOT: systolic.matmul_tile
// CHECK-LABEL: func.func @mm4_k33
// CHECK: m = 4 n = 4 k = 32 on @acc_4x4_0 est_cycles = 133 start_cycle = 0
// CHECK: m = 4 n = 4 k = 1 on @acc_4x4_0 est_cycles = 102 start_cycle = 133
// CHECK-NOT: systolic.matmul_tile
// CHECK-LABEL: func.func @mm4_k47
// CHECK: m = 4 n = 4 k = 32 on @acc_4x4_0 est_cycles = 133 start_cycle = 0
// CHECK: m = 4 n = 4 k = 15 on @acc_4x4_0 est_cycles = 116 start_cycle = 133
// CHECK-NOT: systolic.matmul_tile
// CHECK-LABEL: func.func @mm4_k64
// CHECK: m = 4 n = 4 k = 32 on @acc_4x4_0 est_cycles = 133 start_cycle = 0
// CHECK: m = 4 n = 4 k = 32 on @acc_4x4_0 est_cycles = 133 start_cycle = 133
// CHECK-NOT: systolic.matmul_tile
// CHECK-LABEL: func.func @mm4_k65
// CHECK: m = 4 n = 4 k = 32 on @acc_4x4_0 est_cycles = 133 start_cycle = 0
// CHECK: m = 4 n = 4 k = 32 on @acc_4x4_0 est_cycles = 133 start_cycle = 133
// CHECK: m = 4 n = 4 k = 1 on @acc_4x4_0 est_cycles = 102 start_cycle = 266
// CHECK-NOT: systolic.matmul_tile

// The FPGA lowering passes each invocation's own depth to the runtime
// (m, k, n, ..., device, start_cycle, est_cycles): 32, 32, then 1.
// LOWER-LABEL: func.func @mm8_k65
// LOWER: %[[M0:.*]] = arith.constant 8 : i32
// LOWER-NEXT: %[[K0:.*]] = arith.constant 32 : i32
// LOWER-NEXT: arith.constant 8 : i32
// LOWER-NEXT: arith.constant 0 : i32
// LOWER-NEXT: %[[S0:.*]] = arith.constant 0 : i32
// LOWER-NEXT: %[[E0:.*]] = arith.constant 141 : i32
// LOWER-NEXT: llvm.call @fpga_matmul_tiled_auto_scheduled(%[[M0]], %[[K0]], %{{.*}}, %{{.*}}, %{{.*}}, %{{.*}}, %{{.*}}, %[[S0]], %[[E0]])
// LOWER: %[[M1:.*]] = arith.constant 8 : i32
// LOWER-NEXT: %[[K1:.*]] = arith.constant 32 : i32
// LOWER-NEXT: arith.constant 8 : i32
// LOWER-NEXT: arith.constant 0 : i32
// LOWER-NEXT: %[[S1:.*]] = arith.constant 141 : i32
// LOWER-NEXT: %[[E1:.*]] = arith.constant 141 : i32
// LOWER-NEXT: llvm.call @fpga_matmul_tiled_auto_scheduled(%[[M1]], %[[K1]], %{{.*}}, %{{.*}}, %{{.*}}, %{{.*}}, %{{.*}}, %[[S1]], %[[E1]])
// LOWER: %[[M2:.*]] = arith.constant 8 : i32
// LOWER-NEXT: %[[K2:.*]] = arith.constant 1 : i32
// LOWER-NEXT: arith.constant 8 : i32
// LOWER-NEXT: arith.constant 0 : i32
// LOWER-NEXT: %[[S2:.*]] = arith.constant 282 : i32
// LOWER-NEXT: %[[E2:.*]] = arith.constant 110 : i32
// LOWER-NEXT: llvm.call @fpga_matmul_tiled_auto_scheduled(%[[M2]], %[[K2]], %{{.*}}, %{{.*}}, %{{.*}}, %{{.*}}, %{{.*}}, %[[S2]], %[[E2]])
// LOWER-NOT: llvm.call @fpga_matmul_tiled_auto_scheduled
// LOWER: return

module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 32 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 32 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 32 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 32 tile_overhead = 95 read_bytes_per_cycle = 4.0 write_bytes_per_cycle = 3.2 read_overhead = 11 write_overhead = 11 operand_lanes = 8 k_window = 8
  func.func @mm8_k1(%A: tensor<8x1xf32>, %B: tensor<1x8xf32>, %C: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<8x1xf32>, tensor<1x8xf32>) outs(%C : tensor<8x8xf32>) -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }
  func.func @mm8_k31(%A: tensor<8x31xf32>, %B: tensor<31x8xf32>, %C: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<8x31xf32>, tensor<31x8xf32>) outs(%C : tensor<8x8xf32>) -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }
  func.func @mm8_k32(%A: tensor<8x32xf32>, %B: tensor<32x8xf32>, %C: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<8x32xf32>, tensor<32x8xf32>) outs(%C : tensor<8x8xf32>) -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }
  func.func @mm8_k33(%A: tensor<8x33xf32>, %B: tensor<33x8xf32>, %C: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<8x33xf32>, tensor<33x8xf32>) outs(%C : tensor<8x8xf32>) -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }
  func.func @mm8_k47(%A: tensor<8x47xf32>, %B: tensor<47x8xf32>, %C: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<8x47xf32>, tensor<47x8xf32>) outs(%C : tensor<8x8xf32>) -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }
  func.func @mm8_k64(%A: tensor<8x64xf32>, %B: tensor<64x8xf32>, %C: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<8x64xf32>, tensor<64x8xf32>) outs(%C : tensor<8x8xf32>) -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }
  func.func @mm8_k65(%A: tensor<8x65xf32>, %B: tensor<65x8xf32>, %C: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<8x65xf32>, tensor<65x8xf32>) outs(%C : tensor<8x8xf32>) -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }
  func.func @mm4_k1(%A: tensor<4x1xf32>, %B: tensor<1x4xf32>, %C: tensor<4x4xf32>) -> tensor<4x4xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<4x1xf32>, tensor<1x4xf32>) outs(%C : tensor<4x4xf32>) -> tensor<4x4xf32>
    return %0 : tensor<4x4xf32>
  }
  func.func @mm4_k31(%A: tensor<4x31xf32>, %B: tensor<31x4xf32>, %C: tensor<4x4xf32>) -> tensor<4x4xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<4x31xf32>, tensor<31x4xf32>) outs(%C : tensor<4x4xf32>) -> tensor<4x4xf32>
    return %0 : tensor<4x4xf32>
  }
  func.func @mm4_k32(%A: tensor<4x32xf32>, %B: tensor<32x4xf32>, %C: tensor<4x4xf32>) -> tensor<4x4xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<4x32xf32>, tensor<32x4xf32>) outs(%C : tensor<4x4xf32>) -> tensor<4x4xf32>
    return %0 : tensor<4x4xf32>
  }
  func.func @mm4_k33(%A: tensor<4x33xf32>, %B: tensor<33x4xf32>, %C: tensor<4x4xf32>) -> tensor<4x4xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<4x33xf32>, tensor<33x4xf32>) outs(%C : tensor<4x4xf32>) -> tensor<4x4xf32>
    return %0 : tensor<4x4xf32>
  }
  func.func @mm4_k47(%A: tensor<4x47xf32>, %B: tensor<47x4xf32>, %C: tensor<4x4xf32>) -> tensor<4x4xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<4x47xf32>, tensor<47x4xf32>) outs(%C : tensor<4x4xf32>) -> tensor<4x4xf32>
    return %0 : tensor<4x4xf32>
  }
  func.func @mm4_k64(%A: tensor<4x64xf32>, %B: tensor<64x4xf32>, %C: tensor<4x4xf32>) -> tensor<4x4xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<4x64xf32>, tensor<64x4xf32>) outs(%C : tensor<4x4xf32>) -> tensor<4x4xf32>
    return %0 : tensor<4x4xf32>
  }
  func.func @mm4_k65(%A: tensor<4x65xf32>, %B: tensor<65x4xf32>, %C: tensor<4x4xf32>) -> tensor<4x4xf32> {
    %0 = linalg.matmul ins(%A, %B : tensor<4x65xf32>, tensor<65x4xf32>) outs(%C : tensor<4x4xf32>) -> tensor<4x4xf32>
    return %0 : tensor<4x4xf32>
  }
}
