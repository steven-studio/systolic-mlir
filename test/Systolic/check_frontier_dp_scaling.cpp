#include "Systolic/SystolicDecompositionDP.h"

#include "llvm/ADT/SmallVector.h"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <iostream>

using namespace mlir;
using namespace mlir::systolic;

int main() {
  llvm::SmallVector<int64_t> geometries = {
      4,
      8,
      16,
  };

  constexpr int64_t sizes[] = {
      16,
      20,
      24,
      28,
      32,
      36,
      40,
      44,
      48,
      52,
      56,
      60,
      64,
  };

  std::cout
      << "Frontier DP scaling\n\n";

  for (int64_t size : sizes) {
    auto start =
        std::chrono::steady_clock::now();

    auto result =
        enumerateSystolicGeometryMultisetsFrontierDP(
            size,
            size,
            geometries);

    auto stop =
        std::chrono::steady_clock::now();

    assert(!result.empty());

    const auto elapsedUs =
        std::chrono::duration_cast<
            std::chrono::microseconds>(
                stop - start)
            .count();

    std::cout
        << size << "x" << size
        << ": candidates="
        << result.size()
        << " time_us="
        << elapsedUs
        << "\n";
  }

  return 0;
}
