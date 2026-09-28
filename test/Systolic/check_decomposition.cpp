#include "Systolic/SystolicTiling.h"

#include <cassert>
#include <cstdint>
#include <iostream>

using namespace mlir::systolic;

int main() {
  // ----------------------------------------------------------
  // Parent spatial tile:
  //
  //   16 x 16
  //
  // Available accelerator geometries:
  //
  //   8 x 8
  //   4 x 4
  //
  // This is large enough to admit both homogeneous and mixed
  // exact-cover decompositions.
  // ----------------------------------------------------------

  SystolicTile tile;
  tile.row = 0;
  tile.column = 0;
  tile.size = 16;
  tile.acceleratorSize = 16;
  tile.acceleratorId = -1;

  SystolicFleetState fleet =
      createSystolicFleetState(
          {8, 4},
          {1, 3});

  auto decompositions =
      enumerateSystolicDecompositions(
          tile, fleet.resources);

  assert(!decompositions.empty());

  bool foundHomogeneous8 = false;
  bool foundHomogeneous4 = false;
  bool foundMixed = false;

  std::cout
      << "decompositions="
      << decompositions.size()
      << "\n";

  for (size_t decompositionIndex = 0;
       decompositionIndex < decompositions.size();
       ++decompositionIndex) {

    const auto &decomposition =
        decompositions[decompositionIndex];

    bool has8 = false;
    bool has4 = false;

    int64_t count8 = 0;
    int64_t count4 = 0;

    // Independent coverage check for this candidate.
    bool coverage[16][16] = {};

    for (const auto &task : decomposition) {
      assert(task.acceleratorId == -1);

      assert(
          task.acceleratorSize == 8 ||
          task.acceleratorSize == 4);

      assert(task.size == task.acceleratorSize);

      if (task.acceleratorSize == 8) {
        has8 = true;
        ++count8;
      }

      if (task.acceleratorSize == 4) {
        has4 = true;
        ++count4;
      }

      assert(task.row >= 0);
      assert(task.column >= 0);
      assert(task.row + task.size <= 16);
      assert(task.column + task.size <= 16);

      for (int64_t r = task.row;
           r < task.row + task.size;
           ++r) {
        for (int64_t c = task.column;
             c < task.column + task.size;
             ++c) {

          // Exact cover must never overlap.
          assert(!coverage[r][c]);
          coverage[r][c] = true;
        }
      }
    }

    // Exact cover must leave no holes.
    for (int64_t r = 0; r < 16; ++r) {
      for (int64_t c = 0; c < 16; ++c)
        assert(coverage[r][c]);
    }

    if (count8 == 4 && count4 == 0)
      foundHomogeneous8 = true;

    if (count8 == 0 && count4 == 16)
      foundHomogeneous4 = true;

    if (has8 && has4)
      foundMixed = true;

    std::cout
        << "candidate "
        << decompositionIndex
        << ": tasks="
        << decomposition.size()
        << " 8x8="
        << count8
        << " 4x4="
        << count4
        << "\n";
  }

  assert(foundHomogeneous8);
  assert(foundHomogeneous4);
  assert(foundMixed);

  std::cout
      << "Found homogeneous 8x8 decomposition.\n";

  std::cout
      << "Found homogeneous 4x4 decomposition.\n";

  std::cout
      << "Found mixed-geometry decomposition.\n";

  std::cout
      << "All exact-cover decomposition tests passed.\n";

  return 0;
}
