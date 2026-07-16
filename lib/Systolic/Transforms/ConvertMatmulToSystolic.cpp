#include "Systolic/Passes.h"
#include "Systolic/SystolicOps.h"

#include "mlir/Dialect/Linalg/IR/Linalg.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"

using namespace mlir;
using namespace mlir::systolic;

namespace {

// -----------------------------------------------------------------------
// 阶段 2 的核心 pattern:
//
//   linalg.matmul ins(%A, %B) outs(%C)
//
// 转成:
//
//   %a_s = systolic.stream %A direction(row) skew(1) : ...
//   %b_s = systolic.stream %B direction(col) skew(1) : ...
//   %r   = systolic.pe_array<rows x cols> stationary(weight) %B, %a_s, %C
//
// 这里刻意选 weight-stationary:B(权重)固定在 PE 阵列里不动,
// A 沿着 row 方向、部分和沿着 col 方向流动。
//
// 限制(阶段 2 MVP,之后阶段 4 才会放宽):
//   - M 必须刚好等于 rows,N 必须刚好等于 cols
//   - 不处理 tiling,阵列比矩阵小的情况直接拒绝转换
// -----------------------------------------------------------------------
struct MatmulToSystolicPattern : public OpRewritePattern<linalg::MatmulOp> {
  MatmulToSystolicPattern(MLIRContext *ctx, int64_t rows, int64_t cols)
      : OpRewritePattern<linalg::MatmulOp>(ctx), rows(rows), cols(cols) {}

  LogicalResult matchAndRewrite(linalg::MatmulOp op,
                                 PatternRewriter &rewriter) const override {
    Value a = op.getInputs()[0]; // [M, K]
    Value b = op.getInputs()[1]; // [K, N]
    Value c = op.getOutputs()[0]; // [M, N]

    auto aTy = llvm::dyn_cast<RankedTensorType>(a.getType());
    auto bTy = llvm::dyn_cast<RankedTensorType>(b.getType());
    if (!aTy || !bTy || !aTy.hasStaticShape() || !bTy.hasStaticShape())
      return rewriter.notifyMatchFailure(op, "只处理静态形状的 matmul");

    int64_t M = aTy.getShape()[0];
    int64_t N = bTy.getShape()[1];
    if (M != rows || N != cols)
      return rewriter.notifyMatchFailure(
          op, "matmul 的 [M, N] 必须刚好等于阵列大小 [rows, cols]"
              "(tiling 留给阶段 4 处理)");

    Location loc = op.getLoc();

    // A 沿 row 方向 stream 进阵列,skew=1 表示相邻 row 之间差一个 cycle
    auto aStream = rewriter.create<StreamOp>(
        loc, a.getType(), a, StreamDirection::row, /*skew=*/1);

    // 用 weight-stationary:B 固定不动,不需要额外 stream,直接当
    // stationary_operand 喂进 pe_array
    auto peArray = rewriter.create<PEArrayOp>(
        loc, c.getType(),
        /*stationary_operand=*/b,
        /*moving_operand=*/aStream.getResult(),
        /*acc_operand=*/c,
        /*rows=*/rows, /*cols=*/cols,
        /*stationary=*/StationaryKind::weight);

    rewriter.replaceOp(op, peArray.getResult());
    return success();
  }

  int64_t rows, cols;
};

struct ConvertMatmulToSystolicPass
    : public PassWrapper<ConvertMatmulToSystolicPass,
                          OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(ConvertMatmulToSystolicPass)
  ConvertMatmulToSystolicPass() = default;
  ConvertMatmulToSystolicPass(const ConvertMatmulToSystolicPass &other)
      : PassWrapper(other) {
    copyOptionValuesFrom(&other);
  }
  
  StringRef getArgument() const final { return "convert-matmul-to-systolic"; }
  StringRef getDescription() const final {
    return "Lower a fixed-shape linalg.matmul into the systolic dialect";
  }

  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<SystolicDialect, linalg::LinalgDialect>();
  }

  void runOnOperation() override {
    RewritePatternSet patterns(&getContext());
    patterns.add<MatmulToSystolicPattern>(&getContext(), rows.getValue(),
                                           cols.getValue());
    if (failed(applyPatternsAndFoldGreedily(getOperation(),
                                             std::move(patterns))))
      signalPassFailure();
  }

  // 命令列用法:systolic-opt --convert-matmul-to-systolic="rows=8 cols=8"
  Option<int64_t> rows{*this, "rows", llvm::cl::desc("PE 阵列的 row 数"),
                       llvm::cl::init(8)};
  Option<int64_t> cols{*this, "cols", llvm::cl::desc("PE 阵列的 col 数"),
                       llvm::cl::init(8)};
};

} // namespace

std::unique_ptr<Pass> mlir::systolic::createConvertMatmulToSystolicPass() {
  return std::make_unique<ConvertMatmulToSystolicPass>();
}

void mlir::systolic::registerSystolicPasses() {
  PassRegistration<ConvertMatmulToSystolicPass>();
  registerExpandPEArrayToMacPass();
  registerTileMatmulForFpgaPass();
  registerConv2DToFpgaPass();
  registerConv2DNchwToFpgaPass();
  registerBatchMatmulToFpgaPass();
  registerVecmatToFpgaPass();
  registerMatvecToFpgaPass();
}
