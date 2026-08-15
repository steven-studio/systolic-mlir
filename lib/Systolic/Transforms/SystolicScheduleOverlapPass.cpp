#include "Systolic/CostModel.h"
#include "Systolic/Passes.h"
#include "Systolic/SystolicOps.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"

#include "llvm/ADT/StringMap.h"

using namespace mlir;
using namespace mlir::systolic;

namespace {

// Stage-3 overlap-aware scheduling. Each device has two independent
// resources: a compute engine and a DMA (prefetch) engine. DMA for tile
// i+1 can run concurrently with compute for tile i (double buffering);
// the only hard constraints are (a) a tile's own prefetch must finish
// before its compute starts, and (b) one compute engine per device
// processes tiles strictly in program order. This is intentionally NOT
// full serialization (DMA-then-compute-then-DMA...), which is the
// simplification most closely related work (e.g. MATCHA) makes.
//
// LIMITATION (v1): this models an implicit synthetic prefetch per tile
// (sized from the tile's own operand volume) and does not yet unify with
// pre-existing standalone systolic.dma ops in the same IR -- that's a
// follow-up once this overlap model itself is validated.
struct SystolicScheduleOverlapPass
    : public PassWrapper<SystolicScheduleOverlapPass,
                          OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(SystolicScheduleOverlapPass)

  StringRef getArgument() const final { return "systolic-schedule-overlap"; }
  StringRef getDescription() const final {
    return "Stage-3 overlap-aware scheduling: compute each assigned "
           "systolic.matmul_tile's start_cycle assuming its prefetch DMA "
           "overlaps with the previous tile's compute on the same device.";
  }

  void runOnOperation() override {
    ModuleOp module = getOperation();
    Builder builder(module.getContext());

    llvm::StringMap<::systolic::ArrayConfig> configs;
    module.walk([&](DeviceOp device) {
      ::systolic::ArrayConfig cfg;
      cfg.rows = device.getRows();
      cfg.cols = device.getCols();
      cfg.depth = device.getDepth();
      if (FloatAttr clk = device.getClockHzAttr())
        cfg.clockHz = clk.getValueAsDouble();
      if (FloatAttr bw = device.getDmaBytesPerCycleAttr())
        cfg.dmaBytesPerCycle = bw.getValueAsDouble();
      // Calibration is a property of the microarchitecture, not of the
      // array shape, so it travels on the device op. Leaving the attribute
      // off keeps the ArrayConfig default.
      if (IntegerAttr ii = device.getInitiationIntervalAttr())
        cfg.initiationInterval = ii.getInt();
      if (IntegerAttr fo = device.getFixedOverheadAttr())
        cfg.fixedOverhead = fo.getInt();
      configs[device.getSymName()] = cfg;
    });

    llvm::StringMap<int64_t> computeEnd; // per-device compute-engine clock
    llvm::StringMap<int64_t> dmaEnd;     // per-device DMA-engine clock

    module.walk([&](MatmulTileOp tile) {
      FlatSymbolRefAttr deviceRef = tile.getDeviceAttr();
      if (!deviceRef)
        return; // unassigned tiles have no schedule yet
      StringRef dev = deviceRef.getValue();
      auto cfgIt = configs.find(dev);
      if (cfgIt == configs.end()) {
        tile.emitWarning() << "device '" << deviceRef
                            << "' has no matching systolic.device op";
        return;
      }
      const ::systolic::ArrayConfig &cfg = cfgIt->second;

      int64_t m = tile.getM(), n = tile.getN(), k = tile.getK();

      // Synthetic prefetch volume: A-tile (m*k) + B-tile (k*n) elements,
      // 4 bytes/elem (fp32). Output write-back is ignored in this v1.
      int64_t bytes = (m * k + k * n) * 4;
      int64_t dmaCost =
          ::systolic::estimateDmaCycles(bytes, cfg.dmaBytesPerCycle);

      int64_t computeCost;
      if (IntegerAttr est = tile.getEstCyclesAttr())
        computeCost = est.getInt();
      else
        computeCost = ::systolic::estimateMatmulCycles(m, n, k, cfg);

      int64_t &devDmaEnd = dmaEnd[dev];
      int64_t &devComputeEnd = computeEnd[dev];

      int64_t dmaStart = devDmaEnd;
      int64_t dmaFinish = dmaStart + dmaCost;
      devDmaEnd = dmaFinish;

      int64_t computeStart = std::max(devComputeEnd, dmaFinish);
      int64_t computeFinish = computeStart + computeCost;
      devComputeEnd = computeFinish;

      tile.setStartCycleAttr(builder.getI64IntegerAttr(computeStart));
      if (!tile.getEstCyclesAttr())
        tile.setEstCyclesAttr(builder.getI64IntegerAttr(computeCost));
    });
  }
};

} // namespace

std::unique_ptr<Pass> mlir::systolic::createSystolicScheduleOverlapPass() {
  return std::make_unique<SystolicScheduleOverlapPass>();
}

void mlir::systolic::registerSystolicScheduleOverlapPass() {
  PassRegistration<SystolicScheduleOverlapPass>();
}
