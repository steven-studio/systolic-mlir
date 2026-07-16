# systolic-mlir

Systolic array 加速器的 MLIR 編譯器基礎設施。支援兩條互補的 lowering 路徑：
一條把固定形狀的 `linalg.matmul` 編譯成新硬體(HLS C → bitstream),
另一條把**任意形狀**的 `linalg.matmul`/`linalg.conv_2d_*`/`linalg.batch_matmul`
編譯成呼叫**既有硬體**的 runtime offload 呼叫(自動 tiling + zero-padding)。

已在 Digilent Arty A7-35T 上完成端到端硬體驗證:從 `.mlir` 原始碼直接
編譯出執行檔,透過 UART 驅動真實 FPGA,算出正確結果,並且驗證過一個真實
的 `torch.nn.Conv2d`(經由 torch-mlir 匯出)可以直接跑在這條路徑上。

> **分支說明**:`main` 分支是投稿用的穩定版本(matmul + conv2d NHWC 兩條路徑),
> torch-mlir 整合、conv2d NCHW 支援、`batch_matmul` 支援目前在
> `torch-mlir-integration` 分支上,尚未併回 `main`。

## 兩條 lowering 路徑

|  | 硬體生成路徑 | Runtime offload 路徑 |
|---|---|---|
| Pass | `--convert-matmul-to-systolic` | `--tile-matmul-for-fpga` / `--conv2d-to-fpga` / `--conv2d-nchw-to-fpga`\* / `--batch-matmul-to-fpga`\* |
| 輸入形狀限制 | 必須剛好等於 `rows x cols` 陣列大小 | 任意靜態形狀 |
| 輸出 | `systolic` dialect → HLS C → bitstream | `llvm.call` 呼叫外部 C runtime |
| 用途 | 設計新硬體 | 使用已存在的硬體 |
| 是否依賴 systolic dialect | 是 | 否 |

\* 標記 `*` 的 pass 目前只在 `torch-mlir-integration` 分支上,尚未併回 `main`。

兩條路徑只透過硬體介面耦合:runtime offload 路徑不假設目標加速器是被
本專案的硬體生成路徑產生的,只要求它暴露一個固定大小、支援累加的
matmul 原語(目前是 UART)。

## 專案結構

```
systolic-mlir/
├── include/Systolic/
│   ├── SystolicDialect.td      dialect 本身的定義
│   ├── SystolicOps.td          三個核心 op: pe_array / stream / mac
│   ├── SystolicDialect.h       dialect 的 C++ header
│   ├── SystolicOps.h           op 的 C++ header
│   ├── Passes.h                所有 pass 的宣告
│   └── CMakeLists.txt          跑 TableGen 的規則
├── lib/Systolic/
│   ├── IR/SystolicDialect.cpp        dialect/op 的實作 + pe_array 的 verifier
│   └── Transforms/
│       ├── ConvertMatmulToSystolic.cpp   硬體生成路徑:linalg.matmul → systolic dialect
│       ├── ExpandPEArrayToMac.cpp        把 pe_array 展開成 3 層 scf.for + mac
│       ├── TileMatmulForFpga.cpp         runtime offload:任意形狀 matmul → llvm.call
│       ├── Conv2DToFpga.cpp              runtime offload:conv2d NHWC(im2col)→ llvm.call
│       ├── Conv2DNchwToFpga.cpp*         runtime offload:conv2d NCHW(PyTorch 預設 layout)→ llvm.call
│       └── BatchMatmulToFpga.cpp*        runtime offload:batch_matmul → llvm.call
├── tools/systolic-opt/          命令列工具(跟 mlir-opt 用法一樣)
├── runtime/                     C runtime library(UART 協定、tiling、im2col)
│   ├── fpga_matmul4x4.{c,h}          最底層:單次 4x4 matmul 的 UART 收送
│   ├── fpga_matmul_tiled.{c,h}       任意 MxKxN → 4x4 tile 分解 + zero-padding
│   │                                  + fpga_batch_matmul_tiled_auto*(batch 迴圈)
│   ├── fpga_conv2d_im2col.{c,h}      conv2d NHWC → im2col → matmul(含 batch/stride/dilation)
│   │                                  + NCHW/FCHW 變體*(kernel repack + 輸出 transpose)
│   └── test_*.c, *.py                硬體驗證用測試程式與 shape sweep / ULP 分析腳本
├── hls/
│   ├── gemm_4x4/                4x4 陣列:MLIR 原始碼、HLS C、RTL、Vivado 專案(論文評測用)
│   └── gemm_8x8/                8x8 陣列:保留供參考,DSP 需求超出 Arty A7-35T 容量
└── test/Systolic/matmul.mlir    範例 IR(附 FileCheck)
```

`hls/*/vivado/build/`、`*.dcp`、`*.bit`、`*.vcd`、Vitis HLS 的 `systolic_proj/`
等合成產物與模擬波形檔已被 `.gitignore` 排除,可從對應的 `.tcl`/RTL 原始碼
重新產生。

## Build 方式

前提:你已經 build 好 LLVM/MLIR 18(用你 son-dialect 那份就可以,不用重 build)。

```bash
mkdir build && cd build
cmake -G Ninja .. \
  -DMLIR_DIR=$LLVM_BUILD_DIR/lib/cmake/mlir \
  -DLLVM_DIR=$LLVM_BUILD_DIR/lib/cmake/llvm \
  -DCMAKE_BUILD_TYPE=Release
ninja systolic-opt
```

## 用法範例

### 硬體生成路徑:固定形狀 matmul → HLS C

```bash
./build/tools/systolic-opt/systolic-opt \
  --convert-matmul-to-systolic="rows=4 cols=4" \
  --expand-pe-array-to-mac \
  hls/gemm_4x4/matmul_4x4_only.mlir
```

只有形狀剛好等於 `rows x cols` 的 matmul 才會被轉換;形狀不符的 matmul
會被 pattern 拒絕,留給另一條路徑處理。

### Runtime offload 路徑:任意形狀 matmul → 驅動既有硬體

```bash
./build/tools/systolic-opt/systolic-opt \
  --tile-matmul-for-fpga \
  /tmp/test_tile_fpga.mlir
```

任意靜態形狀的 `linalg.matmul` 都會被轉成一個 `llvm.call
@fpga_matmul_tiled_auto(M, K, N, A*, B*, C*)`,實際的 4x4 tile 分解與
zero-padding 全部在執行期由 `runtime/fpga_matmul_tiled.c` 完成。要編譯
成能實際驅動硬體的執行檔,需要走完整的 bufferization + LLVM lowering +
`mlir-translate` + 連結 `runtime/` 的流程(細節見論文 Implementation
章節的 Compilation Toolchain 小節)。

### Runtime offload 路徑:conv2d(NHWC)→ 重用同一顆 matmul 加速器

```bash
./build/tools/systolic-opt/systolic-opt \
  --conv2d-to-fpga \
  /tmp/test_conv2d_fpga.mlir
```

`linalg.conv_2d_nhwc_hwcf` 會被轉成 `llvm.call
@fpga_conv2d_im2col_general_auto(...)`,支援任意 batch size、不對稱
stride、dilation,不需要新的硬體或 UART 協定——host 端用 im2col 展開後
直接重用 `fpga_matmul_tiled_auto`。

### Runtime offload 路徑:conv2d(NCHW)→ 支援 PyTorch 預設 layout \*

```bash
./build/tools/systolic-opt/systolic-opt \
  --conv2d-nchw-to-fpga \
  /tmp/torch_conv2d.mlir
```

`linalg.conv_2d_nchw_fchw`(PyTorch `nn.Conv2d` 經 torch-mlir
`fx.export_and_import` 匯出後的預設 layout,跟上面的 NHWC 版不同)會被轉成
`llvm.call @fpga_conv2d_im2col_nchw_general_auto(...)`。因為 NCHW/FCHW 的
kernel 與輸出記憶體佈局跟 matmul 所需的佈局不一致,這個 runtime 函式會先
把 kernel 從 `(Cout,Cin,Kh,Kw)` repack 成 `(Kh*Kw*Cin)x Cout`,寫回輸出時
再把結果從 `(Hout*Wout)x Cout` transpose 回 `(Cout,Hout,Wout)`。已驗證過
一個真實的 `torch.nn.Conv2d`(含實際訓練/初始化權重)可以完整走這條路徑,
硬體輸出與 PyTorch 自己算出的結果逐位一致(diff = 0.000000)。

### Runtime offload 路徑:batch_matmul → 重用同一顆 matmul 加速器 \*

```bash
./build/tools/systolic-opt/systolic-opt \
  --batch-matmul-to-fpga \
  /tmp/test_batch_matmul.mlir
```

`linalg.batch_matmul` 會被轉成 `llvm.call
@fpga_batch_matmul_tiled_auto(batch, M, K, N, A*, B*, C*)`,依序對每個
batch 元素呼叫既有的 `fpga_matmul_tiled_auto`。因為 batch 之間沒有資料
依賴,也不需要 im2col 這類資料重排,是目前所有 runtime offload pattern 中
實作成本最低的一個。

## 硬體驗證現況

已在 Digilent Arty A7-35T(`xc7a35ticsg324-1L`)上完成：

- 4x4 單精度浮點 systolic array,合成後 DSP48E1 用量 80/90(88.9%)
- FSM-based UART controller,支援連續多輪 tile 呼叫不需外部 reset
- 100MHz 原生時脈合成失敗時序(WNS -10.982ns),改用 `MMCME2_BASE` 分頻到
  20MHz 後時序閉合(WNS +23ns)
- 多種 matmul 形狀(含 zero-padding 邊界情況)端到端驗證:誤差在
  7–30 ULP 範圍,已追溯到 HLS 浮點運算子精度設定,非 tiling 邏輯問題
- conv2d NHWC 擴充(batch、不對稱 stride、dilation)端到端驗證通過
- \* conv2d NCHW 擴充:對一個真實 `torch.nn.Conv2d` 端到端驗證,硬體輸出
  與 PyTorch 參考輸出完全一致
- \* `batch_matmul` 擴充:batch=3、5x5x5(非對齊,觸發 zero-padding)端到端
  驗證,75/75 PASS

完整數字與方法論見論文 Evaluation 章節(僅涵蓋 `main` 分支上的 matmul 與
conv2d NHWC 兩條路徑;標記 `*` 的項目目前只在 `torch-mlir-integration`
分支上)。

## 已知:可能要調的地方

MLIR 的 C++ API 在不同版本間常有小變動(例如 `bufferization::ToTensorOp`
的 build 重載簽名、`--llvm-request-c-wrappers` 是獨立 pass 而非
`--convert-func-to-llvm` 的 option、`memrefCopy` 需要額外連結
`libmlir_c_runner_utils`)。新增 pass 或調整 lowering pipeline 時大概率
會碰到一兩個跟你手上那份 LLVM commit 版本對不上的問題,照錯誤訊息或
`mlir-opt --<pass> --help` 查對應簽名通常就能解決。

另外一個容易忽略的坑:**`--convert-linalg-to-loops` 必須排在
`--convert-scf-to-cf` 之前**。如果 IR 裡含有 `linalg.fill`(例如
torch-mlir 匯出的 conv2d 會用它把輸出 buffer 歸零),順序寫反會導致
`linalg.fill` 展開出的 `scf.for` 迴圈永遠不會被 `--convert-scf-to-cf`
處理到,最終在 `--convert-to-llvm`/`--reconcile-unrealized-casts` 階段
留下無法消化的 `unrealized_conversion_cast`。手寫的 matmul/conv2d 測試
案例通常不含 `linalg.fill`,因此不會觸發這個問題,只有在對接外部工具
(如 torch-mlir)產生的 IR 時才會顯現。

## 對應回階段路線圖

- ✅ 階段 0:範圍鎖死成 GEMM + weight-stationary + 固定陣列大小
- ✅ 階段 1:三個 op 的 TableGen 定義(`SystolicOps.td`)
- ✅ 階段 2:手動 lowering pass(`ConvertMatmulToSystolic.cpp`),
  只處理形狀剛好對上陣列大小的 matmul
- ✅ 階段 3:HLS C codegen(`systolic-translate`)+ Vitis HLS/Vivado 合成 +
  真實硬體燒錄驗證(4x4 陣列,Arty A7-35T)
- ✅ 階段 4:任意形狀自動映射——`--tile-matmul-for-fpga`(4x4 tiling +
  zero-padding)與 `--conv2d-to-fpga`(im2col,支援 batch/stride/dilation),
  全部端到端硬體驗證通過
- 🔶 階段 5(`torch-mlir-integration` 分支,尚未併回 `main`):
  - `--conv2d-nchw-to-fpga`:支援 PyTorch 預設 NCHW/FCHW layout,已對真實
    `torch.nn.Conv2d` 端到端驗證
  - `--batch-matmul-to-fpga`:支援 `linalg.batch_matmul`
  - 過程中發現並修正了編譯工具鏈裡的 pass 順序問題(見上方說明)
- ⬜ 未來方向:UART 之外的高頻寬介面(AXI/PCIe)、tile size 自動搜尋、
  更廣的 shape sweep、`linalg.generic`(elementwise/normalization)與
  `linalg.pooling_*` 支援、硬體生成路徑本身的 tiling(見論文 Limitations
  and Future Work)