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
cmake --build build --target check-systolic        # 21 個 lit test

printf '2 3 4\n1 2 3 4\n-1 0 2 -3\n1 0 -1\n2 1 0\n0 -2 3\n-1 1 1\n' \
  | ./build/bin/systolic-matmul --dump-ir=/tmp/e2e
```

`systolic-matmul` 需要 MLIR 的 ExecutionEngine 與 `mlir_c_runner_utils`（MLIR 用
host target 建置時預設就有）；沒有的話 CMake 只會跳過這個工具並印 warning。
runtime 需要 `libftdi1`（`pkg-config libftdi1`）。

輸入格式：先 `M N K`，再 M 列各 K 個整數（A），再 K 列各 N 個整數（B），以空白分隔，
換行不計。從終端機輸入時會提示。

| 選項 | 作用 |
|---|---|
| `--dump-ir=DIR` | 每一層的 IR：`00-linalg.mlir` `10-systolic.mlir` `20-fpga.mlir` `30-llvm.mlir` `40-llvm.ll` |
| `--quiet` | 只印驗證結果 |

| 環境變數 | 作用 |
|---|---|
| `SYSTOLIC_RUNTIME_TRACE=1` | 每次 FPGA invocation 一行（device、tile、padding、schedule） |
| `SYSTOLIC_DPTI_TRACE=1` | 每個 DPTI 封包（MEM_WRITE、每個 WRITE32） |
| `SYSTOLIC_DPTI_TIMEOUT_MS` | 等結果的上限，預設 5000 |

結束碼：0 PASS、1 FAIL（不相等）、2 輸入錯、3 編譯失敗、4 FPGA／runtime 錯誤。
runtime 呼叫失敗時直接回報 4，不會把沒算完的 C 當結果比。

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
- 一個 `systolic.matmul_tile` 是一顆 device 的**一次 invocation**。device 宣告
  `k_max` 時，每個 spatial tile 的 reduction 折成 `I = ceil(K / k_max)` 次，用
  `c_in` 串起來；沒宣告 `k_max` 的 device 維持整個 K（舊行為，舊測試輸出不變）。
- optimizer 對每個 spatial tile 計一次 invocation 的成本 `C(g) = 2g - 2 + H`，
  起點 `round * C(g)`；每次 invocation 成本相同，所以每顆 device 的時間軸拉長
  `I` 倍：第 j 次 invocation 從 `start * I + j * C(g)` 開始，順序與不重疊都保留。

### 2.2 Systolic → FPGA：`--lower-systolic-matmul-tile-to-fpga`

每個 tile 變成一次
`fpga_matmul_tiled_auto_scheduled(m, k, n, A, B, C, device_id, start_cycle, est_cycles)`。
合法條件：已指派 device、已排程、`m <= rows`、`n <= cols`、`1 <= k <= k_max`、fp32。
任何一個 tile 不合法就整個 pass 失敗、每個 tile 一條 diagnostic，不會留下半 lower 的
module。device symbol 對應 bitstream 的實體 ID：

| symbol | device_id | 實體 |
|---|---|---|
| `@acc_8x8_0` | 0 | 8x8 #0 |
| `@acc_4x4_0` | 1 | 4x4 #0 |
| `@acc_4x4_1` | 2 | 4x4 #1 |
| `@acc_4x4_2` | 3 | 4x4 #2 |

### 2.3 Runtime：`runtime/fpga_matmul_tiled.cpp`

一次呼叫 = 一次 invocation：

1. 把 `m x k`、`k x n` 補零到實體 `P x 16`、`16 x P`（P = 8 或 4），打包成
   1024-byte 的 slab（K = 16、8x8 的版面；4x4 讀 lane 0..3）。整塊 8x8x16 /
   4x4x16 打包出來的 byte 與 tag 那份 runtime 逐 byte 相同。
2. `MEM_WRITE` 到 DDR 0x10000，等 1 ms（板測時的順序，保留）。
3. 寫 descriptor（job_id、device_id、m = n = P、k = 16、start、est、a_base =
   0x10000、b_base = 0、c_base = 0x3000），最後寫 CONTROL = submit。
4. 讀回 `P x P x 4` byte 的結果 tile，把有效的 `m x n` **加進** C
   （`C_out = C_in + A x B`，所以 K invocation 串起來會累加）。

descriptor 只送板子跑過的形狀（P x P x 16）；邏輯上的 m、n、k 都在補零裡。
runtime 同步執行：一次一個 invocation，結果回來才送下一個。

### 2.4 FPGA

封存的 bitstream：`artifacts/fpga/tile_pipelined/1x8x8-3x4x4/`，tag
`tile-pipelined-1x8x8-3x4x4-20261009`（commit 22027c4），WNS +0.079 ns，
`systolic_dma_top_1x8x8_3x4x4.bit` 的 sha256 是
`b9087bade05bd5efc9cad456736a3c0ebb757e62de64d824de0c79e691e7f3e0`。
`rtl/` 在之後的 branch 上仍在改（per-accelerator context 等），那些版本不是這顆
bitstream。

## 3. Regression：`test/e2e/run_e2e.sh`

```bash
test/e2e/run_e2e.sh --backend board      # 板子（先燒好上面那顆 bitstream）
test/e2e/run_e2e.sh --backend cosim      # RTL co-simulation（見第 4 節）
test/e2e/run_e2e.sh --backend cosim --quick   # 只跑六個必要形狀
```

1. `runtime-dispatch-test`：只測 runtime 層——每顆 device 一次 invocation，含邊界
   tile 與 `C_in != 0`。
2. `gen_cases.py` 的 36 組：形狀 1x1x1、2x3x4、4x4x4、8x8x8、8x4x16、16x8x4，加上
   K 要折的 16x8x40、17x9x33、32x32x32；每個形狀四種值：全 0、正、負、混合。
   每組都走 `systolic-matmul` 全程。

一組 PASS 的條件：`systolic-matmul` 驗證 PASS；每個 `systolic.matmul_tile` 都恰好
變成一次 FPGA invocation；在 co-sim 上，runtime 在 FTDI 那層計的送出 byte 數等於
RTL 真正讀走（RD#）的 byte 數，host 送出的每個 byte 都被讀走，runtime 收到的 byte 數
等於 RTL 送出（WR#）的 byte 數。每組的輸入、輸出與五層 IR 都留在
`build/e2e/<backend>/<case>/`，總表在 `build/e2e/<backend>/summary.txt`。

3. 只在 co-sim：故意弄壞一次傳輸。session 的第 7 次 write（第一個 descriptor 的
   第 5 個 WRITE32）失敗時，`systolic-matmul` 必須以 4 結束、runtime 回報
   `first error -6`，RTL 只收到並讀走 MEM_WRITE 與 4 個 WRITE32（1057 B）——沒有
   submit，所以也沒有任何結果送回。

## 4. RTL co-simulation（沒有板子時）

```bash
test/e2e/cosim/build.sh            # -> build/cosim/Vcosim, build/cosim/libftdi_cosim.so
```

- RTL 取自 bitstream 的 git revision（預設上面那個 tag，`RTL_REV=` 可改，
  `RTL_DIR=.` 用 working tree），source list 與 generics 照 `dma_top_build.tcl`
  （external scheduler、無 legacy job port、v1 operand path、K_MAX 16）。
- 只換 Xilinx IP：MIG／MMCM／VIO 用既有的 `tb/mig_axi_model.sv`、
  `tb/xil_stubs.sv`；FP IP 用 `test/e2e/cosim/fp_ieee.sv`（binary32、round to
  nearest even、denormal flush、latency 8／11 取自 .xci）。`tb/fp_model.sv` 是整數
  運算，給控制路徑用，不能拿來接真的 host。
- `tb_dpti_cosim.sv` 模擬 FT2232H 的 sync FIFO（60 MHz）。host 端是**同一個**
  `systolic-matmul` 執行檔，只用 `LD_PRELOAD=libftdi_cosim.so` 把 libftdi 的
  write/read 接到 UNIX socket；opcode、descriptor、DDR staging、結果讀回全由 RTL 做。
- 每個 session 記三個數：從 host 收到的、RTL 用 RD# 讀走的、RTL 用 WR# 送出的
  byte 數。host 關閉時，它已經寫出的 byte 仍照 RTL 讀取的速度交給 RTL（最多
  2^20 個 FIFO clock）：板子比這個模擬快約千倍，host 走到 close 之前早就讀完了。
- `SYSTOLIC_DPTI_COSIM_FAIL_WRITE=<n>` 讓 session 的第 n 次 `ftdi_write_data()`
  像 USB 寫入錯誤一樣失敗、什麼都不送（第 3 節的失敗檢查用它）。
- 這是模擬，不是板子：證明的是 compiler、runtime、wire protocol 與這份 RTL 的邏輯
  一致；時序收斂、FT2232H 的真實行為、DDR3 的真實延遲都不在其中。

## 5. Checkpoint 指令

| Phase | 指令 | 判準 |
|---|---|---|
| Linalg → Systolic | `systolic-opt --systolic-binary-search-tile-matmul test/Systolic/linalg_to_systolic_4x4x4.mlir` | 1 個 `systolic.matmul_tile ... m = 4 n = 4 k = 4 on @acc_4x4_0 est_cycles = 101 start_cycle = 0` |
| Systolic → FPGA | `systolic-opt --systolic-fpga-pipeline test/Systolic/systolic_to_fpga.mlir` | 只剩 LLVM dialect、`llvm.call @fpga_matmul_tiled_auto_scheduled` |
| lit | `cmake --build build --target check-systolic` | 21/21 |
| runtime → FPGA | `build/bin/runtime-dispatch-test`（板子，或 `run_e2e.sh --backend cosim`） | `RUNTIME DISPATCH: PASS` |
| E2E | `test/e2e/run_e2e.sh --backend board` | `E2E cases: 36/36 PASS` |

## 6. 已知限制

- 一次一個 invocation：四顆陣列在時間上沒有並行（硬體 scheduler 一次接一個 job，
  runtime 也同步送）。`start_cycle` 有寫進 descriptor，但不能讀成硬體強制的發射時間。
- 每次 invocation 都是完整的 P x P x 16 slab：小 tile 也付整塊的傳輸。
- 跨 invocation 的累加在 host（bitstream 沒有 C-in）。
- 輸入只收整數；值域見第 1 節。
- `est_cycles` 用的是 binary search 的每次 invocation 成本 `2g - 2 + H`，不含 k 項；
  那是 optimizer 原本的模型，沒動。
- device symbol 名稱是固定的（第 2.2 節的表）。
