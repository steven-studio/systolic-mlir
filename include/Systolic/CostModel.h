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

  // ---- consecutive invocations on one accelerator -------------------------
  // How the context runs invocation j+1 relative to invocation j (the
  // pipelined per-accelerator timeline below). The defaults are the
  // serialized hardware: one invocation in flight, the next one's fill
  // starts when the previous result tile is written back, T_tile = sum of
  // T_seq. The three come together (DeviceOp's verifier).
  int64_t pipelineDepth = 1;       // invocations a context holds at a time
                                   // (accepted, not yet written back)
  int64_t reduceCycles = 0;        // the serial part of tileOverhead: one
                                   // invocation's bank-reduction tree, which
                                   // the next invocation's cannot overlap
  int64_t feedGap = 0;             // idle cycles of the array input between
                                   // invocation j's last reduction step and
                                   // j+1's first (the PE's boundary gap)
};

/// True if consecutive invocations on this accelerator overlap
/// (pipelineDepth > 1); false for the serialized model.
bool isPipelined(const ArrayConfig &array);

enum class Dataflow { WeightStationary, OutputStationary, RowStationary };

/// True if every field is in range: rows, cols, kWindow, elementBytes,
/// pipelineDepth >= 1; kMax, l1Bytes, tileOverhead, the overheads,
/// operandLanes, reduceCycles and feedGap >= 0; reduceCycles <= tileOverhead;
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
/// countInvocations(k) invocations' terms. On the serialized hardware they
/// run back to back with nothing overlapped -- the RTL takes a context's
/// next job only once the previous result is written back -- so the tile
/// holds its accelerator for sequential() = sum over j of T_seq(j). On a
/// pipelined accelerator (isPipelined) the invocations overlap and the tile
/// holds it for span(): first fill start to last write end of the tile's
/// invocations run alone on an idle context (AcceleratorTimeline below), so
/// span() <= sequential(), with equality at pipelineDepth = 1.
struct TileCost {
  bool ok = false;
  int64_t invocations = 0;
  int64_t read = 0;
  int64_t compute = 0;
  int64_t write = 0;
  int64_t readBytes = 0;
  int64_t writeBytes = 0;
  int64_t sequential() const { return read + compute + write; }
  int64_t span() const { return isolatedSpan; }
  int64_t isolatedSpan = 0;
};

TileCost estimateTileCost(int64_t k, const ArrayConfig &array);

//===----------------------------------------------------------------------===//
// The per-accelerator timeline of consecutive invocations.
//
// One context runs a stream of invocations j = 0, 1, ... (the K-folded
// invocations of its tiles, in order). The events of invocation j, in
// cycles of the array clock, follow from the resources the context has
// (rtl/multi_200t/tile_pipelined/dma/systolic_accel_context.sv, external
// scheduler): a job queue of pipelineDepth entries, two operand buffers
// (ping-pong) of which at most one completely filled tile waits for the
// array, one operand descriptor in flight, one array, two result snapshot
// banks, one write-back stage, and inside every PE two accumulator sets and
// a reduction tree the next invocation's tree cannot overlap:
//
//   accept_j  = max(accept_{j-1}, writeEnd_{j-D} + 2)      queue of depth D; the
//                                      slot frees two cycles after the write
//                                      end (counter, registered acceptance);
//                                      serialized (D = 1) the host's resubmission
//                                      is the gap and the model keeps writeEnd
//   fillStart = max(accept_j, fillEnd_{j-1}, feedStart_{j-1})
//                                      one descriptor at a time; the next fill
//                                      only once the filled tile is being fed
//   fillEnd   = fillStart + T_read(k_j) the operands landed and the fold
//                                      released (c_read holds the release)
//   feedStart = max(fillEnd,
//                   feedStart_{j-1} + k_{j-1} + feedGap,   the PE boundary gap
//                   computeEnd_{j-2} + 3,  tile j-2 published: its accumulator
//                                          set in every PE is free (two sets)
//                   writeEnd_{j-2} + 3)    result bank j%2 written back
//                                      3: the release handshake, ready ->
//                                      scheduler start -> adapter -> fold
//   computeEnd= max(feedStart + T_compute(k_j), computeEnd_{j-1} + reduceCycles)
//                                      the invocation's own latency, or the
//                                      previous reduction holding this one
//   writeStart= max(computeEnd, writeEnd_{j-1});  writeEnd = writeStart + T_write
//
// With D = 1 these are exactly the serialized model: every invocation's
// fill starts at the previous write end, nothing overlaps. With transfers
// free (T_read = T_write = 0) the same recurrences give the compute-only
// timeline, which is what start_cycle in the IR is measured on (as it was
// for the serialized model: the compute terms before it).
//
// Every event is a cycle the RTL bench stamps (tb_pipeline_overlap.sv:
// fire = accept, fold = feedStart, cv = computeEnd, done = writeEnd), and
// eval/pipeline_overlap/compare_timeline.py requires them to agree to the
// cycle; the result readback over the DPTI link, which the host's pace
// decides, is outside this timeline (it may delay accept_j, which the bench
// supplies as observed).
//===----------------------------------------------------------------------===//

struct InvocationEvents {
  int64_t k = 0;
  int64_t accept = 0;
  int64_t fillStart = 0;
  int64_t fillEnd = 0;
  int64_t feedStart = 0;     // the fold's start: what start_cycle describes
  int64_t computeEnd = 0;    // c_valid: the result tile published
  int64_t writeStart = 0;
  int64_t writeEnd = 0;      // the result tile in memory
};

class AcceleratorTimeline {
public:
  /// A context of `array` idle at cycle 0. transfersFree: cost the fills
  /// and write-backs at zero cycles (the compute-only timeline).
  explicit AcceleratorTimeline(const ArrayConfig &array,
                               bool transfersFree = false);

  /// Append one invocation of depth k (1 <= k <= kMax, any k >= 1 without
  /// kMax). ok = false (and nothing appended) for an invalid depth or on
  /// overflow. acceptNotBefore: the descriptor arrives no earlier than this
  /// cycle (an observed acceptance; 0 = as soon as the queue has room).
  /// feedNotBefore: the fold starts no earlier than this cycle (an observed
  /// fold, when the operands landed later than T_read says because the
  /// shared read engine was held by a readback or another context's fill --
  /// which this timeline does not model). writeEndNotBefore: the write-back
  /// completes no earlier than this cycle (an observed completion, when the
  /// shared write engine was serving another context).
  bool push(int64_t k, InvocationEvents &events, int64_t acceptNotBefore = 0,
            int64_t feedNotBefore = 0, int64_t writeEndNotBefore = 0);

  /// The release handshake: accelerator_ready -> the scheduler's start pulse
  /// -> the adapter's fold_start, in cycles (systolic_hw_scheduler.sv,
  /// systolic_hw_scheduler_adapter.sv).
  static constexpr int64_t kReleaseLatency = 3;
  /// A queue slot freed by a write-back end is seen by the acceptance two
  /// cycles later (the counter update and the registered accept decision,
  /// systolic_dma_top.sv).
  static constexpr int64_t kAcceptLatency = 2;

  /// Append every invocation of a K-step reduction (a tile), in order.
  /// Returns the tile's events: fillStart of its first invocation and
  /// writeEnd of its last. ok = false on an invalid K or overflow.
  bool pushTile(int64_t reduction, int64_t &tileStart, int64_t &tileEnd);

  int64_t invocations() const { return count; }
  /// writeEnd of the last invocation; 0 before any.
  int64_t end() const { return lastWriteEnd; }

private:
  ArrayConfig config;
  bool transfersFree = false;
  int64_t count = 0;
  int64_t lastAccept = 0;
  int64_t lastFillEnd = 0;
  int64_t lastFeedStart = 0;
  int64_t lastK = 0;
  int64_t lastComputeEnd = 0;
  int64_t lastWriteEnd = 0;
  // writeEnd / computeEnd of the last max(pipelineDepth, 2) invocations,
  // oldest first.
  int64_t writeEnds[64];
  int64_t computeEnds[64];
  int64_t writeEndCount = 0;
  int64_t window = 2;
};

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
