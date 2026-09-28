#include "Systolic/SystolicTiling.h"

#include <iostream>

using namespace mlir::systolic;

int main() {
  // Deliberately use a non-power-of-two parent tile.
  SystolicTile tile{
      0,
      0,
      10,
      0,
      -1,
  };

  llvm::SmallVector<SystolicArrayResource> fleet = {
      {8, 1},
  };

  auto decompositions =
      enumerateSystolicDecompositions(
          tile,
          fleet);

  bool foundPartial = false;

  for (const auto &decomposition : decompositions) {
    for (const auto &task : decomposition) {
      if (task.size != 8)
        continue;

      const int64_t row = task.row;
      const int64_t column = task.column;

      // The 8x8 accelerator must be allowed to extend beyond
      // the 10x10 parent tile, as long as it has non-zero
      // intersection with the parent.
      if (row + 8 > 10 ||
          column + 8 > 10) {
        const int64_t intersectionRowBegin =
            std::max<int64_t>(0, row);

        const int64_t intersectionColumnBegin =
            std::max<int64_t>(0, column);

        const int64_t intersectionRowEnd =
            std::min<int64_t>(10, row + 8);

        const int64_t intersectionColumnEnd =
            std::min<int64_t>(10, column + 8);

        const int64_t intersectionArea =
            (intersectionRowEnd - intersectionRowBegin) *
            (intersectionColumnEnd - intersectionColumnBegin);

        if (intersectionArea > 0) {
          foundPartial = true;

          std::cout
              << "FOUND PARTIAL OVERLAP\n"
              << "  task=("
              << row << "," << column
              << ") size=8\n"
              << "  intersection area="
              << intersectionArea
              << "\n";

          break;
        }
      }
    }

    if (foundPartial)
      break;
  }

  std::cout
      << "total decompositions="
      << decompositions.size()
      << "\n";

  if (!foundPartial) {
    std::cerr
        << "ERROR: no partial boundary-overlap "
        << "decomposition found.\n";
    return 1;
  }

  std::cout
      << "PASS: partial boundary overlap enumeration works.\n";

  return 0;
}
