module {
  systolic.device @acc_8x8 rows = 8 cols = 8 depth = 1 dataflow = weight_stationary
  systolic.device @acc_4x4 rows = 4 cols = 4 depth = 1 dataflow = weight_stationary

  // ceil(64/8) * ceil(64/8) * ceil(64/1) + (8+8-2) = 8*8*64 + 14 = 4110
  %0 = systolic.matmul_tile(64, 64, 64) on @acc_8x8 : tensor<64x64xf32>

  // ceil(64/4) * ceil(64/4) * ceil(64/1) + (4+4-2) = 16*16*64 + 6 = 16390
  %1 = systolic.matmul_tile(64, 64, 64) on @acc_4x4 : tensor<64x64xf32>

  systolic.dma 4096 on @acc_8x8
}
