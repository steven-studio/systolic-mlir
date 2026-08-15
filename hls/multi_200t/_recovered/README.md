<!-- 由 eval/scalesim/gen_design_readmes.py 產生。改內容請直接編輯本檔。 -->

# _recovered — 從舊路徑還原的檔案

**這個目錄大部分不是原始碼，是 Vitis HLS 的內部產物。**

| 類別 | 例 | 用途 |
|---|---|---|
| HLS 內部資料庫 | `db___home_..._autopilot_db_*.tcl` / `.bc` / `.adb` / `.json` | 無。應移進 attic |
| HLS 產出的子模組 | `..._fadd_32ns_32ns_32_2_full_dsp_1.v`、`..._fmul_..._max_dsp_1.v` | 說明每個 MAC 用 2+3 = 5 顆 DSP，可據此反推 DSP 數 |
| **HLS 原始碼** | `gemm_4x4__design.cpp` / `.h`、`gemm_4x4__run_hls.tcl` | **有價值** — 是重生 `matmul_4x4x4` 的唯一來源 |
| csynth 報告 | `rpt___..._matmul_4x4x4_csynth.rpt` | 有價值 — HLS 階段的週期估計 |

**缺少 `matmul_4x4x4.v` 本身。**只有子模組被還原。要重生完整核心需要
Vitis HLS 跑一次 `gemm_4x4__run_hls.tcl`。

處理建議：把 `db___*` 移進 attic，保留 `*__design.*`、`*__run_hls.tcl`
與 `rpt___*`。
