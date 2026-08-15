#!/usr/bin/env python3
"""
gen_design_readmes.py -- 在每個設計目錄放一份 README.md。

    cd ~/work/systolic-mlir
    python3 eval/scalesim/gen_design_readmes.py          # 預覽
    python3 eval/scalesim/gen_design_readmes.py --write  # 寫入

為什麼
------
這個 repo 有六個設計目錄、兩塊板子、三套 UART 協定,而檔名不帶板子資訊。
後果不是抽象的:`runtime/harness/sweep_out/sweep_results_nexys_8x8.csv` 裡
裝的是 Arty 4x4 的結果,而同一個 commit 的執行 log 說 Nexys 那次是 47/48。
那份 CSV 差一點就以「48/48 bit-exact on Nexys」的形式進了論文。

搬目錄會弄壞每一支 tcl 的相對路徑,而問題本來就不是檔案的位置,是
「看不出哪個檔屬於哪塊板子」。所以只加標籤,不動結構。

加完之後 `find hls -name README.md | xargs head -20` 就是一張硬體地圖。

標記為「待確認」的欄位是我從原始碼推不出來的,跑一次對應的指令即可補上。
"""

import os
import sys

HEADER = "<!-- 由 eval/scalesim/gen_design_readmes.py 產生。改內容請直接編輯本檔。 -->\n\n"

READMES = {

"hls/gemm_4x4": """# gemm_4x4 — Arty A7-35T,單一 4x4

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
""",

"hls/gemm_4x4_dual": """# gemm_4x4_dual — 雙 4x4 的 HLS 探索

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
""",

"hls/gemm_8x8": """# gemm_8x8 — Arty A7-35T,單一 8x8

| | |
|---|---|
| 板子 | Arty A7-35T (`xc7a35ticsg324-1L`，`run_hls.tcl`) |
| HLS top | `matmul_8x8x8` |
| 狀態 | 待確認是否曾上板 |

`matmul_8x8_only.mlir` / `matmul_8x8_systolic.mlir` 是對應的 MLIR 測試輸入。

xc7a35t 只有 90 顆 DSP，而 HLS 的 FP MAC 約 5 顆 DSP，8x8 全展開需要
64 x 5 = 320 顆 —— **這顆很可能塞不下 Arty**。若確實如此，值得在此註明，
它本身就是一個資源受限的資料點。
""",

"hls/multi_200t": """# multi_200t — Nexys Video 的主線

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
| `fold_pipelined/rtl/systolic_uart_fold_top.sv` | 1 x 8x8 fold | **手寫 RTL，非 HLS** | 256 (34.6%) | 100 MHz | `fpga_matmul_fold.h` |

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
""",

"hls/multi_200t/fold_pipelined": """# fold_pipelined — 8x8 fold,目前的主線

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
""",

"hls/multi_200t/fold_pipelined/rtl": """# rtl — 手寫 8x8 fold 陣列

| | |
|---|---|
| 板子 | Nexys Video (`xc7a200tsbg484-1`) |
| 頂層 | `systolic_uart_fold_top.sv` |
| 陣列 | `systolic_array_fold.sv` #(N)，8x8 與 4x4 皆為其 wrapper |
| PE | `systolic_pe_fold.sv` — 1,318 LUT / 1,725 FF / 4 DSP（獨立合成實測） |
| 時脈 | 100 MHz |
| 浮點 | Vivado floating-point IP（`rtl_fp_pe_test/`，gitignore，本機產物） |
| 協定 | `fpga_matmul_fold.h`；wire format 見 `systolic_uart_fold_top.sv` 檔頭 |

**這是整個 repo 唯一從 RTL 到 bitstream 到矽上量測全鏈可重建的設計。**

## 承載的結果

| | |
|---|---|
| 週期公式 | `total = k_dim + 118`，矽上於 k_dim = 16 / 32 / 64 驗證（134 / 150 / 182） |
| 正確性 | 三者皆 bit-exact，含 runtime-K（k_dim < K_MAX） |
| K_MAX 縮放 | 16 / 64 / 128 皆 timing met，見 `eval/scalesim/KMAX_SCALING.md` |
| 成本模型 | `fixedOverhead = 104`，見 `test/Systolic/cost_model_fold_rtl.mlir` |

## 重建

```bash
source ~/tools/Xilinx/2026.1/2026.1/Vivado/settings64.sh
vivado -mode batch -nojournal -source build_kmax.tcl -tclargs 64   # 約 25 分鐘
cat build_kmax/k64/summary.csv
vivado -mode batch -nojournal -source sim_kmax.tcl -tclargs 16     # 約 2 分鐘
python3 test_uart_kmax.py --kmax 64 --k 32                          # 上板
```

`rtl_fp_pe_test/` 底下的 `.xci` 是本機 Vivado 產物，不在版控裡。
路徑可用 `XCI_DIR` 環境變數覆寫。
""",

"hls/multi_200t/_recovered": """# _recovered — 從舊路徑還原的檔案

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
""",
}


def main():
    write = "--write" in sys.argv
    if not os.path.isdir("hls"):
        print("ERROR: 請在 systolic-mlir 的根目錄執行")
        return 1

    for path, body in READMES.items():
        if not os.path.isdir(path):
            print(f"  略過（目錄不存在）: {path}")
            continue
        dest = os.path.join(path, "README.md")
        exists = os.path.exists(dest)
        if write:
            if exists:
                # multi_200t 已經有一份 README,不覆寫,改寫成 README_HW.md
                dest = os.path.join(path, "README_HARDWARE.md")
            open(dest, "w", encoding="utf-8").write(HEADER + body)
            print(f"  寫入 {dest}")
        else:
            note = "（已有 README.md，會改寫成 README_HARDWARE.md）" if exists else ""
            print(f"  預覽 {dest} {note}  {len(body.splitlines())} 行")

    if not write:
        print()
        print("以上為預覽。確認後加 --write 實際寫入。")
    else:
        print()
        print("硬體地圖：")
        print("  find hls -name 'README*.md' | xargs head -12")
    return 0


if __name__ == "__main__":
    sys.exit(main())
