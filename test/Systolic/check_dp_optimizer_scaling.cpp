#include "Systolic/SystolicOptimizer.h"

#include "llvm/ADT/SmallVector.h"
#include "mlir/Support/LogicalResult.h"

#include <cassert>
#include <cstdint>
#include <iostream>

using namespace mlir;
using namespace mlir::systolic;

int main() {
  constexpr int64_t K = 128;

  // ----------------------------------------------------------
  // Scalability case:
  //
  // Logical output:
  //
  //   16 x 16
  //
  // Physical heterogeneous fleet:
  //
  //   one 4x4 array
  //   one 8x8 array
  //   one 16x16 array
  //
  // The decomposition DP has only six unique geometry
  // multisets for this case.  Each multiset is evaluated using
  // the count-based scheduling DP.
  //
  // Importantly, this test does NOT invoke the old spatial
  // decomposition + task-identity DFS oracle, which previously
  // became prohibitively expensive for this case.
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

  auto result =
      optimizeSystolicRectangleWithSchedulingDP(
          16,
          16,
          K,
          fleet,
          costParams);

  assert(succeeded(result));

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

  std::cout
      << "16x16 end-to-end DP result\n";

  std::cout
      << "selected tasks="
      << result->decomposition.size()
      << "\n";

  std::cout
      << "logical 4x4 tasks="
      << count4
      << "\n";

  std::cout
      << "logical 8x8 tasks="
      << count8
      << "\n";

  std::cout
      << "logical 16x16 tasks="
      << count16
      << "\n";

  std::cout
      << "minimum makespan="
      << result->makespan
      << "\n";

  // The selected logical decomposition must exactly cover the
  // 16x16 output area.
  const int64_t coveredArea =
      count4 * 4 * 4 +
      count8 * 8 * 8 +
      count16 * 16 * 16;

  assert(coveredArea == 16 * 16);

  assert(result->makespan > 0);

  std::cout
      << "16x16 end-to-end DP scalability test passed.\n";

  return 0;
}
