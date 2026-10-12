# K_MAX_8X8 與 K_MAX_4X4

**2026-10-12。** 1x8x8 + 3x4x4 的 tile-pipelined fleet 有兩個獨立的 reduction 容量：
8x8 的 `K_MAX_8X8` 與每顆 4x4 的 `K_MAX_4X4`。這份文件記錄它們在 compiler、FPGA
lowering、runtime、DPTI/DMA 協定、RTL 與 build 裡怎麼走、哪些寫死的值被拿掉、還剩
哪些真正的限制、和 golden（1a22f76）相比哪些行為是刻意改的，以及怎麼重現驗證。

## 1. 契約

- 兩個值各自是 `1..16376` 的任何整數，不必相等、不必是 2 的冪，任何一層都不會把
  它們捨入或合併成一個。上限的理由見第 4 節。寫法一律是十進位、不帶前導零
  （bash 與 Tcl 的算術把 `016` 當八進位，C++ 與 Verilog 當十進位，所以乾脆全部拒絕）。
- 對 device d，GEMM 的 reduction K 折成 `I(d) = ceil(K / K_MAX(d))` 次 invocation，
  第 j 次涵蓋 `[j·K_MAX(d), min(K, (j+1)·K_MAX(d)))`，所以每次 `1 <= k <= K_MAX(d)`；
  各次結果依 `c_in` 串起來累加（runtime 把每次的 tile 加回 C）。
- 三個 K 分開：IR 宣告的 `k_max`（`systolic.device`）、載入的 bitstream 的實體
  `K_MAX`（`fpga_config.txt`）、這次 invocation 的 `k`。FPGA lowering 要求前兩者相等
  ——對每一顆屬於 bitstream 的 device，不論它最後有沒有分到 tile（沒分到的也參與了
  分解的成本計算）——第三者在 `1..k_max` 之內；任何一個不成立都報錯，不默默接受。
- 硬體不接受 k 超出那顆加速器 `1..K_MAX` 的 descriptor（不 accept、`err_job_k`
  鎖進 `any_err`／led[6]），不會讓 feeder 讀到 buffer 外面。

## 2. 稽核：寫死的 K_MAX

### 2.1 實際執行路徑上的（已改）

| # | 位置 | 原本 | 後果 | 現在 |
|---|---|---|---|---|
| 1 | `lib/Systolic/Transforms/TileMatmulForFpga.cpp` | 四顆 device 的實體深度都寫死 `kDepth = 16`；只檢查 `k <= 16`；scratch 寫死 8x16／16x8 | 宣告的 `k_max` 比硬體淺時不報錯；任何非 16 的 bitstream 都無法 lower | 實體 K_MAX 依幾何由 `phys-k-max-8x8`／`phys-k-max-4x4` 傳入（預設 16／16，範圍 1..16376）；每顆屬於 bitstream 的 device 都要宣告 `k_max == 實體`（沒分到 tile 的也檢查）；scratch 依實體 K_MAX 配置 |
| 2 | `runtime/fpga_matmul_tiled.cpp` | `SCHED_K_MAX = 16`（所有 device 共用）；slab 是 `std::array<1024>`；descriptor `k = 16`；`K` 只許 `1..16` | 4x4 與 8x8 不可能有不同容量 | 每顆 device 用自己的 K_MAX（`SYSTOLIC_FPGA_CONFIG`，數值十進位、無前導零，否則 -16）；slab 長度＝最深幾何的 `ceil(K_MAX/8)·512` byte；descriptor `k` = 這次 invocation 的 k（variable-K，見第 6 點；先前是 `K_MAX(d)`）；`k < 1` 或 `k > K_MAX(d)` 回傳 -10 |
| 3 | `runtime/dpti_ftdi.cpp` | MEM_WRITE 只收 `length == 1024` | 換了 K_MAX 就送不出去 | 收任何正的 16 byte 倍數（長度由呼叫端依 bitstream 決定） |
| 4 | `tools/systolic-matmul/systolic-matmul.cpp` | fleet 四顆都寫死 `k_max = 16`；invocation 下限 `ceil(K/16)` | compiler、runtime、bitstream 三者各自假設 16 | fleet 的 `k_max`、lowering 的實體 K_MAX 都取自 runtime 讀到的同一份設定 |
| 5 | `include/Systolic/CostModel.h`、`CostModel.cpp` | `ArrayConfig::kMax` 預設 1；`kMax <= 0` 時回傳 0 cycle | 沒宣告 `k_max` 的 device 被當成每一步一次 invocation；`k_max = 0` 被默默算成 0 cycle | 預設 0＝沒宣告＝整個 K 一次 invocation（與 binary-search pass 一致）；`k_max < 1` 由 `DeviceOp` verifier 拒絕 |
| 6 | `lib/Systolic/Transforms/SystolicOptimizer.cpp` | 每個 spatial tile 的成本是 `C(g) = 2g-2+H`，與 K 無關 | `K_MAX_8X8 != K_MAX_4X4` 時，一種幾何每個 tile 要比另一種多跑幾次 invocation，分解卻當成一樣快 | tile 成本 `I(g)·C(g)`，`I(g) = ceil(K/k_max(g))`；兩者相等時決策與原本逐 cycle 相同 |
| 7 | `SystolicBinarySearchTileMatmulPass.cpp` | 第 j 次 invocation 從 `start·I + j·C` 開始 | 只在所有 device 的 I 相同時才是 optimizer 的排程 | tile 的 I 次 invocation 在 optimizer 排的 `[start, end)` 裡連續執行；同一種幾何宣告不同 `k_max` 時報錯 |
| 8 | `rtl/.../dma/systolic_accel_context.sv` | `RX_SHIFT`／`RXW_SHIFT` 要求 slab 是 2 的冪（`$fatal`）；位址用 `<< RX_SHIFT`；`K_W = $clog2(K_MAX)`（K_MAX = 1 時為 0）；傳輸量 `job_k·64` byte | K_MAX 必須是 2 的冪；`k % 8 != 0` 時最後一個 window 的 B 半段根本沒被讀進來（舊 buffer 內容被拿去算，沒有任何錯誤） | slab = `ceil(K_MAX/8)` 個 window；位址用乘法（2 的冪時合成結果與 shift 相同）；`K_W >= 1`；傳輸量 = `ceil(k/8)` 個完整 window（acceptance 時算好存成暫存器）；`>= K_MAX` 的補零步在 buffer 前丟掉 |
| 9 | `rtl/.../dma/dma_operand_writer.sv` | `waddr` 寬 `$clog2(K_MAX)`；`WIN_W = K_W - 3`（K_MAX ≤ 8 時是 0 寬向量）；`LAST_BEAT` 依 `K_MAX·2·N` | 只能是 ≥ 16 的 2 的冪 | 依 slab（整數個 window）解碼；輸出完整的 step（不截斷，免得補零步蓋到真的格子）；K_MAX 是 8 的倍數時與原本的 `{win, koff}` 相同 |
| 10 | `rtl/.../core/operand_buffer.sv` | `K_W` 預設 `$clog2(K_MAX)` | K_MAX = 1 時為 0 寬 | 下限 1；模擬時每拍檢查讀寫位址 `< K_MAX` |
| 11 | `rtl/.../dma/systolic_dma_top.sv` | MEM_WRITE 的 `PAYLOAD_BYTES` 與寫入 beat 數 = `K_MAX_8X8·64` | `K_MAX_4X4 > K_MAX_8X8` 時 4x4 的 slab 根本 stage 不進來 | `HOST_SLAB_BYTES` = 兩種幾何裡最深的 slab（16／16 時仍是 1024） |
| 12 | 同上 | job 的 k 只在模擬裡 `$fatal` 檢查 | 合成後 k > K_MAX 照跑，讀 buffer 外面 | 硬體拒絕（見第 1 節）；模擬印 `JOBREFUSED` |
| 13 | `rtl/.../dma/dma_top_build.tcl` | GOLDEN 表只有 K8 ∈ {16, 32, 256}，其他值直接 `error`；註解說 RTL 要求「2 的冪、至少 16」 | 只能建三種 K8 | 沒有 golden 列的 K8 照樣 build（只有 bring-up `read` 的 checksum 自檢沒常數，led[4]/[5] 不亮），驗 `1..16376`、十進位無前導零；成功後寫 `fpga_config.txt` |
| 14 | `test/e2e/cosim/`、`test/e2e/run_e2e.sh` | cosim tb 寫死 `.K_MAX(16)`；build.sh 沒有 K 參數；斷線檢查寫死 1057 byte | 只能模擬 16／16 | `K_MAX_8X8`／`K_MAX_4X4` 環境變數（co-sim 1..1024，見第 4 節）、`fpga_config.txt`、byte 數由 slab 長度算；export 的 `SYSTOLIC_FPGA_CONFIG` 與 co-sim 不符時直接停；壞掉的設定檔必須被拒絕（run_e2e.sh 第 0 步） |

### 2.2 檢查過、不需要改的

- `SystolicSelectDevicePass.cpp`、`SystolicScheduleOverlapPass.cpp`、
  `SystolicCostAnalysisPass.cpp`：每顆 device 的 `k_max` 讀進 `CostModel::ArrayConfig`，
  沒有寫死的值；它們對 K 的處理全在 CostModel（表 #5）。
- `dma_engine_multi.sv`、`dpti_mem_write_decoder.sv`／`_engine`／`_cdc_engine`、
  `dpti_command_frontend.sv`：長度與 beat 數都是參數（`PAYLOAD_BYTES`、`TOTAL_BEATS`、
  descriptor 的 16-bit beat 數），沒有 1024 或 16 的假設。
- `tile_feeder.sv`：只讀 `k < k_dim <= K_MAX` 的深度；位址寬度跟著 `K_W`。

### 2.3 只影響 instrumentation 的（保留，不是容量限制）

- `systolic_accel_context.sv` 的 `expected_wr_chk_for_k`（k = 16、32 才有常數）：只餵
  `wr_match`／led[4]，是 bring-up seed pattern 的 checksum；其他 k 時燈不亮，資料路徑不受影響。
- `dma_top_build.tcl` 的 GOLDEN 表：只給 `read` 模式的 bring-up 自檢。
- `tb_systolic_dma_top.sv` 的 `expected_wr_chk` 表（16、32、256）與「K=32 gate 實際跑 k=16」：
  regression gate 的既有行為，維持不動。

### 2.4 Legacy（不在這條路徑上，沒動）

- `runtime/lib/nexys_rk_8x8/`：`SYS_K_MAX = 64`，HLS rk 8x8 bitstream 的 UART runtime。
- `runtime/lib/{arty_4x4,nexys_dual_4x4,nexys_fold_8x8,gemm}/`：其他 UART 板子的
  runtime；compiler 的 `--conv2d-to-fpga` 產生的 `fpga_conv2d_im2col_*` 呼叫就是
  `runtime/lib/gemm/` 的（Arty 4x4），不經過 DPTI 這條路。
- `rtl/multi_200t/fold_pipelined/`：UART fold 設計與 `build_kmax.tcl`（單一 K_MAX）。
- `hls/`、`eval/` 的掃描與模型腳本、`tile_pipelined/tools/*.py`（UART 工具）。
- `rtl/.../dma/systolic_dma_core.sv`：在 `dma_top_build.tcl` 的檔案清單裡，但沒有被
  `systolic_dma_top` 實例化。
- `dma_bringup_top.sv`／`bringup_build.tcl`（bring-up 3a）。
- v2 beat-wide operand path（`USE_V2=1`）：沒有一般化，限制見第 4 節第 8 點。

## 3. 一組值怎麼貫穿每一層

| 層 | 從哪裡拿 | 記錄在哪 |
|---|---|---|
| 合成 | `vivado -mode batch -source dma_top_build.tcl -tclargs build v1 <K8> <K4>` | `.bit` 旁的 `fpga_config.txt`（K、fleet、variant、`HOST_SLAB_BYTES`、bitstream sha256、git revision、是否 dirty、WNS/WHS） |
| RTL 模擬 | `run_top_sim.sh` 第 5、6 個參數；`run_kmax_sim.sh <K8> <K4> [v1\|v2]`；`test/e2e/cosim/build.sh`（環境變數 `K_MAX_8X8`、`K_MAX_4X4`） | cosim：`fpga_config.txt` |
| runtime | `SYSTOLIC_FPGA_CONFIG=<fpga_config.txt>`（沒設＝封存那顆的 16／16） | `fpga_runtime_get_config()`；`systolic-matmul` 會印出來 |
| compiler | `systolic-matmul` 用 runtime 的那組值產生 fleet 的 `k_max`、傳給 lowering 當實體 K_MAX；`systolic-opt` 用 `--lower-systolic-matmul-tile-to-fpga="phys-k-max-8x8=N phys-k-max-4x4=M"` 或 `--systolic-fpga-pipeline="..."` | IR 的 `systolic.device ... k_max = N` |

## 4. 剩下的實體與協定限制

1. **K_MAX ≥ 1。** 0 個 reduction step 的 buffer 沒有意義；`DeviceOp` verifier、
   lowering 的選項、build script、RTL elaboration、runtime 都拒絕。
2. **K_MAX ≤ 16376（協定）。** 一個 slab 是 `ceil(K/8)` 個 window、每個 32 beat，DMA
   descriptor 的 beat 數是 16 bit（`BEAT_W = 16`），所以 `ceil(K/8)·32 ≤ 65535`，
   也就是 `ceil(K/8)·8 ≤ 16376`。RTL 在 elaboration 拒絕（`systolic_accel_context`；
   `dma_operand_writer` 自己只檢查 beat 編號放得進 16 bit，那是 K ≤ 16384），FPGA
   lowering 的選項、build script、cosim build、runtime 也檢查同一個上限。
   `run_kmax_elab.sh` 確認 16376 可以 elaborate、16377 被拒絕。
3. **BRAM（資源，由 Vivado 判斷）。** 每個 context 有 A、B 各兩組（ping-pong）、每組
   `N_ARRAY` 個 bank、每個 bank `K_MAX` 個 32-bit 字：8x8 是 `32·K_MAX_8X8` 字，每顆
   4x4 是 `16·K_MAX_4X4` 字。`ram_style = "block"` 下每個 bank 至少一顆 RAMB18（512 深），
   所以 K ≤ 512 時 BRAM 數與 K 無關；更深才線性增加。上限是 xc7a200t 的 BRAM 與整個
   設計其他部分的總和，沒有在這次跑 Vivado 驗證。
4. **wire format 以 8 步為一個 window。** host↔RTL 的 operand slab 一直是 window 為
   單位（A 半段、B 半段各 `8·N` 字）。K 不是 8 的倍數時，最後一個 window 的
   `ceil(K/8)·8 − K` 步是補零（RTL 讀進來後在 buffer 前丟掉），多付一點傳輸，
   不影響正確性。k 是每次 invocation 自己的深度（第 6 點），所以只有最後一個 window
   有補零，不會補到 K_MAX。
5. **MEM_WRITE 長度每顆 bitstream 固定。** RTL 的 `dpti_mem_write_decoder` 只收
   `HOST_SLAB_BYTES`；host 送別的長度會 `err_mem_length`。所以 runtime 的設定必須是那顆
   bitstream 的 `fpga_config.txt`——這是「bitstream 與 host 不符時不默默接受」的那條線。
   DPTI 協定本身（header 裡就有長度欄位）沒改。
6. **descriptor 的 k 是這次 invocation 的 k（variable-K，2026-10-12 起）。** K_MAX(d) 是
   operand buffer 的容量；runtime 送 `job.k = k_j`（`1 <= k_j <= K_MAX(d)`），RTL 讀
   `ceil(k_j/8)` 個 window、餵 `k_j` 步。b971b79 的 RTL 在 Verilator 中逐 PE 計數驗證過
   （每個 PE 的 MAC 次數 = job.k，`docs/COSTMODEL.md` 第 4 節），co-simulation 也逐
   invocation 比對 compiler IR、runtime trace 與 RTL 的 step 數（`test/e2e/check_variable_k.py`）。
   先前 runtime 一律送 `k = K_MAX(d)`，理由是封存的 bitstream 是哪一份 RTL 建的，repo 裡
   判斷不了（`docs/E2E.md` 指 tag `tile-pipelined-1x8x8-3x4x4-20261009`，它的 RTL 不論 k
   都讀整個 slab；golden commit 1a22f76 的 RTL 讀 `k·64` byte，`k % 8 != 0` 時讀不到最後
   一個 window 的 B 半段），而 16／16 時兩者對 `k = 16` 都正確。這個不確定性還在：
   **variable-K 的 descriptor（k < 16）在封存的 bitstream 上沒有板測，也沒有用它的 RTL
   模擬過**（FPGA Board Regression：NOT RUN）。
7. **同一種幾何的 device 共用一個 K_MAX。** 硬體就是這樣參數化的（所有 4x4 context
   都用 `K_MAX_4X4`）；binary-search 分解對一種幾何只算一個成本，所以 compiler 要求
   同幾何宣告相同的 `k_max`。
8. **v2 operand path 要求 K_MAX 是 8 的倍數，而且 ≥ 16。** beat-wide 寫入一次寫 A 的
   4 步（cyclic buffer，`K_MAX/4` 列）或 B 的 4 lane，沒有 v1 那種補零步的丟棄，所以
   只收完整的 window；它的 writer（`dma_operand_writer_v2`）從 `$clog2(K_MAX) − 3`
   個位址 bit 解出 window 編號，K_MAX = 8 時是 0 個。這次沒有把它一般化：RTL
   elaboration、build script、cosim build 與 `run_kmax_sim.sh` 都拒絕其他值。在這個
   範圍內不必是 2 的冪（`run_kmax_sim.sh 24 16 v2`、`16 40 v2` 有測）。v1 才是封存
   bitstream 與 cosim 用的路徑。
9. **模擬環境自己的範圍（不是硬體限制）：** cosim 的 DDR model 128 KiB，runtime 把
   slab 放在 0x10000，所以 cosim 的 K ≤ 1024（`cosim/build.sh` 直接拒絕更大的值）；
   `tb_kmax_hetero` 的位址配置是 K ≤ 56。
10. **時序沒驗。** 這次沒有 Vivado。RTL 新增的邏輯：acceptance 路徑上對 job k 的一個
    比較器（結果進 `accept_q` 暫存器）、acceptance 時的 `(k+7) & ~7` 一個加法器（結果
    存成暫存器）、K 不是 8 的倍數時 operand 寫入前的一個 step 比較。K 是 8 的倍數時
    最後一項是常數。WNS/WHS 要等實際 implementation 才知道。
11. **四顆陣列同時算，取決於送 job 的時機，不取決於 K_MAX（效能，不是正確性）。**
    共用的 read engine（`dma_engine_multi`，沒改）在只有一個 slot 有 descriptor 時發
    16-beat burst，晚幾個 cycle 才到的 fill 排在後面；不同 window 數的 fill 也不同時
    結束。所以 memory ready 之後一個接一個送的四個 job 不一定同時進 COMPUTE——四個
    k = 16 的 job 這樣送，golden（1a22f76）與新 RTL 逐 cycle 相同，8x8 比 4x4 早約 190
    cycle 填完。四個 fill 同一拍開始、window 數相同時，四顆陣列在任何 K_MAX 組合下都
    同時算（`tb_kmax_hetero` wave 1 檢查四路 COMPUTE 重疊）。runtime 本來就一次只送一個
    invocation（`docs/E2E.md` 第 6 節），不受影響。

## 5. 和 golden（1a22f76）相比刻意改變的行為

16／16 時 runtime 送上線的 byte、descriptor、RTL 對 `k` 為 8 的倍數的 job 的行為，
都和 golden 相同（RTL gate 逐 cycle 一致，cosim 的 byte 數一致）。以下是刻意的改變：

- **FPGA lowering 要求宣告的 `k_max` 等於實體 K_MAX。** golden 在預設實體 16 下接受
  `k_max < 16`（例如 8）的 fleet 並照跑；現在報錯——那份 IR 描述的是另一顆 bitstream。
  沒分到 tile 的 device、沒宣告 `k_max` 的 device 也一樣。
- **CostModel：沒宣告 `k_max` 的 device 算一次 invocation**（golden 算 `k` 次）。只影響
  `--systolic-select-device`、`--systolic-cost-analysis`、`--systolic-schedule-overlap`
  對沒宣告 `k_max` 的 IR 的結果（不在 FPGA 路徑上；既有測試結果不變）。
  `k_max <= 0` 現在在 parse 時就被 `DeviceOp` verifier 拒絕。
- **RTL：`k % 8 != 0` 的 descriptor 讀整數個 window**（golden 讀 `k·64` byte、漏掉
  最後一個 window 的 B 半段而算錯）；這種 job 的 fill cycle 數因此不同。runtime 不送
  這種 k。
- **RTL：k 超出 `1..K_MAX` 的 descriptor 在硬體上被拒絕**（golden 只在模擬裡
  `$fatal`）。
- **設定值必須是十進位、不帶前導零**（runtime、build script、cosim）。

## 6. 重現

```bash
# compiler：31 個 lit test（新增 kmax_heterogeneous、kmax_fpga_lowering、
# bad_kmax_device、bad_kmax_fleet、bad_kmax_lowering、cost_kmax、select_device_kmax）
cmake --build build --target check-systolic

# RTL：原本的六個 gate（必須維持 PASS；它們寫進 tree 裡被追蹤的 sim_out_*，
# 想保持 tree 乾淨就在 RTL 的副本裡跑）
cd rtl/multi_200t/tile_pipelined/dma/tb
./run_top_sim.sh v1 16 1 1 && ./run_top_sim.sh v1 16 2 1
./run_top_sim.sh v1 32 1 1 && ./run_top_sim.sh v1 32 2 1
./run_top_sim.sh v1 16 1 1 32 16 hetero_concurrent
./run_top_sim.sh v1 16 1 1 32 16 hetero_full_concurrent
cd -

# RTL：operand writer 對 host 打包方式，任意 K_MAX（Icarus）
cd rtl/multi_200t/tile_pipelined/dma
for K in 1 2 3 7 8 9 12 15 16 17 24 31 33 64 100 255 256 1000; do
  iverilog -g2012 -Ptb_operand_writer_kmax.K_MAX=$K -o /tmp/w.out \
      tb/tb_operand_writer_kmax.sv dma_operand_writer.sv && vvp /tmp/w.out | tail -1
done
cd -

# RTL：哪些 K_MAX 能 elaborate、哪些被拒絕（Verilator --lint-only）
rtl/multi_200t/tile_pipelined/dma/tb/run_kmax_elab.sh

# RTL：整個 top、binary32、每個元素對 reference（一個組態；v2 可選）
rtl/multi_200t/tile_pipelined/dma/tb/run_kmax_sim.sh 17 7
rtl/multi_200t/tile_pipelined/dma/tb/run_kmax_sim.sh 24 16 v2

# 全部：每個組態的 RTL bench + co-simulation（36 組固定案例 + K_MAX 邊界案例）
test/e2e/run_kmax_matrix.sh                 # 16:16 16:8 8:16 24:12 17:7 1:1
test/e2e/run_kmax_matrix.sh 24:12           # 只跑一組

# 新的 host 對舊 RTL（16／16）：bitstream tag 的 RTL、golden commit 的 RTL
COSIM_BUILD_DIR=build/cosim_tag test/e2e/cosim/build.sh
test/e2e/run_e2e.sh --backend cosim --cosim-dir build/cosim_tag --out build/e2e/compat_tag --fixed-only
COSIM_BUILD_DIR=build/cosim_golden RTL_REV=golden-tile-pipelined-1x8x8-3x4x4 test/e2e/cosim/build.sh
test/e2e/run_e2e.sh --backend cosim --cosim-dir build/cosim_golden --out build/e2e/compat_golden --fixed-only

# E2E harness 的設定處理（只在 host 端，不需要 FPGA）
cmake --build build --target check-e2e-config

# 板子（燒好用 K8/K4 build 的 bitstream 之後；harness 沒有預設設定，見 docs/E2E.md 第 3 節）
export SYSTOLIC_FPGA_CONFIG=<impl_1 目錄>/fpga_config.txt
test/e2e/run_e2e.sh --backend board --check-config   # host 端：解析與檢查，不送任何東西
test/e2e/run_e2e.sh --backend board                  # 36 組固定案例 + K_MAX 邊界案例
test/e2e/run_e2e.sh --backend board --kmax-cases     # 只跑 K_MAX 邊界案例
```
