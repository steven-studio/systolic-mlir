#ifndef SYSTOLIC_SCHEDULING_H
#define SYSTOLIC_SCHEDULING_H

#include "Systolic/SystolicTiling.h"

#include "mlir/Support/LogicalResult.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"

#include <cstdint>

namespace mlir {
namespace systolic {

/// Schedule already-assigned execution tasks on their concrete
/// physical accelerators.
///
/// Physical accelerator assignment is NOT changed by this function.
/// The scheduler only determines the temporal execution interval of
/// each task.
///
/// `computeCycles[i]` is the compute-only duration of `tasks[i]`.
///
/// Tasks assigned to the same physical accelerator execute
/// sequentially. Tasks assigned to different physical accelerators
/// may execute concurrently.
FailureOr<llvm::SmallVector<ScheduledSystolicTile>>
scheduleAssignedSystolicTasks(
    llvm::ArrayRef<SystolicExecutionTask> tasks,
    llvm::ArrayRef<int64_t> computeCycles,
    llvm::ArrayRef<SystolicArrayResource> fleet);

} // namespace systolic
} // namespace mlir

#endif // SYSTOLIC_SCHEDULING_H
