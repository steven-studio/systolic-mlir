#ifndef SYSTOLIC_SCHEDULING_H
#define SYSTOLIC_SCHEDULING_H

#include "Systolic/SystolicComputeCost.h"
#include "Systolic/SystolicTiling.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"

#include <cstdint>

namespace mlir {
namespace systolic {

/// Find an exact minimum-makespan schedule for a fixed logical
/// task decomposition.
///
/// Unlike the legacy scheduler, compute cost is not fixed before
/// physical accelerator assignment.
///
/// For every candidate task-to-resource assignment:
///
///   1. verify that the physical resource can execute the task,
///   2. find calibration parameters for that physical geometry,
///   3. compute assignment-specific execution cycles,
///   4. update the physical resource load,
///   5. recursively search the remaining assignments.
///
/// This allows, for example, the same logical 4x4 task to have
/// different costs on physical 4x4 and 8x8 arrays.
FailureOr<llvm::SmallVector<ScheduledSystolicTile>>
minimizeSystolicMakeSpanAssignmentAware(
    llvm::ArrayRef<SystolicExecutionTask> tasks,
    int64_t K,
    llvm::ArrayRef<SystolicArrayResource> fleet,
    llvm::ArrayRef<SystolicGeometryCostParams> costParams);

} // namespace systolic
} // namespace mlir

#endif // SYSTOLIC_SCHEDULING_H
