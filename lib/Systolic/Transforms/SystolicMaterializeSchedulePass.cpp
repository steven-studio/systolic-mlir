#include "Systolic/Passes.h"
#include "Systolic/SystolicOps.h"
#include "Systolic/SystolicScheduling.h"
#include "Systolic/SystolicTiling.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"

using namespace mlir;
using namespace mlir::systolic;

namespace {

struct SystolicMaterializeSchedulePass
    : public PassWrapper<SystolicMaterializeSchedulePass,
                         OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(
      SystolicMaterializeSchedulePass)

  StringRef getArgument() const final {
    return "systolic-materialize-schedule";
  }

  StringRef getDescription() const final {
    return "Materialize start_cycle on already device-assigned "
           "systolic.matmul_tile operations.";
  }

  void runOnOperation() override {
    ModuleOp module = getOperation();
    Builder builder(module.getContext());

    // Physical resources and symbol -> accelerator-id mapping.
    SmallVector<SystolicArrayResource> fleet;
    llvm::StringMap<int64_t> acceleratorIds;
    llvm::StringMap<int64_t> acceleratorSizes;

    int64_t nextAcceleratorId = 0;
    bool invalidDevice = false;

    module.walk([&](DeviceOp device) {
      const int64_t rows = device.getRows();
      const int64_t cols = device.getCols();

      // The scheduler abstraction is currently square.
      if (rows != cols) {
        device.emitError(
            "systolic-materialize-schedule currently requires square devices");
        invalidDevice = true;
        return;
      }

      const int64_t id = nextAcceleratorId++;

      SystolicArrayResource resource;
      resource.arraySize = rows;
      resource.acceleratorId = id;

      fleet.push_back(resource);
      acceleratorIds[device.getSymName()] = id;
      acceleratorSizes[device.getSymName()] = rows;
    });

    if (invalidDevice) {
      signalPassFailure();
      return;
    }

    if (fleet.empty()) {
      module.emitError(
          "systolic-materialize-schedule: no square systolic.device found");
      signalPassFailure();
      return;
    }

    SmallVector<MatmulTileOp> tiles;
    SmallVector<SystolicExecutionTask> tasks;
    SmallVector<int64_t> computeCycles;
    bool hadError = false;

    module.walk([&](MatmulTileOp tile) {
      tiles.push_back(tile);
    });

    for (MatmulTileOp tile : tiles) {
      FlatSymbolRefAttr deviceAttr = tile.getDeviceAttr();
      IntegerAttr cyclesAttr = tile.getEstCyclesAttr();

      if (!deviceAttr || !cyclesAttr) {
        tile.emitError(
            "systolic-materialize-schedule requires device and est_cycles");
        hadError = true;
        return;
      }

      if (tile.getM() != tile.getN()) {
        tile.emitError(
            "systolic-materialize-schedule currently requires square tiles");
        hadError = true;
        return;
      }

      auto it =
          acceleratorIds.find(deviceAttr.getValue());

      if (it == acceleratorIds.end()) {
        tile.emitError(
            "assigned device does not resolve to a physical accelerator");
        hadError = true;
        return;
      }

      SystolicExecutionTask task;
      auto sizeIt =
          acceleratorSizes.find(deviceAttr.getValue());

      if (sizeIt == acceleratorSizes.end()) {
        tile.emitError(
            "assigned device has no physical accelerator geometry");
        hadError = true;
        return;
      }

      task.row = 0;
      task.column = 0;
      task.size = tile.getM();
      task.acceleratorSize = sizeIt->second;
      task.acceleratorId = it->second;

      tasks.push_back(task);
      computeCycles.push_back(cyclesAttr.getInt());
    }

    if (hadError) {
      signalPassFailure();
      return;
    }

    auto schedule =
        scheduleAssignedSystolicTasks(
            tasks, computeCycles, fleet);

    if (failed(schedule)) {
      module.emitError(
          "systolic-materialize-schedule: scheduling failed");
      signalPassFailure();
      return;
    }

    if (schedule->size() != tiles.size()) {
      module.emitError(
          "systolic-materialize-schedule: scheduler result size mismatch");
      signalPassFailure();
      return;
    }

    for (size_t i = 0; i < tiles.size(); ++i) {
      tiles[i].setStartCycleAttr(
          builder.getI64IntegerAttr(
              (*schedule)[i].startCycle));
    }
  }
};

} // namespace

std::unique_ptr<Pass>
mlir::systolic::createSystolicMaterializeSchedulePass() {
  return std::make_unique<SystolicMaterializeSchedulePass>();
}

void mlir::systolic::registerSystolicMaterializeSchedulePass() {
  PassRegistration<SystolicMaterializeSchedulePass>();
}
