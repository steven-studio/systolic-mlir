#include "Systolic/SystolicCostAnalysisPass.h"
#include "CostModel.h"
#include "Systolic/SystolicOps.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"

#include "llvm/ADT/StringMap.h"

using namespace mlir;
using namespace mlir::systolic;

namespace {

struct SystolicCostAnalysisPass
    : public PassWrapper<SystolicCostAnalysisPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(SystolicCostAnalysisPass)

  StringRef getArgument() const final { return "systolic-cost-analysis"; }
  StringRef getDescription() const final {
    return "Annotate systolic.matmul_tile / systolic.dma ops with a "
           "closed-form cycle estimate, given their assigned device.";
  }

  void runOnOperation() override {
    ModuleOp module = getOperation();
    Builder builder(module.getContext());

    // Collect hardware configs for every declared device. Using the
    // Attr-suffixed accessors throughout (e.g. getClockHzAttr() rather
    // than getClockHz()) since those reliably return the raw, nullable
    // Attribute type across MLIR versions -- see README.md caveats.
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
      configs[device.getSymName()] = cfg;
    });

    module.walk([&](MatmulTileOp tile) {
      FlatSymbolRefAttr deviceRef = tile.getDeviceAttr();
      if (!deviceRef)
        return; // not yet assigned by a selection pass -- nothing to cost.
      auto it = configs.find(deviceRef.getValue());
      if (it == configs.end()) {
        tile.emitWarning() << "device '" << deviceRef
                            << "' has no matching systolic.device op";
        return;
      }
      int64_t cycles = ::systolic::estimateMatmulCycles(
          tile.getM(), tile.getN(), tile.getK(), it->second);
      tile.setEstCyclesAttr(builder.getI64IntegerAttr(cycles));
    });

    module.walk([&](DmaOp dma) {
      FlatSymbolRefAttr deviceRef = dma.getDeviceAttr();
      if (!deviceRef)
        return;
      auto it = configs.find(deviceRef.getValue());
      if (it == configs.end()) {
        dma.emitWarning() << "device '" << deviceRef
                           << "' has no matching systolic.device op";
        return;
      }
      int64_t cycles = ::systolic::estimateDmaCycles(
          dma.getBytes(), it->second.dmaBytesPerCycle);
      dma.setEstCyclesAttr(builder.getI64IntegerAttr(cycles));
    });
  }
};

} // namespace

std::unique_ptr<Pass> mlir::systolic::createSystolicCostAnalysisPass() {
  return std::make_unique<SystolicCostAnalysisPass>();
}
