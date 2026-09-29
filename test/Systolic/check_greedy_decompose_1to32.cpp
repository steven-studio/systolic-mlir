#include "Systolic/SystolicTiling.h"

#include <iostream>
#include <map>

using namespace mlir::systolic;

static void printResult(
    int n,
    const SystolicDecompositionResult &result) {

  std::map<int64_t, int64_t> counts;

  std::cout
      << "\n========================================\n"
      << "Matrix: "
      << n << "x" << n << "\n"
      << "tiles: "
      << result.tiles.size()
      << "\n";

  for (const auto &tile : result.tiles) {
    ++counts[tile.size];

    std::cout
        << "  tile"
        << " row=" << tile.row
        << " col=" << tile.column
        << " size=" << tile.size
        << "x" << tile.size
        << "\n";
  }

  std::cout << "summary:";

  for (const auto &[size, count] : counts) {
    std::cout
        << " "
        << count
        << "x"
        << size
        << "x"
        << size;
  }

  std::cout << "\n";
}

int main() {

  // ----------------------------------------------------------
  // Physical hardware hierarchy:
  //
  //   16 -> 8 -> 4
  //
  // with:
  //
  //   16 = 2*8
  //    8 = 2*4
  // ----------------------------------------------------------

  llvm::SmallVector<SystolicArrayResource> fleet = {
      {16, 0},
      {8, 0},
      {4, 0},
  };

  // ----------------------------------------------------------
  // 1x1 through 32x32.
  // ----------------------------------------------------------

  for (int n = 1; n <= 32; ++n) {

    auto result =
        greedySystolicDecompose(
            n,
            n,
            fleet);

    if (failed(result)) {
      std::cout
          << "\n========================================\n"
          << "Matrix: "
          << n << "x" << n
          << "\nRESULT: FAILURE\n";

      continue;
    }

    printResult(
        n,
        *result);
  }

  return 0;
}
