# 非同步多 GEMM：host runtime、DPTI 協定、RTL 與 compiler 的改動

這份文件記錄 `experiment/batch-gemm-parallelism` 上為了讓多個 GEMM 的 tile 同時在四顆
陣列上飛、host 不再每個 job 同步寫／讀所做的改動：協定與 RTL（第 2 節）、非同步 runtime
（第 3 節）、compiler 的 `--async` lowering（第 4 節）、測試（第 5 節）與板上要做的量測
（第 6 節）。設計依據是提案《systolic-mlir 非同步多 GEMM 架構提案》；每一節末尾註明哪些
是在沙盒／co-simulation 實跑過的、哪些要等板子。

**一句話：** host 以 `job_id` 認結果，GEMM 的歸屬由 host 的 GEMM Group 管（設計 A）；
硬體只多了「每個結果前面一筆 16 byte 的 completion record、一個管 FT2232H 匯流排方向的
arbiter、一個管 DDR 寫入埠的 arbiter、被拒絕的 descriptor 也回一筆 record」四件事，
`gemm_id` 不進 descriptor，不同 GEMM 的 tile 在硬體眼裡沒有差別。

## 1. 為什麼是設計 A（gemm_id 只在 host）

| | A：host-managed GEMM Group，硬體只認 job_id | B：gemm_id 寫進 descriptor |
|---|---|---|
| 硬體要改什麼 | completion record 帶 job_id（原本結果裸送、靠順序對應） | 同左，外加 descriptor 多一個欄位、scheduler 認 gemm_id |
| 硬體拒絕的理由 | 只有 k 超過 K_MAX、device 不存在 | 同左；若 scheduler 依 gemm_id 分流，就多出「這個 gemm 沒空間」這種拒絕 |
| host 的簿記 | Pending Job Table：job_id → (gemm, tile, k_index, C, …) | 相同（硬體回的 gemm_id 只是重複 host 已知的事） |
| 跨 GEMM 交錯 | runtime 決定送的順序，硬體看不出差別 | 硬體看得出，但沒有用處：每顆陣列一次只做一個 job |
| 風險 | 無新增硬體狀態 | descriptor 格式改變，舊 runtime／舊 bitstream 互不相容 |

「一顆陣列一次一個 job」這個 baseline 下，硬體拿到 gemm_id 做不了任何 host 做不了的
事，所以選 A。提案要求「不同 gemm_id 不得成為 FPGA 拒絕的理由」在 A 下自動成立。

## 2. 協定與 RTL（提案第 2 階段）

### 2.1 Completion record

bitstream 送回 host 的每個結果前面多一筆 16 byte 的 record（`COMPLETION_HEADER=1`）：

| byte | 欄位 | 內容 |
|---|---|---|
| 0..3 | magic | `0x5953_4A43`（little-endian；wire 上是 `"CJSY"`） |
| 4..7 | job_id | descriptor 的 `JOB_ID` 暫存器 |
| 8 | device_id | descriptor 的 `DEVICE_ID` |
| 9 | status | 0 = OK，後面跟 payload；1 = k 超出 1..K_MAX；2 = 沒有這個 device |
| 10..11 | payload_bytes | 跟在後面的 byte 數：OK 時是 P×P×4（4x4 = 64，8x8 = 256），拒絕時 0 |
| 12..15 | seq | 這顆 bitstream 自 reset 起發出的第幾筆 record（VIO `rb_seq`） |

host 端的對應：`runtime/fpga_matmul_tiled.h` 的 `fpga_completion_t`、
`FPGA_COMPLETION_MAGIC`、`fpga_runtime_read_completion()`。同步 runtime
（`fpga_matmul_tiled_auto_scheduled`）在設定檔說 `COMPLETION_HEADER=1` 時先讀這筆
record，job_id／device／status／payload 任何一項對不上就回 -9 並停用這條 session；
設定檔沒寫（舊 bitstream）就照舊讀裸 tile。所以**舊 bitstream + 新 runtime、新 bitstream +
同步 runtime 都能用**；只有非同步 runtime 一定要新 bitstream。

### 2.2 `dpti_bus_arbiter.sv`：FT2232H 匯流排一次一個方向

原本 `dpti_byte_rx` 與 `dpti_byte_tx` 各自直接驅動腳位：RXF# 低就讀、有東西就寫，
兩邊同時發生時 OE#／RD# 與 WR#／D[7:0] 打架（AN_130：「only a read or a write cycle
can be performed at any one time」）。同步 runtime 從不在結果還沒回來時寫，所以沒踩到；
非同步 host 必踩。`rtl/multi_200t/tile_pipelined/dma/dpti_bus_arbiter.sv` 接管兩個方向：

- 狀態 IDLE／RX_TURN／RX／TX；RX_TURN 是 AN_130 要求的「OE# 先低至少一個 CLKOUT 再拉
  RD#」；RX 與 TX 之間一定經過 OE# 高的 IDLE 週期，FT2232H 放開 D 之後 FPGA 才驅動。
- 兩邊都要匯流排時輪流，每段最多 `CHUNK_BYTES`（64）byte；兩邊同時等時先給 tx（結果
  小、而且 host 要再送東西的 credit 都靠它回來）。
- byte 介面與原本的 `dpti_byte_rx`／`dpti_byte_tx` 完全相同（RD# 一個 cycle 的脈衝、下
  一個 edge 取樣、`byte_valid` 握手），`dpti_command_frontend` 與 `dpti_beat_to_byte`
  不用改；讀取節奏也維持每 3 個 CLKOUT 一個 byte，板子已驗證過的速率不變。

### 2.3 DDR 寫入埠的 grant arbiter

`systolic_dma_top.sv` 原本用 `hs_busy` 決定 AXI write 通道是給 host slab writer 還是
write-back engine，burst 進行中切換會把對方的 beat 寫進自己的位址（`tb_dpti_direct.sv`
的 `collision` 測試在舊 RTL 上重現：`mem_errors`、`err_w_owner`、結果遺失）。現在是
grant 制：`w_grant ∈ {NONE, HS, WB, SD}`，持有者發完 AW 與 W、等到所有 B 回來
（`w_open == 0`）且不再要求時才釋放；優先序 HS > WB > seed writer。`err_w_owner` 變成
「沒有 grant 卻發生握手」的斷言。

### 2.4 被拒絕的 descriptor 回一筆 record

舊 RTL 對 k 超過 K_MAX 的 descriptor 只拉 `err_job_k`，descriptor bridge 卡在那筆上
（head-of-line），之後整條鏈路死掉（`refused` 測試在舊 RTL 上重現）。現在
`refuse_fire` 把它從 bridge 拿走、`ref_pending` 排一筆 status 1／2、payload 0 的 record，
鏈路保持同步；host 看到這筆就知道那個 job 沒跑。device 不存在也一樣（status 2），不再
`$fatal`。

### 2.5 其他

- VIO 多了 `probe_in20..24`：`overlap2_cycles`／`overlap4_cycles`（同時有 ≥2／4 顆
  context 在 fold 的 cycle 數）、`idle_gap_cycles[0]`／`[1]`（context 0、1 兩個 job 之間
  空轉的 cycle）、`rb_seq`；`dma_top_build.tcl` 的 readout 會印出來，是板上「有沒有真的
  重疊」的直接證據。
- `dma_top_build.tcl` 的 source list 加了 `dpti_bus_arbiter.sv`，`fpga_config.txt` 寫
  `COMPLETION_HEADER=1`；`test/e2e/cosim/build.sh` 也寫（working tree 裡有
  `dpti_bus_arbiter.sv` 就是 1）。新 bitstream 的 K_MAX 用 16／16。
- 沒做：RESET 暫存器與 frontend timeout（提案列為選配）。鏈路失步時的恢復手段仍是重開
  session（`ftdi_usb_purge_buffers`）；非同步 runtime 用 job_id 的 epoch 位元（31..24）
  辨認上一個 session 遲到的 record 並丟掉。
- 沒動：XDC、SIWU#（要另外批准）、`eval/`、`artifacts/`。

### 2.6 沙盒實跑結果（Verilator 5.020／Icarus 12；不是板子）

| 測試 | 指令 | 結果 |
|---|---|---|
| 六個 regression gate | `rtl/multi_200t/tile_pipelined/dma/tb/run_top_sim.sh` v1 16/32 × 1/2 invocation、hetero_concurrent、hetero_full_concurrent | 6/6 PASS（82–112 s 各） |
| arbiter 單元 bench | `iverilog -g2012 -I ../../../../../test/e2e/cosim -o /tmp/tb_arb tb_dpti_bus_arbiter.sv ../dpti_bus_arbiter.sv && vvp /tmp/tb_arb` | PASS：4 種流量混合各 20 000 byte 雙向、0 次 strobe 違規、雙向都沒餓死 |
| 直連 bench（新 RTL） | `COMPLETION_HEADER=1 ./run_dpti_direct.sh all` | basic／multi／refused／collision（+w_stall=8）／direction 全 PASS |
| 直連 bench（舊 RTL 6ebdc6a） | 同上 | direction 有 strobe 違規、collision 有 mem_errors／err_w_owner、refused 鏈路卡死——提案列的四個 hazard 都重現 |
| `hol` | `COMPLETION_HEADER=1 ./run_dpti_direct.sh hol` | 當時（2026-10-14 的 RTL）：兩筆 descriptor 送同一顆 device → STALL（descriptor bridge head-of-line；host 不讀結果時 TX buffer 滿）；host 一讀就恢復——這是當時「一顆 device 一個 outstanding descriptor」credit 規則的依據。整合 pipelined context 之後（第 9 節）：24 筆不讀結果也 `DRAINED`，credit 改為 `PIPELINE_DEPTH` |
| runtime-dispatch-test 對 cosim | `SYSTOLIC_FPGA_CONFIG=build/cosim/fpga_config.txt`（COMPLETION_HEADER=1） | 14 invocation 全 PASS，15 638 B 送出、2 080 B 收回（14×16 byte record + 5×256 + 9×64）；`SYSTOLIC_DPTI_SETTLE_US=0` 也 PASS |

協定檢查器 `test/e2e/cosim/dpti_protocol_check.svh` 掛在 `tb_dpti_cosim.sv`、
`tb_dpti_direct.sv` 與 `tb_dpti_bus_arbiter.sv` 上：RD# 必須在 RXF# 低時、WR# 在 TXE#
低時、OE# 低時 FPGA 不得驅動 D、OE# 高後至少一個 cycle 才能 WR#；違規計數
`dpc_violations`，`+dpti_check_fatal=1` 時直接 `$fatal`。

## 3. 非同步 runtime（提案第 3 階段）：`runtime/fpga_async_runtime.{h,cpp}`

### 3.1 API

```c
fpga_gemm_t *fpga_gemm_begin(int M, int N, int K, unsigned tiles_hint);
int fpga_tile_submit(fpga_gemm_t *g, unsigned tile_id, unsigned k_index, unsigned k_count,
                     int m, int k, int n, const float *A, const float *B, float *C, int ldc,
                     unsigned device_id, unsigned start_cycle, unsigned est_cycles);
int fpga_gemm_wait(fpga_gemm_t *g);
```

- `submit` 當場把 A、B 打包成 slab（所以 compiler 的 A／B scratch tile 可以照舊重複使用）
  並排進該 device 的佇列，然後「pump」鏈路：能寫就寫、該讀才讀，回來。
- `wait` pump 到這個 GEMM 的每個 tile 都累加進 C 或失敗為止，釋放 handle，回傳第一個
  錯誤碼。C 在 wait 回來前必須有效。
- 語意與同步版一致：`C[i*ldc+j] += (A×B)[i][j]`；同一個 `tile_id` 的 fold 依 `k_index`
  順序累加，不管結果什麼順序回來（早到的先存著），所以折 K 的 GEMM 與同步路徑**逐位元
  相同**（mode X 實測）。

### 3.2 Credit、DDR slot、Pending Job Table、pump

| 東西 | 內容 |
|---|---|
| credit | 每顆 device：descriptor `depth` + stage 1，`depth` = 設定檔的 `PIPELINE_DEPTH`（context 的 job queue，本樹的 RTL 是 4；沒有這個鍵的舊 bitstream 是 1；`SYSTOLIC_ASYNC_DEPTH` 可往下調，不會高於 `PIPELINE_DEPTH`，上限 8）。預設的 burst 模式（`SYSTOLIC_ASYNC_BURST=1`）先把一顆 device 的 operand slot 全部填滿（`depth + 1` 塊 slab）或等到 `fpga_gemm_wait` 才送它的 descriptor，讓 context 連續收到 job（第 9 節）；`SYSTOLIC_ASYNC_STAGE=0` 時每顆 device 仍然一次一個 descriptor（mode A／B） |
| DDR slot | 每顆 device `depth + 1` 個 operand slot（`0x10000 + (dev*(depth+1)+slot)*slab_bytes`，256 B 對齊）與 `depth` 個 result slot（`0x3000 + (dev*depth+slot)*256`，16 B 對齊），以 free mask 配置；一個 job 的兩個 slot 在它的 completion record 讀到之後才釋放（context 在 fold 之前就讀完 slab，result tile 在 record 的 payload 之前就離開 DDR），所以 staged 的 slab 永遠不會覆蓋陣列還可能在讀的那塊、result slot 不會在 host 拿到之前被覆蓋；設定檔有 `DDR_BYTES`（co-sim 寫 1 MiB）時 depth 會降到 operand 區域放得下為止 |
| job_id | `epoch<<24 \| seq`：epoch 每開一次 session +1（1..255），seq 24 bit；上一個 session 遲到的 record 查不到 PJT 就丟掉 |
| PJT | `job_id → Tile*`（在飛的 tile）；completion 回來依 job_id 找，device／payload 對不上 = 失步（-9，整條 session 停用） |
| 送出順序 | 各 device 佇列 FIFO，多顆 device 都能 stage 時先給最早 submit 的 tile（全域序號）；沒有優先權反轉，不需要 aging |
| 讀的時機 | `SYSTOLIC_ASYNC_READ_POLICY`：`blocked`（預設）＝寫到沒東西能寫了才讀；`always`＝每次 pump 都讀一次；`wait`＝只在 `fpga_gemm_wait` 與佇列到上限時讀。板上一次空讀要付一個 latency timer 週期（FT2232H 沒資料時等 timer 到才回應 IN），cosim 看不出差別——板上量 |
| 佇列上限 | `SYSTOLIC_ASYNC_QUEUE_LIMIT`（預設 64 個打包好的 tile）：到上限 submit 才會停下來 pump 讀 |
| timeout | 有 descriptor 在飛卻 `SYSTOLIC_DPTI_TIMEOUT_MS`（預設 5000）內沒有任何 byte 回來 → -8 |
| 被拒絕的 job | record status ≠ 0：那個 tile 失敗（-18），同一輸出 tile 之後的 fold 也不累加（C 不會半套），其他 tile、其他 GEMM 照跑，鏈路不失步 |
| 鏈路失敗 | -3/-4/-6/-7/-8/-9：所有在飛、staged、排隊的 tile 一起失敗，之後 begin 回 NULL（-14），直到 `fpga_async_runtime_close()` |

單執行緒：沒有背景 thread，所有 I/O 都在 submit／wait 裡做。`SYSTOLIC_ASYNC_POLL_US`
（預設 50）是 cosim 上空讀之間的 sleep，板上可設 0。

### 3.3 錯誤碼

同步版的 -3..-16 意義不變；新增 -17（tile 與 GEMM 不一致：`k_index >= k_count`、同一
`tile_id` 的 `k_count` 不同、同一 fold 送兩次、wait 之後再 submit）、-18（bitstream 拒絕
這筆 descriptor）、-19（設定檔沒有 `COMPLETION_HEADER=1`，這個 runtime 不能用）。

### 3.4 `SYSTOLIC_RUNTIME_TIMING=1` 的輸出

關閉 session 時印一行 `[fpga-async] timing tiles= completed= failed= gemms= span_us=
submit_us= wait_us= reads= empty_reads= mem_writes= descriptors= bytes_to_fpga=
bytes_from_fpga= passes= overlap2= overlap4= staged_behind= max_queued= stage=
read_policy= queue_limit= per_device=`。`span_us` 是第一個 submit 到最後一個 wait；
`overlap2`／`overlap4` 是 pump 看到 ≥2／4 顆 device 有 descriptor 在飛的次數，
`staged_behind` 是看到有 slab staged 在跑中 job 後面的次數（都是 host 視角的計數，
板上的硬體證據是 VIO 的 `overlap2_cycles` 等）。`tools/e2e_timing.py` 會把它排成表。

### 3.5 `test/e2e/runtime_async_test.cpp`（`runtime-async-test`）

模式即實驗：A（單顆 device、不 stage）、B（四顆、不 stage）、C（四顆、stage）、D（兩個
GEMM 交錯送、stage）、X（同一輸出 tile 的 fold 分散到不同 device、非整數資料、與同步
entry point 逐位元比對）、V（host 端拒絕 -10/-11/-15/-17 一個 byte 都不送；
`SYSTOLIC_ASYNC_VIOLATE=1` 把 k = K_MAX+1 送出去，bitstream 回 status 1、wait 回 -18、
C 沒被碰、之後的 GEMM 正常）。A–D 用小整數，逐元素對 int64 參考。`--gemm MxNxK`、
`--devices`、`--stage` 可調；每個 mode 自己開關一個 session。

沙盒實跑（cosim，K_MAX 16／16，全部 PASS）：

| 執行 | tiles | 結果與計數 |
|---|---|---|
| `--mode all`（16x16x32） | A 8、B 20、C 20、D 40、X 20、V 7 | 全 PASS；C 的 `staged_behind` = 31 297／31 732 次 pump、`overlap4` = 25 263；B 的 `staged_behind` = 0（stage 關閉生效） |
| `--mode B,C,D --gemm 32x32x64` | 160／160／320 | 全 PASS；佇列上限 64 有被碰到（`max_queued=64`，submit 阻塞路徑有跑） |
| `SYSTOLIC_ASYNC_QUEUE_LIMIT=4 --mode C,D --gemm 24x24x48` | 63／126 | PASS |
| `SYSTOLIC_ASYNC_READ_POLICY=always`、`=wait` | 20／40 | PASS |
| `--gemm 13x11x37 --devices 0,1,3 --mode B,C,X` | 18 | PASS（邊界 tile、device 子集合、fold 跨 device 逐位元相同） |
| `--devices 1,2,3 --mode A,B,C --gemm 12x12x40`、`--devices 0 --mode B,C --gemm 16x8x20` | 27、4 | PASS |

cosim 的 wall-clock 是 Verilator 的速度（每個 tile 約 150–190 ms，四個 mode 都是 5–6
tile/s），**看不出重疊帶來的加速**；它證明的是正確性與「重疊真的發生」（計數器），
加速要板子量。

## 4. Compiler（提案第 4 階段）

- `--systolic-binary-search-tile-matmul` 在每個 `systolic.matmul_tile` 上多放四個
  discardable attribute：`systolic.gemm`（module 裡第幾個 linalg.matmul）、
  `systolic.tile`（optimizer 的 task index，即輸出 block）、`systolic.k_index`／
  `systolic.k_count`（第幾個 K fold、共幾個）。同步 lowering 不理它們；
  `test/Systolic/binary_search_ids.mlir`。
- `--lower-systolic-matmul-tile-to-fpga="async=true"`（pipeline：
  `--systolic-fpga-pipeline="async=true"`；`systolic-matmul --async`）：每個 GEMM 在第一個
  tile 前 `fpga_gemm_begin`，每個 tile 一個 `fpga_tile_submit`，C **不再經過 scratch**——
  直接給輸出 buffer 裡該 block 左上角的指標與 row stride（`ldc` = 輸出矩陣的 N），
  `fpga_gemm_wait` 放在**第一個讀到這個 GEMM 結果的 op 前面**（沒有就放 block 結尾）。
  結果：同一函式裡兩個獨立 GEMM 的 submit 全部先於兩個 wait（板上四顆陣列可同時忙
  兩個 GEMM），而讀前一個 GEMM 結果的 GEMM 會先等它。A／B 的 staging 不變。
  一個 GEMM 的 tile 必須在同一個 block（否則 diagnostic）。
  `test/Systolic/fpga_lowering_async.mlir`、`bad_fpga_lowering_async.mlir`。
- `systolic-matmul --async`：同一條 pipeline、JIT 裡多註冊三個符號；報告把
  `fpga_tile_submit` 當 runtime call 數，結果用 `fpga_async_runtime_get_stats` 的
  completed／failed 判斷，`FPGA invocations = N`、`DPTI: X B to the FPGA, Y B back`
  兩行格式不變，harness 的 verdict 照用。設定檔沒有 `COMPLETION_HEADER=1` 直接退出 4。
- `test/e2e/run_e2e.sh --mode async`：所有 case 用 `systolic-matmul --async`；runtime
  layer 多跑 `runtime-async-test --mode all`（GEMM 大小可用 `SYSTOLIC_E2E_ASYNC_GEMM`
  改）；`e2e_verdict.py --mode async --header 1` 檢查 `ir/20-fpga.mlir` 裡正好每個 tile
  一個 `fpga_tile_submit`、一個 begin、一個 wait、沒有同步呼叫，以及收回的 byte 數 =
  Σ(P×P×4 + 16)。`--mode sync`（預設）的 summary 行格式與原本完全相同（`check-e2e-config`
  仍照舊比對），只多了 `--header` 的收回 byte 檢查。

沙盒實跑（MLIR 23.1.3 從原始碼建置、clang 18）：lit 41/41（38 個既有 + 3 個新的；
`linalg_to_systolic_shapes.mlir` 與 `linalg_to_systolic_4x4x4.mlir` 原本以 `start_cycle = N :`
收尾的 13 行 CHECK 改成連四個新 attribute 一起比對——比原本更嚴，不是放寬）；
`check-e2e-config` 98 passed, 0 failed；`run_e2e.sh --backend cosim`：sync `--quick`
26/26、預設 43/43，`--mode async` `--quick` 26/26、預設 43/43、`--regression` 52/52
（含 64x64x64 = 616 tiles、32x32x65 = 185 tiles），每次的 configuration refusal、runtime
layer（dispatch test + async test 六個 mode）與注入寫入失敗的 dispatch failure 檢查都 PASS；
verdict 的 `--mode async --header 1` 檢查（IR 每個 tile 一個 `fpga_tile_submit`、收回
byte 數含 16 byte record）全部成立。

## 5. 測試矩陣：實跑過的／沒跑的

「PASS」只寫在真的執行過的項目上。

| 項目 | 哪裡跑 | 狀態 |
|---|---|---|
| 六個 `run_top_sim.sh` gate（新 RTL） | 沙盒 Verilator | PASS 6/6（第 2.6 節） |
| `tb_dpti_bus_arbiter` | 沙盒 Icarus | PASS |
| `tb_dpti_direct` basic/multi/refused/collision/direction；hol | 沙盒 Verilator | PASS 5/5；hol 如預期 STALL 後恢復（2026-10-14 的 RTL；整合後 `DRAINED`，第 9 節） |
| 舊 RTL 上重現四個 hazard | 沙盒 Verilator（worktree 6ebdc6a） | 重現（第 2.6 節） |
| `runtime-dispatch-test`（同步 runtime + completion record） | 沙盒 cosim | PASS 14/14（settle 1000 與 0 µs） |
| `runtime-async-test` A/B/C/D/X/V 與變體 | 沙盒 cosim | PASS（第 3.5 節的表） |
| lit（38 既有 + 3 新） | 沙盒（MLIR 23 原始碼建置） | PASS 41/41 |
| `check-e2e-config`（`test_e2e_config.sh`） | 沙盒 | PASS 98/98 |
| `run_e2e.sh --backend cosim`（sync）`--quick`／預設 | 沙盒 cosim | PASS 26/26、43/43 |
| `run_e2e.sh --backend cosim --mode async` `--quick`／預設／`--regression` | 沙盒 cosim | PASS 26/26、43/43、52/52 |
| 注入寫入失敗（`SYSTOLIC_DPTI_COSIM_FAIL_WRITE=7`，sync 與 async） | 沙盒 cosim | PASS：TRANSPORT rc=-6，5/5 call 失敗、1057 B 送出、0 B 收回 |
| Vivado 合成、timing、新 bitstream | **板子的機器** | 未跑 |
| 板上 `runtime-dispatch-test`／`run_e2e.sh --backend board`（sync 與 async） | **板子** | 未跑 |
| 板上 A0／A／B／C／D 量測 | **板子** | 未跑（第 6 節） |

## 6. 板上要做的事（未跑）

### 6.1 建 bitstream

```bash
cd rtl/multi_200t/tile_pipelined/dma
vivado -mode batch -source dma_top_build.tcl -tclargs build v1 16 16     # K_MAX 16/16
# 成功後 .bit 旁有 fpga_config.txt（含 COMPLETION_HEADER=1、WNS/WHS）；TIMING_MET=0 就停
```

timing 若不收斂（arbiter 在 60 MHz 的 dpti_clkout 域、grant arbiter 在 AXI 域，都只多
一層 mux／暫存器，預期收斂），先回報 WNS／WHS 再決定，不要放寬 constraint。

### 6.2 A0：同步 baseline 矩陣（量「每個 job 的固定成本」）

舊 bitstream 或新 bitstream 的同步路徑都可以；每格跑 `run_e2e.sh --backend board
--quick --timing`，看 `tools/e2e_timing.py` 的 `inv_median_us` 與 phase share：

| | `SYSTOLIC_DPTI_SETTLE_US=1000`（原本） | `=0` |
|---|---|---|
| latency timer 16 ms（預設） | A0-a | A0-b |
| `SYSTOLIC_DPTI_LATENCY_MS=1` | A0-c | A0-d |

預期：read_us 由 latency timer 主宰（16 ms → 1 ms），settle 直接少 1 ms；這決定非同步
路徑每筆 completion 的下限。

### 6.3 A／B／C／D

```bash
export SYSTOLIC_FPGA_CONFIG=<新 bitstream 的 fpga_config.txt>
export SYSTOLIC_RUNTIME_TIMING=1 SYSTOLIC_DPTI_LATENCY_MS=1
build/bin/runtime-dispatch-test                                    # 先確認同步路徑在新 bitstream 上 PASS
build/bin/runtime-async-test --mode all                            # 正確性
build/bin/runtime-async-test --mode A,B,C,D --gemm 64x64x64        # 計時：span_us、tiles/s
SYSTOLIC_ASYNC_READ_POLICY=always build/bin/runtime-async-test --mode C --gemm 64x64x64
SYSTOLIC_ASYNC_READ_POLICY=wait   build/bin/runtime-async-test --mode C --gemm 64x64x64
test/e2e/run_e2e.sh --backend board --mode async --timing          # 整條 pipeline
test/e2e/run_e2e.sh --backend board --timing                       # 同一顆 bitstream 的 sync 對照
```

每次把 stdout／stderr 與 `dma_top_build.tcl` readout 的 VIO 計數（`overlap2_cycles`、
`overlap4_cycles`、`idle_gap_cycles`、`rb_seq`）存進 `eval/async/<日期>/`（提案建議的位
置；本分支沒有動 `eval/`）。板子與模擬的數字分開記。

### 6.4 證據表（模板）

| 量 | 同步（A0-a） | A | B | C | D |
|---|---|---|---|---|---|
| 每 tile 平均 µs（host wall） | | | | | |
| tiles/s | | | | | |
| `overlap2_cycles`／`overlap4_cycles`（VIO） | 0 | 0 | | | |
| `idle_gap_cycles[0]` 平均 | | | | | |
| `staged_behind` 佔 pump 的比例 | – | 0 | 0 | | |
| `empty_reads`／`reads` | – | | | | |

## 7. 限制與下一步

- ~~一顆 device 一個 outstanding descriptor~~ 已由 pipelined context 解決（第 9 節）：
  context 有 `JOB_QUEUE_DEPTH` 深的 job queue，top 在 queue 有空位且尚未讀回的 job 少於
  `RB_DEPTH = 8` 時就收 descriptor，bridge 不再 head-of-line；runtime 的 credit 跟著
  `PIPELINE_DEPTH`。
- `start_cycle` 仍原樣寫進 descriptor；硬體 scheduler 的 `cycle_counter` 只在有 job 忙
  時前進且不歸零，所以它只是下限（最多讓 reset 後最早的幾筆 job 多等 `start_cycle` 個
  cycle），不會卡死，也不是硬體強制的發射時間。
- 空讀在 FT2232H 上的成本是一個 latency timer 週期，`blocked` 策略讓它只發生在「沒東西
  可寫」的時候；真正的數字要板上量（第 6.3 節三種 policy）。
- cosim 的 DDR model 1 MiB（`DDR_BYTES=1048576` 寫在它的 `fpga_config.txt`）：非同步
  runtime 的 4 × (depth + 1) 個 operand slot 從 0x10000 起，depth 4 時 K_MAX ≤ 768 放得下，
  放不下時 runtime 自己把 depth 降低並在 stderr 說明；板子的 DDR（512 MiB）沒有這個限制。
- 同步與非同步 runtime 各自持有一條 session，不能同時開（同一顆 FT2232H）；
  `runtime-async-test` 的 mode X 是先關一邊再開另一邊。
- 沒做的選配：RESET 暫存器、frontend timeout、SIWU#、XDC 變更。

## 8. 檔案

| 階段 | 檔案 |
|---|---|
| 0 | `runtime/dpti_ftdi.cpp`（`SYSTOLIC_DPTI_LATENCY_MS`、`SYSTOLIC_DPTI_READ_CHUNK`、`dpti_ftdi_read_some_session`）、`runtime/fpga_matmul_tiled.{h,cpp}`（計時、`SYSTOLIC_DPTI_SETTLE_US`、completion record、slab packer／device table 的輸出）、`test/e2e/run_e2e.sh --timing`、`tools/e2e_timing.py` |
| 1 | `test/e2e/cosim/dpti_protocol_check.svh`、`fp_ieee_dpi.cpp`、`tb_dpti_cosim.sv`（檢查器）、`build.sh`（預設 working tree、`COMPLETION_HEADER`）、`rtl/.../dma/tb/mig_axi_model.sv`（`+w_stall`）、`tb_dpti_direct.sv`、`run_dpti_direct.sh` |
| 2 | `rtl/.../dma/dpti_bus_arbiter.sv`、`systolic_dma_top.sv`、`tb/xil_stubs.sv`、`tb/run_top_sim.sh`、`tb/tb_dpti_bus_arbiter.sv`、`dma_top_build.tcl` |
| 3 | `runtime/fpga_async_runtime.{h,cpp}`、`runtime/CMakeLists.txt`、`test/e2e/runtime_async_test.cpp`、`test/e2e/CMakeLists.txt` |
| 4 | `lib/Systolic/Transforms/SystolicBinarySearchTileMatmulPass.cpp`、`TileMatmulForFpga.cpp`、`SystolicPipelines.cpp`、`include/Systolic/Passes.h`、`tools/systolic-matmul/systolic-matmul.cpp`、`test/Systolic/binary_search_ids.mlir`、`fpga_lowering_async.mlir`、`bad_fpga_lowering_async.mlir`、`test/e2e/run_e2e.sh --mode`、`e2e_verdict.py --mode/--header` |
| 5 | 這份文件、`docs/E2E.md`、`REPRODUCE.md` |

## 9. 與 pipelined context 的整合（2026-10-18）

main 的 `ff68494`（`docs/COSTMODEL.md` 第 8 節）讓每個 accelerator context 有 4 深的 job
queue：下一個 invocation 的 fill 在前一個 fold 開始時發出、下一個 fold 在前一個 tile 還在歸約時
放行（v2 operand path），結果依序寫回、top 每個 context 一個 8 深的 readback FIFO。本節是把它和
本分支整合後的狀態；所有數字都是 Verilator co-simulation（**板子 NOT RUN**）。

### 9.1 整合的改動

- **RTL（`systolic_dma_top.sv`）**：readback FIFO 的每個 entry 多存 `job_id`（在該 job 的
  `job_done` 那一拍從 context 的 `job_id_reg` 取得——context 緊接著就處理下一個 queue 裡的
  job，之後再讀會拿到別的 job），completion record 的 FSM（RB_IDLE/HDR/DATA）在 grant 時 pop
  FIFO、從 head 取 `job_id` 與 `c_base`；refusal record、FT2232H bus arbiter（只在
  `DPTI_PHY_SYNC245 = 0` 的 legacy pads path；預設的 registered PHY 自己仲裁方向）、DDR 寫入埠
  grant arbiter 都保留。grant arbiter 改成 host slab writer 與 write-back engine 在 burst 邊界
  輪流：slab writer 在前一個 burst 回應後立刻要下一個 burst、資料又以 DPTI 的速度慢慢進來，固定
  優先權會讓它佔住寫入埠一整塊 slab（K_MAX 16 約 5000 拍、K_MAX 512 約 16 萬拍），其間所有
  context 的 write-back 都等，pipelined 的 job 又因為 result bank 沒釋放而卡住後面的 fold
  （co-sim 量到的 cv→done：K_MAX 16 中位數 4787 拍、K_MAX 512 中位數 155 000 拍）。改成在
  burst 邊界讓給等待的一方（持有者至少送過一個 burst 之後才讓——第一個 burst 之前就讓會兩邊互
  讓、誰也不發，`tb_dpti_direct` 的 multi／direction 會死住），cv→done 降到中位數 947 拍
  （一個 host burst 的時間）與 199 拍（K_MAX 512，接近 write-back 本身的 91 拍）。
  `idle_gap_cycles` 改成「context 的 queue 空著」的拍數。
- **Compiler**：`emitSystolicTilePlan(..., gemmIndex)` 設 `systolic.gemm/tile/k_index/k_count`
  （linalg 與 affine 兩條路都編號），`--async` lowering 不變。
- **Runtime（`fpga_async_runtime.cpp`）**：第 3.2 節的 credit／slot 表；`SYSTOLIC_ASYNC_DEPTH`、
  `SYSTOLIC_ASYNC_BURST`；每條 trace 行結尾 `@<us>`；timing 行多 `depth= burst= depth2_passes=
  max_inflight= last_descriptor_us= first_completion_us=`。
- **設定檔**：`PIPELINE_DEPTH`（main）、`COMPLETION_HEADER`（本分支）、`DDR_BYTES`（新）三個鍵一起
  寫、一起讀（`check-e2e-config` 107/107）。
- **證據工具**：`test/e2e/cosim/tb_dpti_cosim.sv` 每個 job 印 `COSIM_JOB ev=fire|fill|fillend|
  fold|feedend|cv|done|rb|rbdone cyc= dev= job=`，`tools/cosim_overlap_evidence.py --cosim cosim.log
  --trace host.log --depth 4 [--session N]` 把 RTL 的 stamp 與 host 的 trace 對起來：每顆 device
  連續兩個 job 的 fold(T+1) 與 cv(T)／feedend(T)／done(T)／rbdone(T) 的先後、queue 最多幾個 job、
  host 每顆 device 最多幾個 descriptor 在飛、每個 GEMM 第一筆 completion 之前送出了幾筆 descriptor。

### 9.2 實跑結果（co-sim，K_MAX 16／16 除非另註）

| 項目 | 結果 |
|---|---|
| lit | 79/79（main 76 + 本分支 3） |
| `check-e2e-config` | 107/107 |
| `run_e2e.sh --backend cosim --quick`（sync）／`--mode async --quick` | 26/26、26/26 |
| `runtime-dispatch-test`、`runtime-async-test --mode all`（v1 與 v2 operand path） | PASS |
| `eval/pipeline_overlap/run.sh`（main 的 RTL 證據，internal job port） | 14 組全 PASS、cost model 0 mismatch：v2 的 fold(T+1) < cv(T) 對數 dev0 5、dev1 9、chain 3；gapbound 最小 PE 間隔 26 = FEED_GAP；`PE_QUIET_GAP=8` 負對照 FAIL |
| `run_top_sim.sh` 六個 gate、`run_kmax_sim.sh 16 16` | PASS（kmax 的 refusal 期望改成「被 refusal path 取走、從不 fire」） |
| `run_dpti_direct.sh all`（兩種 PHY）、`hol`、`tb_dpti_bus_arbiter` | 5/5 PASS ×2、hol `DRAINED`、PASS |
| backpressure：`SYSTOLIC_ASYNC_QUEUE_LIMIT=4` mode C/D、depth 1、`SYSTOLIC_ASYNC_BURST=0` | PASS，每顆 device 在飛數永遠 ≤ depth |

**Batch submission（host 端）**：mode C 16x16x16（10 tiles）、`SYSTOLIC_ASYNC_READ_POLICY=wait`：
10 筆 descriptor 全部在第一筆 completion 之前送出（`last_descriptor_us=167 < first_completion_us=
1962600`）。GEMM 大於 16 個 credit（4 顆 × depth 4）時以 credit 為一批：mode C 32x32x64（160 tiles）
第一筆 completion 之前送出 16 筆，之後每收一筆 completion 補一筆，`max_inflight=4/4/4/4`。

**硬體 queue（RTL 端）**：K_MAX 512、12 個 k = 512 的 job 送同一顆 8x8（mode C `--devices 0`）：
runtime 先 stage 5 塊 slab、再連送 4 筆 descriptor，context 的 queue 被填到 4
（`max_jobs_queued=4`），四筆 descriptor 的 fire 相隔 420 拍（一筆 84 byte 的 descriptor 在鏈路
上的時間），fill(T+1) 在 fold(T) 後 3 拍就發出、整個蓋住 T 的 compute、write-back 與 readback
（job 0：fold 830050、cv 830671、done 830762、rbdone 831690；job 1：fill 830053–838245、fold
838253）。K_MAX 16 時（Test B 的 10-tile run）fold(T+1) < rbdone(T) 出現在 dev 0 與 dev 1：
下一個 tile 的 compute 與前一個 tile 的 DPTI 讀回重疊，這在舊 RTL（readback 結束前不收新 job）
做不到。同一個 GEMM 四顆陣列一起跑（30 個 job）：depth 4 的 makespan 265 萬拍、depth 1 462 萬拍
（1.74×；K_MAX 16 的 160 tiles 兩者都是 85 萬拍——鏈路時間相同，差別只在 host 是否每筆等
completion 才送下一筆）。

**什麼沒有發生，為什麼**：主機路徑上 fold(T+1) < cv(T)（compute 與前一個 tile 的 reduction
重疊）在任何 K_MAX 都量不到：k = 16 的 tile 算 125 拍，但一筆 descriptor 要 420 拍、一塊 1 KiB
slab 約 5000 拍，陣列的 queue 幾乎永遠只有 1 個 job（link-bound：mode C 32x32x64 的 makespan
85 萬拍，其中陣列真正在算的只有 1.9 萬拍，depth 1／4、burst 0／1 都一樣）；k 大時（512）compute 621
拍 > descriptor 420 拍，但 v1 的 fill 8203 拍又把 fold 推到 cv 之後（第 8 節的 v1 fill-bound）。
PE 層的重疊因此只在 `eval/pipeline_overlap/run.sh`（internal job port，host 無限快）上證明；
主機路徑上整合兌現的是：不必每筆 job 寫→等→讀的序列、四顆陣列同時忙、queue 裡 2–4 個 job
時 fill／write-back／readback 與下一個 job 重疊。

**板子**：Vivado 合成、新 bitstream、`run_e2e.sh --backend board`：**NOT RUN**。
