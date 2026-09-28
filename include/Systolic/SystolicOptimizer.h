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

/// Result of the end-to-end DP optimizer.
///
/// Unlike ExactSystolicOptimizationResult, this first DP result does not
/// contain a reconstructed schedule. The scheduling DP currently computes
/// only the exact minimum makespan.
struct SystolicDPOptimizationResult {
  llvm::SmallVector<SystolicExecutionTask> decomposition;
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


/// Enumerate unique geometry multisets for an R x C output
/// rectangle using the decomposition DP, schedule each multiset
/// with the exact assignment-aware scheduler, and return the
/// minimum-makespan result.
///
/// Under the current position-independent compute-cost model,
/// decompositions with the same geometry multiset are scheduling
/// equivalent. Therefore spatial placement is intentionally
/// discarded by this optimizer.
FailureOr<ExactSystolicOptimizationResult>
optimizeSystolicRectangleDP(
    int64_t rows,
    int64_t columns,
    int64_t K,
    llvm::ArrayRef<SystolicArrayResource> fleet,
    llvm::ArrayRef<SystolicGeometryCostParams> costParams);


/// Optimize an R x C logical output using both decomposition DP
/// and count-based scheduling DP.
///
/// The decomposition DP enumerates unique logical geometry
/// multisets. For each multiset, the scheduling DP computes the
/// exact minimum makespan while treating identical logical tasks
/// as indistinguishable.
///
/// This version returns the selected decomposition and minimum
/// makespan. Schedule reconstruction is intentionally deferred.
FailureOr<SystolicDPOptimizationResult>
optimizeSystolicRectangleWithSchedulingDP(
    int64_t rows,
    int64_t columns,
    int64_t K,
    llvm::ArrayRef<SystolicArrayResource> fleet,
    llvm::ArrayRef<SystolicGeometryCostParams> costParams);

} // namespace systolic
} // namespace mlir

#endif // SYSTOLIC_OPTIMIZER_H
