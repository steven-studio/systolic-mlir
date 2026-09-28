#include "Systolic/SystolicComputeCost.h"

#include <cassert>
#include <cstdint>
#include <iostream>

using namespace mlir;
using namespace mlir::systolic;

int main() {
  // ----------------------------------------------------------
  // Shared calibrated parameters used only for this unit test.
  //
  //   K    = 128
  //   kMax = 32
  //   H    = 95
  //
  // Therefore:
  //
  //   I = ceil(128 / 32) = 4
  // ----------------------------------------------------------

  constexpr int64_t K = 128;

  SystolicComputeCostParams params;
  params.kMax = 32;
  params.implementationOverhead = 95;

  // ----------------------------------------------------------
  // 8x8 execution task.
  //
  //   T = 128 + 4 * (8 + 8 - 2 + 95)
  //     = 128 + 4 * 109
  //     = 564
  // ----------------------------------------------------------

  SystolicExecutionTask task8;
  task8.row = 0;
  task8.column = 0;
  task8.size = 8;
  task8.acceleratorSize = 8;
  task8.acceleratorId = -1;

  FailureOr<int64_t> cost8 =
      estimateSystolicTileComputeCycles(
          task8,
          K,
          params);

  assert(succeeded(cost8));
  assert(*cost8 == 564);

  // ----------------------------------------------------------
  // 4x4 execution task.
  //
  //   T = 128 + 4 * (4 + 4 - 2 + 95)
  //     = 128 + 4 * 101
  //     = 532
  // ----------------------------------------------------------

  SystolicExecutionTask task4;
  task4.row = 0;
  task4.column = 0;
  task4.size = 4;
  task4.acceleratorSize = 4;
  task4.acceleratorId = -1;

  FailureOr<int64_t> cost4 =
      estimateSystolicTileComputeCycles(
          task4,
          K,
          params);

  assert(succeeded(cost4));
  assert(*cost4 == 532);

  std::cout
      << "8x8 task compute cycles="
      << *cost8
      << "\n";

  std::cout
      << "4x4 task compute cycles="
      << *cost4
      << "\n";

  std::cout
      << "All execution-task compute-cost tests passed.\n";

  return 0;
}
