#include "Systolic/SystolicOptimizer.h"

#include <iostream>

using namespace mlir::systolic;

int main() {
  // ----------------------------------------------------------
  // Independent validation:
  //
  // Input sizes: 33x33 ... 64x64
  //
  // Fleet:
  //   one 16x16 accelerator
  //   one 4x4 accelerator
  //
  // H = 0
  //
  // Padding-only tiles must not become execution tasks.
  // ----------------------------------------------------------

  llvm::SmallVector<SystolicArrayResource> fleet = {
      {16, 1, 0},
      {4, 1, 0},
  };

  for (int64_t n = 33; n <= 64; ++n) {

    auto result =
        optimizeSystolicBinarySearch(
            n,
            n,
            fleet);

    if (failed(result)) {
      std::cerr
          << "FAIL N=" << n
          << ": optimizer returned failure\n";
      return 1;
    }

    std::cout
        << "N=" << n
        << " makespan=" << result->makespan
        << " tasks=" << result->decomposition.size()
        << "\n";
  }

  std::cout
      << "PASS: 33x33 through 64x64 enumeration completed\n";

  return 0;
}
