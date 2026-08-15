<!-- 由 eval/scalesim/gen_design_readmes.py 產生。改內容請直接編輯本檔。 -->

# gemm_8x8 — Arty A7-35T,單一 8x8

| | |
|---|---|
| 板子 | Arty A7-35T (`xc7a35ticsg324-1L`，`run_hls.tcl`) |
| HLS top | `matmul_8x8x8` |
| 狀態 | 待確認是否曾上板 |

`matmul_8x8_only.mlir` / `matmul_8x8_systolic.mlir` 是對應的 MLIR 測試輸入。

xc7a35t 只有 90 顆 DSP，而 HLS 的 FP MAC 約 5 顆 DSP，8x8 全展開需要
64 x 5 = 320 顆 —— **這顆很可能塞不下 Arty**。若確實如此，值得在此註明，
它本身就是一個資源受限的資料點。
