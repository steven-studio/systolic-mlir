<!-- 由 eval/scalesim/gen_design_readmes.py 產生。改內容請直接編輯本檔。 -->

# gemm_4x4 — Arty A7-35T,單一 4x4

| | |
|---|---|
| 板子 | Arty A7-35T (`xc7a35ticsg324-1L`) |
| HLS top | `matmul_4x4x4`（`run_hls.tcl`） |
| 形狀 | `design.h`: R=4 C=4 K_DIM=4 |
| 主機協定 | `runtime/lib/fpga_matmul4x4.h` — 192 bytes,三個 4x4 矩陣,無 header |
| 時脈 | 待確認 |

## 承載的結果

**conv2d 48/48 bit-exact。** 這是目前唯一一個完整通過的 conv2d 掃描。

依據 `runtime/lib/fpga_tile.h` 的檔頭：

> The existing fpga_matmul4x4 / fpga_matmul_tiled pair is left untouched.
> That path carries the 48/48 bit-exact result **on the Arty** and is the
> only thing the MLIR codegen entry points reach today.

結果檔目前叫 `runtime/harness/sweep_out/sweep_results_nexys_8x8.csv`，
**檔名是錯的**（見 `REPRODUCE.md` 第 2.1 節）。

## 注意

`fpga_matmul_fold.h` 記載：這個 192-byte 協定在 8x8 改寫之後，Nexys 這塊
板子沒有再接受過。要重現這個結果需要 Arty。
