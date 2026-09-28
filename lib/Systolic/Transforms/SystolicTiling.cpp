#include "Systolic/SystolicTiling.h"

#include "llvm/ADT/STLExtras.h"

namespace mlir {
namespace systolic {

bool isPowerOfTwo(int64_t x) {
  return x > 0 && (x & (x - 1)) == 0;
}

/// Validate the GEMM input shape for the current tiling scope.
///
/// Current scope:
///   - input must be square: M == N
///   - the dimension must be a positive power of two
LogicalResult validateTilingInput(
    int64_t M,
    int64_t N) {
  if (M != N)
    return failure();

  if (!isPowerOfTwo(M))
    return failure();

  return success();
}

LogicalResult validateSystolicFleet(
    llvm::ArrayRef<SystolicArrayResource> fleet) {
  if (fleet.empty())
    return failure();

  for (const SystolicArrayResource &resource : fleet) {
    // Current scope only supports square power-of-two arrays.
    if (!isPowerOfTwo(resource.arraySize))
      return failure();
  }

  return success();
}

SystolicFleetState createSystolicFleetState(
    llvm::ArrayRef<int64_t> arraySizes,
    llvm::ArrayRef<int64_t> counts) {
  SystolicFleetState state;

  if (arraySizes.size() != counts.size())
    return state;

  for (size_t i = 0; i < arraySizes.size(); ++i) {
    int64_t arraySize = arraySizes[i];
    int64_t count = counts[i];

    if (!isPowerOfTwo(arraySize) || count <= 0)
      continue;

    state.countBySize[arraySize] = count;
    state.nextAcceleratorId[arraySize] = 1;

    for (int64_t id = 1; id <= count; ++id) {
      state.resources.push_back(
          SystolicArrayResource{
              arraySize,
              id});
    }
  }

  return state;
}

llvm::SmallVector<SystolicArrayResource>
sortSystolicFleetDescending(
    llvm::ArrayRef<SystolicArrayResource> fleet) {
  llvm::SmallVector<SystolicArrayResource> sorted(
      fleet.begin(), fleet.end());

  llvm::sort(
      sorted,
      [](const SystolicArrayResource &lhs,
         const SystolicArrayResource &rhs) {
        return lhs.arraySize > rhs.arraySize;
      });

  return sorted;
}

FailureOr<SystolicArrayResource> selectLargestFittingArray(
    SystolicFleetState &state,
    int64_t remainingRows,
    int64_t remainingColumns) {
  int64_t selectedSize = 0;

  for (const SystolicArrayResource &resource : state.resources) {
    if (resource.arraySize > remainingRows ||
        resource.arraySize > remainingColumns)
      continue;

    if (resource.arraySize > selectedSize)
      selectedSize = resource.arraySize;
  }

  if (selectedSize == 0)
    return failure();

  auto countIt = state.countBySize.find(selectedSize);
  auto nextIt = state.nextAcceleratorId.find(selectedSize);

  if (countIt == state.countBySize.end() ||
      nextIt == state.nextAcceleratorId.end() ||
      countIt->second <= 0)
    return failure();

  int64_t acceleratorId = nextIt->second;

  ++nextIt->second;

  if (nextIt->second > countIt->second)
    nextIt->second = 1;

  return SystolicArrayResource{
      selectedSize,
      acceleratorId};
}

FailureOr<SystolicTile> createSystolicTile(
    SystolicFleetState &state,
    int64_t row,
    int64_t column,
    int64_t remainingRows,
    int64_t remainingColumns) {
  FailureOr<SystolicArrayResource> resource =
      selectLargestFittingArray(
          state,
          remainingRows,
          remainingColumns);

  if (failed(resource))
    return failure();

  SystolicTile tile;
  tile.row = row;
  tile.column = column;
  tile.size = resource->arraySize;
  tile.acceleratorSize = resource->arraySize;
  tile.acceleratorId = resource->acceleratorId;

  return tile;
}

FailureOr<llvm::SmallVector<SystolicTile>>
tileSystolicInput(
    int64_t M,
    int64_t N,
    SystolicFleetState &state) {
  if (failed(validateTilingInput(M, N)))
    return failure();

  llvm::SmallVector<SystolicTile> tiles;

  int64_t row = 0;

  while (row < M) {
    int64_t column = 0;
    int64_t rowHeight = 0;

    while (column < N) {
      int64_t remainingRows = M - row;
      int64_t remainingColumns = N - column;

      FailureOr<SystolicTile> tile =
          createSystolicTile(
              state,
              row,
              column,
              remainingRows,
              remainingColumns);

      if (failed(tile))
        return failure();

      rowHeight = std::max(rowHeight, tile->size);
      column += tile->size;

      tiles.push_back(*tile);
    }

    if (rowHeight <= 0)
      return failure();

    row += rowHeight;
  }

  return tiles;
}

LogicalResult verifySystolicTiling(
    int64_t inputRows,
    int64_t inputColumns,
    llvm::ArrayRef<SystolicTile> tiles,
    const SystolicFleetState &fleet) {
  // The current tiling scope only supports positive square inputs.
  if (inputRows <= 0 || inputColumns <= 0 ||
      inputRows != inputColumns)
    return failure();

  // A tiling must contain at least one tile.
  if (tiles.empty())
    return failure();

  // Build the set of supported accelerator geometries.
  llvm::DenseMap<int64_t, bool> supportedSizes;

  for (const SystolicArrayResource &resource : fleet.resources)
    supportedSizes[resource.arraySize] = true;

  if (supportedSizes.empty())
    return failure();

  // Coverage map:
  //   0 = uncovered
  //   1 = covered exactly once
  //
  // This deliberately verifies spatial legality only.
  // It does NOT model temporal accelerator availability.
  llvm::SmallVector<int8_t> coverage(
      static_cast<size_t>(inputRows * inputColumns),
      0);

  for (const SystolicTile &tile : tiles) {
    // Tile must be a positive square.
    if (tile.size <= 0)
      return failure();

    if (tile.size != tile.acceleratorSize)
      return failure();

    // Tile geometry must correspond to a physical accelerator
    // geometry present in the fleet.
    if (!supportedSizes.count(tile.acceleratorSize))
      return failure();

    // Tile must lie completely inside the input.
    if (tile.row < 0 ||
        tile.column < 0 ||
        tile.row + tile.size > inputRows ||
        tile.column + tile.size > inputColumns)
      return failure();

    // Mark every covered input element.
    for (int64_t r = tile.row;
         r < tile.row + tile.size;
         ++r) {
      for (int64_t c = tile.column;
           c < tile.column + tile.size;
           ++c) {
        size_t index =
            static_cast<size_t>(r * inputColumns + c);

        // Any second visit means the tiling overlaps.
        if (coverage[index] != 0)
          return failure();

        coverage[index] = 1;
      }
    }
  }

  // Every input element must be covered exactly once.
  for (int8_t covered : coverage) {
    if (covered != 1)
      return failure();
  }

  return success();
}

} // namespace systolic
} // namespace mlir
