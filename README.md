# systolic-mlir

Systolic array 加速器的 MLIR 編譯器基礎設施。支援兩條互補的 lowering 路徑：
一條把固定形狀的 `linalg.matmul` 編譯成新硬體(HLS C → bitstream),
另一條把**任意形狀**的 `linalg.matmul`/`linalg.conv_2d_nhwc_hwcf` 編譯成
呼叫**既有硬體**的 runtime offload 呼叫(自動 tiling + zero-padding)。

已在 Digilent Arty A7-35T 上完成端到端硬體驗證:從 `.mlir` 原始碼直接
編譯出執行檔,透過 UART 驅動真實 FPGA,算出正確結果。

## 兩條 lowering 路徑

|  | 硬體生成路徑 | Runtime offload 路徑 |
|---|---|---|
| Pass | `--convert-matmul-to-systolic` | `--tile-matmul-for-fpga` / `--conv2d-to-fpga` |
| 輸入形狀限制 | 必須剛好等於 `rows x cols` 陣列大小 | 任意靜態形狀 |
| 輸出 | `systolic` dialect → HLS C → bitstream | `llvm.call` 呼叫外部 C runtime |
| 用途 | 設計新硬體 | 使用已存在的硬體 |
| 是否依賴 systolic dialect | 是 | 否 |

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
│       └── Conv2DToFpga.cpp              runtime offload:conv2d(im2col)→ llvm.call
├── tools/systolic-opt/          命令列工具(跟 mlir-opt 用法一樣)
├── runtime/                     C runtime library(UART 協定、tiling、im2col)
│   ├── fpga_matmul4x4.{c,h}          最底層:單次 4x4 matmul 的 UART 收送
│   ├── fpga_matmul_tiled.{c,h}       任意 MxKxN → 4x4 tile 分解 + zero-padding
│   ├── fpga_conv2d_im2col.{c,h}      conv2d → im2col → matmul(含 batch/stride/dilation)
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

### Runtime offload 路徑:conv2d → 重用同一顆 matmul 加速器

```bash
./build/tools/systolic-opt/systolic-opt \
  --conv2d-to-fpga \
  /tmp/test_conv2d_fpga.mlir
```

`linalg.conv_2d_nhwc_hwcf` 會被轉成 `llvm.call
@fpga_conv2d_im2col_general_auto(...)`,支援任意 batch size、不對稱
stride、dilation,不需要新的硬體或 UART 協定——host 端用 im2col 展開後
直接重用 `fpga_matmul_tiled_auto`。

## 硬體驗證現況

已在 Digilent Arty A7-35T(`xc7a35ticsg324-1L`)上完成：

- 4x4 單精度浮點 systolic array,合成後 DSP48E1 用量 80/90(88.9%)
- FSM-based UART controller,支援連續多輪 tile 呼叫不需外部 reset
- 100MHz 原生時脈合成失敗時序(WNS -10.982ns),改用 `MMCME2_BASE` 分頻到
  20MHz 後時序閉合(WNS +23ns)
- 多種 matmul 形狀(含 zero-padding 邊界情況)端到端驗證:誤差在
  7–30 ULP 範圍,已追溯到 HLS 浮點運算子精度設定,非 tiling 邏輯問題
- conv2d 擴充(batch、不對稱 stride、dilation)端到端驗證通過

完整數字與方法論見論文 Evaluation 章節。

## 已知:可能要調的地方

MLIR 的 C++ API 在不同版本間常有小變動(例如 `bufferization::ToTensorOp`
的 build 重載簽名、`--llvm-request-c-wrappers` 是獨立 pass 而非
`--convert-func-to-llvm` 的 option、`memrefCopy` 需要額外連結
`libmlir_c_runner_utils`)。新增 pass 或調整 lowering pipeline 時大概率
會碰到一兩個跟你手上那份 LLVM commit 版本對不上的問題,照錯誤訊息或
`mlir-opt --<pass> --help` 查對應簽名通常就能解決。

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
- ⬜ 未來方向:UART 之外的高頻寬介面(AXI/PCIe)、tile size 自動搜尋、
  更廣的 shape sweep、其他 `linalg` structured op(見論文 Limitations
  and Future Work)