# Cost model：Read / Compute / Write（variable-K）

**2026-10-12。** 一次 invocation 的成本由 `lib/Systolic/Transforms/CostModel.cpp` 計算，
這是唯一的成本公式所在。`--systolic-binary-search-tile-matmul`（經
`SystolicOptimizer.cpp`）、`--systolic-select-device`、`--systolic-cost-analysis`、
`--systolic-schedule-overlap` 與 `build/bin/systolic-cost` 都呼叫它；
`SystolicOptimizer.cpp` 不再有自己的 `2g - 2 + H`。FPGA fleet 的常數在
`include/Systolic/FpgaFleet.h`，`systolic-matmul` 用它宣告 fleet，`systolic-cost` 用它
定價。

**Variable-K。** K_MAX 是 operand buffer 的**容量**，不是每次 invocation 的深度。
runtime 送 `job.k = k_j`（這次 invocation 實際的 reduction 深度，
`runtime/fpga_matmul_tiled.cpp`），RTL 讀 `ceil(k_j / 8)` 個 window、餵 `k_j` 個 step
（`systolic_accel_context.sv`：`job_k_reg`、`job_k_slab_reg`、`k_dim`）。最後一次
invocation 不補到 K_MAX；補零只存在於 slab 的儲存格式（每 8 step 一個 window，最後一個
window 補零）。舊的 padded 模式（`job.k = K_MAX`、`k_execution = padded`）已移除。

## 1. 公式（R x C 陣列，operand 深度 K_MAX）

```
一個 K-step reduction：
I          = ceil(K / K_MAX)                           invocation 數
k_j        = min(K_MAX, K - j*K_MAX),  j = 0..I-1      第 j 次的 job.k = 實際執行的 step 數

一次 invocation（深度 k）：
W_read(k)  = 2 * L * ceil(k / w) * w * 4 bytes         L = operand_lanes = 8，w = k_window = 8
           = (R + C) * ceil(k / w) * w * 4             沒宣告 operand_lanes 時
W_write    = R * C * 4 bytes                            整塊結果 tile
T_read     = ceil(W_read  / beta_read)  + c_read
T_compute  = k + R + C - 2 + H
T_write    = ceil(W_write / beta_write) + c_write
T_seq(k)   = T_read + T_compute + T_write

一個 spatial tile：
T_tile     = sum_j T_seq(k_j)                            同一陣列上沒有 overlap（第 3 節）
compute-only: sum_j T_compute(k_j) = K + I*(R + C - 2 + H)
```

FPGA fleet 的數值：`T_read = 128 * ceil(k/8) + 11`；`T_compute = k + 14 + 95`（8x8）、
`k + 6 + 95`（4x4）；`T_write = 91`（8x8）、`31`（4x4）。例：K_MAX = 32、K = 65 在 8x8
上是 32 + 32 + 1 三次：755 + 755 + 340 = 1850 cycles。

沒有宣告任何 transport 屬性的 device：`T_read = T_write = 0`，
`T_tile = K + I*(R + C - 2 + H)`——改動前每個 pass 用的 compute-only 模型，逐 cycle 相同。
`estimateMatmulCycles(m, n, k, cfg)` 仍是 compute-only（folds x sum_j T_compute）：
`est_cycles` 在所有 pass 裡的意思沒變。

Variable-K 的推論：K_MAX 變深**永遠不會增加 cycle**——invocation 變少、最後一個
window 的補零變少。所以 makespan 對 K_MAX_8X8、K_MAX_4X4 都是非遞增的（DSE 在整個網格上
檢查過，`eval/kmax_dse`）；限制 K_MAX 的是 block RAM、timing 與 host 每次 invocation
的傳輸量。

## 2. 參數與來源（FPGA fleet：tile_pipelined、systolic_dma_top、v1 operand writer）

| 參數 | 值 | 來源 | 狀態 |
|---|---|---|---|
| H | 95 | fold RTL（tag paper-hw-v1）板測；本 RTL 的 Verilator 模擬中 fold_start -> c_valid = k + R + C - 2 + 95，validate.py 的 99 個單一 invocation 與 53 個同時執行的 invocation 全部成立 | 板測於前一版硬體；本 RTL 為模擬 |
| beta_read | 4.0 B/cycle（1 word/cycle） | 校準集擬合（第 4 節）；結構上是 v1 operand writer 每個 128-bit beat 拆 4 拍 | 模擬校準、模擬驗證；**未板測** |
| c_read | 11 cycles | 校準集擬合：descriptor 3 + 首 beat 延遲 6 + fold release 2 | 同上 |
| beta_write | 3.2 B/cycle（0.8 word/cycle） | 校準集擬合（兩個幾何）；結構上是陣列每 cycle 送出 1 word + write engine 每 cycle 1 beat（1.25 cycle/word） | 同上 |
| c_write | 11 cycles | 校準集擬合 | 同上 |
| operand_lanes | 8 | slab 格式（`runtime/fpga_matmul_tiled.cpp`）；4x4 也搬 8 lane、留 4 lane | RTL/runtime 原始碼 |
| k_window | 8 | slab 以 8 step 為一個 window；模擬中每個 job 的 `words = 128 * ceil(k/8)` | RTL/runtime 原始碼 + 模擬 |
| 每次 invocation 的深度 | k_j（variable-K） | runtime：`job.k = K`（該次 invocation 的 K）；RTL：`k_dim = job_k_reg`；模擬中每個 PE 的 MAC 次數 = job.k | runtime/RTL 原始碼 + 模擬直接計數 |
| shared read（四顆共用） | 3.51 words/cycle（參考值） | `eval/kmax_sweep/README.md` 的 DDR3 供給（3.51 / 3.627 兩值取低者）；`systolic-matmul` 與 `--systolic-fpga-pipeline` 預設交給 binary search | **非本 bitstream 實測**；可用 `--shared-read-words-per-cycle` 改或設 0 |

### 論文的 beta（3.627 / 3.51 words/cycle）

- 論文 0.8：「a DMA path attached, operand supply is 3.63 words per array cycle and
  effective fill 1.00」——3.63（=3.627，128-bit AXI4、1 MiB 循序讀、4.00 上限的 90.7%）
  是 **DDR3 的供給**；單次 fill 被 operand buffer 的 32-bit 寫入埠限制在 **1.00
  word/cycle**。本 RTL 的模擬校準值 beta_read = 1.0 word/cycle 與論文的 effective fill
  一致。
- 3.51：論文 0.8 正文裡找不到；`eval/kmax_sweep/README.md` 標為「measured, N=8 DMA
  path, DDR3 supply」，repo 裡沒有原始量測紀錄。
- 適用性：兩者都不是單次 invocation 的 fill 速率，而是 **四顆陣列共用一個 AXI read port
  時全部 fill 加總的上限**，所以只出現在 binary search 的 shared read bandwidth 裡。
  本 RTL／bitstream 上都沒有量過，不宣稱是實測值。模擬的記憶體模型供給 4.0 words/cycle
  （每 cycle 一個 128-bit beat）。

## 3. Overlap 與共用資源：RTL 實際有的

- **同一顆加速器：封存的 bitstream 沒有 overlap。** 它的 context 只有在
  `!ctx_job_busy && !result_readback_busy` 時才接下一個 job（`systolic_dma_top.sv`，
  `core_job_ready`），每次 invocation 是一個獨立的 job。所以 `T_overlap = 0`、
  `T_tile = sum T_seq`。2026-10-18 之後的 RTL 把連續 invocation pipeline 起來（第 8 節）；
  device 宣告 `pipeline_depth` 時模型用那一節的 timeline，否則用這裡的序列模型。
- **同一顆的兩次 invocation 之間：result readback。** 上一次的結果從 DDR 讀回 DPTI 輸出
  路徑（done -> rb）做完之前，top 不收這個 context 的下一個 descriptor。模擬（host 永遠
  可收）：8x8 **212～214 cycles**、4x4 **11 cycles**。物理上：結果經 `dpti_output_cdc`
  （`DEPTH = 8` 個 128-bit beat）送到 60 MHz 的 byte 介面；4x4 的 tile 是 4 beats，整塊
  放得進緩衝；8x8 是 16 beats，多出的 8 beats = 128 B 要等 byte 介面送出，
  128 x 100/60 ≈ 213 cycles。這是 DPTI/host 端的時間（host 慢時更長），**不在 T_seq
  裡**；DSE 用 `shared351_rb`（每次 invocation 8x8 +213、4x4 +11）當 what-if。
- **不同加速器：同時執行**（binary search 的排程就是這個）。共用：
  - read engine（`dma_engine_multi`：`BURST_LEN = 16`、`SHARED_BURST_LEN = 1`（v1）、
    `MAX_OUTSTANDING = 8`）。一個 slot 獨佔時發 16-beat burst；v1 writer 每 beat 要 4
    cycles，所以一個 burst 要約 64 cycles 才被消化。**Head-of-line**：一個 fill 開始時，
    另一個 slot 已經發出、還在 return queue 裡的 burst 要先消化完。等待 ≈ 已發出未消化的
    beats x 4 cycles，一次最多約 8 x 16 x 4 = 512 cycles。模擬量到 +75～+466 cycles
    （同時送出時約 190；K = 32、第二個晚 40 cycles 送出時 466——與第一個的整個 slab
    （8 個 burst）都已發出的情況一致，沒有逐波形確認）。這個**不在模型裡**；DSE 用
    `shared351_hol190` 當 what-if。
  - write-back engine：同時寫回時 +4／+8／+16 cycles（模擬）。
  - 頻寬：binary search 的 `shared-read-bytes-per-cycle` 要求「全部 fill 的總 bytes <=
    頻寬 x T」（必要條件）；`systolic-matmul` 與 `--systolic-fpga-pipeline` 預設 3.51
    words/cycle（第 2 節）。模擬的記憶體每 cycle 一個 beat，這個上限在模擬裡不會綁住；
    板上的 DDR3 供給比它低。
- **目前的 runtime 一次只發一個 invocation**：`fpga_matmul_tiled_auto_scheduled` 是同步的
  （MEM_WRITE slab、等 1 ms、送 descriptor、讀回結果、host `C += v` 之後才 return），
  所以板上今天任兩顆陣列都不會同時執行。排程裡的裝置平行是 RTL 支援的行為
  （`tb_cost_concurrent.sv` 驗證過），要在板上兌現需要 runtime 改成多個 invocation 在途。

## 4. 校準與驗證（Verilator，`eval/cost_model_validation/validate.py`）

**相位邊界**（100 MHz array clock，FPGA 內部）：read = job_fire -> fold_start，
compute = fold_start -> c_valid_out，write = c_valid_out -> job_done（結果在 DDR）。
DPTI（done -> rb、slab 的 MEM_WRITE、host 讀回）與 host wall-clock **不和 cycle 比較**，
另外列。

**直接量測的 reduction steps**（`rtl/multi_200t/tile_pipelined/dma/tb/reduction_probe.svh`，
只讀不驅動）：每個 PE 的 `pipe_pair_valid` 次數（實際做的 multiply-accumulate）、feeder
送進 row 0 / column 0 的 operand 數、ST_FEED 的 cycle 數、fold 開始時的 `k_dim`。bench
對每個 job 要求「每個 PE 的 MAC 次數 = job.k」，否則 FAIL。

- **校準集**：K_MAX 24/12，13 個單一 invocation（descriptor 深度 1..K_MAX，四顆
  device；其中兩個是 padded descriptor，真實資料後面補零）。五個參數只從這組擬合；殘差 0。
- **Held-out 集**（從未用於擬合），每個 case 的 job 都是 compiler IR 的
  `systolic.matmul_tile`（k、start_cycle 照 IR）：
  - K_MAX 32/32：K = 1、7、16、31、32、33、47、64、65，8x8 與 4x4 各一組；
  - K_MAX 16/7、17/24、256/64：K = 1、K_MAX - 1、K_MAX、K_MAX + 1（每個幾何用自己的 K_MAX）；
  - K_MAX 512/512（DSE 的建議值，`docs/KMAX_DSE.md`）：同上，再加 K = 1025；
  - K_MAX 1/1：K = 1、2、3。
  共 58 個 case、86 個 invocation（22 個 case 是 K-folding：2～3 次 invocation）。
- 結果：
  - 每個 PE 實際執行的 reduction steps **全部 = job.k**（例：K_MAX 32、K = 65 是
    32 + 32 + 1；K_MAX 256、K = 255 是 255，不是 256）；讀入 words 全部 = 模型；
  - read／compute／write／total 的 MAE、最大相對誤差：校準集與 held-out 集分開算，
    **全部 0 cycle**；
  - 每個 invocation 的結果、與每個 K-folded GEMM 的部分和累加（bench 照 host 的
    `C += v` 做）全部對整數 reference **exact**。
  表格：`eval/cost_model_validation/out/summary.md`。
- 這個 0 的意思：模擬的 RTL 是確定性的，記憶體模型延遲固定；模型的結構與 RTL 的單一
  job 行為一致。**真實 DDR3（refresh、row miss、MIG 延遲）不在模擬裡**，板上的 c_read
  與 beta_read 可能不同——FPGA Board Regression：NOT RUN。
- K-folding 的時間軸：模型把同一 tile 的 invocation 接在一起；模擬中下一個 invocation
  在上一個 readback 後才被接受，所以每個間隔多 8x8 212～214、4x4 11 cycles（第 3 節）。
- **整條 pipeline（co-simulation）**：`systolic-matmul` -> runtime -> DPTI -> 工作樹 RTL，
  K_MAX 16/16、32/32、17/7、24/12、1/1、256/64、512/512，每組 36 個標準 case 加 K_MAX 邊界
  case（K_MAX 32/32 另加 K = 7、16、47、64、65）：320 個 case、10,691 次 invocation
  （1,372 次 job.k < K_MAX）。`test/e2e/check_variable_k.py` 逐 invocation 比對：compiler
  IR 的 k = runtime 送的 job.k = RTL 收到的 k = 每個 PE 實際的 step 數，**全部一致**；
  每次 invocation 的 FPGA 內部 cycle（job 被接受 -> 結果在 DDR）= 模型的 read + compute +
  write，**10,691 次全部 0 誤差**（runtime 一次只發一個 invocation，沒有競爭）。

## 5. 同時執行（`tb_cost_concurrent.sv`，模型的已知誤差）

| K_MAX | 波次（job.k d0/d1/d2/d3，送出間隔） | 模擬 makespan | 模型（各自獨立 fill） | 誤差 |
|---|---|---|---|---|
| 16/16 | 16/16/16/16，0 | 622 | 483 | +139（+22%） |
| 16/16 | 0/16/16/16，0 | 613 | 421 | +192（+31%） |
| 16/16 | 0/16/16/16，300（fill 不重疊） | 1021 | 1021 | 0 |
| 32/32 | 32/32/32/32，0 | 894 | 755 | +139（+16%） |
| 32/32 | 32/32/32/32，40 | 1214 | 816 | +398（+33%） |
| 32/32 | 1/1/1/1，0 | 415 | 340 | +75（+18%） |
| 32/32 | 31/7/16/32，0 | 886 | 754 | +132（+15%） |
| 256/64 | 256/64/64/64，0 | 4563 | 4563 | 0（4x4 的延遲被 8x8 蓋住） |
| 256/64 | 37/63/1/64，0 | 1430 | 1240 | +190（+13%） |

compute 與每個 PE 的 step 數在每個波次都 exact；誤差全在 read（head-of-line）與
write-back（+4～+16）。完整表格：`eval/cost_model_validation/out/summary.md`。

## 6. Binary search 與 IR

- tile 成本 = `estimateTileCost(K, cfg).sequential()`（variable-K）；同一幾何的所有加速器
  必須定價相同。
- feasibility：每個幾何 `count x floor(T / cost)`；有 shared read bandwidth 時另外要求
  `sum read bytes <= B x T`。四顆陣列同時執行（`1 x 8x8 + 3 x 4x4` 異質 fleet）。
- 找第一個未覆蓋 block 改成 cursor + gcd block grid：6000 組隨機案例在舊版成功的情況下
  分解、排程、makespan 全部相同；512x512 從 19.3 s 變 0.004 s。舊版在幾何不整除時上界
  可能不可行而整個失敗，新版在那種情況把上界加倍再搜尋。
- shared read 在 production：`systolic-matmul --shared-read-words-per-cycle=X`（預設
  3.51）、`--systolic-fpga-pipeline="shared-read-words-per-cycle=X"`（預設 3.51）；0 =
  各自獨立 fill。`test/Systolic/fpga_pipeline_shared_read.mlir`：32x40x256（K_MAX 256/256）
  在獨立 fill 時是 11 個 8x8 + 36 個 4x4 tile（makespan 53940，但 770048 B 在 14.04
  B/cycle 要 54847 cycles），有 shared 上限時改成 12 + 32（makespan 54756）。
- IR：`est_cycles` = 該次 invocation 的 compute term（所有 pass 與硬體
  `req_compute_cycles` 的既有意義）；`start_cycle` = 同一顆加速器上前面 invocation 的
  compute term 加總，用每次 invocation 自己的 k（例：K_MAX 32、K = 65 在 8x8：
  start 0 / 141 / 282，est 141 / 141 / 110）。不平均切分，也沒有 `start + j * est`。
  pass 會檢查每個 invocation 的 read + compute + write 加總等於 optimizer 排給這個 tile
  的區間，以及 IR 的 k 等於被定價的 k。含 read/write 的 makespan 在 `report=true` 的
  remark 與 `systolic-cost` 裡。
- 硬體 scheduler 的 `cycle_counter` 從 job 被接受（含 fill）跑到 c_valid；一次一個
  job 發出時，compiler 的 start_cycle 與「含 read+compute+write 的時間軸」的
  start_cycle 都**沒有**讓任何 fold 被 hold（K_MAX 16/16 的 55 個與 17/7 的 84 個
  invocation，`eval/cost_model_validation/start_cycle_check.py`，全部 exact）。

## 7. 重現

```bash
# LLVM/MLIR 23 與 Verilator 在 PATH 上
cmake --build build -j2 && cmake --build build --target check-systolic
build/bin/systolic-cost invocation --geometry 8 --k-max 32 --k 65
build/bin/systolic-cost schedule --m 64 --n 64 --k 64 --shared-read-words-per-cycle 3.51 --tiles
build/bin/systolic-cost check-monotone --m 100 --n 36 --k 147 --k8 24 --k4 12
python3 eval/cost_model_validation/validate.py --jobs 2
python3 eval/cost_model_validation/start_cycle_check.py
```

## 8. 同一顆加速器上連續 invocation 的 pipeline（2026-10-18）

**問題。** PE（`core/systolic_pe.sv`）有兩組 accumulator bank 與兩顆加法器，設計目的是讓
tile T+1 的累加（feed）與 tile T 的 bank 歸約（H = 95 裡的 67 + 2 cycles）重疊；PE 只要求
兩個 tile 的運算元之間有 MUL + ADD + 2 拍的安靜間隔（fp_model 9/12：23；IP 8/11：21，
`tb_pe_overlap.sv`）。但上層把這段重疊全部序列化掉：context 的 compute FSM 在
`ST_WAIT_RESULT` 等 `c_valid_out`（歸約結束之後）才能收下一次 fold；phase FSM 的
`tile_handoff` 也等 `c_done_fold`；硬體 scheduler 的 slot 在 `ST_RUN` 等 `accelerator_done`
（= `c_valid_out`）才收下一個 request；adapter 在 `ST_RUN` 期間丟掉新的 start；top 的
`core_job_ready` 要 `!ctx_job_busy`（上一個 job 寫回完成）且 `!result_readback_busy`。所以同一
顆加速器上每一次 invocation 都是 read → compute → reduce → write → readback 之後才開始下一個
（第 3 節「同一顆加速器：沒有 overlap」描述的就是這個狀態）。

**修正（RTL，`USE_EXTERNAL_SCHEDULER` 路徑）。** `systolic_accel_context.sv` 改成
`JOB_QUEUE_DEPTH = 4` 的 job queue 與三個以計數器銜接的 stage（fill / compute / write-back）：
下一個 job 的 operand fill 在前一個 job 的 fold 開始時就發出（既有的「最多一個填好的 tile 在等」
規則，保證 ping-pong operand buffer 不會被覆寫）；fold 的放行條件是運算元已落地、陣列輸入已
安靜 `FEED_GAP = PE_QUIET_GAP(23) + FEED_GAP_MARGIN(3) = 26` 拍、tile j-2 已發佈（每個 PE 的兩組
accumulator set：PE 沒有 back-pressure，太早餵會把兩個 tile 加在一起）、result bank j%2 已寫回；
陣列依序發佈，結果 n_cv 進 bank n_cv%2；write-back 依序寫回各 job 自己的 c_base。
`systolic_hw_scheduler.sv` 的 slot 在送出 start 之後就釋放（outstanding counter 維持 busy 與
schedule_done）；adapter 轉送每個 start、計數完成；top 的 acceptance 改成「queue 有空位且尚未讀回
的 job 少於 RB_DEPTH = 8」，result readback 每個 context 一個 FIFO，讀回的 slot 在 context 之間
round-robin（固定優先權會讓一條長鏈的 8x8 餓死其他 context 的讀回）。legacy bring-up 路徑
（`USE_EXTERNAL_SCHEDULER = 0`）逐字保留。單一 invocation 的時序不變：fire → fold = T_read、
fold → cv = T_compute、cv → done = T_write，與第 1、2 節完全相同（`tb_cost_phases` 逐 cycle 相同）。

**模型（`CostModel.h` `AcceleratorTimeline`）。** device 多三個屬性（三個一起宣告，缺省 = 序列化
硬體，所有既有 fixture 不變）：`pipeline_depth`（4）、`reduce_cycles`（69 = 67 + 2，連續兩次
發佈的最小間隔，`tb_pipeline_overlap` 4x4 k=1 鏈量到 69）、`feed_gap`（26）。同一顆加速器上
第 j 次 invocation 的事件：

```
accept_j     = max(accept_j-1, writeEnd_j-D + 2)          （2 = 計數器更新 + registered accept）
fillStart_j  = max(accept_j, fillEnd_j-1, feedStart_j-1)
fillEnd_j    = fillStart_j + T_read(k_j)                 （c_read 含放行）
feedStart_j  = max(fillEnd_j, feedStart_j-1 + k_j-1 + feed_gap,
                   computeEnd_j-2 + 3, writeEnd_j-2 + 3)  （3 = ready → start → fold）
computeEnd_j = max(feedStart_j + T_compute(k_j), computeEnd_j-1 + reduce_cycles)
writeEnd_j   = max(computeEnd_j, writeEnd_j-1) + T_write
```

D = 1 時逐 cycle 等於第 1 節的序列模型。binary search 的 capacity 改成「該幾何的 tile stream
在 T 之前完成幾個」（序列化時 = floor(T / cost)），tile 的區間取自同一條 timeline，
`SystolicTilePlan` 的 `start_cycle` 是 transfers-free 的 `feedStart_j`（序列化時仍是前面
compute term 的加總），`systolic-cost --verify` 重算 stream 比對區間。`systolic-cost --pipelined`
/ `timeline` 印出事件；`systolic-matmul` 從 `fpga_config.txt` 的 `PIPELINE_DEPTH` 決定宣告哪一種
fleet（封存的 bitstream 沒有這一行 → 序列化）。

**驗證（`eval/pipeline_overlap/run.sh`）。** `tb_pipeline_overlap.sv` 把 K-fold 鏈以硬體願意接受的
速度送進去，逐 job 蓋章（fire / grant / fill / fold / feed / cv / wbacc / done / rb），每塊結果
tile 與鏈的總和逐元素精確、每個 PE 每 job 的 MAC 數 = k、每個 fold 恰一次 `c_valid_out`、PE(0,0)
看到的 tile 間安靜間隔 ≥ 21，並要求量到的重疊對數不低於這份 RTL 做到的。`compare_timeline.py`
用觀測到的 accept 餵模型，要求 fold / cv / done 逐 cycle 相同；唯一允許的差異是共用 read / write
engine 等待其他 context 或 readback（第 3 節已知、模型沒有的效應），而且只在 job 本身仍顯示
「落地即 fold」「grant 即寫」的特徵時才算；一次一個 job 的 serial run 不允許任何等待。單一裝置
4 組（8x8/4x4 × v1/v2，各 14 個 invocation）、混合 2 組、各自的 serial run：0 mismatch。
`jobs/gapbound.svh`（4x4、K_MAX 32、v2、k = 32,1 交替）讓安靜間隔規則成為放行條件：PE(0,0)
的間隔恰為 FEED_GAP = 26；同一份 list 以 `PE_QUIET_GAP=8` 建出來的 context 把兩個 tile 加在一起
（8 個 fold 只發佈 7 次）、bench FAIL——這是邊界檢查有效的負對照。

**量化（同一支 bench，HEAD d854c40 的 RTL vs 修正後，K_MAX 16/16，cycles）：**

| 案例 | 修正前 makespan（first fire → last done） | 修正後 | fold_j+1 < cv_j 的對數 |
|---|---|---|---|
| 8x8，K=64 四次 k=16 鏈，v1 | 3073 | 1645 | 0（v1 fill-bound：T_read 267 > T_compute 125） |
| 8x8，K=64 四次 k=16 鏈，v2 | 2669 | 954 | 1（fold 1 於 371 < cv 0 於 421） |
| 8x8，14 invocation（4 條鏈），v1 / v2 | 11656 / 11202 | 5653 / 6928 | 0 / 4 |
| 4x4，14 invocation（4 條鏈），v1 / v2 | 5225 / 3017 | 3403 / 1885 | 0 / 7 |

v1（封存 bitstream 的 operand path，1 word/cycle）上 fill 是最長的 stage，fold_j+1 永遠在
cv_j 之後：PE 層的累加／歸約重疊在 v1 上碰不到，省下的是 cv → write-back → readback → accept →
fill 這一串序列尾巴；v2（beat-wide）上 fill 75 < compute 125，下一個 fold 在前一個發佈之前
50 拍開始，PE 的 ping-pong 真正被用到。14-invocation 的數字含 host 永遠就緒時的 DPTI 讀回
（8x8 一塊約 850 拍）對 acceptance 的節流（RB_DEPTH = 8）與共用 read engine 的等待，所以
比 4 次鏈的比例小；兩邊的 bench、job list 與 RTL 以外的環境完全相同。

**仍未解決（量到的）。** (1) 同步 runtime 仍是一次一個 invocation；非同步 runtime
（`docs/ASYNC.md` 第 9 節，2026-10-18）每顆陣列最多 `PIPELINE_DEPTH` 個 descriptor 在飛、每個在途
invocation 自己的 slab／result 區域，co-sim 上 context 的 queue 確實被填到 4，但 DPTI 鏈路本身
（一筆 84 byte 的 descriptor 約 420 拍、一塊 1 KiB slab 約 5000 拍，對上 k = 16 的 tile 約 125 拍
的 compute）讓同一顆陣列上連續 invocation 的 fold 幾乎不可能在前一個 publish 之前開始；主機路徑能
兌現的是跨陣列的並行與「不必每筆等結果」，PE 層的重疊要由本節的 bench（internal job port）證明；
(2) DPTI 結果讀回在
模擬裡 8x8 一塊約 850 拍（4x4 約 213），是 host 永遠就緒時的物理瓶頸，且讀回的 beat 走同一個 AXI
read engine，會把同時進行的 fill 擋住數百拍（`compare_timeline.py` 標成 fill-stalled；mitigation
是把 `dpti_output_cdc` 的 FIFO 加深到能放一整個 burst 並以整個 burst 的空間 gate 讀回）；(3) 多顆
同時 fill / 寫回時共用 engine 的等待仍在模型之外（第 3、5 節）；(4) `cyc_latched` 只對沒有重疊
的 transaction 有意義。
