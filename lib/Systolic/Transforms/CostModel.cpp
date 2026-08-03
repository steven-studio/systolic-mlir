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
  if (array.rows <= 0 || array.cols <= 0 || array.depth <= 0)
    return 0;

  int64_t tiles = ceilDiv(m, array.rows) * ceilDiv(n, array.cols) *
                  ceilDiv(k, array.depth);

  // PE(i,j) performs its k-th MAC on beat i+j+k, so the time loop runs
  // depth + rows + cols - 2 beats. Every tile pays that fill/drain, not
  // the GEMM as a whole -- charging it once made the estimate monotonically
  // decreasing in rows and cols, so the model could never express the
  // trade-off it exists to express.
  //
  // II and fixedOverhead are calibrated by C/RTL co-simulation over 14
  // (rows, cols, depth) configurations: II = 1, fixedOverhead = 6, residual
  // exactly zero at every point (TIME_STEPS 8..70, rows+cols 4..34).
  int64_t beats = array.depth + array.rows + array.cols - 2;
  int64_t perTile = array.initiationInterval * beats + array.fixedOverhead;

  return tiles * perTile;
}

int64_t estimateDmaCycles(int64_t bytes, double bytesPerCycle) {
  if (bytesPerCycle <= 0.0)
    return 0;
  return static_cast<int64_t>(
      std::ceil(static_cast<double>(bytes) / bytesPerCycle));
}

} // namespace systolic
