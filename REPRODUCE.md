# 重現 Systolic-MLIR：C GEMM → systolic-cc → FPGA（實板或 RTL 共同模擬）

本文說明如何從 C 程式開始，在 Systolic Array FPGA 上執行 GEMM，並與 CPU 參考結果比對，提供兩種模式：

- **實板模式**：使用燒錄指定封存 bitstream 的 Digilent Nexys Video，透過 DPTI（FT2232H、USB）通訊。
- **共同模擬模式**：不使用 FPGA；相同主機執行檔透過 DPTI 與 Verilator 中的工作目錄 RTL 通訊，Xilinx IP 則以模型取代。模擬與歷史封存未提交原始碼的精確一致性仍為 UNVERIFIED（尚未驗證），見第 6.1 節。

**2026-10-26 更新：** 新建置的 `shared_wb_r2_tclfix_1025` 映像、乾淨編譯器建置與本次實測證據，統一記錄於第 17 節及新的獨立封存目錄。2026-10-25 舊封存與歷史測試紀錄均保留，不因新結果而改寫其狀態。

`scripts/reproducibility/` 的實板包裝腳本仍固定使用 **2026-10-25** 封存，沒有隨本文修改；要測試本次新映像，必須使用第 17 節的明確組態與新 Build Directory，不能誤用包裝腳本而切回舊組態。

歷史執行另於第 15 節標示日期，不能用來證明所有目前指令或工作目錄版本均已通過。`scripts/reproducibility/` 支援從 `env -i` 開始，會重設第 4 節列出的變數，並接受明確指定的工具鏈／建置目錄，不會修改呼叫端 Shell。

論文數據的來源見 `docs/PROVENANCE.md`。編譯器各 Pass 由 lit 測試集逐一檢查（`lit build/test`，見第 15 節）；成本模型說明見預設的 `docs/COSTMODEL_V2.md` 與 V1 的 `docs/COSTMODEL.md`。

## 1. 流程概覽

```text
使用者 C 程式（test/reproducibility/gemm_example.c）
  │  systolic-cc --runtime=board --fp-reassociation
  ▼
systolic-cfront（clang 24 AST → MLIR，逐函式處理）
  ▼
仿射 GEMM 辨識與精確分割（--affine-matmul-to-systolic）
  ▼
Systolic 降階：每次呼叫以 systolic.matmul_tile 表示，排程至
1x8x8 + 3x4x4 加速器群，依 K_MAX = 16 分段歸約（--lower-systolic-matmul-tile-to-fpga）
  ▼
LLVM → 目的檔；產生的 C 橋接程式呼叫核心，連結
libsystolic_fpga_runtime.a + libftdi1
  ▼
Runtime 提交工作（非同步：每分塊 fpga_tile_submit，每 GEMM fpga_gemm_wait）
  ▼
DPTI 位元組串流（MEM_WRITE 運算元資料區塊、WRITE32 描述子、完成紀錄與分塊）
  ├── 實板模式：系統 libftdi1 → FT2232H → Artix-7 上的 systolic_dma_top
  └── 共同模擬：libftdi_cosim.so（LD_PRELOAD）→ UNIX 通訊端 → Vcosim
                 （Verilator：目前 systolic_dma_top RTL；Xilinx 浮點 IP、
                  MIG/DDR3 與基本元件改用測試平台模型）
  ▼
讀回結果 → C → 程式的 CPU 參考結果 → PASS／FAIL
```

| 路徑 | 用途 |
|---|---|
| `scripts/reproducibility/env.common.sh` | 共用編譯器／Runtime 環境，透過 source 載入 |
| `scripts/reproducibility/env.board.sh` | 實板模式環境，透過 source 載入 |
| `scripts/reproducibility/env.cosim.sh` | 共同模擬模式環境，透過 source 載入 |
| `scripts/reproducibility/check_env.sh` | 檢查 `board`、`cosim` 或 `all` 的各項需求 |
| `scripts/reproducibility/build.sh` | 設定並建置 systolic-cc 與 Runtime；`--cosim` 另建置 Vcosim |
| `scripts/reproducibility/run_board.sh` | 提供 `check`、`verify-program`、`program`、`e2e`、`c FILE.c` |
| `scripts/reproducibility/program_bit.tcl` | Vivado 硬體管理腳本，將指定 `.bit` 燒錄至 xc7a200t，供 `run_board.sh program` 使用 |
| `scripts/reproducibility/run_cosim.sh` | 提供 `check`、`build`、`e2e`、`c FILE.c` |
| `scripts/reproducibility/c_program.sh` | 共用輔助流程：編譯 C 程式、執行並回報 |
| `test/reproducibility/gemm_example.c` | 最小使用者範例：GEMM、CPU 參考計算與 PASS/FAIL |

PASS 表示指定檢查通過，FAIL 表示失敗，UNKNOWN 表示無法判定，UNVERIFIED 表示尚未取得所述範圍的驗證證據，VERIFIED 表示已有對應實測證據；各狀態僅適用於明確指定的版本、映像與測試範圍。

## 2. 硬體與軟體需求

`check_env.sh` 檢查工具、版本、建置產物、組態、存在的封存檔雜湊與 USB 裝置可見性。它不會測試 USB 存取權限、開啟開發板、驗證 Vivado 授權或辨識 FPGA 已載入的映像。請使用第 6 節完整的 `sha256sum -c` 檢查，同時偵測封存檔缺漏；`check_env.sh` 會跳過清單中不存在的檔案。

**硬體（僅實板模式需要）**

- Digilent Nexys Video（`xc7a200tsbg484-1`）；DPTI 介面為 FT2232H，主機必須能看到 USB ID `0403:6010`。
- 使用者須有該 USB 裝置的讀寫權限。測試主機使用 udev 規則，將 Digilent `0403` 裝置設為模式 0666。可依系統管理方式使用 udev、裝置群組或 ACL；USB 可見不代表 libftdi 能成功開啟。
- Vivado 隨附的 JTAG 纜線驅動程式，僅燒錄時需要。

**軟體**

| 項目 | 用途 | 已測版本 |
|---|---|---|
| Linux x86_64、bash | 全部流程 | Ubuntu、bash 5 |
| cmake、ninja、gcc/g++ | 建置 | GCC 13.3.0，Ninja 來自 `/usr/bin` |
| pkg-config、libftdi1 開發檔案（`libftdi1-dev`） | Runtime 連結 | libftdi1 1.5 |
| python3（僅標準函式庫） | `run_e2e.sh` 輔助程式 | 3.12（`/usr/bin/python3`） |
| git、sha256sum、sed、awk | 腳本 | 系統版本 |
| LLVM/MLIR 24 **建置目錄**（`lib/cmake/{llvm,mlir}`、`bin/mlir-translate`、`bin/llc`、`lib/libmlir_c_runner_utils.so`） | 建置與 systolic-cc | 24.0.0git，版本 7615da10b75c |
| clang 24（`-DSYSTOLIC_CFRONT_CLANG`） | systolic-cc 的 C 前端與主機編譯 | 24.0.0git，版本 7615da10b75c |
| Verilator ≥ 5.0 | 共同模擬模式 | 5.020 |
| Vivado + hw_server | 燒錄封存映像；選用的原始碼 FPGA 建置亦需 Vivado（第 6.1 節） | 2026.1，封存 bitstream 使用此版本建置 |

LLVM 18（例如 `/usr/lib/llvm-18`）**無法**建置目前版本。上述實板／共同模擬流程不使用 Icarus Verilog。

## 3. 乾淨環境設定

腳本設計上不依賴開發者環境變數，例如 `SYSTOLIC_FPGA_CONFIG`、`LD_LIBRARY_PATH`、`LD_PRELOAD` 或 `PATH` 中的自訂編譯器目錄。模擬新使用者環境：

```bash
cd ~/work/systolic-mlir

env -i \
  HOME="$HOME" \
  USER="$(id -un)" \
  PATH="/usr/bin:/bin" \
  bash --noprofile --norc
```

在此 Shell 中，從 Repository 根目錄執行：

```bash
bash scripts/reproducibility/check_env.sh all
```

腳本只需要繼承三個變數：

- `PATH=/usr/bin:/bin`：先找到 bash，之後腳本自行重建 `PATH`。
- `HOME`：Vivado 使用者設定；選用 FPGA 建置 Tcl 也據此尋找專案輸出與外部開發板定義。
- `USER`：僅用於報告。

`env -i` **不是**全新作業系統，只會清除繼承變數。第 2 節的 LLVM 24、clang 24、libftdi1、Verilator、Vivado 與 USB 規則仍來自本機安裝。`check_env.sh` 因此分別回報：

- `MISSING`：缺少套件或裝置，須安裝或連接；匯出變數無法解決。
- `NOT BUILT`：`build.sh` 尚未產生的檔案。

## 4. 環境隔離

`run_*.sh` 在自己的程序中以 source 載入環境檔，不寫入 `~/.bashrc` 或 `~/.profile`，執行後不改變呼叫端終端機。包裝腳本會取代目標組態，清除 `LD_PRELOAD`、`LD_LIBRARY_PATH` 與 `SYSTOLIC_DPTI_COSIM_SOCKET`，再設定自身模式。這不是通用環境沙盒：共同模擬的 `RTL_REV`、`RTL_DIR`、`USE_V2`、`K_MAX_8X8` / `K_MAX_4X4` 等控制仍可能繼承；需要指定預設值時請使用 `env -i`。

兩種模式共用 `env.common.sh`：

| 變數 | 值 | 使用者 |
|---|---|---|
| `SYSTOLIC_BUILD` | 預設 `<repo>/build`，可由命令列指定 | systolic-cc 與腳本 |
| `LLVM_BUILD` | `SYSTOLIC_BUILD/CMakeCache.txt` 中的 `LLVM_DIR` 去除 `/lib/cmake/llvm` | systolic-cc：mlir-translate、llc、libmlir_c_runner_utils |
| `SYSTOLIC_CFRONT_CLANG` | `CMakeCache.txt` 的 `SYSTOLIC_CFRONT_CLANG` | systolic-cc、`test/e2e/systolic_cc/run_board.sh` |
| `PATH` | `SYSTOLIC_BUILD/bin:LLVM_BUILD/bin:/usr/local/bin:/usr/bin:/bin` | 全部工具 |
| 清除設定 | `LD_PRELOAD`、`LD_LIBRARY_PATH`、`SYSTOLIC_DPTI_COSIM_SOCKET`、`SYSTOLIC_FPGA_CONFIG` | — |

編譯後程式不需要 `LD_LIBRARY_PATH`。libftdi1 是系統函式庫，`libmlir_c_runner_utils.so` 使用 rpath 連結。

`env.board.sh` 增加：

| 變數 | 值 |
|---|---|
| `SYSTOLIC_FPGA_CONFIG` | `<repo>/artifacts/fpga/tile_pipelined/1x8x8-3x4x4/2026-10-25-k8_16-k4_16/fpga_config.txt` |
| `PATH`（僅找不到 `vivado` 時） | 加入 `$XILINX_VIVADO/bin`；`XILINX_VIVADO` 由 Vivado 的 `settings64.sh` 設定 |

`env.cosim.sh` 增加：

| 變數 | 值 |
|---|---|
| `COSIM_BUILD_DIR` | `SYSTOLIC_BUILD/cosim`，屬於 `test/e2e/cosim/build.sh` 的變數 |
| `SYSTOLIC_FPGA_CONFIG` | 共同模擬建置產生的 `COSIM_BUILD_DIR/fpga_config.txt` |
| `LD_PRELOAD`、`SYSTOLIC_DPTI_COSIM_SOCKET`、`SYSTOLIC_DPTI_TIMEOUT_MS=600000` | `run_cosim.sh` 僅對連接運作中 Vcosim 的單一程式設定，不匯出為全域環境 |

共同模擬不使用實板的 `fpga_config.txt`。兩者均為 K_MAX 16/16、完成紀錄與 `PIPELINE_DEPTH=4`，但 `DDR_BYTES` 不同：模擬 DDR 為 1 MiB，實板為 512 MiB，非同步 Runtime 依 `DDR_BYTES` 配置緩衝區。

變數可在命令列覆寫，例如 `SYSTOLIC_BUILD=/tmp/b bash scripts/reproducibility/run_cosim.sh e2e`；第 15 節的乾淨建置即採此方式。Shell 原先 `SYSTOLIC_FPGA_CONFIG` 指向的檔案會被**忽略**，各模式自行設定；`check_env.sh` 會列出發現的相關變數。

多數變數不需手動 `export`；首次 CMake 設定則須提供工具鏈位置作為參數（第 5 節）。

## 5. 建置 systolic-cc

首次在主機建置時，指定 LLVM 24 建置目錄與 clang 24：

```bash
cd ~/work/systolic-mlir
# 請替換為實際安裝的工具鏈路徑。
LLVM24_BUILD_DIR=/path/to/llvm24/build
CLANG24=/path/to/clang24
bash scripts/reproducibility/build.sh --llvm-build "$LLVM24_BUILD_DIR" --clang "$CLANG24"
# LLVM24_BUILD_DIR 包含 lib/cmake/llvm 與 lib/cmake/mlir；
# CLANG24 的版本輸出須包含 "clang version 24."。
```

此腳本執行：

```bash
cmake -G Ninja -S . -B build -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ \
  -DLLVM_DIR="$LLVM24_BUILD_DIR/lib/cmake/llvm" -DMLIR_DIR="$LLVM24_BUILD_DIR/lib/cmake/mlir" \
  -DSYSTOLIC_CFRONT_CLANG="$CLANG24"
cmake --build build
```

`build.sh` 拒絕非 24 版 LLVM 或 clang，並確認已產生 `bin/systolic-cc`、`systolic-cfront`、`systolic-opt`、`systolic-matmul`、`runtime-dispatch-test`、`runtime-async-test` 與 `runtime/libsystolic_fpga_runtime.a`。預設全部建置目標包含上述產物，不需額外 CMake 目標。

後續建置從 `build/CMakeCache.txt` 讀取位置，不再需要參數：

```bash
bash scripts/reproducibility/build.sh            # 增量建置
bash scripts/reproducibility/build.sh --cosim    # 另建置共同模擬模式的 Vcosim
```

選用的迴歸測試需要 `lit`、LLVM 24 的 `count` 與 `not`；測試所用 LLVM 24 建置目錄原先缺少後兩者。入口為 `cmake --build build --target check-systolic`。第 17 節記錄本次如何從相同 LLVM 24 原始碼另行建置輔助工具，並實際驗證新的編譯器。

## 6. FPGA bitstream 與封存資訊

本次新驗證封存位於：

`artifacts/fpga/tile_pipelined/1x8x8-3x4x4/2026-10-26-k8_16-k4_16-tclfix_1025/`

新映像 SHA-256 為 `75f982b4161bbf7351e018d1652439d4d0aeb2275b3590281dc080ec5f5bc1a4`。本次燒錄與測試紀錄詳見第 17 節。以下保留 **2026-10-25 歷史封存**的全部參數與重現指令；實板包裝腳本仍使用該舊映像，不會自動切換至新封存。

```text
artifacts/fpga/tile_pipelined/1x8x8-3x4x4/2026-10-25-k8_16-k4_16/systolic_dma_top_1x8x8_3x4x4.bit
SHA256 bd8aba7212a24347eba1dc497204dd6eb5117e8a8ff77094fd94c496a41f3db5
```

| 欄位（`fpga_config.txt`） | 歷史封存值 |
|---|---|
| 加速器群 | `NUM_8X8=1`（8×8）、`NUM_4X4=3`（4×4）、`N=8` |
| K_MAX | `K_MAX_8X8=16`、`K_MAX_4X4=16`、`K_DIM=16` |
| 運算元路徑 | `VARIANT=v1`、`USE_V2=0` |
| 主機協定 | `HOST_SLAB_BYTES=1024`、`COMPLETION_HEADER=1`、`PIPELINE_DEPTH=4`、`DDR_BYTES=536870912` |
| RTL | `RTL_GIT_REV=229fe2431c0c72d25e6654a88e8e5d47c697301b`、`RTL_TREE_DIRTY=1`；僅 Git 版本不足以辨識該次建置的完整 RTL 狀態 |
| 實作 | `IMPL_PLACE_DIRECTIVE=ExtraTimingOpt`、`IMPL_ROUTE_DIRECTIVE=Explore`、`DPTI_CLKOUT0_PHASE=0.0`；建置時間 `2026-10-25T13:08:22+0800`，Vivado v2026.1 |
| 時序（繞線後報告） | WNS **+0.099 ns**、TNS 0、WHS **+0.016 ns**、THS 0，違反時序的端點為 0；報告確認所有使用者指定的時序限制均滿足。`TIMING_MET=1` |
| DPTI I/O 檢查關卡（`dpti_io_summary.txt`） | **PASS**：FT2232H 無未約束連接埠，所有 DPTI 腳位的建立／保持時間餘裕為正，I/O 暫存器位於 ILOGIC/OLOGIC |
| 資源使用量（佈局後報告） | Slice LUTs 116852 / 133800（87.33 %）、Slice Registers 157382 / 267600（58.81 %）、Slices 33226 / 33450（99.33 %）、Block RAM tiles 40 / 365（10.96 %）、DSPs 672 / 740（90.81 %） |

舊封存的 `.bit`、`.ltx`、組態、報告與硬體驗證紀錄已納入 Git；全域 `*.bit` / `*.log` 忽略規則不會排除已追蹤檔案。從 Repository 根目錄驗證全部舊封存檔案：

```bash
(cd artifacts/fpga/tile_pipelined/1x8x8-3x4x4/2026-10-25-k8_16-k4_16 && sha256sum -c SHA256SUMS)
```

舊 `fpga_config.txt` 記錄 `BITSTREAM=systolic_dma_top_1x8x8_3x4x4.bit` 與 SHA256，已由 `SHA256SUMS` 涵蓋，內容保持不變。曾確認 Runtime 可直接接受此檔：

```bash
SYSTOLIC_FPGA_CONFIG="$PWD/artifacts/fpga/tile_pipelined/1x8x8-3x4x4/2026-10-25-k8_16-k4_16/fpga_config.txt" \
  build/bin/runtime-dispatch-test --print-config
```

輸出逐行包含 `K_MAX_8X8=16`、`K_MAX_4X4=16`、`SLAB_BYTES=1024`、`PIPELINE_DEPTH=4`、`COMPLETION_HEADER=1`，以及 `SOURCE=<configuration path>`。此檢查只驗證主機剖析，不驗證已燒錄 FPGA 的身分。

歷史文件審查沒有重新產生 bitstream；本次亦未重新啟動 Vivado Build，而是驗證既有新建置產物、實際燒錄並重新執行實板測試。

## 6.1. 原始碼建置與來源追溯限制

使用封存映像與重新建置 RTL 是兩個程序。**對 2026-10-25 舊映像而言**，其 `RTL_GIT_REV=229fe2431c0c72d25e6654a88e8e5d47c697301b`、`RTL_TREE_DIRTY=1`，且缺少完整未提交差異／原始碼快照。取出該基礎版本無法恢復已驗證的 Revision 2 實作；10/25 映像的精確來源身分與重建仍為 **UNVERIFIED**。原專案引用另一份工作目錄的 `tile_pipelined` 原始碼；將目前建置輸入加入 Git，不能證明它們就是當時的內容。本次新建置的驗證不會補足舊映像來源缺漏，也不代表兩個二進位檔相同。

目前兩個建置腳本共用邏輯，但保留以下目錄差異；MIG 位置已修正為兩者各自的 `dma/ip/ddr3/`：

| 輸入／輸出 | `tile_pipelined` | `heterogeneous_fleet` |
|---|---|---|
| Tcl，相對於 Repository | `rtl/multi_200t/tile_pipelined/dma/dma_top_build.tcl` | `rtl/multi_200t/heterogeneous_fleet/dma/dma_top_build.tcl` |
| MIG XCI 與相鄰 `nexys_video_mig_axi128.prj`，相對於各流程根目錄 | `dma/ip/ddr3/` | `dma/ip/ddr3/` |
| Vivado 輸出上層目錄 | `$HOME/work/vivado/tile_pipelined/` | `$HOME/work/vivado/heterogeneous_fleet/` |
| 浮點 IP，相對於各流程根目錄 | `ip/fp32/` | `ip/fp32/` |
| 約束檔，相對於各流程根目錄 | 先 `dma/nexys_video_dma_top.xdc`，再 `dma/nexys_video_dpti_timing.xdc` | 相同相對路徑 |

`PROJ_DIR` 尾端為 `systolic_dma_<variant>_k8_<K8>_k4_<K4>_<NUM_8X8>x8x8_<NUM_4X4>x4x4`，設定標籤時另加 `_<SYSTOLIC_PROJ_TAG>`。兩個流程不共用專案目錄。每專案 `.lock` 防止同時建置；取得鎖之前會建立新的輸出上層目錄。建置會將既有專案移至 `.prev` 並移除較舊 `.prev`，因此請用新標籤保留既有專案；首次建置失敗仍可能留下輸出目錄。

兩者均指定元件 `xc7a200tsbg484-1` 與開發板 `digilentinc.com:nexys_video:part0:1.2`。外部 Digilent 定義檔請安裝至 `$HOME/work/vivado/vivado-boards/new/board_files`，或透過 Vivado 的開發板 Repository 提供；本 Repository 未隨附。已追蹤 MIG XCI 使用可搬移的 PRJ 檔名。因 MIG 自訂過程會切換目錄，`prepare_mig_xci` 在匯入前寫入建置目錄內的 XCI，將 PRJ 改為目前工作目錄的絕對路徑。浮點 IP 匯入與 RTL／約束／掛鉤新增使用 Tcl 串列，確保含空白的路徑仍視為單一檔名。

目前清單有 44 個 RTL 檔，包含 DPTI 時脈／鏈路控制器、sync245 PHY/FIFO 與匯流排仲裁器；會載入兩份 XDC 與保持時間／DRC 掛鉤。VIO 有 25 個輸入探針（probe 4 為 8 位元，其餘 32 位元）及兩個輸出探針（4 與 1 位元），與頂層相符。建置設定 `USE_EXTERNAL_SCHEDULER=1`、`USE_LEGACY_JOB_PORTS=0`，不含 UART。

**目前 `read` / `sweep` 模式入口已恢復，不再於參數剖析時一律拒絕。** 它們仍保留早期 VIO 假設：`read_vio` 呼叫 `arm_run`，`sweep` 重複此操作；不是單純唯讀查詢，也不能假定 VIO 重新執行會提交外部工作或重設累計計數器。本次未執行或驗證這兩個模式，狀態為 **UNVERIFIED**；不得作為正式數值驗證方法。數值驗證請用 Runtime／E2E，Tcl 內保留的早期資料種入輔助程式不是本次已驗證路徑。

原始碼 FPGA 建置為選用且耗時操作。以下保留歷史審查提供的建置指令，**該次審查未執行**；本次驗證亦未重新執行它。不能單憑指令存在宣稱合成、時序或硬體通過：

```bash
cd ~/work/systolic-mlir
# 將 VIVADO 設為已安裝執行檔；選擇其中一個原始碼流程。
VIVADO=/path/to/Vivado/bin/vivado
BUILD_TCL=rtl/multi_200t/tile_pipelined/dma/dma_top_build.tcl
# 另一個選項：BUILD_TCL=rtl/multi_200t/heterogeneous_fleet/dma/dma_top_build.tcl
# 每次實驗請用新標籤；此處策略與封存相同，
# 但無法重建缺失的未提交原始碼快照。
SYSTOLIC_PROJ_TAG=source_review_new \
SYSTOLIC_PLACE_DIRECTIVE=ExtraTimingOpt SYSTOLIC_ROUTE_DIRECTIVE=Explore \
  "$VIVADO" -mode batch -source "$BUILD_TCL" -tclargs build v1 16 16 1 1 3
```

位置參數為 `mode variant K8 K4 n_inv NUM_8X8 NUM_4X4`；`n_inv` 是早期 VIO 次數，不是外部 DPTI 工作數。K8/K4 預設 16，接受十進位 1..16376；v2 另要求至少 16 且為 8 的倍數。即使 Tcl 接受其他非空加速器群幾何，Runtime 只支援 1x8x8 + 3x4x4。

預設策略為 `AltSpreadLogic_high` / `Explore`，不是封存使用的 `ExtraTimingOpt` / `Explore`。覆寫變數為 `SYSTOLIC_PLACE_DIRECTIVE`、`SYSTOLIC_ROUTE_DIRECTIVE`、`DPTI_CLK_PHASE`（-180..180 度）與選用的繞線檢查點 `SYSTOLIC_INCR_REF`。省略模式時為 `all`（建置並燒錄）；只建置時務必明寫 `build`，指定專案標籤則禁止 `all`。

合成後須先通過時脈群組／I/O 延遲檢查，才進入實作；繞線後強制檢查 DPTI I/O／時脈。DPTI XDC 的 PCB 走線延遲與抖動上界標記為假設，因此時序 PASS 以這些假設為條件，不是走線延遲實測。

新 `.bit` 旁的 `fpga_config.txt` 記錄幾何、完成標頭、512 MiB DDR、佇列深度、映像 SHA、Git 版本／未提交狀態、時序與策略。Runtime 剖析幾何、標頭、DDR 與深度，由 K_MAX 推導資料區塊大小，忽略來源追溯欄位；組態紀錄本身不證明數值正確，也不構成完整來源快照。

2026-10-25 曾以正式 `build_body` 執行 Vivado 2026.1 匯入／組態預檢，在重新定位、包含空白路徑的工作目錄通過。MIG、浮點加法／乘法、VIO 均存在且 `IS_LOCKED=0`；已檢查原始碼／XDC／掛鉤註冊、泛型參數與策略。當時攔截 `generate_target`、`launch_runs`，因此 IP 輸出產物、合成、實作、繞線後關卡與 FPGA 操作當時仍為 **UNVERIFIED**。匯入 PASS 不等於 FPGA 重建成功；本次新產物的後續證據另見第 17 節。

## 7. FPGA 燒錄

以下是**舊封存映像**的燒錄方式；本次新映像使用第 17 節的指定專案／封存路徑。

```bash
cd ~/work/systolic-mlir

bash scripts/reproducibility/run_board.sh verify-program   # 確認 .bit 存在且 SHA256 與封存相符：MATCH
XILINX_VIVADO="/path/to/Vivado" bash scripts/reproducibility/run_board.sh program
```

`verify-program` 確認 `artifacts/fpga/tile_pipelined/1x8x8-3x4x4/2026-10-25-k8_16-k4_16/systolic_dma_top_1x8x8_3x4x4.bit` 存在且 SHA256 為 `bd8aba72…41f3db5`（第 6 節），回報 MATCH（一致）、MISMATCH（檔案不同）或 NOT VERIFIED（目前工作目錄缺少已追蹤檔案）。`program` 僅在 MATCH 後以 Vivado 硬體管理員燒錄該檔：

```bash
vivado -mode batch -nojournal \
  -source scripts/reproducibility/program_bit.tcl \
  -tclargs artifacts/fpga/tile_pipelined/1x8x8-3x4x4/2026-10-25-k8_16-k4_16/systolic_dma_top_1x8x8_3x4x4.bit
```

`program_bit.tcl` 選取含 xc7a200t 的 JTAG 目標。FT2232H 會註冊兩個目標，只有一個連到 FPGA；另一個顯示 `No devices detected` 並略過。燒錄後裝置若未回報 DONE，腳本以非零狀態結束。此流程不需 Vivado 專案，不會合成、實作或寫出 bitstream；具有封存 `.bit` 的主機均使用同一檔案。

**不要混淆封存燒錄與專案燒錄。** `dma_top_build.tcl -tclargs program v1 16 16 1 1 3` 會燒錄第 6.1 節指定流程輸出專案的第一個 `.bit`，不是自動選封存檔；內容取決於本機建置歷史與 `SYSTOLIC_PROJ_TAG`。自行建置的 bitstream 可用此步驟，但須先核對所選專案與 SHA；要用舊封存才使用 `run_board.sh program`。本次依指示使用新 tag 的專案燒錄，並已比對新 SHA。

2026-10-22 歷史紀錄曾回報：當時預設 10/19 映像（SHA 前綴 `18329505…`）的 `verify-program` MATCH、燒錄 DONE=1，接著執行第 15 節附日期的測試。它們不驗證目前 10/25 舊封存的燒錄；該封存另有自己的 `hardware-validation/` 證據與限制。

DPTI 沒有暫存器讀取協定，FPGA 無法回報所載入 bitstream 身分；實板測試信任最後一次燒錄的映像。如有疑慮，測試前重新燒錄；燒錄本身只需數秒。

## 8. 實板模式

以下包裝腳本使用既有 10/25 封存；新映像流程見第 17 節。

```bash
cd ~/work/systolic-mlir
# 1. 建置 systolic-cc 與 Runtime（第 5 節）
bash scripts/reproducibility/build.sh
# 2. 檢查需求、組態、封存雜湊與 USB 上的 FT2232H
bash scripts/reproducibility/run_board.sh check
# 3. 燒錄指定 bitstream（第 7 節）
bash scripts/reproducibility/run_board.sh verify-program
XILINX_VIVADO="/path/to/Vivado" bash scripts/reproducibility/run_board.sh program
# 4. 實板 E2E 測試（既有框架與封存組態）
bash scripts/reproducibility/run_board.sh e2e                  # = test/e2e/run_e2e.sh --backend board --quick
bash scripts/reproducibility/run_board.sh e2e --quick --mode async # 非同步 Runtime，相同快速案例
# 5.-7. 編譯 C GEMM、在 FPGA 執行，並與 CPU 參考結果比對
bash scripts/reproducibility/run_board.sh c test/reproducibility/gemm_example.c
```

`run_board.sh e2e` 會設定環境後執行既有測試：

```bash
cd ~/work/systolic-mlir
. scripts/reproducibility/env.board.sh
bash test/e2e/run_e2e.sh --backend board --quick
# 省略 --quick 可執行全部產生案例；包裝腳本只在無參數時加入 --quick。
```

只有在沒有其他程式使用開發板或 FT2232H 時才可執行。

**證明由 FPGA 而非 CPU 計算的依據**

- `run_e2e.sh` 使用的 `systolic-matmul` 沒有 CPU 退回路徑。各案例回報 DPTI 收送位元組數；Runtime 階段 `runtime-dispatch-test` 直接送原始分塊至裝置 0 至 3 並檢查結果。
- `run_board.sh c` 印出 `ldd` 載入的系統 libftdi1，且不應出現 `[libftdi_cosim]`。驅動程式逐 GEMM 顯示 `[systolic-cc] FPGA Executed: GEMM #N`，只有真實硬體 Runtime 完成所有規劃呼叫且無失敗時才會出現；否則橋接程式復原核心寫入、改用 CPU 重跑並顯示 `CPU Executed`。另印出 `[fpga-async] timing` 的 `completed=`、`bytes_to_fpga=` / `bytes_from_fpga=`。
- 若沒有 GEMM 顯示 `FPGA Executed`，腳本以 3（`NOT ON THE FPGA`）結束。

## 9. 不使用 FPGA 的共同模擬

**已支援 C 至 RTL 共同模擬。** 以下元件均在 Repository 中；歷史執行證據與限制列於第 15 節。

| 橋接層 | 檔案 | 行為 |
|---|---|---|
| 編譯器產生的呼叫 | systolic-cc → `fpga_tile_submit` / `fpga_gemm_wait`（非同步），或 `fpga_matmul_tiled_auto_scheduled`（同步） | 與實板使用相同執行檔（`--runtime=board`） |
| Runtime | `runtime/fpga_async_runtime.cpp`、`fpga_matmul_tiled.cpp`、`dpti_ftdi.cpp` | 不修改打包、工作描述子與完成紀錄邏輯 |
| DPTI 介面 | `test/e2e/cosim/libftdi_cosim.c`（LD_PRELOAD） | 取代 libftdi1 呼叫，透過 UNIX 通訊端轉送位元組串流 |
| 模擬 FT2232H | `test/e2e/cosim/dpti_cosim.cpp`、`tb_dpti_cosim.sv`、`dpti_protocol_check.svh` | 通訊端與同步 FIFO 腳位互接，並檢查協定 |
| RTL | 工作目錄的 `rtl/multi_200t/tile_pipelined/`，清單明列於 `test/e2e/cosim/build.sh` | 模擬目前 RTL；與舊封存未提交原始碼的精確一致性為 UNVERIFIED（第 6.1 節） |
| 被取代的 Xilinx IP | `test/e2e/cosim/fp_ieee.sv`（另含 `fp_ieee_dpi.cpp`）、`tb/mig_axi_model.sv`（1 MiB AXI DDR 模型）、`tb/xil_stubs.sv` | 浮點乘加、MIG/DDR3、基本元件與 VIO |

```bash
cd ~/work/systolic-mlir
bash scripts/reproducibility/build.sh                     # systolic-cc 與 Runtime
bash scripts/reproducibility/run_cosim.sh build          # Vcosim, libftdi_cosim.so, fpga_config.txt
bash scripts/reproducibility/run_cosim.sh check          # 不需 FPGA、USB 裝置或 Vivado
bash scripts/reproducibility/run_cosim.sh e2e            # = run_e2e.sh --backend cosim --quick
bash scripts/reproducibility/run_cosim.sh c test/reproducibility/gemm_example.c
```

`run_cosim.sh c` 執行：

1. 啟動 `Vcosim +socket=<short /tmp path> +max_sessions=0`；AF_UNIX 路徑上限約 107 位元組。
2. 用 `systolic-cc --runtime=board --fp-reassociation`，依共同模擬的 `fpga_config.txt` 編譯。
3. 設定 `LD_PRELOAD=<cosim>/libftdi_cosim.so SYSTOLIC_DPTI_COSIM_SOCKET=<socket>` 後執行。
4. 依 PID 停止 Vcosim。
5. 從 `cosim.log` 印出模擬本身的會話統計。例如歷史 V1 範例的原始輸出為 `cosim: session 1 closed (host closed): host->fpga 60318 B received, 60318 B consumed by the RTL, fpga->host 6624 B`。位元組數取決於分解與 Runtime 模式，不是 V2 的固定預期值；`[libftdi_cosim] connected to RTL co-simulation at …` 確認使用模擬器而非 USB 裝置。

相同流程的手動指令：

```bash
cd ~/work/systolic-mlir
C=build/cosim; D=$(mktemp -d /tmp/systolic-cosim.XXXXXX)
L=$(sed -n "s|^LLVM_DIR:[A-Z]*=\(.*\)/lib/cmake/llvm\$|\1|p" build/CMakeCache.txt)   # LLVM 24 建置目錄
$C/Vcosim +socket=$D/dpti.sock +max_sessions=0 > $D/cosim.log 2>&1 &  VPID=$!
sleep 1
SYSTOLIC_FPGA_CONFIG=$PWD/$C/fpga_config.txt \
  build/bin/systolic-cc --runtime=board --fp-reassociation --llvm-build "$L" \
  test/reproducibility/gemm_example.c -o $D/gemm_example
LD_PRELOAD=$PWD/$C/libftdi_cosim.so SYSTOLIC_DPTI_COSIM_SOCKET=$D/dpti.sock \
  SYSTOLIC_FPGA_CONFIG=$PWD/$C/fpga_config.txt $D/gemm_example
kill $VPID; grep '^cosim: session' $D/cosim.log
```

`--llvm-build` 指定 LLVM 24；否則 systolic-cc 從 `LLVM_BUILD` 尋找，預設 `$HOME/work/llvm-project/build`。包裝腳本從 `CMakeCache.txt` 設定 `LLVM_BUILD`。

共同模擬的界線：

- 它證明未修改主機軟體堆疊後方的 RTL 可算出正確結果，不是特定開發板、bitstream 或速度的證據。
- Xilinx 浮點 IP 改用 IEEE 模型，MIG/DDR3 改用 AXI 模型。共同模擬與 Cost Model RTL 測試平台選用 `core/systolic_pe_bram.sv`；FPGA Tcl 選用 `core/systolic_pe.sv`（分散式累加器 RAM）。這是不同原始碼檔，Verilator 不驗證實體記憶體映射，不能據模擬宣稱 FPGA 原始碼清單逐位元組相同或繞線等價。
- `+max_sessions=0` 讓一個 Vcosim 服務多個程式；若程式在會話中途終止，須重啟模擬器。

## 10. 撰寫自己的 C GEMM

以 `test/reproducibility/gemm_example.c` 為範本。可卸載函式須符合：

- 回傳型別為 `void`，參數為固定維度且帶 **`restrict`** 的陣列；可能有記憶體別名的運算元不降階。
- 運算元為 **`float`**（fp32），FPGA 資料路徑為 binary32；`double` GEMM 留在 CPU。
- 巢狀迴圈為 `C[i][j] += A[i][k] * B[k][j]` 或等價仿射形式，詳見 `docs/AFFINE_GEMM_MAPPING.md`；允許先以獨立巢狀迴圈將 `C[i][j] = 0`。
- 除迴圈計數器外，不得有區域變數；超出支援 C 子集的函式維持 C。範例 `gemm_reference` 刻意使用區域變數，確保在 CPU 執行。
- 編譯時加 **`--fp-reassociation`**。卸載會將每個 K 分塊從零加總，再加至 C，改變 IEEE 加法順序；預設嚴格浮點模式會讓所有 GEMM 留在主機，`--explain` 會說明。
- M、N、K 可為任意支援的靜態尺寸：邊界分塊補零，K 依 K_MAX 分段歸約。`main` 與子集外程式維持原本 C。

用 `--explain` 查看各函式決策；以下保留工具原始診斷輸出，不翻譯程式輸出字串：

```text
note: 'gemm' is offloaded: 45 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet
note: 'gemm_reference' is outside the supported C subset and is not translated: local variables are not supported (only for-loop counters)
```

呼叫數取決於二分搜尋的成本模型。歷史範例中，預設 V2（`docs/COSTMODEL_V2.md`）為 45；`systolic-cc --cost-model v1` 的 V1 為 54。

## 11. 所需 C 標頭檔

**透過 systolic-cc 的 C 使用者：只需 C 標準函式庫。** 範例僅引入 `<stdio.h>` 與 `<math.h>`。systolic-cc 自行加入 `-I<repo>/runtime/offload`；其中 `systolic_offload.h` 是產生呼叫橋接程式使用的內部標頭，使用者不需引入。

**直接呼叫 DPTI Runtime 的開發者（不使用 systolic-cc）：**

| 標頭檔 | API |
|---|---|
| `runtime/fpga_matmul_tiled.h` | `fpga_matmul_tiled_auto_scheduled`（單裝置單分塊）、`fpga_runtime_get_config`、`fpga_runtime_get_stats`、`fpga_runtime_close` |
| `runtime/fpga_async_runtime.h` | `fpga_gemm_begin`、`fpga_tile_submit`、`fpga_gemm_poll` / `wait` / `wait_for` / `release`（docs/ASYNC.md） |

下列編譯／連結指令曾以單分塊程式在共同模擬中回報通過；歷史文件審查未獨立重跑：

```bash
gcc -std=c11 -I runtime my_dpti.c build/runtime/libsystolic_fpga_runtime.a \
    $(pkg-config --libs libftdi1) -lstdc++ -o my_dpti
SYSTOLIC_FPGA_CONFIG="/path/to/target/fpga_config.txt" ./my_dpti
```

既有範例為 `test/e2e/runtime_dispatch_test.cpp` 與 `runtime_async_test.cpp`，建置為 `build/bin/runtime-{dispatch,async}-test`。直接使用 Runtime 須自行處理 systolic-cc 原本負責的資料區塊與 K_MAX 限制：每分塊一次呼叫，裝置 0 的 M,N ≤ 8，裝置 1 至 3 的 M,N ≤ 4，且 1 ≤ K ≤ K_MAX。

## 12. 編譯與執行

建議透過腳本設定目標組態及第 4 節所列環境；實板包裝腳本仍使用舊封存：

```bash
bash scripts/reproducibility/run_board.sh c path/to/your.c    # FPGA
bash scripts/reproducibility/run_cosim.sh c path/to/your.c    # RTL 模擬器
# 可選擇將輸出目錄作為下一個參數。
```

輸出位於 `build/repro/{board,cosim}/<name>/`，包含執行檔 `<name>`、`compile.log`、`stdout.txt`、Runtime 追蹤 `run.log`，共同模擬另有 `cosim.log`。中間 IR 位於 `<name>.systolic/`。

底層 systolic-cc 指令：

```bash
SYSTOLIC_FPGA_CONFIG="/path/to/target/fpga_config.txt" \
  build/bin/systolic-cc --runtime=board --fp-reassociation --explain --verify your.c -o your
```

| 選項 | 意義 |
|---|---|
| `--runtime=board` | 連結真實 DPTI Runtime；相同執行檔可用於實板與共同模擬，執行時是否透過 `LD_PRELOAD` 載入 `libftdi_cosim.so` 決定後端 |
| `--runtime=soft` | 軟體 Runtime 模型，**不是 FPGA**；GEMM 回報 `CPU Executed` |
| `--fpga-config FILE` | 與 `SYSTOLIC_FPGA_CONFIG` 相同：目標的 K_MAX、完成紀錄與 Pipeline 深度 |
| `--submission auto\|sync\|async` | 預設 `auto`，當組態具有 `COMPLETION_HEADER=1` 時使用非同步 |
| `--verify` | 每次卸載呼叫另與 CPU 版本比對；不符以 4 結束 |
| `--explain` | 解釋每個函式的卸載決策 |

實板執行檔只需要 `SYSTOLIC_FPGA_CONFIG`。未設定時，Runtime 使用不含完成紀錄的內建預設組態；目前映像會傳送完成紀錄，因此 Runtime 拒絕結果，所有 GEMM 退回 CPU。

## 13. 數值驗證

- **程式本身：** `gemm_example.c` 以 `double` 計算 CPU 參考值，允許相對容許誤差 1e-5，印出最先不符的元素，並以 `PASS`（結束碼 0）或 `FAIL`（結束碼 1）回報。輸入為小整數，每個部分和均可在 fp32 精確表示，各種加總順序均逐位元一致；歷史觀測 `max |error|` 為 0。
- **驅動程式：** `systolic-cc --verify` 將每次卸載與相同函式的 CPU 編譯版本比對，不符以 4 結束。
- **E2E 測試框架：** `run_e2e.sh` 逐元素比對精確整數參考值，區分 PASS、MISMATCH、TRANSPORT、CONFIG 等狀態，見 `test/e2e/e2e_verdict.py`。
- **實板與共同模擬對照：** 歷史範例兩者位元組數相同：送至 FPGA 60318 B、讀回 6624 B、54 次呼叫。這是 V2 成為預設前的 V1 分解紀錄。

## 14. 問題排除

| 現象 | 原因／處理方式 |
|---|---|
| `check_env.sh`：`MISSING libftdi1`、`verilator`、`FT2232H` | 缺少系統套件或開發板；請安裝或連接，環境變數無法解決 |
| `no LLVM 24 build / clang 24 known` | 首次設定需 `build.sh --llvm-build DIR --clang CLANG` |
| `… is LLVM 18…; this tree needs LLVM/MLIR 24` | 不支援 LLVM 18，將 `--llvm-build` 指向 LLVM 24 建置目錄 |
| `'gemm' stays on the host: … --fp-reassociation` | 加上 `--fp-reassociation`，見第 10 節 |
| 實板 GEMM 顯示 `[systolic-cc] CPU Executed` | 某次呼叫失敗後改由 CPU 重算。查看 `run.log` 的 Runtime 回傳碼；`-3` 表示鏈路未開啟，可能未插板、鏈路占用或 USB 權限；`-9` 表示完成紀錄不符，可能組態與已載入映像不同 |
| `run_e2e.sh` 以 2（`CONFIG`）結束 | 缺少或錯誤的 `SYSTOLIC_FPGA_CONFIG`；使用相符腳本，或對新映像明確設定 |
| 共同模擬：`TRANSPORT rc=-3`、`cosim: socket path too long` | 通訊端路徑須短於約 100 位元組；腳本使用 `/tmp/systolic-cosim.*` |
| 共同模擬建置：`%Error: Internal Error: attempted to destroy locked Thread Pool` | 測試主機以 `-j 8` 遇到的 Verilator 5.020 問題。改用 `JOBS=6 bash scripts/reproducibility/run_cosim.sh build`；`JOBS` 屬於 `test/e2e/cosim/build.sh`，`build.sh --jobs N --cosim` 會轉交 |
| 程式中止後下一次共同模擬卡住 | 重新啟動 Vcosim；`run_cosim.sh c` 每次均啟動新程序 |
| `verify-program`：MISMATCH | 封存路徑檔案不是指定映像；取得正確檔案並驗證 `sha256sum -c SHA256SUMS`，見第 6 節；不要覆寫既有封存來掩蓋差異 |
| `verify-program`：NOT VERIFIED | 本機缺少已追蹤 `.bit`；恢復正確版本／檔案並驗證完整清單 |
| `program` 所有目標均顯示 `no xc7a200t on this chain`，或 `cannot reach hw_server` | 開發板未開機、PROG micro-USB 不在本機，或缺少 Vivado 纜線驅動，見第 2 節 |
| 其他程式使用鏈路後，實板結果陳舊或錯誤 | FT2232H 一次只能由一個程式使用，待開發板空閒後重跑 |

## 15. 歷史實驗重現檢查紀錄

下表保留開發主機的歷史報告：第 1–12、15、16 列日期為 2026-10-21（V1，C 範例 54 次呼叫）；第 13–14b 列為 2026-10-22（V2、當時預設 10/19 映像）。這些 PASS **對目前工作目錄的獨立驗證仍為 UNVERIFIED**，不是新的 10/25 執行結果；本次 10/26 新編譯器／新映像測試另列第 17 節，不回填歷史列。歷史指令行為可能已變，重現時使用前述目前指令。

| 編號 | 檢查 | 指令 | 歷史結果 |
|---|---|---|---|
| 1 | Shell 語法 | `bash -n scripts/reproducibility/*.sh` | 紀錄為 PASS，但此指令僅剖析第一份腳本，未建立完整語法覆蓋；shellcheck NOT RUN |
| 2 | 環境腳本 | `env -i … bash scripts/reproducibility/check_env.sh all` | PASS：每項 OK，未繼承 systolic 變數 |
| 3 | 隔離 | `check_env.sh board`，事先設定錯誤 `SYSTOLIC_FPGA_CONFIG`、共同模擬 `LD_PRELOAD`、`LD_LIBRARY_PATH`，並讓 llvm-18 位於 `PATH` 最前方 | PASS：均被取代或清除，使用封存組態 |
| 4 | 乾淨環境全新建置 | `env -i … SYSTOLIC_BUILD=<new dir> bash scripts/reproducibility/build.sh --llvm-build … --clang … --cosim` | PASS：58/58 建置步驟與 Vcosim，4 分 37 秒 |
| 5 | 乾淨環境增量建置 `build/` | `env -i … bash scripts/reproducibility/build.sh` | PASS |
| 6 | 僅由腳本設定 `SYSTOLIC_FPGA_CONFIG` | 第 7 至 11 項於 `env -i` 執行，未手動匯出變數 | PASS |
| 7 | C 範例軟體模型（lit） | `lit build/test/reproducibility build/test/SystolicCC` | PASS 28/28 |
| 8 | C 範例共同模擬（全新建置、`env -i`） | `run_cosim.sh c test/reproducibility/gemm_example.c` | PASS：GEMM #1 FPGA Executed，54 次呼叫，0/480 不符；Vcosim 會話輸入 60318 B、輸出 6624 B |
| 9 | 共同模擬 E2E（全新建置、`env -i`） | `run_cosim.sh e2e` | PASS：26/26，組態拒絕 PASS、Runtime 層 PASS，已回報注入的寫入失敗 |
| 9a | 預設 `build/cosim` 重新建置（`env -i`） | `JOBS=6 run_cosim.sh build`，接著 `run_cosim.sh e2e`、`run_cosim.sh e2e --quick --mode async`、`run_cosim.sh c …` | PASS：同步 26/26、非同步 26/26 + RUNTIME ASYNC PASS、範例 PASS；首次預設 `-j 8` 遇到 Verilator 5.020 內部錯誤（第 14 節） |
| 9b | 第 9 節手動共同模擬指令逐字執行（`env -i`） | — | PASS |
| 10 | 實板 E2E（`env -i`） | `run_board.sh e2e` | PASS：實板後端 26/26、組態拒絕 PASS、Runtime 層 PASS；硬體組態未獨立查驗 |
| 10a | 實板非同步 Runtime E2E（`env -i`） | `run_board.sh e2e --quick --mode async` | PASS：26/26、runtime-async-test PASS；模式 F 設計上於實板為 NOT APPLICABLE |
| 11 | C 範例實板執行（`env -i`） | `run_board.sh c test/reproducibility/gemm_example.c` | PASS：GEMM #1 FPGA Executed，54 次呼叫完成、0 失敗、0/480 不符，系統 libftdi1 |
| 12 | 直接 DPTI API | 以 `fpga_matmul_tiled_auto_scheduled` 在共同模擬執行單一 8×16×8 分塊 | PASS |
| 13 | 預定燒錄映像（2026-10-22、`env -i`） | `run_board.sh verify-program` | MATCH（`18329505…`） |
| 14 | 燒錄（2026-10-22、`env -i`） | `run_board.sh program` | PASS：封存檔已燒錄，xc7a200t_0，DONE = 1 |
| 14a | 燒錄後實板 E2E（2026-10-22、`env -i`） | 歷史 `run_board.sh e2e`（當時記為全部案例，目前預設快速測試）、`run_board.sh e2e --quick --mode async` | PASS：43/43；非同步 26/26（Cost Model V2） |
| 14b | 實板 C 範例（2026-10-22、`env -i`） | `run_board.sh c test/reproducibility/gemm_example.c` | PASS：GEMM #1 FPGA Executed，45 次呼叫（Cost Model V2）、0/480 不符 |
| 15 | 完整 lit 測試集 | `lit build/test`，開發者 Shell 的 `PATH` 含 LLVM 24 `count`/`not` | PASS 154/154 |
| 16 | 空白字元檢查 | `git diff --check` | PASS |

### 2026-10-25 文件審查與證據界線

請逐份檢查腳本語法，不要把萬用字元交給單一 `bash -n`：

```bash
for script in scripts/reproducibility/*.sh; do
  bash -n "$script" || exit 1
done
```

10/25 封存的 `README.md`、`hardware-report.md` 與 `hardware-validation/` 含壓力測試 2560/2560、同步 26/26、非同步 52/52 的歷史證據。其 README 明確指出編譯器案例使用另一工作目錄、成本模型支援不同的編譯器；這些歷史結果不驗證目前編譯器的確切建置，也未直接觀測內部循環輪詢公平性／提早重用記憶體分組。

Cost Model R2 驗證入口（Repository 根目錄）：

```bash
python3 eval/cost_model_v2/check_r2_invariants.py --tool build/bin/systolic-cost
JOBS=2 bash eval/cost_model_v2/build_bench.sh 16 16 v1 /tmp/systolic-r2-bench
python3 eval/cost_model_v2/validate_r2.py --bench /tmp/systolic-r2-bench/obj/tbrun --out /tmp/systolic-r2-results
# 選用的完整多 K／運算元路徑掃描，工作量明顯較大：
python3 eval/cost_model_v2/run_validation.py --out /tmp/systolic-cost-full
```

測試平台使用目前 `heterogeneous_fleet` RTL 與 `tb_fleet_jobs_r2.sv`。`docs/COSTMODEL_V2.md`、`eval/cost_model_v2/README.md` 仍含早期固定優先權／測試平台描述；應以目前實作與 R2 差異比對流程判定行為。31 案例特定差異比對測試集不是完整多 K 掃描，也不是硬體校準結果；不得為彌補缺少實測而調整 H=95、reduceCycles=69、feedGap=26 或 DMA 常數。

| 歷史審查項目 | 該次證據與狀態 |
|---|---|
| bitstream 完整性／預設組態 | PASS：指定 SHA 與全部 314 個封存清單項目通過；實板包裝腳本使用相同路徑／組態 |
| Vivado 腳本預檢 | 僅匯入／專案組態 PASS，範圍見第 6.1 節；該次完整 FPGA 建置與繞線後關卡為 UNVERIFIED |
| 精確重建 10/25 原始碼 | UNVERIFIED：未提交快照缺失 |
| 該次實板 Runtime/E2E | UNVERIFIED：該次審查未燒錄或執行實板，剖析器與包裝腳本檢查 PASS |
| 共同模擬 | 較早 10/25 稽核紀錄有全新建置、C 範例、同步／非同步 26/26 PASS；Tcl 審查未重跑，不是封存映像證據 |
| Cost Model | PASS：重用較早 10/25 稽核的既有 RTL 測試平台執行檔，重跑特定 R2 不變量與 31 個精確差異比對案例；該次未重新建置測試平台，完整掃描與實板總完成時間驗證為 UNVERIFIED |
| 歷史硬體 | 封存含附日期的數值證據與編譯器／來源限制；該次對目前工作目錄的完整驗證為 UNVERIFIED |

該次暫存審查紀錄位於審查主機 `/tmp/fpga-flow-review-9ftwwgdn/`，包含 `tile-import-retry.vivado.log`，未封裝為 Repository 產物；後續審查應保留自己的紀錄。該次未執行合成、實作、bitstream 產生或燒錄。本次新增驗證另見第 17 節。

自行執行時的檢查表：

- [ ] `check_env.sh all`（或 `board` / `cosim`）回報 OK。
- [ ] `build.sh` 成功；首次須提供 `--llvm-build` / `--clang`。
- [ ] 實板 `.bit` 與 `SHA256SUMS` 相符；若使用包裝腳本，燒錄前 `verify-program` 為 MATCH。
- [ ] `run_board.sh e2e` / `run_cosim.sh e2e` 顯示 `E2E: PASS`；新映像則用明確組態的直接 E2E。
- [ ] `run_*.sh c test/reproducibility/gemm_example.c` 顯示 `FPGA Executed: GEMM #1` 與 `PASS`。
- [ ] 記錄 `git rev-parse HEAD`、bitstream SHA256，以及 `build/repro/`、`build/e2e/` 或獨立輸出目錄的完整紀錄。

## 16. 已知限制

- **10/25 舊封存不是完整來源快照。** `.bit` 已在 Git 且可驗證雜湊；歷史未提交 RTL 狀態仍為 UNVERIFIED（第 6.1 節）。新映像的獨立重建與測試不能證明舊二進位精確重現。
- **依目標選擇燒錄入口。** `run_board.sh program` 固定燒錄舊封存；`dma_top_build.tcl program` 燒錄本機專案 `.bit`，取決於建置歷史與 tag（第 7 節）。本次指定 tag 的新映像已獨立核對，不應混用。
- **無法查詢目前載入映像。** DPTI 沒有暫存器讀取，實板測試依賴燒錄流程與組態。
- **工具鏈位置依主機而異。** LLVM 24 與 clang 24 首次提供給 `build.sh` 後從 `CMakeCache.txt` 讀回，未隨附工具鏈；`env -i` 不隔離已安裝套件（第 3 節）。
- **共同模擬不是實板。** 使用 IEEE 浮點模型取代 Xilinx 浮點 IP、1 MiB AXI DDR 模型取代 MIG/DDR3，以 Verilator 速度執行，不提供實板時序證據。
- **C 子集。** 僅 fp32、固定維度 `restrict` 陣列、卸載函式不得有一般區域變數，卸載須允許 `--fp-reassociation`。廣播與其他非 GEMM 模式留在 CPU（`docs/AFFINE_GEMM_MAPPING.md`）。
- **鏈路單一使用者。** 實板測試須獨占 FT2232H。
- **lit 輔助工具。** 需要 `lit` 與由 LLVM 24 建置的 `count` / `not`，腳本不提供；歷史純 `env -i` Shell 下的 `check-systolic` 為 NOT RUN。本次完整測試使用第 17 節明列的新工具與環境。

## 17. 2026-10-26 新原始碼重建與實板整合驗證

### 17.1. 驗證結論與適用範圍

本次實際執行 FPGA 燒錄、兩輪實板 E2E、乾淨編譯器建置與測試，以及新編譯器產生的 C 程式實板執行。以下結論均對應獨立封存中的紀錄：

| 項目 | 結論 | 證據 |
|---|---|---|
| FPGA Source Rebuild | VERIFIED | 新專案完整建置紀錄、合成與實作紀錄、bitstream、乾淨 Git 版本組態及最終時序／DPTI 報告 |
| FPGA Programming | PASS | Vivado `program_hw_devices` 成功、啟動狀態 HIGH，結束碼 0 |
| FPGA Board E2E | PASS | 兩輪各四組實板測試全部通過；主要整合結論採第二輪新編譯器結果 |
| Compiler Source Rebuild | VERIFIED | 獨立全新目錄完成 CMake 設定與 59/59 建置步驟，未重用既有 `build/bin` |
| Compiler Regression Tests | PASS | 新工具執行 lit/FileCheck 172/172，FAIL 0、SKIP 0 |
| Compiler → FPGA End-to-End | PASS | 新 `systolic-cc` 與新 Runtime 的四個 C 程式執行組合均通過，規劃中的 FPGA 函式執行 16 次，非預期 CPU fallback 0 |
| 舊 bitstream 二進位精確重現 | 未驗證為相同，MATCH: NO | 新舊 SHA-256 不同；不標示 Bitstream SHA-256 Reproduction: VERIFIED |

上述 VERIFIED 僅涵蓋本次指定來源、組態與測試，不表示所有可能輸入、內部時序重疊或完整硬體成本模型均已驗證。本次未重新啟動耗時 Vivado Implementation，而是核對已完成的新原始碼建置、燒錄並實測其產物。

完整封存：[2026-10-26 新映像 README](artifacts/fpga/tile_pipelined/1x8x8-3x4x4/2026-10-26-k8_16-k4_16-tclfix_1025/README.md)、[硬體驗證報告](artifacts/fpga/tile_pipelined/1x8x8-3x4x4/2026-10-26-k8_16-k4_16-tclfix_1025/hardware-report.md)。

### 17.2. 來源、時序、燒錄與組態

- Git HEAD：`075640b4872d4240a99d89af625d7dd7aa3fc207`；建置組態記錄 `RTL_TREE_DIRTY=0`。開始驗證時已追蹤檔案無修改，僅有既存未追蹤 `build_recovery/`；後續新增封存與本文修改不涉及 RTL／Compiler／Runtime／Cost Model／Tcl／測試原始碼。
- 使用者指定的 `~/work/systolic-mlir-heterogeneous` 不存在。實際 Vivado XPR 引用 `~/work/systolic-mlir`，其 HEAD 與新映像組態一致，故使用該實際目錄。
- 專案標籤：`shared_wb_r2_tclfix_1025`；`v1 16 16 1 1 3`；佈局 `ExtraTimingOpt`、繞線 `Explore`、DPTI 相位 0.0。
- Vivado v2026.1（64 位元），SW Build 6511674；原建置於 2026-10-25T20:44:12 啟動，2026-10-26T01:21:49+0800 完成，`write_bitstream` 成功，實作紀錄為 0 Errors、0 Critical Warnings。
- 組態精確時序：WNS **+0.020144 ns**、TNS **0.000 ns**、WHS **+0.002302 ns**、THS **0.000 ns**；繞線報告顯示所有使用者指定限制均滿足。`TIMING_MET=1`、`DPTI_IO_GATE=PASS`。報告以顯示精度列 WNS 0.020、WHS 0.002；保持時間含 0.150 ns 額外保守量，仍以 XDC 假設為條件。
- 新組態為 `NUM_8X8=1`、`NUM_4X4=3`、`K_MAX_8X8=16`、`K_MAX_4X4=16`、`HOST_SLAB_BYTES=1024`、`COMPLETION_HEADER=1`、`PIPELINE_DEPTH=4`、`DDR_BYTES=536870912`。

本次燒錄與測試使用原建置輸出的組態；以下指令會操作硬體，重現前須確保開發板與 USB 空閒：

```bash
cd ~/work/systolic-mlir
SYSTOLIC_PROJ_TAG=shared_wb_r2_tclfix_1025 \
  vivado -mode batch -nojournal -log /tmp/systolic-program-new.log \
  -source rtl/multi_200t/tile_pipelined/dma/dma_top_build.tcl \
  -tclargs program v1 16 16 1 1 3
export SYSTOLIC_FPGA_CONFIG="$HOME/work/vivado/tile_pipelined/systolic_dma_v1_k8_16_k4_16_1x8x8_3x4x4_shared_wb_r2_tclfix_1025/systolic_dma.runs/impl_1/fpga_config.txt"
```

實際燒錄從獨立暫存目錄執行、使用 Tcl 絕對路徑，保留 `programming.log` 與 `programming-console.log`，於 2026-10-26T08:21:09 結束。Runtime `--print-config` 已確認接受同一份新組態。封存的 `fpga_config.txt` 僅將 `BITSTREAM` 改為相鄰封存檔名；原始組態完整保留在 `provenance/fpga_config.original.txt`，原建置檔未改。

### 17.3. 乾淨編譯器建置與工具選擇

獨立 Build Directory：`/tmp/systolic-rebuild-validation-20261026-vrZKJ5/compiler-build`。它原先不存在，未覆蓋既有 `build/`。使用 LLVM/MLIR 24.0.0git、clang 24.0.0git，來源版本 `7615da10b75c57286a4032eac8bb43c1b8d2ba46`。

```bash
SYSTOLIC_BUILD=/tmp/systolic-rebuild-validation-20261026-vrZKJ5/compiler-build \
  bash scripts/reproducibility/build.sh \
  --llvm-build /home/steven-studio/work/llvm-project/build \
  --clang /home/steven-studio/work/clang24-build/bin/clang --jobs 4
cmake -S . -B /tmp/systolic-rebuild-validation-20261026-vrZKJ5/compiler-build \
  -DLLVM_EXTERNAL_LIT=/home/steven-studio/miniconda3/bin/lit
```

首次 CMake 設定與編譯均完整成功。建置出現既有 API 棄用與 `_GLIBCXX_USE_CXX11_ABI` 重複定義警告，沒有修改原始碼來消除或隱藏；完整內容保留於 `compiler-validation/compiler-configure-build.log`。另封存 CMakeCache、build.ninja、lit.site.cfg 與 Ninja 建置紀錄。

LLVM 24 建置目錄原先沒有 `count`／`not`。本次以同版本 LLVM 原始碼及 clang 24，將兩者編譯至獨立 `llvm24-test-utils/`；`not` 使用該 LLVM 24 的 Support 函式庫，沒有使用系統 LLVM 18 的輔助工具。完整指令與輸出見 `compiler-validation/llvm24-test-utils-build.log`，不曾修改 LLVM 或本專案原始碼。

新產物完整 SHA-256 列於 `compiler-validation/new-build-sha256.json`；選列如下：

| 產物 | SHA-256 |
|---|---|
| `systolic-cc` | `c97b1485fcfcf95437d1ca02279daeead9221788744ce6c66d1e0b6f3b526ec5` |
| `systolic-opt` | `2f4390f62885af1f716a8045e8732505573fef59b07ff64ad1b4ed5ad3993f20` |
| `systolic-matmul` | `35dba5b9546e98ff155956b0f41f10d1b6c09d7091a71083f2075c46088f6d00` |
| `libsystolic_fpga_runtime.a` | `37b1dccba2541de4d40bc75bcc80ddde2f13f3b770cbe2a0c65e06861a2cfa2b` |

### 17.4. 新編譯器迴歸與整合測試

```bash
PATH=/tmp/systolic-rebuild-validation-20261026-vrZKJ5/compiler-build/bin:/tmp/systolic-rebuild-validation-20261026-vrZKJ5/llvm24-test-utils:/home/steven-studio/work/llvm-project/build/bin:/usr/local/bin:/usr/bin:/bin \
  /home/steven-studio/miniconda3/bin/lit -a -j 4 \
  -o /tmp/systolic-rebuild-validation-20261026-vrZKJ5/compiler-lit-results.json \
  /tmp/systolic-rebuild-validation-20261026-vrZKJ5/compiler-build/test
cmake --build /tmp/systolic-rebuild-validation-20261026-vrZKJ5/compiler-build \
  --target check-e2e-config
python3 eval/cost_model_v2/check_r2_invariants.py \
  --tool /tmp/systolic-rebuild-validation-20261026-vrZKJ5/compiler-build/bin/systolic-cost
```

完整 lit/FileCheck **172/172 PASS、FAIL 0、SKIP 0**，耗時 66.81 秒：CFront 14、Systolic 114、SystolicCC 31、gemm_recognition 12、reproducibility 1。涵蓋仿射 GEMM 辨識／映射、Dialect 驗證、分塊／裝置選擇／排程、成本模型與編譯器／軟體 Runtime 整合。詳細案例與執行命令見 `compiler-validation/compiler-lit-results.json`、`compiler-lit.log`；實際命令對舊 `/home/steven-studio/work/systolic-mlir/build/bin/` 的引用為 0。

`check-e2e-config` 為 **109 PASS、0 FAIL、3 SKIP**。三項均因新目錄未建置 `libftdi_cosim.so`：缺少共同模擬函式庫、無鏈路的非同步 Runtime 測試，以及 Runtime 派送失敗後的模擬檢查。SKIP 不算 PASS，也未掩飾為失敗；本次未重建共同模擬，實板整合另由下節直接驗證。新 `systolic-cost` 的 R2 資源、循環輪詢、回應錯誤與輸出排空不變量檢查 **PASS**，未另宣稱重跑完整多 K 差異比對掃描。

### 17.5. 新編譯器與新 Runtime 的實板 E2E

所有測試使用新 `SYSTOLIC_FPGA_CONFIG`，清除繼承的模擬／Runtime 調整設定，使用系統 libftdi1 實際連接 FPGA。正式入口範例：

```bash
# 每次實驗請選新的輸出目錄；以下 Build Directory 為本次實際新建置位置。
bash test/e2e/run_e2e.sh --backend board \
  --build-dir /tmp/systolic-rebuild-validation-20261026-vrZKJ5/compiler-build \
  --quick --mode sync --out /tmp/systolic-new-quick-sync
bash test/e2e/run_e2e.sh --backend board \
  --build-dir /tmp/systolic-rebuild-validation-20261026-vrZKJ5/compiler-build \
  --regression --mode sync --out /tmp/systolic-new-regression-sync
bash test/e2e/run_e2e.sh --backend board \
  --build-dir /tmp/systolic-rebuild-validation-20261026-vrZKJ5/compiler-build \
  --mode sync --out /tmp/systolic-new-full-sync
bash test/e2e/run_e2e.sh --backend board \
  --build-dir /tmp/systolic-rebuild-validation-20261026-vrZKJ5/compiler-build \
  --regression --mode async --out /tmp/systolic-new-regression-async
```

實際完整命令、環境與起訖時間見封存 `compiler-validation/fresh-executions.json`；測試輸出位於 `hardware-validation/fresh-*/`，每案保留輸入、輸出及各層 IR。

| 新建置工具測試組 | 案例 PASS／總數 | CPU 比對元素 | Mismatch | 非預期逾時 | 非預期 Runtime 錯誤 |
|---|---|---|---|---|---|
| Quick，同步 | 26/26 | 1,068 | 0 | 0 | 0 |
| Regression，同步 | 52/52 | 15,875 | 0 | 0 | 0 |
| 完整 Sync | 43/43 | 7,008 | 0 | 0 | 0 |
| Regression，Async | 52/52 | 15,875 | 0 | 0 | 0 |
| 合計（包含重複案例的獨立執行次數） | 173/173 | 39,826 | 0 | 0 | 0 |

Runtime 組態拒絕與派送測試均 PASS；非同步測試各適用模式 PASS，模式 F 只適用模擬，記為 NOT APPLICABLE。模式 P 的 `wait_for(1 ms)` 預期回傳 -20，以及 V／H 的刻意拒絕／不完整歸約錯誤均依測試預期通過，不能計為非預期逾時或 Runtime 故障。

第一輪使用既有工具的四組測試亦為 26/26、52/52、43/43、52/52 PASS，獨立保留於不含 `fresh-` 前綴的目錄。新編譯器整合結論不依賴該輪；兩輪各 173 次執行，不是 346 個不同案例。

### 17.6. 新 systolic-cc 編譯 C 工作負載並在 FPGA 執行

```bash
/tmp/systolic-rebuild-validation-20261026-vrZKJ5/compiler-build/bin/systolic-cc \
  --build-dir /tmp/systolic-rebuild-validation-20261026-vrZKJ5/compiler-build \
  --llvm-build /home/steven-studio/work/llvm-project/build \
  --clang /home/steven-studio/work/clang24-build/bin/clang \
  --runtime=board --fp-reassociation --submission async --explain --verify \
  --keep-dir /tmp/systolic-c-new-ir test/reproducibility/gemm_example.c \
  -o /tmp/systolic-c-new
SYSTOLIC_RUNTIME_TRACE=1 SYSTOLIC_RUNTIME_TIMING=1 SYSTOLIC_DPTI_TRACE=1 \
  /tmp/systolic-c-new
```

本次依同一入口執行四個組合，各自保留編譯命令、完整中間 IR、連結指令、`ldd`、DPTI MEM_WRITE／WRITE32／結果讀取紀錄及 CPU 比對結果，位於 `compiler-validation/c-board/`。

| C 工作負載 | 模式 | 規劃中的 FPGA 函式執行次數 | `--verify` 比對元素 | 結果 |
|---|---|---|---|---|
| `test/reproducibility/gemm_example.c` | 同步 | 1 | 480 | PASS，非預期 CPU fallback 0 |
| `test/reproducibility/gemm_example.c` | 非同步 | 1 | 480 | PASS，非預期 CPU fallback 0 |
| `test/SystolicCC/affine_mapping.c` | 非同步 | 5 | 619 | PASS，非預期 CPU fallback 0 |
| `test/SystolicCC/affine_batched.c` | 非同步 | 9 | 1,232 | PASS，非預期 CPU fallback 0 |
| 合計 | — | 16 | 2,811 | 全部逐位元符合 CPU 參考結果 |

各 `ir/commands.txt` 證明使用新 `systolic-cfront`、`systolic-opt` 與新 `compiler-build/runtime/libsystolic_fpga_runtime.a`，未連結舊 Runtime。軟體 Runtime 的模擬硬體標籤只作「預期執行標籤」參考，不能作 FPGA PASS；正式實板環境未設定 `SOFT_RUNTIME_MOCK_HARDWARE` 或 `LD_PRELOAD`，實際標籤必須與預期一致，且至少有真正 FPGA 工作及完整 DPTI 流量。

`gemm_example.c` 刻意保留 `gemm_reference` 在 CPU，會印 `CPU Executed: GEMM #2`；這不是 GEMM #1 的 fallback。初版暫存包裝檢查誤將所有 CPU 標籤當 fallback 而中止，但該輪程式本身已結束碼 0、480 元素驗證一致。原始紀錄保留在 `compiler-validation/initial-wrapper-check/`，未刪除或改寫；只修正本次暫存包裝的判定方法，再於新目錄重新執行四組，未修改 Repository 的編譯器、Runtime 或測試程式。

同步 Runtime 的 `p95_us` 欄位出現大於 `max_us` 的不一致值；本次保留原始紀錄、不更動程式，不採該欄位作效能或加速比結論。數值／派送／編譯器重建驗證不依賴該統計欄位。

### 17.7. 新舊 SHA-256、封存完整性與重現範圍

```text
OLD_SHA256: bd8aba7212a24347eba1dc497204dd6eb5117e8a8ff77094fd94c496a41f3db5
NEW_SHA256: 75f982b4161bbf7351e018d1652439d4d0aeb2275b3590281dc080ec5f5bc1a4
MATCH: NO
```

兩個二進位檔不同，**未達舊 bitstream 的位元組精確重現**。這不否定新映像 Build PASS、FPGA Source Rebuild VERIFIED 或新編譯器／新 Runtime 的實板 PASS；本次驗證的是目前已記錄原始碼所產生的新產物。

```bash
(cd artifacts/fpga/tile_pipelined/1x8x8-3x4x4/2026-10-26-k8_16-k4_16-tclfix_1025 && sha256sum -c SHA256SUMS)
```

新封存不覆蓋舊版，包含 bitstream、LTX、可搬移組態與原始組態、完整建置／燒錄／實板／編譯器測試紀錄、時序與 DPTI 報告、Git 來源清單、工具雜湊及繁體中文文件。紀錄中的絕對暫存路徑屬歷史來源追溯；要重現請另選新的目錄。新編譯器建置目錄留在暫存位置，封存以建置紀錄、設定及雜湊記錄其來源，不保證暫存目錄永久存在。

全域 `*.bit`／`*.log` 等忽略規則仍存在，因此部分新封存檔不會出現在一般未追蹤清單，但實際檔案與 `SHA256SUMS` 均保留。本次未執行 git add、commit 或 push；待使用者審核後，另行決定如何納入版本控制。
