#include "CostModel.h"

#include <cassert>

namespace systolic {

namespace {
int64_t ceilDiv(int64_t a, int64_t b) {
  assert(b > 0 && "divisor must be positive");
  return (a + b - 1) / b;
}
} // namespace

int64_t estimateMatmulCycles(int64_t m, int64_t n, int64_t k,
                              const ArrayConfig &array,
                              Dataflow /*dataflow*/) {
  assert(array.rows > 0 && array.cols > 0 && array.depth > 0 &&
         "ArrayConfig must be fully populated before costing");

  // NOTE: dataflow is accepted but not yet used to change which GEMM
  // dimension maps to which array axis -- every dataflow currently uses
  // the same (rows -> M, cols -> N, depth -> K) mapping. Once
  // weight-stationary vs. output-stationary vs. row-stationary need
  // genuinely different formulas, branch on `dataflow` here. This is a
  // tracked simplification, not an oversight.
  int64_t passesM = ceilDiv(m, array.rows);
  int64_t passesN = ceilDiv(n, array.cols);
  int64_t passesK = ceilDiv(k, array.depth);

  int64_t fillDrain = array.rows + array.cols - 2;
  if (fillDrain < 0)
    fillDrain = 0;

  return passesM * passesN * passesK + fillDrain;
}

int64_t estimateDmaCycles(int64_t bytes, double bytesPerCycle) {
  if (bytesPerCycle <= 0.0)
    return 0;
  double cycles = static_cast<double>(bytes) / bytesPerCycle;
  auto whole = static_cast<int64_t>(cycles);
  return cycles > static_cast<double>(whole) ? whole + 1 : whole;
}

} // namespace systolic
