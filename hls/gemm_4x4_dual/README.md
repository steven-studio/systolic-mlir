<!-- 由 eval/scalesim/gen_design_readmes.py 產生。改內容請直接編輯本檔。 -->

# gemm_4x4_dual — 雙 4x4 的 HLS 探索

| | |
|---|---|
| 板子 | **Arty A7-35T** (`xc7a35ticsg324-1L`，`vivado/build_project_dual.tcl:8`) |
| 形狀 | `design.h`: R=4 C=4 K_DIM=4 |
| 狀態 | 早期探索。`board_smoke_dual.py` / `board_smoke_single.py` 是當時的煙霧測試 |

## 與 `multi_200t` 的 `matmul_top_dual` 不是同一個東西

同名容易混淆，但板子不同：

| | 這裡 | `multi_200t/rtl/matmul_top_dual.v` |
|---|---|---|
| 板子 | Arty A7-35T (`xc7a35t`) | Nexys Video (`xc7a200t`) |
| DSP | 待確認（xc7a35t 只有 90 顆） | 640 |

xc7a35t 的 90 顆 DSP 放不下兩個全展開的 `matmul_4x4x4`（各需約 320 顆），
所以這裡的「dual」與 Nexys 那個的規模必然不同。實際做了什麼待確認。
