# runtime/lib — 依 bitstream 分目錄

**一個目錄 = 一種 wire protocol = 一顆 bitstream。三者不互通。**
選錯的症狀是「tile 迴圈深處讀取逾時」,不是明顯的失敗,所以才分開放。

| 目錄 | 協定 | bitstream |
|---|---|---|
| `common/` | 無（port / baud / FPGA_DIM / FPGA_NDEV、signal handler） | — |
| `arty_4x4/` | 192 B,三個 4x4,無 header | `hls/gemm_4x4/vivado/build/…/matmul_top.bit`（Arty A7-35T） |
| `nexys_dual_4x4/` | `[dev][A][B][C_init]`,維度/裝置通用 | `hls/multi_200t/vivado/baseline_40mhz/matmul_top_dual_40mhz_fixedK.bit` |
| `nexys_rk_8x8/` | `[dev][K][A][B][C_init]` -> 256 B | `…/baseline_40mhz/matmul_8x8x8_rk_40mhz_K1to64.bit` |
| `nexys_fold_8x8/` | `[k_dim][payload]` -> 512 B（兩個 context,主機相加） | `hls/multi_200t/fold_pipelined/rtl/build_kmax/k64/*.bit` |
| `gemm/` | 無自己的協定 | im2col 與 tiling。**目前只接得到 `arty_4x4/`** |
| `dispatch/` | — | MLIR codegen 的進入點。`DISPATCH=uart` 或 `sim` 二選一 |

`gemm/fpga_conv2d_im2col.c` 呼叫 `fpga_matmul_tiled_auto`,而後者寫死 4x4。
要讓 conv2d 跑在 8x8 上,缺的是一個把 `nexys_rk_8x8/` 的單筆交易包成
M×K×N tiling 的函式。
