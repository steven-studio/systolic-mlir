#include "Systolic/CostModel.h"
#include "Systolic/DeviceConfig.h"
#include "Systolic/Passes.h"
#include "Systolic/SystolicOptimizer.h"
#include "Systolic/SystolicOps.h"
#include "Systolic/SystolicTilePlan.h"
#include "Systolic/SystolicTiling.h"

#include "mlir/Dialect/Linalg/IR/Linalg.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Builders.h"
#include "mlir/Pass/Pass.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>

using namespace mlir;
using namespace mlir::systolic;

namespace {

struct SystolicBinarySearchTileMatmulPass
    : public PassWrapper<
          SystolicBinarySearchTileMatmulPass,
          OperationPass<ModuleOp>> {

  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(
      SystolicBinarySearchTileMatmulPass)

  StringRef getArgument() const final {
    return "systolic-binary-search-tile-matmul";
  }

  StringRef getDescription() const final {
    return
        "Lower linalg.matmul using the binary-search systolic "
        "decomposition into mixed-geometry systolic.matmul_tile ops.";
  }

  SystolicBinarySearchTileMatmulPass() = default;
  SystolicBinarySearchTileMatmulPass(
      const SystolicBinarySearchTileMatmulPass &other)
      : PassWrapper(other) {}
  explicit SystolicBinarySearchTileMatmulPass(
      double sharedReadBytesPerCycleValue) {
    sharedReadBytesPerCycle = sharedReadBytesPerCycleValue;
  }

  Option<double> sharedReadBytesPerCycle{
      *this, "shared-read-bytes-per-cycle",
      llvm::cl::desc(
          "Operand-read bandwidth all accelerators share, bytes per cycle "
          "(the FPGA fleet's single AXI read port). 0: every accelerator "
          "fills at its own read_bytes_per_cycle, independently"),
      llvm::cl::init(0.0)};

  Option<bool> report{
      *this, "report",
      llvm::cl::desc(
          "Emit a remark per linalg.matmul with the schedule's predicted "
          "makespan (read + compute + write), its compute-only makespan "
          "and the operand bytes it reads"),
      llvm::cl::init(false)};

  void getDependentDialects(
      DialectRegistry &registry) const override {
    registry.insert<
        linalg::LinalgDialect,
        tensor::TensorDialect,
        SystolicDialect>();
  }

  void runOnOperation() override {
    ModuleOp module = getOperation();

    // The physical systolic fleet, from systolic.device.
    FailureOr<SystolicFleet> fleet = collectSystolicFleet(module);
    if (failed(fleet)) {
      signalPassFailure();
      return;
    }

    if (fleet->resources.empty()) {
      module.emitError()
          << "systolic-binary-search-tile-matmul requires at least "
             "one systolic.device";
      signalPassFailure();
      return;
    }

    // Collect linalg.matmul first because the pass erases them.
    SmallVector<linalg::MatmulOp> targets;
    module.walk([&](linalg::MatmulOp op) { targets.push_back(op); });

    for (linalg::MatmulOp matmul : targets) {
      if (matmul->getNumResults() != 1)
        continue;

      // The decomposition slices A as M x K and B as K x N, which is what
      // the default indexing maps C(m, n) += A(m, k) * B(k, n) mean.
      // indexing_maps can transpose an operand (A^T B, A B^T); lowering such
      // a matmul by the default reading would compute a different product.
      if (matmul.hasUserDefinedMaps()) {
        matmul.emitError()
            << "linalg.matmul with user-defined indexing_maps is not "
               "supported; only C(m, n) += A(m, k) * B(k, n) is lowered";
        signalPassFailure();
        return;
      }

      auto aTy =
          dyn_cast<RankedTensorType>(matmul.getDpsInputs()[0].getType());
      auto bTy =
          dyn_cast<RankedTensorType>(matmul.getDpsInputs()[1].getType());
      auto cTy =
          dyn_cast<RankedTensorType>(matmul.getDpsInits()[0].getType());

      if (!aTy || !bTy || !cTy || !aTy.hasStaticShape() ||
          !bTy.hasStaticShape() || !cTy.hasStaticShape()) {
        matmul.emitError() << "requires static ranked tensor operands";
        signalPassFailure();
        return;
      }

      if (aTy.getRank() != 2 || bTy.getRank() != 2 || cTy.getRank() != 2) {
        matmul.emitError() << "requires rank-2 tensor matmul operands";
        signalPassFailure();
        return;
      }

      const int64_t M = aTy.getDimSize(0);
      const int64_t K = aTy.getDimSize(1);
      const int64_t BK = bTy.getDimSize(0);
      const int64_t N = bTy.getDimSize(1);

      // M and N need not be equal: optimizeSystolicBinarySearch pads each to
      // a multiple of the smallest geometry on its own and covers only the
      // original rows x columns cells.

      if (K != BK) {
        matmul.emitError() << "matmul reduction dimensions do not match";
        signalPassFailure();
        return;
      }

      if (cTy.getDimSize(0) != M || cTy.getDimSize(1) != N) {
        matmul.emitError() << "C shape does not match M x N";
        signalPassFailure();
        return;
      }

      // An empty dimension leaves nothing to run on an array, and the
      // K-folding divides by the number of invocations.
      if (M < 1 || N < 1 || K < 1) {
        matmul.emitError() << "requires non-empty M, N and K, got " << M
                           << "x" << N << "x" << K;
        signalPassFailure();
        return;
      }

      if (aTy.getElementType() != bTy.getElementType() ||
          aTy.getElementType() != cTy.getElementType()) {
        matmul.emitError() << "matmul element types must match";
        signalPassFailure();
        return;
      }

      // The binary-search decomposition, K-folded by every device's own
      // k_max and scheduled (SystolicTilePlan.h). An accelerator with a
      // smaller k_max is slower per tile by exactly what its extra
      // invocations cost, and the decomposition balances the geometries
      // accordingly.
      SystolicOptimizerOptions optimizerOptions;
      optimizerOptions.sharedReadBytesPerCycle = sharedReadBytesPerCycle;

      FailureOr<SystolicTilePlan> plan =
          planSystolicMatmul(matmul, M, N, K, *fleet, optimizerOptions);
      if (failed(plan)) {
        signalPassFailure();
        return;
      }

      if (report) {
        const ExactSystolicOptimizationResult &optimization =
            plan->optimization;
        matmul.emitRemark()
            << "binary-search schedule " << M << "x" << N << "x" << K
            << ": makespan = " << optimization.makespan
            << " cycles (read + compute + write), compute-only = "
            << optimization.computeOnlyMakespan << ", schedule end = "
            << optimization.scheduleEnd << ", operand bytes read = "
            << optimization.totalReadBytes << ", shared-read cycles = "
            << optimization.sharedReadCycles << ", tiles = "
            << optimization.decomposition.size();
      }

      OpBuilder builder(matmul);
      Value result = emitSystolicTilePlan(
          builder, matmul.getLoc(), matmul.getDpsInputs()[0],
          matmul.getDpsInputs()[1], matmul.getDpsInits()[0], *plan, *fleet);

      matmul.getResult(0).replaceAllUsesWith(result);
      matmul.erase();
    }
  }
};

} // namespace

std::unique_ptr<Pass>
mlir::systolic::createSystolicBinarySearchTileMatmulPass() {
  return std::make_unique<
      SystolicBinarySearchTileMatmulPass>();
}

std::unique_ptr<Pass>
mlir::systolic::createSystolicBinarySearchTileMatmulPass(
    double sharedReadBytesPerCycle) {
  return std::make_unique<
      SystolicBinarySearchTileMatmulPass>(sharedReadBytesPerCycle);
}

void mlir::systolic::registerSystolicBinarySearchTileMatmulPass() {
  PassRegistration<SystolicBinarySearchTileMatmulPass>();
}
