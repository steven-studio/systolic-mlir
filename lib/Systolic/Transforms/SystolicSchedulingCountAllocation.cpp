#include "Systolic/SystolicSchedulingDP.h"

#include "llvm/ADT/SmallVector.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <vector>

namespace mlir {
namespace systolic {

namespace {

const SystolicGeometryCostParams *
findCountAllocationCostParamsForGeometry(
    int64_t arraySize,
    llvm::ArrayRef<SystolicGeometryCostParams> costParams) {

  for (const auto &entry : costParams) {
    if (entry.arraySize == arraySize)
      return &entry;
  }

  return nullptr;
}

} // namespace

FailureOr<int64_t>
minimizeSystolicMakeSpanCountAllocation(
    llvm::ArrayRef<SystolicExecutionTask> tasks,
    int64_t K,
    llvm::ArrayRef<SystolicArrayResource> fleet,
    llvm::ArrayRef<SystolicGeometryCostParams> costParams) {

  if (K <= 0 || fleet.empty())
    return failure();

  if (tasks.empty())
    return int64_t{0};

  // ----------------------------------------------------------
  // Collapse logical tasks into geometry counts.
  // ----------------------------------------------------------

  std::vector<int64_t> logicalGeometries;

  for (const auto &task : tasks) {
    if (task.size <= 0)
      return failure();

    logicalGeometries.push_back(task.size);
  }

  std::sort(
      logicalGeometries.begin(),
      logicalGeometries.end());

  logicalGeometries.erase(
      std::unique(
          logicalGeometries.begin(),
          logicalGeometries.end()),
      logicalGeometries.end());

  std::vector<int64_t> counts(
      logicalGeometries.size(),
      0);

  for (const auto &task : tasks) {
    auto it =
        std::lower_bound(
            logicalGeometries.begin(),
            logicalGeometries.end(),
            task.size);

    if (it == logicalGeometries.end() ||
        *it != task.size)
      return failure();

    const size_t geometryIndex =
        static_cast<size_t>(
            it - logicalGeometries.begin());

    ++counts[geometryIndex];
  }

  // ----------------------------------------------------------
  // Precompute assignment-aware cost[g][r].
  //
  // -1 means that logical geometry g cannot execute on
  // physical resource r.
  // ----------------------------------------------------------

  std::vector<std::vector<int64_t>> executionCost(
      logicalGeometries.size(),
      std::vector<int64_t>(
          fleet.size(),
          -1));

  for (size_t geometryIndex = 0;
       geometryIndex < logicalGeometries.size();
       ++geometryIndex) {

    const int64_t logicalSize =
        logicalGeometries[geometryIndex];

    bool hasCompatibleResource = false;

    for (size_t resourceIndex = 0;
         resourceIndex < fleet.size();
         ++resourceIndex) {

      const auto &resource =
          fleet[resourceIndex];

      if (resource.arraySize < logicalSize)
        continue;

      const auto *params =
          findCountAllocationCostParamsForGeometry(
              resource.arraySize,
              costParams);

      if (!params)
        return failure();

      SystolicExecutionTask representativeTask;
      representativeTask.row = 0;
      representativeTask.column = 0;
      representativeTask.size = logicalSize;
      representativeTask.acceleratorSize =
          logicalSize;
      representativeTask.acceleratorId = -1;

      auto cycles =
          estimateSystolicTaskComputeCycles(
              representativeTask,
              resource,
              K,
              params->params);

      if (failed(cycles))
        return failure();

      executionCost[geometryIndex][resourceIndex] =
          *cycles;

      hasCompatibleResource = true;
    }

    if (!hasCompatibleResource)
      return failure();
  }

  // ----------------------------------------------------------
  // Exact count-allocation search.
  //
  // At recursion level g, distribute all count[g] identical
  // logical tasks among compatible physical resources.
  //
  // Unlike the previous DP, we do not explore permutations of
  // individual task assignments.
  // ----------------------------------------------------------

  std::vector<int64_t> loads(
      fleet.size(),
      0);

  int64_t best =
      std::numeric_limits<int64_t>::max();

  std::function<void(size_t)> assignGeometry;

  assignGeometry =
      [&](size_t geometryIndex) {

    if (geometryIndex ==
        logicalGeometries.size()) {

      const int64_t makespan =
          *std::max_element(
              loads.begin(),
              loads.end());

      best =
          std::min(
              best,
              makespan);

      return;
    }

    const int64_t taskCount =
        counts[geometryIndex];

    std::vector<size_t> compatibleResources;

    for (size_t resourceIndex = 0;
         resourceIndex < fleet.size();
         ++resourceIndex) {
      if (executionCost[
              geometryIndex][resourceIndex] >= 0) {
        compatibleResources.push_back(
            resourceIndex);
      }
    }

    if (compatibleResources.empty())
      return;

    // --------------------------------------------------------
    // Enumerate all weak compositions of taskCount across the
    // compatible resources.
    // --------------------------------------------------------

    std::function<void(size_t, int64_t)>
        distribute;

    distribute =
        [&](size_t compatibleIndex,
            int64_t remainingCount) {

      const size_t resourceIndex =
          compatibleResources[
              compatibleIndex];

      const int64_t cycles =
          executionCost[
              geometryIndex][resourceIndex];

      // Last compatible resource receives all remaining tasks.
      if (compatibleIndex + 1 ==
          compatibleResources.size()) {

        if (remainingCount >
            0 &&
            cycles >
                (std::numeric_limits<int64_t>::max() -
                 loads[resourceIndex]) /
                    remainingCount)
          return;

        const int64_t addedLoad =
            remainingCount * cycles;

        loads[resourceIndex] += addedLoad;

        // Branch-and-bound: once a resource load already reaches
        // the best complete makespan, no completion can improve it.
        if (loads[resourceIndex] < best)
          assignGeometry(
              geometryIndex + 1);

        loads[resourceIndex] -= addedLoad;

        return;
      }

      for (int64_t assigned = 0;
           assigned <= remainingCount;
           ++assigned) {

        if (assigned >
            0 &&
            cycles >
                (std::numeric_limits<int64_t>::max() -
                 loads[resourceIndex]) /
                    assigned)
          break;

        const int64_t addedLoad =
            assigned * cycles;

        loads[resourceIndex] += addedLoad;

        if (loads[resourceIndex] < best) {
          distribute(
              compatibleIndex + 1,
              remainingCount - assigned);
        }

        loads[resourceIndex] -= addedLoad;
      }
    };

    distribute(
        0,
        taskCount);
  };

  assignGeometry(0);

  if (best ==
      std::numeric_limits<int64_t>::max())
    return failure();

  return best;
}

} // namespace systolic
} // namespace mlir
