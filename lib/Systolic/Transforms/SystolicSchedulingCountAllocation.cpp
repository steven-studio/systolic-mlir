#include "Systolic/SystolicSchedulingDP.h"

#include "llvm/ADT/SmallVector.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <vector>

namespace mlir {
namespace systolic {

namespace {

const SystolicGeometryCostParams *
findCountAllocationCostParamsForGeometry(
    int64_t arraySize,
    llvm::ArrayRef<SystolicGeometryCostParams> costParams) {

  for (const auto &entry : costParams) {
    if (entry.arraySize == arraySize)
      return &entry;
  }

  return nullptr;
}

} // namespace

} // namespace systolic
} // namespace mlir
