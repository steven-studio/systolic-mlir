# systolic-mlir

Systolic array 空间映射 dialect 的 out-of-tree MLIR 专案骨架。
对应之前讨论的阶段 0-2:dialect 设计 + 手动 lowering(阶段 2 MVP)。

## 专案结构

```
systolic-mlir/
├── CMakeLists.txt              顶层 build 设定
├── include/Systolic/
│   ├── SystolicDialect.td      dialect 本身的定义
│   ├── SystolicOps.td          三个核心 op: pe_array / stream / mac
│   ├── SystolicDialect.h       dialect 的 C++ header
│   ├── SystolicOps.h           op 的 C++ header
│   ├── Passes.h                lowering pass 的宣告
│   └── CMakeLists.txt          跑 TableGen 的规则
├── lib/Systolic/
│   ├── IR/SystolicDialect.cpp  dialect/op 的实作 + pe_array 的 verifier
│   └── Transforms/
│       └── ConvertMatmulToSystolic.cpp   阶段 2 手动 lowering pass
├── tools/systolic-opt/         命令列工具(跟 mlir-opt 用法一样)
└── test/Systolic/matmul.mlir   范例 IR(附 FileCheck)
```

## 三个核心 op 在做什么

- **`systolic.pe_array<RxC> stationary(weight|input|output)`**——
  把一段计算标记成映射到 R x C 的 PE 阵列,并宣告哪个 operand 固定不动。
  这是整个 dialect 里唯一带 verifier 的 op:检查阵列大小是正数,以及
  weight-stationary 时 weight 形状要跟阵列大小对上。
- **`systolic.stream direction(row|col) skew(N)`**——
  表达资料沿 row 或 col 方向、以 skew 个 cycle 的相位差流进阵列
  (systolic array 最大特征:资料像波一样斜向传播)。
- **`systolic.mac`**——单一 PE 内部的 `acc = a*b + acc`,之后阶段 3
  接 HLS codegen 时,这个 op 会被展开成阵列大小份,变成实际的 HLS C
  或 RTL。

## Build 方式

前提:你已经 build 好 LLVM/MLIR(用你 son-dialect 那份就可以,不用重 build)。

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

预期看到 8x8x8 的 `linalg.matmul` 被转成:

```mlir
%0 = systolic.stream %A direction(row) skew(1) : (tensor<8x8xf32>) -> tensor<8x8xf32>
%1 = systolic.pe_array<8x8> stationary(weight) %B, %0, %C
     : (tensor<8x8xf32>, tensor<8x8xf32>, tensor<8x8xf32>) -> tensor<8x8xf32>
```

而 16x16x16 的 matmul 应该完全不变(阶段 2 MVP 刻意只处理形状刚好对上
阵列大小的情况,tiling 是阶段 4 的事)。

## 关于 lit 测试

`test/Systolic/matmul.mlir` 是用 `FileCheck` 语法写的,但这个骨架**没有**
设定完整的 `lit` 测试基础设施(那需要额外接 `llvm-lit` 的 site config)。
现在你可以先手动跑上面的命令肉眼比对,等专案稳定一点再补 lit config。

## 已知:可能要调的地方

这份骨架是照标准 out-of-tree MLIR dialect 的写法(跟 `mlir/examples/standalone`
同一个套路)写的,但 MLIR 的 C++ API 在不同版本间常有小变动
(例如 `applyPatternsAndFoldGreedily` 的参数顺序、`EnumAttr` 的写法在
LLVM 18 前后有调整)。第一次 build 大概率会碰到一两个跟你手上那份 LLVM
commit 版本对不上的编译错误——这是正常的,通常照错误讯息改一下 include
路径或函式签名就能过,不代表设计有问题。

## 对应回阶段路线图

- ✅ 阶段 0:范围锁死成 GEMM + weight-stationary + 固定阵列大小
- ✅ 阶段 1:三个 op 的 TableGen 定义(`SystolicOps.td`)
- ✅ 阶段 2:手动 lowering pass(`ConvertMatmulToSystolic.cpp`),
  只处理形状刚好对上阵列大小的 8x8x8 matmul
- ⬜ 阶段 3:接 HLS C codegen(可以照你 dstogov/ir → C 的经验迁移)
- ⬜ 阶段 4:自动映射(support tiling、多种 stationary 策略选择)

下一步建议先把这份骨架 build 起来、跑通 `test/Systolic/matmul.mlir`
这个 demo,再决定要不要往阶段 3(HLS codegen)推进。
