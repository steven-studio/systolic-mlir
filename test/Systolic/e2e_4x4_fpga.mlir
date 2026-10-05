// ============================================================================
// E2E smoke test: 4x4 systolic.matmul_tile -> FPGA backend
//
// This is intentionally the smallest physical FPGA test.
// The 16x16 -> 16x 4x4 decomposition test comes later.
//
// Expected contract:
//
//   systolic.matmul_tile
//       m=4 n=4 k=4
//       on @acc_4x4_1
//       start_cycle = 0
//       est_cycles  > 0
//
//        |
//        v
//
//   lower-systolic-matmul-tile-to-fpga
//
//        |
//        v
//
//   bufferization.to_memref
//   llvm.call @fpga_matmul_tiled_auto
// ============================================================================

module {
  systolic.device @acc_4x4_1
      rows = 4 cols = 4 depth = 1
      dataflow = weight_stationary

  func.func @test_4x4_fpga(
      %a: tensor<4x4xf32>,
      %b: tensor<4x4xf32>,
      %c: tensor<4x4xf32>) -> tensor<4x4xf32> {

    %r = systolic.matmul_tile
        %a, %b, %c
        m = 4 n = 4 k = 4
        on @acc_4x4_1
        : (tensor<4x4xf32>,
           tensor<4x4xf32>,
           tensor<4x4xf32>)
          -> tensor<4x4xf32>

    return %r : tensor<4x4xf32>
  }
}
