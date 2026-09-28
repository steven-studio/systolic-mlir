#ifndef SYSTOLIC_TILING_H
#define SYSTOLIC_TILING_H

#include "mlir/Support/LogicalResult.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"

#include <cstdint>

namespace mlir {
namespace systolic {

/// Returns true iff x is a positive power of two.
bool isPowerOfTwo(int64_t x);

/// Validate the GEMM input shape for the current tiling scope.
///
/// Current scope:
///   - input must be square: M == N
///   - M and N must be positive powers of two
///
/// Examples:
///   128 x 128 -> valid
///   64 x 64   -> valid
///   100 x 100 -> invalid
///   128 x 64  -> invalid
LogicalResult validateTilingInput(
    int64_t M,
    int64_t N);

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

  // Whether this physical accelerator is currently available.
  bool available;
};

/// Runtime state of the physical systolic-array fleet.
///
/// `resources` tracks every physical accelerator instance.
///
/// `availableCount[size]` tracks how many accelerators of a
/// particular geometry are still available.
///
/// Example:
///   8x8: two physical accelerators
///   4x4: three physical accelerators
///
/// Initial state:
///   availableCount[8] = 2
///   availableCount[4] = 3
struct SystolicFleetState {
  llvm::SmallVector<SystolicArrayResource> resources;
  llvm::DenseMap<int64_t, int64_t> availableCount;
};

/// Validate a physical systolic-array fleet.
///
/// Current tiling scope:
///   - every array is square: a x a
///   - every array dimension is a positive power of two
///   - every resource count is positive
///   - each geometry appears at most once
LogicalResult validateSystolicFleet(
    llvm::ArrayRef<SystolicArrayResource> fleet);

/// A square tile assigned to one systolic array.
///
/// Coordinates are expressed in the original square input matrix.
struct SystolicTile {
  int64_t row;
  int64_t column;
  int64_t size;

  // Physical accelerator assigned to this tile.
  int64_t acceleratorSize;
  int64_t acceleratorId;
};

/// Return physical hardware resources sorted from largest to
/// smallest array geometry.
///
/// The resource count is preserved during sorting.
///
/// Example:
///   {{8, 1}, {128, 1}, {4, 3}}
///       -> {{128, 1}, {8, 1}, {4, 3}}
llvm::SmallVector<SystolicArrayResource>
sortSystolicFleetDescending(
    llvm::ArrayRef<SystolicArrayResource> fleet);

/// Create physical accelerator instances from per-geometry
/// resource counts.
///
/// Example:
///   arraySizes = {8, 4}
///   counts     = {2, 3}
///
/// creates:
///   8x8 #1, 8x8 #2
///   4x4 #1, 4x4 #2, 4x4 #3
SystolicFleetState createSystolicFleetState(
    llvm::ArrayRef<int64_t> arraySizes,
    llvm::ArrayRef<int64_t> counts);

/// Select the largest fleet array that fits inside the
/// remaining rectangular region.
///
/// Returns 0 if no fleet array fits.
FailureOr<SystolicArrayResource> selectLargestAvailableArray(
    SystolicFleetState &state,
    int64_t remainingRows,
    int64_t remainingColumns);

/// Create one square tile at the given position.
///
/// Returns failure if no fleet array fits in the remaining region.
FailureOr<SystolicTile> createSystolicTile(
    SystolicFleetState &state,
    int64_t row,
    int64_t column,
    int64_t remainingRows,
    int64_t remainingColumns);

} // namespace systolic
} // namespace mlir

#endif // SYSTOLIC_TILING_H
