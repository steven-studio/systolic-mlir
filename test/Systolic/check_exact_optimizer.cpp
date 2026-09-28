#include "Systolic/SystolicOptimizer.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>

using namespace mlir;
using namespace mlir::systolic;

int main() {
  constexpr int64_t K = 128;

  // ----------------------------------------------------------
  // Parent logical tile: 8x8.
  //
  // With available geometries {8, 4}, decomposition enumeration
  // should consider:
  //
  //   D0: one 8x8 task
  //   D1: four 4x4 tasks
  //
  // The optimizer must evaluate scheduling cost for every
  // decomposition and return the globally minimum makespan.
  // ----------------------------------------------------------

  SystolicTile tile;
  tile.row = 0;
  tile.column = 0;
  tile.size = 8;
  tile.acceleratorSize = 8;
  tile.acceleratorId = -1;

  llvm::SmallVector<SystolicArrayResource> fleet = {
      {4, 0},
      {8, 1},
  };

  // ----------------------------------------------------------
  // Physical 4x4:
  //
  //   T4 = 128 + (4 + 4 - 2 + 0)
  //      = 134
  //
  // Physical 8x8:
  //
  //   T8 = 128 + (8 + 8 - 2 + 100)
  //      = 242
  //
  // The unsplit 8x8 decomposition therefore has makespan 242.
  //
  // The four-4x4 decomposition is also explored by the exact
  // scheduler, including legal execution of 4x4 work on the
  // larger physical 8x8 resource.
  // ----------------------------------------------------------

  llvm::SmallVector<SystolicGeometryCostParams> costParams = {
      {4, {128, 0}},
      {8, {128, 100}},
  };

  auto result =
      optimizeSystolicTileExact(
          tile,
          K,
          fleet,
          costParams);

  assert(succeeded(result));

  std::cout
      << "selected tasks="
      << result->decomposition.size()
      << "\n";

  std::cout
      << "minimum makespan="
      << result->makespan
      << "\n";

  for (const ScheduledSystolicTile &entry :
       result->schedule) {
    std::cout
        << "task "
        << entry.tile.size
        << "x"
        << entry.tile.size
        << " -> physical "
        << entry.tile.acceleratorSize
        << "x"
        << entry.tile.acceleratorSize
        << " #"
        << entry.tile.acceleratorId
        << " ["
        << entry.startCycle
        << ", "
        << entry.endCycle
        << ")\n";
  }

  // The single 8x8 decomposition costs exactly 242 cycles.
  //
  // Any result below 242 would indicate that the decomposed
  // alternative achieved a better globally scheduled makespan.
  //
  // For this calibration, the exact optimum is expected to be
  // the unsplit 8x8 task.
  assert(result->makespan == 242);
  assert(result->decomposition.size() == 1);
  assert(result->schedule.size() == 1);

  const ScheduledSystolicTile &only =
      result->schedule.front();

  assert(only.tile.size == 8);
  assert(only.tile.acceleratorSize == 8);
  assert(only.tile.acceleratorId == 1);
  assert(only.startCycle == 0);
  assert(only.endCycle == 242);

  std::cout
      << "Exact joint decomposition/scheduling test passed.\n";

  // ----------------------------------------------------------
  // Reverse case:
  //
  // Change only the calibrated hardware cost so that keeping the
  // parent tile intact on the physical 8x8 array becomes much
  // more expensive.
  //
  // Physical 4x4:
  //
  //   T4 = 128 + (4 + 4 - 2 + 0)
  //      = 134
  //
  // Physical 8x8:
  //
  //   T8 = 128 + (8 + 8 - 2 + 400)
  //      = 542
  //
  // The unsplit decomposition therefore costs 542 cycles.
  //
  // The exact optimizer should now prefer the decomposition into
  // four logical 4x4 tasks.
  //
  // This regression test verifies that changing calibrated
  // hardware cost can change the selected spatial decomposition.
  // ----------------------------------------------------------

  llvm::SmallVector<SystolicGeometryCostParams>
      reverseCostParams = {
          {4, {128, 0}},
          {8, {128, 400}},
      };

  auto reverseResult =
      optimizeSystolicTileExact(
          tile,
          K,
          fleet,
          reverseCostParams);

  assert(succeeded(reverseResult));

  std::cout
      << "reverse selected tasks="
      << reverseResult->decomposition.size()
      << "\n";

  std::cout
      << "reverse minimum makespan="
      << reverseResult->makespan
      << "\n";

  for (const ScheduledSystolicTile &entry :
       reverseResult->schedule) {
    std::cout
        << "reverse task "
        << entry.tile.size
        << "x"
        << entry.tile.size
        << " -> physical "
        << entry.tile.acceleratorSize
        << "x"
        << entry.tile.acceleratorSize
        << " #"
        << entry.tile.acceleratorId
        << " ["
        << entry.startCycle
        << ", "
        << entry.endCycle
        << ")\n";
  }

  // The decomposition decision must flip from:
  //
  //   one 8x8 task
  //
  // to:
  //
  //   four 4x4 tasks.
  assert(reverseResult->decomposition.size() == 4);
  assert(reverseResult->schedule.size() == 4);

  for (const SystolicExecutionTask &task :
       reverseResult->decomposition) {
    assert(task.size == 4);
  }

  // It must strictly beat the unsplit 8x8 alternative.
  constexpr int64_t unsplit8x8Cost = 542;

  assert(reverseResult->makespan < unsplit8x8Cost);

  std::cout
      << "Calibration changed the optimal decomposition.\n";

  // ----------------------------------------------------------
  // DP geometry-multiset optimizer must agree with the
  // brute-force spatial-decomposition oracle.
  // ----------------------------------------------------------

  auto dpResult =
      optimizeSystolicRectangleDP(
          8,
          8,
          K,
          fleet,
          costParams);

  assert(succeeded(dpResult));

  std::cout
      << "DP minimum makespan="
      << dpResult->makespan
      << "\n";

  assert(dpResult->makespan ==
         result->makespan);

  assert(dpResult->makespan == 242);

  auto reverseDPResult =
      optimizeSystolicRectangleDP(
          8,
          8,
          K,
          fleet,
          reverseCostParams);

  assert(succeeded(reverseDPResult));

  std::cout
      << "reverse DP minimum makespan="
      << reverseDPResult->makespan
      << "\n";

  assert(reverseDPResult->makespan ==
         reverseResult->makespan);

  assert(reverseDPResult->makespan == 536);

  std::cout
      << "DP optimizer matches brute-force oracle.\n";

  std::cout
      << "All exact optimizer tests passed.\n";

  return 0;
}
