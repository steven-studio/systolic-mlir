#ifndef SYSTOLIC_SYSTOLICSCHEDULINGDP_H
#define SYSTOLIC_SYSTOLICSCHEDULINGDP_H

#include "Systolic/SystolicComputeCost.h"
#include "Systolic/SystolicTiling.h"

#include "llvm/ADT/ArrayRef.h"
#include "mlir/Support/LogicalResult.h"

#include <cstdint>

namespace mlir {
namespace systolic {

/// Compute the exact minimum makespan using a count-based dynamic program.
///
/// Logical tasks with the same geometry are treated as indistinguishable.
/// A DP state consists of:
///
///   - remaining task counts by logical geometry;
///   - accumulated load of every physical accelerator.
///
/// This removes permutations among identical logical tasks while preserving
/// assignment-dependent costs and physical-resource identity.
///
/// This first version returns only the minimum makespan.  Schedule
/// reconstruction can be added later with predecessor information.
FailureOr<int64_t>
minimizeSystolicMakeSpanDP(
    llvm::ArrayRef<SystolicExecutionTask> tasks,
    int64_t K,
    llvm::ArrayRef<SystolicArrayResource> fleet,
    llvm::ArrayRef<SystolicGeometryCostParams> costParams);


/// Compute the exact minimum makespan by enumerating task-count
/// allocations rather than task-by-task assignment histories.
///
/// For each logical geometry g and physical resource r, the search
/// chooses an integer x[g][r] giving the number of tasks of geometry
/// g assigned to resource r.
///
/// Tasks of each logical geometry must be assigned exactly once:
///
///   sum_r x[g][r] = count[g]
///
/// and the objective is to minimize the maximum accumulated physical
/// resource load.
///
/// This formulation preserves exact assignment-aware costs while
/// avoiding permutations of individual task assignments.
FailureOr<int64_t>
minimizeSystolicMakeSpanCountAllocation(
    llvm::ArrayRef<SystolicExecutionTask> tasks,
    int64_t K,
    llvm::ArrayRef<SystolicArrayResource> fleet,
    llvm::ArrayRef<SystolicGeometryCostParams> costParams);

} // namespace systolic
} // namespace mlir

#endif // SYSTOLIC_SYSTOLICSCHEDULINGDP_H
