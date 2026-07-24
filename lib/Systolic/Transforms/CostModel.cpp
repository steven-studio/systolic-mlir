#include "Systolic/CostModel.h"

#include <algorithm>
#include <cmath>

namespace systolic {

static int64_t ceilDiv(int64_t a, int64_t b) {
  if (b <= 0)
    return 0;
  return (a + b - 1) / b;
}

int64_t estimateMatmulCycles(int64_t m, int64_t n, int64_t k,
                              const ArrayConfig &array, Dataflow /*dataflow*/) {
  if (array.rows <= 0 || array.cols <= 0)
    return 0;
  int64_t tiles = ceilDiv(m, array.rows) * ceilDiv(n, array.cols) *
                  ceilDiv(k, array.depth);
  int64_t fillDrain = std::max<int64_t>(0, array.rows + array.cols - 2);
  return tiles + fillDrain;
}

int64_t estimateDmaCycles(int64_t bytes, double bytesPerCycle) {
  if (bytesPerCycle <= 0.0)
    return 0;
  return static_cast<int64_t>(
      std::ceil(static_cast<double>(bytes) / bytesPerCycle));
}

} // namespace systolic
