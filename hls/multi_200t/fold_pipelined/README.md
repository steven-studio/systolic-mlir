<!-- 由 eval/scalesim/gen_design_readmes.py 產生。改內容請直接編輯本檔。 -->

# fold_pipelined — 8x8 fold,目前的主線

| | |
|---|---|
| 板子 | Nexys Video (`xc7a200tsbg484-1`)，100 MHz |
| 形狀 | `design.h`: R=8 C=8 |
| 實作 | **`rtl/` 底下的手寫 SystemVerilog**，不是 HLS 產出 |

`design.cpp` / `design.h` 是這條線的 HLS 起點，但實際上板的是 `rtl/`
的手寫 RTL。兩者不要混淆：`rtl/` 不依賴任何 HLS 產物，只用 Vivado 的
floating-point IP。

`design_before_fadd_timing.cpp` 與 `design_before_rotating_acc.cpp` 是
HLS 階段的歷史版本，保留作為設計演進的紀錄。

詳見 `rtl/README.md`。
