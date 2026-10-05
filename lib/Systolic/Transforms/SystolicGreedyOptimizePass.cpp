#include "Systolic/Passes.h"
#include "Systolic/SystolicOptimizer.h"
#include "Systolic/SystolicOps.h"
#include "Systolic/SystolicTiling.h"

#include "mlir/Dialect/Linalg/IR/Linalg.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/Pass/Pass.h"

#include <cstdint>

using namespace mlir;
using namespace mlir::systolic;

namespace {

struct SystolicGreedyOptimizePass
    : public PassWrapper<
          SystolicGreedyOptimizePass,
          OperationPass<ModuleOp>> {

  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(
      SystolicGreedyOptimizePass)

  StringRef getArgument() const final {
    return "systolic-greedy-optimize";
  }

  StringRef getDescription() const final {
    return
        "Run greedy systolic decomposition and hardware-aware "
        "scheduling using declared systolic devices.";
  }

  void getDependentDialects(
      DialectRegistry &registry) const override {
    registry.insert<
        SystolicDialect,
        linalg::LinalgDialect>();
  }

  void runOnOperation() override {
    ModuleOp module = getOperation();

    // ----------------------------------------------------------
    // Stage 1:
    // Translate systolic.device declarations into the
    // optimizer's physical fleet representation.
    // ----------------------------------------------------------

    SmallVector<SystolicArrayResource> fleet;

    int64_t nextAcceleratorId = 0;

    module.walk([&](DeviceOp device) {
      const int64_t rows = device.getRows();
      const int64_t cols = device.getCols();

      if (rows <= 0 || cols <= 0 || rows != cols) {
        device.emitError()
            << "systolic.device must have a positive square geometry";
        signalPassFailure();
        return;
      }

      int64_t tileOverhead = 0;

      if (IntegerAttr attr = device.getTileOverheadAttr()) {
        tileOverhead = attr.getInt();

        if (tileOverhead < 0) {
          device.emitError()
              << "tile_overhead must be non-negative";
          signalPassFailure();
          return;
        }
      }

      SystolicArrayResource resource;
      resource.arraySize = rows;
      resource.acceleratorId = nextAcceleratorId++;
      resource.tileOverhead = tileOverhead;

      fleet.push_back(resource);
    });

    if (fleet.empty()) {
      module.emitError()
          << "systolic-greedy-optimize requires at least one "
             "systolic.device";
      signalPassFailure();
      return;
    }

    // ----------------------------------------------------------
    // Stage 2:
    // Run the optimizer on every static tensor linalg.matmul.
    //
    // This pass intentionally does NOT rewrite the IR yet.
    // It only proves that:
    //
    //   linalg.matmul
    //       -> greedy decomposition
    //       -> H-aware scheduling
    //
    // works through the real MLIR device declarations.
    // ----------------------------------------------------------

    SmallVector<linalg::MatmulOp> matmuls;

    module.walk([&](linalg::MatmulOp op) {
      matmuls.push_back(op);
    });

    for (linalg::MatmulOp matmul : matmuls) {
      auto aTy =
          dyn_cast<RankedTensorType>(
              matmul.getDpsInputs()[0].getType());

      auto bTy =
          dyn_cast<RankedTensorType>(
              matmul.getDpsInputs()[1].getType());

      if (!aTy || !bTy ||
          !aTy.hasStaticShape() ||
          !bTy.hasStaticShape()) {
        matmul.emitError()
            << "systolic-greedy-optimize currently requires "
               "static ranked tensor matmul operands";
        signalPassFailure();
        return;
      }

      if (aTy.getRank() != 2 ||
          bTy.getRank() != 2) {
        matmul.emitError()
            << "systolic-greedy-optimize currently requires "
               "rank-2 matmul operands";
        signalPassFailure();
        return;
      }

      const int64_t M = aTy.getDimSize(0);
      const int64_t K = aTy.getDimSize(1);
      const int64_t BK = bTy.getDimSize(0);
      const int64_t N = bTy.getDimSize(1);

      if (K != BK) {
        matmul.emitError()
            << "matmul reduction dimensions do not match";
        signalPassFailure();
        return;
      }

      auto result =
          optimizeSystolicGreedy(
              M,
              N,
              fleet);

      if (failed(result)) {
        matmul.emitError()
            << "greedy systolic optimization failed for "
            << M << "x" << N;
        signalPassFailure();
        return;
      }

      matmul.emitRemark()
          << "systolic greedy optimization: "
          << M << "x" << N
          << ", tiles = "
          << result->decomposition.size()
          << ", makespan = "
          << result->makespan;
    }
  }
};

} // namespace

std::unique_ptr<Pass>
mlir::systolic::createSystolicGreedyOptimizePass() {
  return std::make_unique<
      SystolicGreedyOptimizePass>();
}

void mlir::systolic::registerSystolicGreedyOptimizePass() {
  PassRegistration<SystolicGreedyOptimizePass>();
}
