<!-- 由 eval/scalesim/gen_design_readmes.py 產生。改內容請直接編輯本檔。 -->

# rtl — 手寫 8x8 fold 陣列

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
