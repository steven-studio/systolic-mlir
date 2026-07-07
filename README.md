# systolic-mlir

Systolic array 空間映射 dialect 的 out-of-tree MLIR 專案骨架。
對應之前討論的階段 0-2:dialect 設計 + 手動 lowering(階段 2 MVP)。

## 專案結構

```
systolic-mlir/
├── CMakeLists.txt              頂層 build 設定
├── include/Systolic/
│   ├── SystolicDialect.td      dialect 本身的定義
│   ├── SystolicOps.td          三個核心 op: pe_array / stream / mac
│   ├── SystolicDialect.h       dialect 的 C++ header
│   ├── SystolicOps.h           op 的 C++ header
│   ├── Passes.h                lowering pass 的宣告
│   └── CMakeLists.txt          跑 TableGen 的規則
├── lib/Systolic/
│   ├── IR/SystolicDialect.cpp  dialect/op 的實作 + pe_array 的 verifier
│   └── Transforms/
│       └── ConvertMatmulToSystolic.cpp   階段 2 手動 lowering pass
├── tools/systolic-opt/         命令列工具(跟 mlir-opt 用法一樣)
└── test/Systolic/matmul.mlir   範例 IR(附 FileCheck)
```

## 三個核心 op 在做什麼

- **`systolic.pe_array<RxC> stationary(weight|input|output)`**——
  把一段計算標記成映射到 R x C 的 PE 陣列,並宣告哪個 operand 固定不動。
  這是整個 dialect 里唯一帶 verifier 的 op:檢查陣列大小是正數,以及
  weight-stationary 時 weight 形狀要跟陣列大小對上。
- **`systolic.stream direction(row|col) skew(N)`**——
  表達資料沿 row 或 col 方向、以 skew 個 cycle 的相位差流進陣列
  (systolic array 最大特征:資料像波一樣斜向傳播)。
- **`systolic.mac`**——單一 PE 內部的 `acc = a*b + acc`,之後階段 3
  接 HLS codegen 時,這個 op 會被展開成陣列大小份,變成實際的 HLS C
  或 RTL。

## Build 方式

前提:你已經 build 好 LLVM/MLIR(用你 son-dialect 那份就可以,不用重 build)。

```bash
mkdir build && cd build
cmake -G Ninja .. \
  -DMLIR_DIR=$LLVM_BUILD_DIR/lib/cmake/mlir \
  -DLLVM_DIR=$LLVM_BUILD_DIR/lib/cmake/llvm \
  -DCMAKE_BUILD_TYPE=Release
ninja systolic-opt
```

## 跑一下看看

```bash
./build/tools/systolic-opt/systolic-opt \
  --convert-matmul-to-systolic="rows=8 cols=8" \
  ../test/Systolic/matmul.mlir
```

預期看到 8x8x8 的 `linalg.matmul` 被轉成:

```mlir
%0 = systolic.stream %A direction(row) skew(1) : (tensor<8x8xf32>) -> tensor<8x8xf32>
%1 = systolic.pe_array<8x8> stationary(weight) %B, %0, %C
     : (tensor<8x8xf32>, tensor<8x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
```

而 16x16x16 的 matmul 應該完全不變(階段 2 MVP 刻意只處理形狀剛好對上
陣列大小的情況,tiling 是階段 4 的事)。

## 關於 lit 測試

`test/Systolic/matmul.mlir` 是用 `FileCheck` 語法寫的,但這個骨架**沒有**
設定完整的 `lit` 測試基礎設施(那需要額外接 `llvm-lit` 的 site config)。
現在你可以先手動跑上面的命令肉眼比對,等專案穩定一點再補 lit config。

## 已知:可能要調的地方

這份骨架是照標準 out-of-tree MLIR dialect 的寫法(跟 `mlir/examples/standalone`
同一個套路)寫的,但 MLIR 的 C++ API 在不同版本間常有小變動
(例如 `applyPatternsAndFoldGreedily` 的參數順序、`EnumAttr` 的寫法在
LLVM 18 前後有調整)。第一次 build 大概率會碰到一兩個跟你手上那份 LLVM
commit 版本對不上的編譯錯誤——這是正常的,通常照錯誤訊息改一下 include
路徑或函式簽名就能過,不代表設計有問題。

## 對應回階段路線圖

- ✅ 階段 0:範圍鎖死成 GEMM + weight-stationary + 固定陣列大小
- ✅ 階段 1:三個 op 的 TableGen 定義(`SystolicOps.td`)
- ✅ 階段 2:手動 lowering pass(`ConvertMatmulToSystolic.cpp`),
  只處理形狀剛好對上陣列大小的 8x8x8 matmul
- ⬜ 階段 3:接 HLS C codegen(可以照你 dstogov/ir → C 的經驗遷移)
- ⬜ 階段 4:自動映射(support tiling、多種 stationary 策略選擇)

下一步建議先把這份骨架 build 起來、跑通 `test/Systolic/matmul.mlir`
這個 demo,再決定要不要往階段 3(HLS codegen)推進。
