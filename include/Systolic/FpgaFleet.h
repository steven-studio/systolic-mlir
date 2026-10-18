#ifndef SYSTOLIC_FPGAFLEET_H
#define SYSTOLIC_FPGAFLEET_H

// The 1x8x8 + 3x4x4 FPGA fleet (rtl/multi_200t/tile_pipelined, systolic_dma_top
// with the v1 operand writer), as the cost model sees it: one place for the
// constants, used by systolic-matmul to declare the fleet it compiles for and
// by systolic-cost to price it. Where each number comes from is in
// docs/COSTMODEL.md; in short:
//
//   H = 95                fold_start -> c_valid minus k + rows + cols - 2.
//                         Board-measured on the fold RTL (tag paper-hw-v1);
//                         the same 95 in Verilator simulation of this RTL.
//   read 4.0 B/cycle, +11 operand fill, descriptor accepted -> fold start.
//                         One 32-bit word per cycle: the v1 operand writer
//                         unpacks a 128-bit beat in four cycles. Fitted in
//                         simulation (K_MAX_8X8 = 24, K_MAX_4X4 = 12
//                         calibration set), not measured on the board.
//   write 3.2 B/cycle, +11 c_valid -> result tile in DDR: a word per cycle
//                         out of the array plus a beat per cycle into the
//                         write engine (1.25 cycles/word). Same fit.
//   8 lanes, windows of 8 the slab format (runtime/fpga_matmul_tiled.cpp):
//                         ceil(k / 8) windows of 8 steps x 8 lanes per
//                         operand, for the 4x4s too.
//
// Variable-K: the runtime sends each invocation's own depth (job.k = k_i,
// 1 <= k_i <= K_MAX), and the RTL reads ceil(k_i / 8) windows and runs k_i
// reduction steps (systolic_accel_context.sv: job_k_slab_reg, k_dim).
//
// None of these is the DDR3 supply the paper measures (3.63 words/cycle,
// sequential); that is a limit on all four fills together -- the four arrays
// run at the same time and fill through one AXI read port (dma_engine_multi)
// -- applied as the binary search's shared read bandwidth:
//
//   shared read 3.51 words/cycle   the lower of the two DDR3 supplies
//                         eval/kmax_sweep/README.md lists as measured on the
//                         N = 8 DMA path (3.51, 3.627). A reference taken
//                         from that record, not measured on this bitstream;
//                         systolic-matmul and --systolic-fpga-pipeline take
//                         another value, or 0 for independent fills.

#include "Systolic/CostModel.h"

#include <cstdint>
#include <sstream>
#include <string>

namespace systolic {
namespace fpga {

constexpr int64_t kNum8x8 = 1;
constexpr int64_t kNum4x4 = 3;

constexpr int64_t kTileOverhead = 95;
constexpr double kReadBytesPerCycle = 4.0;
constexpr int64_t kReadOverhead = 11;
constexpr double kWriteBytesPerCycle = 3.2;
constexpr int64_t kWriteOverhead = 11;
constexpr int64_t kOperandLanes = 8;
constexpr int64_t kKWindow = 8;
constexpr int64_t kElementBytes = 4;

// How one context runs consecutive invocations (CostModel.h,
// AcceleratorTimeline). The archived bitstream (artifacts/fpga/
// tile_pipelined/1x8x8-3x4x4) serializes them: the next invocation's fill
// starts once the previous result tile is written back -- pipeline depth 1,
// which is what deviceConfig(edge, kMax) declares. The pipelined RTL
// (systolic_accel_context.sv with its job queue, 2026-10-18) overlaps them:
//
//   kPipelineDepth  4   JOB_QUEUE_DEPTH: invocations a context holds at a
//                       time, accepted and not yet written back
//   kReduceCycles  69   the least spacing of two result publishes of one
//                       array: the four-level bank-reduction tree of one
//                       invocation (21 + 17 + 15 + 14 = 67 cycles,
//                       tb_array_h_decomp.sv) plus the two cycles from
//                       RED_DONE back to RED_IDLE before the next tile's
//                       set can be handed to it; the part of H = 95 the
//                       next invocation cannot overlap (measured 69 in
//                       tb_pipeline_overlap, 4x4 k = 1 chain)
//   kFeedGap       26   array-input idle cycles the context keeps between
//                       invocation j's last reduction step and j+1's first:
//                       the PE detects a transaction boundary as a quiet
//                       gap of MUL + ADD + 2 cycles (23 at the integer
//                       model's 9 / 12, 21 at the IP's 8 / 11,
//                       tb_pe_overlap.sv) and the context adds three cycles
//                       of margin (PE_QUIET_GAP + FEED_GAP_MARGIN)
//
// A bitstream's record (fpga_config.txt) says which it is: PIPELINE_DEPTH,
// absent for the archived bitstream.
constexpr int64_t kPipelineDepth = 4;
constexpr int64_t kReduceCycles = 69;
constexpr int64_t kFeedGap = 26;

/// Operand words per array cycle all four fills share (see above), and the
/// same in bytes, as the binary search takes it.
constexpr double kSharedReadWordsPerCycle = 3.51;
inline double sharedReadBytesPerCycle(double wordsPerCycle) {
  return wordsPerCycle * static_cast<double>(kElementBytes);
}

/// One accelerator of the fleet with operand depth kMax, as the cost model
/// prices it: serialized consecutive invocations (the archived bitstream),
/// or, with pipelined = true, the pipelined RTL's.
inline ArrayConfig deviceConfig(int64_t edge, int64_t kMax,
                                bool pipelined = false) {
  ArrayConfig c;
  c.rows = edge;
  c.cols = edge;
  c.kMax = kMax;
  c.tileOverhead = kTileOverhead;
  c.readBytesPerCycle = kReadBytesPerCycle;
  c.readOverhead = kReadOverhead;
  c.writeBytesPerCycle = kWriteBytesPerCycle;
  c.writeOverhead = kWriteOverhead;
  c.operandLanes = kOperandLanes;
  c.kWindow = kKWindow;
  c.elementBytes = kElementBytes;
  if (pipelined) {
    c.pipelineDepth = kPipelineDepth;
    c.reduceCycles = kReduceCycles;
    c.feedGap = kFeedGap;
  }
  return c;
}

/// A double as an MLIR float literal that parses back to the same value:
/// the shortest such decimal, always with a decimal point (3.2, 4.0).
inline std::string floatLiteral(double v) {
  std::string text;
  for (int precision = 1; precision <= 17; ++precision) {
    std::ostringstream os;
    os.precision(precision);
    os << v;
    text = os.str();
    if (std::stod(text) == v)
      break;
  }
  if (text.find_first_of(".eEn") == std::string::npos)
    text += ".0";
  return text;
}

/// The same accelerator as a systolic.device declaration.
inline std::string deviceDeclaration(const std::string &symbol, int64_t edge,
                                     int64_t kMax, bool pipelined = false) {
  std::ostringstream os;
  os << "systolic.device @" << symbol << " rows = " << edge
     << " cols = " << edge << " dataflow = output_stationary k_max = "
     << kMax << " tile_overhead = " << kTileOverhead
     << " read_bytes_per_cycle = " << floatLiteral(kReadBytesPerCycle)
     << " write_bytes_per_cycle = " << floatLiteral(kWriteBytesPerCycle)
     << " read_overhead = " << kReadOverhead
     << " write_overhead = " << kWriteOverhead
     << " operand_lanes = " << kOperandLanes << " k_window = " << kKWindow;
  if (pipelined)
    os << " pipeline_depth = " << kPipelineDepth
       << " reduce_cycles = " << kReduceCycles << " feed_gap = " << kFeedGap;
  return os.str();
}

} // namespace fpga
} // namespace systolic

#endif // SYSTOLIC_FPGAFLEET_H
