#include "Systolic/SystolicOptimizer.h"
#include "Systolic/SystolicTiling.h"

#include <algorithm>
#include <limits>

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

  llvm::SmallVector<int64_t> computeCycles;
  computeCycles.reserve(tiles.size());

  for (const SystolicTile &tile : tiles) {

    if (tile.size <= 0)
      return failure();

    const int64_t maxGeometry =
        std::numeric_limits<int64_t>::max() / 2 + 1;

    if (tile.size > maxGeometry)
      return failure();

    computeCycles.push_back(
        2 * tile.size - 2);
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

} // namespace systolic
} // namespace mlir
