# 端到端：Linalg → Systolic → FPGA → C

`./build/bin/systolic-matmul` 從 stdin 讀 `M N K`、`A[M][K]`、`B[K][N]`，
把這個 GEMM 走完整條 compiler，在 1x8x8 + 3x4x4 的 FPGA 上算出 `C[M][N]`，
再和整數 CPU reference 逐元素比對。中間沒有任何一步在 CPU 上算 C：CPU 只做兩件事——
把同一個輸出 tile 的多次 invocation 結果加起來（bitstream 沒有 C-in port），以及算
拿來比對的 reference。

```text
user input (stdin, 整數)
  -> matrix representation          binary32，FPGA 看到的就是這個
  -> linalg.matmul                  為這個 M x N x K 產生的 module + 板子的 fleet
  -> systolic.matmul_tile           --systolic-binary-search-tile-matmul
                                    (decomposition、device、schedule、K 依 k_max 折)
  -> llvm.call @fpga_matmul_tiled_auto_scheduled
                                    --lower-systolic-matmul-tile-to-fpga
  -> LLVM dialect -> JIT            --systolic-runtime-to-llvm, MLIR ExecutionEngine
  -> runtime                        runtime/fpga_matmul_tiled.cpp
  -> DPTI (FT2232H sync FIFO)       runtime/dpti_ftdi.cpp, dpti_runtime.c
  -> FPGA                           systolic_dma_top, tag tile-pipelined-1x8x8-3x4x4-20261009
  -> C[M][N] -> CPU reference -> PASS / FAIL
```

## 1. Build 與執行

```bash
./configure.sh --build                 # 或 cmake --build build
cmake --build build --target check-systolic        # 38 個 lit test

printf '2 3 4\n1 2 3 4\n-1 0 2 -3\n1 0 -1\n2 1 0\n0 -2 3\n-1 1 1\n' \
  | ./build/bin/systolic-matmul --dump-ir=/tmp/e2e
```

`systolic-matmul` 需要 MLIR 的 ExecutionEngine 與 `mlir_c_runner_utils`（MLIR 用
host target 建置時預設就有）；沒有的話 CMake 只會跳過這個工具並印 warning。
runtime 需要 `libftdi1`（`pkg-config libftdi1`）。

輸入格式：先 `M N K`，再 M 列各 K 個整數（A），再 K 列各 N 個整數（B），以空白分隔，
換行不計。從終端機輸入時會提示。

輸入的範圍（超出時以 2 結束並說明原因）：

| 項目 | 範圍 | 為什麼 |
|---|---|---|
| A、B 的元素 | −16777216..16777216（±2^24） | FPGA 用 binary32，這是它能精確表示的整數；CPU reference 因此也不會溢位（\|C\| ≤ K·2^48） |
| M、N | 1..512 | binary-search 分解 512x512 約 20 s，再大增長很快 |
| K | 1..4096 | |
| FPGA invocation 數 | ≤ 16384 | 每次 invocation 在板子上是一次 DPTI 往返（runtime 在 MEM_WRITE 後就固定等 1 ms，再加 USB 的來回），編譯＋JIT 約 2.5 ms、120 KB；編譯前先用下限 `ceil(M·N/64)·ceil(K/max(K_MAX_8X8, K_MAX_4X4))` 擋，Linalg → Systolic 之後再用實際數擋 |

| 選項 | 作用 |
|---|---|
| `--dump-ir=DIR` | 每一層的 IR：`00-linalg.mlir` `10-systolic.mlir` `20-fpga.mlir` `30-llvm.mlir` `40-llvm.ll` |
| `--quiet` | 只印驗證結果 |

| 環境變數 | 作用 |
|---|---|
| `SYSTOLIC_RUNTIME_TRACE=1` | 每次 FPGA invocation 一行（device、tile、job.k、schedule） |
| `SYSTOLIC_DPTI_TRACE=1` | 每個 DPTI 封包（MEM_WRITE、每個 WRITE32） |
| `SYSTOLIC_DPTI_TIMEOUT_MS` | 等結果的上限，預設 5000 |
| `SYSTOLIC_FPGA_CONFIG` | 板上 bitstream 的 `fpga_config.txt`（`K_MAX_8X8`、`K_MAX_4X4`，見第 7 節）。沒設時 runtime 用封存那顆的 16／16（built-in default，輸出會印出來源）；E2E harness 不接受沒設（第 3 節） |

結束碼：0 PASS、1 FAIL（不相等）、2 輸入錯或超出範圍、3 編譯失敗、4 FPGA／runtime
錯誤。runtime 呼叫失敗時直接回報 4，不會把沒算完的 C 當結果比。

比對是整數對整數：FPGA 的 binary32 結果先確認是整數、再轉成 int64 和 reference 比，
不經過 double（double 會把超過 2^53 的 reference 捨入，可能把不同的值當成相等）。

JIT 不做最佳化（IR 與 codegen 都是 -O0）：編出來的函式是一長串 runtime 呼叫與 tile
複製，每次呼叫都要等一次 DPTI 往返（至少 1 ms），最佳化沒有好處，而 -O2 在上千個呼叫的單一 basic
block 上遠超過線性（128x128x128：74 s → 8.7 s）。-O0 的 stack frame 隨 invocation
數成長（每次幾百 byte，量到 323–457 B），所以編出來的函式在一條依 invocation 數配置 stack（每次 1 KB 再加 16 MB）的 thread
上執行。

輸出會列出：輸入、每個 `systolic.matmul_tile`（C 的區塊、k 範圍、device、
start_cycle／est_cycles）、runtime 呼叫數、每個 device 的 FPGA invocation 數、在
FTDI 那層計到的 DPTI byte 數，然後 FPGA 結果與 CPU reference 並排，最後是
`M N K`、不相等的元素數、第一個不相等的 `i j FPGA CPU`、`Verification: PASS/FAIL`。

**精確度**：整數在 binary32 內精確的條件是 `K * max|A| * max|B| <= 2^24`；超過時
程式會先警告，因為陣列的加法順序（16 個 bank 加上歸約樹）和逐項相加不同，fp32
的捨入可能讓 FPGA 和精確整數不同——那是 fp32，不是硬體錯。

## 2. 每一層的契約

### 2.1 Linalg → Systolic：`--systolic-binary-search-tile-matmul`

- spatial decomposition 是 `optimizeSystolicBinarySearch`（沒改）。pass 原本拒絕
  `M != N`，但 optimizer 本來就把 rows／columns 分開 pad，只覆蓋原本的 cells，
  所以這個限制拿掉了。
- 一個 `systolic.matmul_tile` 是一顆 device 的**一次 invocation**。device d 宣告
  `k_max` 時，每個 spatial tile 的 reduction 依**它自己的** `k_max(d)` 折成
  `I(d) = ceil(K / k_max(d))` 次（每次 `1 <= k <= k_max(d)`），用 `c_in` 串起來；
  沒宣告 `k_max` 的 device 維持整個 K（舊行為，舊測試輸出不變）。8x8 與 4x4 的
  `k_max` 可以不同、不必是 2 的冪；同一種幾何的 device 必須宣告相同的 `k_max`
  （optimizer 對一種幾何只算一個成本，跟 H 一樣）。
- optimizer 對一個 spatial tile 的成本是 cost model 的
  `estimateTileCost(K, cfg).sequential()`：I(d) 次 invocation 的
  read + compute + write 加總（`docs/COSTMODEL.md`）；它自己不再有公式。
  `k_max` 較小的幾何每個 tile 比較慢，分解會照這個平衡（例：K_MAX_8X8 = 17、
  K_MAX_4X4 = 7 的 8x16x17 全給 8x8；`test/Systolic/kmax_heterogeneous.mlir`）。
- IR 裡 `est_cycles` 是該次 invocation 的 compute term（`k_exec + R + C - 2 + H`），
  `start_cycle` 是同一顆 device 上前面各次 compute term 的加總；invocation 成本
  不同時（logical 深度的最後一次較短）不平均切分。含 read／write 的 makespan 用
  `--systolic-binary-search-tile-matmul="report=true"` 的 remark 或
  `build/bin/systolic-cost` 看。

### 2.2 Systolic → FPGA：`--lower-systolic-matmul-tile-to-fpga`

每個 tile 變成一次
`fpga_matmul_tiled_auto_scheduled(m, k, n, A, B, C, device_id, start_cycle, est_cycles)`。
合法條件：已指派 device、已排程、`m <= rows`、`n <= cols`、IR 宣告的 `k_max` 等於
載入的 bitstream 那種幾何的實體 K_MAX（pass option `phys-k-max-8x8` /
`phys-k-max-4x4`，預設 16／16；`--systolic-fpga-pipeline` 也收這兩個）、
`1 <= k <= k_max`、fp32。宣告與實體不同時（不論大或小）報錯，不默默接受。
任何一個 tile 不合法就整個 pass 失敗、每個 tile 一條 diagnostic，不會留下半 lower 的
module。

runtime 吃連續、row-major 的 `m x k`、`k x n`、`m x n` tile。lowering 自己在 buffer
層準備：A、B 的 tile 從整個矩陣的 buffer 複製到 scratch tile（只被切成 tile 的函式參數
整個函式共用一個 `to_buffer`）；C 的每個輸出區塊複製到 scratch tile 一次、它所有的 K
invocation 在上面累加、最後複製回去，`insert_slice` 換成 C buffer 的
`to_tensor ... restrict`。scratch 是每個函式三塊 stack buffer（依實體 K_MAX：
`max(rows·K_MAX)`、`max(K_MAX·cols)`、8x8；16／16 時是 8x16、16x8、8x8），
runtime 同步，所以所有 invocation 輪流用。這樣 bufferization 沒有一長串互相 alias 的
tensor 要分析，成本跟 invocation 數成線性（原本 96x96x16 要 34 s、128x128x16 要
208 s；現在 0.3 s、0.7 s），invocation 也不在 heap 上配置任何東西。

`restrict` 等於宣稱沒有別的 tensor 碰得到這塊 buffer，所以 C 的區塊只有在下列條件都成立
時才這樣做：C（`%R`）是只被這條 chain 用到的函式參數，或是同一條 chain 前一個已這樣處理
的區塊的結果；這個區塊的 extract 到 insert 之間沒有夾著別的區塊。其他情況（`%R` 來自
`tensor.empty`／`linalg.fill`、迴圈帶的值、交錯的區塊……）照原本的 tensor 方式 lower，由
bufferization 看見所有 alias（`test/Systolic/fpga_operand_staging_fallback.mlir`）。
`systolic-matmul` 產生的 module 是前一種。

device symbol 對應 bitstream 的實體 ID：

| symbol | device_id | 實體 |
|---|---|---|
| `@acc_8x8_0` | 0 | 8x8 #0 |
| `@acc_4x4_0` | 1 | 4x4 #0 |
| `@acc_4x4_1` | 2 | 4x4 #1 |
| `@acc_4x4_2` | 3 | 4x4 #2 |

### 2.3 Runtime：`runtime/fpga_matmul_tiled.cpp`

一次呼叫 = 一次 invocation：

1. 把 `m x k`、`k x n` 放進實體 `P x K_MAX(d)`、`K_MAX(d) x P` 的 slab（P = 8 或 4；
   device 0 用 K_MAX_8X8、device 1..3 用 K_MAX_4X4；`m`、`n` 以外的列與行、`k` 以後的
   step 是 0），以 8 步一個 window 打包（8 lane 寬；4x4 讀 lane 0..3）。slab 長度是兩種
   幾何裡最深那個的 `ceil(K_MAX/8)·512` byte（16／16 時 1024）——MEM_WRITE 的長度每顆
   bitstream 固定（`dpti_mem_write_decoder` 只收 `HOST_SLAB_BYTES`），這是保留的 DMA
   padding。整塊 8x8x16 / 4x4x16 打包出來的 byte 與 tag 那份 runtime 逐 byte 相同。
2. `MEM_WRITE` 到 DDR 0x10000，等 1 ms（板測時的順序，保留）。
3. 寫 descriptor（job_id、device_id、m = n = P、**k = 這次 invocation 的 k**（1..K_MAX(d)）、
   start、est、a_base = 0x10000、b_base = 0、c_base = 0x3000），最後寫 CONTROL = submit。
4. 讀回 `P x P x 4` byte 的結果 tile，把有效的 `m x n` **加進** C
   （`C_out = C_in + A x B`，所以 K invocation 串起來會累加）。

descriptor 送的是 P x P x k（variable-K）：K_MAX 是 operand buffer 的容量，RTL 讀
`ceil(k/8)` 個 window、餵 k 步（`systolic_accel_context.sv`：`job_k_slab_reg`、`k_dim`；
b971b79 的 RTL，Verilator 模擬中每個 PE 的 MAC 次數 = k，`docs/COSTMODEL.md` 第 4 節）。
邏輯上的 m、n 在補零的列與行裡。K_MAX_8X8／K_MAX_4X4 來自
`SYSTOLIC_FPGA_CONFIG`（`fpga_runtime_get_config()`），`k < 1` 或 `k > K_MAX(d)` 回傳
-10、什麼都不送。

**封存的 bitstream 與 variable-K：** 改成 variable-K 之前，runtime 一律送 k = K_MAX(d)，
封存的 bitstream 在板上只跑過 k = 16。現在 K 不是 16 的倍數時會送 k < 16 的 descriptor；
這個組合在封存的 bitstream 上**沒有板測、也沒有用它的 RTL 模擬過**（FPGA Board
Regression：NOT RUN）。
runtime 同步執行：一次一個 invocation，結果回來才送下一個。

失敗時（`runtime/fpga_matmul_tiled.h`）：參數錯誤在送出任何東西之前就回傳
（-10 K、-11 device／tile、-12 null、-13 打包、-15 `est_cycles = 0`、-16
`SYSTOLIC_FPGA_CONFIG` 無效），不影響鏈路。
鏈路錯誤依階段回傳：-3 session 開不起來、-4 MEM_WRITE 失敗、-5 descriptor 不被接受、
-6 某個 WRITE32 沒送出去（該 descriptor 剩下的部分，包括 CONTROL = submit，都不送）、
-7 讀回失敗、-8 讀回逾時。之後同一個 session 不再送任何東西，呼叫都回傳 -14（只印一次），
直到 `fpga_runtime_close()`——鏈路可能停在封包中間，晚到的結果也可能被當成下一次的。
`fpga_runtime_get_stats()` 的 byte 數從上一次 reset 起算，失敗的呼叫送出的 byte 也算。

### 2.4 FPGA

封存的 bitstream：`artifacts/fpga/tile_pipelined/1x8x8-3x4x4/`，tag
`tile-pipelined-1x8x8-3x4x4-20261009`（commit 22027c4），WNS +0.079 ns，
`systolic_dma_top_1x8x8_3x4x4.bit` 的 sha256 是
`71b6847d5b4c30b23fc99c5ff30a24254552161702ddfd1fb79a3d5657edff7d`（repo 裡的檔案與
`SHA256SUMS`，自 commit 1a22f76 起）。2026-10-12 以前這裡與 `run_e2e.sh` 寫的是
`b9087bade05bd5efc9cad456736a3c0ebb757e62de64d824de0c79e691e7f3e0`：那是 commit 7d2d28b 的
`SHA256SUMS`，當時 `.bit` 還沒進 repo；1a22f76 把 `.bit` 加進來並把 `SHA256SUMS` 改成現在的值。
兩個值為何不同，repo 裡沒有紀錄。它的設定檔是 `test/e2e/profiles/archived-1x8x8-3x4x4.cfg`
（第 3 節）。
`rtl/` 在之後的 branch 上仍在改（per-accelerator context 等），那些版本不是這顆
bitstream。

## 3. Regression：`test/e2e/run_e2e.sh`

先指名 FPGA 上那顆 bitstream 的設定；harness 沒有預設值。

```bash
# 板子：dma_top_build.tcl 寫在 .bit 旁邊的 fpga_config.txt
export SYSTOLIC_FPGA_CONFIG=<bitstream 的目錄>/fpga_config.txt
test/e2e/run_e2e.sh --backend board --check-config   # 只做 host 端：解析、檢查、列出會送的 case；不碰 FPGA
test/e2e/run_e2e.sh --backend board --quick          # 六個必要形狀 + 8x8x(K8+1)、4x4x(K4+1)
test/e2e/run_e2e.sh --backend board                  # 36 組固定案例 + K_MAX 邊界案例

# 封存的 bitstream（第 2.4 節）：明確指名它的 profile
test/e2e/run_e2e.sh --backend board --profile archived-1x8x8-3x4x4

# RTL co-simulation：設定取自 co-sim 自己的 fpga_config.txt（第 4 節）
test/e2e/run_e2e.sh --backend cosim
test/e2e/run_e2e.sh --backend cosim --fixed-only     # 只跑 36 組固定案例（2026-10-13 以前的預設）
test/e2e/run_e2e.sh --backend cosim --kmax-cases     # 只跑 K_MAX 邊界案例

# 設定處理的測試，不需要 FPGA、不啟動 co-sim
cmake --build build --target check-e2e-config        # 即 test/e2e/test_e2e_config.sh
```

`--out DIR` 指定輸出目錄（預設 `build/e2e/<backend>`，`--check-config` 時
`build/e2e/check-config-<backend>`）；目錄會先清空，所以只接受空的或 `run_e2e.sh` 自己建的
目錄（裡面有 `.run_e2e_out`），指到別的目錄會拒絕、不刪。`--cases a,b` 只跑指名的 case，
`--extra-k K,K` 在邊界案例之外再加 12x12xK。

**設定從哪裡來。** K_MAX_8X8、K_MAX_4X4 與 operand slab 一律由 runtime 自己的 parser 解析
（`runtime-dispatch-test --print-config`，與 `systolic-matmul` 讀同一個檔、同樣的規則），case
與每一項檢查都由這三個值推出。兩個 K_MAX 各自獨立，沒有任何地方假設它們相等。

- board：`SYSTOLIC_FPGA_CONFIG`，或 `--profile NAME`（`test/e2e/profiles/NAME.cfg`），二選一。
  兩個都給、都沒給、profile 不存在、檔案打不開、缺 `K_MAX_8X8` 或 `K_MAX_4X4`、格式或範圍不對，
  都在送出任何東西之前、清空 `--out` 之前以 2 結束（`E2E: FAIL -- CONFIGURATION`）。
- cosim：co-sim 的 `fpga_config.txt`；另外指名的設定（環境變數或 `--profile`）必須解析成同一組
  K_MAX_8X8、K_MAX_4X4 與 slab，否則停下。co-sim 沒有 `fpga_config.txt` 時必須另外指名。
- 同一個檔裡的 metadata 必須與 K_MAX 一致，否則同樣停下：`HOST_SLAB_BYTES` 等於 slab、`K_DIM`
  等於 K_MAX_8X8（`dma_top_build.tcl` 的定義）、每個 key 只出現一次（runtime 會取最後一個）、
  `BITSTREAM` 在本機時它的 sha256 等於 `BITSTREAM_SHA256`（相對路徑以設定檔的目錄為準）。
- 三件事分開印：host 端的設定（runtime 解析的值）、bitstream metadata（檔案記的內容，與本機
  `.bit` 的比對結果）、實際載入的硬體。FPGA 不回報自己的 K_MAX（DPTI 只有 WRITE32 與
  MEM_WRITE，沒有讀暫存器），所以板子上一律印 `Hardware configuration not independently
  verified`；設定與板上的 bitstream 不符時，只會表現成 invocation 失敗或結果錯誤。
- runtime 本身（直接執行 `systolic-matmul` 時）沒設 `SYSTOLIC_FPGA_CONFIG` 仍用 built-in 的
  16／16；harness 不依賴它：`runtime-dispatch-test` 沒設就拒絕執行，每組 case 也檢查
  `systolic-matmul` 讀到的來源與數值。

步驟：

0. 設定檔檢查：十五個壞掉的 `SYSTOLIC_FPGA_CONFIG`（前導零、0、超過 16376、非數字、缺
   `K_MAX_4X4`、缺 `K_MAX_8X8`、帶正負號、空值、十六進位、別的 fleet（4x4 或 8x8 的數量）、
   不是 KEY=VALUE、檔案不存在、空檔），`systolic-matmul` 都必須以 4 結束並說出原因。co-sim 上
   用合法的題目，而且不得多出 session；其他情況用 `M = 0` 的題目，設定就算被誤收，也會在讀
   題目時停下，不會碰到板子。
1. `runtime-dispatch-test`：只測 runtime 層，依兩個 K_MAX 各自產生——8x8 與 4x4 的整塊 tile
   （k = K_MAX）、邊界 tile、`C_in != 0`；device 0、1、3 上 k = 1、K_MAX − 1（K_MAX = 1 時沒有
   這個 K，列為 n/a）、K_MAX；k = K_MAX + 1 在 device 1 與 0 上必須回傳 -10、什麼都不送。數值是
   小整數，K_MAX 到 16376 都在 binary32 內精確。它印出的設定必須與上面解析的相同。`--list`
   只列出會送的 invocation、`--print-config` 只印設定，兩者都不開連線。
2. 每組 case 走 `systolic-matmul` 全程（`gen_cases.py`）：
   - 預設：36 組固定案例（形狀 1x1x1、2x3x4、4x4x4、8x8x8、8x4x16、16x8x4、16x8x40、
     17x9x33、32x32x32，各四種值：全 0、正、負、混合），加上 K_MAX 邊界案例：12x12xK，K = 1、
     K_MAX − 1、K_MAX、K_MAX + 1（兩個 K_MAX 各一組）、2·max(K8, K4) + 3，以及 4x4xK4、8x8xK8
     （混合值）。固定形狀不變，它們怎麼折 K 由設定決定（16／16 時 16x8x40 折成 16 + 16 + 8，
     K_MAX 512 時都不折）；跟著容量走的是邊界案例。
   - `--quick`：六個必要形狀 x 四種值，加上 8x8x(K8+1)、4x4x(K4+1)。兩個 K_MAX 裡較大的那個
     比每顆 device 都深，所以任何設定下都至少有一組被折成兩次 invocation（第二次累加到不是零
     的 C 上）；8x8、4x4、邊界 tile 由六個形狀涵蓋（每次實際用到的見 summary 的 coverage 行）。
   - `--fixed-only`：只有 36 組固定案例。`--kmax-cases`：只有邊界案例。
   - K 超過 `systolic-matmul` 的輸入上限（K ≤ 4096，從它的 `--help` 讀）的 case 不跑，列為
     NOT RUN 並寫明原因。
3. 只在 co-sim：故意弄壞一次傳輸。session 的第 7 次 write（第一個 descriptor 的
   第 5 個 WRITE32）失敗時，`systolic-matmul` 必須以 4 結束、runtime 回報
   `first error -6`（TRANSPORT），RTL 只收到並讀走 MEM_WRITE 與 4 個 WRITE32（`9 + slab + 24`
   byte，16／16 時 1057 B）——沒有 submit，所以也沒有任何結果送回。16x8x4 要是五次
   invocation 這個檢查才成立，所以 K_MAX_8X8 或 K_MAX_4X4 小於 4 時記為 NOT TESTED。

每組的分類（`test/e2e/e2e_verdict.py`），表中同時保留原始的 exit status（EXIT）與 runtime 的
回傳碼（RT_RC，第一個錯誤）：

| 分類 | 條件 |
|---|---|
| PASS | 結果與 CPU reference 逐元素相等；每個 `systolic.matmul_tile` 恰好一次 invocation；每次 invocation 送出 9 + slab + 84 byte（設定的 slab）；co-sim 上 RTL 讀走（RD#）與送出（WR#）的 byte 數等於 runtime 計的 |
| MISMATCH | 數值不符（exit 1） |
| CONFIG | `systolic-matmul` 拒絕設定（exit 4）、runtime -16，或設定沒有原樣到達某一層：runtime 讀到別的值或別的檔（例如 built-in 的 16／16）、compiler 用別的 k_max 折 K、某個 tile 比它的 device 深、K 沒有照該幾何的 K_MAX 折、slab 長度不對 |
| TRANSPORT | 鏈路失敗：-3、-4、-6、-7、-8（-14 是先前失敗之後被拒絕的呼叫）；co-sim 上 RTL 搬的 byte 數與 runtime 計的不同 |
| SUBMISSION | runtime 拒絕或無法送出 invocation：-5、-10、-11、-12、-13、-15 |
| INPUT／COMPILE | `systolic-matmul` 拒絕輸入（exit 2）／編譯失敗（exit 3） |
| FAIL | 其他（signal、沒有驗證結果、tile 數不等於 invocation 數……） |
| NOT RUN | 沒有執行：K 超過輸入上限，或 runtime 層已經 TRANSPORT／CONFIG 失敗（`--keep-going` 照跑） |

rc = -6 是傳輸失敗，不是 GEMM 算錯；-14 是先前那次失敗的後果，不另算一個獨立的錯誤。每組的
輸入、輸出與五層 IR 都留在 `<out>/<case>/`，總表在 `<out>/summary.txt`，最後幾行是：設定、硬體
是否驗證、`configuration refusal`、`runtime layer`、`E2E cases: P/T PASS on backend B`（加上
各分類的數量與 NOT RUN）、通過的 case 實際涵蓋了什麼（每顆 device 的 tile 數、邊界 tile、8x8 與
4x4 是否折 K、是否跑到 k = K_MAX），以及 `E2E: PASS …` 或 `E2E: FAIL -- …`。結束碼：0 全部通過、
2 設定被拒絕（什麼都沒送）、1 其他。

**換一顆新的 bitstream。** `dma_top_build.tcl -tclargs build v1 <K8> <K4>` 成功後，`.bit` 旁邊
有 `fpga_config.txt`（K_MAX、fleet、`HOST_SLAB_BYTES`、`BITSTREAM_SHA256`、git revision、
WNS／WHS）。確認燒進板子的就是這顆 `.bit` 之後：

```bash
export SYSTOLIC_FPGA_CONFIG=<那個目錄>/fpga_config.txt
test/e2e/run_e2e.sh --backend board --check-config   # 先看解析出的 K8、K4、slab 與 metadata
test/e2e/run_e2e.sh --backend board --quick
test/e2e/run_e2e.sh --backend board
```

要長期保存的 bitstream，可以把它的設定檔放進 `test/e2e/profiles/<名字>.cfg`（`BITSTREAM` 用
相對路徑、附 `BITSTREAM_SHA256`），之後用 `--profile <名字>`。

## 4. RTL co-simulation（沒有板子時）

```bash
test/e2e/cosim/build.sh            # -> build/cosim/Vcosim, build/cosim/libftdi_cosim.so
```

- RTL 取自 bitstream 的 git revision（預設上面那個 tag，`RTL_REV=` 可改，
  `RTL_DIR=.` 用 working tree），source list 與 generics 照 `dma_top_build.tcl`
  （external scheduler、無 legacy job port、v1 operand path）。K_MAX 預設 16／16；
  有 `K_MAX_8X8`／`K_MAX_4X4` 參數的 RTL（`RTL_DIR=.`）可用同名環境變數各自設定
  （十進位、不帶前導零；co-sim 的範圍是 1..1024：DDR model 只有 128 KiB，而 runtime
  把 slab 放在 0x10000，`ceil(K/8)·512` byte 要放得進 64 KiB——這是模擬環境的限制，
  板子的 DDR 沒有）。build.sh 把這次用的參數寫進 `fpga_config.txt`，
  `run_e2e.sh --backend cosim` 自動把 `SYSTOLIC_FPGA_CONFIG` 指過去，所以 compiler、
  runtime 與模擬的 RTL 用同一組值；已經 export 的 `SYSTOLIC_FPGA_CONFIG`（或 `--profile`）
  若寫的是別的 K_MAX 或 slab，run_e2e.sh 直接停下，不讓 runtime 對著不同的 RTL 送 slab。
- 只換 Xilinx IP：MIG／MMCM／VIO 用既有的 `tb/mig_axi_model.sv`、
  `tb/xil_stubs.sv`；FP IP 用 `test/e2e/cosim/fp_ieee.sv`（binary32、round to
  nearest even、denormal flush、latency 8／11 取自 .xci）。`tb/fp_model.sv` 是整數
  運算，給控制路徑用，不能拿來接真的 host。
- `tb_dpti_cosim.sv` 模擬 FT2232H 的 sync FIFO（60 MHz）。host 端是**同一個**
  `systolic-matmul` 執行檔，只用 `LD_PRELOAD=libftdi_cosim.so` 把 libftdi 的
  write/read 接到 UNIX socket；opcode、descriptor、DDR staging、結果讀回全由 RTL 做。
- 每個 session 記三個數：從 host 收到的、RTL 用 RD# 讀走的、RTL 用 WR# 送出的
  byte 數。host 關閉之後，模擬的板子照常跑，直到 RTL 讀完 host 寫出的每個 byte、送完
  host 已 submit 的每個 job 的結果（最多 2^22 個 FIFO clock），這段送出的東西丟掉——
  板子上，下一個 session 的 `ftdi_usb_purge_buffers()` 也會把它丟掉。板子比這個模擬快
  約千倍：不這樣做的話，帶著未完成 job 離開的 host 會把那個 job 的結果留給下一個
  session。
- `SYSTOLIC_DPTI_COSIM_FAIL_WRITE=<n>` 讓 session 的第 n 次 `ftdi_write_data()`
  像 USB 寫入錯誤一樣失敗、什麼都不送（第 3 節的失敗檢查用它）。
- 有 `K_MAX_8X8`／`K_MAX_4X4` 參數的 RTL（`RTL_DIR=.`）建出來的 co-simulation，每個 job
  的結果寫回 DDR 時印一行 `COSIM_STEPS`：descriptor 的 k、每個 PE 實際做的 MAC 次數、
  feeder 送進 row 0／column 0 的 operand 數、`k_dim`、job 被接受與結果進 DDR 的 cycle
  （`rtl/multi_200t/tile_pipelined/dma/tb/reduction_probe.svh`，只讀不驅動）。
  `SYSTOLIC_RUNTIME_TRACE=1` 跑 `run_e2e.sh` 之後，`test/e2e/check_variable_k.py`
  逐 invocation 比對 compiler IR 的 k、runtime 送的 job.k、RTL 收到的 k 與每個 PE 的
  step 數；加 `--cost build/bin/systolic-cost` 時也比對 FPGA 內部 cycle（job 被接受 ->
  結果在 DDR）與 cost model 的 read + compute + write。
- 這是模擬，不是板子：證明的是 compiler、runtime、wire protocol 與這份 RTL 的邏輯
  一致；時序收斂、FT2232H 的真實行為、DDR3 的真實延遲都不在其中。

## 5. Checkpoint 指令

| Phase | 指令 | 判準 |
|---|---|---|
| Linalg → Systolic | `systolic-opt --systolic-binary-search-tile-matmul test/Systolic/linalg_to_systolic_4x4x4.mlir` | 1 個 `systolic.matmul_tile ... m = 4 n = 4 k = 4 on @acc_4x4_0 est_cycles = 105 start_cycle = 0` |
| Systolic → FPGA | `systolic-opt --systolic-fpga-pipeline test/Systolic/systolic_to_fpga.mlir` | 只剩 LLVM dialect、`llvm.call @fpga_matmul_tiled_auto_scheduled` |
| lit | `cmake --build build --target check-systolic` | 38/38 |
| E2E 設定（host 端） | `cmake --build build --target check-e2e-config` | `test_e2e_config: N passed, 0 failed` |
| runtime → FPGA | `SYSTOLIC_FPGA_CONFIG=<fpga_config.txt> build/bin/runtime-dispatch-test`（板子，或 `run_e2e.sh --backend cosim`） | `RUNTIME DISPATCH: PASS` |
| E2E | `SYSTOLIC_FPGA_CONFIG=<fpga_config.txt> test/e2e/run_e2e.sh --backend board` | `E2E: PASS`（`E2E cases: N/N PASS`；N 依設定而定） |

## 6. 已知限制

- 一次一個 invocation：四顆陣列在時間上沒有並行（硬體 scheduler 一次接一個 job，
  runtime 也同步送）。`start_cycle` 有寫進 descriptor，但不能讀成硬體強制的發射時間。
- 每次 invocation 的 MEM_WRITE 都是整塊 slab（`HOST_SLAB_BYTES`，最深幾何的
  `ceil(K_MAX/8)·512` byte）：小 tile 也付整塊的 USB 傳輸，K_MAX 越深越多。FPGA 端只讀
  `ceil(k/8)` 個 window。
- 跨 invocation 的累加在 host（bitstream 沒有 C-in）。
- 輸入只收整數；值域與大小限制見第 1 節。
- pipeline 只 lower `linalg.matmul`（預設 indexing maps、靜態形狀的 f32 tensor）。
  帶 `indexing_maps` 的轉置 matmul、空的 M／N／K 會被拒絕並給 diagnostic。module 裡
  其他的 linalg op（例如把輸出歸零的 `linalg.fill`）不會被編成 CPU 程式碼——沒有 CPU
  fallback，bufferization 會以 `op was not bufferized` 失敗；C 要從函式參數傳進來。
- `est_cycles` 是 cost model 的 compute term（這次 invocation 的 k + R + C - 2 + H）；
  read／write 由 binary search 拿來做分解，不在 `est_cycles` 裡（`docs/COSTMODEL.md`）。
- device symbol 名稱是固定的（第 2.2 節的表）。

## 7. K_MAX_8X8 與 K_MAX_4X4

8x8 的 operand 深度 K_MAX_8X8 與每顆 4x4 的 K_MAX_4X4 是兩個獨立的合成參數，各自
是 1..16376 的任何整數（十進位寫法，不帶前導零），不必相等、不必是 2 的冪。一組值從頭
到尾是同一組：

| 層 | 從哪裡拿 |
|---|---|
| bitstream | `dma_top_build.tcl -tclargs build v1 <K8> <K4>`；成功後在 `.bit` 旁寫 `fpga_config.txt`（K、fleet、variant、bitstream sha256、git revision、WNS/WHS） |
| RTL 模擬 | `run_top_sim.sh`（第 5、6 個參數）、`run_kmax_sim.sh <K8> <K4> [v1\|v2]`、`test/e2e/cosim/build.sh`（環境變數，同樣寫 `fpga_config.txt`） |
| runtime | `SYSTOLIC_FPGA_CONFIG=<fpga_config.txt>`；沒設＝16／16（built-in default） |
| E2E harness | 同一個檔，由 runtime 的 parser 解析（`runtime-dispatch-test --print-config`）；沒有預設值，board 要 `SYSTOLIC_FPGA_CONFIG` 或 `--profile`（第 3 節） |
| compiler | `systolic-matmul` 從 runtime 拿同一組值產生 fleet 的 `k_max`，並交給 FPGA lowering 當實體 K_MAX；`systolic-opt` 用 `phys-k-max-8x8=` / `phys-k-max-4x4=` |

全部組態一次跑完（RTL bench、co-sim build、36 組固定案例、K_MAX 邊界案例）：
`test/e2e/run_kmax_matrix.sh`。限制與理由見 `docs/KMAX.md`。
