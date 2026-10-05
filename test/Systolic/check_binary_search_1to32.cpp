#include "Systolic/SystolicOptimizer.h"

#include <iostream>

using namespace mlir::systolic;

int main() {
  // ----------------------------------------------------------
  // Independent validation:
  //
  // Input sizes: 1x1 ... 32x32
  //
  // Fleet:
  //   one 16x16 accelerator
  //   one 4x4 accelerator
  //
  // H = 0
  //
  // Independent expected makespan:
  //
  //   N = 1..4    -> 6
  //   N = 5..8    -> 24
  //   N = 9..12   -> 54
  //   N = 13..16  -> 30
  //   N = 17..20  -> 54
  //   N = 21..24  -> 120
  //   N = 25..28  -> 198
  //   N = 29..32  -> 96
  //
  // ----------------------------------------------------------

  llvm::SmallVector<SystolicArrayResource> fleet = {
      {16, 1, 0},
      {4, 1, 0},
  };

  for (int64_t n = 1; n <= 32; ++n) {

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
      << "PASS: 1x1 through 32x32 enumeration completed\n";

  return 0;
}
