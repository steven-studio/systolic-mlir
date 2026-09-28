#include "Systolic/SystolicScheduling.h"

#include <algorithm>
#include <functional>
#include <limits>

namespace mlir {
namespace systolic {

namespace {

const SystolicComputeCostParams *
findCostParamsForGeometry(
    int64_t arraySize,
    llvm::ArrayRef<SystolicGeometryCostParams> costParams) {

  for (const SystolicGeometryCostParams &entry : costParams) {
    if (entry.arraySize == arraySize)
      return &entry.params;
  }

  return nullptr;
}

} // namespace

FailureOr<llvm::SmallVector<ScheduledSystolicTile>>
minimizeSystolicMakeSpanAssignmentAware(
    llvm::ArrayRef<SystolicExecutionTask> tasks,
    int64_t K,
    llvm::ArrayRef<SystolicArrayResource> fleet,
    llvm::ArrayRef<SystolicGeometryCostParams> costParams) {

  if (tasks.empty() ||
      fleet.empty() ||
      costParams.empty() ||
      K <= 0)
    return failure();

  // Validate geometry calibration entries.
  for (const SystolicGeometryCostParams &entry : costParams) {
    if (entry.arraySize <= 0 ||
        entry.params.kMax <= 0 ||
        entry.params.implementationOverhead < 0)
      return failure();
  }

  // Every physical resource must have a valid geometry.
  for (const SystolicArrayResource &resource : fleet) {
    if (resource.arraySize <= 0)
      return failure();

    if (!findCostParamsForGeometry(
            resource.arraySize,
            costParams))
      return failure();
  }

  // Every logical task must be valid and executable by at least
  // one physical resource.
  for (const SystolicExecutionTask &task : tasks) {
    if (task.size <= 0)
      return failure();

    bool hasCompatibleResource = false;

    for (const SystolicArrayResource &resource : fleet) {
      if (resource.arraySize >= task.size) {
        hasCompatibleResource = true;
        break;
      }
    }

    if (!hasCompatibleResource)
      return failure();
  }

  const size_t numTasks = tasks.size();
  const size_t numResources = fleet.size();

  llvm::SmallVector<int64_t> load(
      numResources,
      0);

  llvm::SmallVector<bool> scheduled(
      numTasks,
      false);

  llvm::SmallVector<ScheduledSystolicTile> currentSchedule;
  currentSchedule.reserve(numTasks);

  llvm::SmallVector<ScheduledSystolicTile> bestSchedule;

  int64_t bestMakeSpan =
      std::numeric_limits<int64_t>::max();

  std::function<void(size_t, int64_t)> search =
      [&](size_t scheduledCount,
          int64_t currentMakeSpan) {

        if (scheduledCount == numTasks) {
          if (currentMakeSpan < bestMakeSpan) {
            bestMakeSpan = currentMakeSpan;
            bestSchedule = currentSchedule;
          }
          return;
        }

        // Branch-and-bound:
        // assigning more work can never reduce the current
        // maximum resource completion time.
        if (currentMakeSpan >= bestMakeSpan)
          return;

        // Enumerate which unscheduled logical task is placed next.
        for (size_t taskIndex = 0;
             taskIndex < numTasks;
             ++taskIndex) {

          if (scheduled[taskIndex])
            continue;

          const SystolicExecutionTask &task =
              tasks[taskIndex];

          // Enumerate every compatible physical accelerator.
          for (size_t resourceIndex = 0;
               resourceIndex < numResources;
               ++resourceIndex) {

            const SystolicArrayResource &resource =
                fleet[resourceIndex];

            // A larger physical array may execute a smaller
            // logical task.
            if (resource.arraySize < task.size)
              continue;

            const SystolicComputeCostParams *params =
                findCostParamsForGeometry(
                    resource.arraySize,
                    costParams);

            if (!params)
              continue;

            FailureOr<int64_t> assignmentCycles =
                estimateSystolicTaskComputeCycles(
                    task,
                    resource,
                    K,
                    *params);

            if (failed(assignmentCycles))
              continue;

            const int64_t startCycle =
                load[resourceIndex];

            const int64_t endCycle =
                startCycle +
                *assignmentCycles;

            const int64_t nextMakeSpan =
                std::max(
                    currentMakeSpan,
                    endCycle);

            if (nextMakeSpan >= bestMakeSpan)
              continue;

            // Preserve the logical spatial tile, but record the
            // physical accelerator selected by this assignment.
            SystolicTile scheduledTile;
            scheduledTile.row = task.row;
            scheduledTile.column = task.column;
            scheduledTile.size = task.size;
            scheduledTile.acceleratorSize =
                resource.arraySize;
            scheduledTile.acceleratorId =
                resource.acceleratorId;

            ScheduledSystolicTile scheduledTileInfo{
                scheduledTile,
                startCycle,
                endCycle};

            const int64_t oldLoad =
                load[resourceIndex];

            load[resourceIndex] =
                endCycle;

            scheduled[taskIndex] =
                true;

            currentSchedule.push_back(
                scheduledTileInfo);

            search(
                scheduledCount + 1,
                nextMakeSpan);

            currentSchedule.pop_back();

            scheduled[taskIndex] =
                false;

            load[resourceIndex] =
                oldLoad;
          }
        }
      };

  search(0, 0);

  if (bestSchedule.empty())
    return failure();

  return bestSchedule;
}

} // namespace systolic
} // namespace mlir
