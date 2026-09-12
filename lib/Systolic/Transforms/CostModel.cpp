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
  if (array.rows <= 0 || array.cols <= 0 || array.kMax <= 0)
    return 0;

  // One fold is one rows x cols output tile. The reduction is split into
  // ceil(k / k_max) invocations, and -- this is the part a capacity-only
  // count gets wrong -- the last invocation runs at whatever depth is left,
  // not at the buffer's capacity. Summing the true depths gives exactly k,
  // so the arithmetic term is k per fold regardless of how it is split, and
  // only the per-invocation costs recur.
  int64_t folds = ceilDiv(m, array.rows) * ceilDiv(n, array.cols);
  int64_t invocations = ceilDiv(k, array.kMax);

  // PE(i,j) performs its k-th MAC on beat i+j+k, so one invocation of depth
  // k_i runs k_i + rows + cols - 2 beats. Every tile pays that fill/drain, not
  // the GEMM as a whole -- charging it once made the estimate monotonically
  // decreasing in rows and cols, so the model could never express the
  // trade-off it exists to express.
  //
  // II and tileOverhead are calibration constants, and which values are
  // correct depends on the microarchitecture rather than on the geometry
  // this function computes.
  //
  //   HLS pipeline    II = 1, tileOverhead =   6
  //     C/RTL cosim over 14 (rows, cols, k_max) configurations, residual
  //     exactly zero at every point (TIME_STEPS 8..70, rows+cols 4..34).
  //
  //   fold RTL 8x8    II = 1, tileOverhead = 104
  //     xc7a200t at 100 MHz. Silicon measurement at k_dim = 16 and 64
  //     gives 134 and 182 cycles; subtracting the geometric term
  //     (k_dim + 8 + 8 - 2) leaves 104 at both points.
  //
  // The two differ by more than 17x on the same formula, which is why the
  // constants live on the device op and not here.
  int64_t perInvocation =
      array.initiationInterval * (array.rows + array.cols - 2) +
      array.tileOverhead;
  int64_t perFold = array.initiationInterval * k + invocations * perInvocation;

  return folds * perFold;
}

int64_t estimateDmaCycles(int64_t bytes, double bytesPerCycle) {
  if (bytesPerCycle <= 0.0)
    return 0;
  return static_cast<int64_t>(
      std::ceil(static_cast<double>(bytes) / bytesPerCycle));
}

} // namespace systolic
