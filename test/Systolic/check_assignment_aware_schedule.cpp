#include "Systolic/SystolicScheduling.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>

using namespace mlir;
using namespace mlir::systolic;

int main() {
  constexpr int64_t K = 128;

  // ----------------------------------------------------------
  // Two independent logical 4x4 tasks.
  // ----------------------------------------------------------

  llvm::SmallVector<SystolicExecutionTask> tasks = {
      {0, 0, 4, 4, -1},
      {0, 4, 4, 4, -1},
  };

  // ----------------------------------------------------------
  // Physical heterogeneous fleet:
  //
  //   4x4 accelerator #0
  //   8x8 accelerator #1
  //
  // A 4x4 logical task is legal on either resource.
  // ----------------------------------------------------------

  llvm::SmallVector<SystolicArrayResource> fleet = {
      {4, 0},
      {8, 1},
  };

  // ----------------------------------------------------------
  // Geometry-specific calibrated cost:
  //
  // 4x4 physical:
  //
  //   T4 = 128 + (4 + 4 - 2 + 100)
  //      = 234
  //
  // 8x8 physical:
  //
  //   T8 = 128 + (8 + 8 - 2 + 0)
  //      = 142
  //
  // Individually, each task prefers the 8x8 accelerator.
  //
  // But assigning both tasks to 8x8 serially gives:
  //
  //   makespan = 142 + 142 = 284
  //
  // The global optimum instead runs them in parallel:
  //
  //   one task on 8x8: 142 cycles
  //   one task on 4x4: 234 cycles
  //
  //   makespan = max(142, 234) = 234
  //
  // This verifies global makespan optimization rather than
  // greedy per-task accelerator selection.
  // ----------------------------------------------------------

  llvm::SmallVector<SystolicGeometryCostParams> costParams = {
      {4, {128, 100}},
      {8, {128, 0}},
  };

  auto schedule =
      minimizeSystolicMakeSpanAssignmentAware(
          tasks,
          K,
          fleet,
          costParams);

  assert(succeeded(schedule));
  assert(schedule->size() == 2);

  int64_t makeSpan = 0;
  int count4 = 0;
  int count8 = 0;

  for (const ScheduledSystolicTile &entry : *schedule) {
    makeSpan =
        std::max(
            makeSpan,
            entry.endCycle);

    if (entry.tile.acceleratorSize == 4) {
      ++count4;

      assert(entry.tile.acceleratorId == 0);
      assert(entry.startCycle == 0);
      assert(entry.endCycle == 234);
    } else if (entry.tile.acceleratorSize == 8) {
      ++count8;

      assert(entry.tile.acceleratorId == 1);
      assert(entry.startCycle == 0);
      assert(entry.endCycle == 142);
    } else {
      assert(false && "unexpected physical geometry");
    }
  }

  assert(count4 == 1);
  assert(count8 == 1);
  assert(makeSpan == 234);

  std::cout
      << "tasks on 4x4="
      << count4
      << "\n";

  std::cout
      << "tasks on 8x8="
      << count8
      << "\n";

  std::cout
      << "minimum makespan="
      << makeSpan
      << "\n";

  std::cout
      << "Assignment-aware exact scheduling test passed.\n";

  return 0;
}
