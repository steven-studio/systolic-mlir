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
struct MatmulToSystolicPattern
    : public OpRewritePattern<linalg::MatmulOp> {
  MatmulToSystolicPattern(MLIRContext *ctx, int64_t rows, int64_t cols)
      : OpRewritePattern<linalg::MatmulOp>(ctx), rows(rows), cols(cols) {}

  LogicalResult matchAndRewrite(linalg::MatmulOp op,
                                 PatternRewriter &rewriter) const override {
    Value a = op.getDpsInputs()[0];
    Value b = op.getDpsInputs()[1];
    Value c = op.getDpsInits()[0];

    auto aTy = dyn_cast<RankedTensorType>(a.getType());
    auto bTy = dyn_cast<RankedTensorType>(b.getType());
    auto cTy = dyn_cast<RankedTensorType>(c.getType());

    if (!aTy || !bTy || !cTy ||
        !aTy.hasStaticShape() ||
        !bTy.hasStaticShape() ||
        !cTy.hasStaticShape())
      return rewriter.notifyMatchFailure(
          op, "only static ranked-tensor matmul is supported");

    if (aTy.getRank() != 2 ||
        bTy.getRank() != 2 ||
        cTy.getRank() != 2)
      return rewriter.notifyMatchFailure(
          op, "matmul operands must be rank-2 tensors");

    const int64_t M = aTy.getDimSize(0);
    const int64_t K = aTy.getDimSize(1);
    const int64_t BK = bTy.getDimSize(0);
    const int64_t N = bTy.getDimSize(1);

    if (BK != K)
      return rewriter.notifyMatchFailure(
          op, "A and B reduction dimensions do not match");

    if (cTy.getDimSize(0) != M ||
        cTy.getDimSize(1) != N)
      return rewriter.notifyMatchFailure(
          op, "C shape does not match M x N");

    if (aTy.getElementType() != bTy.getElementType() ||
        aTy.getElementType() != cTy.getElementType())
      return rewriter.notifyMatchFailure(
          op, "matmul element types must match");

    Location loc = op.getLoc();

    // No tiling here.
    //
    // The entire linalg.matmul becomes one systolic.matmul_tile:
    //
    //   A : M x K
    //   B : K x N
    //   C : M x N
    //
    // Tiling is deliberately left to a later pass.
    auto result = rewriter.create<MatmulTileOp>(
        loc,
        cTy,
        a,
        b,
        c,
        rewriter.getI64IntegerAttr(M),
        rewriter.getI64IntegerAttr(N),
        rewriter.getI64IntegerAttr(K),
        /*device=*/FlatSymbolRefAttr(),
        /*est_cycles=*/IntegerAttr(),
        /*start_cycle=*/IntegerAttr());

    rewriter.replaceOp(op, result.getResult());
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
    if (failed(applyPatternsGreedily(getOperation(),
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
  registerConv2DToFpgaPass();
  registerConv2DNchwToFpgaPass();
  registerBatchMatmulToFpgaPass();
  registerVecmatToFpgaPass();
  registerMatvecToFpgaPass();
  registerDotGenericToFpgaPass();

  registerSystolicCostAnalysisPass();
  registerSystolicSelectDevicePass();
  registerSystolicTileMatmulPass();
  registerAffineMatmulToSystolicPass();
  registerTestAffineMatmulDiscoveryPass();
}
