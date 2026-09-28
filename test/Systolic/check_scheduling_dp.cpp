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

  auto smallAllocation =
      minimizeSystolicMakeSpanCountAllocation(
          smallTasks,
          K,
          smallFleet,
          smallCostParams);

  assert(succeeded(smallAllocation));

  std::cout
      << "small count-allocation makespan="
      << *smallAllocation
      << "\n";

  assert(exactMakeSpan == 234);
  assert(*dp == exactMakeSpan);
  assert(*smallAllocation == exactMakeSpan);

  std::cout
      << "Scheduling DP and count allocation "
         "match exact DFS.\n";

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

  auto manyAllocation =
      minimizeSystolicMakeSpanCountAllocation(
          manyTasks,
          K,
          largeFleet,
          largeCostParams);

  assert(succeeded(manyAllocation));

  std::cout
      << "16 identical 4x4 tasks DP makespan="
      << *manyDP
      << "\n";

  std::cout
      << "16 identical 4x4 tasks "
         "count-allocation makespan="
      << *manyAllocation
      << "\n";

  assert(*manyDP == 1072);
  assert(*manyAllocation == *manyDP);

  // ----------------------------------------------------------
  // Case 3:
  //
  // Geometry multiset selected by the 16x24 rectangular
  // end-to-end optimizer:
  //
  //   2 x logical 8x8
  //   1 x logical 16x16
  //
  // Expected exact makespan: 364.
  // ----------------------------------------------------------

  llvm::SmallVector<SystolicExecutionTask>
      rectangleTasks = {
          makeTask(8),
          makeTask(8),
          makeTask(16),
      };

  auto rectangleDP =
      minimizeSystolicMakeSpanDP(
          rectangleTasks,
          K,
          largeFleet,
          largeCostParams);

  auto rectangleAllocation =
      minimizeSystolicMakeSpanCountAllocation(
          rectangleTasks,
          K,
          largeFleet,
          largeCostParams);

  assert(succeeded(rectangleDP));
  assert(succeeded(rectangleAllocation));

  std::cout
      << "16x24 selected multiset DP makespan="
      << *rectangleDP
      << "\n";

  std::cout
      << "16x24 selected multiset "
         "count-allocation makespan="
      << *rectangleAllocation
      << "\n";

  assert(*rectangleDP == 364);
  assert(*rectangleAllocation == *rectangleDP);

  // ----------------------------------------------------------
  // Case 4:
  //
  // Geometry multiset selected by the 32x32 end-to-end
  // optimizer:
  //
  //   4 x logical 8x8
  //   3 x logical 16x16
  //
  // Expected exact makespan: 834.
  // ----------------------------------------------------------

  llvm::SmallVector<SystolicExecutionTask>
      square32Tasks;

  for (int i = 0; i < 4; ++i)
    square32Tasks.push_back(
        makeTask(8));

  for (int i = 0; i < 3; ++i)
    square32Tasks.push_back(
        makeTask(16));

  auto square32DP =
      minimizeSystolicMakeSpanDP(
          square32Tasks,
          K,
          largeFleet,
          largeCostParams);

  auto square32Allocation =
      minimizeSystolicMakeSpanCountAllocation(
          square32Tasks,
          K,
          largeFleet,
          largeCostParams);

  assert(succeeded(square32DP));
  assert(succeeded(square32Allocation));

  std::cout
      << "32x32 selected multiset DP makespan="
      << *square32DP
      << "\n";

  std::cout
      << "32x32 selected multiset "
         "count-allocation makespan="
      << *square32Allocation
      << "\n";

  assert(*square32DP == 834);
  assert(*square32Allocation == *square32DP);

  std::cout
      << "Count-allocation scheduler matches "
         "all scheduling correctness oracles.\n";

  return 0;
}
