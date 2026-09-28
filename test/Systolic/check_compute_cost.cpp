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

  // ----------------------------------------------------------
  // Assignment-aware compute cost.
  //
  // The logical work remains 4x4, while the physical resource
  // geometry changes.
  //
  // Cost must therefore depend on the selected physical
  // accelerator:
  //
  //   4x4 work -> 4x4 physical:
  //
  //     T = 128 + 4 * (4 + 4 - 2 + 95)
  //       = 532
  //
  //   4x4 work -> 8x8 physical:
  //
  //     T = 128 + 4 * (8 + 8 - 2 + 95)
  //       = 564
  //
  // A 2x2 physical resource cannot execute 4x4 work.
  // ----------------------------------------------------------

  SystolicArrayResource physical4;
  physical4.arraySize = 4;
  physical4.acceleratorId = 0;

  SystolicArrayResource physical8;
  physical8.arraySize = 8;
  physical8.acceleratorId = 1;

  SystolicArrayResource physical2;
  physical2.arraySize = 2;
  physical2.acceleratorId = 2;

  FailureOr<int64_t> cost4On4 =
      estimateSystolicTaskComputeCycles(
          task4,
          physical4,
          K,
          params);

  FailureOr<int64_t> cost4On8 =
      estimateSystolicTaskComputeCycles(
          task4,
          physical8,
          K,
          params);

  FailureOr<int64_t> cost4On2 =
      estimateSystolicTaskComputeCycles(
          task4,
          physical2,
          K,
          params);

  assert(succeeded(cost4On4));
  assert(succeeded(cost4On8));
  assert(failed(cost4On2));

  assert(*cost4On4 == 532);
  assert(*cost4On8 == 564);

  std::cout
      << "4x4 work on 4x4 physical="
      << *cost4On4
      << "\n";

  std::cout
      << "4x4 work on 8x8 physical="
      << *cost4On8
      << "\n";

  std::cout
      << "4x4 work on 2x2 physical=illegal\n";

  std::cout
      << "All assignment-aware compute-cost tests passed.\n";

  return 0;
}
