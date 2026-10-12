#ifndef SYSTOLIC_TILING_H
#define SYSTOLIC_TILING_H

// The data the decomposition and scheduling code exchanges: the physical
// fleet, spatial tiles, and their schedule. The binary-search optimizer
// (SystolicOptimizer.h) produces them; --systolic-binary-search-tile-matmul
// and --systolic-materialize-schedule consume them.

#include "Systolic/CostModel.h"

#include <cstdint>

namespace mlir {
namespace systolic {

/// A physical systolic-array resource in the hardware fleet.
///
/// This describes actual hardware availability, not tile decomposition.
///
/// Example:
///   {8, 1} means one physical 8x8 systolic array.
///   {4, 3} means three physical 4x4 systolic arrays.
struct SystolicArrayResource {
  // Physical accelerator geometry.
  int64_t arraySize;

  // Unique ID within this geometry.
  //
  // Example:
  //   8x8 #1 -> arraySize = 8, acceleratorId = 1
  //   8x8 #2 -> arraySize = 8, acceleratorId = 2
  int64_t acceleratorId;

  // Everything the cost model knows about this accelerator: k_max, H, the
  // read and write terms, how an invocation's depth is executed
  // (CostModel.h). The binary-search optimizer prices a spatial tile on it
  // with ::systolic::estimateTileCost(K, config) -- it keeps no formula of
  // its own. config.rows and config.cols must equal arraySize.
  ::systolic::ArrayConfig config;
};

/// A square tile assigned to one systolic array.
///
/// Coordinates are expressed in the original (unpadded) output matrix.
struct SystolicTile {
  int64_t row;
  int64_t column;
  int64_t size;

  // Physical accelerator assigned to this tile.
  int64_t acceleratorSize;
  int64_t acceleratorId;
};

/// A tile together with its scheduled execution interval.
///
/// The binary-search optimizer fills startCycle / endCycle on its own
/// timeline: a tile occupies its accelerator for every invocation's read,
/// compute and write (CostModel.h); scheduleAssignedSystolicTasks fills them
/// from the compute cycles it is given.
struct ScheduledSystolicTile {
  SystolicTile tile;

  int64_t startCycle;
  int64_t endCycle;
};

/// One compute task generated from a spatial tile decomposition.
///
/// This represents an execution unit, not a physical accelerator
/// assignment.  acceleratorId remains unset until scheduling.
struct SystolicExecutionTask {
  int64_t row;
  int64_t column;
  int64_t size;

  /// Physical accelerator geometry required by this task.
  int64_t acceleratorSize;

  /// Physical accelerator instance assigned by the scheduler.
  ///
  /// -1 means not assigned yet.
  int64_t acceleratorId;
};

} // namespace systolic
} // namespace mlir

#endif // SYSTOLIC_TILING_H
