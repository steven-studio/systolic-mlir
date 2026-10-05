#include "Systolic/SystolicSchedulingDP.h"

#include "llvm/ADT/SmallVector.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <utility>
#include <vector>

namespace mlir {
namespace systolic {

namespace {

struct DPState {
  std::vector<int64_t> remaining;
  std::vector<int64_t> loads;

  bool operator<(const DPState &other) const {
    if (remaining != other.remaining)
      return remaining < other.remaining;
    return loads < other.loads;
  }
};

const SystolicGeometryCostParams *
findCostParamsForGeometry(
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
