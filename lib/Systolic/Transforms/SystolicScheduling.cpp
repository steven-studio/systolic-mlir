#include "Systolic/SystolicScheduling.h"

#include <cstdint>

namespace mlir {
namespace systolic {

namespace {

const SystolicArrayResource *
findPhysicalResource(
    int64_t arraySize,
    int64_t acceleratorId,
    llvm::ArrayRef<SystolicArrayResource> fleet) {

  for (const SystolicArrayResource &resource : fleet) {
    if (resource.arraySize == arraySize &&
        resource.acceleratorId == acceleratorId)
      return &resource;
  }

  return nullptr;
}

} // namespace


FailureOr<llvm::SmallVector<ScheduledSystolicTile>>
scheduleAssignedSystolicTasks(
    llvm::ArrayRef<SystolicExecutionTask> tasks,
    llvm::ArrayRef<int64_t> computeCycles,
    llvm::ArrayRef<SystolicArrayResource> fleet) {

  // ------------------------------------------------------------
  // Basic input validation.
  // ------------------------------------------------------------

  if (tasks.size() != computeCycles.size())
    return failure();

  if (fleet.empty() && !tasks.empty())
    return failure();

  // One running clock per physical accelerator.
  //
  // load[i] = first cycle at which accelerator i becomes free.
  llvm::SmallVector<int64_t> load(fleet.size(), 0);

  llvm::SmallVector<ScheduledSystolicTile> result;
  result.reserve(tasks.size());

  // ------------------------------------------------------------
  // Schedule each already-assigned task.
  //
  // IMPORTANT:
  //
  // The accelerator assignment is already fixed in
  // task.acceleratorId.  This function does NOT change it.
  //
  // Therefore there is no resource-selection step here.
  //
  // Since all tasks are available at cycle 0 and tasks sharing
  // one accelerator execute sequentially, the order among tasks
  // assigned to the same accelerator does not change that
  // accelerator's final load.  We preserve input order to keep
  // the result deterministic.
  // ------------------------------------------------------------

  for (size_t i = 0; i < tasks.size(); ++i) {
    const SystolicExecutionTask &task = tasks[i];
    const int64_t cycles = computeCycles[i];

    if (cycles < 0)
      return failure();

    if (task.size <= 0 ||
        task.acceleratorSize <= 0 ||
        task.acceleratorId < 0)
      return failure();

    const SystolicArrayResource *resource =
        findPhysicalResource(
            task.acceleratorSize,
            task.acceleratorId,
            fleet);

    if (!resource)
      return failure();

    // A physical accelerator must be large enough for the task.
    if (resource->arraySize < task.size)
      return failure();

    // Find the physical resource index.
    size_t resourceIndex = 0;
    while (resourceIndex < fleet.size() &&
           !(fleet[resourceIndex].arraySize ==
                 resource->arraySize &&
             fleet[resourceIndex].acceleratorId ==
                 resource->acceleratorId)) {
      ++resourceIndex;
    }

    if (resourceIndex == fleet.size())
      return failure();

    const int64_t startCycle = load[resourceIndex];
    const int64_t endCycle = startCycle + cycles;

    SystolicTile tile{
        task.row,
        task.column,
        task.size,
        task.acceleratorSize,
        task.acceleratorId
    };

    result.push_back(
        ScheduledSystolicTile{
            tile,
            startCycle,
            endCycle
        });

    load[resourceIndex] = endCycle;
  }

  return result;
}

} // namespace systolic
} // namespace mlir
