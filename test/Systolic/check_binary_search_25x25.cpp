#include "Systolic/SystolicOptimizer.h"

#include <cassert>
#include <iostream>

using namespace mlir::systolic;

int main() {
  // ----------------------------------------------------------
  // Padding validation:
  //
  // Original matrix:
  //
  //   25 x 25
  //
  // Hardware:
  //
  //   one 8x8 accelerator
  //
  // The spatial tiler may internally pad to:
  //
  //   32 x 32
  //
  // BUT a tile containing no original matrix element must NOT
  // become a real execution task.
  //
  // Therefore the optimizer must not execute the entire
  // 32x32 padded region as if it were real computation.
  // ----------------------------------------------------------

  const int64_t rows = 25;
  const int64_t columns = 25;

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
        << "FAIL: 25x25 padding case returned failure\n";
    return 1;
  }

  std::cout
      << "makespan="
      << result->makespan
      << "\n";

  // ----------------------------------------------------------
  // Every returned task must intersect the ORIGINAL 25x25
  // domain.
  //
  // A tile entirely inside:
  //
  //   row >= 25
  // or
  //   column >= 25
  //
  // would be padding-only and is illegal.
  // ----------------------------------------------------------

  for (const SystolicExecutionTask &task :
       result->decomposition) {

    if (task.row >= rows ||
        task.column >= columns) {

      std::cerr
          << "FAIL: padding-only tile found at ("
          << task.row
          << ", "
          << task.column
          << ") size="
          << task.size
          << "\n";

      return 1;
    }

    if (task.row + task.size <= 0 ||
        task.column + task.size <= 0) {

      std::cerr
          << "FAIL: invalid tile outside original domain\n";

      return 1;
    }
  }

  std::cout
      << "PASS: no padding-only tile\n";

  // ----------------------------------------------------------
  // Case 2:
  //
  // 25x25 input
  // two physical 8x8 accelerators
  // H = 0
  //
  // Internally the spatial domain may be padded to 32x32.
  //
  // Valid execution tiles are:
  //
  //   (0,  0)   (0,  8)   (0, 16)   (0, 24)
  //   (8,  0)   (8,  8)   (8, 16)   (8, 24)
  //   (16, 0)   (16, 8)   (16,16)   (16,24)
  //   (24, 0)   (24, 8)   (24,16)   (24,24)
  //
  // The (24,24) tile contains the valid original element
  // (24,24), so it is NOT padding-only.
  //
  // The remaining padding inside boundary tiles is allowed.
  //
  // Therefore:
  //
  //   16 execution tasks
  //
  // Each 8x8 task costs:
  //
  //   C = 2*8 - 2 = 14
  //
  // With two identical accelerators:
  //
  //   ceil(15 / 2) = 8 tasks
  //
  // Expected makespan:
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
        << "FAIL: 25x25 / two 8x8 binary-search optimizer "
           "returned failure\n";
    return 1;
  }

  std::cout
      << "makespan(two accelerators)="
      << result2->makespan
      << "\n";

  std::cout
      << "decomposition tiles="
      << result2->decomposition.size()
      << "\n";

  // All 16 8x8 tiles intersect the original 25x25 domain.
  // The boundary tiles may contain padding, but they still contain
  // at least one valid original matrix element.
  assert(result2->decomposition.size() == 16);

  // Verify that every generated task actually intersects
  // the original matrix.
  for (const SystolicExecutionTask &task :
       result2->decomposition) {

    assert(task.row >= 0);
    assert(task.column >= 0);
    assert(task.size > 0);

    assert(task.row < rows);
    assert(task.column < columns);

    std::cout
        << "tile=("
        << task.row
        << ","
        << task.column
        << ") size="
        << task.size
        << "\n";
  }

  assert(result2->makespan == 112);

  std::cout
      << "PASS: 25x25 / two 8x8 / 16 boundary-aware tasks "
         "-> makespan 112\n";

  return 0;
}
