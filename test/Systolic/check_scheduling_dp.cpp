#include "Systolic/SystolicScheduling.h"
#include "Systolic/SystolicSchedulingDP.h"

#include "llvm/ADT/SmallVector.h"
#include "mlir/Support/LogicalResult.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>

using namespace mlir;
using namespace mlir::systolic;

namespace {

int64_t getMakeSpan(
    llvm::ArrayRef<ScheduledSystolicTile> schedule) {

  int64_t makeSpan = 0;

  for (const auto &entry : schedule)
    makeSpan =
        std::max(
            makeSpan,
            entry.endCycle);

  return makeSpan;
}

SystolicExecutionTask
makeTask(int64_t size) {

  SystolicExecutionTask task;
  task.row = 0;
  task.column = 0;
  task.size = size;
  task.acceleratorSize = size;
  task.acceleratorId = -1;

  return task;
}

} // namespace

int main() {
  constexpr int64_t K = 128;

  // ----------------------------------------------------------
  // Case 1:
  //
  // Cross-check the count-based DP against the existing exact
  // assignment-aware DFS.
  //
  // Each 4x4 task individually prefers the physical 8x8 array,
  // but the global optimum uses both resources in parallel.
  // ----------------------------------------------------------

  llvm::SmallVector<SystolicExecutionTask> smallTasks = {
      makeTask(4),
      makeTask(4),
  };

  llvm::SmallVector<SystolicArrayResource> smallFleet = {
      {4, 0},
      {8, 1},
  };

  llvm::SmallVector<SystolicGeometryCostParams>
      smallCostParams = {
          {4, {128, 100}},
          {8, {128, 0}},
      };

  auto exact =
      minimizeSystolicMakeSpanAssignmentAware(
          smallTasks,
          K,
          smallFleet,
          smallCostParams);

  assert(succeeded(exact));

  const int64_t exactMakeSpan =
      getMakeSpan(*exact);

  auto dp =
      minimizeSystolicMakeSpanDP(
          smallTasks,
          K,
          smallFleet,
          smallCostParams);

  assert(succeeded(dp));

  std::cout
      << "small exact makespan="
      << exactMakeSpan
      << "\n";

  std::cout
      << "small DP makespan="
      << *dp
      << "\n";

  assert(exactMakeSpan == 234);
  assert(*dp == exactMakeSpan);

  std::cout
      << "Scheduling DP matches exact DFS.\n";

  // ----------------------------------------------------------
  // Case 2:
  //
  // Sixteen identical logical 4x4 tasks.
  //
  // The old task-identity DFS has a large permutation search
  // space.  The count-based DP represents them using one
  // remaining-task count.
  //
  // This case intentionally runs only the DP.
  // ----------------------------------------------------------

  llvm::SmallVector<SystolicExecutionTask> manyTasks;

  for (int i = 0; i < 16; ++i)
    manyTasks.push_back(
        makeTask(4));

  llvm::SmallVector<SystolicArrayResource> largeFleet = {
      {4, 0},
      {8, 1},
      {16, 2},
  };

  llvm::SmallVector<SystolicGeometryCostParams>
      largeCostParams = {
          {4, {128, 0}},
          {8, {128, 40}},
          {16, {128, 120}},
      };

  auto manyDP =
      minimizeSystolicMakeSpanDP(
          manyTasks,
          K,
          largeFleet,
          largeCostParams);

  assert(succeeded(manyDP));

  std::cout
      << "16 identical 4x4 tasks DP makespan="
      << *manyDP
      << "\n";

  std::cout
      << "Count-based scheduling DP test passed.\n";

  return 0;
}
