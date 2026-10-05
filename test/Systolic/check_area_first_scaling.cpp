#include "Systolic/SystolicDecompositionDP.h"

#include "llvm/ADT/SmallVector.h"

#include <chrono>
#include <cstdint>
#include <iostream>

using namespace mlir;
using namespace mlir::systolic;

template <typename Fn>
static int64_t measureMicroseconds(Fn &&fn) {
  const auto start =
      std::chrono::steady_clock::now();

  auto result = fn();

  const auto end =
      std::chrono::steady_clock::now();

  // Keep the result observable.
  std::cout
      << " candidates=" << result.size();

  return std::chrono::duration_cast<
      std::chrono::microseconds>(
      end - start)
      .count();
}

int main() {

  const llvm::SmallVector<int64_t> geometries{
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
      << "Frontier DP vs AreaFirstDP scaling\n\n";

  for (int64_t size : sizes) {

    std::cout
        << size << "x" << size
        << " frontier:";

    const int64_t frontierUs =
        measureMicroseconds([&]() {
          return
              enumerateSystolicGeometryMultisetsFrontierDP(
                  size,
                  size,
                  geometries);
        });

    std::cout
        << " time_us=" << frontierUs
        << "\n";

    std::cout
        << size << "x" << size
        << " area-first:";

    const int64_t areaFirstUs =
        measureMicroseconds([&]() {
          return
              enumerateSystolicGeometryMultisetsAreaFirstDP(
                  size,
                  size,
                  geometries);
        });

    std::cout
        << " time_us=" << areaFirstUs
        << "\n\n";
  }

  return 0;
}
