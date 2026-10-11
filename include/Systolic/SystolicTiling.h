#ifndef SYSTOLIC_TILING_H
#define SYSTOLIC_TILING_H

// The data the decomposition and scheduling code exchanges: the physical
// fleet, spatial tiles, and their schedule. The binary-search optimizer
// (SystolicOptimizer.h) produces them; --systolic-binary-search-tile-matmul
// and --systolic-materialize-schedule consume them.

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

  // Hardware-calibrated fixed implementation overhead per invocation.
  //
  // H = 0 means the pure geometric model.
  int64_t tileOverhead = 0;

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
/// startCycle and endCycle are compute-only cycles.
/// DMA is intentionally not represented here.
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
