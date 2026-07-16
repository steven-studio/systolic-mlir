# systolic-mlir

Systolic array 加速器的 MLIR 編譯器基礎設施。支援兩條互補的 lowering 路徑：
一條把固定形狀的 `linalg.matmul` 編譯成新硬體(HLS C → bitstream),
另一條把**任意形狀**的 `linalg.matmul`/`linalg.vecmat`/`linalg.matvec`/
`linalg.conv_2d_*`/`linalg.batch_matmul`,以及 torch-mlir 為 `torch.dot`
產生的 `linalg.generic` 內積模式,編譯成呼叫**既有硬體**的 runtime offload
呼叫(自動 tiling + zero-padding)。

已在 Digilent Arty A7-35T 上完成端到端硬體驗證:從 `.mlir` 原始碼直接
編譯出執行檔,透過 UART 驅動真實 FPGA,算出正確結果,並且驗證過真實的
`torch.nn.Conv2d`、`torch.nn.Linear`、矩陣-向量乘法與 `torch.dot` 可以
直接跑在這條路徑上,單頭與雙頭 attention 的核心矩陣乘法也可以無縫透過
同一套機制 offload。

> **分支說明**:`main` 分支是投稿用的穩定版本(matmul + conv2d NHWC 兩條路徑),
> 這個分支(`torch-mlir-integration`)額外包含 conv2d NCHW 支援、
> `batch_matmul`/`vecmat`/`matvec`/`torch.dot` 支援、torch-mlir 相容性
> 工具腳本,以及 attention 端到端驗證,尚未併回 `main`。

## 兩條 lowering 路徑

|  | 硬體生成路徑 | Runtime offload 路徑 |
|---|---|---|
| Pass | `--convert-matmul-to-systolic` | `--tile-matmul-for-fpga` / `--vecmat-to-fpga` / `--matvec-to-fpga` / `--dot-generic-to-fpga` / `--conv2d-to-fpga` / `--conv2d-nchw-to-fpga` / `--batch-matmul-to-fpga` |
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
│       ├── VecmatToFpga.cpp              runtime offload:vecmat(向量×矩陣,rank-1 輸入)→ llvm.call
│       ├── MatvecToFpga.cpp              runtime offload:matvec(矩陣×向量,rank-1 輸入)→ llvm.call
│       ├── DotGenericToFpga.cpp          runtime offload:torch.dot 的 linalg.generic 內積模式(結構化匹配)→ llvm.call
│       ├── Conv2DToFpga.cpp              runtime offload:conv2d NHWC(im2col)→ llvm.call
│       ├── Conv2DNchwToFpga.cpp          runtime offload:conv2d NCHW(PyTorch 預設 layout)→ llvm.call
│       └── BatchMatmulToFpga.cpp         runtime offload:batch_matmul → llvm.call
├── tools/systolic-opt/          命令列工具(跟 mlir-opt 用法一樣)
├── runtime/                     C runtime library(UART 協定、tiling、im2col)
│   ├── fpga_matmul4x4.{c,h}          最底層:單次 4x4 matmul 的 UART 收送
│   ├── fpga_matmul_tiled.{c,h}       任意 MxKxN → 4x4 tile 分解 + zero-padding
│   │                                  + fpga_batch_matmul_tiled_auto(batch 迴圈)
│   │                                  + fpga_vecmat_tiled_auto(M=1 的退化 matmul)
│   │                                  + fpga_matvec_tiled_auto(N=1 的退化 matmul)
│   │                                  + fpga_dot_tiled_auto(M=N=1 的雙重退化 matmul)
│   ├── fpga_conv2d_im2col.{c,h}      conv2d NHWC → im2col → matmul(含 batch/stride/dilation)
│   │                                  + NCHW/FCHW 變體(kernel repack + 輸出 transpose)
│   └── test_*.c, *.py                硬體驗證用測試程式與 shape sweep / ULP 分析腳本
├── scripts/                     torch-mlir 輸出的相容性工具
│   ├── fix_torch_mlir_syntax.py      去掉較新 MLIR 版本才有、mlir-opt-18 認不得的
│   │                                  expand_shape/collapse_shape output_shape 屬性,
│   │                                  並可順手把 @main 改名避免跟 C harness 衝突
│   └── compile_torch_mlir.sh         包好完整 lowering pipeline(含已知踩過的
│                                      pass 順序坑),一行指令從 .mlir 產出 .ll
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
章節的 Compilation Toolchain 小節,或直接用下方 `scripts/compile_torch_mlir.sh`)。

### Runtime offload 路徑:vecmat(向量×矩陣)→ 重用同一顆 matmul 加速器

```bash
./build/tools/systolic-opt/systolic-opt \
  --vecmat-to-fpga \
  /tmp/test_vecmat_fpga.mlir
```

`linalg.vecmat`(`y[N] = x[K] @ A[K,N]`,torch-mlir 對 `nn.Linear` 套用在
不帶 batch 維度的 rank-1 輸入時會匯出這個 op)會被轉成 `llvm.call
@fpga_vecmat_tiled_auto(K, N, x*, A*, y*)`,把輸入向量當成 `1xK` 矩陣直接
重用 `fpga_matmul_tiled_auto`,`M=1` 這個維度的 zero-padding 不需要任何
特殊處理。已驗證過一個真實的 `torch.nn.Linear` 可以完整走這條路徑,硬體
輸出與 PyTorch 自己算出的結果逐位一致(diff = 0.000000)。

### Runtime offload 路徑:matvec(矩陣×向量)→ 重用同一顆 matmul 加速器

```bash
./build/tools/systolic-opt/systolic-opt \
  --matvec-to-fpga \
  /tmp/test_matvec_fpga.mlir
```

`linalg.matvec`(`y[M] = A[M,K] @ x[K]`,torch-mlir 對顯式的 `A @ x`
矩陣-向量乘法匯出這個 op,`ins()` 的運算元順序 `(matrix, vector)` 跟
`linalg.vecmat` 的 `(vector, matrix)` 相反)會被轉成 `llvm.call
@fpga_matvec_tiled_auto(M, K, A*, x*, y*)`,把輸入向量當成 `Kx1` 矩陣重用
`fpga_matmul_tiled_auto`,是 `vecmat` 的鏡像(`N=1` 而非 `M=1`)。已驗證過
一個真實的 `A @ x` 模型可以完整走這條路徑,硬體輸出與 PyTorch 自己算出的
結果逐位一致(diff = 0.000000)。

### Runtime offload 路徑:torch.dot(向量內積)→ 結構化匹配 linalg.generic

```bash
./build/tools/systolic-opt/systolic-opt \
  --dot-generic-to-fpga \
  /tmp/test_dot_fpga.mlir
```

跟以上所有 pattern 不同,torch-mlir **不會**把 `torch.dot` 匯出成具名的
`linalg.dot` op,而是拆成一對 `linalg.generic`——一個 elementwise 乘法,
接一個 reduction-to-scalar 加總。`DotGenericToFpgaPattern` 因此不是靠
op 型別匹配(`OpRewritePattern<linalg::DotOp>` 之類),而是結構化地驗證
這對 `linalg.generic` 的精確形狀:`iterator_types`、region body 裡的
算術運算(`arith.mulf`/`arith.addf`,用共用的 `hasSimpleBinaryBody` helper
檢查)、reduction 輸入的來源是否確實是符合條件的 elementwise-multiply
`linalg.generic`,以及累加器初值是否為 `linalg.fill` 產生的零常數。任何
一項不符,pattern 一律拒絕匹配,不猜測——這個設計已經過驗證:對單頭/雙頭
attention IR 裡的 softmax(reduce-max 用 `arith.maximumf`、reduce-sum 用
`arith.addf`,跟 dot product 的 reduction body 相同,僅能靠「producer 是
否為 elementwise multiply」這一項區分)跑過這個 pass,結果完全沒有被
誤判轉換,softmax 原封不動保留。

轉換後會產生 `llvm.call @fpga_dot_tiled_auto(K, x*, y*, result*)`,把
兩個輸入向量當成 `1xK` 與 `Kx1` 矩陣,重用 `fpga_matmul_tiled_auto` 並
同時設 `M=N=1`,是 `vecmat`/`matvec` 之後的雙重退化版本。已驗證過一個
真實的 `torch.dot` 可以完整走這條路徑,硬體輸出與 PyTorch 自己算出的
結果逐位一致(diff = 0.000000)。

開發過程中這個 pattern 曾經卡在一個真正的循環定義 bug:初版實作把
`reduceOp` **自己的結果**拿去 bufferize 成輸出 memref,但
`rewriter.replaceOp()` 會把所有用到該結果的地方(包括這個 memref 建構
op 本身)都改指向由這個 memref 算出的新 tensor,形成循環定義。這個
bug 不會 crash,也不是貪婪 driver 重複觸發 pattern(用 `llvm::errs()`
在每一步插旗確認過,`matchAndRewrite` 只被呼叫一次、完整跑完並回傳
`success()`),而是卡在後續的 SSA use-list 維護階段。修法是改用
reduction 真正的 `outs()` 操作數(已被 `linalg.fill` 歸零的那個值)去
bufferize,跟其他所有 pattern 的寫法一致。

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

### Runtime offload 路徑:conv2d(NCHW)→ 支援 PyTorch 預設 layout

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

### Runtime offload 路徑:batch_matmul → 重用同一顆 matmul 加速器

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

這個 pattern 不對「batch」這個維度做任何語意假設,只當成一般的獨立維度
處理——這讓它意外地完美對應到 multi-head attention 裡的 head 維度:
torch-mlir 匯出 attention 時,會把 `[batch, heads, seq, dim]` 先
`collapse` 成 `[batch*heads, seq, dim]` 才餵給 `linalg.batch_matmul`。
已驗證過單頭與雙頭 attention(3 個線性投影 + `Q@K^T` + `attn@V`,共 5 次
`linalg.batch_matmul`,其中雙頭版本 `Q@K^T`/`attn@V` 的 `batch` 維度精確
對應 `num_heads=2`)可以完全不修改此 pattern、直接端到端驅動硬體,結果與
PyTorch 參考輸出逐位一致(diff = 0.000000)。softmax(reduce-max、減法、
`math.exp`、reduce-sum、除法)完全留在 host 端執行,是合理的混合執行
模式,而非本 pattern 的能力缺口。

### 對接 torch-mlir 輸出:相容性工具腳本

torch-mlir 綁定的 LLVM/MLIR 版本通常比系統上 apt 裝的 `mlir-opt-18` 新,
偶爾會用到較新的語法(例如 `tensor.expand_shape` 帶 `output_shape` 屬性)。
`scripts/` 底下的兩個工具處理這個落差,不需要每次手動修改：

```bash
# 1. 修正語法差異,並把 @main 改名(避免跟 C harness 的 main() 衝突)
python3 scripts/fix_torch_mlir_syntax.py \
    /tmp/torch_model_raw.mlir \
    /tmp/torch_model_fixed.mlir \
    --rename-main my_model_forward

# 2. 一行指令跑完 systolic-opt + 完整 lowering pipeline,產出可編譯的 .ll
./scripts/compile_torch_mlir.sh \
    /tmp/torch_model_fixed.mlir \
    /tmp/torch_model \
    --batch-matmul-to-fpga --conv2d-nchw-to-fpga --vecmat-to-fpga \
    --matvec-to-fpga --dot-generic-to-fpga
```

`compile_torch_mlir.sh` 的第三個以後參數會原封不動傳給 `systolic-opt`,
可以疊加多個 offload pass。腳本內建的 pass 順序(`--convert-linalg-to-loops`
在 `--convert-scf-to-cf` 之前、`--expand-strided-metadata` 在
`--convert-scf-to-cf` 之前)是實測過的正確順序,見下方「已知:可能要調的
地方」。

## 硬體驗證現況

已在 Digilent Arty A7-35T(`xc7a35ticsg324-1L`)上完成：

- 4x4 單精度浮點 systolic array,合成後 DSP48E1 用量 80/90(88.9%)
- FSM-based UART controller,支援連續多輪 tile 呼叫不需外部 reset
- 100MHz 原生時脈合成失敗時序(WNS -10.982ns),改用 `MMCME2_BASE` 分頻到
  20MHz 後時序閉合(WNS +23ns)
- 多種 matmul 形狀(含 zero-padding 邊界情況)端到端驗證:誤差在
  7–30 ULP 範圍,已追溯到 HLS 浮點運算子精度設定,非 tiling 邏輯問題
- conv2d NHWC 擴充(batch、不對稱 stride、dilation)端到端驗證通過
- conv2d NCHW 擴充:對一個真實 `torch.nn.Conv2d` 端到端驗證,硬體輸出
  與 PyTorch 參考輸出完全一致
- `batch_matmul` 擴充:batch=3、5x5x5(非對齊,觸發 zero-padding)端到端
  驗證,75/75 PASS
- attention 端到端驗證:單頭與雙頭 attention 的核心矩陣乘法(3 個線性
  投影 + `Q@K^T` + `attn@V`)透過 `--batch-matmul-to-fpga` 完全不需修改
  即可正確 offload,硬體輸出與 PyTorch 參考輸出逐位一致
- `vecmat` 擴充:C 層 K=7、N=5(雙向非對齊)端到端驗證 5/5 PASS;對一個
  真實 `torch.nn.Linear`(rank-1、無 batch 維度輸入)端到端驗證,硬體
  輸出與 PyTorch 參考輸出逐位一致(diff = 0.000000)
- `matvec` 擴充:C 層 M=5、K=7(雙向非對齊)端到端驗證 5/5 PASS;對一個
  真實 `A @ x` 模型端到端驗證,硬體輸出與 PyTorch 參考輸出逐位一致
  (diff = 0.000000)
- `torch.dot` 擴充(結構化 `linalg.generic` 匹配):C 層 K=9(非對齊)
  端到端驗證 PASS;對一個真實 `torch.dot` 模型端到端驗證,硬體輸出與
  PyTorch 參考輸出逐位一致(diff = 0.000000);另外針對性驗證此 pattern
  不會誤判 attention IR 裡結構相似(reduction body 同為 `arith.addf`)
  但語意不同的 softmax reduce-sum

至此,runtime offload 架構已涵蓋 rank-0(dot 的純量輸出)、rank-1
(vecmat、matvec)、rank-2/3(matmul、conv2d NHWC、batch_matmul)、rank-4
(conv2d NCHW)五種 tensor rank、九個 `linalg` op/模式,全部建立在同一個
`fpga_matmul_tiled_auto` 核心上,未曾修改過它或 `fpga_matmul4x4.c`。

完整數字與方法論見論文 Evaluation 章節(僅涵蓋 `main` 分支上的 matmul 與
conv2d NHWC 兩條路徑;這個分支上的其餘項目屬於探索性驗證,尚未納入正式
評測)。

## 已知:可能要調的地方

MLIR 的 C++ API 在不同版本間常有小變動(例如 `bufferization::ToTensorOp`
的 build 重載簽名、`--llvm-request-c-wrappers` 是獨立 pass 而非
`--convert-func-to-llvm` 的 option、`memrefCopy` 需要額外連結
`libmlir_c_runner_utils`)。新增 pass 或調整 lowering pipeline 時大概率
會碰到一兩個跟你手上那份 LLVM commit 版本對不上的問題,照錯誤訊息或
`mlir-opt --<pass> --help` 查對應簽名通常就能解決。

另外兩個容易忽略的坑,`scripts/compile_torch_mlir.sh` 已經處理好順序,
但如果你自己手動組 pipeline 要注意：

- **`--convert-linalg-to-loops` 必須排在 `--convert-scf-to-cf` 之前**。
  如果 IR 裡含有 `linalg.fill`(例如 torch-mlir 匯出的 conv2d/softmax/
  vecmat/matvec/dot 都會用它把輸出 buffer 歸零),順序寫反會導致
  `linalg.fill` 展開出的 `scf.for` 迴圈永遠不會被 `--convert-scf-to-cf`
  處理到,最終在 `--convert-to-llvm`/`--reconcile-unrealized-casts` 階段
  留下無法消化的 `unrealized_conversion_cast`。手寫的 matmul/conv2d 測試
  案例通常不含 `linalg.fill`,因此不會觸發這個問題,只有在對接外部工具
  產生的 IR 時才會顯現。
- **`--expand-strided-metadata` 必須排在 `--convert-scf-to-cf` 之前**。
  如果 IR 裡含有 `memref.expand_shape`/`collapse_shape`(例如 softmax
  的 reduce-then-broadcast 模式,或 multi-head attention 的
  head 維度拆分/合併),缺少這個 pass 會讓對應的 memref descriptor 型別
  轉換不完整,同樣留下無法消化的 `unrealized_conversion_cast`。
- torch-mlir 綁定的 MLIR 版本可能比系統 `mlir-opt-18` 新,偶爾會用到
  舊版 parser 認不得的語法(目前已知的例子是 `tensor.expand_shape` 的
  `output_shape [...]` 屬性,是純冗餘標註,可安全移除,不影響語意)。用
  `scripts/fix_torch_mlir_syntax.py` 處理,不要直接修改系統的 LLVM 版本
  ——torch-mlir 通常綁定持續滾動的 nightly commit,沒有一個「對齊了就
  一勞永逸」的版本,追版本是無底洞。
- **在 `OpRewritePattern` 裡 bufferize 一個 op 的輸出 buffer 時,務必
  用該 op 的 `outs()` 操作數,不要用 `op.getResult(...)` 本身。** 用
  op 自己的結果去建構它的輸出 memref,會在 `rewriter.replaceOp()` 執行
  後形成循環 SSA 定義(該 memref 依賴替換後的 tensor,而替換後的 tensor
  又是從該 memref 算出來的),不會立即 crash 或觸發明顯的無限 pattern
  重寫迴圈,而是在後續的 use-list 維護階段卡死,難以定位。`DotGenericToFpga.cpp`
  的開發過程完整踩過這個坑,詳見上方 `--dot-generic-to-fpga` 小節。

## 對應回階段路線圖

- ✅ 階段 0:範圍鎖死成 GEMM + weight-stationary + 固定陣列大小
- ✅ 階段 1:三個 op 的 TableGen 定義(`SystolicOps.td`)
- ✅ 階段 2:手動 lowering pass(`ConvertMatmulToSystolic.cpp`),
  只處理形狀剛好對上陣列大小的 matmul
- ✅ 階段 3:HLS C codegen(`systolic-translate`)+ Vitis HLS/Vivado 合成 +
  真實硬體燒錄驗證(4x4 陣列,Arty A7-35T)
- ✅ 階段 4:任意形狀自動映射——`--tile-matmul-for-fpga`(4x4 tiling +
  zero-padding)與 `--conv2d-to-fpga`(im2col,支援 batch/stride/dilation),
  全部端到端硬體驗證通過(以上四階段是 `main` 分支的範圍)
- ✅ 階段 5(這個分支):
  - `--conv2d-nchw-to-fpga`:支援 PyTorch 預設 NCHW/FCHW layout,已對真實
    `torch.nn.Conv2d` 端到端驗證
  - `--batch-matmul-to-fpga`:支援 `linalg.batch_matmul`
  - `--vecmat-to-fpga` / `--matvec-to-fpga`:支援 `linalg.vecmat`/
    `linalg.matvec`(兩種退化的 rank-1 matmul,一個向量在左一個在右),
    已對真實 `torch.nn.Linear` 與 `A @ x` 模型端到端驗證
  - `--dot-generic-to-fpga`:透過結構化匹配(而非型別匹配)支援
    `torch.dot` 匯出的 `linalg.generic` 內積模式,已對真實 `torch.dot`
    模型端到端驗證,並驗證過不會誤判 attention 的 softmax
  - `scripts/`:torch-mlir 相容性工具,封裝已知的語法差異修正與正確的
    lowering pass 順序
  - 單頭與雙頭 attention 端到端驗證,確認 `batch_matmul` 的抽象天然對應
    multi-head 的 head 維度,不需修改既有 pattern
- ⬜ 未來方向:UART 之外的高頻寬介面(AXI/PCIe)、tile size 自動搜尋、
  更廣的 shape sweep、其他 `linalg.generic` 模式(elementwise/
  normalization,即 LayerNorm/BatchNorm 這類刻意選擇不 offload 的運算)、
  `linalg.pooling_*` 支援、硬體生成路徑本身的 tiling、把此分支的內容
  正式併回 `main` 或整理成獨立論文(見論文 Limitations and Future Work)