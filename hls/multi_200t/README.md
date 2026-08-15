<!-- 由 eval/scalesim/gen_design_readmes.py 產生。改內容請直接編輯本檔。 -->

# multi_200t — Nexys Video 的主線

| | |
|---|---|
| 板子 | Nexys Video (`xc7a200tsbg484-1`) |
| 約束檔 | `vivado/nexys_video.xdc` |

這個目錄同時容納三代設計，**根目錄的 `design.h` 只屬於第一代**。

## 三個上板頂層

| 頂層 | 陣列 | 核心來源 | DSP | 時脈 | 主機協定 |
|---|---|---|---|---|---|
| `rtl/matmul_top_dual.v` | **2 x 4x4** | 根目錄 `design.h`（R=C=K=4）→ `matmul_4x4x4` | 640 (86.5%) | 40 MHz，UART 預設 baud | `fpga_tile.h`（帶 dev byte） |
| `rtl/matmul_top_rk.v` | 1 x 8x8 runtime-K | `_rk/design.h` → `matmul_8x8x8` | 待確認 | 40 MHz，UART **2 Mbaud** | `fpga_matmul_rk_new.h` |
| `_fp_beat/rtl_fp/systolic_uart_fold_top.sv` | 1 x 8x8 fold | **手寫 RTL，非 HLS** | 256 (34.6%) | 100 MHz | `fpga_matmul_fold.h` |

`matmul_top_dual` 是唯一一顆真的放了兩個陣列、協定帶陣列選擇位元組的硬體。
`select-device` 這個 pass 要的異質裝置，目前只有它是真的。

## 承載的結果

- `vivado/explore_*_util.rpt`、`explore_*_timing.rpt` — `matmul_top_dual`
- `runtime/logs/sweep_nexys_8x8.log` — conv2d **47/48**（`conv_sweep_003`
  以 `rc=-2` 讀取逾時失敗，是傳輸層問題不是數值問題）

## 時脈：三處陳舊文件都說 20 MHz，都是錯的

`clk_gen.v` 現在是 `CLKOUT0_DIVIDE_F(25.0)`，1000 MHz / 25 = **40 MHz**。
但有三處還寫著 20 MHz：

| 位置 | 寫的 | 實際 |
|---|---|---|
| 埠名 `clk_out20` | 20 MHz | 40 MHz（`matmul_top_rk.v:51` 已註明是歷史遺留） |
| `clk_gen.v` 的註解 | `/ 50 = 20MHz` | 已於 2026-08-15 修正 |
| `clock_explore_new.tcl` 檔頭 | 「仍是 50.0」 | 已改成 25.0 |

`clock_explore_new.tcl` 的檔頭說「知道答案之後把 CLKOUT0_DIVIDE_F 改掉並
正式重建」—— 改是改了（50.0 → 25.0），但**是否有在 40 MHz 下做過正式的
post-route 建置，未確認**，因為 `build_project.tcl` 現在跑不起來（見下）。

所以引用 `explore_*.rpt` 時的正確說法是：那是約束探索的結果，RTL 目前
設定在 40 MHz，45 / 50 MHz 僅為探索，未經正式建置。

## 重建

`vivado/build_project.tcl` 的 top 是 `matmul_top_rk`（不是 dual），且需要
`work_systolic/hls/impl/ip` —— Vitis HLS 的產物，`.gitignore:75` 排除。

**2026-08-15 於本機確認：該目錄不存在，且 `which vitis_hls` 為空。**
因此這條線目前無法重建。HLS 原始碼保留在 `_recovered/gemm_4x4__design.cpp`，
要重生需先安裝 Vitis HLS。

重建 `matmul_top_dual` 更沒有現成腳本 —— `build_project.tcl` 的 top 是
`matmul_top_rk`。
