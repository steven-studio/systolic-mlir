#include "Systolic/CostModel.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace systolic {

namespace {

constexpr int64_t kInt64Max = std::numeric_limits<int64_t>::max();

bool mul(int64_t a, int64_t b, int64_t &r) {
  return !__builtin_mul_overflow(a, b, &r);
}

bool add(int64_t a, int64_t b, int64_t &r) {
  return !__builtin_add_overflow(a, b, &r);
}

int64_t ceilDiv(int64_t a, int64_t b) {
  // a >= 0, b > 0 at every call site.
  return a / b + (a % b != 0 ? 1 : 0);
}

bool finiteNonNegative(double v) { return std::isfinite(v) && v >= 0.0; }

// Bytes of an operand fill of depth k; false on overflow.
bool readBytesOf(int64_t k, const ArrayConfig &a, int64_t &bytes) {
  const int64_t steps = ceilDiv(k, a.kWindow);
  int64_t onWire = 0;
  if (!mul(steps, a.kWindow, onWire))
    return false;
  int64_t lanes = 0;
  if (a.operandLanes > 0) {
    if (!mul(2, a.operandLanes, lanes))
      return false;
  } else if (!add(a.rows, a.cols, lanes)) {
    return false;
  }
  int64_t words = 0;
  return mul(lanes, onWire, words) && mul(words, a.elementBytes, bytes);
}

bool writeBytesOf(const ArrayConfig &a, int64_t &bytes) {
  int64_t words = 0;
  return mul(a.rows, a.cols, words) && mul(words, a.elementBytes, bytes);
}

// sum = count * part + last, every term checked.
bool scaleAdd(int64_t count, int64_t part, int64_t last, int64_t &sum) {
  int64_t scaled = 0;
  return mul(count, part, scaled) && add(scaled, last, sum);
}

} // namespace

bool isValidArrayConfig(const ArrayConfig &a) {
  if (a.rows < 1 || a.cols < 1 || a.kMax < 0 || a.l1Bytes < 0 ||
      a.tileOverhead < 0 || a.readOverhead < 0 || a.writeOverhead < 0 ||
      a.operandLanes < 0 || a.kWindow < 1 || a.elementBytes < 1)
    return false;
  if (a.operandLanes > 0 && a.operandLanes < std::max(a.rows, a.cols))
    return false;
  if (!finiteNonNegative(a.readBytesPerCycle) ||
      !finiteNonNegative(a.writeBytesPerCycle))
    return false;
  return true;
}

int64_t countInvocations(int64_t k, const ArrayConfig &a) {
  if (k <= 0 || !isValidArrayConfig(a))
    return 0;
  return a.kMax > 0 ? ceilDiv(k, a.kMax) : 1;
}

int64_t invocationDepth(int64_t k, int64_t j, const ArrayConfig &a) {
  const int64_t invocations = countInvocations(k, a);
  if (j < 0 || j >= invocations)
    return 0;
  if (a.kMax == 0)
    return k;
  // j < ceil(k / kMax), so j * kMax < k: no overflow, depth >= 1.
  return std::min(a.kMax, k - j * a.kMax);
}

int64_t operandReadBytes(int64_t k, const ArrayConfig &a) {
  int64_t bytes = 0;
  if (k <= 0 || !isValidArrayConfig(a) || !readBytesOf(k, a, bytes))
    return 0;
  return bytes;
}

int64_t resultWriteBytes(const ArrayConfig &a) {
  int64_t bytes = 0;
  if (!isValidArrayConfig(a) || !writeBytesOf(a, bytes))
    return 0;
  return bytes;
}

int64_t transferCycles(int64_t bytes, double bytesPerCycle) {
  if (bytes <= 0 || !std::isfinite(bytesPerCycle) || bytesPerCycle <= 0.0)
    return 0;
  const double q = static_cast<double>(bytes) / bytesPerCycle;
  if (!(q < 9.0e18))
    return kInt64Max;
  // A bandwidth written in decimal is rarely exact in binary: 3.2 is
  // 3.2000000000000002, and 256 / 3.2 must still be 80 cycles. A quotient
  // within a part in 10^9 of an integer is that integer.
  const double nearest = std::nearbyint(q);
  if (std::fabs(q - nearest) <= 1e-9 * std::max(1.0, q))
    return static_cast<int64_t>(nearest);
  return static_cast<int64_t>(std::ceil(q));
}

InvocationCost estimateInvocationCost(int64_t k, const ArrayConfig &a) {
  InvocationCost c;
  if (k < 1 || !isValidArrayConfig(a) || (a.kMax > 0 && k > a.kMax))
    return c;

  // The invocation runs exactly k steps, whatever k_max is: k_max is the
  // operand buffer's capacity, not the depth of every invocation.
  c.k = k;
  if (!readBytesOf(c.k, a, c.readBytes) || !writeBytesOf(a, c.writeBytes))
    return c;

  // Operand fill: the whole windows the k steps occupy, at the device's own
  // fill rate, plus a fixed cost (descriptor, first-beat latency, the
  // hand-off to the fold).
  const int64_t readTransfer = transferCycles(c.readBytes, a.readBytesPerCycle);
  if (!add(readTransfer, a.readOverhead, c.read))
    return c;

  // Fold. PE(i,j) performs its k-th MAC on beat i+j+k, so an invocation of
  // depth k runs k + rows + cols - 2 beats; every invocation pays
  // that fill/drain, not the GEMM as a whole -- charging it once made the
  // estimate monotonically decreasing in rows and cols, so the model could
  // never express the trade-off it exists to express.
  //
  // tileOverhead (H) is a calibration constant, and which value is correct
  // depends on the microarchitecture rather than on the geometry:
  //
  //   HLS pipeline    tileOverhead =  6
  //     C/RTL cosim over 14 (rows, cols, k_max) configurations, residual
  //     exactly zero at every point (TIME_STEPS 8..70, rows+cols 4..34).
  //
  //   fold RTL 8x8    tileOverhead = 95   (tag paper-hw-v1)
  //     xc7a200t. Board measurement at k = 16, 64 and 128 gives 125, 173
  //     and 237 cycles; subtracting k and the geometric term 8 + 8 - 2
  //     leaves 95 at every point, and the held-out depths 8, 32, 96, 192
  //     and 256, every other k_max, every K > k_max and the whole N = 4
  //     sweep reproduce to the cycle. 95 = 22 drain + 67 tree + 6 hand-off
  //     (rtl/multi_200t/fold_pipelined/tb/tb_array_h_decomp.sv).
  //
  // The two differ by more than 15x on the same formula, which is why the
  // constant lives on the device op and not here. There is no initiation
  // interval: the array sustains one product per PE per cycle by
  // construction, so the arithmetic term is exactly k.
  int64_t geometric = 0;
  if (!add(a.rows, a.cols, geometric) || !add(geometric, -2, geometric) ||
      !add(geometric, a.tileOverhead, geometric) ||
      !add(c.k, geometric, c.compute))
    return c;

  // Write-back of the whole rows x cols result tile.
  const int64_t writeTransfer =
      transferCycles(c.writeBytes, a.writeBytesPerCycle);
  if (!add(writeTransfer, a.writeOverhead, c.write))
    return c;

  int64_t total = 0;
  if (!add(c.read, c.compute, total) || !add(total, c.write, total))
    return c;

  c.ok = true;
  return c;
}

InvocationCost estimateInvocationCost(int64_t reduction, int64_t j,
                                      const ArrayConfig &a) {
  const int64_t depth = invocationDepth(reduction, j, a);
  if (depth < 1)
    return InvocationCost{};
  return estimateInvocationCost(depth, a);
}

TileCost estimateTileCost(int64_t k, const ArrayConfig &a) {
  TileCost t;
  const int64_t invocations = countInvocations(k, a);
  if (invocations < 1)
    return t;

  // Every invocation but the last is a full k_max one; the last runs the
  // remainder. Closed form, so a deep reduction on a shallow device costs
  // O(1), not O(invocations).
  const int64_t lastDepth = invocationDepth(k, invocations - 1, a);
  const InvocationCost last = estimateInvocationCost(lastDepth, a);
  if (!last.ok)
    return t;

  InvocationCost full;
  const int64_t fullCount = invocations - 1;
  if (fullCount > 0) {
    full = estimateInvocationCost(a.kMax, a);
    if (!full.ok)
      return t;
  }

  if (!scaleAdd(fullCount, full.read, last.read, t.read) ||
      !scaleAdd(fullCount, full.compute, last.compute, t.compute) ||
      !scaleAdd(fullCount, full.write, last.write, t.write) ||
      !scaleAdd(fullCount, full.readBytes, last.readBytes, t.readBytes) ||
      !scaleAdd(fullCount, full.writeBytes, last.writeBytes, t.writeBytes))
    return t;

  int64_t total = 0;
  if (!add(t.read, t.compute, total) || !add(total, t.write, total))
    return t;

  t.invocations = invocations;
  t.ok = true;
  return t;
}

int64_t estimateMatmulCycles(int64_t m, int64_t n, int64_t k,
                             const ArrayConfig &array, Dataflow /*dataflow*/) {
  // kMax < 0 cannot come from IR (DeviceOp's verifier rejects k_max < 1);
  // kMax == 0 means the device declares no reduction capacity.
  if (!isValidArrayConfig(array) || m <= 0 || n <= 0 || k <= 0)
    return 0;

  // One fold is one rows x cols output tile. The reduction is split into
  // ceil(k / k_max) invocations; the last one runs at whatever depth is
  // left, not at the buffer's capacity, so the arithmetic term sums to
  // exactly k and only the per-invocation costs recur. The capacity is the device's own: two accelerators in one fleet
  // may declare different k_max (K_MAX_8X8 and K_MAX_4X4 are independent
  // synthesis parameters), and nothing here assumes a power of two. A device
  // without k_max keeps the reduction whole (one invocation).
  const TileCost tile = estimateTileCost(k, array);
  if (!tile.ok)
    return kInt64Max;

  int64_t folds = 0;
  int64_t cycles = 0;
  if (!mul(ceilDiv(m, array.rows), ceilDiv(n, array.cols), folds) ||
      !mul(folds, tile.compute, cycles))
    return kInt64Max;
  return cycles;
}

int64_t estimateDmaCycles(int64_t bytes, double bytesPerCycle) {
  return transferCycles(bytes, bytesPerCycle);
}

} // namespace systolic
