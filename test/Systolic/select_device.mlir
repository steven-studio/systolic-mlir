module {
  systolic.device @acc_8x8 rows = 8 cols = 8 depth = 8 dataflow = weight_stationary
  systolic.device @acc_4x4 rows = 4 cols = 4 depth = 4 dataflow = weight_stationary

  %0 = systolic.matmul_tile(64, 64, 64) : tensor<64x64xf32>
  %1 = systolic.matmul_tile(32, 32, 32) : tensor<32x32xf32>
  %2 = systolic.matmul_tile(64, 64, 64) : tensor<64x64xf32>
  %3 = systolic.matmul_tile(16, 16, 16) : tensor<16x16xf32>
}
