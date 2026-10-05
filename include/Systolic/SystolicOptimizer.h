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

/// Greedy end-to-end systolic optimization.
///
/// The optimizer performs exactly three stages:
///
///   1. greedy spatial decomposition
///   2. geometry-based compute-cost construction
///   3. physical accelerator scheduling
///
/// The decomposition is unique: this function does not enumerate
/// alternative decompositions and does not perform DP/backtracking.
///
/// For the current analytical model:
///
///   C(g) = 2g - 2
///
/// where g is the systolic-array geometry.
FailureOr<ExactSystolicOptimizationResult>
optimizeSystolicGreedy(
    int64_t rows,
    int64_t columns,
    llvm::ArrayRef<SystolicArrayResource> fleet);

/// Exact deadline-search systolic optimization.
///
/// Searches for the minimum feasible makespan T by binary search.
///
/// For geometry g:
///
///   C(g) = 2g - 2 + H(g)
///
/// and for m physical accelerators of that geometry:
///
///   capacity(T) = m * floor(T / C(g))
///
/// The feasibility test constructs a spatial decomposition using
/// the largest geometry whose spatial and deadline capacity are
/// both feasible.
///
/// Padding is allowed internally for spatial alignment, but a tile
/// containing no element of the original rows x columns region is
/// never emitted.
FailureOr<ExactSystolicOptimizationResult>
optimizeSystolicBinarySearch(
    int64_t rows,
    int64_t columns,
    llvm::ArrayRef<SystolicArrayResource> fleet);

} // namespace systolic
} // namespace mlir

#endif // SYSTOLIC_OPTIMIZER_H
