#ifndef SYSTOLIC_COMPUTE_COST_H
#define SYSTOLIC_COMPUTE_COST_H

#include "Systolic/SystolicTiling.h"

#include <cstdint>

namespace mlir {
namespace systolic {

/// Parameters of the hardware-calibrated compute cost model.
///
/// DMA transfer time is intentionally outside this model.
/// DMA-aware scheduling is future work.
struct SystolicComputeCostParams {
  /// Maximum reduction depth of one invocation.
  int64_t kMax;

  /// Hardware-calibrated per-invocation implementation overhead.
  int64_t implementationOverhead;
};

/// Compute-cost parameters associated with one physical
/// systolic-array geometry.
///
/// Multiple physical accelerator instances may share the same
/// geometry and therefore the same calibrated parameters.
struct SystolicGeometryCostParams {
  int64_t arraySize;
  SystolicComputeCostParams params;
};

/// Estimate compute cycles for one spatial systolic tile.
///
/// For one tile mapped to one accelerator:
///
///   I = ceil(K / kMax)
///
///   T = sum_i [
///         k_i
///         + (rows + cols - 2)
///         + H
///       ]
///
/// where:
///   sum_i k_i = K
///   k_i <= kMax
///   H is charged once per invocation.
///
/// Therefore:
///
///   T = K + I * (rows + cols - 2 + H)
///
/// DMA transfer time is NOT included.
FailureOr<int64_t> estimateSystolicTileComputeCycles(
    const SystolicTile &tile,
    int64_t K,
    const SystolicComputeCostParams &params);

/// Estimate compute cycles for an execution task.
///
/// This overload allows decomposition candidates to be costed
/// directly before physical accelerator assignment.
///
/// The current model requires:
///
///   task.size == task.acceleratorSize
///
/// and uses the same analytical model as SystolicTile:
///
///   I = ceil(K / kMax)
///
///   T = K + I * (rows + cols - 2 + H)
///
FailureOr<int64_t> estimateSystolicTileComputeCycles(
    const SystolicExecutionTask &task,
    int64_t K,
    const SystolicComputeCostParams &params);

/// Estimate compute cycles for a logical task assigned to a
/// physical systolic-array resource.
///
/// The logical task size and physical accelerator geometry are
/// intentionally separate:
///
///   task.size          = spatial work size
///   resource.arraySize = physical systolic-array geometry
///
/// A larger physical array may execute a smaller logical task.
///
/// The physical resource determines the systolic geometry term:
///
///   rows = resource.arraySize
///   cols = resource.arraySize
///
/// Therefore:
///
///   I = ceil(K / kMax)
///
///   T = K + I * (rows + cols - 2 + H)
///
/// The assignment is legal when:
///
///   resource.arraySize >= task.size
///
/// The supplied kMax and H calibration parameters must
/// correspond to the physical resource geometry.
FailureOr<int64_t> estimateSystolicTaskComputeCycles(
    const SystolicExecutionTask &task,
    const SystolicArrayResource &resource,
    int64_t K,
    const SystolicComputeCostParams &params);

} // namespace systolic
} // namespace mlir

#endif // SYSTOLIC_COMPUTE_COST_H
