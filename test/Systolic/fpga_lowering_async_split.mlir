// RUN: systolic-opt --systolic-binary-search-tile-matmul --lower-systolic-matmul-tile-to-fpga="async=true" %s \
// RUN:   | FileCheck %s --implicit-check-not=fpga_matmul_tiled_auto_scheduled

// A GEMM whose output blocks cannot be staged (C's root is a linalg.fill of
// a tensor.empty, whose buffer other tensors may share: see
// fpga_operand_staging_fallback.mlir): every tile goes through the per-tile
// path, and each block's result is inserted into the full matrix right
// after the block's last fold -- a reader of the GEMM's result that comes
// before the next block's submits. The asynchronous lowering then waits for
// the tiles submitted so far and re-opens the GEMM for the rest (the
// runtime refuses a submit after the wait), so the GEMM runs as one
// batch per output block instead of losing the later tiles. 8x4x20: two
// 4x4 blocks on devices 1 and 2, each k = 16 then 4.
//
// CHECK-LABEL: func.func @fill_root
// CHECK:       %[[H0:.*]] = llvm.call @fpga_gemm_begin
// CHECK:       llvm.call @fpga_tile_submit(%[[H0]],
// CHECK:       llvm.call @fpga_tile_submit(%[[H0]],
// CHECK-NOT:   llvm.call @fpga_
// CHECK:       llvm.call @fpga_gemm_wait(%[[H0]])
// CHECK-NOT:   llvm.call @fpga_
// CHECK:       %[[H1:.*]] = llvm.call @fpga_gemm_begin
// CHECK-NOT:   llvm.call @fpga_
// CHECK:       tensor.insert_slice
// CHECK:       llvm.call @fpga_tile_submit(%[[H1]],
// CHECK:       llvm.call @fpga_tile_submit(%[[H1]],
// CHECK-NOT:   llvm.call @fpga_
// CHECK:       llvm.call @fpga_gemm_wait(%[[H1]])
// CHECK-NOT:   llvm.call @fpga_
// CHECK:       tensor.insert_slice
// CHECK:       return

module {
  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95
  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = output_stationary k_max = 16 tile_overhead = 95

  func.func @fill_root(%a: tensor<8x20xf32>, %b: tensor<20x4xf32>) -> tensor<8x4xf32> {
    %zero = arith.constant 0.0 : f32
    %e = tensor.empty() : tensor<8x4xf32>
    %c = linalg.fill ins(%zero : f32) outs(%e : tensor<8x4xf32>) -> tensor<8x4xf32>
    %r = linalg.matmul ins(%a, %b : tensor<8x20xf32>, tensor<20x4xf32>)
                       outs(%c : tensor<8x4xf32>) -> tensor<8x4xf32>
    return %r : tensor<8x4xf32>
  }
}
