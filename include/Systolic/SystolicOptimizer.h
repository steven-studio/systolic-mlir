#ifndef SYSTOLIC_OPTIMIZER_H
#define SYSTOLIC_OPTIMIZER_H

#include "Systolic/SystolicTiling.h"

#include "mlir/Support/LogicalResult.h"

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

/// Exact deadline-search systolic optimization.
///
/// Searches for the minimum feasible makespan T by binary search.
///
/// For geometry g:
///
///   C(g) = 2g - 2 + H(g)              one invocation
///   cost(g) = I(g) * C(g)             one spatial tile
///
/// where I(g) is the resources' invocationsPerTile (ceil(K / k_max(g)) for
/// the GEMM at hand), and for m physical accelerators of that geometry:
///
///   capacity(T) = m * floor(T / cost(g))
///
/// A scheduled tile's [startCycle, endCycle) spans all I(g) invocations.
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
