#include "Systolic/SystolicTiling.h"

#include <cstdint>
#include <iostream>
#include <vector>

using namespace mlir::systolic;

static SystolicTile makeTile(int64_t size) {
  SystolicTile tile;
  tile.row = 0;
  tile.column = 0;
  tile.size = size;
  tile.acceleratorSize = size;
  tile.acceleratorId = -1;
  return tile;
}

static int64_t runCase(
    const char *name,
    const std::vector<int64_t> &sizes,
    int64_t expectedMakespan) {

  SystolicFleetState fleet;

  fleet.resources = {
      {16, 0},
      {8, 0},
      {8, 1},
      {4, 0},
      {4, 1},
      {4, 2},
  };

  llvm::SmallVector<SystolicTile> tiles;
  llvm::SmallVector<int64_t> computeCycles;

  for (int64_t size : sizes) {
    tiles.push_back(makeTile(size));
    computeCycles.push_back(2 * size - 2);
  }

  auto scheduled =
      scheduleSystolicTiles(
          tiles,
          computeCycles,
          fleet);

  if (failed(scheduled)) {
    std::cerr
        << "[FAIL] " << name
        << ": scheduler returned failure\n";
    return 1;
  }

  int64_t makespan = 0;

  for (const auto &entry : *scheduled) {
    // Scheduler must never promote a tile to a larger geometry.
    if (entry.tile.acceleratorSize != entry.tile.size) {
      std::cerr
          << "[FAIL] " << name
          << ": tile " << entry.tile.size
          << " assigned to accelerator geometry "
          << entry.tile.acceleratorSize << "\n";
      return 1;
    }

    makespan =
        std::max(
            makespan,
            entry.endCycle);
  }

  std::cout
      << "[CASE] " << name
      << "  makespan=" << makespan
      << "  expected=" << expectedMakespan
      << "\n";

  if (makespan != expectedMakespan) {
    std::cerr
        << "[FAIL] " << name
        << ": wrong makespan\n";
    return 1;
  }

  std::cout
      << "[PASS] " << name << "\n";

  return 0;
}

int main() {

  // ----------------------------------------------------------
  // C16 = 30
  // C8  = 14
  // C4  = 6
  // ----------------------------------------------------------

  // Case 1:
  //
  //   4 x 16x16
  //
  // One physical 16x16 accelerator:
  //
  //   ceil(4 / 1) * 30 = 120
  //
  if (runCase(
          "4x16",
          {16, 16, 16, 16},
          120))
    return 1;

  // Case 2:
  //
  //   1 x 16x16
  //   5 x 8x8
  //
  //   max(30, ceil(5/2)*14)
  // = max(30, 42)
  // = 42
  //
  if (runCase(
          "1x16 + 5x8",
          {16, 8, 8, 8, 8, 8},
          42))
    return 1;

  // Case 3:
  //
  //   1 x 16x16
  //   13 x 4x4
  //
  //   max(30, ceil(13/3)*6)
  // = max(30, 30)
  // = 30
  //
  if (runCase(
          "1x16 + 13x4",
          {16,
           4, 4, 4, 4, 4, 4, 4,
           4, 4, 4, 4, 4, 4},
          30))
    return 1;

  // Case 4:
  //
  //   1 x 16x16
  //   5 x 8x8
  //   13 x 4x4
  //
  //   max(30, 42, 30) = 42
  //
  if (runCase(
          "1x16 + 5x8 + 13x4",
          {16,
           8, 8, 8, 8, 8,
           4, 4, 4, 4, 4, 4, 4,
           4, 4, 4, 4, 4, 4},
          42))
    return 1;

  std::cout
      << "\nALL SCHEDULER CASES PASSED\n";

  return 0;
}
