#include "Systolic/SystolicTiling.h"

#include <cassert>
#include <iostream>

using namespace mlir::systolic;

int main() {
  SystolicTile tile;
  tile.row = 0;
  tile.column = 0;
  tile.size = 8;
  tile.acceleratorSize = 8;
  tile.acceleratorId = -1;

  SystolicFleetState fleet =
      createSystolicFleetState(
          {8, 4},
          {1, 3});

  auto decompositions =
      enumerateSystolicDecompositions(
          tile, fleet.resources);

  // We expect:
  //   1 x 8x8
  //   4 x 4x4
  assert(decompositions.size() == 2);

  assert(decompositions[0].size() == 1);
  assert(decompositions[1].size() == 4);

  std::cout
      << "decompositions="
      << decompositions.size()
      << "\n";

  for (const auto &decomposition : decompositions) {
    std::cout
        << "candidate tasks="
        << decomposition.size()
        << "\n";

    for (const auto &task : decomposition) {
      std::cout
          << "  task=("
          << task.row << ","
          << task.column << ") "
          << "size="
          << task.size
          << " acceleratorSize="
          << task.acceleratorSize
          << " acceleratorId="
          << task.acceleratorId
          << "\n";

      // Enumeration must NOT assign a physical accelerator yet.
      assert(task.acceleratorId == -1);
    }
  }

  std::cout
      << "All decomposition tests passed.\n";

  return 0;
}
