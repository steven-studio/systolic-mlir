#ifndef SYSTOLIC_OPTIMIZER_H
#define SYSTOLIC_OPTIMIZER_H

#include "Systolic/CostModelV2.h"
#include "Systolic/SystolicTiling.h"

#include "mlir/IR/Operation.h"
#include "mlir/Support/LogicalResult.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"

#include <cstdint>

namespace mlir {
namespace systolic {

/// Which cost model prices the search (docs/COSTMODEL_V2.md).
///   V1  every accelerator on its own (CostModel.h, AcceleratorTimeline)
///   V2  the fleet as one machine: every geometry's tile stream measured with
///       all accelerators busy at once, contending for the shared read port,
///       write-back engine and readback path (CostModelV2.h). A fleet the
///       V2 simulator cannot model (no FPGA transport declaration) is priced
///       by V1; the result says which model ran.
enum class CostModelVersion { V1, V2 };

/// "v1" / "v2" -> the version; false for anything else.
bool parseCostModelVersion(llvm::StringRef text, CostModelVersion &version);

struct SystolicOptimizerOptions {
  /// Operand-read bandwidth all accelerators share, bytes per cycle. The
  /// FPGA fleet fills every array's operand buffer through one AXI read
  /// port (dma_engine_multi), so the arrays' fills together cannot move
  /// more than this. 0: no shared limit -- every accelerator fills at its
  /// own read_bytes_per_cycle, as if it had a port of its own.
  double sharedReadBytesPerCycle = 0.0;

  /// The cost model (default V2).
  CostModelVersion costModel = CostModelVersion::V2;
  /// V2: the operand path of the bitstream -- false = v1 (a 128-bit beat
  /// unpacked in four cycles, the production bitstream), true = v2
  /// (beat-wide).
  bool beatWideOperandPath = false;
};

/// The V2 fleet the search would simulate for `fleet`: the accelerators in
/// fleet order (accelerator i is the read engine's slot i), the bitstream's
/// shared hardware. failure() (with the reason in *why) if some accelerator
/// lacks what the simulator models: the FPGA transport declaration
/// (read/write terms, operand_lanes = 8, k_window = 8, 4-byte elements),
/// square, rows <= 8, k_max.
FailureOr<::systolic::v2::FleetHardware>
v2FleetFor(llvm::ArrayRef<SystolicArrayResource> fleet, bool beatWide,
           std::string *why = nullptr);

/// One geometry's tile stream as the search prices it: a tile n of the
/// stream holds its accelerator over [tileStart[n], tileEnd[n]).
struct GeometryStream {
  int64_t geometry = 0;
  std::vector<int64_t> tileStart;
  std::vector<int64_t> tileEnd;
};

/// The streams buildSearchProblem gives the search for this problem, per
/// geometry of the fleet (largest first), and the model that produced them.
/// SystolicTilePlan holds every scheduled tile against them.
FailureOr<llvm::SmallVector<GeometryStream>>
systolicGeometryStreams(int64_t rows, int64_t columns, int64_t reduction,
                        llvm::ArrayRef<SystolicArrayResource> fleet,
                        const SystolicOptimizerOptions &options,
                        CostModelVersion &used);

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

  /// The model that priced the search (V2 falls back to V1 for a fleet the
  /// simulator cannot model; v2FallbackReason says why).
  CostModelVersion costModel = CostModelVersion::V1;
  std::string v2FallbackReason;

  /// V2 only: the chosen schedule run on the fleet simulator -- the tiles in
  /// decomposition order (the order the lowering submits them), offered as
  /// the asynchronous runtime offers them. -1 under V1.
  int64_t predictedDeviceMakespan = -1;   // first acceptance -> last result in DDR
  int64_t predictedReadbackEnd = -1;      // ... -> last result read back
  int64_t predictedArBursts = -1;         // AXI read requests (operands + readback)
  int64_t predictedOperandBytes = -1;     // DDR3 -> accelerators
  int64_t predictedContentionCycles = -1; // fill cycles lost to sharing

  /// Feasibility tests the search ran.
  int64_t feasibilityChecks = 0;
};

/// The cost-model remark of a search result (`report`): which model priced
/// it, and for V2 the chosen schedule's predicted device makespan, readback
/// end, read requests, operand bytes and contention.
void emitCostModelRemark(Operation *op,
                         const ExactSystolicOptimizationResult &result);

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

/// The feasibility test the search runs at every deadline in [from, to]
/// (one problem build, the predicate unchanged) -- for scanning the search's
/// monotonicity exhaustively. failure() on invalid input.
FailureOr<std::vector<bool>> scanSystolicDeadlineFeasibility(
    int64_t rows, int64_t columns, int64_t reduction,
    llvm::ArrayRef<SystolicArrayResource> fleet,
    const SystolicOptimizerOptions &options, int64_t from, int64_t to);

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
