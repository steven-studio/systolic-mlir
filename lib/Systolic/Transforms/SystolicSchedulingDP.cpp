#include "Systolic/SystolicSchedulingDP.h"

#include "llvm/ADT/SmallVector.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <utility>
#include <vector>

namespace mlir {
namespace systolic {

namespace {

struct DPState {
  std::vector<int64_t> remaining;
  std::vector<int64_t> loads;

  bool operator<(const DPState &other) const {
    if (remaining != other.remaining)
      return remaining < other.remaining;
    return loads < other.loads;
  }
};

const SystolicGeometryCostParams *
findCostParamsForGeometry(
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
minimizeSystolicMakeSpanDP(
    llvm::ArrayRef<SystolicExecutionTask> tasks,
    int64_t K,
    llvm::ArrayRef<SystolicArrayResource> fleet,
    llvm::ArrayRef<SystolicGeometryCostParams> costParams) {

  if (K <= 0)
    return failure();

  if (fleet.empty())
    return failure();

  // ----------------------------------------------------------
  // Collect the distinct logical task geometries.
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

  // Empty task set has zero makespan.
  if (logicalGeometries.empty())
    return int64_t{0};

  // ----------------------------------------------------------
  // Convert individual tasks into counts by geometry.
  // ----------------------------------------------------------

  std::vector<int64_t> initialRemaining(
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

    ++initialRemaining[geometryIndex];
  }

  // ----------------------------------------------------------
  // Precompute assignment-aware execution costs:
  //
  //   executionCost[logical geometry][physical resource]
  //
  // A negative value means that assignment is illegal.
  // ----------------------------------------------------------

  std::vector<std::vector<int64_t>>
      executionCost(
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
          findCostParamsForGeometry(
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
  // Memoized exact DP.
  //
  // State:
  //
  //   remaining[g] = number of unscheduled logical tasks
  //                  of geometry g
  //
  //   loads[r]     = accumulated execution time on physical
  //                  resource r
  //
  // Logical tasks of the same geometry have no identity.
  // ----------------------------------------------------------

  std::map<DPState, int64_t> memo;

  std::function<int64_t(
      const std::vector<int64_t> &,
      const std::vector<int64_t> &)>
      solve;

  solve =
      [&](const std::vector<int64_t> &remaining,
          const std::vector<int64_t> &loads)
          -> int64_t {

    DPState state{
        remaining,
        loads,
    };

    auto memoIt =
        memo.find(state);

    if (memoIt != memo.end())
      return memoIt->second;

    bool done = true;

    for (int64_t count : remaining) {
      if (count != 0) {
        done = false;
        break;
      }
    }

    if (done) {
      const int64_t makespan =
          *std::max_element(
              loads.begin(),
              loads.end());

      memo.emplace(
          std::move(state),
          makespan);

      return makespan;
    }

    int64_t best =
        std::numeric_limits<int64_t>::max();

    for (size_t geometryIndex = 0;
         geometryIndex < remaining.size();
         ++geometryIndex) {

      if (remaining[geometryIndex] == 0)
        continue;

      for (size_t resourceIndex = 0;
           resourceIndex < fleet.size();
           ++resourceIndex) {

        const int64_t cycles =
            executionCost[
                geometryIndex][resourceIndex];

        if (cycles < 0)
          continue;

        if (loads[resourceIndex] >
            std::numeric_limits<int64_t>::max() -
                cycles)
          continue;

        std::vector<int64_t> nextRemaining =
            remaining;

        std::vector<int64_t> nextLoads =
            loads;

        --nextRemaining[geometryIndex];

        nextLoads[resourceIndex] += cycles;

        const int64_t candidate =
            solve(
                nextRemaining,
                nextLoads);

        best =
            std::min(
                best,
                candidate);
      }
    }

    memo.emplace(
        std::move(state),
        best);

    return best;
  };

  std::vector<int64_t> initialLoads(
      fleet.size(),
      0);

  const int64_t result =
      solve(
          initialRemaining,
          initialLoads);

  if (result ==
      std::numeric_limits<int64_t>::max())
    return failure();

  return result;
}

} // namespace systolic
} // namespace mlir
