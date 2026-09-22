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

工作目錄一律是 `rtl/multi_200t/fold_pipelined/`，且需先
`source ~/tools/Xilinx/2026.1/2026.1/Vivado/settings64.sh`。

`build_kmax/` 被 gitignore 擋著，重跑一次就覆蓋，所以要引用的那幾份
已複製出來：傳輸實驗的在 `eval/transport/`，資源掃描（論文 tab:resources）
的在 `eval/resources/util_k<tag>.csv`。

| 數字 | 指令 | 輸出 |
|---|---|---|
| 任一 (K_MAX, N, BAUD) 的 LUT / FF / BRAM / DSP / WNS / WHS | `vivado -mode batch -source uart/build_kmax.tcl -tclargs <K_MAX> [DBG] [DIRECTIVE] [N] [BAUD]` | `build_kmax/k<tag>/summary.csv`、`.../reports/post_route_utilization.rpt` |
| N=8, K_MAX=16：**DSP 384 / 740 = 51.89%、LUT 51,131 / 133,800 = 38.21%、BRAM 136 / 365 = 37.26%** | 上列，`-tclargs 16` | `eval/transport/util_k16_b115200.csv`、`eval/transport/post_route_utilization_k16_b115200.rpt` |
| 同組態但 BAUD=2000000：LUT 51,089（差 42 —— 除數 868→50，計數器少 4 bits） | 上列，`-tclargs 16 0 Default 8 2000000` | `eval/transport/util_k16_b2000000.csv` |
| 矽上週期 **125**（K_MAX=16, k_dim=16, N=8）、bit-exact | `cd tools && python3 test_uart_kmax.py --kmax 16 --baud <115200\|2000000>` | stdout `hardware cycles`、`BIT-EXACT` |
| 兩速率往返時間，與主機端固定成本 **13.7 ms** | `cd tools && python3 bench_uart.py --kmax 16 --baud <rate> --reps 20 --csv <out>` | `eval/transport/bench_115200.csv`、`eval/transport/bench_2m.csv` |
| tab:resources 的 LUT / FF / BRAM / DSP / fmax（N=8：k_max 64/256/512/1024/2048；N=4 各點） | `build_kmax.tcl` 各跑一次 | `eval/resources/util_k<tag>.csv`（複製自 `build_kmax/k<tag>/summary.csv`） |
| **H = 95 的拆解 22 + 67 + 6**（論文 eq:H-decomp）：在 PE(N-1,N-1) 與陣列輸出控制器打時間戳 | `mkdir -p sim_out/h && verilator --binary -Wno-fatal --top-module tb_array_h_decomp -DFP_MUL_LAT=8 -DFP_ADD_LAT=11 -GN=8 -GK=8 tb/fp_model.sv core/systolic_pe_bram.sv core/systolic_array.sv tb/tb_array_h_decomp.sv -o tb_h --Mdir sim_out/h && sim_out/h/tb_h` | stdout `H = g + r + c = 22 + 67 + 6 = 95`，`T = 117`（與板測相同）。8/11 是 `ip/fp32/*/*.xci` 的 C_Latency；用 9/12 會得到 101，對不上板測 |
| survey 四列板測 **903 / 685 / 2491 / 1837** 與多次 invocation 的逐次拍數 | `python3 tools/measure_fold.py --kmax <K_MAX> --K <K> --label <layer>` | `results/tbd_633/summary.csv`（含 bitstream sha256 前 12 碼）與同目錄的 `.log`（`.gitignore` 擋 `*.log`，新增要 `git add -f`） |
| **32 banks：先預測 124，後板測 146 / 202 / 394**（論文 §7.3.5） | build：`-tclargs 256 0 Default 8 115200 32`；燒錄：`program_kmax.tcl -tclargs 256_banks32`；量：`python3 tools/measure_fold.py --kmax 256 --K <8\|64\|256> --label banks32 --h 124 --tag 256_banks32` | `results/tbd_633/summary.csv` 的 banks32 三列（sha256 `83302e6057d7`）、`eval/resources/util_k256_banks32.csv`。模擬預測：上一列的指令加 `-GACC_BANKS=32` → `22 + 96 + 6 = 124` |

LUT 的分母以合成報告為準：部件標稱 134,600，該次建置有 800 顆 prohibited，
Available 是 133,800，所以報告印的是 38.21%。論文 7.5 寫「51.2k LUT（38% of
the part）」用的是標稱值 134,600，兩者四捨五入後同為 38%，但引用時要指明是哪
一個分母。DSP 384 = 6N² 是這份報告第一次替論文 §4 的資源數字提供合成證據。

今天這一點也延長了 7.5 的 K_MAX 掃描表：該表 N=8 在 k_max 64/256/512/1024/2048
量到 LUT 51,157--51,212、DSP 384、BRAM 136--160；k_max=16 的 51,131 / 384 / 136
落在同一條線的下端，符合「buffer 變淺、位址位元變少」的預期。

最後一列的判準是**不變性**而不是單一數字：read 的殘差在 115200 下是
13.68 ms、在 2 Mbaud 下是 13.76 ms，中間隔著 17.36 倍的位元率。
on-chip counter 兩邊都是 125，兩邊都 bit-exact —— 計算被當成對照變數握住，
差額才能讀成傳輸。

### 1.2 成本模型

| 數字 | 來源 |
|---|---|
| `tileOverhead = 95` | 2026-09-12 的 N=8 板測：`125 = 16 + 2(8-1) + 95`。CGO 稿另在 N=4 量到同一個常數（H₄ = H₈ = 95，零殘差）。拆解 22 + 67 + 6 見 1.1 |
| `cost_model_fold_rtl.mlir` 釘的 17 個板測點（k = 8…1728、k_max 256/512/1024/2048、多次 invocation）＋ 32 banks 的 146/202/394（同一幾何、`tile_overhead = 124` 的第二顆 device） | `./configure.sh --test`（或 `cmake --build build --target check-systolic`）→ `test/Systolic/cost_model_fold_rtl.mlir` |
| l1_bytes 容量檢查（論文 ch05 §5.3.1：開／關各落到不同 device；放不下報錯） | 同上 → `test/Systolic/select_device_l1.mlir`、`bad_l1_capacity.mlir` |
| 十一種陣列組態的 makespan（論文 tab:config-sweep） | `python3 eval/config_sweep/sweep_opt.py`（實跑 `systolic-opt --systolic-select-device`，不是 Python 重演） | 

### 1.3 SCALE-Sim 幾何項驗證

`eval/scalesim/`。設定檔 `nexys_8x8_os.cfg`，切分依 `VALIDATION_PLAN.md`
與 `VALIDATION_PLAN_AMENDMENT_01.md`（皆在取得任何量測值之前宣告）。

### 1.4 建置環境

`./configure.sh --test`。LLVM/MLIR 18，Ninja，Release。Verilator 5.020 跑
`tb/` 底下的 bench；Icarus 12 也能跑 `tb_array_h_decomp`（見檔頭）。

### 1.5 論文對應的 repo 狀態

論文（ntu-thesis）附錄 A.4 引用的是 tag `thesis-draft-20260916`：板測硬體
仍是 `paper-hw-v1`（RTL 內容未變），這個 tag 多的是之後加進來的
測試、量測紀錄與 32 banks 的 build 參數。

---

## 2. MLIR / compiler reproducibility（2026-09-22 新增）

這一節回答另一種「可重現」：不是數字能不能追到 bitstream，而是第三方能不能從
clean checkout 建出同一顆 compiler，對同一份 MLIR input 跑同一條 pass pipeline，
並看到相同的 intermediate/output IR。

### 2.1 Clean build 與 canonical binary

建議從明確的 source revision 開始：

```bash
git rev-parse HEAD
git status --short
LLVM_VERSION=18 ./configure.sh --fresh --test
```

一般已設定好的 build 可直接：

```bash
./configure.sh --test
```

本文件後續一律使用：

```bash
./build/bin/systolic-opt
```

其中 `tools/systolic-opt/` 是 source directory，`build/tools/systolic-opt/` 是
CMake/Ninja build directory；不要把這三個路徑誤認成三個不同版本的 compiler。

確認 binary 與已註冊的 Systolic passes：

```bash
file ./build/bin/systolic-opt
./build/bin/systolic-opt --help | grep systolic
```

### 2.2 `linalg.matmul -> systolic.pe_array`

Input：`test/Systolic/matmul.mlir`

```bash
./build/bin/systolic-opt test/Systolic/matmul.mlir \
  --convert-matmul-to-systolic="rows=8 cols=8"
```

8x8x8 case 的 output 應可直接看到：

```mlir
systolic.stream
systolic.pe_array<8 x 8> stationary(weight)
```

16x16x16 case 在這個 pass 中維持 `linalg.matmul`，因為它不等於設定的 8x8
array geometry。

Machine-checkable regression：

```bash
./build/bin/systolic-opt test/Systolic/matmul.mlir \
  --convert-matmul-to-systolic="rows=8 cols=8" | \
  FileCheck test/Systolic/matmul.mlir
```

### 2.3 `systolic.pe_array -> scf.for + systolic.mac`

Input：`test/Systolic/expand_to_mac.mlir`

```bash
./build/bin/systolic-opt test/Systolic/expand_to_mac.mlir \
  --convert-matmul-to-systolic="rows=8 cols=8" \
  --expand-pe-array-to-mac
```

預期：

- `systolic.pe_array` 消失
- `systolic.stream` 消失
- 出現三層 `scf.for`
- 出現 `systolic.mac`

因此這條 lowering 可寫成：

```text
linalg.matmul
  -> systolic.stream + systolic.pe_array
  -> scf.for + systolic.mac
```

### 2.4 `linalg.matmul -> systolic.matmul_tile`

Input：`test/Systolic/tile_matmul.mlir`

```bash
./build/bin/systolic-opt \
  --systolic-tile-matmul="tile-m=4 tile-n=4 tile-k=4" \
  test/Systolic/tile_matmul.mlir
```

16x16x16 GEMM 以 4x4x4 切分，regression test 預期總共 64 個
`systolic.matmul_tile`；K-dimension tiles 透過 accumulator input 串接。

### 2.5 Device selection / `est_cycles`

Input：`test/Systolic/select_device.mlir`

```bash
./build/bin/systolic-opt --systolic-select-device \
  test/Systolic/select_device.mlir
```

output 應看到 `systolic.matmul_tile` 被標註 `on @acc_...` 與
`est_cycles`。這裡的 regression fixture 有自己的測試參數；**不可把測試中的
小常數直接當成論文板測的 H=95**。論文硬體校準數字仍以第 1 節的板測 provenance
為準。

### 2.6 Overlap scheduling / `start_cycle`

Input：`test/Systolic/schedule_overlap.mlir`

```bash
./build/bin/systolic-opt \
  --systolic-tile-matmul="tile-m=8 tile-n=8 tile-k=8" \
  --systolic-select-device \
  --systolic-schedule-overlap \
  test/Systolic/schedule_overlap.mlir
```

目前 regression test 會檢查 `est_cycles = 28`，以及例示的
`start_cycle = 16, 44, 72`。

這證明的是：

- compiler 有計算 overlap-aware schedule；
- schedule 以 `start_cycle` attribute 寫回 IR。

這**還不等於** runtime/FPGA 已按照這些 cycle 發射工作。目前
`SystolicTileToFpga.cpp` 沒有消費 `start_cycle`；因此在 runtime 真正 enforce
它以前，應稱為 compiler schedule metadata/model output，而不是 hardware-enforced
launch timing。

### 2.7 Scheduled tile -> LLVM dialect runtime call

Implementation：`lib/Systolic/Transforms/SystolicTileToFpga.cpp`

Pass：

```text
--systolic-tile-to-fpga
```

這個 rewrite 會 match `systolic.matmul_tile`、要求已有 device assignment，
把 device symbol 映射成整數 `device_id`，準備/pad operand，並以
`LLVM::CallOp` 產生 external runtime calls。textual MLIR 中對應 `llvm.call`。

目前它宣告/呼叫：

```text
systolic_dispatch_open()
systolic_dispatch_matmul4x4_dev(handle, device_id, A, B, C_init, C_out)
```

而且 executable lowering 目前只接受 `m,n,k <= 4`。

### 2.8 目前尚未閉合的 compiler/runtime interface

新版 runtime contract `runtime/lib/dispatch/systolic_dispatch_new.h` 定義的是：

```c
#define SYS_DISPATCH_R      8
#define SYS_DISPATCH_C      8
#define SYS_DISPATCH_K_MAX  64

int systolic_dispatch_open(void);
int systolic_dispatch_matmul(int handle, int K,
                             const float *A, const float *B,
                             const float *C_init, float *C_out);
```

也就是說，目前 repo 中至少存在兩個 interface generation：

```text
scheduling lowering:
  systolic.matmul_tile
    -> systolic_dispatch_matmul4x4_dev(...)

new 8x8 runtime:
  systolic_dispatch_matmul(handle, K, ...)
```

在兩者真正接起來並跑過 end-to-end regression 前，不應把它們寫成「已經完全
閉合的單一路徑」。

理想的最終 reproducibility artifact 應從同一份 input 一路留下：

```text
00-input.mlir
10-tiled.mlir
20-selected.mlir
30-scheduled.mlir
40-runtime-call.mlir
```

最後再連到**目前實際使用的 8x8 runtime ABI、bitstream 與 bit-exact board run**。


---

## 3. 尚未可追溯（處理順序即優先順序）

### 3.1 conv2d 的 48/48 bit-exact 是在哪顆 bitstream 上跑的

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

### 3.2 conv2d 的 tile 數從未與硬體對照

三份 CSV 的 `measured_tiles` 欄位全部是空的，只有 `predicted_tiles` 有值。
也就是說成本模型預測的 tile 數在 conv2d 上**沒有任何驗證**。

這與 `fixedOverhead` 在今天之前的狀況同型：欄位存在、從未被填、模型安靜地
無法被檢查。

### 3.3 兩份 CSV 內容完全相同

```
dfdbec135d284e4e853202254f8a0dd3  sweep_results_48_bitexact.csv
dfdbec135d284e4e853202254f8a0dd3  sweep_results_nexys_8x8.csv
```

需決定哪一份是正本，另一份刪除或移進 `attic/`。引用時指到兩個檔名會造成
不必要的疑問。

### 3.4 `matmul_top_dual` 的數字沒有人確認過還能重現

投影片第 4 頁引用 `explore_40mhz_util.rpt`（LUT 31.5%、DSP 86.5%）。
報告檔存在且內容合理，但那次建置是 2026-08-12，之後沒有人重跑過
`hls/multi_200t/vivado/build_project.tcl`。

若論文要保留這一列，至少要確認該 tcl 仍能執行完成。

### 3.5 重構前的數字曾留在第 1 節（2026-09-13 移出，此處記其去向）

第 1 節的判準是「指得出來」，但以下四項在 ctx 移除與 paper-hw-v1 之後已經
指不出來了，仍留在表裡到今天為止。移出不刪除，去向如下：

- **矽上週期 134 / 150 / 182**（K_MAX=64，k_dim=16/32/64）：對應 H = 104。
  重構後 H = 95，同樣三點應為 125 / 141 / 173，但**只有 K_MAX=16 的 125
  實測過**。要保留這三點就得重跑
  `python3 test_uart_kmax.py --kmax 64 --k <16|32|64>`。
- **drain 111**：`tb/sim_kmax.tcl` 的輸出。論文 6.3.2 仍引用，而該節自己的
  註解指出 111 的拆解只在 N=8 成立（套到 N=4 會預測 115，板上量到 111）。
  論文的 TODO 寫「用 tb_pe_counters 重測」，但那個 testbench 量的是
  pass-through 延遲與 bank counter 序列，不量 drain —— 指令本身要先重寫。
- **一顆 PE 的 1,318 LUT / 4 DSP**：paper-hw-v1 之前的 build。現在是
  6 DSP/PE；2026-09-12 的 N=8 合成是 51,131 LUT / 64 PE ≈ 799 LUT/PE
  （含 UART、operand buffer、feeder，所以是每 PE 的上界）。多的兩顆 DSP
  換回了每 PE 約 520 個 LUT，這是這個設計裝得進 200T 的原因。
- **8x8+4x4 的 78.1% LUT**：`eval/scalesim/dual.tcl` 仍在，但輸出寫到
  `/tmp/util_dual.rpt`，重開機即消失；而且那是 HLS 的 5 DSP/PE 設計。
  `pe.tcl` 同樣寫 `/tmp/util_pe.rpt`。要保留就得把輸出路徑改進 repo。

另外兩個純粹的路徑錯誤，一併記在這裡：舊版寫「工作目錄一律是
`hls/multi_200t/fold_pipelined/rtl/`」，該路徑不存在 —— RTL 在
`rtl/multi_200t/fold_pipelined/`，而 `hls/multi_200t/fold_pipelined/` 是
HLS 的 C++ 版本，兩者是不同東西；並且引用了從未存在的
`eval/scalesim/KMAX_SCALING.md`。

---

## 4. 明確可以移進 attic 的東西

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

## 5. 這份文件怎麼維護

新增一個要發表的數字時，在第 1 節加一列。做不到就代表那個數字還不能用。

第 2 節清空的那天，這個 repo 就不亂了 —— 與檔案數量無關。
