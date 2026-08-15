# 數字的來源

**建立於 2026-08-15。**

這份文件的判準只有一條：**投影片與論文裡的每個數字，能不能指出「哪支腳本、
哪顆 bitstream、哪份輸出檔」。** 能指出的列在第 1 節，指不出的列在第 2 節。

第 2 節是這個 repo 目前唯一真正的問題。其餘看起來混亂的部分（34 個 tcl、
六個設計目錄、三套 UART 協定）都是歷史，不影響任何一個要發表的數字 ——
處理方式是移進 `attic/`，不是整理。

平台：Nexys Video，`xc7a200tsbg484-1`，100 MHz，Vivado 2026.1。

---

## 1. 可追溯的數字

### 1.1 手寫 fold RTL（今日的主線）

| 數字 | 指令 | 輸出 |
|---|---|---|
| K_MAX=16/64/128 的 LUT / FF / DSP / WNS | `vivado -mode batch -source build_kmax.tcl -tclargs <K>` | `build_kmax/k<K>/summary.csv` |
| drain 111、feed K+7、errors 0 | `vivado -mode batch -source sim_kmax.tcl -tclargs <K>` | stdout 的 `KMAXCSV,` 那行 |
| 矽上週期 134 / 150 / 182 | `python3 test_uart_kmax.py --kmax 64 --k <16\|32\|64>` | stdout `hardware cycles` |
| bit-exact | 同上 | stdout 三行 `BIT-EXACT` |
| 一顆 PE 的 1,318 LUT / 4 DSP | `/tmp/pe.tcl`（見 `eval/scalesim/`） | `/tmp/util_pe.rpt` |
| 8x8+4x4 的 78.1% LUT | `eval/scalesim/dual.tcl` | `/tmp/util_dual.rpt` |

工作目錄一律是 `hls/multi_200t/_fp_beat/rtl_fp/`，且需先
`source ~/tools/Xilinx/2026.1/2026.1/Vivado/settings64.sh`。

彙整見 `eval/scalesim/KMAX_SCALING.md`。

### 1.2 成本模型

| 數字 | 來源 |
|---|---|
| `fixedOverhead = 104` | 由 1.1 的矽上兩點（k_dim=16 → 134、k_dim=64 → 182）減去幾何項 `k_dim+14` |
| 六個 est_cycles | `cmake --build build --target check-systolic` → `test/Systolic/cost_model_fold_rtl.mlir` |
| k_dim=32 → 150 的「先預測後驗證」 | 測試先寫入 150，之後 `test_uart_kmax.py --kmax 64 --k 32` 回報 150 |

### 1.3 SCALE-Sim 幾何項驗證

`eval/scalesim/`。設定檔 `nexys_8x8_os.cfg`，切分依 `VALIDATION_PLAN.md`
與 `VALIDATION_PLAN_AMENDMENT_01.md`（皆在取得任何量測值之前宣告）。

### 1.4 建置環境

`./configure.sh --test`。LLVM/MLIR 18，Ninja，Release。

---

## 2. 尚未可追溯（處理順序即優先順序）

### 2.1 conv2d 的 48/48 bit-exact 是在哪顆 bitstream 上跑的

**這是唯一一個會影響論文主張的洞。**

已知：

- `runtime/harness/sweep_out/sweep_results_nexys_8x8.csv` 記錄 48 個組態全部
  `BIT-EXACT`，涵蓋 stride、dilation、padding、多通道、batch
- 對照 `sweep_results_template.csv`（同樣 48 筆）是 20 PASS / 24 FAIL /
  4 BIT-EXACT，兩者構成很強的前後對照
- commit `e3b8817` 的訊息是
  `Add dim/ndev-generic tile path; 48/48 bit-exact on Nexys Video 2x(8x8)`

矛盾之處：

- 呼叫鏈是 `fpga_conv2d_im2col_*_auto` → `fpga_matmul_tiled_auto` →
  `fpga_matmul4x4_reliable(fd, A[16], B[16], C_init[16], C_out[16])`，
  也就是 **4x4 的 192 byte 協定**
- 但 `runtime/lib/fpga_matmul_fold.h` 的檔頭明寫：
  「`fpga_matmul4x4.h` … Targets the old 4x4 bitstream.
  **Nothing on this board has accepted it since the 8x8 rewrite.**」
- 而 `hls/multi_200t/vivado/explore_40mhz_util.rpt` 的 top 是
  `matmul_top_dual`，DSP 640/740 = 86.5%。以 HLS 的 5 DSP/PE 計算，
  640 = 2 x 64 PE，指向兩個 **8x8**，不是兩個 4x4

三者無法同時為真。要確認的指令：

```bash
git log --format='%h %ad %s' --date=short -- \
    runtime/harness/sweep_out/sweep_results_nexys_8x8.csv
git show e3b8817 --stat
grep -rn "MATRIX_ELEMENTS\|192\|ndev\|dev" runtime/lib/fpga_matmul4x4_reliable.c | head
```

在確認之前，投影片與論文**不應宣稱 conv2d 已在目前的 fold bitstream 上
驗證**。目前能說的是：conv2d 在某一顆歷史 bitstream 上 48/48 bit-exact，
而那顆是哪一顆待查。

### 2.2 conv2d 的 tile 數從未與硬體對照

三份 CSV 的 `measured_tiles` 欄位全部是空的，只有 `predicted_tiles` 有值。
也就是說成本模型預測的 tile 數在 conv2d 上**沒有任何驗證**。

這與 `fixedOverhead` 在今天之前的狀況同型：欄位存在、從未被填、模型安靜地
無法被檢查。

### 2.3 兩份 CSV 內容完全相同

```
dfdbec135d284e4e853202254f8a0dd3  sweep_results_48_bitexact.csv
dfdbec135d284e4e853202254f8a0dd3  sweep_results_nexys_8x8.csv
```

需決定哪一份是正本，另一份刪除或移進 `attic/`。引用時指到兩個檔名會造成
不必要的疑問。

### 2.4 `matmul_top_dual` 的數字沒有人確認過還能重現

投影片第 4 頁引用 `explore_40mhz_util.rpt`（LUT 31.5%、DSP 86.5%）。
報告檔存在且內容合理，但那次建置是 2026-08-12，之後沒有人重跑過
`hls/multi_200t/vivado/build_project.tcl`。

若論文要保留這一列，至少要確認該 tcl 仍能執行完成。

---

## 3. 明確可以移進 attic 的東西

`runtime/attic/` 已經存在，沿用同一個慣例即可。**移動不刪除。**

| 路徑 | 理由 |
|---|---|
| `hls/multi_200t/_recovered/db___home_*.tcl` | Vitis HLS 的內部資料庫檔被還原出來，不是原始碼 |
| `hls/multi_200t/_recovered/` 其餘 | 還原自舊路徑的重複品 |
| `sweep_results_48_bitexact.csv` 或 `_nexys_8x8.csv` | 二者之一，見 2.3 |

**不要**在確認 2.1 之前動 `runtime/lib/` 底下任何東西 —— 那三套協定看起來
重複，但 `fpga_matmul_fold.h` 的檔頭說明了它們為何不能互通，而錯用的症狀是
「在 tile 迴圈深處讀取逾時」，不是明顯的失敗。

---

## 4. 這份文件怎麼維護

新增一個要發表的數字時，在第 1 節加一列。做不到就代表那個數字還不能用。

第 2 節清空的那天，這個 repo 就不亂了 —— 與檔案數量無關。
