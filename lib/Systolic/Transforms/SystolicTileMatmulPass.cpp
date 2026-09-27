#include "Systolic/Passes.h"
#include "Systolic/SystolicOps.h"

#include "mlir/Dialect/Linalg/IR/Linalg.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"

#include <algorithm>

using namespace mlir;
using namespace mlir::systolic;

namespace {

static int64_t ceilDiv(int64_t a, int64_t b) { return (a + b - 1) / b; }

// Stage-2 tile partitioning: rewrites a static-shape linalg.matmul into a
// grid of systolic.matmul_tile ops wired to real tensor slices. Tiles along
// K are chained through the accumulator operand, so the emitted IR is a
// dataflow program, not merely an annotated task graph. Devices are left
// unassigned -- that is systolic-select-device's job.
struct SystolicTileMatmulPass
    : public PassWrapper<SystolicTileMatmulPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(SystolicTileMatmulPass)

  SystolicTileMatmulPass() = default;
  SystolicTileMatmulPass(const SystolicTileMatmulPass &other)
      : PassWrapper(other),
        tileM(*this, "tile-m", llvm::cl::desc("Tile size along M"),
              llvm::cl::init(other.tileM.getValue())),
        tileN(*this, "tile-n", llvm::cl::desc("Tile size along N"),
              llvm::cl::init(other.tileN.getValue())),
        tileK(*this, "tile-k", llvm::cl::desc("Tile size along K"),
              llvm::cl::init(other.tileK.getValue())) {}

  StringRef getArgument() const final { return "systolic-tile-matmul"; }
  StringRef getDescription() const final {
    return "Stage-2 tile partitioning: rewrite a static-shape linalg.matmul "
           "into a grid of systolic.matmul_tile ops on real tensor slices.";
  }

  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<tensor::TensorDialect, mlir::systolic::SystolicDialect>();
  }

  Option<int64_t> tileM{*this, "tile-m", llvm::cl::desc("Tile size along M"),
                        llvm::cl::init(8)};
  Option<int64_t> tileN{*this, "tile-n", llvm::cl::desc("Tile size along N"),
                        llvm::cl::init(8)};
  Option<int64_t> tileK{*this, "tile-k", llvm::cl::desc("Tile size along K"),
                        llvm::cl::init(8)};

  void runOnOperation() override {
    ModuleOp module = getOperation();
    OpBuilder builder(module.getContext());

    // Collect first: we erase ops below, which is unsafe during a walk.
    SmallVector<linalg::MatmulOp> targets;
    module.walk([&](linalg::MatmulOp op) { targets.push_back(op); });

    for (linalg::MatmulOp matmul : targets) {
      if (matmul->getNumResults() != 1)
        continue; // memref (bufferized) form: no tensor dataflow to wire

      auto lhsTy =
          dyn_cast<RankedTensorType>(matmul.getDpsInputs()[0].getType());
      auto rhsTy =
          dyn_cast<RankedTensorType>(matmul.getDpsInputs()[1].getType());
      if (!lhsTy || !rhsTy || !lhsTy.hasStaticShape() ||
          !rhsTy.hasStaticShape())
        continue; // dynamic shapes: out of scope for this pass

      const int64_t M = lhsTy.getShape()[0];
      const int64_t K = lhsTy.getShape()[1];
      const int64_t N = rhsTy.getShape()[1];
      Type elemTy = lhsTy.getElementType();

      Value A = matmul.getDpsInputs()[0];
      Value B = matmul.getDpsInputs()[1];
      Value acc = matmul.getDpsInits()[0]; // linalg.matmul is DPS: C += A*B

      builder.setInsertionPoint(matmul);
      Location loc = matmul.getLoc();
      SmallVector<OpFoldResult> strides(2, builder.getIndexAttr(1));

      const int64_t numM = ceilDiv(M, tileM);
      const int64_t numN = ceilDiv(N, tileN);
      const int64_t numK = ceilDiv(K, tileK);

      for (int64_t i = 0; i < numM; ++i) {
        const int64_t m = std::min<int64_t>(tileM, M - i * tileM);
        OpFoldResult offM = builder.getIndexAttr(i * tileM);

        for (int64_t j = 0; j < numN; ++j) {
          const int64_t n = std::min<int64_t>(tileN, N - j * tileN);
          OpFoldResult offN = builder.getIndexAttr(j * tileN);

          SmallVector<OpFoldResult> cOff{offM, offN};
          SmallVector<OpFoldResult> cSize{builder.getIndexAttr(m),
                                          builder.getIndexAttr(n)};

          Value cTile = builder.create<tensor::ExtractSliceOp>(
              loc, acc, cOff, cSize, strides);

          // K tiles chain through the accumulator: this is the dependency
          // that makes the schedule a program rather than an annotation.
          for (int64_t k = 0; k < numK; ++k) {
            const int64_t kk = std::min<int64_t>(tileK, K - k * tileK);
            OpFoldResult offK = builder.getIndexAttr(k * tileK);

            SmallVector<OpFoldResult> aOff{offM, offK};
            SmallVector<OpFoldResult> aSize{builder.getIndexAttr(m),
                                            builder.getIndexAttr(kk)};
            Value aTile = builder.create<tensor::ExtractSliceOp>(
                loc, A, aOff, aSize, strides);

            SmallVector<OpFoldResult> bOff{offK, offN};
            SmallVector<OpFoldResult> bSize{builder.getIndexAttr(kk),
                                            builder.getIndexAttr(n)};
            Value bTile = builder.create<tensor::ExtractSliceOp>(
                loc, B, bOff, bSize, strides);

            auto tileTy = RankedTensorType::get({m, n}, elemTy);
            cTile = builder.create<MatmulTileOp>(
                loc, tileTy, aTile, bTile, cTile,
                builder.getI64IntegerAttr(m), builder.getI64IntegerAttr(n),
                builder.getI64IntegerAttr(kk),
                /*device=*/FlatSymbolRefAttr(),
                /*est_cycles=*/IntegerAttr(),
                /*start_cycle=*/IntegerAttr());
          }

          acc = builder.create<tensor::InsertSliceOp>(loc, cTile, acc, cOff,
                                                      cSize, strides);
        }
      }

      matmul.getResult(0).replaceAllUsesWith(acc);
      matmul.erase();
    }
  }
};

} // namespace

std::unique_ptr<Pass> mlir::systolic::createSystolicTileMatmulPass() {
  return std::make_unique<SystolicTileMatmulPass>();
}

void mlir::systolic::registerSystolicTileMatmulPass() {
  PassRegistration<SystolicTileMatmulPass>();
}
