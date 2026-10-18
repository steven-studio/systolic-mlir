//===- systolic-cost.cpp - the cost model and the binary search, on the CLI -===//
//
// Prices invocations and schedules GEMMs on the 1x8x8 + 3x4x4 FPGA fleet
// with exactly the code the compiler uses (CostModel.cpp, SystolicOptimizer
// .cpp, the constants of Systolic/FpgaFleet.h) -- for the Verilog-simulation
// validation (eval/cost_model_validation) and the K_MAX design-space
// exploration (eval/kmax_dse), which must not carry a model of their own.
//
//   systolic-cost invocation --geometry G --k-max KM --k K [fleet options]
//       every invocation of one G x G tile of reduction K, and the tile
//   systolic-cost schedule --m M --n N --k K [fleet options] [--tiles]
//       the binary-search schedule of an M x N x K GEMM on the fleet
//   systolic-cost check-monotone --m M --n N --k K [fleet options] [--span S]
//       the search's minimum T*: infeasible at T* - 1, feasible at
//       T* .. T* + S and at 2 T*, 4 T*, 8 T*
//   systolic-cost batch [--input FILE] [--verify]
//       one schedule per input line, CSV out (see batchUsage)
//   systolic-cost fleet --k8 N --k4 N
//       the fleet's systolic.device declarations, exactly as systolic-matmul
//       emits them
//   systolic-cost timeline --geometry G --k-max KM --k k0,k1,... [--accept t0,t1,...]
//                          [--feed t0,t1,...] [--write t0,t1,...]
//       the events of that stream of invocations on one context (each
//       accepted no earlier than its --accept cycle, its fold started no
//       earlier than its --feed cycle and its write-back complete no earlier
//       than its --write cycle, if given: observed stamps for what the
//       timeline does not model -- the host's pace and the shared read and
//       write engines' waits on other contexts), one line per invocation -- what
//       eval/pipeline_overlap/compare_timeline.py holds against the RTL
//       bench's stamps
//
// fleet options:
//   --k8 N --k4 N        K_MAX_8X8 / K_MAX_4X4 (default 16 / 16)
//   --n8 N --n4 N        accelerators of each geometry (default 1 / 3)
//   --compute-only       no read or write term
//   --shared-read-words-per-cycle B
//                        all fills together at most B 32-bit words per
//                        cycle (default 0: none)
//   --extra-read-overhead C | C8/C4
//                        C more cycles on every invocation's read term
//                        (C8 on the 8x8s, C4 on the 4x4s) -- a what-if for
//                        a cost the model does not have: head-of-line
//                        waits on the shared read port, the result
//                        readback between two invocations (default 0)
//   --pipelined          the fleet's contexts pipeline consecutive
//                        invocations (Systolic/FpgaFleet.h: the job queue,
//                        the serial reduction tree and the PE's feed gap of
//                        the pipelined RTL); default: serialized, as the
//                        archived bitstream runs
//   --pipeline-depth D --reduce-cycles R --feed-gap G
//                        the same three, given explicitly
//   --read-bytes-per-cycle B
//                        one invocation's own fill rate instead of the v1
//                        operand writer's 4.0 (the beat-wide v2 path fills
//                        at 16.0: a 128-bit beat per cycle), a what-if
//
// `invocation` also prints every invocation's events on the tile's own
// per-accelerator timeline (accept, fill, feed = fold start, cv = result
// published, write) and, with --stream N, the stream of N such tiles back to
// back -- what the RTL bench tb_pipeline_overlap stamps per job.
//
//===----------------------------------------------------------------------===//

#include "Systolic/CostModel.h"
#include "Systolic/FpgaFleet.h"
#include "Systolic/SystolicOptimizer.h"
#include "Systolic/SystolicTiling.h"

#include "llvm/ADT/SmallVector.h"

#include <cerrno>
#include <cinttypes>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace mlir::systolic;

namespace {

struct FleetSpec {
  int64_t k8 = 16, k4 = 16, n8 = ::systolic::fpga::kNum8x8,
          n4 = ::systolic::fpga::kNum4x4;
  bool computeOnly = false;
  double sharedReadWordsPerCycle = 0.0;
  // Cycles added to every invocation's read term, per geometry (a what-if
  // for a cost outside the model, e.g. the readback gap between two
  // invocations on one array).
  int64_t extraReadOverhead8 = 0, extraReadOverhead4 = 0;
  // Consecutive invocations on one context (CostModel.h): 1 / 0 / 0 is the
  // serialized hardware.
  int64_t pipelineDepth = 1, reduceCycles = 0, feedGap = 0;
  // One invocation's fill rate, when not the fleet's v1 constant.
  double readBytesPerCycle = 0.0;
};

[[noreturn]] void die(const std::string &message) {
  std::fprintf(stderr, "systolic-cost: %s\n", message.c_str());
  std::exit(2);
}

bool parseInt(const std::string &text, int64_t &value) {
  if (text.empty())
    return false;
  char *end = nullptr;
  errno = 0;
  long long v = std::strtoll(text.c_str(), &end, 10);
  if (errno != 0 || !end || *end != '\0')
    return false;
  value = v;
  return true;
}

bool parseDouble(const std::string &text, double &value) {
  if (text.empty())
    return false;
  char *end = nullptr;
  errno = 0;
  double v = std::strtod(text.c_str(), &end);
  if (errno != 0 || !end || *end != '\0' || !std::isfinite(v))
    return false;
  value = v;
  return true;
}

/// "C" (both geometries) or "C8/C4" (the 8x8s / the 4x4s), each >= 0.
bool parseExtra(const std::string &text, FleetSpec &f) {
  const size_t slash = text.find('/');
  if (slash == std::string::npos) {
    if (!parseInt(text, f.extraReadOverhead8) || f.extraReadOverhead8 < 0)
      return false;
    f.extraReadOverhead4 = f.extraReadOverhead8;
    return true;
  }
  return parseInt(text.substr(0, slash), f.extraReadOverhead8) &&
         parseInt(text.substr(slash + 1), f.extraReadOverhead4) &&
         f.extraReadOverhead8 >= 0 && f.extraReadOverhead4 >= 0;
}

/// --key value / --key=value / --flag, after the subcommand.
std::map<std::string, std::string> parseOptions(int argc, char **argv,
                                                int first) {
  static const char *const flags[] = {"--compute-only", "--tiles",
                                      "--verify", "--pipelined"};
  std::map<std::string, std::string> options;
  for (int i = first; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg.rfind("--", 0) != 0)
      die("unexpected argument '" + arg + "'");
    const size_t eq = arg.find('=');
    if (eq != std::string::npos) {
      options[arg.substr(0, eq)] = arg.substr(eq + 1);
      continue;
    }
    bool isFlag = false;
    for (const char *f : flags)
      isFlag |= arg == f;
    if (isFlag) {
      options[arg] = "1";
      continue;
    }
    if (i + 1 >= argc)
      die("option " + arg + " needs a value");
    options[arg] = argv[++i];
  }
  return options;
}

int64_t intOption(const std::map<std::string, std::string> &options,
                  const std::string &key, int64_t fallback, bool required) {
  auto it = options.find(key);
  if (it == options.end()) {
    if (required)
      die("missing " + key);
    return fallback;
  }
  int64_t v = 0;
  if (!parseInt(it->second, v))
    die(key + " must be a decimal integer, got '" + it->second + "'");
  return v;
}

FleetSpec fleetOptions(const std::map<std::string, std::string> &options) {
  FleetSpec f;
  f.k8 = intOption(options, "--k8", f.k8, false);
  f.k4 = intOption(options, "--k4", f.k4, false);
  f.n8 = intOption(options, "--n8", f.n8, false);
  f.n4 = intOption(options, "--n4", f.n4, false);
  f.computeOnly = options.count("--compute-only") != 0;
  auto it = options.find("--shared-read-words-per-cycle");
  if (it != options.end() &&
      (!parseDouble(it->second, f.sharedReadWordsPerCycle) ||
       f.sharedReadWordsPerCycle < 0.0))
    die("--shared-read-words-per-cycle must be a non-negative number");
  it = options.find("--extra-read-overhead");
  if (it != options.end() && !parseExtra(it->second, f))
    die("--extra-read-overhead must be C or C8/C4, each >= 0");
  if (options.count("--pipelined")) {
    f.pipelineDepth = ::systolic::fpga::kPipelineDepth;
    f.reduceCycles = ::systolic::fpga::kReduceCycles;
    f.feedGap = ::systolic::fpga::kFeedGap;
  }
  const bool explicitPipeline = options.count("--pipeline-depth") ||
                                options.count("--reduce-cycles") ||
                                options.count("--feed-gap");
  if (explicitPipeline) {
    if (options.count("--pipelined"))
      die("--pipelined and --pipeline-depth / --reduce-cycles / --feed-gap "
          "exclude each other");
    f.pipelineDepth = intOption(options, "--pipeline-depth", 0, true);
    f.reduceCycles = intOption(options, "--reduce-cycles", 0, true);
    f.feedGap = intOption(options, "--feed-gap", 0, true);
    if (f.pipelineDepth < 1 || f.reduceCycles < 0 || f.feedGap < 0 ||
        f.reduceCycles > ::systolic::fpga::kTileOverhead)
      die("--pipeline-depth must be >= 1, --reduce-cycles in 0.." +
          std::to_string(::systolic::fpga::kTileOverhead) +
          " and --feed-gap >= 0");
  }
  it = options.find("--read-bytes-per-cycle");
  if (it != options.end() &&
      (!parseDouble(it->second, f.readBytesPerCycle) || f.readBytesPerCycle <= 0.0))
    die("--read-bytes-per-cycle must be a positive number");
  if (f.k8 < 1 || f.k4 < 1 || f.n8 < 0 || f.n4 < 0 || f.n8 + f.n4 < 1)
    die("K_MAX must be >= 1 and the fleet non-empty");
  return f;
}

::systolic::ArrayConfig makeConfig(int64_t edge, int64_t kMax,
                                   const FleetSpec &f) {
  ::systolic::ArrayConfig c = ::systolic::fpga::deviceConfig(edge, kMax);
  c.pipelineDepth = f.pipelineDepth;
  c.reduceCycles = f.reduceCycles;
  c.feedGap = f.feedGap;
  if (f.readBytesPerCycle > 0.0)
    c.readBytesPerCycle = f.readBytesPerCycle;
  if (f.computeOnly) {
    c.readBytesPerCycle = 0.0;
    c.readOverhead = 0;
    c.writeBytesPerCycle = 0.0;
    c.writeOverhead = 0;
  } else {
    c.readOverhead += edge == 8 ? f.extraReadOverhead8 : f.extraReadOverhead4;
  }
  return c;
}

llvm::SmallVector<SystolicArrayResource> makeFleet(const FleetSpec &f) {
  llvm::SmallVector<SystolicArrayResource> fleet;
  int64_t id = 0;
  for (int64_t i = 0; i < f.n8; ++i)
    fleet.push_back({8, id++, makeConfig(8, f.k8, f)});
  for (int64_t i = 0; i < f.n4; ++i)
    fleet.push_back({4, id++, makeConfig(4, f.k4, f)});
  return fleet;
}

SystolicOptimizerOptions optimizerOptions(const FleetSpec &f) {
  SystolicOptimizerOptions o;
  o.sharedReadBytesPerCycle =
      f.sharedReadWordsPerCycle * ::systolic::fpga::kElementBytes;
  return o;
}

// ---------------------------------------------------------------------------
// invocation
// ---------------------------------------------------------------------------

int runInvocation(const std::map<std::string, std::string> &options) {
  const FleetSpec f = fleetOptions(options);
  const int64_t geometry = intOption(options, "--geometry", 0, true);
  const int64_t kMax = intOption(options, "--k-max", 0, true);
  const int64_t k = intOption(options, "--k", 0, true);
  if (geometry < 1 || kMax < 1 || k < 1)
    die("--geometry, --k-max and --k must be >= 1");

  const ::systolic::ArrayConfig cfg = makeConfig(geometry, kMax, f);
  const ::systolic::TileCost t = ::systolic::estimateTileCost(k, cfg);
  if (!t.ok)
    die("cost model cannot price the tile (invalid or overflowing)");

  // Every invocation but the last runs the full k_max, so past a few dozen
  // the list is the first one repeated: print the first 16 and the last.
  const int64_t invocations = t.invocations;
  for (int64_t j = 0; j < invocations; ++j) {
    if (j == 16 && invocations > 17) {
      std::printf("invocation j=16..%" PRId64 " same as j=15\n",
                  invocations - 2);
      j = invocations - 1;
    }
    const ::systolic::InvocationCost c =
        ::systolic::estimateInvocationCost(k, j, cfg);
    if (!c.ok)
      die("cost model cannot price invocation " + std::to_string(j));
    std::printf("invocation j=%" PRId64 " k=%" PRId64
                " read_bytes=%" PRId64
                " write_bytes=%" PRId64 " read=%" PRId64 " compute=%" PRId64
                " write=%" PRId64 " total=%" PRId64 "\n",
                j, c.k, c.readBytes, c.writeBytes, c.read,
                c.compute, c.write, c.sequential());
  }
  std::printf("tile geometry=%" PRId64 " k_max=%" PRId64 " k=%" PRId64
              " invocations=%" PRId64 " read=%" PRId64 " compute=%" PRId64
              " write=%" PRId64 " total=%" PRId64 " read_bytes=%" PRId64
              " write_bytes=%" PRId64 " span=%" PRId64 " pipelined=%s\n",
              geometry, kMax, k, t.invocations, t.read, t.compute, t.write,
              t.sequential(), t.readBytes, t.writeBytes, t.span(),
              ::systolic::isPipelined(cfg) ? "yes" : "no");

  // The tile's invocations on the accelerator's timeline, and with --stream N
  // a stream of N such tiles back to back: every event in cycles from the
  // first acceptance. The compute-only column is the fold start with the
  // transfers free -- what start_cycle carries in the IR.
  int64_t tiles = intOption(options, "--stream", 1, false);
  if (tiles < 1)
    die("--stream must be >= 1");
  ::systolic::AcceleratorTimeline full(cfg);
  ::systolic::AcceleratorTimeline computeOnly(cfg, /*transfersFree=*/true);
  int64_t printed = 0;
  for (int64_t tile = 0; tile < tiles; ++tile) {
    for (int64_t j = 0; j < invocations; ++j) {
      ::systolic::InvocationEvents e, c;
      const int64_t depth = ::systolic::invocationDepth(k, j, cfg);
      if (!full.push(depth, e) || !computeOnly.push(depth, c))
        die("cost model cannot place invocation " + std::to_string(j) +
            " of tile " + std::to_string(tile) + " on the timeline");
      // Past a few dozen the list is one steady state repeated; print the
      // first 32 events and the last tile.
      if (printed < 32 || tile == tiles - 1) {
        std::printf("event tile=%" PRId64 " j=%" PRId64 " k=%" PRId64
                    " accept=%" PRId64 " fill_start=%" PRId64
                    " fill_end=%" PRId64 " feed_start=%" PRId64
                    " compute_end=%" PRId64 " write_start=%" PRId64
                    " write_end=%" PRId64 " start_cycle=%" PRId64 "\n",
                    tile, j, e.k, e.accept, e.fillStart, e.fillEnd,
                    e.feedStart, e.computeEnd, e.writeStart, e.writeEnd,
                    c.feedStart);
        ++printed;
      }
    }
  }
  std::printf("stream tiles=%" PRId64 " invocations=%" PRId64 " end=%" PRId64
              " compute_only_end=%" PRId64 "\n",
              tiles, full.invocations(), full.end(), computeOnly.end());
  return 0;
}

// ---------------------------------------------------------------------------
// schedule / batch
// ---------------------------------------------------------------------------

struct ScheduleSummary {
  bool ok = false;
  int64_t makespan = 0, computeOnly = 0, scheduleEnd = 0;
  int64_t tiles8 = 0, tiles4 = 0, invocations8 = 0, invocations4 = 0;
  int64_t read = 0, compute = 0, write = 0;   // summed over all invocations
  int64_t readBytes = 0, sharedReadCycles = 0, checks = 0;
  int64_t tileCost8 = 0, tileCost4 = 0;       // one spatial tile, sequential
  int64_t tileSpan8 = 0, tileSpan4 = 0;       // one spatial tile alone on a
                                              // context (= cost serialized)
  // --verify: infeasible at makespan - 1, feasible at every later deadline
  // probed.
  int minimal = -1, monotone = -1;
  // --verify: the schedule is a valid one (validSchedule).
  int valid = -1;
};

/// An independent check of the optimizer's output: every original cell of
/// the m x n output covered exactly once, no tile entirely in the padding,
/// every tile on an accelerator of its own size, the tiles of one
/// accelerator holding it, in schedule order, for exactly the intervals the
/// cost model's per-accelerator timeline gives them (serialized: each for
/// the cost model's tile cost, never overlapping in time; pipelined:
/// consecutive tiles' invocations overlap), and everything over by the
/// makespan.
bool validSchedule(int64_t m, int64_t n, int64_t k,
                   const ExactSystolicOptimizationResult &r,
                   llvm::ArrayRef<SystolicArrayResource> fleet) {
  if (r.decomposition.size() != r.schedule.size())
    return false;
  std::vector<uint8_t> cover(static_cast<size_t>(m * n), 0);
  // Per accelerator: (startCycle, index) of its tiles.
  std::map<int64_t, std::vector<std::pair<int64_t, size_t>>> busy;
  for (size_t i = 0; i < r.decomposition.size(); ++i) {
    const SystolicExecutionTask &t = r.decomposition[i];
    const ScheduledSystolicTile &s = r.schedule[i];
    if (t.row < 0 || t.column < 0 || t.row >= m || t.column >= n)
      return false;
    const SystolicArrayResource *res = nullptr;
    for (const SystolicArrayResource &x : fleet)
      if (x.acceleratorId == t.acceleratorId)
        res = &x;
    if (!res || res->arraySize != t.size)
      return false;
    if (s.startCycle < 0 || s.endCycle < s.startCycle ||
        s.endCycle > r.makespan)
      return false;
    busy[t.acceleratorId].push_back({s.startCycle, i});
    for (int64_t a = t.row; a < std::min(m, t.row + t.size); ++a)
      for (int64_t b = t.column; b < std::min(n, t.column + t.size); ++b)
        if (cover[static_cast<size_t>(a * n + b)]++ != 0)
          return false;
  }
  for (uint8_t c : cover)
    if (c != 1)
      return false;
  for (auto &entry : busy) {
    std::stable_sort(entry.second.begin(), entry.second.end(),
                     [](const std::pair<int64_t, size_t> &a,
                        const std::pair<int64_t, size_t> &b) {
                       return a.first < b.first;
                     });
    const SystolicArrayResource *res = nullptr;
    for (const SystolicArrayResource &x : fleet)
      if (x.acceleratorId == entry.first)
        res = &x;
    if (!res)
      return false;
    const ::systolic::TileCost cost = ::systolic::estimateTileCost(k, res->config);
    if (!cost.ok)
      return false;
    ::systolic::AcceleratorTimeline timeline(res->config);
    int64_t previousEnd = 0;
    for (const auto &[start, index] : entry.second) {
      const ScheduledSystolicTile &s = r.schedule[index];
      int64_t tileStart = 0, tileEnd = 0;
      if (!timeline.pushTile(k, tileStart, tileEnd))
        return false;
      if (s.startCycle != tileStart || s.endCycle != tileEnd)
        return false;
      if (!::systolic::isPipelined(res->config) &&
          (s.endCycle - s.startCycle != cost.sequential() ||
           s.startCycle < previousEnd))
        return false;
      previousEnd = s.endCycle;
    }
  }
  return true;
}

ScheduleSummary schedule(int64_t m, int64_t n, int64_t k, const FleetSpec &f,
                         bool verify, int64_t span,
                         ExactSystolicOptimizationResult *full = nullptr) {
  ScheduleSummary s;
  const llvm::SmallVector<SystolicArrayResource> fleet = makeFleet(f);
  const SystolicOptimizerOptions opts = optimizerOptions(f);
  auto result = optimizeSystolicBinarySearch(m, n, k, fleet, opts);
  if (mlir::failed(result))
    return s;

  const ::systolic::TileCost t8 =
      ::systolic::estimateTileCost(k, makeConfig(8, f.k8, f));
  const ::systolic::TileCost t4 =
      ::systolic::estimateTileCost(k, makeConfig(4, f.k4, f));
  s.tileCost8 = t8.ok ? t8.sequential() : 0;
  s.tileCost4 = t4.ok ? t4.sequential() : 0;
  s.tileSpan8 = t8.ok ? t8.span() : 0;
  s.tileSpan4 = t4.ok ? t4.span() : 0;

  for (const SystolicExecutionTask &task : result->decomposition) {
    const ::systolic::TileCost &t = task.size == 8 ? t8 : t4;
    if (task.size == 8) {
      ++s.tiles8;
      s.invocations8 += t.invocations;
    } else {
      ++s.tiles4;
      s.invocations4 += t.invocations;
    }
    s.read += t.read;
    s.compute += t.compute;
    s.write += t.write;
  }
  s.makespan = result->makespan;
  s.computeOnly = result->computeOnlyMakespan;
  s.scheduleEnd = result->scheduleEnd;
  s.readBytes = result->totalReadBytes;
  s.sharedReadCycles = result->sharedReadCycles;
  s.checks = result->feasibilityChecks;

  if (verify) {
    s.valid = validSchedule(m, n, k, *result, fleet) ? 1 : 0;
    s.minimal = 1;
    s.monotone = 1;
    if (s.makespan > 0) {
      auto before = isSystolicDeadlineFeasible(m, n, k, fleet, opts,
                                               s.makespan - 1);
      if (mlir::failed(before) || *before)
        s.minimal = 0;
    }
    llvm::SmallVector<int64_t> probes;
    for (int64_t d = 0; d <= span; ++d)
      probes.push_back(s.makespan + d);
    for (int64_t factor : {2, 4, 8})
      probes.push_back(s.makespan * factor);
    for (int64_t deadline : probes) {
      auto ok = isSystolicDeadlineFeasible(m, n, k, fleet, opts, deadline);
      if (mlir::failed(ok) || !*ok)
        s.monotone = 0;
    }
  }
  if (full)
    *full = *result;
  s.ok = true;
  return s;
}

void printSummary(const ScheduleSummary &s) {
  std::printf("schedule makespan=%" PRId64 " compute_only=%" PRId64
              " schedule_end=%" PRId64 " tiles_8x8=%" PRId64
              " tiles_4x4=%" PRId64 " invocations_8x8=%" PRId64
              " invocations_4x4=%" PRId64 " read=%" PRId64
              " compute=%" PRId64 " write=%" PRId64 " read_bytes=%" PRId64
              " shared_read_cycles=%" PRId64 " tile_cost_8x8=%" PRId64
              " tile_cost_4x4=%" PRId64 " checks=%" PRId64
              " tile_span_8x8=%" PRId64 " tile_span_4x4=%" PRId64 "\n",
              s.makespan, s.computeOnly, s.scheduleEnd, s.tiles8, s.tiles4,
              s.invocations8, s.invocations4, s.read, s.compute, s.write,
              s.readBytes, s.sharedReadCycles, s.tileCost8, s.tileCost4,
              s.checks, s.tileSpan8, s.tileSpan4);
}

int runSchedule(const std::map<std::string, std::string> &options,
                bool verify) {
  const FleetSpec f = fleetOptions(options);
  const int64_t m = intOption(options, "--m", 0, true);
  const int64_t n = intOption(options, "--n", 0, true);
  const int64_t k = intOption(options, "--k", 0, true);
  const int64_t span = intOption(options, "--span", 16, false);
  if (m < 1 || n < 1 || k < 1)
    die("--m, --n and --k must be >= 1");

  ExactSystolicOptimizationResult full;
  const ScheduleSummary s = schedule(m, n, k, f, verify, span, &full);
  if (!s.ok) {
    std::printf("schedule FAILED\n");
    return 1;
  }
  printSummary(s);
  if (options.count("--tiles"))
    for (size_t i = 0; i < full.decomposition.size(); ++i) {
      const SystolicExecutionTask &t = full.decomposition[i];
      std::printf("tile row=%" PRId64 " col=%" PRId64 " size=%" PRId64
                  " accelerator=%" PRId64 " start=%" PRId64 " end=%" PRId64
                  "\n",
                  t.row, t.column, t.size, t.acceleratorId,
                  full.schedule[i].startCycle, full.schedule[i].endCycle);
    }
  if (verify) {
    std::printf("check valid=%s minimal=%s monotone=%s\n",
                s.valid ? "yes" : "no", s.minimal ? "yes" : "no",
                s.monotone ? "yes" : "no");
    return (s.valid && s.minimal && s.monotone) ? 0 : 1;
  }
  return 0;
}

// Input lines (blank lines and lines starting with # ignored):
//   id M N K K8 K4 model shared_words_per_cycle [extra_read_overhead]
// model: seq | compute; extra_read_overhead: C, or C8/C4 per geometry.
const char *batchUsage =
    "batch input: id M N K K8 K4 seq|compute shared_words_per_cycle "
    "[extra_read_overhead: C or C8/C4]";

int runBatch(const std::map<std::string, std::string> &options) {
  const bool verify = options.count("--verify") != 0;
  std::ifstream file;
  std::istream *in = &std::cin;
  auto it = options.find("--input");
  if (it != options.end() && it->second != "-") {
    file.open(it->second);
    if (!file)
      die("cannot read " + it->second);
    in = &file;
  }

  std::printf("id,m,n,k,k8,k4,model,shared_words_per_cycle,ok,makespan,"
              "compute_only,schedule_end,tiles_8x8,tiles_4x4,"
              "invocations_8x8,invocations_4x4,read,compute,write,"
              "read_bytes,shared_read_cycles,tile_cost_8x8,tile_cost_4x4,"
              "checks,valid,minimal,monotone\n");
  std::string line;
  int failures = 0;
  while (std::getline(*in, line)) {
    if (line.empty() || line[0] == '#')
      continue;
    std::istringstream fields(line);
    std::string id, sm, sn, sk, s8, s4, model, shared;
    if (!(fields >> id >> sm >> sn >> sk >> s8 >> s4 >> model >> shared))
      die(std::string(batchUsage) + "; got '" + line + "'");
    int64_t m = 0, n = 0, k = 0;
    FleetSpec f;
    std::string extra;
    if (fields >> extra) {
      if (!parseExtra(extra, f))
        die(std::string(batchUsage) + "; got '" + line + "'");
    }
    if (!parseInt(sm, m) || !parseInt(sn, n) || !parseInt(sk, k) ||
        !parseInt(s8, f.k8) || !parseInt(s4, f.k4) ||
        !parseDouble(shared, f.sharedReadWordsPerCycle) || m < 1 || n < 1 ||
        k < 1 || f.k8 < 1 || f.k4 < 1 || f.sharedReadWordsPerCycle < 0.0 ||
        (model != "seq" && model != "compute"))
      die(std::string(batchUsage) + "; got '" + line + "'");
    f.computeOnly = model == "compute";

    const ScheduleSummary s = schedule(m, n, k, f, verify, 8);
    failures += s.ok ? 0 : 1;
    std::printf("%s,%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64
                ",%" PRId64 ",%s,%s,%d,%" PRId64 ",%" PRId64 ",%" PRId64
                ",%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64
                ",%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64
                ",%" PRId64 ",%" PRId64 ",%d,%d,%d\n",
                id.c_str(), m, n, k, f.k8, f.k4, model.c_str(),
                shared.c_str(), s.ok ? 1 : 0, s.makespan, s.computeOnly,
                s.scheduleEnd, s.tiles8, s.tiles4, s.invocations8,
                s.invocations4, s.read, s.compute, s.write, s.readBytes,
                s.sharedReadCycles, s.tileCost8, s.tileCost4, s.checks,
                s.valid, s.minimal, s.monotone);
    std::fflush(stdout);
  }
  return failures == 0 ? 0 : 1;
}

// ---------------------------------------------------------------------------
// timeline
// ---------------------------------------------------------------------------

std::vector<int64_t> parseIntList(const std::string &text, const char *what) {
  std::vector<int64_t> values;
  std::stringstream ss(text);
  std::string item;
  while (std::getline(ss, item, ',')) {
    int64_t v = 0;
    if (!parseInt(item, v))
      die(std::string(what) + " must be a comma-separated list of integers, got '" +
          text + "'");
    values.push_back(v);
  }
  if (values.empty())
    die(std::string(what) + " must name at least one value");
  return values;
}

int runTimeline(const std::map<std::string, std::string> &options) {
  const FleetSpec f = fleetOptions(options);
  const int64_t geometry = intOption(options, "--geometry", 0, true);
  const int64_t kMax = intOption(options, "--k-max", 0, true);
  auto kIt = options.find("--k");
  if (kIt == options.end())
    die("missing --k");
  const std::vector<int64_t> depths = parseIntList(kIt->second, "--k");
  std::vector<int64_t> accepts, feeds;
  auto aIt = options.find("--accept");
  if (aIt != options.end()) {
    accepts = parseIntList(aIt->second, "--accept");
    if (accepts.size() != depths.size())
      die("--accept must name one cycle per invocation of --k");
  }
  auto fIt = options.find("--feed");
  if (fIt != options.end()) {
    feeds = parseIntList(fIt->second, "--feed");
    if (feeds.size() != depths.size())
      die("--feed must name one cycle per invocation of --k");
  }
  std::vector<int64_t> writes;
  auto wIt = options.find("--write");
  if (wIt != options.end()) {
    writes = parseIntList(wIt->second, "--write");
    if (writes.size() != depths.size())
      die("--write must name one cycle per invocation of --k");
  }
  if (geometry < 1 || kMax < 1)
    die("--geometry and --k-max must be >= 1");
  const ::systolic::ArrayConfig cfg = makeConfig(geometry, kMax, f);
  ::systolic::AcceleratorTimeline full(cfg);
  ::systolic::AcceleratorTimeline computeOnly(cfg, /*transfersFree=*/true);
  for (size_t j = 0; j < depths.size(); ++j) {
    ::systolic::InvocationEvents e, c;
    const int64_t accept = accepts.empty() ? 0 : accepts[j];
    const int64_t feed = feeds.empty() ? 0 : feeds[j];
    const int64_t write = writes.empty() ? 0 : writes[j];
    if (!full.push(depths[j], e, accept, feed, write) ||
        !computeOnly.push(depths[j], c, accept, feed))
      die("cost model cannot place invocation " + std::to_string(j) +
          " (k = " + std::to_string(depths[j]) + ") on the timeline");
    std::printf("event j=%zu k=%" PRId64 " accept=%" PRId64 " fill_start=%" PRId64
                " fill_end=%" PRId64 " feed_start=%" PRId64 " compute_end=%" PRId64
                " write_start=%" PRId64 " write_end=%" PRId64 " start_cycle=%" PRId64 "\n",
                j, e.k, e.accept, e.fillStart, e.fillEnd, e.feedStart, e.computeEnd,
                e.writeStart, e.writeEnd, c.feedStart);
  }
  std::printf("stream invocations=%" PRId64 " end=%" PRId64 " compute_only_end=%" PRId64 "\n",
              full.invocations(), full.end(), computeOnly.end());
  return 0;
}

int runFleet(const std::map<std::string, std::string> &options) {
  const FleetSpec f = fleetOptions(options);
  if (f.n8 != ::systolic::fpga::kNum8x8 || f.n4 != ::systolic::fpga::kNum4x4)
    die("fleet prints the bitstream's fleet: 1 x 8x8 + 3 x 4x4");
  std::printf("%s\n%s\n%s\n%s\n",
              ::systolic::fpga::deviceDeclaration("acc_8x8_0", 8, f.k8).c_str(),
              ::systolic::fpga::deviceDeclaration("acc_4x4_0", 4, f.k4).c_str(),
              ::systolic::fpga::deviceDeclaration("acc_4x4_1", 4, f.k4).c_str(),
              ::systolic::fpga::deviceDeclaration("acc_4x4_2", 4, f.k4).c_str());
  return 0;
}

[[noreturn]] void usage(int status) {
  std::fprintf(status ? stderr : stdout,
               "usage: systolic-cost invocation --geometry G --k-max KM "
               "--k K [--stream N] [fleet options]\n"
               "       systolic-cost schedule --m M --n N --k K [fleet "
               "options] [--tiles]\n"
               "       systolic-cost check-monotone --m M --n N --k K "
               "[fleet options] [--span S]\n"
               "       systolic-cost batch [--input FILE] [--verify]\n"
               "       systolic-cost fleet --k8 N --k4 N\n"
               "       systolic-cost timeline --geometry G --k-max KM "
               "--k k0,k1,... [--accept t0,t1,...] [--feed t0,t1,...] "
               "[--write t0,t1,...] [fleet options]\n"
               "fleet options: --k8 N --k4 N --n8 N --n4 N "
               "--compute-only --shared-read-words-per-cycle B "
               "--extra-read-overhead C|C8/C4 --pipelined | "
               "--pipeline-depth D --reduce-cycles R --feed-gap G "
               "--read-bytes-per-cycle B\n"
               "%s\n",
               batchUsage);
  std::exit(status);
}

} // namespace

int main(int argc, char **argv) {
  if (argc < 2)
    usage(2);
  const std::string command = argv[1];
  if (command == "-h" || command == "--help")
    usage(0);
  const std::map<std::string, std::string> options =
      parseOptions(argc, argv, 2);
  if (command == "invocation")
    return runInvocation(options);
  if (command == "schedule")
    return runSchedule(options, false);
  if (command == "check-monotone")
    return runSchedule(options, true);
  if (command == "batch")
    return runBatch(options);
  if (command == "fleet")
    return runFleet(options);
  if (command == "timeline")
    return runTimeline(options);
  usage(2);
}
