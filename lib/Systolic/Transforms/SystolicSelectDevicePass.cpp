#include "Systolic/CostModel.h"
#include "Systolic/Passes.h"
#include "Systolic/SystolicOps.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/Pass/Pass.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"

#include <algorithm>

using namespace mlir;
using namespace mlir::systolic;

namespace {

// Bytes the A (m x k) and B (k x n) operands of one tile occupy, at the
// element width the tile carries -- 4 bytes for f32. Output write-back is
// not counted. This is the footprint the optional l1_bytes check compares
// against a device's declared scratchpad; it is a coarse admissibility
// filter, not an allocator.
static int64_t operandFootprintBytes(MatmulTileOp tile) {
  int64_t elemBytes = 4;
  if (auto tensorTy = llvm::dyn_cast<RankedTensorType>(tile.getA().getType()))
    if (tensorTy.getElementType().isIntOrFloat())
      elemBytes = std::max<int64_t>(
          1, tensorTy.getElementType().getIntOrFloatBitWidth() / 8);
  return (tile.getM() * tile.getK() + tile.getK() * tile.getN()) * elemBytes;
}

// Stage-1 accelerator selection. Greedy list scheduling for unrelated
// parallel machines (R||Cmax): process tiles largest-first, and for each
// tile assign it to whichever declared device currently minimizes
// (that device's running load + this tile's cost on that device).
// Not optimal, but a well-understood, cheap heuristic -- a natural next
// step later is swapping this greedy core for an LP-relaxation + rounding
// scheme (Lenstra-Shmoys-Tardos style) for a provable approximation ratio.
struct SystolicSelectDevicePass
    : public PassWrapper<SystolicSelectDevicePass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(SystolicSelectDevicePass)
  SystolicSelectDevicePass() = default;
  SystolicSelectDevicePass(const SystolicSelectDevicePass &other)
      : PassWrapper(other) {
    copyOptionValuesFrom(&other);
  }

  // Local-scratchpad admissibility filter. A device that declares
  // `l1_bytes` is excluded for a tile whose A and B operands do not fit in
  // it; a device that does not declare it is unconstrained, so IR that
  // predates the check schedules exactly as it did before. The check can
  // only remove candidates, never add a faster one, so enabling it cannot
  // improve a predicted makespan -- it replaces a prediction that
  // described an execution the hardware could not perform with one that
  // does not. A tile that fits no declared device is an error: the remedy
  // is to re-tile it, not to place it anyway.
  Option<bool> enforceL1Capacity{
      *this, "enforce-l1-capacity",
      llvm::cl::desc("Exclude devices whose declared l1_bytes cannot hold a "
                     "tile's A and B operands"),
      llvm::cl::init(true)};

  StringRef getArgument() const final { return "systolic-select-device"; }
  StringRef getDescription() const final {
    return "Stage-1 accelerator selection: greedily assign each "
           "unassigned systolic.matmul_tile to the declared systolic.device "
           "that minimizes overall makespan, using the closed-form cost "
           "model.";
  }

  void runOnOperation() override {
    ModuleOp module = getOperation();
    Builder builder(module.getContext());

    // 1. Collect all declared devices + configs.
    llvm::StringMap<::systolic::ArrayConfig> configs;
    SmallVector<std::string> deviceNames;
    module.walk([&](DeviceOp device) {
      ::systolic::ArrayConfig cfg;
      cfg.rows = device.getRows();
      cfg.cols = device.getCols();
      if (IntegerAttr km = device.getKMaxAttr())
        cfg.kMax = km.getInt();
      if (IntegerAttr l1 = device.getL1BytesAttr())
        cfg.l1Bytes = l1.getInt();
      if (FloatAttr clk = device.getClockHzAttr())
        cfg.clockHz = clk.getValueAsDouble();
      if (FloatAttr bw = device.getDmaBytesPerCycleAttr())
        cfg.dmaBytesPerCycle = bw.getValueAsDouble();
      // Calibration is a property of the microarchitecture, not of the
      // array shape, so it travels on the device op. Leaving the attribute
      // off keeps the ArrayConfig default (H = 0, the geometric model).
      if (IntegerAttr to = device.getTileOverheadAttr())
        cfg.tileOverhead = to.getInt();
      std::string name = device.getSymName().str();
      configs[name] = cfg;
      deviceNames.push_back(name);
    });

    if (deviceNames.empty()) {
      module.emitWarning() << "systolic-select-device: no systolic.device "
                               "ops found, nothing to assign";
      return;
    }

    // 2. Collect unassigned tiles.
    SmallVector<MatmulTileOp> tiles;
    module.walk([&](MatmulTileOp tile) {
      if (!tile.getDeviceAttr())
        tiles.push_back(tile);
    });

    // 3. Largest-first ordering: schedule big tasks before they get stuck
    //    queued behind a pile of small ones near the end.
    llvm::sort(tiles, [](MatmulTileOp a, MatmulTileOp b) {
      int64_t volA = a.getM() * a.getN() * a.getK();
      int64_t volB = b.getM() * b.getN() * b.getK();
      return volA > volB;
    });

    // 4. Running load per device, greedily minimized per tile.
    llvm::StringMap<int64_t> load;
    for (const std::string &name : deviceNames)
      load[name] = 0;

    for (MatmulTileOp tile : tiles) {
      int64_t footprintBytes = operandFootprintBytes(tile);
      std::string best;
      int64_t bestFinish = -1;
      int64_t bestCost = -1;
      for (const std::string &name : deviceNames) {
        const ::systolic::ArrayConfig &cfg = configs[name];
        if (enforceL1Capacity && cfg.l1Bytes > 0 &&
            footprintBytes > cfg.l1Bytes)
          continue;
        int64_t cost = ::systolic::estimateMatmulCycles(
            tile.getM(), tile.getN(), tile.getK(), cfg);
        int64_t finish = load[name] + cost;
        if (bestFinish < 0 || finish < bestFinish) {
          bestFinish = finish;
          bestCost = cost;
          best = name;
        }
      }
      if (bestFinish < 0) {
        tile.emitError()
            << "no declared systolic.device can hold this tile's operands ("
            << footprintBytes << " bytes); re-tile it or raise l1_bytes";
        signalPassFailure();
        return;
      }
      tile.setDeviceAttr(FlatSymbolRefAttr::get(module.getContext(), best));
      tile.setEstCyclesAttr(builder.getI64IntegerAttr(bestCost));
      load[best] = bestFinish;
    }
  }
};

} // namespace

std::unique_ptr<Pass> mlir::systolic::createSystolicSelectDevicePass() {
  return std::make_unique<SystolicSelectDevicePass>();
}

void mlir::systolic::registerSystolicSelectDevicePass() {
  PassRegistration<SystolicSelectDevicePass>();
}
