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
                                      "--verify"};
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
  if (f.k8 < 1 || f.k4 < 1 || f.n8 < 0 || f.n4 < 0 || f.n8 + f.n4 < 1)
    die("K_MAX must be >= 1 and the fleet non-empty");
  return f;
}

::systolic::ArrayConfig makeConfig(int64_t edge, int64_t kMax,
                                   const FleetSpec &f) {
  ::systolic::ArrayConfig c = ::systolic::fpga::deviceConfig(edge, kMax);
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
              " write_bytes=%" PRId64 "\n",
              geometry, kMax, k, t.invocations, t.read, t.compute, t.write,
              t.sequential(), t.readBytes, t.writeBytes);
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
  // --verify: infeasible at makespan - 1, feasible at every later deadline
  // probed.
  int minimal = -1, monotone = -1;
  // --verify: the schedule is a valid one (validSchedule).
  int valid = -1;
};

/// An independent check of the optimizer's output: every original cell of
/// the m x n output covered exactly once, no tile entirely in the padding,
/// every tile on an accelerator of its own size, holding it for exactly the
/// cost model's tile cost, tiles on one accelerator never overlapping in
/// time, and everything over by the makespan.
bool validSchedule(int64_t m, int64_t n, int64_t k,
                   const ExactSystolicOptimizationResult &r,
                   llvm::ArrayRef<SystolicArrayResource> fleet) {
  if (r.decomposition.size() != r.schedule.size())
    return false;
  std::vector<uint8_t> cover(static_cast<size_t>(m * n), 0);
  std::map<int64_t, std::vector<std::pair<int64_t, int64_t>>> busy;
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
    const ::systolic::TileCost cost = ::systolic::estimateTileCost(k, res->config);
    if (!cost.ok || s.startCycle < 0 ||
        s.endCycle - s.startCycle != cost.sequential() ||
        s.endCycle > r.makespan)
      return false;
    busy[t.acceleratorId].push_back({s.startCycle, s.endCycle});
    for (int64_t a = t.row; a < std::min(m, t.row + t.size); ++a)
      for (int64_t b = t.column; b < std::min(n, t.column + t.size); ++b)
        if (cover[static_cast<size_t>(a * n + b)]++ != 0)
          return false;
  }
  for (uint8_t c : cover)
    if (c != 1)
      return false;
  for (auto &entry : busy) {
    std::sort(entry.second.begin(), entry.second.end());
    for (size_t i = 1; i < entry.second.size(); ++i)
      if (entry.second[i].first < entry.second[i - 1].second)
        return false;
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
              " tile_cost_4x4=%" PRId64 " checks=%" PRId64 "\n",
              s.makespan, s.computeOnly, s.scheduleEnd, s.tiles8, s.tiles4,
              s.invocations8, s.invocations4, s.read, s.compute, s.write,
              s.readBytes, s.sharedReadCycles, s.tileCost8, s.tileCost4,
              s.checks);
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
               "--k K [fleet options]\n"
               "       systolic-cost schedule --m M --n N --k K [fleet "
               "options] [--tiles]\n"
               "       systolic-cost check-monotone --m M --n N --k K "
               "[fleet options] [--span S]\n"
               "       systolic-cost batch [--input FILE] [--verify]\n"
               "       systolic-cost fleet --k8 N --k4 N\n"
               "fleet options: --k8 N --k4 N --n8 N --n4 N "
               "--compute-only --shared-read-words-per-cycle B "
               "--extra-read-overhead C|C8/C4\n"
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
  usage(2);
}
