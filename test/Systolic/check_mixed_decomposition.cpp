#include "Systolic/SystolicTiling.h"

#include <iostream>

using namespace mlir::systolic;

int main() {
  SystolicTile tile{
      0,
      0,
      12,
      0,
      -1,
  };

  llvm::SmallVector<SystolicArrayResource> fleet = {
      {8, 1},
      {4, 1},
      {4, 2},
      {4, 3},
  };

  auto decompositions =
      enumerateSystolicDecompositions(
          tile,
          fleet);

  bool foundMixed = false;

  for (const auto &decomposition : decompositions) {
    bool has8 = false;
    bool has4 = false;

    for (const auto &task : decomposition) {
      if (task.size == 8)
        has8 = true;

      if (task.size == 4)
        has4 = true;
    }

    if (has8 && has4) {
      foundMixed = true;

      std::cout
          << "FOUND MIXED DECOMPOSITION\n";

      std::cout
          << "tasks="
          << decomposition.size()
          << "\n";

      for (const auto &task : decomposition) {
        std::cout
            << "  row=" << task.row
            << " column=" << task.column
            << " size=" << task.size
            << "\n";
      }

      break;
    }
  }

  std::cout
      << "total decompositions="
      << decompositions.size()
      << "\n";

  if (!foundMixed) {
    std::cerr
        << "ERROR: no mixed 8x8 + 4x4 decomposition found.\n";
    return 1;
  }

  std::cout
      << "PASS: mixed geometry enumeration works.\n";

  return 0;
}
