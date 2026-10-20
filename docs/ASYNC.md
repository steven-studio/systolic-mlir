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

DPTI 沒有暫存器可讀，host 只能從設定檔知道 bitstream 送不送 record。設定檔沒寫
`COMPLETION_HEADER=1`（包括完全沒有設定檔時的 built-in default）卻接上會送 record 的
bitstream 時，2026-10-20 之前同步 runtime 把 record 當成 tile 的前 16 byte 讀進來：rc 0、
數字是錯的（C[0] 3.717e15 就是 magic 被當成 float），tile 的尾巴留在鏈路上給下一次讀，
`systolic-cc --runtime=board` 的程式印 `FPGA Executed` 並以 0 結束。現在沒有
`COMPLETION_HEADER=1` 時每個結果的前 16 byte 先單獨讀：開頭是 record 的 magic 就回 -9、
stderr 一行指出用的是哪個設定檔，這條 session 停用，什麼都不會加進 C（systolic-cc 的程式
因此在 CPU 上算）；不是 magic 就讀剩下的 tile（沒有 record 的舊 bitstream 照常）。

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
  辨認上一個 session 遲到的 record 並丟掉。（2026-10-20 更正：epoch 原本每個 process 都從 1
  開始、同步 runtime 的 job_id 每個 process 都從 0 開始，所以這只在同一個 process 裡成立——
  另一個程式（例如帶著在飛的 job 被殺掉的）留下的 record 會被當成自己的結果。現在兩個
  runtime 的 job_id 都是 `epoch<<24 | seq`，process 的第一個 session 用隨機的 epoch（1..254），
  見第 3.2 節。）
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
| credit | 每顆 device：descriptor `depth` + stage 1，`depth` = 設定檔的 `PIPELINE_DEPTH`（context 的 job queue，本樹的 RTL 是 4；沒有這個鍵的舊 bitstream 是 1；`SYSTOLIC_ASYNC_DEPTH` 可往下調，不會高於 `PIPELINE_DEPTH`，上限 8）。預設的 burst 模式（`SYSTOLIC_ASYNC_BURST=1`）先把一顆 device 的 operand slot 全部填滿（`depth + 1` 塊 slab）或等到 `fpga_gemm_wait` 才送它**第一批** descriptor，讓 context 連續收到 job（第 9 節）；之後只要該 device 有 job 在飛，stage 好的 descriptor 一有 credit 就送（在飛的 job 佔著 operand slot 直到 completion，第一批之後 slot 永遠填不滿——2026-10-19 之前 burst 模式在長 GEMM 上因此退化成每顆一次一筆，第 10 節）；`SYSTOLIC_ASYNC_STAGE=0` 時每顆 device 仍然一次一個 descriptor（mode A／B） |
| DDR slot | 每顆 device `depth + 1` 個 operand slot（`0x10000 + (dev*(depth+1)+slot)*slab_bytes`，256 B 對齊）與 `depth` 個 result slot（`0x3000 + (dev*depth+slot)*256`，16 B 對齊），以 free mask 配置；一個 job 的兩個 slot 在它的 completion record 讀到之後才釋放（context 在 fold 之前就讀完 slab，result tile 在 record 的 payload 之前就離開 DDR），所以 staged 的 slab 永遠不會覆蓋陣列還可能在讀的那塊、result slot 不會在 host 拿到之前被覆蓋；設定檔有 `DDR_BYTES`（co-sim 寫 1 MiB）時 depth 會降到 operand 區域放得下為止 |
| job_id | `epoch<<24 \| seq`：epoch 每開一次 session 換下一個，process 的第一個 session 隨機（1..254；0 不用、255 保留；`SYSTOLIC_DPTI_EPOCH` 可固定第一個，測試用），seq 24 bit；上一個 session 或另一個程式遲到的 record 查不到 PJT 就丟掉（兩個程式同 epoch 的機率 1/254）。同步 runtime 用同樣的格式，它對不上的 record 回 -9 |
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

GEMM 的第一次 wait／wait_for（不論 timeout）把它封住：之後不收 submit，而且 `k_count`
個 fold 沒有全部被收下的輸出 tile（有 fold 在 submit 時被拒，例如 k 超過這個 bitstream 的
K_MAX——為更深的 K_MAX 編譯的程式——或根本沒送）永遠不能照 k_index 順序累加：它在封住時
以 -17 失敗，已經 held 的 fold 立刻失敗、還在路上的到了就丟掉，GEMM 的 wait 回 -17（stderr
一行 `[fpga-async] incomplete K folds: ...`）。在這之前（2026-10-20），被拒的 fold 不留任何
紀錄，後面已收下的 fold 被 held 等一個永遠不會來的 fold，`fpga_gemm_wait` 沒有期限、鏈路
timeout 也管不到它，程式永遠卡住；被拒的是最後一個 fold 時則回 0、C 只有部分 K 的和。編譯出
的 kernel 在涵蓋一個輸出 tile 的 wait 之前送完它所有的 fold，所以 submit 全被收下的 GEMM 不會
觸發。軟體模型（`soft_runtime.c`）做同樣的檢查。

submit 時被拒（任何非 0 的回傳）的 tile 是一次失敗的 invocation：session 統計
（`fpga_async_stats_t` 的 `tiles_submitted`、`tiles_failed`、`first_error`，handle 不是開著的 GEMM
時記失敗的 begin 的錯誤碼）把它算進去，就像同步 entry point 算被拒的呼叫——編譯出的 kernel
不看回傳碼，`board_stats.c`／shim 與 `systolic-matmul` 看的是統計；GEMM 自己的 wait 仍只報
它收下的 tile（mode V 的語意不變）。一個 session 的前 8 筆拒絕印在 stderr（`[fpga-async]
refused at submit: ...`，-11 附 `ldc`；鏈路失敗後的 -14 不印）。2026-10-20 之前拒絕不留紀錄，
只有 shim 的 invocation 計數看得出來。軟體模型同樣計數，它的同步 wrapper 不重複計。

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
C 沒被碰、之後的 GEMM 正常）。2026-10-18 多了三個（第 10 節）：S（提交語意：每個 submit
回來的 wall-clock 對 runtime 記的第一筆 completion、第一筆 completion 前送出的 descriptor 數、
排在在飛 descriptor 後面送出的數、沒有 submit 被擋；任一項不成立報 SERIALIZED）、Q（背壓：
`SYSTOLIC_ASYNC_QUEUE_LIMIT=1`、GEMM 放大四倍，submit 必須等空位——等的是任一舊 job 的
completion 騰出的 operand slot，不是前一筆 job；在飛數永不超過 depth）、M（三個不同形狀的
GEMM 輪流送、反序 wait，每個 GEMM 在自己的 wait 回來後、其他兩個還在飛時就逐元素檢查）。
2026-10-20 加 H（第 3.3 節的封住：輸出 tile 的 fold 0 在 submit 時以 -10 被拒、fold 1 收下，
有期限的 wait 必須回 -17 而不是 -20；fold 1 FAILED/-17、另一個輸出 tile DONE 且正確、失敗的
tile 的 C 沒被碰；用逾時的 wait_for 封住後只 poll 也要在期限內 done；被拒的是最後一個 fold 時
wait 回 -17；之後的 GEMM 正常。只用有期限的 wait，沒有封住的 runtime 讓這個 mode 失敗而不是
卡住整個測試；板子上也能跑）。
2026-10-19 再加 P（poll／wait_for／release 的語意，第 11.1 節）與 F（提交中途鏈路失敗）。
A–D 用小整數，逐元素對 int64 參考。`--gemm MxNxK`、
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
  **2026-10-18 起這是預設**：不加旗標時設定檔有 `COMPLETION_HEADER=1` 就走非同步 lowering，
  `--sync` 才回到同步 runtime（第 10 節）。
- affine 路徑（`--affine-matmul-to-systolic` 的輸出，C 是 `bufferization.to_tensor %C restrict
  writable` 包起來的 memref）：lowering 現在把這個 restrict、writable、row-major（identity，或
  C front end 對 `memref<?x16xf32>` 引數取的 `strided<[16, 1]>` 靜態 subview）的 to_tensor 當成
  output block 的根（原本只收函式的 tensor 引數），直接用它的 memref（不再經 `to_buffer`），
  async 模式下每個 block 的 C 指標從 layout 的 offset／stride 算進 `%C`（`ldc` = stride[0]），
  唯一的 reader 是最後的 `materialize_in_destination`，wait 落在最後一個 submit 之後。改之前
  每個 tile 走 per-tile fallback，它的 `insert_slice` 立刻讀結果，wait 被放在**第一個** submit
  後面，後面的 submit 被 runtime 以 -17 拒絕——affine／C 路徑的非同步 lowering 實際上只跑第
  一個 tile（systolic-cc 的程式因此全部 fallback 到 CPU）。
  `test/Systolic/affine_to_fpga_pipeline_async.mlir`（含 C front end 的 strided 形狀）。
  2026-10-20：runtime 在 `C[i * ldc + j]` 累加、`ldc` 是 i32、`ldc < n` 回 -11，所以只有 row stride
  是靜態、介於 row 長度與 INT32_MAX 之間的 view 能直接寫（`isRuntimeAddressable`）。反向的 row
  （`C[11 - i][j]` 的 `strided<[-10, 1]>`，含 run-time offset 的版本）、存成一行的單一 row（3x1 array
  的 1x3 view，stride 1 < n）、run-time 的 row stride：每個 GEMM 一塊連續的 M x N buffer，在 GEMM 的
  begin 之後從 view 複製進去，每個 block 以 `ldc = N` submit 進它，wait 之後由 front end 的
  `materialize_in_destination` 複製回去（deallocation pipeline 之後釋放）。之前這種 view 以負的或太小的
  `ldc` submit、被 runtime 拒絕、不留紀錄，C 沒被寫：直接呼叫 kernel 的程式得到錯的結果，systolic-cc
  的程式整個函式（連同同一函式裡已在 FPGA 上跑完的 GEMM）fallback 到 CPU。
  `test/Systolic/affine_to_fpga_async_views{,_large}.mlir`、`test/SystolicCC/async_views.c`。
- 仍走 per-tile fallback 的 GEMM（C 的根是 `tensor.empty`／`linalg.fill`、loop-carried、
  interleaved：`fpga_operand_staging_fallback.mlir` 的形狀）：reader（每個 block 的
  `insert_slice`）出現在後面的 submit 之前時，lowering 在那裡 wait、再開一個新的
  `fpga_gemm_begin` 給剩下的 tile（runtime 拒絕 wait 之後的 submit），GEMM 變成每個 output
  block 一批；切點只能落在 output tile 之間——一個 tile 的 K fold 若跨切點，runtime 的
  k-ordered 累加永遠等不齊，lowering 直接報錯。`test/Systolic/fpga_lowering_async_split.mlir`、
  `bad_fpga_lowering_async.mlir`（read_between_folds）。
- `systolic-cc --runtime board`：`runtime-dispatch-test --print-config` 多印 `COMPLETION_HEADER=`，
  有 record 就把 `async=true` 交給 lowering；同一份輸出的 `PIPELINE_DEPTH` 交給
  `--systolic-declare-fpga-fleet=... pipeline-depth=D`（D > 1 宣告 pipelined fleet，與
  `systolic-matmul` 同一規則，`docs/COSTMODEL.md` 第 8 節）（`--submission auto|sync|async`，預設 auto；async 在
  soft runtime 或沒有 record 的設定上被拒絕）；`runtime/offload/board_stats.c` 同時回報兩個
  runtime 的統計並關閉兩條 session，shim 的 FPGA／CPU 判定不變。soft runtime 只有同步 entry
  point，lit 的 `test/SystolicCC` 照舊走同步 lowering。
- `test/e2e/run_e2e.sh --mode async`：所有 case 用 `systolic-matmul --async`；runtime
  layer 多跑 `runtime-async-test --mode all --backend <backend>`（GEMM 大小可用
  `SYSTOLIC_E2E_ASYNC_GEMM` 改；mode F 只能在 co-sim 跑，板子上是 NOT APPLICABLE，
  co-sim 上必須 PASS）；`e2e_verdict.py --mode async --header 1` 檢查 `ir/20-fpga.mlir` 裡正好每個 tile
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
| `runtime-async-test` S/Q/M（提交語意、背壓、多 GEMM 隔離） | 沙盒 cosim | PASS（第 10 節） |
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
build/bin/runtime-async-test --mode all --backend board            # 正確性（mode F 只在 co-sim：NOT APPLICABLE）
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
- affine 路徑上 C 若是 strided 的 subview（GEMM 只寫 memref 的一個子區塊），lowering 仍走
  per-tile fallback：結果正確，但非同步模式下 wait 落在第一個 submit 後面（一次一個 tile）。
  identity layout 的整塊 memref（`test/e2e/affine_c`、`test/SystolicCC` 的 kernel）不受影響。
- `fpga_matmul_tiled_auto_scheduled` 的契約就是回來時結果已在 C；它保留給 `--sync`、
  `--submission sync` 與沒有 completion record 的 bitstream，本身不會被「批次化」。
- 沒做的選配：RESET 暫存器、frontend timeout、SIWU#、XDC 變更。


## 10. 同一個 GEMM 的 job 不再逐筆等待：batch 提交成為預設（2026-10-19）

第 9 節的報告寫「同步 runtime 仍一次一筆（設計如此）」。這不成立：預設的編譯路徑
（`systolic-matmul` 不加旗標、`systolic-cc --runtime board`、`test/e2e/affine_c`）都還在走
`fpga_matmul_tiled_auto_scheduled`——每個 tile 寫 slab、送 descriptor、等結果回來才送下一個——
而 affine／C 路徑即使開 `async=true`，lowering 也把 `fpga_gemm_wait` 放在**第一個** submit 後面
（第 4 節），之後的 submit 被 runtime 以 -17 拒絕。本節是把「同一個 GEMM 的 job 不等前一筆完成
就送出；queue 滿了等的是 credit／空位，不是前一筆的 compute 或 write-back；每筆 completion 以
job_id 對回 GEMM 與 tile；wait 收齊全部結果」做成每條路徑的預設，並留下 host trace 與 cycle 級
co-sim 的證據。所有數字都是 Verilator co-sim（K_MAX 16／16、PIPELINE_DEPTH 4、1 KiB slab）；
**板子 NOT RUN**。

### 10.1 改動

- **Compiler（`TileMatmulForFpga.cpp`）**：affine 路徑的 C 根（`to_tensor %C restrict writable`，
  含 C front end 對 `memref<?x16xf32>` 取的 `strided<[16, 1]>` subview）成為 output block 的根，
  直接用它的 memref，C 指標與 `ldc` 從 layout 的 offset／stride 算；仍走 per-tile fallback 的 GEMM
  在 reader 處 wait 並重開一個 `fpga_gemm_begin`（每個 output block 一批），fold 跨切點報錯
  （第 4 節；`affine_to_fpga_pipeline_async.mlir`、`fpga_lowering_async_split.mlir`、
  `bad_fpga_lowering_async.mlir`）。
- **Drivers**：`systolic-matmul` 在設定檔有 `COMPLETION_HEADER=1` 時預設非同步，`--sync` 強制同步、
  `--async` 堅持非同步（沒有 record 退出 4）；`systolic-cc --runtime board` 同樣預設非同步
  （`--submission auto|sync|async`，`runtime-dispatch-test --print-config` 多印 `COMPLETION_HEADER=`），
  `runtime/offload/board_stats.c` 合併兩個 runtime 的統計並關閉兩條 session；
  `test/e2e/affine_c/run.sh` 加 `SUBMISSION=auto|sync|async`（async 時檢查 begin 1、wait 1、wait 在最後
  一個 submit 之後）；`run_e2e.sh --mode sync` 明確傳 `--sync`；`run_board.sh` 把 `[fpga-async]
  accumulate` 算成硬體 invocation。soft runtime 只有同步 entry point，lit 的 `test/SystolicCC` 不變。
- **Runtime（`fpga_async_runtime.cpp`）**：burst 模式原本要等一顆 device 的 operand slot 全部填滿
  才放行 descriptor，但在飛的 job 佔著 slot 直到 completion，第一批之後永遠填不滿——長 GEMM 上每顆
  device 退化成一次一筆在飛（32x32x64 的 148 tiles：第一筆 completion 前只送出 4 筆）。現在 device
  一有 job 在飛，stage 好的 descriptor 一有 credit 就送。新計數器 `descriptors_before_first_completion`、
  `descriptors_behind_inflight`、`credit_waits`、`submits_blocked`、`ns_submit_blocked`（timing 行與
  `systolic-matmul` 的 `batch:` 行都印），新 trace 行 `backpressure`（device 的 credit 用完／host
  queue 滿）、`credit`（descriptor 用哪一筆 job 的 completion 騰出的 credit 送出、還有幾筆更新的在飛）、
  `resume`（被擋的 submit 在哪一筆 job 的 completion 之後恢復）。
- **測試**：`runtime-async-test` 多了 S／Q／M（第 3.5 節）；`tools/cosim_overlap_evidence.py` 從 host
  trace 算每顆 device 的 `sent_with_previous_still_in_flight`、`credit_bound`／`freed_by_oldest`／
  `next_oldest_still_in_flight`、`waited_for_previous`（同一 GEMM 內前一筆已完成、credit 又空著卻才送
  ——逐筆等待唯一會露出的樣子），`--host-session`、`--strict`。

### 10.2 證據（co-sim；每列都是獨立的實跑）

| 執行 | tiles | submit 回來早於第一筆 completion | 第一筆 completion 前已送出的 descriptor | 排在同顆 device 在飛 descriptor 後面送出 | 等 credit 才送出（全部由該 device 最舊 job 的 completion 放行、前一筆仍在飛） | host queue（64）擋住的 submit | 結果 |
|---|---|---|---|---|---|---|---|
| `runtime-async-test --mode S` 16x16x32 | 20 | 20/20（最後一筆 submit 278 µs 回來，第一筆 completion 2.70 s） | 16 = Σ min(tiles_d, 4) | 16 | 4 | 0 | PASS |
| `--mode S` 32x32x32 | 80 | 80/80（939 µs vs 2.44 s） | 16 | 76 | 64 | 0 | PASS |
| `--mode S --devices 0` 32x32x16（單顆） | 16 | 16/16（164 µs vs 0.77 s） | 4 | 15 | 12（每筆放行時 3 筆更新的還在飛） | 0 | PASS |
| `--mode S` 64x64x64 | 640 | 83（之後的 submit 被 host queue 擋住，等的是空位） | 16 | 633 | 612（host trace：credit_bound 115／168／168／164，全部 freed_by_oldest 且 next_oldest_still_in_flight；waited_for_previous 0；`--strict` 過） | 531（531 筆 `resume` 全部在比前一筆更舊的 job 的 completion 之後） | PASS；makespan 3.68M 拍 |
| `systolic-matmul` 32x32x64（預設、無旗標） | 148 | 84 | 16（修 burst 前 4） | 144（修前 79） | 132（修前 67） | 65 | PASS；24.3 s（修 burst 前 29.8 s；`--sync` 35.7 s） |
| `systolic-matmul` 13x11x37（邊界 tile、K fold 16+16+5） | 18 | 18/18 | 14 | 14 | 4 | 0 | PASS |
| `--mode Q`（`SYSTOLIC_ASYNC_QUEUE_LIMIT=1`）64x64x32 | 320 | — | 4 | 286 | 125 | 84（共等 50.1 s，每次都在某筆更舊的 job 的 completion 之後恢復） | PASS；每顆在飛 ≤ 4 |
| `--mode M` 三個 GEMM（32³、32x32x64、8x8x32）輪流送、反序 wait | 242 | — | 16 | 238 | 226 | 159 | PASS：GEMM 2 在整個 session 完成 159/242 時就驗證正確、GEMM 0 在 170/242 |
| `affine_c/run.sh BACKEND=board` 16x16x16／24x24x40（C front end） | 10／63 | — | 10／16 | 6／59 | 0／47 | 0 | PASS，wait 在最後一個 submit 之後（修前：wait 在第一個 submit 後，stage FAIL） |
| `run_board.sh --board-is-free`（cosim）program／faults／fallback | 47／10／22 | — | — | — | — | — | PASS，0 個 fallback（修前三個程式全部 FALLBACK：每個 GEMM 只送出第一個 tile） |
| `run_e2e.sh --backend cosim --quick`：`--mode sync`（`--sync`）／`--mode async` | 26 cases ×2 | — | — | — | — | — | 26/26、26/26 |
| `runtime-async-test --mode all`（九個 mode）、lit、`check-e2e-config`、`eval/pipeline_overlap/run.sh` | | | | | | | PASS、81/81、107/107、14 組 0 mismatch（RTL 沒動） |

submit 與 completion 的時間戳在 host trace（`@<us>`），job 的 fire／fold／cv／done／rb 在 cosim log
（`COSIM_JOB`），`tools/cosim_overlap_evidence.py --cosim --trace --depth 4 --session N --strict`
把兩邊對起來（上表的 host 欄位都是它算的）。

### 10.3 硬體 queue 的實況（不要多說）

credit 4 是 top 的 readback FIFO／DDR slot 層的在飛數；context 的 job queue 本身在 K_MAX 16 的
co-sim 上**最多同時 2 個 job**（dev 0 `max_jobs_queued=2`，三顆 4x4 都是 1）：一筆 84 byte 的
descriptor 在鏈路上約 420 拍、一塊 slab 約 5000 拍，而 k = 16 的 tile 算 125 拍，所以下一筆 descriptor
到達時前一筆幾乎都算完了（64³ 的 640 個 job：makespan 3.68M 拍，陣列真正在算的合計 75.9k 拍）。
fold(T+1) < rbdone(T)（下一個 tile 的 compute 與前一個 tile 的讀回重疊）在 dev 0 出現 12/127 對；
fold(T+1) < cv(T) 仍是 0（第 9 節的 link-bound 結論不變）。四筆 job「同時在硬體 queue 裡」在這條
鏈路上沒有發生，本節也不這樣宣稱；兌現的是 host 不再逐筆等結果、四顆陣列同時忙、queue 的空位由
completion 立刻補上。

### 10.4 sync 與 async 的語意

- `fpga_matmul_tiled_auto_scheduled` 的契約是「回來時結果已在 C」；它沒有被改成批次，保留給
  `--sync`／`--submission sync`、沒有 completion record 的 bitstream、soft runtime。
- `fpga_tile_submit` 在 packing 完、pump 把能寫的 slab／descriptor 寫完後就回來，不等任何 job 的結果；
  它只在 host queue 滿（64 筆已打包、每顆 device 的 operand slot 都有 slab、credit 都在飛）時等
  **任一** completion 騰出的空位（trace 的 `backpressure`／`resume`）。slab 寫入在 pump 裡同步進行，
  所以長 GEMM 的 submit wall time 含鏈路寫入時間（64³：最後一筆 submit 92.5 s 後回來，其中 92.5 s
  是在等空位），這不是在等前一筆 job 完成——上表 `resume` 全部在比前一筆更舊的 job 之後。

### 10.5 限制

- 板子：Vivado 合成、新 bitstream、`run_e2e.sh --backend board`、`run_board.sh`、`affine_c
  BACKEND=board` 真板：**NOT RUN**。
- context 的 job queue 在這條鏈路上最多 2 個 job（第 10.3 節）；PE 層的 compute／reduction 重疊仍只在
  `eval/pipeline_overlap/run.sh`（internal job port）上證明。
- C 的根若是 offset 動態的 subview（GEMM 寫 memref 的一個動態子區塊）仍走 per-tile fallback：結果
  正確，每個 output block 一批（wait + 新的 begin）。
- soft runtime 只有同步 entry point：`systolic-cc --runtime soft`、`affine_c BACKEND=soft` 是同步 lowering。
- `fpga_tile_submit` 的 wall time 含 pump 的鏈路寫入（上面）；真正的非阻塞要把寫入搬到另一條 thread，
  沒做。


## 11. 完整的非同步 Runtime API 與 Dynamic Offset 的 FPGA lowering（2026-10-19）

### 11.1 Runtime：transfer thread 與 poll／wait_for／release

第 10 節的 runtime 把所有鏈路工作放在呼叫者的 thread 上（`fpga_tile_submit` 裡 pump）：submit
不等 job 完成，但它的 wall time 含 slab 寫入，`fpga_gemm_wait` 是唯一的觀察方式，GEMM 在 wait 時
一併釋放。現在（`runtime/fpga_async_runtime.{h,cpp}`）：

- **一條 transfer thread 持有 DPTI session**：stage slab、送 descriptor、讀 completion record、
  依 job_id 對回 tile、k_index 順序累加進 C，都在它上面；呼叫者的 thread 只打包 slab 與排隊。一個
  mutex 保護兩邊共用的結構，thread 在每次傳輸呼叫**之前**放鎖、之後再取，所以 submit／poll 從不等
  一筆 slab 寫入或一次鏈路讀取（mode P 量到：worker 正在寫 slab 時 `fpga_gemm_poll` 最長 10.5 µs、
  `fpga_tile_poll` 59.6 µs）。兩個 condition variable：`cv_worker`（新 tile、有人在等、關閉）、
  `cv_caller`（completion、host queue 有空位、鏈路失敗）；沒事做時 thread 睡在 cv 上，空讀之後的
  暫停從 `SYSTOLIC_ASYNC_POLL_US`（50）起每次加倍到 `SYSTOLIC_ASYNC_POLL_MAX_US`（1000）——板上
  FT2232H 本來就以 latency timer 節流空讀，co-sim 上每個 request 都花模擬時間——不忙等。
- **API**（都在 runtime 的鎖上序列化，可從多個呼叫者 thread 呼叫）：
  | 呼叫 | 語意 |
  |---|---|
  | `fpga_gemm_begin` | 開一個 GEMM，回 handle（session 內不重用） |
  | `fpga_tile_submit` | 打包 A／B、排隊、回來；只在 host queue 滿（`SYSTOLIC_ASYNC_QUEUE_LIMIT`，64）時等**任一** completion 騰出的空位，從不等某筆 job |
  | `fpga_gemm_poll` | 不阻塞：submitted／queued／staged／in_flight／held／completed／failed、first_error、done、link_failed |
  | `fpga_tile_poll` | 不阻塞：某個 (tile_id, k_index) 的狀態（queued → staged → in_flight → [held] → done｜failed）、它的 job_id、錯誤碼 |
  | `fpga_gemm_wait_for(gemm, ms)` | 等到全部 tile 完成或失敗，最多 ms（`FPGA_ASYNC_WAIT_FOREVER` 不限）；逾時回 -20，GEMM 仍開著 |
  | `fpga_gemm_wait` | = wait_for(forever) + release（原本的契約，compiler 生的程式碼用這個） |
  | `fpga_gemm_release` | 釋放已完成（或鏈路已失敗）的 GEMM；還有 tile 在飛回 -21、什麼都不動 |
  | `fpga_async_runtime_close` | 停 thread、關 session；所有還開著的 GEMM 丟掉，之後不會再寫任何 C |
- **擁有權**（header 第一段）：A、B 在 submit 裡讀完、回來後可重用；C 是呼叫者的，transfer thread
  在 completion 到時寫它，從 submit 到 poll／wait 回報 done 之前呼叫者不能讀寫；handle 從 begin 到
  wait／release／close 有效；slab、descriptor、DDR slot、record、job_id 是 runtime 的。
- **錯誤**：submit 後鏈路失敗（mode F，`SYSTOLIC_DPTI_COSIM_FAIL_WRITE=7`；只有 co-sim 的
  libftdi 替身注入這個失敗，板子上 mode F 是 NOT APPLICABLE）：失敗前收下的 submit
  全部以 -4 失敗、之後的 submit 回 -14、wait 回 -4、`fpga_tile_poll` 每筆都是 failed/-4、C 一個元素
  都沒被碰、鏈路上只有三塊 slab（3099 B）、之後的 begin 回 NULL/-14，close 之後新 session 正常。
  refusal（mode V）、k-ordered 累加（mode X）、多 GEMM（mode D／M）都不變。
- **時間拆開算**（`fpga_async_stats_t`、timing 行、`systolic-matmul` 的 `time:` 行）：
  `ns_submit`（打包＋排隊）、`ns_submit_blocked`（等 host queue 空位）、`ns_link_write`（thread 在
  slab／descriptor 寫入裡）、`ns_link_read`（thread 在讀取裡）、`ns_collect`（累加進 C）、`ns_wait`
  （呼叫者在 wait 裡）、`polls`、`waits_timed_out`。
- **burst 的放行順序**：有人在等（或 `SYSTOLIC_ASYNC_BURST_HOLD_US`=1000 µs 沒有新 submit）時先把
  放得下的 slab 全部 stage 再送 descriptor（slabs first，與第 9 節單 thread 時 submit 同步 stage 的順
  序相同——e2e 注入失敗的檢查靠它），其餘時間空出的 credit 先用。

**軟體模型**（`runtime/offload/soft_runtime.c`）實作同一組 API：submit 複製 A／B、排隊、不算；
poll 跑一筆、wait 跑完該 GEMM 需要的；k_index 順序累加；`SOFT_RUNTIME_FAULT=fail` 讓第 N 筆
submit（process 內計數）以 -8 失敗並把「鏈路」標成失敗直到 close（與真 runtime 相同）。
`fpga_matmul_tiled_auto_scheduled` 現在是它上面的 wrapper（一個 GEMM 一個 tile，submit + wait，
每次呼叫後重開「鏈路」，所以一個 fault 只花那一次 invocation——舊語意不變）。`systolic-cc --runtime
soft --submission async` 因此可用（預設仍是 sync，lit 的 `test/SystolicCC` 舊期望不變）；
`test/SystolicCC/async.c` 跑 program.c 的程式走非同步 lowering：37 個 `fpga_tile_submit`、2 個 wait、
輸出與 clang 相同、`--verify` 逐位元、fault 之後整個 session 的 GEMM 都 fallback 到 CPU（真 runtime
的語意）。板上的 `fpga_matmul_tiled_auto_scheduled`（`fpga_matmul_tiled.cpp`）**沒有**改成 wrapper：它
是沒有 completion record 的 bitstream 唯一能用的路，而且 e2e sync 模式逐 byte 比對它的鏈路序列。

### 11.2 Dynamic Offset：從 C 原始碼到 runtime 的指標

之前「C view 的 offset 是動態的就走 per-tile fallback」其實到不了 lowering——整條路在更前面就拒絕：

| 階段 | 之前 | 現在 |
|---|---|---|
| C front end（`tools/systolic-cfront`） | 下標用到非 counter 的整數（`C[r0 + i][j]`）→「subscript uses 'r0', which is not an enclosing loop counter」 | 有號整數參數是 nest 的 affine symbol：函式入口 `%r0_idx = arith.index_cast %r0 : i32 to index`，下標 `(symbol(%r0_idx) + %i)`；無號參數拒絕（wrap 不是 affine）；`symbol × counter` 拒絕（semi-affine）。interface JSON 多 `leading_extent_expr`／`leading_min_expr`（參數的 C 運算式）：shim 用 `leading_extent_expr` 在 run time 算 memref 的 size[0] 與 snapshot／verify 的元素數，用 `leading_min_expr` 把最小 row 為負的呼叫（任何一個 array，讀的或寫的）交給 CPU（2026-10-20） |
| 辨識（`AffineMatmulAnalysis.cpp`） | access map 有 symbol → failure | `separateSymbols`：每個 result 拆成 loop 部分 + symbol 部分（+ 常數），symbol 部分非線性（symbol×dim、floordiv）→ failure；loop 部分照舊辨識；symbol 部分存成 `LogicalMatmul::symbolicA/B/C`（0 dim、S symbol、每個 memref 維度一個 result）+ `symbolsA/B/C` |
| view（`AffineMatmulToSystolic.cpp`） | `std::array<int64_t,2> offsets` 靜態 subview | `LogicalOperand::symbolic`：有 symbol 的維度以 `arith` 運算算出 index 值，`memref.subview %C[%off, 0] [8, 16] [1, 1]` → `memref<8x16xf32, strided<[16, 1], offset: ?>>`；有 symbol 的維度不做靜態邊界檢查（與動態 extent 同一條規則：nest 自己的保證，tile 不碰 nest 沒碰的元素）；A×A 的 `sameView` 連 symbol 一起比 |
| lowering（`TileMatmulForFpga.cpp`） | root 要靜態 offset | `isExclusiveBufferView` 收任何 row-major（stride[1]=1）的 restrict writable to_tensor；同步路徑用 subview + memref.copy（layout 任意）；非同步路徑 offset 動態時 `memref.extract_strided_metadata` 讀 descriptor，指標 = aligned base + (offset + o0·stride0 + o1)·4、`ldc` = stride0（i32 值）——參數的實際值一路到 runtime，沒有截斷、沒有常數替代；row stride 不是靜態且介於 row 長度與 INT32_MAX 之間的 view（反向的 row、單一 row、run-time stride）每個 GEMM 先 stage 到一塊連續 buffer（第 6 節，2026-10-20） |
| runtime | 指標 + ldc | 不變 |

同一個編譯出的函式可以用任何讓每個 access 落在指標之後（最小 row ≥ 0）的 offset 呼叫（`dynoffset.c`：
同一函式 0／8／16／24／5 等呼叫，每次 verify 的元素數 `(r0 + 8) × 16` 不同）；最小 row 為負的呼叫
（指標指在 array 中間、`C[r0 + i][j]` 而 r0 < 0，合法的 C）照原樣在 CPU 上跑：kernel 的 view、shim
的 snapshot／還原／verify 都從指標指的元素開始（`SystolicCC/negoffset.c`）。邊界 tile、K fold（40 = 16+16+8）、access stride（`C[2i + r0]`）、
兩個 GEMM 在同一函式都走完整路徑；仍拒絕的：A 與 C 是同一 memref 的 view（「A may overlap C」，不
因 offset 運算式不同就當成不重疊）、無號參數、`off + i*ldc`（symbol × counter）、符號化的 loop 邊界。

**GEMM 之間的順序**：front end 在每個 nest 後面立刻 `materialize_in_destination`，它是結果的第一
個 reader，所以同一函式的兩個 nest 一前一後各自 wait。lowering 另外把「後面的 GEMM 的 view 可能
碰到前面 GEMM 寫的 buffer」（同一 root memref、不是兩個靜態不相交的 box——動態 offset 的 view 一律
視為可能重疊）當成前者的 reader：手寫 IR 把 write-back 延後也保住記憶體依賴；靜態不相交的 view 兩
個 GEMM 同時在飛（`affine_to_fpga_two_gemms.mlir`）。另一個修掉的 bug：直接用 to_tensor 的 memref
（第 10 節）曾把「後一個 GEMM 的 A 是前一個 GEMM 的結果」的 SSA 依賴也跳掉，`fpga_lowering_async.mlir`
的 `@dependent` 抓到——現在只對非本 lowering 結果的 to_tensor 走捷徑。

### 11.3 證據（co-sim，K_MAX 16／16，depth 4；板子 NOT RUN）

| 執行 | 結果 |
|---|---|
| lit | 86/86（新：`affine_gemm_symbolic_offset.mlir`、`affine_to_fpga_symbolic.mlir`、`affine_to_fpga_two_gemms.mlir`、`SystolicCC/async.c`、`SystolicCC/dynoffset.c`） |
| `check-e2e-config` | 107/107 |
| `runtime-async-test --mode all`（11 個 mode：A B C D X V S Q M P F） | PASS |
| mode P | 20 submit 後第一次 poll：submitted 20、done 0；`wait_for(1 ms)` -20；wait 後 submit -17；poll 到 done（queued → staged → in_flight → done）；release 0、再 poll -17；有 tile 在飛時 release -21；最長 poll 10.5 µs／59.6 µs |
| mode F | 收下 10 筆後鏈路失敗：wait -4、completed 0、failed 10、之後 submit -14、tile_poll 全 failed/-4、C 未被寫、鏈路 3099 B、begin NULL/-14、close 後新 GEMM PASS |
| mode S 64x64x64（640 tiles，threaded） | 16 筆在第一筆 completion 前、622 筆排在在飛的後面、568 筆等 credit、557 筆 submit 等 host queue；host trace `--strict` 過（waited_for_previous 0） |
| `run_e2e.sh --backend cosim --quick` sync／async | 26/26、26/26（注入失敗 -6／-4 如預期） |
| `dynoffset.c` on cosim（`systolic-cc --runtime board`，預設非同步） | 12 個 GEMM、91 tiles 全在 FPGA、11 次 verify 逐位元、輸出與 clang 相同；`--submission sync` 同 |
| `run_board.sh --board-is-free`（cosim）program／faults／fallback／dynoffset／async | 5/5 PASS、0 fallback |

### 11.4 量測（同一個 co-sim、同一組輸入；wall-clock 是 Verilator 的速度，不是板子）

| 問題 | 路徑 | tiles | 第一筆 completion 前的 descriptor | 等 host queue 的 submit | submit 時間 | 鏈路寫／讀（thread） | wait | 總 wall | RTL makespan（拍） |
|---|---|---|---|---|---|---|---|---|---|
| 16x16x16 | `--sync` | 10 | — | — | — | — | — | 2.415 s | 58 630 |
| 16x16x16 | 預設（async） | 10 | 10 | 0 | 3.8 ms | 0.11 / 3.9 ms | 2.20 s | 2.208 s | 7 002 |
| 13x11x37（邊界 tile、K fold） | `--sync` | 18 | — | — | — | — | — | 4.337 s | 111 063 |
| 13x11x37 | async | 18 | 14 | 0 | 0.22 ms | 0.21 / 7.4 ms | 3.97 s | 3.974 s | 49 743 |
| 32x32x64 | `--sync` | 148 | — | — | — | — | — | 35.74 s | 953 410 |
| 32x32x64 | async | 148 | 16 | 65（15.2 s） | 1.7 ms | 2.0 / 61.7 ms | 17.8 s | 33.01 s | 847 313 |
| `runtime-async-test --mode S` 32x32x32 | async | 80 | 16 | 0 | 最後一筆 submit 0.67 ms 回來 | | | 17.7 s | |
| `--mode M` 三個 GEMM 同時 | async | 242 | 16 | 159 | | | | 53.6 s | GEMM 2 在 159/242 完成時已驗證 |
| `dynoffset.c`（12 GEMM、91 tiles） | sync／async | 91 | | | | | | 23.16 s／20.52 s | |

讀法：submit 本身只要打包（µs 級），鏈路寫入全在 thread 上（ms 級）；wait 是等硬體；co-sim 的
wall-clock 幾乎都是 Verilator 跑 RTL 的時間，所以 sync 與 async 相差不大（8–10%），而 RTL 端的
makespan（descriptor 連續到達、queue 被餵滿）小 1.1–8 倍——板上的 wall-clock 才會反映這個差距，
**沒有量**。

### 11.5 限制

- 板子：NOT RUN（bitstream、`run_e2e.sh --backend board`、`run_board.sh`、dynoffset 真板）。
- context 的 job queue 在這條鏈路上最多 2 個 job（第 10.3 節的結論不變）。
- 同一 memref 的兩個動態 offset view 在同一函式 → 一律序列化（lowering 無法證明不相交）；front end
  的 nest 本來就逐個 write-back。
- 辨識器的 logical coordinate 是所有 operand 共用的：只有 C 有常數 offset（`C[i+1][j] += A[i][k]…`）或
  只有 C 有 stride 仍不被辨識（與 symbol 無關的既有邊界）。
- symbol 只能是加法項（常數係數）；symbol × counter、symbol 在 loop 邊界、無號參數：拒絕。
- run time 的最小 row 為負（access 落在 array 參數的指標之前）的呼叫不 offload，在 CPU 上照原樣跑
  （shim 在任何 local 之前檢查 `leading_min_expr`，讀的與寫的 array 都算）。內層維度的 symbol
  offset（`C[i][c0 + j]`）沒有 run-time 範圍檢查：值必須落在該維度內，跟 ISO C 對 array 下標的要求
  一樣（辨識器的 in-bounds 假設）。
- 軟體模型的非同步 API 在 poll／wait 時才算（沒有 thread）；它是契約的模型，不是硬體。
- `fpga_tile_submit` 在 host queue 滿時仍會等（等空位，不是等 job）；沒有「立即回 EAGAIN」的變體。

## 8. 檔案

| 階段 | 檔案 |
|---|---|
| 0 | `runtime/dpti_ftdi.cpp`（`SYSTOLIC_DPTI_LATENCY_MS`、`SYSTOLIC_DPTI_READ_CHUNK`、`dpti_ftdi_read_some_session`）、`runtime/fpga_matmul_tiled.{h,cpp}`（計時、`SYSTOLIC_DPTI_SETTLE_US`、completion record、slab packer／device table 的輸出）、`test/e2e/run_e2e.sh --timing`、`tools/e2e_timing.py` |
| 1 | `test/e2e/cosim/dpti_protocol_check.svh`、`fp_ieee_dpi.cpp`、`tb_dpti_cosim.sv`（檢查器）、`build.sh`（預設 working tree、`COMPLETION_HEADER`）、`rtl/.../dma/tb/mig_axi_model.sv`（`+w_stall`）、`tb_dpti_direct.sv`、`run_dpti_direct.sh` |
| 2 | `rtl/.../dma/dpti_bus_arbiter.sv`、`systolic_dma_top.sv`、`tb/xil_stubs.sv`、`tb/run_top_sim.sh`、`tb/tb_dpti_bus_arbiter.sv`、`dma_top_build.tcl` |
| 3 | `runtime/fpga_async_runtime.{h,cpp}`、`runtime/CMakeLists.txt`、`test/e2e/runtime_async_test.cpp`、`test/e2e/CMakeLists.txt` |
| 4 | `lib/Systolic/Transforms/SystolicBinarySearchTileMatmulPass.cpp`、`TileMatmulForFpga.cpp`、`SystolicPipelines.cpp`、`include/Systolic/Passes.h`、`tools/systolic-matmul/systolic-matmul.cpp`、`test/Systolic/binary_search_ids.mlir`、`fpga_lowering_async.mlir`、`bad_fpga_lowering_async.mlir`、`test/e2e/run_e2e.sh --mode`、`e2e_verdict.py --mode/--header` |
| 5 | 這份文件、`docs/E2E.md`、`REPRODUCE.md` |
| 11 | `runtime/fpga_async_runtime.{h,cpp}`（transfer thread、poll／wait_for／release、時間拆分）、`runtime/offload/soft_runtime.c`（非同步 API 的模型 + 同步 wrapper）、`tools/systolic-cfront/systolic-cfront.c`（整數參數 = affine symbol、extent 運算式）、`lib/Systolic/Transforms/AffineMatmulAnalysis.cpp`（`separateSymbols`）、`AffineMatmulToSystolic.cpp`（動態 offset 的 view）、`TileMatmulForFpga.cpp`（動態 layout 的指標、GEMM 間的記憶體順序）、`tools/systolic-cc/systolic-cc.c`（`--submission async` on soft、`leading_extent_expr`）、`test/e2e/runtime_async_test.cpp`（P／F）、`test/SystolicCC/{async,dynoffset}.c`、`test/Systolic/affine_{gemm_symbolic_offset,to_fpga_symbolic,to_fpga_two_gemms}.mlir` |
| 10 | `lib/Systolic/Transforms/TileMatmulForFpga.cpp`（strided 根、segmentation）、`runtime/fpga_async_runtime.{h,cpp}`（burst 放行、計數器、trace）、`runtime/offload/board_stats.c`、`tools/systolic-matmul/systolic-matmul.cpp`（預設非同步、`--sync`）、`tools/systolic-cc/systolic-cc.c`（`--submission`）、`test/e2e/runtime_dispatch_test.cpp`（`COMPLETION_HEADER=`）、`test/e2e/runtime_async_test.cpp`（S／Q／M）、`test/e2e/run_e2e.sh`、`test/e2e/affine_c/run.sh`、`test/e2e/systolic_cc/run_board.sh`、`test/e2e/test_e2e_config.sh`、`tools/cosim_overlap_evidence.py`、`test/Systolic/affine_to_fpga_pipeline_async.mlir`、`fpga_lowering_async_split.mlir`、`bad_fpga_lowering_async.mlir` |

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
