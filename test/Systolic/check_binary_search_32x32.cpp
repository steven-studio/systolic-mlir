#include "Systolic/SystolicOptimizer.h"

#include <cassert>
#include <iostream>

using namespace mlir::systolic;

int main() {
  // ----------------------------------------------------------
  // Case 1:
  //
  // 32x32 input
  // one physical 8x8 accelerator
  // H = 0
  //
  // 32x32 / 8x8 = 16 tiles
  //
  // C(8) = 2*8 - 2 = 14
  //
  // One accelerator:
  //
  //   T* = 16 * 14 = 224
  // ----------------------------------------------------------

  const int64_t rows = 32;
  const int64_t columns = 32;

  llvm::SmallVector<SystolicArrayResource> fleet = {
      {8, 1, 0},
  };

  auto result =
      optimizeSystolicBinarySearch(
          rows,
          columns,
          fleet);

  if (failed(result)) {
    std::cerr
        << "FAIL: binary-search optimizer returned failure\n";
    return 1;
  }

  std::cout
      << "makespan="
      << result->makespan
      << "\n";

  assert(result->makespan == 224);

  std::cout
      << "PASS: 32x32 / one 8x8 / H=0 -> 224\n";

  // ----------------------------------------------------------
  // Case 2:
  //
  // Same 32x32 input, but now two physical 8x8 accelerators.
  //
  // 16 logical tiles / 2 accelerators = 8 tiles per accelerator.
  //
  // C(8) = 2*8 - 2 = 14
  //
  // Expected:
  //
  //   T* = 8 * 14 = 112
  // ----------------------------------------------------------

  llvm::SmallVector<SystolicArrayResource> twoAccelerators = {
      {8, 1, 0},
      {8, 2, 0},
  };

  auto result2 =
      optimizeSystolicBinarySearch(
          rows,
          columns,
          twoAccelerators);

  if (failed(result2)) {
    std::cerr
        << "FAIL: two-accelerator binary-search optimizer "
           "returned failure\n";
    return 1;
  }

  std::cout
      << "makespan(two accelerators)="
      << result2->makespan
      << "\n";

  assert(result2->makespan == 112);

  std::cout
      << "PASS: 32x32 / two 8x8 / H=0 -> 112\n";

  // ----------------------------------------------------------
  // Case 3:
  //
  // Hardware-calibrated fixed overhead:
  //
  //   H = 10
  //
  // For an 8x8 accelerator:
  //
  //   C(8) = 2*8 - 2 + 10
  //        = 24
  //
  // One accelerator:
  //
  //   T* = 16 * 24 = 384
  //
  // Two accelerators:
  //
  //   T* = 8 * 24 = 192
  // ----------------------------------------------------------

  llvm::SmallVector<SystolicArrayResource> oneAcceleratorH10 = {
      {8, 1, 10},
  };

  auto result3 =
      optimizeSystolicBinarySearch(
          rows,
          columns,
          oneAcceleratorH10);

  if (failed(result3)) {
    std::cerr
        << "FAIL: H=10 one-accelerator case "
           "returned failure\n";
    return 1;
  }

  std::cout
      << "makespan(one accelerator, H=10)="
      << result3->makespan
      << "\n";

  assert(result3->makespan == 384);

  std::cout
      << "PASS: 32x32 / one 8x8 / H=10 -> 384\n";

  llvm::SmallVector<SystolicArrayResource> twoAcceleratorsH10 = {
      {8, 1, 10},
      {8, 2, 10},
  };

  auto result4 =
      optimizeSystolicBinarySearch(
          rows,
          columns,
          twoAcceleratorsH10);

  if (failed(result4)) {
    std::cerr
        << "FAIL: H=10 two-accelerator case "
           "returned failure\n";
    return 1;
  }

  std::cout
      << "makespan(two accelerators, H=10)="
      << result4->makespan
      << "\n";

  assert(result4->makespan == 192);

  std::cout
      << "PASS: 32x32 / two 8x8 / H=10 -> 192\n";

  // ----------------------------------------------------------
  // Case 4:
  //
  // Mixed geometry:
  //
  //   32x32 input
  //   one physical 8x8 accelerator
  //   one physical 4x4 accelerator
  //   H = 0
  //
  // The optimizer must be able to construct a solution using
  // BOTH geometries in the same decomposition.
  //
  // This test is specifically intended to reject an optimizer
  // that only considers homogeneous decompositions.
  // ----------------------------------------------------------

  llvm::SmallVector<SystolicArrayResource> mixedFleet = {
      {8, 1, 0},
      {4, 1, 0},
  };

  auto result5 =
      optimizeSystolicBinarySearch(
          rows,
          columns,
          mixedFleet);

  if (failed(result5)) {
    std::cerr
        << "FAIL: mixed-geometry binary-search optimizer "
           "returned failure\n";
    return 1;
  }

  bool used8x8 = false;
  bool used4x4 = false;

  int64_t coveredArea = 0;

  for (const SystolicExecutionTask &task :
       result5->decomposition) {

    assert(task.row >= 0);
    assert(task.column >= 0);
    assert(task.size > 0);

    if (task.size == 8)
      used8x8 = true;

    if (task.size == 4)
      used4x4 = true;

    assert(task.size == 8 || task.size == 4);

    coveredArea += task.size * task.size;

    std::cout
        << "mixed tile=("
        << task.row
        << ","
        << task.column
        << ") size="
        << task.size
        << "\n";
  }

  // Every generated tile must be one of the supported
  // mixed geometries.
  assert(used8x8);
  assert(used4x4);

  // The spatial decomposition must cover exactly 32x32.
  //
  // NOTE:
  // This area check assumes the decomposition is non-overlapping.
  // The optimizer's internal feasibility checks are responsible
  // for enforcing the actual spatial legality.
  assert(coveredArea == rows * columns);

  std::cout
      << "mixed makespan="
      << result5->makespan
      << "\n";

  std::cout
      << "PASS: 32x32 / mixed 8x8 + 4x4 geometry\n";

  return 0;
}
