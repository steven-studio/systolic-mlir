#include "Systolic/SystolicOptimizer.h"
#include "Systolic/SystolicTiling.h"

#include "llvm/ADT/SmallVector.h"

#include <algorithm>
#include <limits>
#include <vector>

namespace mlir {
namespace systolic {

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

/// Compute the calibrated execution cost of one spatial tile.
///
///     C(g)    = 2g - 2 + H(g)          one invocation
///     cost(g) = I(g) * C(g)            one tile, I(g) = invocationsPerTile
///
/// I(g) = ceil(K / k_max(g)) is the number of invocations the tile's
/// reduction is folded into on that geometry. k_max is per accelerator, so
/// two geometries of one fleet can need different I for the same GEMM;
/// billing every geometry one invocation (as this function did before I
/// existed) balanced the rounds as if the 8x8 and a 4x4 with a smaller
/// k_max finished a tile in the same number of invocations.
///
/// All physical accelerators of one geometry must agree on H and on I: the
/// capacity below is #accelerators(g) * floor(T / cost(g)), one cost per
/// geometry.
static bool getBinarySearchGeometryCost(
    int64_t geometry,
    llvm::ArrayRef<SystolicArrayResource> fleet,
    int64_t &cost) {

  if (geometry <= 0)
    return false;

  bool found = false;
  int64_t overhead = 0;
  int64_t invocations = 1;

  for (const SystolicArrayResource &resource : fleet) {

    if (resource.arraySize != geometry)
      continue;

    if (resource.tileOverhead < 0 ||
        resource.invocationsPerTile < 1)
      return false;

    if (!found) {
      overhead = resource.tileOverhead;
      invocations = resource.invocationsPerTile;
      found = true;
      continue;
    }

    if (resource.tileOverhead != overhead ||
        resource.invocationsPerTile != invocations)
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

  return checkedMultiply(
      invocations,
      baseCost + overhead,
      cost);
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
        resource.tileOverhead < 0 ||
        resource.invocationsPerTile < 1)
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
