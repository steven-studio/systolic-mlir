#include "Systolic/Passes.h"
#include "Systolic/SystolicOptimizer.h"
#include "Systolic/SystolicOps.h"
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

  void getDependentDialects(
      DialectRegistry &registry) const override {
    registry.insert<
        linalg::LinalgDialect,
        tensor::TensorDialect,
        SystolicDialect>();
  }

  void runOnOperation() override {
    ModuleOp module = getOperation();

    // ----------------------------------------------------------
    // Collect the physical systolic fleet from systolic.device.
    // ----------------------------------------------------------

    SmallVector<SystolicArrayResource> fleet;
    SmallVector<FlatSymbolRefAttr> acceleratorSymbols;

    // k_max of each accelerator, indexed like acceleratorSymbols; 0 when
    // the device does not declare one.
    SmallVector<int64_t> acceleratorKMax;

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

      // Reduction capacity of one invocation. A device that declares
      // k_max gets every tile's reduction folded into ceil(K / k_max)
      // invocations below; a device that does not keeps K whole, which
      // is exactly what this pass emitted before folding existed.
      int64_t kMax = 0;

      if (IntegerAttr attr = device.getKMaxAttr()) {
        kMax = attr.getInt();

        if (kMax <= 0) {
          device.emitError()
              << "k_max must be positive";
          signalPassFailure();
          return;
        }
      }

      SystolicArrayResource resource;
      resource.arraySize = rows;
      resource.acceleratorId = nextAcceleratorId++;
      resource.tileOverhead = tileOverhead;
      acceleratorKMax.push_back(kMax);

      auto symName =
          device->getAttrOfType<StringAttr>("sym_name");

      if (!symName) {
        device.emitError()
            << "systolic.device must have a symbol name";
        signalPassFailure();
        return;
      }

      acceleratorSymbols.push_back(
          FlatSymbolRefAttr::get(
              device.getContext(),
              symName.getValue()));

      fleet.push_back(resource);
    });

    if (fleet.empty()) {
      module.emitError()
          << "systolic-binary-search-tile-matmul requires at least "
             "one systolic.device";
      signalPassFailure();
      return;
    }

    // ----------------------------------------------------------
    // Collect linalg.matmul first because the pass erases them.
    // ----------------------------------------------------------

    SmallVector<linalg::MatmulOp> targets;

    module.walk([&](linalg::MatmulOp op) {
      targets.push_back(op);
    });

    // ----------------------------------------------------------
    // Lower every static tensor linalg.matmul.
    // ----------------------------------------------------------

    for (linalg::MatmulOp matmul : targets) {
      if (matmul->getNumResults() != 1)
        continue;

      auto aTy =
          dyn_cast<RankedTensorType>(
              matmul.getDpsInputs()[0].getType());

      auto bTy =
          dyn_cast<RankedTensorType>(
              matmul.getDpsInputs()[1].getType());

      auto cTy =
          dyn_cast<RankedTensorType>(
              matmul.getDpsInits()[0].getType());

      if (!aTy || !bTy || !cTy ||
          !aTy.hasStaticShape() ||
          !bTy.hasStaticShape() ||
          !cTy.hasStaticShape()) {
        matmul.emitError()
            << "requires static ranked tensor operands";
        signalPassFailure();
        return;
      }

      if (aTy.getRank() != 2 ||
          bTy.getRank() != 2 ||
          cTy.getRank() != 2) {
        matmul.emitError()
            << "requires rank-2 tensor matmul operands";
        signalPassFailure();
        return;
      }

      const int64_t M = aTy.getDimSize(0);
      const int64_t K = aTy.getDimSize(1);
      const int64_t BK = bTy.getDimSize(0);
      const int64_t N = bTy.getDimSize(1);

      // M and N need not be equal. optimizeSystolicBinarySearch takes
      // (rows, columns) separately, pads each to a multiple of the
      // smallest geometry on its own, and covers only the original
      // rows x columns cells; this pass used to reject M != N before
      // ever asking it.

      if (K != BK) {
        matmul.emitError()
            << "matmul reduction dimensions do not match";
        signalPassFailure();
        return;
      }

      if (cTy.getDimSize(0) != M ||
          cTy.getDimSize(1) != N) {
        matmul.emitError()
            << "C shape does not match M x N";
        signalPassFailure();
        return;
      }

      if (aTy.getElementType() != bTy.getElementType() ||
          aTy.getElementType() != cTy.getElementType()) {
        matmul.emitError()
            << "matmul element types must match";
        signalPassFailure();
        return;
      }

      // --------------------------------------------------------
      // Ask the already-tested binary-search optimizer for the
      // spatial decomposition.
      // --------------------------------------------------------

      auto optimization =
          optimizeSystolicBinarySearch(
              M,
              N,
              fleet);

      if (failed(optimization)) {
        matmul.emitError()
            << "binary-search systolic optimization failed for "
            << M << "x" << N;
        signalPassFailure();
        return;
      }

      OpBuilder builder(matmul);
      Location loc = matmul.getLoc();

      Value A = matmul.getDpsInputs()[0];
      Value B = matmul.getDpsInputs()[1];
      Value C = matmul.getDpsInits()[0];

      SmallVector<OpFoldResult> strides(
          2, builder.getIndexAttr(1));

      Value result = C;

      // --------------------------------------------------------
      // Materialize exactly the optimizer's spatial decomposition.
      //
      // Each spatial task covers a tileM x tileN block of C. Its
      // reduction is folded by the assigned device's k_max (see the
      // K-folding block below), so the resulting matmul_tiles are
      //
      //   tileM x k_j  @  k_j x tileN  ->  tileM x tileN
      //
      // with sum_j k_j = K, chained through c_in. The spatial
      // decomposition and its device assignment are the optimizer's,
      // unchanged; folding only splits each task's reduction.
      // --------------------------------------------------------

      for (const SystolicExecutionTask &task :
           optimization->decomposition) {

        if (task.size <= 0) {
          matmul.emitError()
              << "optimizer returned non-positive tile size";
          signalPassFailure();
          return;
        }

        if (task.acceleratorId < 0 ||
            task.acceleratorId >=
                static_cast<int64_t>(
                    acceleratorSymbols.size())) {
          matmul.emitError()
              << "optimizer returned invalid accelerator id "
              << task.acceleratorId;
          signalPassFailure();
          return;
        }

        if (task.row < 0 ||
            task.column < 0 ||
            task.row >= M ||
            task.column >= N) {
          matmul.emitError()
              << "optimizer returned invalid tile origin "
              << "(" << task.row << ", " << task.column << ")";
          signalPassFailure();
          return;
        }

        // Valid portion of this spatial tile.
        //
        // The optimizer may use an internally padded domain.
        // A tile touching the boundary is therefore clipped to
        // the original matrix when creating tensor slices.
        const int64_t m =
            std::min<int64_t>(
                task.size,
                M - task.row);

        const int64_t n =
            std::min<int64_t>(
                task.size,
                N - task.column);

        // A padding-only tile would have been outside the original
        // domain. The optimizer promises not to emit one, but keep
        // this guard here at the IR boundary as a second invariant.
        if (m <= 0 || n <= 0)
          continue;

        // ------------------------------------------------------
        // Recover the physical schedule selected by the optimizer.
        //
        // The decomposition task identifies the spatial tile and
        // physical accelerator.  The corresponding schedule entry
        // supplies the compute-only execution interval.
        // ------------------------------------------------------

        const ScheduledSystolicTile *scheduledTile = nullptr;

        for (const ScheduledSystolicTile &candidate :
             optimization->schedule) {
          if (candidate.tile.row == task.row &&
              candidate.tile.column == task.column &&
              candidate.tile.size == task.size &&
              candidate.tile.acceleratorId ==
                  task.acceleratorId) {
            scheduledTile = &candidate;
            break;
          }
        }

        if (!scheduledTile) {
          matmul.emitError()
              << "optimizer decomposition task has no matching "
                 "schedule entry for tile ("
              << task.row << ", " << task.column
              << ", size=" << task.size
              << ", accelerator="
              << task.acceleratorId << ")";
          signalPassFailure();
          return;
        }

        if (scheduledTile->startCycle < 0 ||
            scheduledTile->endCycle <
                scheduledTile->startCycle) {
          matmul.emitError()
              << "optimizer returned invalid schedule interval "
                 "for tile ("
              << task.row << ", " << task.column
              << ", size=" << task.size << ")";
          signalPassFailure();
          return;
        }

        const int64_t estCycles =
            scheduledTile->endCycle -
            scheduledTile->startCycle;

        const int64_t startCycle =
            scheduledTile->startCycle;

        // ------------------------------------------------------
        // K-folding.
        //
        // One systolic.matmul_tile is one invocation of its device:
        // the operand buffers hold at most k_max reduction steps, so
        // a tile whose reduction is longer runs as
        //
        //   I = ceil(K / k_max)
        //
        // invocations, invocation j covering
        // [j * k_max, min(K, (j + 1) * k_max)), chained through c_in
        // -- the chaining --systolic-tile-matmul uses for its K tiles.
        // A device without k_max keeps K whole (I = 1).
        //
        // The optimizer bills one invocation of cost C(g) per spatial
        // tile and starts it at round * C(g). Every invocation of a
        // folded tile is billed the same C(g), so each device's
        // timeline is stretched by I: invocation j of the tile the
        // optimizer started at round * C(g) starts at
        // (round * I + j) * C(g) = startCycle * I + j * C(g).
        // Order and non-overlap on every device are preserved.
        // ------------------------------------------------------

        const int64_t kMax =
            acceleratorKMax[task.acceleratorId];

        const int64_t kStep = kMax > 0 ? kMax : K;

        const int64_t numInvocations =
            (K + kStep - 1) / kStep;

        if (startCycle >
                (std::numeric_limits<int64_t>::max() -
                 numInvocations * estCycles) /
                    numInvocations) {
          matmul.emitError()
              << "folded schedule overflows int64 for tile ("
              << task.row << ", " << task.column << ")";
          signalPassFailure();
          return;
        }

        OpFoldResult offM =
            builder.getIndexAttr(task.row);
        OpFoldResult offN =
            builder.getIndexAttr(task.column);

        SmallVector<OpFoldResult> cOff{
            offM,
            offN};

        SmallVector<OpFoldResult> cSize{
            builder.getIndexAttr(m),
            builder.getIndexAttr(n)};

        Value cTile =
            builder.create<tensor::ExtractSliceOp>(
                loc,
                result,
                cOff,
                cSize,
                strides);

        auto tileTy =
            RankedTensorType::get(
                {m, n},
                aTy.getElementType());

        for (int64_t j = 0; j < numInvocations; ++j) {
          const int64_t k0 = j * kStep;
          const int64_t kc = std::min<int64_t>(kStep, K - k0);

          SmallVector<OpFoldResult> aOff{
              offM,
              builder.getIndexAttr(k0)};

          SmallVector<OpFoldResult> aSize{
              builder.getIndexAttr(m),
              builder.getIndexAttr(kc)};

          Value aTile =
              builder.create<tensor::ExtractSliceOp>(
                  loc,
                  A,
                  aOff,
                  aSize,
                  strides);

          SmallVector<OpFoldResult> bOff{
              builder.getIndexAttr(k0),
              offN};

          SmallVector<OpFoldResult> bSize{
              builder.getIndexAttr(kc),
              builder.getIndexAttr(n)};

          Value bTile =
              builder.create<tensor::ExtractSliceOp>(
                  loc,
                  B,
                  bOff,
                  bSize,
                  strides);

          const int64_t invocationStart =
              startCycle * numInvocations + j * estCycles;

          cTile =
              builder.create<MatmulTileOp>(
                  loc,
                  tileTy,
                  aTile,
                  bTile,
                  cTile,
                  builder.getI64IntegerAttr(m),
                  builder.getI64IntegerAttr(n),
                  builder.getI64IntegerAttr(kc),
                  /*device=*/acceleratorSymbols[
                      task.acceleratorId],
                  /*est_cycles=*/
                  builder.getI64IntegerAttr(estCycles),
                  /*start_cycle=*/
                  builder.getI64IntegerAttr(invocationStart));
        }

        result =
            builder.create<tensor::InsertSliceOp>(
                loc,
                cTile,
                result,
                cOff,
                cSize,
                strides);
      }

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

void mlir::systolic::registerSystolicBinarySearchTileMatmulPass() {
  PassRegistration<SystolicBinarySearchTileMatmulPass>();
}
