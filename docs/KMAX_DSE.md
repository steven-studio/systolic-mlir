# K_MAX_8X8 / K_MAX_4X4 的 Design Space Exploration（variable-K）

**2026-10-12。** 對 1 x 8x8 + 3 x 4x4 FPGA fleet 的兩個 operand 深度做 DSE。所有數字由
`eval/kmax_dse/dse.py` 產生，它只列舉候選、加總與排名；每一個排程都是 compiler 自己的
binary search（`SystolicOptimizer.cpp`）配上 cost model（`CostModel.cpp`、
`FpgaFleet.h`），經 `build/bin/systolic-cost batch --verify` 執行。結果檔：
`eval/kmax_dse/out/summary.md`（表格）、`summary.json`、`candidates.csv`（每組候選、每個
模型、每個網路的加總）、`runs.csv`（決策相關候選的逐層結果）。

單位：100 MHz array clock 的 cycle，FPGA 內部（descriptor 被接受 -> 結果在 DDR）。
一個網路的 makespan = 各層 makespan 的和（層與層依序執行）。**沒有任何 host wall-clock
時間和這些 cycle 比較**；host 端只列 invocation 次數與 bytes。

## 1. 設定

- **工作負載**：ResNet-18 與 AlexNet（224 x 224、batch 1）的 29 個 conv / FC 層，GEMM
  形狀由 `eval/kmax_dse/layers.py` 依 torchvision v0.19.0 的超參數推出（給
  `--torchvision-src` 時逐行對原始碼檢查），`M = H_out·W_out`、`K = C_in·kh·kw`、
  `N = C_out`，FC 為 `M = 1`；另有 5 個標明為 synthetic 的 GEMM。
- **Invocation**：variable-K。K-step reduction 在深度 K_MAX 的陣列上是
  `ceil(K / K_MAX)` 次，最後一次只有 `K mod K_MAX` 步、讀 `ceil(k/8)` 個 window。
- **模型**（`docs/COSTMODEL.md`）：

| 模型 | 內容 | 依據 |
|---|---|---|
| `compute` | 只有 compute term | 比較用（改動前 optimizer 的模型） |
| `seq` | 每次 invocation read + compute + write，同一陣列不重疊，四顆陣列同時，各自 fill | 單一 invocation 在模擬中 held-out MAE 0 cycle |
| **`shared351`**（主要） | `seq` + 四顆 fill 總和 ≤ 3.51 words/cycle（一個 AXI read port） | RTL 支援的 overlap；3.51 是參考值，**非本 bitstream 實測** |
| `shared3627` | 同上，3.627 words/cycle（論文的 DDR3 供給） | beta 敏感度 |
| `shared351_rb` | `shared351` + 每次 invocation 的 result readback（8x8 +213、4x4 +11） | what-if，模擬量測（host 永遠可收） |
| `shared351_hol190` | `shared351` + 每次 fill +190（head-of-line） | what-if，模擬量測的典型值（實測 75～466） |

- **候選**：每個幾何 35 個值（1、2、4、8..64 每 8、80..160 每 16、192..320 每 32、
  384、448、512、576、640、768、896、1024..3584 每 512），1225 組；RAMB18 估計超過 730 的
  不排名。
- **RAMB18 估計**（沒有跑 synthesis）：`448 + 32·ceil(K8/512) + 48·ceil(K4/512)`——PE
  accumulator 448，加上每個 operand bank 各一顆 512 深的 RAMB18（保守：全部放 block RAM）。
  封存的 16/16 bitstream 實際用 480 顆（估計 528），所以這個估計在小 K_MAX 時偏高。
- **排名**：ResNet-18 與 AlexNet 各自除以最佳可行候選後的幾何平均（score，1 = 最佳）；
  同分時取 RAMB18 少、再取 K_MAX 小的。

## 2. 結構性結果

**makespan 對 K_MAX_8X8、K_MAX_4X4 都非遞增**（variable-K：更深的 K_MAX 只會減少
invocation 與最後一個 window 的補零）。249,900 個排程（1225 組 x 34 個 GEMM x 6 個模型）
全部 valid、T* minimal、feasibility monotone，網格上沒有任何一處 makespan 隨 K_MAX 變深
而增加。所以最佳點一定在 RAMB18 上限附近，真正的取捨是 **RAMB18 / timing / host 傳輸
vs 越來越小的 cycle 收益**。

主要模型下 makespan ≈ 讀取 bytes / 14.04 B/cycle（ResNet-18 在 512/512 讀 4.16 GB），也就是
四顆陣列共用的 read port 決定了時間：v1 operand writer 每 cycle 1 word，read 佔每次
invocation 的 92% 左右。K_MAX 只能削掉每次 invocation 的固定成本（H = 95、write、
c_read）與補零。

## 3. RAMB18 / makespan frontier（主要模型 `shared351`）

| K_MAX 8x8 / 4x4 | RAMB18 估計 | ResNet-18 | AlexNet | Score | Invocations R18 / Alex | MEM_WRITE B / invocation |
|---|---|---|---|---|---|---|
| **512 / 512** | **528** | **296,444,070** | **190,801,621** | **1.0085** | 192,341 / 82,952 | 32,768 |
| 896 / 512 | 560 | 294,845,340 | 190,275,403 | 1.0044 | 174,408 / 76,833 | 57,344 |
| 1536 / 512 | 592 | 294,234,734 | 190,214,319 | 1.0032 | 166,980 / 76,004 | 98,304 |
| 896 / 1024 | 608 | 294,845,340 | 189,591,577 | 1.0026 | 124,752 / 46,273 | 65,536 |
| 2048 / 512 | 624 | 294,234,734 | 189,979,554 | 1.0026 | 166,980 / 73,549 | 131,072 |
| 1536 / 1024 | 640 | 294,234,734 | 189,530,493 | 1.0014 | 117,476 / 45,452 | 98,304 |
| 2048 / 1024 | 672 | 294,234,734 | 189,295,728 | 1.0008 | 117,476 / 43,073 | 131,072 |
| 2560 / 1024 | 704 | 294,110,734 | 189,251,426 | 1.0004 | 115,037 / 42,645 | 163,840 |
| 2048 / 1536 | 720 | 294,234,734 | 189,088,092 | 1.0002 | 99,876 / 36,475 | 131,072 |
| 16 / 16（封存） | 528 | 466,215,128 | 294,338,628 | 1.5708 | 4,338,852 / 2,471,131 | 1,024 |

MEM_WRITE 那一欄是 host 端：每次 invocation 都要 MEM_WRITE 整塊 slab（最深幾何的
`ceil(K_MAX/8)·512` byte；RTL 的 `dpti_mem_write_decoder` 只收這個長度），不管這次的 k 多小。

## 4. 結論

| 項目 | 結果 |
|---|---|
| 模型最佳（`shared351`） | **2048 / 1536**：ResNet-18 294,234,734、AlexNet 189,088,092 cycles，RAMB18 估計 720 / 730 |
| 第二（下一個 makespan 等級） | **2560 / 1024**：score 差 0.02%（ResNet-18 −124,000、AlexNet +163,334 cycles），RAMB18 704 |
| **建議** | **K_MAX_8X8 = 512、K_MAX_4X4 = 512** |
| 建議值的預測 makespan | ResNet-18 **296,444,070**、AlexNet **190,801,621** cycles（`shared351`） |
| 與模型最佳的差 | score +0.83%（ResNet-18 +0.75%、AlexNet +0.91%） |
| 與封存 16/16 比 | 16/16 的 score 1.5708：512/512 在 ResNet-18 少 36.4%、AlexNet 少 35.2% cycles；invocation 少 22.6 倍（R18）、29.8 倍（AlexNet） |
| BRAM | 估計 528 / 730（與 16/16 的估計相同：每個 operand bank 一顆 RAMB18，K ≤ 512 不再增加）；**未合成** |
| Timing | **未驗證**（沒有 synthesis / implementation）。封存的 16/16 是 WNS +0.079 ns；K_MAX 512 時 operand 位址與計數器變寬（`K_W` 4 → 9 bit），operand bank 從可能的 LUTRAM 變 block RAM；> 512 每個 bank 要串接多顆 RAMB18 |
| beta 敏感度 | 512/512 的 score：3.51 → 1.0085、3.627 → 1.0088、無共用上限（`seq`）→ 1.0134。模型最佳在 3.51 是 2048/1536、在 3.627 是 2560/1024——前兩名的次序隨 beta 對調，差距 < 0.05% |
| What-if | 512/512 的 score：readback（`shared351_rb`）1.0150、head-of-line（`shared351_hol190`）1.0172；16/16 在同樣兩個 what-if 下是 1.8314、2.2309 |
| 模擬誤差 | 單一 invocation：Verilator held-out 58 個 case / 86 個 invocation（含 512/512 的 K = 1、511、512、513、1025）**0 cycle**；整條 pipeline 的 co-simulation（含 512/512 的 36 個標準 case 與 7 個 K_MAX 邊界 case）每次 invocation 的 FPGA 內部 cycle 與模型 **0 誤差**、結果全部 exact；同時執行：head-of-line 不在模型裡，模擬中每個波次 +0～+33%（`docs/COSTMODEL.md` 第 5 節） |

**為什麼不選模型最佳。** 512/512 以上的 frontier 總共只差 0.83%，小於模型外的誤差
（同時執行的 head-of-line 在模擬中最多 +33%、板上 DDR3 沒模擬、readback 與 host 傳輸），
所以模型**分不出** frontier 上的各點（512/512 到 2048/1536）誰在板上比較快——這部分
證據不足，要合成與板測才知道。分得出的是：

1. 512/512 比 16/16 快約 1.56 倍（主要模型；ResNet-18 1.57 倍、AlexNet 1.54 倍），在 6 個
   模型下都成立（16/16 的 score 1.57～2.23；compute-only 下是 6.70）；這個差距遠大於模型
   誤差。
2. 512/512 是 frontier 的起點：RAMB18 估計沒有增加、K_MAX 最淺（timing 風險最低）、
   每次 invocation 的 MEM_WRITE 32 KB（2048/1536 是 128 KB）。
3. 比 512/512 更淺時：128/128 的 score 1.0439、256/256 是 1.0193（512/512 是 1.0085），
   收益還不算小，所以不建議更淺。

**證據不足的部分（照實說）**：RAMB18 與 timing 只有估計，沒有合成；beta 3.51 / 3.627
不是本 bitstream 的實測值；head-of-line 與 readback 只在模擬中量過；同步 runtime 一次
只發一個 invocation，非同步 runtime（`docs/ASYNC.md`）四顆同時忙但只在 co-sim 跑過，所以
`shared351` 的 makespan 是「四顆同時執行」這個 RTL 做得到的情況的預測，不是現在板上量到的
行為。**FPGA Board Regression：NOT RUN。**

## 5. 重現

```bash
cmake --build build -j2
python3 eval/kmax_dse/layers.py --torchvision-src DIR   # 可選：對 torchvision 原始碼檢查
python3 eval/kmax_dse/dse.py --verify --jobs 2           # 約 15 分鐘（2 核）
```
