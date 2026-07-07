#include "Systolic/Passes.h"
#include "Systolic/SystolicOps.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"

using namespace mlir;
using namespace mlir::systolic;

namespace {

struct ExpandPEArrayToMacPattern : public OpRewritePattern<PEArrayOp> {
  using OpRewritePattern<PEArrayOp>::OpRewritePattern;

  LogicalResult matchAndRewrite(PEArrayOp op,
                                 PatternRewriter &rewriter) const override {
    if (op.getStationary() != StationaryKind::weight)
      return rewriter.notifyMatchFailure(op, "目前只处理 weight-stationary");

    Value acc = op.getAccOperand();
    if (!acc)
      return rewriter.notifyMatchFailure(op,
                                          "目前需要有 acc_operand 才能展开");

    Value weight = op.getStationaryOperand(); // B: [K, N]
    Value moving = op.getMovingOperand();     // A: [M, K]

    auto weightTy = cast<RankedTensorType>(weight.getType());
    auto movingTy = cast<RankedTensorType>(moving.getType());
    int64_t rows = movingTy.getShape()[0];
    int64_t k = movingTy.getShape()[1];
    int64_t cols = weightTy.getShape()[1];

    Location loc = op.getLoc();
    Value c0 = rewriter.create<arith::ConstantIndexOp>(loc, 0);
    Value c1 = rewriter.create<arith::ConstantIndexOp>(loc, 1);
    Value cRows = rewriter.create<arith::ConstantIndexOp>(loc, rows);
    Value cCols = rewriter.create<arith::ConstantIndexOp>(loc, cols);
    Value cK = rewriter.create<arith::ConstantIndexOp>(loc, k);

    auto outerI = rewriter.create<scf::ForOp>(
        loc, c0, cRows, c1, ValueRange{acc},
        [&](OpBuilder &bI, Location locI, Value i, ValueRange iterArgsI) {
          Value tensorAfterI = iterArgsI[0];
          auto innerJ = bI.create<scf::ForOp>(
              locI, c0, cCols, c1, ValueRange{tensorAfterI},
              [&](OpBuilder &bJ, Location locJ, Value j,
                  ValueRange iterArgsJ) {
                Value tensorAfterJ = iterArgsJ[0];
                Value accInit = bJ.create<tensor::ExtractOp>(
                    locJ, tensorAfterJ, ValueRange{i, j});
                auto kLoop = bJ.create<scf::ForOp>(
                    locJ, c0, cK, c1, ValueRange{accInit},
                    [&](OpBuilder &bK, Location locK, Value kk,
                        ValueRange iterArgsK) {
                      Value accCur = iterArgsK[0];
                      Value aElem = bK.create<tensor::ExtractOp>(
                          locK, moving, ValueRange{i, kk});
                      Value bElem = bK.create<tensor::ExtractOp>(
                          locK, weight, ValueRange{kk, j});
                      Type elemTy = accCur.getType();
                      auto macOp = bK.create<MacOp>(
                          locK, TypeRange{elemTy, elemTy, elemTy}, aElem,
                          bElem, accCur);
                      bK.create<scf::YieldOp>(
                          locK, ValueRange{macOp.getAccOut()});
                    });
                Value accFinal = kLoop.getResult(0);
                Value updatedTensor = bJ.create<tensor::InsertOp>(
                    locJ, accFinal, tensorAfterJ, ValueRange{i, j});
                bJ.create<scf::YieldOp>(locJ, ValueRange{updatedTensor});
              });
          bI.create<scf::YieldOp>(locI, ValueRange{innerJ.getResult(0)});
        });

    rewriter.replaceOp(op, outerI.getResult(0));
    return success();
  }
};

struct ExpandPEArrayToMacPass
    : public PassWrapper<ExpandPEArrayToMacPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(ExpandPEArrayToMacPass)

  StringRef getArgument() const final { return "expand-pe-array-to-mac"; }
  StringRef getDescription() const final {
    return "Expand systolic.pe_array into explicit rows x cols x K loops of "
           "systolic.mac";
  }

  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<SystolicDialect, scf::SCFDialect, tensor::TensorDialect,
                     arith::ArithDialect>();
  }

  void runOnOperation() override {
    RewritePatternSet patterns(&getContext());
    patterns.add<ExpandPEArrayToMacPattern>(&getContext());
    if (failed(applyPatternsAndFoldGreedily(getOperation(),
                                             std::move(patterns))))
      signalPassFailure();
  }
};

} // namespace

std::unique_ptr<Pass> mlir::systolic::createExpandPEArrayToMacPass() {
  return std::make_unique<ExpandPEArrayToMacPass>();
}

void mlir::systolic::registerExpandPEArrayToMacPass() {
  PassRegistration<ExpandPEArrayToMacPass>();
}
