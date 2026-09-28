#include "Systolic/SystolicOptimizer.h"
#include "Systolic/SystolicDecompositionDP.h"
#include "Systolic/SystolicSchedulingDP.h"

#include <algorithm>
#include <limits>

namespace mlir {
namespace systolic {

FailureOr<ExactSystolicOptimizationResult>
optimizeSystolicTileExact(
    const SystolicTile &tile,
    int64_t K,
    llvm::ArrayRef<SystolicArrayResource> fleet,
    llvm::ArrayRef<SystolicGeometryCostParams> costParams) {

  if (tile.size <= 0 ||
      K <= 0 ||
      fleet.empty() ||
      costParams.empty())
    return failure();

  auto decompositions =
      enumerateSystolicDecompositions(
          tile,
          fleet);

  if (decompositions.empty())
    return failure();

  ExactSystolicOptimizationResult bestResult;
  int64_t bestMakeSpan =
      std::numeric_limits<int64_t>::max();

  bool foundValidSchedule = false;

  for (const auto &decomposition : decompositions) {
    auto schedule =
        minimizeSystolicMakeSpanAssignmentAware(
            decomposition,
            K,
            fleet,
            costParams);

    // A spatial decomposition may exist but still be impossible
    // to schedule under the supplied physical/calibration model.
    if (failed(schedule))
      continue;

    int64_t makeSpan = 0;

    for (const ScheduledSystolicTile &entry : *schedule)
      makeSpan =
          std::max(
              makeSpan,
              entry.endCycle);

    if (!foundValidSchedule ||
        makeSpan < bestMakeSpan) {
      foundValidSchedule = true;
      bestMakeSpan = makeSpan;

      bestResult.decomposition =
          decomposition;

      bestResult.schedule =
          *schedule;

      bestResult.makespan =
          makeSpan;
    }
  }

  if (!foundValidSchedule)
    return failure();

  return bestResult;
}


FailureOr<ExactSystolicOptimizationResult>
optimizeSystolicRectangleDP(
    int64_t rows,
    int64_t columns,
    int64_t K,
    llvm::ArrayRef<SystolicArrayResource> fleet,
    llvm::ArrayRef<SystolicGeometryCostParams> costParams) {

  if (rows <= 0 ||
      columns <= 0 ||
      K <= 0 ||
      fleet.empty() ||
      costParams.empty())
    return failure();

  // ----------------------------------------------------------
  // Extract unique logical geometry sizes from the physical
  // fleet.
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
      geometries.push_back(
          resource.arraySize);
    }
  }

  std::sort(
      geometries.begin(),
      geometries.end());

  // ----------------------------------------------------------
  // DP[R][C]:
  //
  // Enumerate only unique geometry multisets. Spatial
  // placements that produce the same multiset are intentionally
  // collapsed.
  // ----------------------------------------------------------

  auto multisets =
      enumerateSystolicGeometryMultisetsDP(
          rows,
          columns,
          geometries);

  if (multisets.empty())
    return failure();

  ExactSystolicOptimizationResult bestResult;
  int64_t bestMakeSpan =
      std::numeric_limits<int64_t>::max();

  bool foundValidSchedule = false;

  for (const SystolicGeometryMultiset &multiset :
       multisets) {

    if (multiset.counts.size() !=
        geometries.size())
      return failure();

    llvm::SmallVector<SystolicExecutionTask> tasks;

    // --------------------------------------------------------
    // Materialize the geometry multiset as logical tasks.
    //
    // row/column are intentionally set to zero because the
    // current compute-cost and scheduling models are
    // position-independent.
    // --------------------------------------------------------

    for (size_t geometryIndex = 0;
         geometryIndex < geometries.size();
         ++geometryIndex) {

      const int64_t geometry =
          geometries[geometryIndex];

      const int64_t count =
          multiset.counts[geometryIndex];

      if (count < 0)
        return failure();

      for (int64_t taskIndex = 0;
           taskIndex < count;
           ++taskIndex) {

        SystolicExecutionTask task;

        task.row = 0;
        task.column = 0;
        task.size = geometry;
        task.acceleratorSize = geometry;
        task.acceleratorId = -1;

        tasks.push_back(task);
      }
    }

    if (tasks.empty())
      continue;

    auto schedule =
        minimizeSystolicMakeSpanAssignmentAware(
            tasks,
            K,
            fleet,
            costParams);

    if (failed(schedule))
      continue;

    int64_t makeSpan = 0;

    for (const ScheduledSystolicTile &entry :
         *schedule) {
      makeSpan =
          std::max(
              makeSpan,
              entry.endCycle);
    }

    if (!foundValidSchedule ||
        makeSpan < bestMakeSpan) {

      foundValidSchedule = true;
      bestMakeSpan = makeSpan;

      bestResult.decomposition =
          tasks;

      bestResult.schedule =
          *schedule;

      bestResult.makespan =
          makeSpan;
    }
  }

  if (!foundValidSchedule)
    return failure();

  return bestResult;
}



FailureOr<SystolicDPOptimizationResult>
optimizeSystolicRectangleWithSchedulingDP(
    int64_t rows,
    int64_t columns,
    int64_t K,
    llvm::ArrayRef<SystolicArrayResource> fleet,
    llvm::ArrayRef<SystolicGeometryCostParams> costParams) {

  if (rows <= 0 ||
      columns <= 0 ||
      K <= 0 ||
      fleet.empty() ||
      costParams.empty())
    return failure();

  // ----------------------------------------------------------
  // Extract unique logical geometry sizes from the physical
  // fleet.
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
      geometries.push_back(
          resource.arraySize);
    }
  }

  std::sort(
      geometries.begin(),
      geometries.end());

  // ----------------------------------------------------------
  // Pad the logical problem to the minimum supported geometry.
  //
  // This guarantees that every positive input rectangle has at
  // least one legal decomposition: in the worst case, the
  // padded domain can be covered entirely by minGeometry tiles.
  //
  // Padding is currently modeled as real zero-padded work, so
  // the existing decomposition, cost, and scheduling models do
  // not need special boundary-tile handling.
  // ----------------------------------------------------------

  const int64_t minGeometry =
      geometries.front();

  const int64_t rowRemainder =
      rows % minGeometry;

  const int64_t columnRemainder =
      columns % minGeometry;

  int64_t paddedRows = rows;
  int64_t paddedColumns = columns;

  if (rowRemainder != 0) {
    const int64_t padding =
        minGeometry - rowRemainder;

    if (rows >
        std::numeric_limits<int64_t>::max() -
            padding)
      return failure();

    paddedRows += padding;
  }

  if (columnRemainder != 0) {
    const int64_t padding =
        minGeometry - columnRemainder;

    if (columns >
        std::numeric_limits<int64_t>::max() -
            padding)
      return failure();

    paddedColumns += padding;
  }

  // ----------------------------------------------------------
  // First DP layer:
  //
  // Enumerate only unique geometry multisets over the padded
  // logical domain. Spatial placements producing the same
  // multiset are collapsed.
  // ----------------------------------------------------------

  auto multisets =
      enumerateSystolicGeometryMultisetsDP(
          paddedRows,
          paddedColumns,
          geometries);

  if (multisets.empty())
    return failure();

  SystolicDPOptimizationResult bestResult;

  int64_t bestMakeSpan =
      std::numeric_limits<int64_t>::max();

  bool foundValidSchedule = false;

  for (const SystolicGeometryMultiset &multiset :
       multisets) {

    if (multiset.counts.size() !=
        geometries.size())
      return failure();

    llvm::SmallVector<SystolicExecutionTask> tasks;

    // --------------------------------------------------------
    // Materialize the geometry multiset as logical tasks.
    //
    // Spatial coordinates are intentionally discarded because
    // the current compute-cost and scheduling models are
    // position-independent.
    // --------------------------------------------------------

    for (size_t geometryIndex = 0;
         geometryIndex < geometries.size();
         ++geometryIndex) {

      const int64_t geometry =
          geometries[geometryIndex];

      const int64_t count =
          multiset.counts[geometryIndex];

      if (count < 0)
        return failure();

      for (int64_t taskIndex = 0;
           taskIndex < count;
           ++taskIndex) {

        SystolicExecutionTask task;

        task.row = 0;
        task.column = 0;
        task.size = geometry;
        task.acceleratorSize = geometry;
        task.acceleratorId = -1;

        tasks.push_back(task);
      }
    }

    if (tasks.empty())
      continue;

    // --------------------------------------------------------
    // Exact scheduling layer:
    //
    // Enumerate count allocations of indistinguishable logical
    // geometries across physical resources.  This preserves the
    // assignment-aware cost model without exploring task-order
    // permutations.
    // --------------------------------------------------------

    auto makeSpan =
        minimizeSystolicMakeSpanCountAllocation(
            tasks,
            K,
            fleet,
            costParams);

    if (failed(makeSpan))
      continue;

    if (!foundValidSchedule ||
        *makeSpan < bestMakeSpan) {

      foundValidSchedule = true;
      bestMakeSpan = *makeSpan;

      bestResult.decomposition =
          tasks;

      bestResult.makespan =
          *makeSpan;
    }
  }

  if (!foundValidSchedule)
    return failure();

  return bestResult;
}
} // namespace systolic
} // namespace mlir
