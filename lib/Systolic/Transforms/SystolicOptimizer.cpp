#include "Systolic/SystolicOptimizer.h"

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

} // namespace systolic
} // namespace mlir
