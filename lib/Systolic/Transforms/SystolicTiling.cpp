#include "Systolic/SystolicTiling.h"

#include "llvm/ADT/STLExtras.h"

namespace mlir {
namespace systolic {

bool isPowerOfTwo(int64_t x) {
  return x > 0 && (x & (x - 1)) == 0;
}

/// Validate the GEMM input shape for the current tiling scope.
///
/// Current scope:
///   - input must be square: M == N
///   - the dimension must be a positive power of two
LogicalResult validateTilingInput(
    int64_t M,
    int64_t N) {
  if (M != N)
    return failure();

  if (!isPowerOfTwo(M))
    return failure();

  return success();
}

LogicalResult validateSystolicFleet(
    llvm::ArrayRef<SystolicArrayResource> fleet) {
  if (fleet.empty())
    return failure();

  for (const SystolicArrayResource &resource : fleet) {
    // Current scope only supports square power-of-two arrays.
    if (!isPowerOfTwo(resource.arraySize))
      return failure();
  }

  return success();
}

SystolicFleetState createSystolicFleetState(
    llvm::ArrayRef<int64_t> arraySizes,
    llvm::ArrayRef<int64_t> counts) {
  SystolicFleetState state;

  if (arraySizes.size() != counts.size())
    return state;

  for (size_t i = 0; i < arraySizes.size(); ++i) {
    int64_t arraySize = arraySizes[i];
    int64_t count = counts[i];

    if (!isPowerOfTwo(arraySize) || count <= 0)
      continue;

    state.availableCount[arraySize] = count;

    for (int64_t id = 1; id <= count; ++id) {
      state.resources.push_back(
          SystolicArrayResource{
              arraySize,
              id,
              true});
    }
  }

  return state;
}

llvm::SmallVector<SystolicArrayResource>
sortSystolicFleetDescending(
    llvm::ArrayRef<SystolicArrayResource> fleet) {
  llvm::SmallVector<SystolicArrayResource> sorted(
      fleet.begin(), fleet.end());

  llvm::sort(
      sorted,
      [](const SystolicArrayResource &lhs,
         const SystolicArrayResource &rhs) {
        return lhs.arraySize > rhs.arraySize;
      });

  return sorted;
}

FailureOr<SystolicArrayResource> selectLargestAvailableArray(
    SystolicFleetState &state,
    int64_t remainingRows,
    int64_t remainingColumns) {
  SystolicArrayResource *selected = nullptr;

  for (SystolicArrayResource &resource : state.resources) {
    if (!resource.available)
      continue;

    if (resource.arraySize > remainingRows ||
        resource.arraySize > remainingColumns)
      continue;

    if (!selected ||
        resource.arraySize > selected->arraySize) {
      selected = &resource;
    }
  }

  if (!selected)
    return failure();

  auto it = state.availableCount.find(
      selected->arraySize);

  if (it == state.availableCount.end() ||
      it->second <= 0)
    return failure();

  // Commit the allocation only after both physical-resource
  // availability and geometry-level availability are verified.
  selected->available = false;
  --it->second;

  return *selected;
}

FailureOr<SystolicTile> createSystolicTile(
    SystolicFleetState &state,
    int64_t row,
    int64_t column,
    int64_t remainingRows,
    int64_t remainingColumns) {
  FailureOr<SystolicArrayResource> resource =
      selectLargestAvailableArray(
          state,
          remainingRows,
          remainingColumns);

  if (failed(resource))
    return failure();

  SystolicTile tile;
  tile.row = row;
  tile.column = column;
  tile.size = resource->arraySize;
  tile.acceleratorSize = resource->arraySize;
  tile.acceleratorId = resource->acceleratorId;

  return tile;
}

} // namespace systolic
} // namespace mlir
