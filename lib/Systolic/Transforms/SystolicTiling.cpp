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

FailureOr<SystolicDecompositionDPResult>
minimizeSystolicMakeSpanMacroSplit(
    int64_t rows,
    int64_t columns,
    int64_t largestGeometry,
    int64_t splitGeometry,
    llvm::ArrayRef<SystolicArrayResource> fleet,
    const std::function<int64_t(int64_t, int64_t)> &costFn) {

  // ----------------------------------------------------------
  // Validate the structured split model.
  // ----------------------------------------------------------

  if (rows <= 0 ||
      columns <= 0 ||
      rows != columns ||
      largestGeometry <= 0 ||
      splitGeometry <= 0 ||
      splitGeometry * 2 != largestGeometry ||
      rows % largestGeometry != 0 ||
      !costFn ||
      fleet.empty())
    return failure();

  // ----------------------------------------------------------
  // Locate physical accelerators.
  // ----------------------------------------------------------

  llvm::SmallVector<SystolicArrayResource> largeMachines;
  llvm::SmallVector<SystolicArrayResource> smallMachines;

  for (const SystolicArrayResource &resource : fleet) {
    if (resource.arraySize == largestGeometry)
      largeMachines.push_back(resource);

    if (resource.arraySize == splitGeometry)
      smallMachines.push_back(resource);
  }

  if (largeMachines.empty() ||
      smallMachines.empty())
    return failure();

  // This structured formulation intentionally models:
  //
  //   one largest accelerator
  //   plus multiple half-size accelerators.
  //
  // For the current target this is:
  //
  //   1 x 8x8
  //   3 x 4x4
  //
  // If there are multiple largest accelerators, this formulation
  // would need to be generalized because the objective changes.
  if (largeMachines.size() != 1)
    return failure();

  // ----------------------------------------------------------
  // The input consists of a regular grid of largestGeometry
  // macro-blocks.
  //
  // Example:
  //
  //   32 / 8 = 4
  //
  // therefore:
  //
  //   4 x 4 = 16 macro-blocks.
  // ----------------------------------------------------------

  const int64_t macroRows =
      rows / largestGeometry;

  const int64_t macroColumns =
      columns / largestGeometry;

  const int64_t macroCount =
      macroRows * macroColumns;

  // ----------------------------------------------------------
  // Cost of the two possible forms.
  // ----------------------------------------------------------

  const int64_t largeCost =
      costFn(
          largestGeometry,
          largestGeometry);

  const int64_t smallCost =
      costFn(
          splitGeometry,
          splitGeometry);

  if (largeCost <= 0 ||
      smallCost <= 0)
    return failure();

  // ----------------------------------------------------------
  // For x split macro-blocks:
  //
  //   largeCount = macroCount - x
  //   smallCount = 4*x
  //
  // The one 8x8 accelerator executes all remaining large tiles.
  //
  // The small tiles are distributed over the three 4x4
  // accelerators.
  //
  // Since all small tiles have identical cost, the minimum
  // small-array makespan is:
  //
  //   ceil((4*x) / numSmallMachines) * smallCost
  //
  // The global objective is:
  //
  //   max(
  //       (macroCount-x) * largeCost,
  //       ceil(4*x/numSmallMachines) * smallCost
  //   )
  //
  // This is the optimization problem actually solved here.
  // ----------------------------------------------------------

  const int64_t smallMachineCount =
      static_cast<int64_t>(smallMachines.size());

  int64_t bestMakespan =
      std::numeric_limits<int64_t>::max();

  int64_t bestSplitCount = -1;

  for (int64_t splitCount = 0;
       splitCount <= macroCount;
       ++splitCount) {

    const int64_t largeCount =
        macroCount - splitCount;

    const int64_t smallCount =
        4 * splitCount;

    const int64_t largeMakespan =
        largeCount * largeCost;

    const int64_t smallRounds =
        (smallCount + smallMachineCount - 1) /
        smallMachineCount;

    const int64_t smallMakespan =
        smallRounds * smallCost;

    const int64_t makespan =
        std::max(
            largeMakespan,
            smallMakespan);

    if (makespan < bestMakespan) {
      bestMakespan = makespan;
      bestSplitCount = splitCount;
    }
  }

  if (bestSplitCount < 0)
    return failure();

  // ----------------------------------------------------------
  // Reconstruct the spatial decomposition.
  //
  // We deliberately choose the first `bestSplitCount`
  // macro-blocks in row-major order to be split.
  //
  // The objective depends only on counts, not on which
  // macro-blocks are split, so this gives a deterministic
  // representative.
  // ----------------------------------------------------------

  llvm::SmallVector<SystolicTile> tiles;

  int64_t macroIndex = 0;

  for (int64_t mr = 0;
       mr < macroRows;
       ++mr) {

    for (int64_t mc = 0;
         mc < macroColumns;
         ++mc) {

      const int64_t originRow =
          mr * largestGeometry;

      const int64_t originColumn =
          mc * largestGeometry;

      const bool split =
          macroIndex < bestSplitCount;

      ++macroIndex;

      if (!split) {

        SystolicTile tile;
        tile.row = originRow;
        tile.column = originColumn;
        tile.size = largestGeometry;
        tile.acceleratorSize = largestGeometry;
        tile.acceleratorId = -1;

        tiles.push_back(tile);
        continue;
      }

      // Split one GxG macro-block into four
      // (G/2)x(G/2) tiles.

      for (int64_t sr = 0;
           sr < 2;
           ++sr) {

        for (int64_t sc = 0;
             sc < 2;
             ++sc) {

          SystolicTile tile;

          tile.row =
              originRow +
              sr * splitGeometry;

          tile.column =
              originColumn +
              sc * splitGeometry;

          tile.size =
              splitGeometry;

          tile.acceleratorSize =
              splitGeometry;

          tile.acceleratorId = -1;

          tiles.push_back(tile);
        }
      }
    }
  }

  // ----------------------------------------------------------
  // Verify the generated spatial decomposition.
  // ----------------------------------------------------------

  if (failed(
          verifySystolicTiling(
              rows,
              columns,
              tiles,
              SystolicFleetState{
                  llvm::SmallVector<SystolicArrayResource>(
                      fleet.begin(),
                      fleet.end())})))
    return failure();

  // ----------------------------------------------------------
  // Physical accelerator assignment.
  //
  // Large tiles all execute on the single largest accelerator.
  //
  // Small tiles are assigned round-robin to the small
  // accelerators. Because all small jobs have identical cost,
  // this realizes the ceil(N/P) bound exactly.
  // ----------------------------------------------------------

  llvm::SmallVector<ScheduledSystolicTile> schedule;

  int64_t largeAvailable = 0;

  llvm::SmallVector<int64_t> smallAvailable(
      smallMachines.size(),
      0);

  int64_t smallIndex = 0;

  for (const SystolicTile &tile : tiles) {

    if (tile.size == largestGeometry) {

      const int64_t start =
          largeAvailable;

      const int64_t end =
          start + largeCost;

      ScheduledSystolicTile scheduled;

      scheduled.tile = tile;
      scheduled.tile.acceleratorSize =
          largestGeometry;
      scheduled.tile.acceleratorId =
          largeMachines[0].acceleratorId;
      scheduled.startCycle = start;
      scheduled.endCycle = end;

      schedule.push_back(scheduled);

      largeAvailable = end;
      continue;
    }

    if (tile.size == splitGeometry) {

      const size_t machineIndex =
          static_cast<size_t>(
              smallIndex %
              static_cast<int64_t>(
                  smallMachines.size()));

      const int64_t start =
          smallAvailable[machineIndex];

      const int64_t end =
          start + smallCost;

      ScheduledSystolicTile scheduled;

      scheduled.tile = tile;
      scheduled.tile.acceleratorSize =
          splitGeometry;
      scheduled.tile.acceleratorId =
          smallMachines[machineIndex].acceleratorId;

      scheduled.startCycle = start;
      scheduled.endCycle = end;

      schedule.push_back(scheduled);

      smallAvailable[machineIndex] = end;
      ++smallIndex;
      continue;
    }

    return failure();
  }

  // ----------------------------------------------------------
  // Check the actual reconstructed schedule.
  // ----------------------------------------------------------

  int64_t observedMakespan = 0;

  for (const ScheduledSystolicTile &scheduled :
       schedule) {

    observedMakespan =
        std::max(
            observedMakespan,
            scheduled.endCycle);
  }

  if (observedMakespan != bestMakespan)
    return failure();

  return SystolicDecompositionDPResult{
      std::move(schedule),
      bestMakespan};
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

  // ----------------------------------------------------------
  // Collect distinct legal accelerator geometries.
  //
  // Enumeration is spatial only.  Multiple physical instances
  // of the same geometry do not create different spatial
  // decompositions.
  // ----------------------------------------------------------

  llvm::SmallVector<int64_t> geometries;

  for (const SystolicArrayResource &resource : fleet) {
    const int64_t size = resource.arraySize;

    if (size <= 0 || size > tile.size)
      continue;

    if (llvm::find(geometries, size) ==
        geometries.end())
      geometries.push_back(size);
  }

  if (geometries.empty())
    return decompositions;

  llvm::sort(
      geometries,
      [](int64_t lhs, int64_t rhs) {
        return lhs > rhs;
      });

  // ----------------------------------------------------------
  // Exact-cover state.
  //
  // coverage[r * tile.size + c]:
  //   false -> not covered yet
  //   true  -> already occupied by one execution task
  //
  // Coordinates in coverage are local to the parent tile.
  // ----------------------------------------------------------

  const int64_t dimension = tile.size;

  llvm::SmallVector<bool> coverage(
      static_cast<size_t>(dimension * dimension),
      false);

  llvm::SmallVector<SystolicExecutionTask>
      currentDecomposition;

  // Return true iff a square accelerator of the requested size
  // can be placed at local coordinate (row, column).
  auto canPlace =
      [&](int64_t row,
          int64_t column,
          int64_t size) -> bool {

    if (row < 0 ||
        column < 0 ||
        row + size > dimension ||
        column + size > dimension)
      return false;

    for (int64_t r = row; r < row + size; ++r) {
      for (int64_t c = column;
           c < column + size;
           ++c) {
        const size_t index =
            static_cast<size_t>(
                r * dimension + c);

        if (coverage[index])
          return false;
      }
    }

    return true;
  };

  auto setCoverage =
      [&](int64_t row,
          int64_t column,
          int64_t size,
          bool value) {

    for (int64_t r = row; r < row + size; ++r) {
      for (int64_t c = column;
           c < column + size;
           ++c) {
        const size_t index =
            static_cast<size_t>(
                r * dimension + c);

        coverage[index] = value;
      }
    }
  };

  // ----------------------------------------------------------
  // Recursive exact-cover enumeration.
  //
  // Always choose the first uncovered cell in row-major order.
  // Every legal decomposition must place exactly one tile whose
  // top-left corner is this cell.
  //
  // Fixing this canonical next position avoids enumerating
  // permutations of the same spatial decomposition.
  // ----------------------------------------------------------

  std::function<void()> search = [&]() {
    int64_t uncoveredRow = -1;
    int64_t uncoveredColumn = -1;

    for (int64_t r = 0;
         r < dimension && uncoveredRow < 0;
         ++r) {
      for (int64_t c = 0;
           c < dimension;
           ++c) {
        const size_t index =
            static_cast<size_t>(
                r * dimension + c);

        if (!coverage[index]) {
          uncoveredRow = r;
          uncoveredColumn = c;
          break;
        }
      }
    }

    // No uncovered cell remains: exact cover found.
    if (uncoveredRow < 0) {
      decompositions.push_back(
          currentDecomposition);
      return;
    }

    // Try every legal accelerator geometry at the canonical
    // uncovered position.
    for (int64_t acceleratorSize : geometries) {
      if (!canPlace(
              uncoveredRow,
              uncoveredColumn,
              acceleratorSize))
        continue;

      setCoverage(
          uncoveredRow,
          uncoveredColumn,
          acceleratorSize,
          true);

      SystolicExecutionTask task;

      task.row =
          tile.row + uncoveredRow;

      task.column =
          tile.column + uncoveredColumn;

      task.size = acceleratorSize;
      task.acceleratorSize = acceleratorSize;

      // Physical accelerator assignment belongs to scheduling.
      task.acceleratorId = -1;

      currentDecomposition.push_back(task);

      search();

      currentDecomposition.pop_back();

      setCoverage(
          uncoveredRow,
          uncoveredColumn,
          acceleratorSize,
          false);
    }
  };

  search();

  return decompositions;
}

} // namespace systolic
} // namespace mlir
