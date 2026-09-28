#include "Systolic/SystolicTiling.h"

#include "llvm/ADT/STLExtras.h"

#include <algorithm>
#include <functional>
#include <limits>

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

FailureOr<llvm::SmallVector<ScheduledSystolicTile>>
minimizeSystolicMakeSpan(
    llvm::ArrayRef<SystolicExecutionTask> tasks,
    llvm::ArrayRef<int64_t> computeCycles,
    llvm::ArrayRef<SystolicArrayResource> fleet) {

  if (tasks.empty() ||
      tasks.size() != computeCycles.size() ||
      fleet.empty())
    return failure();

  // ----------------------------------------------------------
  // Build the list of physical accelerator instances.
  // ----------------------------------------------------------

  llvm::SmallVector<SystolicArrayResource> compatibleResources;

  for (const SystolicArrayResource &resource : fleet)
    compatibleResources.push_back(resource);

  // ----------------------------------------------------------
  // Validate tasks and compute costs.
  // ----------------------------------------------------------

  for (int64_t i = 0;
       i < static_cast<int64_t>(tasks.size());
       ++i) {
    if (tasks[i].size <= 0 ||
        tasks[i].acceleratorSize <= 0 ||
        computeCycles[i] <= 0)
      return failure();
  }

  // ----------------------------------------------------------
  // Exhaustive search state.
  //
  // Each physical accelerator has a current load.
  //
  // load[k] = cycle at which accelerator k becomes free.
  // ----------------------------------------------------------

  llvm::SmallVector<int64_t> bestLoad(
      compatibleResources.size(), 0);

  int64_t bestMakespan =
      std::numeric_limits<int64_t>::max();

  llvm::SmallVector<ScheduledSystolicTile> bestSchedule(
      tasks.size());

  llvm::SmallVector<ScheduledSystolicTile> currentSchedule(
      tasks.size());

  llvm::SmallVector<bool> scheduled(
      tasks.size(), false);

  llvm::SmallVector<int64_t> load(
      compatibleResources.size(), 0);

  // ----------------------------------------------------------
  // Recursive exhaustive enumeration.
  //
  // At every level:
  //
  //   choose one unscheduled task
  //   choose one compatible physical accelerator
  //   start it when that accelerator becomes free
  //
  // This enumerates both task ordering and accelerator
  // assignment.
  // ----------------------------------------------------------

  std::function<void(int64_t)> search =
      [&](int64_t scheduledCount) {

    if (scheduledCount ==
        static_cast<int64_t>(tasks.size())) {

      int64_t makespan = 0;

      for (int64_t value : load)
        makespan = std::max(makespan, value);

      if (makespan < bestMakespan) {
        bestMakespan = makespan;
        bestSchedule = currentSchedule;
        bestLoad = load;
      }

      return;
    }

    // --------------------------------------------------------
    // Branch-and-bound:
    //
    // If the current partial schedule already reaches the
    // best known makespan, no descendant can improve it.
    // --------------------------------------------------------

    int64_t currentMakespan = 0;

    for (int64_t value : load)
      currentMakespan =
          std::max(currentMakespan, value);

    if (currentMakespan >= bestMakespan)
      return;

    for (int64_t taskIndex = 0;
         taskIndex < static_cast<int64_t>(tasks.size());
         ++taskIndex) {

      if (scheduled[taskIndex])
        continue;

      const SystolicExecutionTask &task =
          tasks[taskIndex];

      for (int64_t resourceIndex = 0;
           resourceIndex <
               static_cast<int64_t>(
                   compatibleResources.size());
           ++resourceIndex) {

        const SystolicArrayResource &resource =
            compatibleResources[resourceIndex];

        // Physical accelerator must be large enough.
        if (resource.arraySize < task.acceleratorSize)
          continue;

        const int64_t startCycle =
            load[resourceIndex];

        const int64_t endCycle =
            startCycle + computeCycles[taskIndex];

        if (endCycle >= bestMakespan)
          continue;

        scheduled[taskIndex] = true;

        const int64_t oldLoad =
            load[resourceIndex];

        load[resourceIndex] = endCycle;

        ScheduledSystolicTile scheduledTile;

        scheduledTile.tile.row =
            task.row;

        scheduledTile.tile.column =
            task.column;

        scheduledTile.tile.size =
            task.size;

        scheduledTile.tile.acceleratorSize =
            resource.arraySize;

        scheduledTile.tile.acceleratorId =
            resource.acceleratorId;

        scheduledTile.startCycle =
            startCycle;

        scheduledTile.endCycle =
            endCycle;

        currentSchedule[taskIndex] =
            scheduledTile;

        search(scheduledCount + 1);

        load[resourceIndex] = oldLoad;
        scheduled[taskIndex] = false;
      }
    }
  };

  search(0);

  if (bestMakespan ==
      std::numeric_limits<int64_t>::max())
    return failure();

  return bestSchedule;
}

llvm::SmallVector<llvm::SmallVector<SystolicExecutionTask>>
enumerateSystolicDecompositions(
    const SystolicTile &tile,
    llvm::ArrayRef<SystolicArrayResource> fleet) {

  llvm::SmallVector<
      llvm::SmallVector<SystolicExecutionTask>>
      decompositions;

  if (tile.size <= 0 || fleet.empty())
    return decompositions;

  // Collect distinct accelerator geometries that can exactly
  // divide this spatial tile.
  llvm::SmallVector<int64_t> geometries;

  for (const SystolicArrayResource &resource : fleet) {
    const int64_t size = resource.arraySize;

    if (size <= 0 || tile.size % size != 0)
      continue;

    if (llvm::find(geometries, size) == geometries.end())
      geometries.push_back(size);
  }

  llvm::sort(
      geometries,
      [](int64_t lhs, int64_t rhs) {
        return lhs > rhs;
      });

  // ----------------------------------------------------------
  // Generate one homogeneous decomposition for each geometry.
  //
  // A tile of size T using accelerator geometry A produces:
  //
  //   (T / A) x (T / A)
  //
  // execution tasks.
  // ----------------------------------------------------------

  for (int64_t acceleratorSize : geometries) {
    const int64_t tilesPerDimension =
        tile.size / acceleratorSize;

    llvm::SmallVector<SystolicExecutionTask> decomposition;

    for (int64_t r = 0;
         r < tilesPerDimension;
         ++r) {
      for (int64_t c = 0;
           c < tilesPerDimension;
           ++c) {

        SystolicExecutionTask task;
        task.row =
            tile.row +
            r * acceleratorSize;
        task.column =
            tile.column +
            c * acceleratorSize;
        task.size = acceleratorSize;
        task.acceleratorSize = acceleratorSize;

        // Assignment belongs to scheduling.
        task.acceleratorId = -1;

        decomposition.push_back(task);
      }
    }

    decompositions.push_back(
        std::move(decomposition));
  }

  return decompositions;
}

} // namespace systolic
} // namespace mlir
