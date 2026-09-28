#ifndef SYSTOLIC_OPTIMIZER_H
#define SYSTOLIC_OPTIMIZER_H

#include "Systolic/SystolicComputeCost.h"
#include "Systolic/SystolicScheduling.h"
#include "Systolic/SystolicTiling.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"

#include <cstdint>

namespace mlir {
namespace systolic {

/// Result of exact joint decomposition and scheduling search.
struct ExactSystolicOptimizationResult {
  /// Spatial decomposition selected by the optimizer.
  llvm::SmallVector<SystolicExecutionTask> decomposition;

  /// Exact minimum-makespan physical schedule for that
  /// decomposition.
  llvm::SmallVector<ScheduledSystolicTile> schedule;

  /// Completion time of the final scheduled task.
  int64_t makespan;
};

/// Enumerate every legal spatial decomposition of `tile`,
/// compute the exact assignment-aware minimum-makespan schedule
/// for each decomposition, and return the globally best result.
///
/// This is intended primarily as a correctness oracle for future
/// scalable heuristics / dynamic-programming algorithms.
FailureOr<ExactSystolicOptimizationResult>
optimizeSystolicTileExact(
    const SystolicTile &tile,
    int64_t K,
    llvm::ArrayRef<SystolicArrayResource> fleet,
    llvm::ArrayRef<SystolicGeometryCostParams> costParams);

} // namespace systolic
} // namespace mlir

#endif // SYSTOLIC_OPTIMIZER_H
