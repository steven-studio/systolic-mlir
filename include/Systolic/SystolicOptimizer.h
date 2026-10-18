#ifndef SYSTOLIC_OPTIMIZER_H
#define SYSTOLIC_OPTIMIZER_H

#include "Systolic/SystolicTiling.h"

#include "mlir/Support/LogicalResult.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"

#include <cstdint>

namespace mlir {
namespace systolic {

struct SystolicOptimizerOptions {
  /// Operand-read bandwidth all accelerators share, bytes per cycle. The
  /// FPGA fleet fills every array's operand buffer through one AXI read
  /// port (dma_engine_multi), so the arrays' fills together cannot move
  /// more than this. 0: no shared limit -- every accelerator fills at its
  /// own read_bytes_per_cycle, as if it had a port of its own.
  double sharedReadBytesPerCycle = 0.0;
};

/// Result of exact joint decomposition and scheduling search.
struct ExactSystolicOptimizationResult {
  /// Spatial decomposition selected by the optimizer.
  llvm::SmallVector<SystolicExecutionTask> decomposition;

  /// The physical schedule of that decomposition: a tile occupies its
  /// accelerator over [startCycle, endCycle) on the cost model's
  /// per-accelerator timeline (::systolic::AcceleratorTimeline): the tiles
  /// of one accelerator run in this order, each one's invocations after the
  /// previous tile's. Serialized (pipeline_depth = 1) that is every
  /// invocation's read, compute and write back to back and the intervals of
  /// one accelerator do not overlap; on a pipelined accelerator consecutive
  /// tiles' invocations overlap and so do the intervals.
  llvm::SmallVector<ScheduledSystolicTile> schedule;

  /// The smallest deadline T the search found feasible: every tile ends by
  /// T, and with a shared read bandwidth B, all operand bytes moved fit in
  /// B * T. The predicted makespan.
  int64_t makespan = 0;

  /// Latest endCycle in the schedule (<= makespan; smaller only when the
  /// shared read bandwidth, not an accelerator, decides T).
  int64_t scheduleEnd = 0;

  /// The same schedule with transfers free: for each accelerator the end of
  /// its tile stream on the compute-only timeline (the sum of its tiles'
  /// T_compute when serialized), maximized over the fleet. What the
  /// schedule costs if reads and writes were free.
  int64_t computeOnlyMakespan = 0;

  /// Operand bytes all invocations of the schedule read, and the cycles
  /// the shared read bandwidth needs for them (0 without one).
  int64_t totalReadBytes = 0;
  int64_t sharedReadCycles = 0;

  /// Feasibility tests the search ran.
  int64_t feasibilityChecks = 0;
};

/// Exact deadline-search systolic optimization of a rows x columns output
/// whose reduction is `reduction` steps deep.
///
/// Searches for the minimum feasible makespan T by binary search. For
/// geometry g, one accelerator runs its spatial tiles as one stream of
/// invocations on the cost model's per-accelerator timeline
/// (::systolic::AcceleratorTimeline). Serialized hardware: every tile costs
///
///   cost(g) = ::systolic::estimateTileCost(reduction, config(g)).sequential()
///           = sum over its I(g) = ceil(K / k_max(g)) invocations of
///             T_read + T_compute + T_write
///
/// and the stream is those back to back; pipelined hardware overlaps
/// consecutive invocations. The cost model's numbers, not ones kept here.
/// For m physical accelerators of that geometry
///
///   capacity(T) = m * (tiles of the stream that end by T)
///               = m * floor(T / cost(g)) when serialized.
///
/// All accelerators of one geometry must price a tile identically.
///
/// The feasibility test constructs a spatial decomposition using the
/// largest geometry whose spatial and deadline capacity are both feasible,
/// and, with options.sharedReadBytesPerCycle > 0, additionally requires the
/// operand bytes of all placed tiles to fit the shared bandwidth by T.
///
/// Padding is allowed internally for spatial alignment, but a tile
/// containing no element of the original rows x columns region is
/// never emitted.
FailureOr<ExactSystolicOptimizationResult>
optimizeSystolicBinarySearch(
    int64_t rows,
    int64_t columns,
    int64_t reduction,
    llvm::ArrayRef<SystolicArrayResource> fleet,
    const SystolicOptimizerOptions &options = {});

/// The feasibility test the search runs, at one deadline -- for testing the
/// search's monotonicity. failure() on invalid input.
FailureOr<bool> isSystolicDeadlineFeasible(
    int64_t rows,
    int64_t columns,
    int64_t reduction,
    llvm::ArrayRef<SystolicArrayResource> fleet,
    const SystolicOptimizerOptions &options,
    int64_t deadline);

} // namespace systolic
} // namespace mlir

#endif // SYSTOLIC_OPTIMIZER_H
