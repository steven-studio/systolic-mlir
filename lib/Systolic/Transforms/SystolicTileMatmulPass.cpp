#include "Systolic/Passes.h"
#include "Systolic/SystolicOps.h"

#include "mlir/Dialect/Linalg/IR/Linalg.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"

#include <algorithm>

using namespace mlir;
using namespace mlir::systolic;

namespace {

static int64_t ceilDiv(int64_t a, int64_t b) { return (a + b - 1) / b; }

// Stage-2 tile partitioning: splits a static-shape linalg.matmul into a
// grid of systolic.matmul_tile task nodes, sized by --tile-m/--tile-n/
// --tile-k. Devices are intentionally left unassigned -- that is the job
// of systolic-select-device, which runs afterward and picks, per tile,
// whichever declared systolic.device minimizes overall makespan.
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
    return "Stage-2 tile partitioning: split a static-shape linalg.matmul "
           "into a grid of unassigned systolic.matmul_tile task nodes.";
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

    module.walk([&](linalg::MatmulOp matmul) {
      auto lhsTy = dyn_cast<RankedTensorType>(matmul.getDpsInputs()[0].getType());
      auto rhsTy = dyn_cast<RankedTensorType>(matmul.getDpsInputs()[1].getType());
      if (!lhsTy || !rhsTy || !lhsTy.hasStaticShape() || !rhsTy.hasStaticShape())
        return; // dynamic shapes: out of scope for this pass

      int64_t M = lhsTy.getShape()[0];
      int64_t K = lhsTy.getShape()[1];
      int64_t N = rhsTy.getShape()[1];

      builder.setInsertionPoint(matmul);
      Location loc = matmul.getLoc();
      Type resultTy = matmul->getResult(0).getType();

      int64_t numM = ceilDiv(M, tileM);
      int64_t numN = ceilDiv(N, tileN);
      int64_t numK = ceilDiv(K, tileK);

      for (int64_t i = 0; i < numM; ++i) {
        int64_t m = std::min<int64_t>(tileM, M - i * tileM);
        for (int64_t j = 0; j < numN; ++j) {
          int64_t n = std::min<int64_t>(tileN, N - j * tileN);
          for (int64_t k = 0; k < numK; ++k) {
            int64_t kk = std::min<int64_t>(tileK, K - k * tileK);
            builder.create<MatmulTileOp>(
                loc, resultTy, builder.getI64IntegerAttr(m),
                builder.getI64IntegerAttr(n), builder.getI64IntegerAttr(kk),
                /*device=*/FlatSymbolRefAttr(),
                /*est_cycles=*/IntegerAttr(),
                /*start_cycle=*/IntegerAttr());
          }
        }
      }
      // matmul_tile is currently a schedule-only task node (no real tensor
      // operands yet -- see SystolicOps.td), so we leave the original
      // linalg.matmul in place instead of erasing/rewiring dataflow.
      // Wiring real tensor slices through these tiles is future work.
    });
  }
};

} // namespace

std::unique_ptr<Pass> mlir::systolic::createSystolicTileMatmulPass() {
  return std::make_unique<SystolicTileMatmulPass>();
}

void mlir::systolic::registerSystolicTileMatmulPass() {
  PassRegistration<SystolicTileMatmulPass>();
}
