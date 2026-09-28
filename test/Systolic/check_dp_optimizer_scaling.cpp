#include "Systolic/SystolicOptimizer.h"

#include "llvm/ADT/SmallVector.h"
#include "mlir/Support/LogicalResult.h"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <iostream>

using namespace mlir;
using namespace mlir::systolic;

int main() {
  constexpr int64_t K = 128;

  // ----------------------------------------------------------
  // End-to-end scalability sweep.
  //
  // For every square output size:
  //
  //   decomposition DP
  //       -> unique geometry multisets
  //       -> count-based scheduling DP
  //       -> global minimum makespan
  //
  // No spatial-enumeration / task-identity DFS oracle is run
  // here.  The purpose of this test is to characterize how far
  // the new end-to-end DP path scales.
  // ----------------------------------------------------------

  llvm::SmallVector<SystolicArrayResource> fleet = {
      {4, 0},
      {8, 1},
      {16, 2},
  };

  llvm::SmallVector<SystolicGeometryCostParams>
      costParams = {
          {4, {128, 0}},
          {8, {128, 40}},
          {16, {128, 120}},
      };

  constexpr int64_t sizes[] = {
      16,
      24,
      32,
      40,
      48,
      64,
  };

  std::cout
      << "End-to-end DP scalability sweep\n"
      << "K=" << K << "\n"
      << "fleet={4x4,8x8,16x16}\n\n";

  for (const int64_t size : sizes) {
    const int64_t rows = size;
    const int64_t columns = size;

    const auto start =
        std::chrono::steady_clock::now();

    auto result =
        optimizeSystolicRectangleWithSchedulingDP(
            rows,
            columns,
            K,
            fleet,
            costParams);

    const auto stop =
        std::chrono::steady_clock::now();

    assert(succeeded(result));

    const auto elapsedUs =
        std::chrono::duration_cast<
            std::chrono::microseconds>(
                stop - start)
            .count();

    int64_t count4 = 0;
    int64_t count8 = 0;
    int64_t count16 = 0;

    for (const SystolicExecutionTask &task :
         result->decomposition) {
      switch (task.size) {
      case 4:
        ++count4;
        break;

      case 8:
        ++count8;
        break;

      case 16:
        ++count16;
        break;

      default:
        assert(false &&
               "unexpected logical geometry");
      }
    }

    const int64_t coveredArea =
        count4 * 4 * 4 +
        count8 * 8 * 8 +
        count16 * 16 * 16;

    assert(coveredArea == rows * columns);
    assert(result->makespan > 0);

    std::cout
        << rows
        << "x"
        << columns
        << ": "
        << "4x4="
        << count4
        << " "
        << "8x8="
        << count8
        << " "
        << "16x16="
        << count16
        << " "
        << "tasks="
        << result->decomposition.size()
        << " "
        << "area="
        << coveredArea
        << " "
        << "makespan="
        << result->makespan
        << " "
        << "time_us="
        << elapsedUs
        << "\n";
  }

  // ----------------------------------------------------------
  // Padding regression.
  //
  // The minimum supported geometry is 4x4, so a 17x19 logical
  // problem is zero-padded to a 20x20 decomposition domain.
  // The selected decomposition must therefore cover exactly
  // 400 elements.
  // ----------------------------------------------------------

  {
    constexpr int64_t rows = 17;
    constexpr int64_t columns = 19;
    constexpr int64_t paddedRows = 20;
    constexpr int64_t paddedColumns = 20;

    auto result =
        optimizeSystolicRectangleWithSchedulingDP(
            rows,
            columns,
            K,
            fleet,
            costParams);

    assert(succeeded(result));

    int64_t coveredArea = 0;

    for (const SystolicExecutionTask &task :
         result->decomposition) {
      assert(
          (task.size == 4 ||
           task.size == 8 ||
           task.size == 16) &&
          "unexpected logical geometry");

      coveredArea +=
          task.size * task.size;
    }

    assert(
        coveredArea ==
        paddedRows * paddedColumns);

    assert(result->makespan > 0);

    std::cout
        << "\nPadding regression\n"
        << rows << "x" << columns
        << " -> "
        << paddedRows << "x" << paddedColumns
        << ": tasks="
        << result->decomposition.size()
        << " area="
        << coveredArea
        << " makespan="
        << result->makespan
        << "\n";
  }

  std::cout
      << "\nEnd-to-end DP scalability sweep passed.\n";

  return 0;
}
