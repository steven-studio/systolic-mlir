#include "Systolic/SystolicOptimizer.h"
#include "Systolic/SystolicTiling.h"

#include <algorithm>
#include <limits>
#include <vector>

namespace mlir {
namespace systolic {

FailureOr<ExactSystolicOptimizationResult>
optimizeSystolicGreedy(
    int64_t rows,
    int64_t columns,
    llvm::ArrayRef<SystolicArrayResource> fleet) {

  // ----------------------------------------------------------
  // Stage 1:
  //
  // Produce the unique greedy spatial decomposition.
  // ----------------------------------------------------------

  auto decomposition =
      greedySystolicDecompose(
          rows,
          columns,
          fleet);

  if (failed(decomposition))
    return failure();

  llvm::SmallVector<SystolicTile> tiles =
      decomposition->tiles;

  // ----------------------------------------------------------
  // Stage 2:
  //
  // Construct compute cost from tile geometry.
  //
  // Current analytical model:
  //
  //     C(g) = 2g - 2
  //
  // Same geometry => same compute cost.
  // ----------------------------------------------------------

  // ----------------------------------------------------------
  // Stage 2:
  //
  // Compute hardware-calibrated cost for every tile.
  //
  //     C(g) = 2g - 2 + H
  //
  // where:
  //
  //   g = systolic-array geometry
  //   H = fixed hardware implementation overhead per invocation
  //
  // The scheduler receives one computeCycles value per tile.
  // Therefore all physical accelerators of the same geometry must
  // use the same calibrated H.
  // ----------------------------------------------------------

  llvm::SmallVector<int64_t> computeCycles;
  computeCycles.reserve(tiles.size());

  for (const SystolicTile &tile : tiles) {

    if (tile.size <= 0)
      return failure();

    const int64_t maxGeometry =
        std::numeric_limits<int64_t>::max() / 2 + 1;

    if (tile.size > maxGeometry)
      return failure();

    bool foundGeometry = false;
    int64_t tileOverhead = 0;

    for (const SystolicArrayResource &resource : fleet) {

      if (resource.arraySize != tile.size)
        continue;

      if (resource.tileOverhead < 0)
        return failure();

      if (!foundGeometry) {
        tileOverhead = resource.tileOverhead;
        foundGeometry = true;
        continue;
      }

      // One geometry must have one calibrated H.
      if (resource.tileOverhead != tileOverhead)
        return failure();
    }

    if (!foundGeometry)
      return failure();

    const int64_t baseCost =
        2 * tile.size - 2;

    if (tileOverhead >
        std::numeric_limits<int64_t>::max() - baseCost)
      return failure();

    computeCycles.push_back(
        baseCost + tileOverhead);
  }

  // ----------------------------------------------------------
  // Stage 3:
  //
  // Schedule the already-decomposed tiles on the physical
  // fleet.
  //
  // Scheduling does not split tiles and does not promote a tile
  // to a larger geometry.
  // ----------------------------------------------------------

  SystolicFleetState fleetState;

  fleetState.resources.assign(
      fleet.begin(),
      fleet.end());

  auto schedule =
      scheduleSystolicTiles(
          tiles,
          computeCycles,
          fleetState);

  if (failed(schedule))
    return failure();

  int64_t makespan = 0;

  for (const ScheduledSystolicTile &entry : *schedule) {

    if (entry.endCycle < entry.startCycle)
      return failure();

    makespan =
        std::max(
            makespan,
            entry.endCycle);
  }

  // ----------------------------------------------------------
  // Return the complete end-to-end result.
  // ----------------------------------------------------------

  llvm::SmallVector<SystolicExecutionTask> tasks;
  tasks.reserve(tiles.size());

  for (const SystolicTile &tile : tiles) {

    SystolicExecutionTask task;

    task.row = tile.row;
    task.column = tile.column;
    task.size = tile.size;
    task.acceleratorSize = tile.acceleratorSize;
    task.acceleratorId = -1;

    tasks.push_back(task);
  }

  ExactSystolicOptimizationResult result;

  result.decomposition = std::move(tasks);
  result.schedule = std::move(*schedule);
  result.makespan = makespan;

  return result;
}



namespace {

struct BinarySearchPlacement {
  SystolicTile tile;
  int64_t acceleratorId;
  int64_t startCycle;
  int64_t endCycle;
};

static bool checkedMultiply(
    int64_t a,
    int64_t b,
    int64_t &result) {

  if (a < 0 || b < 0)
    return false;

  if (a != 0 &&
      b > std::numeric_limits<int64_t>::max() / a)
    return false;

  result = a * b;
  return true;
}

static bool checkedCeilDiv(
    int64_t value,
    int64_t divisor,
    int64_t &result) {

  if (value <= 0 ||
      divisor <= 0)
    return false;

  result =
      value / divisor +
      ((value % divisor) != 0 ? 1 : 0);

  return true;
}

/// Compute the calibrated execution cost of one invocation.
///
///     C(g) = 2g - 2 + H(g)
///
/// All physical accelerators of one geometry must agree on H.
static bool getBinarySearchGeometryCost(
    int64_t geometry,
    llvm::ArrayRef<SystolicArrayResource> fleet,
    int64_t &cost) {

  if (geometry <= 0)
    return false;

  bool found = false;
  int64_t overhead = 0;

  for (const SystolicArrayResource &resource : fleet) {

    if (resource.arraySize != geometry)
      continue;

    if (resource.tileOverhead < 0)
      return false;

    if (!found) {
      overhead = resource.tileOverhead;
      found = true;
      continue;
    }

    if (resource.tileOverhead != overhead)
      return false;
  }

  if (!found)
    return false;

  if (geometry >
      std::numeric_limits<int64_t>::max() / 2 + 1)
    return false;

  const int64_t baseCost =
      2 * geometry - 2;

  if (overhead >
      std::numeric_limits<int64_t>::max() - baseCost)
    return false;

  cost = baseCost + overhead;
  return true;
}

/// Check whether the original rows x columns region can be
/// decomposed and executed completely by deadline T.
///
/// IMPORTANT:
///
/// The padded region is only an implementation domain.
/// Feasibility terminates when all ORIGINAL cells are covered.
///
/// Therefore a tile whose entire area is padding can never be
/// generated.
static bool binarySearchFeasible(
    int64_t rows,
    int64_t columns,
    llvm::ArrayRef<int64_t> geometries,
    llvm::ArrayRef<SystolicArrayResource> fleet,
    int64_t deadline,
    llvm::SmallVectorImpl<BinarySearchPlacement> &placements) {

  placements.clear();

  if (rows <= 0 ||
      columns <= 0 ||
      fleet.empty() ||
      geometries.empty() ||
      deadline < 0)
    return false;

  // ----------------------------------------------------------
  // Normalize geometry order: largest -> smallest.
  // ----------------------------------------------------------

  llvm::SmallVector<int64_t> sortedGeometries;

  for (int64_t geometry : geometries) {

    if (geometry <= 0)
      return false;

    if (std::find(
            sortedGeometries.begin(),
            sortedGeometries.end(),
            geometry) ==
        sortedGeometries.end()) {

      sortedGeometries.push_back(geometry);
    }
  }

  std::sort(
      sortedGeometries.begin(),
      sortedGeometries.end(),
      std::greater<int64_t>());

  if (sortedGeometries.empty())
    return false;

  const int64_t smallestGeometry =
      sortedGeometries.back();

  // ----------------------------------------------------------
  // Build padded spatial domain.
  // ----------------------------------------------------------

  int64_t rowBlocks = 0;
  int64_t columnBlocks = 0;

  if (!checkedCeilDiv(
          rows,
          smallestGeometry,
          rowBlocks) ||
      !checkedCeilDiv(
          columns,
          smallestGeometry,
          columnBlocks))
    return false;

  int64_t paddedRows = 0;
  int64_t paddedColumns = 0;

  if (!checkedMultiply(
          rowBlocks,
          smallestGeometry,
          paddedRows) ||
      !checkedMultiply(
          columnBlocks,
          smallestGeometry,
          paddedColumns))
    return false;

  int64_t paddedElements = 0;

  if (!checkedMultiply(
          paddedRows,
          paddedColumns,
          paddedElements))
    return false;

  // ----------------------------------------------------------
  // Per-geometry deadline capacity.
  //
  // For geometry g:
  //
  //   one invocation cost = C(g)
  //
  //   invocations / accelerator
  //       = floor(T / C(g))
  //
  //   total capacity
  //       = #accelerators(g) *
  //         floor(T / C(g))
  // ----------------------------------------------------------

  struct GeometryCapacity {
    int64_t geometry;
    int64_t cost;
    int64_t acceleratorCount;
    int64_t capacity;
  };

  llvm::SmallVector<GeometryCapacity>
      capacities;

  for (int64_t geometry : sortedGeometries) {

    int64_t cost = 0;

    if (!getBinarySearchGeometryCost(
            geometry,
            fleet,
            cost))
      return false;

    int64_t acceleratorCount = 0;

    for (const SystolicArrayResource &resource :
         fleet) {

      if (resource.arraySize != geometry)
        continue;

      if (acceleratorCount ==
          std::numeric_limits<int64_t>::max())
        return false;

      ++acceleratorCount;
    }

    if (acceleratorCount <= 0)
      return false;

    const int64_t perAcceleratorCapacity =
        deadline / cost;

    int64_t totalCapacity = 0;

    if (!checkedMultiply(
            acceleratorCount,
            perAcceleratorCapacity,
            totalCapacity))
      return false;

    capacities.push_back(
        GeometryCapacity{
            geometry,
            cost,
            acceleratorCount,
            totalCapacity});
  }

  // ----------------------------------------------------------
  // Occupancy of the padded domain.
  //
  // This is NOT the completion condition.
  // ----------------------------------------------------------

  std::vector<uint8_t> covered(
      static_cast<size_t>(paddedElements),
      0);

  const auto indexOf =
      [paddedColumns](
          int64_t row,
          int64_t column) -> size_t {

    return static_cast<size_t>(
        row * paddedColumns + column);
  };

  int64_t originalElements = 0;

  if (!checkedMultiply(
          rows,
          columns,
          originalElements))
    return false;

  int64_t coveredOriginalElements = 0;

  llvm::SmallVector<int64_t>
      usedCapacity(
          capacities.size(),
          0);

  // ----------------------------------------------------------
  // Row-major first uncovered ORIGINAL element.
  //
  // This is the key padding rule:
  //
  //   We search only [0, rows) x [0, columns).
  //
  // Therefore we never create a padding-only tile.
  // ----------------------------------------------------------

  while (coveredOriginalElements <
         originalElements) {

    int64_t startRow = -1;
    int64_t startColumn = -1;

    for (int64_t row = 0;
         row < rows && startRow < 0;
         ++row) {

      for (int64_t column = 0;
           column < columns;
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
      return false;

    bool placed = false;

    // --------------------------------------------------------
    // Largest geometry first.
    // --------------------------------------------------------

    for (size_t geometryIndex = 0;
         geometryIndex < capacities.size();
         ++geometryIndex) {

      const GeometryCapacity &capacity =
          capacities[geometryIndex];

      const int64_t geometry =
          capacity.geometry;

      // Spatial fit inside padded domain.
      if (geometry >
              paddedRows - startRow ||
          geometry >
              paddedColumns - startColumn)
        continue;

      // Deadline capacity.
      if (usedCapacity[geometryIndex] >=
          capacity.capacity)
        continue;

      // Check overlap.
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

      if (!legal)
        continue;

      // ------------------------------------------------------
      // Because (startRow,startColumn) is an uncovered
      // ORIGINAL cell, this tile contains at least one
      // original element.
      //
      // A padding-only tile is therefore impossible here.
      // ------------------------------------------------------

      const int64_t slot =
          usedCapacity[geometryIndex];

      const int64_t acceleratorIndex =
          slot % capacity.acceleratorCount;

      const int64_t executionRound =
          slot / capacity.acceleratorCount;

      const int64_t startCycle =
          executionRound * capacity.cost;

      const int64_t endCycle =
          startCycle + capacity.cost;

      if (endCycle > deadline)
        continue;

      int64_t acceleratorId = -1;
      int64_t seen = 0;

      for (const SystolicArrayResource &resource :
           fleet) {

        if (resource.arraySize != geometry)
          continue;

        if (seen == acceleratorIndex) {
          acceleratorId =
              resource.acceleratorId;
          break;
        }

        ++seen;
      }

      if (acceleratorId < 0)
        return false;

      SystolicTile tile;

      tile.row = startRow;
      tile.column = startColumn;
      tile.size = geometry;
      tile.acceleratorSize = geometry;
      tile.acceleratorId = acceleratorId;

      // ------------------------------------------------------
      // Commit occupancy.
      // ------------------------------------------------------

      for (int64_t r = startRow;
           r < startRow + geometry;
           ++r) {

        for (int64_t c = startColumn;
             c < startColumn + geometry;
             ++c) {

          const size_t index =
              indexOf(r, c);

          if (covered[index] != 0)
            return false;

          covered[index] = 1;

          if (r < rows &&
              c < columns)
            ++coveredOriginalElements;
        }
      }

      ++usedCapacity[geometryIndex];

      placements.push_back(
          BinarySearchPlacement{
              tile,
              acceleratorId,
              startCycle,
              endCycle});

      placed = true;
      break;
    }

    if (!placed)
      return false;
  }

  return true;
}

} // namespace


FailureOr<ExactSystolicOptimizationResult>
optimizeSystolicBinarySearch(
    int64_t rows,
    int64_t columns,
    llvm::ArrayRef<SystolicArrayResource> fleet) {

  if (rows <= 0 ||
      columns <= 0 ||
      fleet.empty())
    return failure();

  // ----------------------------------------------------------
  // Collect distinct geometries.
  // ----------------------------------------------------------

  llvm::SmallVector<int64_t> geometries;

  for (const SystolicArrayResource &resource :
       fleet) {

    if (resource.arraySize <= 0 ||
        resource.tileOverhead < 0)
      return failure();

    if (std::find(
            geometries.begin(),
            geometries.end(),
            resource.arraySize) ==
        geometries.end()) {

      geometries.push_back(
          resource.arraySize);
    }
  }

  if (geometries.empty())
    return failure();

  std::sort(
      geometries.begin(),
      geometries.end(),
      std::greater<int64_t>());

  const int64_t smallestGeometry =
      geometries.back();

  // ----------------------------------------------------------
  // Compute costs.
  // ----------------------------------------------------------

  llvm::SmallVector<int64_t> costs;

  for (int64_t geometry : geometries) {

    int64_t cost = 0;

    if (!getBinarySearchGeometryCost(
            geometry,
            fleet,
            cost))
      return failure();

    costs.push_back(cost);
  }

  // ----------------------------------------------------------
  // Construct a safe upper bound.
  //
  // Use the smallest geometry everywhere and execute those
  // padded tiles sequentially on one accelerator.
  // ----------------------------------------------------------

  int64_t rowBlocks = 0;
  int64_t columnBlocks = 0;

  if (!checkedCeilDiv(
          rows,
          smallestGeometry,
          rowBlocks) ||
      !checkedCeilDiv(
          columns,
          smallestGeometry,
          columnBlocks))
    return failure();

  int64_t tileCount = 0;

  if (!checkedMultiply(
          rowBlocks,
          columnBlocks,
          tileCount))
    return failure();

  if (tileCount <= 0)
    return failure();

  const int64_t smallestCost =
      costs.back();

  int64_t high = 0;

  if (!checkedMultiply(
          tileCount,
          smallestCost,
          high))
    return failure();

  // ----------------------------------------------------------
  // Binary search for the smallest feasible T.
  //
  // feasible(T) is monotone:
  //
  //   feasible(T)
  //       =>
  //   feasible(T + delta)
  //
  // because increasing T never removes deadline capacity.
  // ----------------------------------------------------------

  int64_t low = 0;

  while (low < high) {

    const int64_t mid =
        low +
        (high - low) / 2;

    llvm::SmallVector<BinarySearchPlacement>
        candidate;

    const bool feasible =
        binarySearchFeasible(
            rows,
            columns,
            geometries,
            fleet,
            mid,
            candidate);

    if (feasible) {
      high = mid;
    } else {
      low = mid + 1;
    }
  }

  // ----------------------------------------------------------
  // Reconstruct at optimal deadline T*.
  // ----------------------------------------------------------

  llvm::SmallVector<BinarySearchPlacement>
      finalPlacements;

  if (!binarySearchFeasible(
          rows,
          columns,
          geometries,
          fleet,
          low,
          finalPlacements))
    return failure();

  ExactSystolicOptimizationResult result;

  result.makespan = low;

  for (const BinarySearchPlacement &placement :
       finalPlacements) {

    SystolicExecutionTask task;

    task.row =
        placement.tile.row;

    task.column =
        placement.tile.column;

    task.size =
        placement.tile.size;

    task.acceleratorSize =
        placement.tile.acceleratorSize;

    task.acceleratorId =
        placement.acceleratorId;

    result.decomposition.push_back(task);

    ScheduledSystolicTile scheduled;

    scheduled.tile =
        placement.tile;

    scheduled.startCycle =
        placement.startCycle;

    scheduled.endCycle =
        placement.endCycle;

    result.schedule.push_back(
        scheduled);
  }

  return result;
}


} // namespace systolic
} // namespace mlir
