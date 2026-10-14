#include "Systolic/CostModel.h"
#include "Systolic/DeviceConfig.h"
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

    // ----------------------------------------------------------
    // Collect the physical systolic fleet from systolic.device.
    // ----------------------------------------------------------

    SmallVector<SystolicArrayResource> fleet;
    SmallVector<FlatSymbolRefAttr> acceleratorSymbols;

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
      if (IntegerAttr attr = device.getKMaxAttr()) {
        if (attr.getInt() <= 0) {
          device.emitError()
              << "k_max must be positive";
          signalPassFailure();
          return;
        }
      }

      // Everything the cost model prices an invocation with (k_max, H,
      // the read and write terms) -- read once, through the same helper
      // every costing pass uses.
      SystolicArrayResource resource;
      resource.arraySize = rows;
      resource.acceleratorId = nextAcceleratorId++;
      resource.config = getArrayConfig(device);

      if (!::systolic::isValidArrayConfig(resource.config)) {
        device.emitError()
            << "systolic.device has cost parameters the cost model "
               "cannot use";
        signalPassFailure();
        return;
      }

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

    // k_max is per accelerator and the geometries are independent: the 8x8
    // and the 4x4s of one fleet may declare different values, and none of
    // them has to be a power of two. Within ONE geometry they must agree,
    // and so must every other cost parameter: the optimizer bills one cost
    // per geometry (the cost model's tile cost, sum over I(g) =
    // ceil(K / k_max(g)) invocations) and spreads a geometry's tiles
    // round-robin over its accelerators.
    for (size_t i = 0; i < fleet.size(); ++i) {
      for (size_t j = 0; j < i; ++j) {
        if (fleet[i].arraySize != fleet[j].arraySize)
          continue;
        const ::systolic::ArrayConfig &a = fleet[i].config;
        const ::systolic::ArrayConfig &b = fleet[j].config;
        if (a.kMax != b.kMax) {
          auto describe = [](int64_t kMax) {
            return kMax > 0 ? std::to_string(kMax) : std::string("none");
          };
          module.emitError()
              << "systolic.device " << acceleratorSymbols[i]
              << " declares k_max = " << describe(a.kMax)
              << " but " << acceleratorSymbols[j] << " of the same "
              << fleet[i].arraySize << "x" << fleet[i].arraySize
              << " geometry declares k_max = " << describe(b.kMax)
              << "; the binary-search decomposition bills one cost per "
                 "geometry, so every accelerator of one geometry must "
                 "declare the same k_max";
          signalPassFailure();
          return;
        }
        if (a.tileOverhead != b.tileOverhead ||
            a.readBytesPerCycle != b.readBytesPerCycle ||
            a.writeBytesPerCycle != b.writeBytesPerCycle ||
            a.readOverhead != b.readOverhead ||
            a.writeOverhead != b.writeOverhead ||
            a.operandLanes != b.operandLanes || a.kWindow != b.kWindow ||
            a.elementBytes != b.elementBytes) {
          module.emitError()
              << "systolic.device " << acceleratorSymbols[i] << " and "
              << acceleratorSymbols[j] << " of the same "
              << fleet[i].arraySize << "x" << fleet[i].arraySize
              << " geometry declare different cost parameters "
                 "(tile_overhead, read/write terms, operand_lanes or "
                 "k_window); the binary-search decomposition bills one "
                 "cost per geometry";
          signalPassFailure();
          return;
        }
      }
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
    //
    // Every matmul_tile this pass emits also carries four discardable
    // attributes that name its place in the GEMM, for the asynchronous
    // FPGA lowering (TileMatmulForFpga.cpp, async=true), which hands the
    // tiles of a GEMM to the runtime without waiting for each one and
    // needs to know which tiles accumulate into the same output block, and
    // in what order:
    //   systolic.gemm     which linalg.matmul of the module (0, 1, ...)
    //   systolic.tile     which output block of that GEMM (the optimizer's
    //                     task index)
    //   systolic.k_index  which K fold of that block (0 .. k_count - 1)
    //   systolic.k_count  how many K folds the block has
    // The synchronous lowering ignores them.
    // ----------------------------------------------------------

    int64_t gemmIndex = -1;
    for (linalg::MatmulOp matmul : targets) {
      if (matmul->getNumResults() != 1)
        continue;
      ++gemmIndex;

      // The decomposition below slices A as M x K and B as K x N, which is
      // what the default indexing maps C(m, n) += A(m, k) * B(k, n) mean.
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

      // An empty dimension leaves nothing to run on an array, and the
      // K-folding below divides by the number of invocations.
      if (M < 1 || N < 1 || K < 1) {
        matmul.emitError()
            << "requires non-empty M, N and K, got "
            << M << "x" << N << "x" << K;
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
      // Ask the binary-search optimizer for the spatial
      // decomposition and its schedule.
      //
      // Every accelerator folds this GEMM's K by its own k_max into
      // I(d) = ceil(K / k_max(d)) invocations (1 without k_max), and
      // the optimizer prices a spatial tile on it with the cost
      // model: the sum of those invocations' read + compute + write
      // (CostModel.h, estimateTileCost). An accelerator with a
      // smaller k_max is slower per tile by exactly what its extra
      // invocations cost, and the decomposition balances the
      // geometries accordingly.
      // --------------------------------------------------------

      SystolicOptimizerOptions optimizerOptions;
      optimizerOptions.sharedReadBytesPerCycle = sharedReadBytesPerCycle;

      auto optimization =
          optimizeSystolicBinarySearch(
              M,
              N,
              K,
              fleet,
              optimizerOptions);

      if (failed(optimization)) {
        matmul.emitError()
            << "binary-search systolic optimization failed for "
            << M << "x" << N << "x" << K;
        signalPassFailure();
        return;
      }

      if (report)
        matmul.emitRemark()
            << "binary-search schedule " << M << "x" << N << "x" << K
            << ": makespan = " << optimization->makespan
            << " cycles (read + compute + write), compute-only = "
            << optimization->computeOnlyMakespan << ", schedule end = "
            << optimization->scheduleEnd << ", operand bytes read = "
            << optimization->totalReadBytes << ", shared-read cycles = "
            << optimization->sharedReadCycles << ", tiles = "
            << optimization->decomposition.size();

      // --------------------------------------------------------
      // The compute clock of every accelerator.
      //
      // The optimizer's schedule is on the full timeline: a tile
      // holds its accelerator for every invocation's read, compute
      // and write. est_cycles has always meant an invocation's
      // compute duration -- in every costing pass, and in the
      // hardware scheduler (systolic_hw_scheduler.sv), which takes
      // it as req_compute_cycles. The IR keeps that meaning and
      // carries the optimizer's schedule projected onto compute
      // time: on each accelerator, in the order the optimizer runs
      // its tiles, an invocation's start_cycle is where the
      // previous one's compute term ends. The scheduler gates a
      // fold on start_cycle against a clock that runs while any
      // accelerator holds a job, from its acceptance (fill
      // included) to c_valid; issued one at a time, as the runtime
      // does, neither this nor the full read + compute + write
      // timeline makes it hold a fold
      // (eval/cost_model_validation/start_cycle_check.py). The full
      // timeline is in the remark (`report`) and in systolic-cost.
      // --------------------------------------------------------

      SmallVector<int64_t> tileComputeStart(
          optimization->decomposition.size(), 0);
      {
        SmallVector<size_t> order(optimization->decomposition.size());
        for (size_t i = 0; i < order.size(); ++i)
          order[i] = i;
        std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) {
          const ScheduledSystolicTile &x = optimization->schedule[a];
          const ScheduledSystolicTile &y = optimization->schedule[b];
          if (x.tile.acceleratorId != y.tile.acceleratorId)
            return x.tile.acceleratorId < y.tile.acceleratorId;
          return x.startCycle < y.startCycle;
        });
        int64_t accelerator = -1;
        int64_t clock = 0;
        for (size_t idx : order) {
          const SystolicExecutionTask &task =
              optimization->decomposition[idx];
          if (task.acceleratorId != accelerator) {
            accelerator = task.acceleratorId;
            clock = 0;
          }
          tileComputeStart[idx] = clock;
          if (task.acceleratorId < 0 ||
              task.acceleratorId >= static_cast<int64_t>(fleet.size())) {
            matmul.emitError()
                << "optimizer returned invalid accelerator id "
                << task.acceleratorId;
            signalPassFailure();
            return;
          }
          const ::systolic::TileCost tileCost = ::systolic::estimateTileCost(
              K, fleet[task.acceleratorId].config);
          if (!tileCost.ok ||
              __builtin_add_overflow(clock, tileCost.compute, &clock)) {
            matmul.emitError()
                << "cost model cannot price a " << task.size << "x"
                << task.size << " tile of reduction " << K;
            signalPassFailure();
            return;
          }
        }
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

      for (size_t taskIndex = 0;
           taskIndex < optimization->decomposition.size(); ++taskIndex) {
        const SystolicExecutionTask &task =
            optimization->decomposition[taskIndex];

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
        // decomposition[i] and schedule[i] describe the same tile:
        // the task names the spatial tile and its accelerator, the
        // schedule entry the interval the tile holds that
        // accelerator for (every invocation's read, compute and
        // write).
        // ------------------------------------------------------

        if (taskIndex >= optimization->schedule.size()) {
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

        const ScheduledSystolicTile *scheduledTile =
            &optimization->schedule[taskIndex];

        if (scheduledTile->tile.row != task.row ||
            scheduledTile->tile.column != task.column ||
            scheduledTile->tile.size != task.size ||
            scheduledTile->tile.acceleratorId != task.acceleratorId) {
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

        // ------------------------------------------------------
        // K-folding.
        //
        // One systolic.matmul_tile is one invocation of its device:
        // the operand buffers of accelerator d hold at most
        // k_max(d) reduction steps, so a tile whose reduction is
        // longer runs as
        //
        //   I(d) = ceil(K / k_max(d))
        //
        // invocations, invocation j covering
        // [j * k_max(d), min(K, (j + 1) * k_max(d))), so every
        // invocation has 1 <= k <= k_max(d), chained through c_in
        // -- the chaining --systolic-tile-matmul uses for its K
        // tiles. A device without k_max keeps K whole (I = 1).
        // k_max(d) is the device's own: the 8x8 and the 4x4s fold
        // the same K independently.
        //
        // Invocation j runs exactly k_j steps (k_max is a capacity)
        // and costs what the cost model says it costs: the last one
        // is the short one, so the tile is NOT split into equal
        // parts. Its
        // invocations run back to back: est_cycles is invocation
        // j's compute term, start_cycle the tile's compute start
        // plus the compute terms of invocations 0 .. j-1. Their
        // read + compute + write must add up to exactly the
        // interval the optimizer scheduled the tile over; anything
        // else would mean the optimizer and the IR were priced by
        // two different models.
        // ------------------------------------------------------

        const ::systolic::ArrayConfig &deviceConfig =
            fleet[task.acceleratorId].config;

        const int64_t kStep =
            deviceConfig.kMax > 0 ? deviceConfig.kMax : K;

        const int64_t numInvocations =
            ::systolic::countInvocations(K, deviceConfig);

        SmallVector<::systolic::InvocationCost> invocationCosts;
        int64_t sequentialCycles = 0;
        for (int64_t j = 0; j < numInvocations; ++j) {
          ::systolic::InvocationCost cost =
              ::systolic::estimateInvocationCost(K, j, deviceConfig);
          if (!cost.ok ||
              __builtin_add_overflow(sequentialCycles, cost.sequential(),
                                     &sequentialCycles)) {
            matmul.emitError()
                << "cost model cannot price invocation " << j
                << " of tile (" << task.row << ", " << task.column
                << ")";
            signalPassFailure();
            return;
          }
          invocationCosts.push_back(cost);
        }

        if (numInvocations < 1 ||
            numInvocations != (K + kStep - 1) / kStep ||
            sequentialCycles !=
                scheduledTile->endCycle - scheduledTile->startCycle) {
          matmul.emitError()
              << "optimizer scheduled tile (" << task.row << ", "
              << task.column << ") over "
              << scheduledTile->endCycle - scheduledTile->startCycle
              << " cycles, but its " << numInvocations
              << " invocations cost " << sequentialCycles;
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

        int64_t computeClock = tileComputeStart[taskIndex];

        for (int64_t j = 0; j < numInvocations; ++j) {
          const int64_t k0 = j * kStep;
          const int64_t kc = std::min<int64_t>(kStep, K - k0);
          if (kc != invocationCosts[j].k) {
            matmul.emitError()
                << "invocation " << j << " covers " << kc
                << " reduction steps but was priced for "
                << invocationCosts[j].k;
            signalPassFailure();
            return;
          }

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

          const int64_t estCycles = invocationCosts[j].compute;
          const int64_t invocationStart = computeClock;
          computeClock += estCycles;

          auto tile =
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
          tile->setAttr("systolic.gemm",
                        builder.getI64IntegerAttr(gemmIndex));
          tile->setAttr("systolic.tile",
                        builder.getI64IntegerAttr(
                            static_cast<int64_t>(taskIndex)));
          tile->setAttr("systolic.k_index",
                        builder.getI64IntegerAttr(j));
          tile->setAttr("systolic.k_count",
                        builder.getI64IntegerAttr(numInvocations));
          cTile = tile.getResult();
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

std::unique_ptr<Pass>
mlir::systolic::createSystolicBinarySearchTileMatmulPass(
    double sharedReadBytesPerCycle) {
  return std::make_unique<
      SystolicBinarySearchTileMatmulPass>(sharedReadBytesPerCycle);
}

void mlir::systolic::registerSystolicBinarySearchTileMatmulPass() {
  PassRegistration<SystolicBinarySearchTileMatmulPass>();
}
