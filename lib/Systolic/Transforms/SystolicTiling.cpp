#include "Systolic/SystolicTiling.h"

#include "llvm/ADT/STLExtras.h"

#include <algorithm>
#include <functional>
#include <limits>
#include <map>

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

FailureOr<SystolicDecompositionResult>
greedySystolicDecompose(
    int64_t rows,
    int64_t columns,
    llvm::ArrayRef<SystolicArrayResource> fleet) {

  if (rows <= 0 ||
      columns <= 0 ||
      fleet.empty())
    return failure();

  // ----------------------------------------------------------
  // Collect distinct hardware geometries.
  //
  // The supported hierarchy is strictly dyadic:
  //
  //   g_i = 2 * g_{i+1}
  //
  // Therefore:
  //
  //   32 -> 16 -> 8 -> 4
  //
  // is legal, while:
  //
  //   128 -> 32 -> 4
  //
  // is intentionally outside the current problem scope.
  // ----------------------------------------------------------

  llvm::SmallVector<int64_t> geometries;

  for (const SystolicArrayResource &resource : fleet) {
    if (resource.arraySize <= 0)
      return failure();

    if (std::find(
            geometries.begin(),
            geometries.end(),
            resource.arraySize) ==
        geometries.end()) {
      geometries.push_back(resource.arraySize);
    }
  }

  if (geometries.empty())
    return failure();

  std::sort(
      geometries.begin(),
      geometries.end(),
      std::greater<int64_t>());

  // ----------------------------------------------------------
  // Enforce the dyadic hierarchy.
  //
  // Every adjacent pair must satisfy:
  //
  //   larger = 2 * smaller
  //
  // This removes arbitrary/sparse power-of-two hierarchies from
  // the current decomposition problem.
  // ----------------------------------------------------------

  for (size_t i = 0;
       i + 1 < geometries.size();
       ++i) {

    if (geometries[i] !=
        2 * geometries[i + 1])
      return failure();
  }

  const int64_t smallestGeometry =
      geometries.back();

  // ----------------------------------------------------------
  // Padding.
  //
  // The original matrix does not have to be divisible by the
  // smallest hardware geometry.
  //
  // Extend it to the smallest geometry boundary:
  //
  //   paddedRows =
  //       ceil(rows / g_min) * g_min
  //
  //   paddedColumns =
  //       ceil(columns / g_min) * g_min
  //
  // Tiles are allowed to cover this padding region.
  // ----------------------------------------------------------

  if (rows >
          std::numeric_limits<int64_t>::max() -
              (smallestGeometry - 1) ||
      columns >
          std::numeric_limits<int64_t>::max() -
              (smallestGeometry - 1))
    return failure();

  const int64_t paddedRows =
      ((rows + smallestGeometry - 1) /
       smallestGeometry) *
      smallestGeometry;

  const int64_t paddedColumns =
      ((columns + smallestGeometry - 1) /
       smallestGeometry) *
      smallestGeometry;

  if (paddedRows <= 0 ||
      paddedColumns <= 0)
    return failure();

  // ----------------------------------------------------------
  // Greedy decomposition.
  //
  // Work from the first uncovered padded element in row-major
  // order.
  //
  // At that position, choose the LARGEST geometry that fits.
  //
  // Because the geometry hierarchy is dyadic, choosing a smaller
  // geometry is equivalent to recursively splitting the larger
  // geometry:
  //
  //   g_i x g_i
  //       ->
  //   4 * (g_{i+1} x g_{i+1})
  //
  // No enumeration, backtracking, DP, skyline, or scheduling
  // occurs here.
  // ----------------------------------------------------------

  if (paddedRows >
          std::numeric_limits<int64_t>::max() /
              paddedColumns)
    return failure();

  const int64_t paddedElements =
      paddedRows * paddedColumns;

  std::vector<uint8_t> covered(
      static_cast<size_t>(paddedElements),
      0);

  llvm::SmallVector<SystolicTile> tiles;

  const auto indexOf =
      [paddedColumns](int64_t row, int64_t column) {
        return static_cast<size_t>(
            row * paddedColumns + column);
      };

  int64_t coveredElements = 0;

  while (coveredElements < paddedElements) {

    // --------------------------------------------------------
    // First uncovered padded element.
    // --------------------------------------------------------

    int64_t startRow = -1;
    int64_t startColumn = -1;

    for (int64_t row = 0;
         row < paddedRows && startRow < 0;
         ++row) {

      for (int64_t column = 0;
           column < paddedColumns;
           ++column) {

        if (covered[indexOf(row, column)] == 0) {
          startRow = row;
          startColumn = column;
          break;
        }
      }
    }

    if (startRow < 0 ||
        startColumn < 0)
      return failure();

    // --------------------------------------------------------
    // Largest geometry first.
    // --------------------------------------------------------

    int64_t selectedGeometry = -1;

    for (int64_t geometry : geometries) {

      if (geometry >
              paddedRows - startRow ||
          geometry >
              paddedColumns - startColumn)
        continue;

      bool legal = true;

      for (int64_t r = startRow;
           r < startRow + geometry && legal;
           ++r) {

        for (int64_t c = startColumn;
             c < startColumn + geometry;
             ++c) {

          if (covered[indexOf(r, c)] != 0) {
            legal = false;
            break;
          }
        }
      }

      if (legal) {
        selectedGeometry = geometry;
        break;
      }
    }

    if (selectedGeometry <= 0)
      return failure();

    // --------------------------------------------------------
    // Commit one greedy tile.
    // --------------------------------------------------------

    SystolicTile tile;
    tile.row = startRow;
    tile.column = startColumn;
    tile.size = selectedGeometry;
    tile.acceleratorSize = selectedGeometry;
    tile.acceleratorId = -1;

    tiles.push_back(tile);

    // --------------------------------------------------------
    // Mark the selected square as covered.
    // --------------------------------------------------------

    for (int64_t r = startRow;
         r < startRow + selectedGeometry;
         ++r) {

      for (int64_t c = startColumn;
           c < startColumn + selectedGeometry;
           ++c) {

        const size_t index =
            indexOf(r, c);

        if (covered[index] != 0)
          return failure();

        covered[index] = 1;
        ++coveredElements;
      }
    }
  }

  // ----------------------------------------------------------
  // Exact coverage of the padded domain.
  // ----------------------------------------------------------

  if (coveredElements != paddedElements)
    return failure();

  for (uint8_t value : covered) {
    if (value != 1)
      return failure();
  }

  return SystolicDecompositionResult{
      std::move(tiles)};
}





FailureOr<llvm::SmallVector<ScheduledSystolicTile>>
scheduleSystolicTiles(
    llvm::ArrayRef<SystolicTile> tiles,
    llvm::ArrayRef<int64_t> computeCycles,
    const SystolicFleetState &fleet) {

  if (tiles.size() != computeCycles.size())
    return failure();

  if (tiles.empty())
    return llvm::SmallVector<ScheduledSystolicTile>{};

  if (fleet.resources.empty())
    return failure();

  // ----------------------------------------------------------
  // Sort tasks by decreasing compute time.
  //
  // This is the scheduling policy:
  //
  //   largest task first
  //
  // Spatial decomposition has already been completed before
  // entering this function.
  // ----------------------------------------------------------

  llvm::SmallVector<size_t> order;

  for (size_t i = 0; i < tiles.size(); ++i) {
    if (tiles[i].size <= 0 ||
        computeCycles[i] < 0)
      return failure();

    order.push_back(i);
  }

  std::sort(
      order.begin(),
      order.end(),
      [&](size_t lhs, size_t rhs) {
        if (computeCycles[lhs] !=
            computeCycles[rhs])
          return computeCycles[lhs] >
                 computeCycles[rhs];

        if (tiles[lhs].size !=
            tiles[rhs].size)
          return tiles[lhs].size >
                 tiles[rhs].size;

        if (tiles[lhs].row !=
            tiles[rhs].row)
          return tiles[lhs].row <
                 tiles[rhs].row;

        return tiles[lhs].column <
               tiles[rhs].column;
      });

  // ----------------------------------------------------------
  // Track the next available cycle of every physical
  // accelerator.
  // ----------------------------------------------------------

  llvm::SmallVector<int64_t> load(
      fleet.resources.size(),
      0);

  llvm::SmallVector<ScheduledSystolicTile> result;
  result.reserve(tiles.size());

  // ----------------------------------------------------------
  // Greedy list scheduling.
  //
  // For each task:
  //
  //   1. find compatible accelerators
  //   2. choose the least-loaded one
  //   3. schedule the task there
  //
  // An accelerator may be reused after its previous task
  // finishes.
  // ----------------------------------------------------------

  for (size_t taskIndex : order) {

    const SystolicTile &tile =
        tiles[taskIndex];

    const int64_t cycles =
        computeCycles[taskIndex];

    size_t selectedResource =
        fleet.resources.size();

    for (size_t resourceIndex = 0;
         resourceIndex < fleet.resources.size();
         ++resourceIndex) {

      const SystolicArrayResource &resource =
          fleet.resources[resourceIndex];

      // A tile may execute only on the same physical geometry.
      //
      // Decomposition has already decided the tile geometry.
      // Scheduling must not remap a tile onto a larger array.
      if (resource.arraySize != tile.size)
        continue;

      if (selectedResource ==
          fleet.resources.size()) {
        selectedResource = resourceIndex;
        continue;
      }

      // Prefer the accelerator that becomes available first.
      if (load[resourceIndex] <
          load[selectedResource]) {
        selectedResource = resourceIndex;
        continue;
      }

      // Deterministic tie-break.
      if (load[resourceIndex] ==
              load[selectedResource] &&
          resource.acceleratorId <
              fleet.resources[selectedResource]
                  .acceleratorId) {
        selectedResource = resourceIndex;
      }
    }

    if (selectedResource ==
        fleet.resources.size())
      return failure();

    const int64_t startCycle =
        load[selectedResource];

    if (cycles >
        std::numeric_limits<int64_t>::max() -
            startCycle)
      return failure();

    const int64_t endCycle =
        startCycle + cycles;

    ScheduledSystolicTile scheduled;

    scheduled.tile = tile;
    scheduled.tile.acceleratorSize =
        fleet.resources[selectedResource]
            .arraySize;
    scheduled.tile.acceleratorId =
        fleet.resources[selectedResource]
            .acceleratorId;

    scheduled.startCycle =
        startCycle;

    scheduled.endCycle =
        endCycle;

    result.push_back(
        scheduled);

    load[selectedResource] =
        endCycle;
  }

  return result;
}

} // namespace systolic
} // namespace mlir
