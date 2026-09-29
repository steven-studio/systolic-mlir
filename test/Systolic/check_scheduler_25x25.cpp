#include "Systolic/SystolicTiling.h"

#include <cstdint>
#include <iostream>

using namespace mlir::systolic;

int main() {
  // ----------------------------------------------------------
  // Physical fleet:
  //
  //   1 x 16x16
  //   2 x 8x8
  //   3 x 4x4
  // ----------------------------------------------------------

  SystolicFleetState fleet;

  fleet.resources = {
      {16, 0},
      {8, 0},
      {8, 1},
      {4, 0},
      {4, 1},
      {4, 2},
  };

  // ----------------------------------------------------------
  // Greedy decomposition of 25x25.
  //
  // Expected:
  //
  //   1 x 16x16
  //   5 x 8x8
  //   13 x 4x4
  // ----------------------------------------------------------

  auto decomposition =
      greedySystolicDecompose(
          25,
          25,
          fleet.resources);

  if (failed(decomposition)) {
    std::cerr << "ERROR: decomposition failed\n";
    return 1;
  }

  const auto &tiles =
      decomposition->tiles;

  int64_t count16 = 0;
  int64_t count8 = 0;
  int64_t count4 = 0;

  for (const auto &tile : tiles) {
    if (tile.size == 16)
      ++count16;
    else if (tile.size == 8)
      ++count8;
    else if (tile.size == 4)
      ++count4;
    else {
      std::cerr
          << "ERROR: unexpected tile size "
          << tile.size << "\n";
      return 1;
    }
  }

  std::cout
      << "Decomposition:\n"
      << "  16x16 = " << count16 << "\n"
      << "   8x8  = " << count8 << "\n"
      << "   4x4  = " << count4 << "\n";

  if (count16 != 1 ||
      count8 != 5 ||
      count4 != 13) {
    std::cerr
        << "ERROR: unexpected decomposition\n";
    return 1;
  }

  // ----------------------------------------------------------
  // Compute cost:
  //
  //   C(g) = 2g - 2
  //
  // 16 -> 30
  //  8 -> 14
  //  4 ->  6
  // ----------------------------------------------------------

  llvm::SmallVector<int64_t> computeCycles;

  for (const auto &tile : tiles) {
    computeCycles.push_back(
        2 * tile.size - 2);
  }

  // ----------------------------------------------------------
  // Schedule.
  // ----------------------------------------------------------

  auto scheduled =
      scheduleSystolicTiles(
          tiles,
          computeCycles,
          fleet);

  if (failed(scheduled)) {
    std::cerr << "ERROR: scheduling failed\n";
    return 1;
  }

  int64_t makespan = 0;

  std::cout << "\nSchedule:\n";

  for (const auto &entry : *scheduled) {
    std::cout
        << "  "
        << entry.tile.size << "x"
        << entry.tile.size
        << " -> accelerator "
        << entry.tile.acceleratorId
        << " ["
        << entry.startCycle
        << ", "
        << entry.endCycle
        << ")\n";

    makespan =
        std::max(
            makespan,
            entry.endCycle);
  }

  std::cout
      << "\nMakespan = "
      << makespan
      << "\n";

  // Expected:
  //
  //   M16 = 30
  //   M8  = ceil(5/2) * 14 = 42
  //   M4  = ceil(13/3) * 6 = 30
  //
  // Therefore:
  //
  //   M = 42

  if (makespan != 42) {
    std::cerr
        << "ERROR: expected makespan 42, got "
        << makespan << "\n";
    return 1;
  }

  std::cout
      << "\nPASS: scheduler makespan = 42\n";

  return 0;
}
