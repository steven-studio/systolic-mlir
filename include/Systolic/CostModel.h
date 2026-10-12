#ifndef SYSTOLIC_COSTMODEL_H
#define SYSTOLIC_COSTMODEL_H

// The analytical cost model -- the one place a cycle count is computed.
// Every pass that needs a cost (--systolic-binary-search-tile-matmul through
// SystolicOptimizer, --systolic-select-device, --systolic-cost-analysis,
// --systolic-schedule-overlap) and the systolic-cost tool call the functions
// below; none of them keeps a formula of its own. docs/COSTMODEL.md states
// the formulas, where each constant comes from, and how they were validated.
//
// Variable-K execution. K_MAX (kMax) is the operand buffer's capacity, not
// the depth every invocation runs: a K-step reduction is split into
// I = ceil(K / K_MAX) invocations, invocation i running exactly
//
//   k_i = min(K_MAX, K - i * K_MAX)       steps,  sum_i k_i = K,
//
// and one invocation -- one run of the array on one operand slab -- costs
//
//   T_read    = ceil(W_read(k_i) / beta_read) + c_read      operand fill
//   T_compute = k_i + rows + cols - 2 + H                    fold
//   T_write   = ceil(W_write / beta_write) + c_write         result write-back
//   T_seq     = T_read + T_compute + T_write
//
// so a spatial tile's compute is K + I * (rows + cols - 2 + H). A device
// that declares none of the transport parameters has T_read = T_write = 0:
// the compute-only model every pass used before the transport terms
// existed, to the cycle.

#include <cstdint>

namespace systolic {

struct ArrayConfig {
  int64_t rows = 0;
  int64_t cols = 0;
  int64_t kMax = 0;               // reduction capacity per invocation;
                                  // 0 = not declared = the whole reduction
                                  // is one invocation (what
                                  // --systolic-binary-search-tile-matmul
                                  // does for such a device)
  int64_t l1Bytes = 0;            // local scratchpad capacity, bytes;
                                  // 0 = not declared = unconstrained
  // Calibration constant. It describes the datapath, not the array shape,
  // and must be measured per microarchitecture -- two arrays of identical
  // geometry can differ here by more than an order of magnitude.
  //
  // tileOverhead has no default: a device that does not declare
  // `tile_overhead` is costed with H = 0, which is exactly the geometric
  // model. The HLS pipeline the model was first fitted to measures 6
  // (C/RTL cosim, 14 configurations, residual zero); the hand-written fold
  // RTL on xc7a200t (tag paper-hw-v1) measures 95 on the board with zero
  // residual, see test/Systolic/cost_model_fold_rtl.mlir. Both are stated
  // on the device that was measured, not defaulted here.
  int64_t tileOverhead = 0;
  double clockHz = 0.0;
  // Bandwidth of explicit systolic.dma ops and of --systolic-schedule-
  // overlap's separate per-tile prefetch. Not used by the invocation model
  // below, which has its own read and write bandwidths, so the two can never
  // be charged for the same transfer.
  double dmaBytesPerCycle = 0.0;

  // ---- one invocation's operand fill and result write-back ----------------
  // All left at their defaults: T_read = T_write = 0 (compute-only model).
  double readBytesPerCycle = 0.0;  // beta_read, one invocation's own fill;
                                   // 0 = no bandwidth term
  double writeBytesPerCycle = 0.0; // beta_write; 0 = no bandwidth term
  int64_t readOverhead = 0;        // c_read, fixed cycles per fill
  int64_t writeOverhead = 0;       // c_write, fixed cycles per write-back
  // Lanes one operand occupies per reduction step on the wire; 0 = rows
  // for A and cols for B, i.e. exactly the operand. The FPGA's slab is 8
  // lanes wide for every geometry: a 4x4 moves 8 lanes and keeps 4.
  int64_t operandLanes = 0;
  // Reduction steps per transfer window: a fill moves whole windows, so
  // ceil(k_i / kWindow) * kWindow steps are on the wire (FPGA: 8; the
  // padding steps of the last window are moved but never fed).
  int64_t kWindow = 1;
  int64_t elementBytes = 4;        // binary32, the only type lowered
};

enum class Dataflow { WeightStationary, OutputStationary, RowStationary };

/// True if every field is in range: rows, cols, kWindow, elementBytes >= 1;
/// kMax, l1Bytes, tileOverhead, the overheads and operandLanes >= 0;
/// bandwidths finite and >= 0; operandLanes, when set, >= max(rows, cols).
bool isValidArrayConfig(const ArrayConfig &array);

/// Invocations a k-step reduction takes: ceil(k / kMax), 1 for a device
/// without kMax, 0 for k <= 0 or an invalid config.
int64_t countInvocations(int64_t k, const ArrayConfig &array);

/// Reduction steps invocation j (0-based) of a k-step reduction runs:
/// min(kMax, k - j * kMax); k itself without kMax. 0 if j is out of range.
int64_t invocationDepth(int64_t k, int64_t j, const ArrayConfig &array);

/// Bytes the operand fill of an invocation of depth k moves: both operands,
/// whole windows, the wire's lanes.
///   operandLanes set:  2 * lanes * ceil(k / kWindow) * kWindow * elementBytes
///   otherwise:   (rows + cols) * ceil(k / kWindow) * kWindow * elementBytes
int64_t operandReadBytes(int64_t k, const ArrayConfig &array);

/// Bytes the write-back of one invocation moves: the whole rows x cols
/// result tile.
int64_t resultWriteBytes(const ArrayConfig &array);

/// ceil(bytes / bytesPerCycle), robust to the representation error of a
/// decimal bandwidth (256 / 3.2 is 80 cycles, not 81). 0 for bytes <= 0 or
/// bytesPerCycle <= 0.
int64_t transferCycles(int64_t bytes, double bytesPerCycle);

/// One invocation, its three phases.
struct InvocationCost {
  bool ok = false;          // false: invalid config, depth or overflow
  int64_t k = 0;            // reduction steps it runs (its share of K)
  int64_t readBytes = 0;
  int64_t writeBytes = 0;
  int64_t read = 0;         // T_read
  int64_t compute = 0;      // T_compute
  int64_t write = 0;        // T_write
  int64_t sequential() const { return read + compute + write; }
};

/// One invocation of depth k, 1 <= k <= kMax (any k >= 1 without kMax).
InvocationCost estimateInvocationCost(int64_t k, const ArrayConfig &array);

/// Invocation j (0-based) of a K-step reduction: depth invocationDepth(K, j).
InvocationCost estimateInvocationCost(int64_t reduction, int64_t j,
                                      const ArrayConfig &array);

/// One rows x cols output tile over a k-step reduction: the sum of its
/// countInvocations(k) invocations. They run back to back with nothing
/// overlapped -- the RTL takes a context's next job only once the previous
/// result is written back -- so T_tile = sum over j of T_seq(j).
struct TileCost {
  bool ok = false;
  int64_t invocations = 0;
  int64_t read = 0;
  int64_t compute = 0;
  int64_t write = 0;
  int64_t readBytes = 0;
  int64_t writeBytes = 0;
  int64_t sequential() const { return read + compute + write; }
};

TileCost estimateTileCost(int64_t k, const ArrayConfig &array);

/// Compute-only cycles of an m x n x k matmul on one array: the number of
/// rows x cols output tiles times one tile's compute term,
///   ceil(m / rows) * ceil(n / cols) * (k + I * (rows + cols - 2 + H)).
/// What est_cycles carries in --systolic-select-device / -cost-analysis /
/// -schedule-overlap; the read and write terms are not in it (schedule-
/// overlap adds its own prefetch). 0 for an invalid config; saturates at
/// INT64_MAX on overflow.
int64_t estimateMatmulCycles(int64_t m, int64_t n, int64_t k,
                             const ArrayConfig &array,
                             Dataflow dataflow = Dataflow::WeightStationary);

/// Cycles to move `bytes` at `bytesPerCycle` (transferCycles).
int64_t estimateDmaCycles(int64_t bytes, double bytesPerCycle);

} // namespace systolic

#endif // SYSTOLIC_COSTMODEL_H
