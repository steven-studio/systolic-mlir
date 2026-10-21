#ifndef SYSTOLIC_COSTMODELV2_H
#define SYSTOLIC_COSTMODELV2_H

// Cost Model V2: the FPGA fleet as one machine (docs/COSTMODEL_V2.md).
//
// Cost Model V1 (CostModel.h) prices every accelerator on its own: an
// invocation's operand fill takes T_read = 128 * ceil(k / 8) + 11 cycles
// whatever the other accelerators do. On the hardware the four contexts of
// systolic_dma_top share
//
//   - one AXI read port (dma_engine_multi): round robin over the four
//     operand slots and the result readback, 16-beat bursts while one slot
//     is active and SHARED_BURST_LEN-beat bursts (1 on the v1 operand path)
//     while two or more are, at most 8 bursts in flight, one ID, so the data
//     returns in request order and the destination of the beat at the head
//     stalls every beat behind it (head-of-line blocking);
//   - one write-back engine, granted to the lowest-numbered waiting context,
//     one result tile at a time;
//   - one readback path: a round-robin FSM over the contexts' completion
//     queues, a 16-byte completion record and the tile's beats through the
//     same read port into an 8-beat asynchronous FIFO that the DPTI byte
//     interface drains at two 60 MHz clocks per byte;
//   - one job port.
//
// The fleet simulator below steps that machine cycle by cycle -- the array
// clock for the contexts and engines, the DPTI clock for the output path --
// with every rule taken from the RTL (the sources are cited at each rule in
// CostModelV2.cpp). The per-context events (acceptance, fill, fold, publish,
// write-back) keep V1's recurrences, which the RTL bench reproduces to the
// cycle; what V2 adds is everything the contexts share. Shared means shared
// hardware: two contexts that read the same matrix region still fetch it
// twice, as the runtime and the RTL do (every invocation has its own operand
// slab); nothing here deduplicates reads.
//
// What is outside: the host's DPTI link traffic into the FPGA (operand slab
// MEM_WRITEs and descriptors, docs/COSTMODEL_V2.md section 6), the hardware
// scheduler's start_cycle hold (the compiler's start_cycle is the
// compute-only timeline, which never holds a fold on a fleet it was planned
// for), DDR3 timing beyond an ideal one-beat-per-cycle memory (the RTL
// simulation's memory model; the board's MIG is UNCALIBRATED).

#include "Systolic/CostModel.h"

#include <cstdint>
#include <string>
#include <vector>

namespace systolic {
namespace v2 {

/// One context (accelerator) of the fleet: context i is the read engine's
/// operand slot i and the write-back arbiter's priority i.
struct ContextConfig {
  int64_t rows = 0;            // the array is rows x rows
  int64_t kMax = 0;            // operand buffer depth (job.k <= kMax)
  int64_t tileOverhead = 95;   // H: fold start -> publish minus k + 2 rows - 2
  int64_t reduceCycles = 69;   // least spacing of two publishes
  int64_t feedGap = 26;        // array-input quiet gap between invocations
  int64_t pipelineDepth = 4;   // JOB_QUEUE_DEPTH
};

/// The shared hardware, as the RTL builds it. Every default is the
/// bitstream's (rtl/multi_200t/tile_pipelined/dma, USE_V2 = 0).
struct FleetHardware {
  std::vector<ContextConfig> contexts;

  // ---- operand fill (systolic_accel_context.sv, dma_operand_writer*.sv) ----
  int64_t writerCyclesPerBeat = 4;  // v1 writer: a 128-bit beat in 4 cycles; v2: 1
  int64_t landTail = 5;             // last beat -> last word landed (v1 5, v2 2)
  int64_t release = 4;              // landed -> fold start (ready, scheduler, adapter)
  int64_t descLatency = 3;          // fill start -> descriptor at the read engine
  int64_t wireLanes = 8;            // N_WIRE: every context moves 8 lanes
  int64_t kWindow = 8;              // slab window, reduction steps
  int64_t elementBytes = 4;
  int64_t beatBytes = 16;           // AXI data width / 8

  // ---- the read engine (dma_engine_multi.sv) -------------------------------
  int64_t burstLen = 16;            // beats per burst, one active slot
  int64_t sharedBurstLen = 1;       // beats per burst, >= 2 active slots (v1 1, v2 16)
  int64_t maxOutstanding = 8;       // bursts in flight, all slots
  int64_t pageBytes = 4096;         // a burst never crosses a page

  // ---- write-back (systolic_accel_context.sv W_SCAN, dma_writeback_engine) -
  int64_t scanExtra = 5;            // publish -> write-back request = rows^2 + 5
  int64_t wbEngineExtra = 6;        // request granted -> done = rows^2 / 4 + 6

  // ---- readback (systolic_dma_top.sv) --------------------------------------
  bool readback = true;             // results are read back to the host
  int64_t rbDepth = 8;              // RB_DEPTH: accepted and not read back
  int64_t rbFifoDepth = 8;          // dpti_output_cdc DEPTH, beats

  // ---- the DPTI output path (dpti_beat_to_byte, dpti_sync245_phy TX) -------
  int64_t uiPeriodPs = 10000;       // the array clock, 100 MHz
  int64_t dptiPeriodPs = 16667;     // CLKOUT, 60 MHz
  int64_t dptiPhasePs = 0;          // first DPTI edge after the array edge of cycle 0
  int64_t txFifoDepth = 4;
  int64_t turnCycles = 2;           // TURN_CYCLES

  // ---- DDR layout of the asynchronous runtime (fpga_async_runtime.cpp) -----
  // Bursts are clamped at 4 KiB pages, so where a slab lies matters.
  int64_t operandBase = 0x10000;
  int64_t operandSlots = 5;         // depth + 1 per context
  int64_t slabBytes = 0;            // 0 = max over contexts of ceil(kMax / 8) * 512
  int64_t resultBase = 0x3000;
  int64_t resultSlots = 4;
  int64_t resultSlotBytes = 256;
};

/// The bitstream's fleet: one 8x8 (context 0) and three 4x4s, operand depths
/// k8 / k4; beatWide = the v2 operand path; pipelined = the job queue of
/// depth 4 (false: one job at a time, the archived bitstream's depth).
FleetHardware fpgaFleet(int64_t k8, int64_t k4, bool beatWide = false,
                        bool pipelined = true);

/// True if `hw` is something the simulator can run: at least one context,
/// rows in 1..wireLanes, kMax >= 1, depths and constants in range.
bool isValidFleetHardware(const FleetHardware &hw, std::string *why = nullptr);

/// Read beats of an invocation of depth k: ceil(k / kWindow) windows of
/// kWindow steps x wireLanes lanes, A and B, elementBytes each.
int64_t operandBeats(int64_t k, const FleetHardware &hw);

/// How jobs reach the contexts.
///   InOrder  one port, jobs offered in list order; a job whose context is
///            full blocks the jobs behind it (the RTL bench's port).
///   Credit   a job is offered only once its context can take it, the
///            lowest-numbered context first among those that can -- what the
///            asynchronous runtime does (it sends a descriptor only to a
///            device with a free slot).
enum class PortPolicy { InOrder, Credit };

struct FleetJob {
  int64_t context = 0;
  int64_t k = 1;
  int64_t present = 0;   // offered no earlier than this cycle (>= 1 is used)
  int64_t dep = -1;      // index of an earlier job, or -1
  int64_t gap = 0;       // offered no earlier than that job's done + gap
};

/// Every event of one job, in cycles of the array clock (-1: never).
struct FleetJobEvents {
  int64_t offer = -1, fire = -1;          // offered, accepted (job_fire)
  int64_t fillStart = -1, desc = -1;      // the context's fill, its read descriptor
  int64_t lastBeat = -1, land = -1;       // last operand beat, last word landed
  int64_t fold = -1, cv = -1;             // fold start, result published
  int64_t wbReq = -1, wbAcc = -1, done = -1;
  int64_t rbTake = -1, rbDesc = -1, rbDone = -1;
  int64_t readCycles() const { return land - desc; }  // descriptor -> landed
};

struct FleetStats {
  int64_t arBursts = 0;                   // AR requests, all slots
  int64_t operandArBursts = 0;
  std::vector<int64_t> burstLengths;      // histogram, index = beats
  int64_t maxOutstanding = 0;             // bursts in flight, at most
  int64_t operandBeats = 0, readbackBeats = 0;
  int64_t operandReadBytes = 0;           // DDR3 -> contexts
  int64_t readbackBytes = 0;              // DDR3 -> host (records + tiles)
  int64_t sharedCycles = 0;               // cycles with >= 2 slots active
  int64_t rStallCycles = 0;               // R beat held by its destination
  int64_t contentionCycles = 0;           // sum over jobs of readCycles - alone
  int64_t simulatedCycles = 0;
};

struct FleetResult {
  bool ok = false;
  std::string error;
  std::vector<FleetJobEvents> jobs;
  FleetStats stats;
  /// First acceptance -> last result in DDR (what V1's makespan measures).
  int64_t deviceMakespan = 0;
  /// The same per context (0 for a context with no job).
  std::vector<int64_t> contextMakespan;
  /// First acceptance -> last result read back (rbDone; the last byte of it
  /// reaches the host lastByteToHost).
  int64_t readbackEnd = 0;
  int64_t lastByteToHost = 0;
};

struct FleetOptions {
  PortPolicy port = PortPolicy::InOrder;
  /// Jump over cycles in which provably nothing changes: every engine,
  /// queue and FIFO empty, and the next cycle anything can happen known.
  /// The result is identical either way (eval/cost_model_v2 checks both).
  bool skipIdle = true;
};

/// No event for this many cycles while something is in flight means the
/// simulated hardware is stuck: simulateFleet reports a deadlock instead of
/// running on (every in-flight state of the RTL moves within a few hundred
/// cycles; the bound is far beyond any of them).
constexpr int64_t kDeadlockCycles = 10000000;

/// Run the jobs on the fleet.
FleetResult simulateFleet(const FleetHardware &hw,
                          const std::vector<FleetJob> &jobs,
                          const FleetOptions &options = {});

/// The cycles one invocation's fill takes with the read port to itself:
/// descriptor -> landed = (writer * (beats - 1)) + 3 + landTail, and
/// fill start -> fold = descLatency + that + release (V1's T_read).
int64_t aloneReadCycles(int64_t k, const FleetHardware &hw);

//===----------------------------------------------------------------------===//
// The binary search's view: every accelerator busy at once.
//===----------------------------------------------------------------------===//

/// One geometry's tile stream under a saturated fleet: every context of the
/// fleet runs tiles of a `reduction`-deep GEMM back to back (each tile its
/// ceil(K / kMax) K-folded invocations, offered as the runtime offers them,
/// PortPolicy::Credit), all contending for the shared engines at once. For
/// the contexts of geometry g, tile n of a context ends at its last
/// invocation's done; the stream is the pointwise maximum over g's contexts
/// (so every accelerator of g is at least this fast) -- nondecreasing in n,
/// which keeps the search's capacity(T) nondecreasing in T.
struct SaturatedStreams {
  bool ok = false;
  std::string error;
  std::vector<int64_t> geometries;              // distinct rows, as given
  std::vector<std::vector<int64_t>> tileStart;  // per geometry
  std::vector<std::vector<int64_t>> tileEnd;
  /// Fleet simulations run until every context was still busy when the
  /// last measured tile ended (each run lengthens the streams that ran dry).
  int64_t simulationRuns = 0;
};

/// tilesPerGeometry[i]: the stream length needed for geometry rows[i]
/// (`geometries` in the result follows `rows`). Every tile of every stream
/// is simulated -- nothing is extrapolated -- so the cost grows with the
/// streams' length.
SaturatedStreams saturatedStreams(const FleetHardware &hw, int64_t reduction,
                                  const std::vector<int64_t> &rows,
                                  const std::vector<int64_t> &tilesPerGeometry);

} // namespace v2
} // namespace systolic

#endif // SYSTOLIC_COSTMODELV2_H
