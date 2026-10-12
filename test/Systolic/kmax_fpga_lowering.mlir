// RUN: systolic-opt --systolic-binary-search-tile-matmul --lower-systolic-matmul-tile-to-fpga="phys-k-max-8x8=17 phys-k-max-4x4=7" %s | FileCheck %s
// RUN: systolic-opt --systolic-fpga-pipeline="phys-k-max-8x8=17 phys-k-max-4x4=7" %s | FileCheck %s --check-prefix=LLVM

// Systolic -> FPGA on a bitstream built with K_MAX_8X8 = 17 and
// K_MAX_4X4 = 7: the fleet declares the same capacities (the lowering refuses
// any other; bad_kmax_lowering.mlir), the binary-search pass folds K by each
// device's own k_max, and every invocation becomes one runtime call whose k
// is that invocation's depth, 1 <= k <= K_MAX of its device.
//
// The scratch tiles are sized for the deepest invocation any device takes:
// A is rows x K_MAX = 8 x 17 = 136 floats on the 8x8 (4 x 7 = 28 on a 4x4),
// B 17 x 8 = 136, C 8 x 8 = 64; the 8x8's first invocation views them as
// 8 x 17 and 17 x 8.

module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 17 tile_overhead = 95
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 7 tile_overhead = 95
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 7 tile_overhead = 95
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 7 tile_overhead = 95

  // 12x12x18: the 8x8 tile is k = 17 then 1 on device 0, each of the five
  // 4x4 tiles k = 7, 7, 4 on devices 1..3. est_cycles is each invocation's
  // own compute term (k + 109 on the 8x8, k + 101 on a 4x4; these devices
  // declare no read or write term), start_cycle the sum of the terms before
  // it on the same device: 126 then 110 at 0 and 126; 108, 108, 105 at 0,
  // 108 and 216.
  // CHECK-LABEL: func.func @mm_12x12x18
  // CHECK-DAG: memref.alloca() : memref<136xf32>
  // CHECK-DAG: memref.alloca() : memref<136xf32>
  // CHECK-DAG: memref.alloca() : memref<64xf32>
  // CHECK: memref.reinterpret_cast {{.*}} sizes: [8, 17], strides: [17, 1] : memref<136xf32> to memref<8x17xf32>
  // CHECK: memref.reinterpret_cast {{.*}} sizes: [17, 8], strides: [8, 1] : memref<136xf32> to memref<17x8xf32>
  // CHECK: %[[M0:.*]] = arith.constant 8 : i32
  // CHECK-NEXT: %[[K0:.*]] = arith.constant 17 : i32
  // CHECK-NEXT: %[[N0:.*]] = arith.constant 8 : i32
  // CHECK-NEXT: %[[D0:.*]] = arith.constant 0 : i32
  // CHECK-NEXT: %[[S0:.*]] = arith.constant 0 : i32
  // CHECK-NEXT: %[[E0:.*]] = arith.constant 126 : i32
  // CHECK-NEXT: llvm.call @fpga_matmul_tiled_auto_scheduled(%[[M0]], %[[K0]], %[[N0]], %{{.*}}, %{{.*}}, %{{.*}}, %[[D0]], %[[S0]], %[[E0]])
  // CHECK: arith.constant 8 : i32
  // CHECK-NEXT: %[[K1:.*]] = arith.constant 1 : i32
  // CHECK-NEXT: arith.constant 8 : i32
  // CHECK-NEXT: %[[D1:.*]] = arith.constant 0 : i32
  // CHECK-NEXT: %[[S1:.*]] = arith.constant 126 : i32
  // CHECK-NEXT: arith.constant 110 : i32
  // CHECK-NEXT: llvm.call @fpga_matmul_tiled_auto_scheduled(%{{.*}}, %[[K1]], %{{.*}}, %{{.*}}, %{{.*}}, %{{.*}}, %[[D1]], %[[S1]],
  // CHECK: %[[M2:.*]] = arith.constant 4 : i32
  // CHECK-NEXT: %[[K2:.*]] = arith.constant 7 : i32
  // CHECK-NEXT: arith.constant 4 : i32
  // CHECK-NEXT: %[[D2:.*]] = arith.constant 1 : i32
  // CHECK-NEXT: arith.constant 0 : i32
  // CHECK-NEXT: arith.constant 108 : i32
  // CHECK-NEXT: llvm.call @fpga_matmul_tiled_auto_scheduled(%[[M2]], %[[K2]], {{.*}}, %[[D2]],
  // CHECK: arith.constant 7 : i32
  // CHECK: llvm.call @fpga_matmul_tiled_auto_scheduled
  // CHECK: arith.constant 4 : i32
  // CHECK-NEXT: %[[K4:.*]] = arith.constant 4 : i32
  // CHECK-NEXT: arith.constant 4 : i32
  // CHECK-NEXT: arith.constant 1 : i32
  // CHECK-NEXT: arith.constant 216 : i32
  // CHECK-NEXT: arith.constant 105 : i32
  // CHECK-NEXT: llvm.call @fpga_matmul_tiled_auto_scheduled(%{{.*}}, %[[K4]],
  // CHECK-NOT: systolic.matmul_tile
  // CHECK: return

  // 2 + 5 * 3 = 17 calls, nothing systolic left, plain LLVM dialect.
  // LLVM-LABEL: llvm.func @mm_12x12x18
  // LLVM-COUNT-17: llvm.call @fpga_matmul_tiled_auto_scheduled
  // LLVM-NOT: llvm.call @fpga_matmul_tiled_auto_scheduled
  // LLVM-NOT: systolic.
  func.func @mm_12x12x18(%A: tensor<12x18xf32>, %B: tensor<18x12xf32>,
                         %C: tensor<12x12xf32>) -> tensor<12x12xf32>
      attributes {llvm.emit_c_interface} {
    %0 = linalg.matmul ins(%A, %B : tensor<12x18xf32>, tensor<18x12xf32>)
                       outs(%C : tensor<12x12xf32>) -> tensor<12x12xf32>
    return %0 : tensor<12x12xf32>
  }
}
