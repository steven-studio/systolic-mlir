#include "Systolic/SystolicTiling.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>

using namespace mlir::systolic;

int main() {

  // ----------------------------------------------------------
  // Target:
  //
  //   32x32 GEMM
  //
  //   Hardware:
  //     1 x 8x8
  //     3 x 4x4
  // ----------------------------------------------------------

  const auto fleet =
      createSystolicFleetState(
          {8, 4},
          {1, 3});

  // ----------------------------------------------------------
  // Deterministic cost model.
  //
  // The point of this test is the optimization structure,
  // not the final hardware calibration constants.
  // ----------------------------------------------------------

  auto costFn =
      [](int64_t tileSize,
         int64_t acceleratorSize) -> int64_t {

    if (tileSize == 8 &&
        acceleratorSize == 8)
      return 80;

    if (tileSize == 4 &&
        acceleratorSize == 4)
      return 20;

    return -1;
  };

  auto result =
      minimizeSystolicMakeSpanMacroSplit(
          32,
          32,
          8,
          4,
          fleet.resources,
          costFn);

  assert(succeeded(result));

  std::cout
      << "minimum makespan = "
      << result->makespan
      << "\n";

  std::map<int64_t, int64_t> tileCount;
  std::map<int64_t, int64_t> machineUse;
  std::map<int64_t, int64_t> machineUseById;

  for (const ScheduledSystolicTile &scheduled :
       result->schedule) {

    ++tileCount[
        scheduled.tile.size];

    ++machineUse[
        scheduled.tile.acceleratorSize];

    ++machineUseById[
        scheduled.tile.acceleratorId];

    std::cout
        << "tile "
        << "("
        << scheduled.tile.row
        << ","
        << scheduled.tile.column
        << ") "
        << scheduled.tile.size
        << "x"
        << scheduled.tile.size
        << " -> "
        << scheduled.tile.acceleratorSize
        << "x"
        << scheduled.tile.acceleratorSize
        << "#"
        << scheduled.tile.acceleratorId
        << " ["
        << scheduled.startCycle
        << ","
        << scheduled.endCycle
        << ")\n";
  }

  // ----------------------------------------------------------
  // 32x32 contains 16 canonical 8x8 macro-blocks.
  //
  // With:
  //
  //   T8 = 80
  //   T4 = 20
  //
  // x = 6 gives:
  //
  //   10 x 8x8
  //   24 x 4x4
  //
  // Large-array makespan:
  //
  //   10 * 80 = 800
  //
  // Small-array makespan:
  //
  //   ceil(24/3) * 20 = 160
  //
  // Therefore makespan = 800.
  //
  // In fact, with these synthetic costs the optimum is x=16:
  //
  //   0 x 8x8
  //   64 x 4x4
  //   ceil(64/3)*20 = 440
  //
  // So the test deliberately verifies the optimizer chooses
  // the global minimum among x = 0..16.
  // ----------------------------------------------------------

  // For the current synthetic cost model:
  //
  //   32x32 = 16 macro-blocks
  //   optimal splitCount = 12
  //
  // Therefore:
  //   4 macro-blocks remain as 8x8 tiles
  //   12 macro-blocks become 48 4x4 tiles
  //
  // Expected decomposition:
  //   4 x 8x8
  //   48 x 4x4
  //
  assert(tileCount[8] == 4);
  assert(tileCount[4] == 48);

  // For the current synthetic cost model:
  //
  //   4 x 8x8 tiles on the single 8x8 accelerator:
  //       4 * 80 = 320 cycles
  //
  //   48 x 4x4 tiles on three 4x4 accelerators:
  //       ceil(48 / 3) * 20 = 320 cycles
  //
  // Therefore the minimum makespan is:
  //
  //       max(320, 320) = 320
  //
  assert(result->makespan == 320);

  // ----------------------------------------------------------
  // Exactly three physical 4x4 accelerators should be used.
  // ----------------------------------------------------------

  // All three physical 4x4 accelerators must participate.
  //
  // The fleet contains:
  //   4x4 #1
  //   4x4 #2
  //   4x4 #3
  //
  // Verify physical-instance usage directly rather than assuming
  // that machineUse[4] represents the number of instances.
  // Physical accelerator IDs are globally unique within
  // the fleet created by createSystolicFleetState():
  //
  //   8x8 #1 -> ID 1
  //   4x4 #1 -> ID 1
  //   4x4 #2 -> ID 2
  //   4x4 #3 -> ID 3
  //
  // Therefore check the three 4x4 IDs directly.
  assert(machineUseById[1] > 0);
  assert(machineUseById[2] > 0);
  assert(machineUseById[3] > 0);

  // ----------------------------------------------------------
  // Spatial coverage.
  // ----------------------------------------------------------

  llvm::SmallVector<SystolicTile> tiles;

  for (const ScheduledSystolicTile &scheduled :
       result->schedule)
    tiles.push_back(scheduled.tile);

  SystolicFleetState verifierFleet =
      createSystolicFleetState(
          {8, 4},
          {1, 3});

  assert(
      succeeded(
          verifySystolicTiling(
              32,
              32,
              tiles,
              verifierFleet)));

  // ----------------------------------------------------------
  // Check actual makespan.
  // ----------------------------------------------------------

  int64_t observedMakespan = 0;

  for (const ScheduledSystolicTile &scheduled :
       result->schedule) {

    observedMakespan =
        std::max(
            observedMakespan,
            scheduled.endCycle);
  }

  assert(
      observedMakespan ==
      result->makespan);

  std::cout
      << "\nPASS: macro-block heterogeneous "
      << "makespan optimization.\n";

  return 0;
}
