#include "Systolic/SystolicTiling.h"

#include <cassert>
#include <iostream>

using namespace mlir::systolic;

static void expectValid(
    int64_t rows,
    int64_t columns,
    llvm::ArrayRef<SystolicTile> tiles,
    const SystolicFleetState &fleet) {
  assert(succeeded(
      verifySystolicTiling(
          rows, columns, tiles, fleet)));
}

static void expectInvalid(
    int64_t rows,
    int64_t columns,
    llvm::ArrayRef<SystolicTile> tiles,
    const SystolicFleetState &fleet) {
  assert(failed(
      verifySystolicTiling(
          rows, columns, tiles, fleet)));
}

int main() {
  SystolicFleetState fleet =
      createSystolicFleetState(
          {8, 4},
          {1, 3});

  // ----------------------------------------------------------
  // 1. Valid: four 8x8 tiles cover the complete 16x16 input.
  //
  // This MUST be valid even though the fleet has only one
  // physical 8x8 accelerator.
  //
  // Temporal reuse is a scheduling problem, not a tiling
  // legality problem.
  // ----------------------------------------------------------
  llvm::SmallVector<SystolicTile> valid8x8 = {
      {0, 0, 8, 8, 1},
      {0, 8, 8, 8, 1},
      {8, 0, 8, 8, 1},
      {8, 8, 8, 8, 1},
  };

  expectValid(16, 16, valid8x8, fleet);

  // ----------------------------------------------------------
  // 2. Valid: sixteen 4x4 tiles cover the complete input.
  // ----------------------------------------------------------
  llvm::SmallVector<SystolicTile> valid4x4;

  for (int64_t row = 0; row < 16; row += 4) {
    for (int64_t column = 0; column < 16; column += 4) {
      valid4x4.push_back(
          {row, column, 4, 4, 1});
    }
  }

  expectValid(16, 16, valid4x4, fleet);

  // ----------------------------------------------------------
  // 3. Invalid: overlap.
  // ----------------------------------------------------------
  llvm::SmallVector<SystolicTile> overlap = {
      {0, 0, 8, 8, 1},
      {0, 4, 8, 8, 1},
  };

  expectInvalid(16, 16, overlap, fleet);

  // ----------------------------------------------------------
  // 4. Invalid: hole in the input.
  // ----------------------------------------------------------
  llvm::SmallVector<SystolicTile> hole = {
      {0, 0, 8, 8, 1},
      {0, 8, 8, 8, 1},
      {8, 0, 8, 8, 1},
  };

  expectInvalid(16, 16, hole, fleet);

  // ----------------------------------------------------------
  // 5. Invalid: tile goes out of bounds.
  // ----------------------------------------------------------
  llvm::SmallVector<SystolicTile> outOfBounds = {
      {0, 0, 8, 8, 1},
      {0, 8, 8, 8, 1},
      {8, 0, 8, 8, 1},
      {8, 9, 8, 8, 1},
  };

  expectInvalid(16, 16, outOfBounds, fleet);

  // ----------------------------------------------------------
  // 6. Invalid: unsupported accelerator geometry.
  // ----------------------------------------------------------
  llvm::SmallVector<SystolicTile> unsupported = {
      {0, 0, 2, 2, 2},
  };

  expectInvalid(2, 2, unsupported, fleet);

  // ----------------------------------------------------------
  // 7. Invalid: non-square tile.
  // ----------------------------------------------------------
  llvm::SmallVector<SystolicTile> nonSquare = {
      {0, 0, 4, 8, 8},
  };

  expectInvalid(8, 8, nonSquare, fleet);

  std::cout << "All systolic tiling verifier tests passed.\n";

  return 0;
}
