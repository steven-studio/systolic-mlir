#include "Systolic/SystolicDecompositionDP.h"

#include "llvm/ADT/SmallVector.h"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>

using namespace mlir;
using namespace mlir::systolic;

namespace {

using Clock = std::chrono::steady_clock;

template <typename Fn>
static double benchmark(Fn &&fn) {
  const auto start = Clock::now();

  volatile size_t resultSize = fn().size();
  (void)resultSize;

  const auto end = Clock::now();

  return std::chrono::duration<double, std::milli>(
             end - start)
      .count();
}

static void benchmarkCase(
    int64_t rows,
    int64_t columns,
    const llvm::SmallVector<int64_t> &geometries) {

  std::cout
      << "\n"
      << rows << "x" << columns
      << "\n";

  const double oldMs =
      benchmark([&]() {
        return enumerateSystolicGeometryMultisetsDP(
            rows,
            columns,
            geometries);
      });

  const double frontierMs =
      benchmark([&]() {
        return enumerateSystolicGeometryMultisetsFrontierDP(
            rows,
            columns,
            geometries);
      });

  const double areaFirstMs =
      benchmark([&]() {
        return enumerateSystolicGeometryMultisetsAreaFirstDP(
            rows,
            columns,
            geometries);
      });

  std::cout
      << "  old       : "
      << oldMs
      << " ms\n";

  std::cout
      << "  frontier  : "
      << frontierMs
      << " ms\n";

  std::cout
      << "  area-first: "
      << areaFirstMs
      << " ms\n";

  if (frontierMs > 0.0) {
    std::cout
        << "  old/frontier speedup: "
        << oldMs / frontierMs
        << "x\n";
  }
}

} // namespace

int main() {

  const llvm::SmallVector<int64_t> geometries{
      4,
      8,
      16,
  };

  benchmarkCase(
      32,
      32,
      geometries);

  benchmarkCase(
      40,
      40,
      geometries);

  benchmarkCase(
      48,
      48,
      geometries);

  benchmarkCase(
      64,
      64,
      geometries);

  return 0;
}
