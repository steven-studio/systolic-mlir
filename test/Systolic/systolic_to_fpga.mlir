// RUN: systolic-opt --systolic-binary-search-tile-matmul --lower-systolic-matmul-tile-to-fpga %s | FileCheck %s
// RUN: systolic-opt --systolic-fpga-pipeline %s | FileCheck %s --check-prefix=LLVM
// RUN: systolic-opt --systolic-fpga-pipeline %s | FileCheck %s --check-prefix=CLEAN

// Systolic -> FPGA: every scheduled matmul_tile becomes one call of the
// runtime the board was verified with (runtime/fpga_matmul_tiled.cpp):
//
//   fpga_matmul_tiled_auto_scheduled(m, k, n, A, B, C,
//                                    device_id, start_cycle, est_cycles)
//
// The device symbol maps onto the bitstream's physical IDs: @acc_8x8_0 -> 0,
// @acc_4x4_0..2 -> 1..3. After lowering, no systolic op is left.

module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95

  // CHECK-NOT: systolic.device
  // CHECK: llvm.func @fpga_matmul_tiled_auto_scheduled(i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32

  // 16x8x4: the 8x8 (device 0) takes rows 0..7; the bottom half is four 4x4
  // tiles on devices 1, 2, 3 and then 1 again at start_cycle 105. est_cycles
  // is the invocation's compute term (k + 109 on the 8x8, k + 101 on a 4x4:
  // these devices declare no read or write term).
  // CHECK-LABEL: func.func @mm_16x8x4
  // CHECK: %[[M0:.*]] = arith.constant 8 : i32
  // CHECK-NEXT: %[[K0:.*]] = arith.constant 4 : i32
  // CHECK-NEXT: %[[N0:.*]] = arith.constant 8 : i32
  // CHECK-NEXT: %[[D0:.*]] = arith.constant 0 : i32
  // CHECK-NEXT: %[[S0:.*]] = arith.constant 0 : i32
  // CHECK-NEXT: %[[E0:.*]] = arith.constant 113 : i32
  // CHECK-NEXT: llvm.call @fpga_matmul_tiled_auto_scheduled(%[[M0]], %[[K0]], %[[N0]], %{{.*}}, %{{.*}}, %{{.*}}, %[[D0]], %[[S0]], %[[E0]])
  // CHECK: %[[M1:.*]] = arith.constant 4 : i32
  // CHECK-NEXT: %[[K1:.*]] = arith.constant 4 : i32
  // CHECK-NEXT: %[[N1:.*]] = arith.constant 4 : i32
  // CHECK-NEXT: %[[D1:.*]] = arith.constant 1 : i32
  // CHECK-NEXT: %[[S1:.*]] = arith.constant 0 : i32
  // CHECK-NEXT: %[[E1:.*]] = arith.constant 105 : i32
  // CHECK-NEXT: llvm.call @fpga_matmul_tiled_auto_scheduled(%[[M1]], %[[K1]], %[[N1]], %{{.*}}, %{{.*}}, %{{.*}}, %[[D1]], %[[S1]], %[[E1]])
  // CHECK: %[[D2:.*]] = arith.constant 2 : i32
  // CHECK-NEXT: arith.constant 0 : i32
  // CHECK-NEXT: arith.constant 105 : i32
  // CHECK-NEXT: llvm.call @fpga_matmul_tiled_auto_scheduled({{.*}}, %[[D2]],
  // CHECK: %[[D3:.*]] = arith.constant 3 : i32
  // CHECK-NEXT: arith.constant 0 : i32
  // CHECK-NEXT: arith.constant 105 : i32
  // CHECK-NEXT: llvm.call @fpga_matmul_tiled_auto_scheduled({{.*}}, %[[D3]],
  // CHECK: %[[D4:.*]] = arith.constant 1 : i32
  // CHECK-NEXT: %[[S4:.*]] = arith.constant 105 : i32
  // CHECK-NEXT: arith.constant 105 : i32
  // CHECK-NEXT: llvm.call @fpga_matmul_tiled_auto_scheduled({{.*}}, %[[D4]], %[[S4]],
  // CHECK-NOT: llvm.call @fpga_matmul_tiled_auto_scheduled
  // CHECK-NOT: systolic.matmul_tile
  // CHECK: return
  func.func @mm_16x8x4(%A: tensor<16x4xf32>, %B: tensor<4x8xf32>,
                       %C: tensor<16x8xf32>) -> tensor<16x8xf32>
      attributes {llvm.emit_c_interface} {
    %0 = linalg.matmul ins(%A, %B : tensor<16x4xf32>, tensor<4x8xf32>)
                       outs(%C : tensor<16x8xf32>) -> tensor<16x8xf32>
    return %0 : tensor<16x8xf32>
  }

  // 2x3x40: one 4x4 tile clipped to 2x3, folded into k = 16, 16, 8 on
  // device 1, costing 117, 117 and 109 and starting at 0, 117 and 234 -- not
  // an equal split. m and n are the clipped sizes; the runtime pads them to
  // the array.
  // CHECK-LABEL: func.func @mm_2x3x40
  // CHECK: %[[MA:.*]] = arith.constant 2 : i32
  // CHECK-NEXT: %[[KA:.*]] = arith.constant 16 : i32
  // CHECK-NEXT: %[[NA:.*]] = arith.constant 3 : i32
  // CHECK-NEXT: %[[DA:.*]] = arith.constant 1 : i32
  // CHECK-NEXT: %[[SA:.*]] = arith.constant 0 : i32
  // CHECK-NEXT: %[[EA:.*]] = arith.constant 117 : i32
  // CHECK-NEXT: llvm.call @fpga_matmul_tiled_auto_scheduled(%[[MA]], %[[KA]], %[[NA]], %{{.*}}, %{{.*}}, %{{.*}}, %[[DA]], %[[SA]], %[[EA]])
  // CHECK: arith.constant 2 : i32
  // CHECK-NEXT: arith.constant 16 : i32
  // CHECK-NEXT: arith.constant 3 : i32
  // CHECK-NEXT: arith.constant 1 : i32
  // CHECK-NEXT: arith.constant 117 : i32
  // CHECK-NEXT: arith.constant 117 : i32
  // CHECK-NEXT: llvm.call @fpga_matmul_tiled_auto_scheduled
  // CHECK: arith.constant 2 : i32
  // CHECK-NEXT: %[[KC:.*]] = arith.constant 8 : i32
  // CHECK-NEXT: arith.constant 3 : i32
  // CHECK-NEXT: arith.constant 1 : i32
  // CHECK-NEXT: %[[SC:.*]] = arith.constant 234 : i32
  // CHECK-NEXT: arith.constant 109 : i32
  // CHECK-NEXT: llvm.call @fpga_matmul_tiled_auto_scheduled(%{{.*}}, %[[KC]], %{{.*}}, %{{.*}}, %{{.*}}, %{{.*}}, %{{.*}}, %[[SC]],
  // CHECK-NOT: llvm.call @fpga_matmul_tiled_auto_scheduled
  // CHECK: return
  func.func @mm_2x3x40(%A: tensor<2x40xf32>, %B: tensor<40x3xf32>,
                       %C: tensor<2x3xf32>) -> tensor<2x3xf32>
      attributes {llvm.emit_c_interface} {
    %0 = linalg.matmul ins(%A, %B : tensor<2x40xf32>, tensor<40x3xf32>)
                       outs(%C : tensor<2x3xf32>) -> tensor<2x3xf32>
    return %0 : tensor<2x3xf32>
  }
}

// The whole pipeline ends in the LLVM dialect: C-callable entry points, the
// runtime calls, and no tensor, memref or systolic op left.
// LLVM-DAG: llvm.func @fpga_matmul_tiled_auto_scheduled(i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
// LLVM-DAG: llvm.func @_mlir_ciface_mm_16x8x4(
// LLVM-DAG: llvm.func @_mlir_ciface_mm_2x3x40(
// CLEAN-NOT: systolic.
// CLEAN-NOT: tensor<
// CLEAN-NOT: memref<
// CLEAN-NOT: linalg.
// CLEAN-NOT: bufferization.
