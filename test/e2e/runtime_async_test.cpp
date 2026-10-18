// runtime_async_test.cpp -- the asynchronous runtime on its own:
//
//   host -> fpga_gemm_begin / fpga_tile_submit / fpga_gemm_wait -> DPTI -> FPGA
//
// without the compiler in front of it, against the board or the RTL
// co-simulation (a bitstream with the completion record: COMPLETION_HEADER=1
// in SYSTOLIC_FPGA_CONFIG). Each mode is one of the experiments of
// docs/ASYNC.md, run as GEMMs of small integers (exact in binary32, so every
// element is checked against an integer reference) decomposed the way the
// compiler decomposes them: 8x8 output tiles on device 0, 4x4 on devices
// 1..3, K folded into k_count invocations of at most K_MAX steps that
// accumulate into the same C.
//
//   A   one device, no staging behind a running job   (the async host path alone)
//   B   every device, no staging                       (cross-accelerator parallelism)
//   C   every device, the next slab staged while a job runs  (transfer/compute overlap)
//   D   two GEMMs submitted interleaved, C's staging     (multi-GEMM)
//   X   the folds of each output tile spread across devices, float data
//       whose sums are inexact, the asynchronous result compared bit for
//       bit with the synchronous entry point's on the same devices: the
//       runtime accumulates in k_index order whatever order results arrive
//   V   the refusals: host-side (-10, -11, -15, -17, nothing sent) and the
//       bitstream's (SYSTOLIC_ASYNC_VIOLATE=1: k = K_MAX + 1 goes out, comes
//       back as a completion record with status BAD_K, the GEMM's wait
//       reports -18 and the link stays in step for the GEMM after it)
//   S   the submission semantics of one GEMM (docs/ASYNC.md section 10):
//       every submit returns without waiting for the tile before it -- the
//       wall-clock of each return against the session's first completion;
//       more than one descriptor on the link before any result; descriptors
//       sent behind an in-flight one of the same array; no submit blocked
//   Q   backpressure: the host queue cut to SYSTOLIC_ASYNC_QUEUE_LIMIT=1 on
//       a GEMM four times the size, so submits must wait for room -- room a
//       completion of ANY older job makes, never the previous job's; every
//       result still right, never more than PIPELINE_DEPTH in flight
//   M   three GEMMs of different shapes submitted round-robin into one
//       session and waited for out of order (the last begun first): each
//       result checked right after its own wait, while the others are still
//       outstanding -- completions are matched to jobs, not to the order
//       GEMMs were opened or waited for
//   P   the observing API (fpga_async_runtime.h, 2026-10-19): fpga_gemm_poll
//       and fpga_tile_poll never block (each call timed while the transfer
//       thread writes slabs), the first poll after the submits sees every
//       tile submitted and none done, a tile is seen queued / staged / in
//       flight / done, fpga_gemm_wait_for with a 1 ms timeout returns -20
//       and leaves the GEMM open, a submit after a wait is -17, release
//       while a tile is outstanding is -21, release after the wait is 0 and
//       the handle is then invalid (-17)
//   F   a transport failure in the middle of a submission (the
//       co-simulation's SYSTOLIC_DPTI_COSIM_FAIL_WRITE=7: the 4th slab's
//       MEM_WRITE header fails): the submits accepted before it stay
//       accepted, the wait reports -4, every tile is failed (none done, none
//       lost), nothing but the 3 slabs crossed the link, a further begin is
//       refused (-14), and after fpga_async_runtime_close a new session runs
//       a GEMM correctly
//
//   runtime-async-test [--mode A,B,C,D,X,V,S,Q,M,P,F | all] [--gemm MxNxK]
//                      [--devices 0,1,2,3] [--stage 0|1] [--print-config] [--list]
//
// Every mode is reported as PASS, MISMATCH, TRANSPORT (link failure),
// SUBMISSION (a tile refused or an unexpected code), CONFIG or NOT RUN (the
// link had failed before it), with the runtime's own counters next to it.
// Exit status as runtime-dispatch-test's: 0 all as expected; 2 configuration,
// 3 transport, 4 submission / unexpected, 1 numerical mismatch.

#include "fpga_async_runtime.h"
#include "fpga_matmul_tiled.h"

#include <algorithm>
#include <chrono>
#include <cinttypes>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

namespace {

using clock_t_ = std::chrono::steady_clock;

uint32_t g_lcg = 20261014u;
int next_int() {
  g_lcg = g_lcg * 1664525u + 1013904223u;
  return static_cast<int>((g_lcg >> 16) % 17) - 8;  // -8 .. 8
}
float next_float() {
  g_lcg = g_lcg * 1664525u + 1013904223u;
  // Values with inexact binary32 sums: k/7 for k in -20..20.
  return static_cast<float>(static_cast<int>((g_lcg >> 16) % 41) - 20) / 7.0f;
}

struct Config {
  int k8 = 0, k4 = 0;
  unsigned slab = 0;
};

struct TileSpec {
  unsigned tile_id, k_index, k_count;
  int row, col, m, n;        // output block
  int k0, k;                 // reduction range
  unsigned device;
};

// The compiler's decomposition of an M x N x K GEMM over a set of devices:
// 8x8 blocks go whole to device 0 when it is in the set and its turn comes,
// else as four 4x4 quarters round-robin over the 4x4 devices; K is folded
// by the device's own K_MAX. `spread_k` sends successive folds of one output
// tile to successive devices of the same geometry instead.
std::vector<TileSpec> decompose(int M, int N, int K, const std::vector<unsigned> &devs,
                                const Config &cfg, bool spread_k) {
  std::vector<unsigned> small;   // the 4x4 devices in the set
  bool have8 = false;
  for (unsigned d : devs) {
    if (d == 0) have8 = true;
    else small.push_back(d);
  }
  std::vector<TileSpec> tiles;
  unsigned tile_id = 0, rr4 = 0, blocks = 0;
  auto fold = [&](unsigned id, int row, int col, int m, int n, unsigned device,
                  const std::vector<unsigned> &alternatives) {
    const int kmax = device == 0 ? cfg.k8 : cfg.k4;
    const unsigned folds = static_cast<unsigned>((K + kmax - 1) / kmax);
    for (unsigned f = 0; f < folds; ++f) {
      const int k0 = static_cast<int>(f) * kmax;
      unsigned dev = device;
      if (spread_k && !alternatives.empty())
        dev = alternatives[f % alternatives.size()];
      tiles.push_back({id, f, folds, row, col, m, n, k0, std::min(kmax, K - k0), dev});
    }
  };
  for (int row = 0; row < M; row += 8) {
    for (int col = 0; col < N; col += 8) {
      const int m8 = std::min(8, M - row), n8 = std::min(8, N - col);
      const bool whole = have8 && (small.empty() || blocks % 2 == 0);
      ++blocks;
      if (whole) {
        fold(tile_id++, row, col, m8, n8, 0, {0});
        continue;
      }
      for (int r = row; r < row + m8; r += 4)
        for (int c = col; c < col + n8; c += 4) {
          const int m = std::min(4, row + m8 - r), n = std::min(4, col + n8 - c);
          std::vector<unsigned> alts = small;
          if (have8) alts.push_back(0);
          const unsigned dev = small.empty() ? 0u : small[rr4++ % small.size()];
          fold(tile_id++, r, c, m, n, dev, alts);
        }
    }
  }
  return tiles;
}

enum Class { PASS, MISMATCH, TRANSPORT, SUBMISSION, CONFIG, NOT_RUN, UNKNOWN };
const char *class_name(Class c) {
  switch (c) {
  case PASS:       return "PASS";
  case MISMATCH:   return "MISMATCH";
  case TRANSPORT:  return "TRANSPORT";
  case SUBMISSION: return "SUBMISSION";
  case CONFIG:     return "CONFIG";
  case NOT_RUN:    return "NOT RUN";
  default:         return "FAIL";
  }
}
Class class_of_rc(int rc) {
  switch (rc) {
  case 0:   return PASS;
  case -16: case -19: return CONFIG;
  case -14: return NOT_RUN;
  case -3: case -4: case -6: case -7: case -8: case -9: return TRANSPORT;
  case -5: case -10: case -11: case -12: case -13: case -15: case -17: case -18:
    return SUBMISSION;
  default:  return UNKNOWN;
  }
}

struct Gemm {
  int M, N, K;
  std::vector<float> A, B, C;
  std::vector<int64_t> ref;   // integer reference (integer data only)
};

Gemm make_gemm(int M, int N, int K, bool integers) {
  Gemm g{M, N, K, {}, {}, {}, {}};
  g.A.resize(static_cast<size_t>(M) * K);
  g.B.resize(static_cast<size_t>(K) * N);
  g.C.resize(static_cast<size_t>(M) * N);
  for (float &v : g.A) v = integers ? static_cast<float>(next_int()) : next_float();
  for (float &v : g.B) v = integers ? static_cast<float>(next_int()) : next_float();
  for (float &v : g.C) v = integers ? static_cast<float>(next_int() * 10) : next_float();
  if (integers) {
    g.ref.assign(g.C.size(), 0);
    for (size_t i = 0; i < g.C.size(); ++i) g.ref[i] = static_cast<int64_t>(g.C[i]);
    for (int i = 0; i < M; ++i)
      for (int j = 0; j < N; ++j)
        for (int k = 0; k < K; ++k)
          g.ref[static_cast<size_t>(i) * N + j] +=
              static_cast<int64_t>(g.A[static_cast<size_t>(i) * K + k]) *
              static_cast<int64_t>(g.B[static_cast<size_t>(k) * N + j]);
  }
  return g;
}

// Contiguous copies of the A and B blocks of one tile, as the compiler
// stages them.
void tile_operands(const Gemm &g, const TileSpec &t, std::vector<float> &a,
                   std::vector<float> &b) {
  a.resize(static_cast<size_t>(t.m) * t.k);
  b.resize(static_cast<size_t>(t.k) * t.n);
  for (int i = 0; i < t.m; ++i)
    for (int kk = 0; kk < t.k; ++kk)
      a[static_cast<size_t>(i) * t.k + kk] = g.A[static_cast<size_t>(t.row + i) * g.K + t.k0 + kk];
  for (int kk = 0; kk < t.k; ++kk)
    for (int j = 0; j < t.n; ++j)
      b[static_cast<size_t>(kk) * t.n + j] = g.B[static_cast<size_t>(t.k0 + kk) * g.N + t.col + j];
}

int mismatches_vs_ref(const Gemm &g, int *first) {
  int bad = 0;
  *first = -1;
  for (size_t i = 0; i < g.C.size(); ++i)
    if (g.C[i] != static_cast<float>(g.ref[i])) {
      if (*first < 0) *first = static_cast<int>(i);
      ++bad;
    }
  return bad;
}

unsigned est_cycles_of(unsigned device) { return device == 0 ? 109u : 101u; }

// Submit every tile of `tiles` for `g` and wait. Returns the wait's code,
// or the first submit's non-zero code (wait is still called).
int run_gemm(Gemm &g, const std::vector<TileSpec> &tiles, int *submit_rc,
             fpga_gemm_t **handle_out = nullptr) {
  fpga_gemm_t *h = fpga_gemm_begin(g.M, g.N, g.K, static_cast<unsigned>(tiles.size()));
  if (!h) {
    *submit_rc = 0;
    return fpga_async_runtime_last_error();
  }
  if (handle_out) *handle_out = h;
  *submit_rc = 0;
  std::vector<float> a, b;
  for (const TileSpec &t : tiles) {
    tile_operands(g, t, a, b);
    const int rc = fpga_tile_submit(h, t.tile_id, t.k_index, t.k_count, t.m, t.k, t.n,
                                    a.data(), b.data(),
                                    &g.C[static_cast<size_t>(t.row) * g.N + t.col], g.N,
                                    t.device, 0, est_cycles_of(t.device));
    if (rc != 0 && *submit_rc == 0) *submit_rc = rc;
  }
  return fpga_gemm_wait(h);
}

struct Options {
  std::vector<char> modes = {'A', 'B', 'C', 'D', 'X', 'V', 'S', 'Q', 'M', 'P', 'F'};
  int M = 16, N = 16, K = 32;
  std::vector<unsigned> devices = {0, 1, 2, 3};
  int stage = -1;     // -1: the mode decides
  bool print_config = false, list = false;
};

bool parse_options(int argc, char **argv, Options &o) {
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto value = [&]() -> const char * { return i + 1 < argc ? argv[++i] : nullptr; };
    if (a == "--print-config") o.print_config = true;
    else if (a == "--list") o.list = true;
    else if (a == "--mode") {
      const char *v = value();
      if (!v) return false;
      o.modes.clear();
      if (std::string(v) == "all") { o.modes = {'A', 'B', 'C', 'D', 'X', 'V', 'S', 'Q', 'M', 'P', 'F'}; continue; }
      for (const char *p = v; *p; ++p)
        if (*p != ',') {
          if (!std::strchr("ABCDXVSQMPF", *p)) return false;
          o.modes.push_back(*p);
        }
    } else if (a == "--gemm") {
      const char *v = value();
      if (!v || std::sscanf(v, "%dx%dx%d", &o.M, &o.N, &o.K) != 3 || o.M < 1 || o.N < 1 || o.K < 1)
        return false;
    } else if (a == "--devices") {
      const char *v = value();
      if (!v) return false;
      o.devices.clear();
      for (const char *p = v; *p; ++p)
        if (*p >= '0' && *p <= '3') o.devices.push_back(static_cast<unsigned>(*p - '0'));
        else if (*p != ',') return false;
      if (o.devices.empty()) return false;
    } else if (a == "--stage") {
      const char *v = value();
      if (!v || (std::string(v) != "0" && std::string(v) != "1")) return false;
      o.stage = v[0] - '0';
    } else return false;
  }
  return true;
}

void set_stage(const Options &o, int mode_default) {
  const int s = o.stage >= 0 ? o.stage : mode_default;
  setenv("SYSTOLIC_ASYNC_STAGE", s ? "1" : "0", 1);
}

std::string stats_line(const fpga_async_stats_t &s, double wall_ms) {
  char buf[512];
  std::snprintf(buf, sizeof buf,
                "tiles=%llu completed=%llu failed=%llu wall_ms=%.1f tiles_per_s=%.1f "
                "reads=%llu empty=%llu overlap2=%llu overlap4=%llu staged_behind=%llu "
                "bytes_to=%llu bytes_from=%llu per_device=%llu/%llu/%llu/%llu "
                "depth=%u depth2_passes=%llu max_inflight=%llu/%llu/%llu/%llu "
                "last_descriptor_us=%llu first_completion_us=%llu "
                "before_first_completion=%llu behind_inflight=%llu credit_waits=%llu "
                "submits_blocked=%llu blocked_us=%llu "
                "submit_us=%llu link_write_us=%llu link_read_us=%llu collect_us=%llu wait_us=%llu polls=%llu",
                s.tiles_submitted, s.tiles_completed, s.tiles_failed, wall_ms,
                wall_ms > 0 ? s.tiles_completed * 1000.0 / wall_ms : 0.0, s.reads,
                s.empty_reads, s.passes_2_inflight, s.passes_4_inflight,
                s.passes_staged_behind, s.bytes_to_fpga, s.bytes_from_fpga,
                s.tiles_per_device[0], s.tiles_per_device[1], s.tiles_per_device[2],
                s.tiles_per_device[3], s.depth, s.passes_depth2,
                s.max_inflight_per_device[0], s.max_inflight_per_device[1],
                s.max_inflight_per_device[2], s.max_inflight_per_device[3],
                s.ns_last_descriptor / 1000, s.ns_first_completion / 1000,
                s.descriptors_before_first_completion, s.descriptors_behind_inflight,
                s.credit_waits, s.submits_blocked, s.ns_submit_blocked / 1000,
                s.ns_submit / 1000, s.ns_link_write / 1000, s.ns_link_read / 1000,
                s.ns_collect / 1000, s.ns_wait / 1000, s.polls);
  return buf;
}

const char *tile_state_name(fpga_tile_state_t s) {
  switch (s) {
  case FPGA_TILE_QUEUED:    return "queued";
  case FPGA_TILE_STAGED:    return "staged";
  case FPGA_TILE_IN_FLIGHT: return "in_flight";
  case FPGA_TILE_HELD:      return "held";
  case FPGA_TILE_DONE:      return "done";
  case FPGA_TILE_FAILED:    return "failed";
  default:                  return "unknown";
  }
}

// Tiles per device of a decomposition.
std::vector<unsigned> tiles_per_device(const std::vector<TileSpec> &tiles) {
  std::vector<unsigned> n(4, 0);
  for (const TileSpec &t : tiles) ++n[t.device];
  return n;
}

} // namespace

int main(int argc, char **argv) {
  std::setvbuf(stdout, nullptr, _IOLBF, 0);
  Options o;
  if (!parse_options(argc, argv, o)) {
    std::fprintf(stderr,
                 "usage: runtime-async-test [--mode A,B,C,D,X,V,S,Q,M,P,F|all] [--gemm MxNxK] "
                 "[--devices 0,1,2,3] [--stage 0|1] [--print-config] [--list]\n");
    return 2;
  }

  const char *path = std::getenv("SYSTOLIC_FPGA_CONFIG");
  if (!path || !*path) {
    std::printf("%sSYSTOLIC_FPGA_CONFIG is not set: name the fpga_config.txt of the "
                "bitstream (COMPLETION_HEADER=1)\n",
                o.print_config ? "CONFIG_ERROR=" : "configuration failure: ");
    if (!o.print_config && !o.list) std::printf("RUNTIME ASYNC: FAIL -- CONFIG\n");
    return 2;
  }
  fpga_runtime_config_t hw{};
  if (fpga_runtime_get_config(&hw) != 0) {
    std::printf("%s%s\n", o.print_config ? "CONFIG_ERROR=" : "configuration failure: ", hw.error);
    if (!o.print_config && !o.list) std::printf("RUNTIME ASYNC: FAIL -- CONFIG\n");
    return 2;
  }
  if (o.print_config) {
    std::printf("K_MAX_8X8=%u\nK_MAX_4X4=%u\nSLAB_BYTES=%u\nCOMPLETION_HEADER=%u\nSOURCE=%s\n",
                hw.k_max_8x8, hw.k_max_4x4, hw.slab_bytes, hw.completion_header, hw.source);
    return 0;
  }
  if (!hw.completion_header) {
    std::printf("configuration failure: %s has no COMPLETION_HEADER=1; the asynchronous "
                "runtime needs the completion record\n", hw.source);
    if (!o.list) std::printf("RUNTIME ASYNC: FAIL -- CONFIG\n");
    return 2;
  }
  Config cfg{static_cast<int>(hw.k_max_8x8), static_cast<int>(hw.k_max_4x4), hw.slab_bytes};

  std::printf("configuration: K_MAX_8X8 = %d, K_MAX_4X4 = %d, %u-byte operand slab, "
              "completion record (%s)\n", cfg.k8, cfg.k4, cfg.slab, hw.source);
  std::printf("GEMM %dx%dx%d; devices", o.M, o.N, o.K);
  for (unsigned d : o.devices) std::printf(" %u", d);
  std::printf("\n\n");

  struct Plan { char mode; std::string what; int stage; };
  std::vector<Plan> plans;
  for (char m : o.modes) {
    switch (m) {
    case 'A': plans.push_back({m, "one device (" + std::to_string(o.devices[0]) + "), no staging", 0}); break;
    case 'B': plans.push_back({m, "every device, no staging", 0}); break;
    case 'C': plans.push_back({m, "every device, slab staged behind the running job", 1}); break;
    case 'D': plans.push_back({m, "two GEMMs interleaved, staging on", 1}); break;
    case 'X': plans.push_back({m, "folds spread across devices, float data, bit-exact vs the synchronous entry point", 1}); break;
    case 'V': plans.push_back({m, "refusals: host-side codes, then the bitstream's BAD_K record", 1}); break;
    case 'S': plans.push_back({m, "submission semantics: every submit returns before the first completion, descriptors queued behind in-flight ones", 1}); break;
    case 'Q': plans.push_back({m, "backpressure: SYSTOLIC_ASYNC_QUEUE_LIMIT=1 on a 4x larger GEMM, submits wait for room, never for the previous job", 1}); break;
    case 'M': plans.push_back({m, "three GEMMs of different shapes interleaved, waited for out of order, each checked while the others are outstanding", 1}); break;
    case 'P': plans.push_back({m, "poll / timed wait / release semantics: never blocking, states observed, -20 / -17 / -21 where due", 1}); break;
    case 'F': plans.push_back({m, "transport failure mid-submission (SYSTOLIC_DPTI_COSIM_FAIL_WRITE=7): nothing lost, -4, -14 after, recovery after close", 1}); break;
    }
  }
  if (o.list) {
    for (const Plan &p : plans) std::printf("mode %c  %s\n", p.mode, p.what.c_str());
    return 0;
  }

  int count[UNKNOWN + 1] = {};
  int first_rc[UNKNOWN + 1] = {};
  std::string first_what[UNKNOWN + 1];
  auto record = [&](Class c, int rc, const std::string &what) {
    if (count[c]++ == 0) { first_rc[c] = rc; first_what[c] = what; }
  };
  auto report = [&](const Plan &p, Class c, int rc, const std::string &detail) {
    record(c, rc, std::string("mode ") + p.mode);
    std::printf("mode %c (%s): %s%s%s\n", p.mode, p.what.c_str(), class_name(c),
                rc ? (" rc=" + std::to_string(rc)).c_str() : "",
                detail.empty() ? "" : ("\n    " + detail).c_str());
  };

  bool link_failed = false;
  for (const Plan &p : plans) {
    if (link_failed) {
      report(p, NOT_RUN, -14, "the link failed in an earlier mode");
      continue;
    }
    // Every mode is its own session: the runtime reads SYSTOLIC_ASYNC_STAGE
    // (and the rest of its settings) when a session opens, and its timing
    // line, if asked for, comes out per mode at the close.
    fpga_async_runtime_close();
    set_stage(o, p.stage);
    fpga_async_runtime_reset_stats();
    const clock_t_::time_point t0 = clock_t_::now();

    if (p.mode == 'A' || p.mode == 'B' || p.mode == 'C') {
      Gemm g = make_gemm(o.M, o.N, o.K, true);
      const std::vector<unsigned> devs =
          p.mode == 'A' ? std::vector<unsigned>{o.devices[0]} : o.devices;
      const std::vector<TileSpec> tiles = decompose(o.M, o.N, o.K, devs, cfg, false);
      int submit_rc = 0;
      const int rc = run_gemm(g, tiles, &submit_rc);
      const double wall = std::chrono::duration<double, std::milli>(clock_t_::now() - t0).count();
      fpga_async_stats_t s; fpga_async_runtime_get_stats(&s);
      int first = -1;
      const int bad = rc == 0 ? mismatches_vs_ref(g, &first) : 0;
      Class c = submit_rc ? class_of_rc(submit_rc) : rc ? class_of_rc(rc) : bad ? MISMATCH : PASS;
      if (c == NOT_RUN) c = TRANSPORT;
      std::string detail = stats_line(s, wall);
      if (bad) {
        char b[160];
        std::snprintf(b, sizeof b, "\n    %d mismatch(es); first C[%d][%d] = %.9g, reference %" PRId64,
                      bad, first / o.N, first % o.N, static_cast<double>(g.C[first]), g.ref[first]);
        detail += b;
      }
      if (c == TRANSPORT) link_failed = true;
      report(p, c, submit_rc ? submit_rc : rc, detail);
    } else if (p.mode == 'D') {
      Gemm g0 = make_gemm(o.M, o.N, o.K, true), g1 = make_gemm(o.M, o.N, o.K, true);
      const std::vector<TileSpec> t0s = decompose(o.M, o.N, o.K, o.devices, cfg, false);
      const std::vector<TileSpec> t1s = decompose(o.M, o.N, o.K, o.devices, cfg, false);
      fpga_gemm_t *h0 = fpga_gemm_begin(g0.M, g0.N, g0.K, static_cast<unsigned>(t0s.size()));
      fpga_gemm_t *h1 = fpga_gemm_begin(g1.M, g1.N, g1.K, static_cast<unsigned>(t1s.size()));
      int submit_rc = 0, rc = 0;
      if (!h0 || !h1) {
        rc = fpga_async_runtime_last_error();
      } else {
        std::vector<float> a, b;
        const size_t n = std::max(t0s.size(), t1s.size());
        for (size_t i = 0; i < n; ++i) {
          for (int which = 0; which < 2; ++which) {
            const std::vector<TileSpec> &ts = which ? t1s : t0s;
            Gemm &g = which ? g1 : g0;
            if (i >= ts.size()) continue;
            const TileSpec &t = ts[i];
            tile_operands(g, t, a, b);
            const int src = fpga_tile_submit(which ? h1 : h0, t.tile_id, t.k_index, t.k_count,
                                             t.m, t.k, t.n, a.data(), b.data(),
                                             &g.C[static_cast<size_t>(t.row) * g.N + t.col], g.N,
                                             t.device, 0, est_cycles_of(t.device));
            if (src != 0 && submit_rc == 0) submit_rc = src;
          }
        }
        const int rc0 = fpga_gemm_wait(h0);
        const int rc1 = fpga_gemm_wait(h1);
        rc = rc0 ? rc0 : rc1;
      }
      const double wall = std::chrono::duration<double, std::milli>(clock_t_::now() - t0).count();
      fpga_async_stats_t s; fpga_async_runtime_get_stats(&s);
      int f0 = -1, f1 = -1;
      const int bad = rc == 0 ? mismatches_vs_ref(g0, &f0) + mismatches_vs_ref(g1, &f1) : 0;
      Class c = submit_rc ? class_of_rc(submit_rc) : rc ? class_of_rc(rc) : bad ? MISMATCH : PASS;
      if (c == NOT_RUN) c = TRANSPORT;
      if (c == TRANSPORT) link_failed = true;
      std::string detail = stats_line(s, wall);
      if (bad) detail += "\n    " + std::to_string(bad) + " mismatch(es) over the two GEMMs";
      report(p, c, submit_rc ? submit_rc : rc, detail);
    } else if (p.mode == 'X') {
      // The synchronous entry point first, tile by tile in k_index order on
      // the same devices, then the asynchronous runtime; the two C must agree
      // bit for bit. Both runtimes open their own session, one after the other.
      Gemm g = make_gemm(o.M, o.N, o.K, false);
      Gemm g_sync = g;
      const std::vector<TileSpec> tiles = decompose(o.M, o.N, o.K, o.devices, cfg, true);
      fpga_async_runtime_close();   // the asynchronous session, if open
      int rc = 0;
      {
        std::vector<float> a, b, c;
        for (const TileSpec &t : tiles) {
          tile_operands(g_sync, t, a, b);
          c.resize(static_cast<size_t>(t.m) * t.n);
          for (int i = 0; i < t.m; ++i)
            for (int j = 0; j < t.n; ++j)
              c[static_cast<size_t>(i) * t.n + j] = g_sync.C[static_cast<size_t>(t.row + i) * g_sync.N + t.col + j];
          rc = fpga_matmul_tiled_auto_scheduled(t.m, t.k, t.n, a.data(), b.data(), c.data(),
                                                t.device, 0, est_cycles_of(t.device));
          if (rc != 0) break;
          for (int i = 0; i < t.m; ++i)
            for (int j = 0; j < t.n; ++j)
              g_sync.C[static_cast<size_t>(t.row + i) * g_sync.N + t.col + j] = c[static_cast<size_t>(i) * t.n + j];
        }
        fpga_runtime_close();
      }
      int submit_rc = 0;
      if (rc == 0) {
        set_stage(o, p.stage);
        fpga_async_runtime_reset_stats();
        rc = run_gemm(g, tiles, &submit_rc);
      }
      const double wall = std::chrono::duration<double, std::milli>(clock_t_::now() - t0).count();
      fpga_async_stats_t s; fpga_async_runtime_get_stats(&s);
      int bad = 0, first = -1;
      if (rc == 0 && submit_rc == 0)
        for (size_t i = 0; i < g.C.size(); ++i) {
          uint32_t x, y;
          std::memcpy(&x, &g.C[i], 4);
          std::memcpy(&y, &g_sync.C[i], 4);
          if (x != y) { if (first < 0) first = static_cast<int>(i); ++bad; }
        }
      Class c = submit_rc ? class_of_rc(submit_rc) : rc ? class_of_rc(rc) : bad ? MISMATCH : PASS;
      if (c == NOT_RUN) c = TRANSPORT;
      if (c == TRANSPORT) link_failed = true;
      std::string detail = stats_line(s, wall);
      if (bad) {
        char b[160];
        std::snprintf(b, sizeof b, "\n    %d element(s) differ from the synchronous result; first C[%d][%d]: async %.9g, sync %.9g",
                      bad, first / o.N, first % o.N, static_cast<double>(g.C[first]),
                      static_cast<double>(g_sync.C[first]));
        detail += b;
      }
      report(p, c, submit_rc ? submit_rc : rc, detail);
    } else if (p.mode == 'S') {
      // One GEMM over every device. The wall-clock at which each submit
      // returned, against the runtime's own stamp of the first completion
      // (both relative to the first submit), and the runtime's counters of
      // what went out before any result and behind an in-flight descriptor.
      Gemm g = make_gemm(o.M, o.N, o.K, true);
      const std::vector<TileSpec> tiles = decompose(o.M, o.N, o.K, o.devices, cfg, false);
      const std::vector<unsigned> per_dev = tiles_per_device(tiles);
      std::vector<unsigned long long> returned_us;
      int submit_rc = 0, rc = 0;
      fpga_gemm_t *h = fpga_gemm_begin(g.M, g.N, g.K, static_cast<unsigned>(tiles.size()));
      if (!h) {
        rc = fpga_async_runtime_last_error();
      } else {
        std::vector<float> a, b;
        clock_t_::time_point first;
        for (size_t i = 0; i < tiles.size(); ++i) {
          const TileSpec &t = tiles[i];
          tile_operands(g, t, a, b);
          const clock_t_::time_point ts = clock_t_::now();
          if (i == 0) first = ts;
          const int src = fpga_tile_submit(h, t.tile_id, t.k_index, t.k_count, t.m, t.k, t.n,
                                           a.data(), b.data(),
                                           &g.C[static_cast<size_t>(t.row) * g.N + t.col], g.N,
                                           t.device, 0, est_cycles_of(t.device));
          returned_us.push_back(std::chrono::duration_cast<std::chrono::microseconds>(
                                    clock_t_::now() - first).count());
          if (src != 0 && submit_rc == 0) submit_rc = src;
        }
        rc = fpga_gemm_wait(h);
      }
      const double wall = std::chrono::duration<double, std::milli>(clock_t_::now() - t0).count();
      fpga_async_stats_t s; fpga_async_runtime_get_stats(&s);
      int first = -1;
      const int bad = rc == 0 ? mismatches_vs_ref(g, &first) : 0;
      Class c = submit_rc ? class_of_rc(submit_rc) : rc ? class_of_rc(rc) : bad ? MISMATCH : PASS;
      if (c == NOT_RUN) c = TRANSPORT;
      if (c == TRANSPORT) link_failed = true;
      std::string detail = stats_line(s, wall);
      if (c == PASS) {
        // What the batch semantics must show on a GEMM of >= 2 tiles with
        // a credit of >= 2: at least two descriptors on the link before any
        // result, at least one sent behind an in-flight one, no submit
        // blocked (the host queue holds 64), and every submit returned
        // before the wait -- the per-return times say how many returned
        // before the first result.
        const unsigned long long first_c_us = s.ns_first_completion / 1000;
        size_t before = 0;
        for (unsigned long long us : returned_us) if (us < first_c_us) ++before;
        unsigned expect_batch = 0, max_dev_tiles = 0;
        for (unsigned n : per_dev) {
          expect_batch += std::min(n, s.depth);
          max_dev_tiles = std::max(max_dev_tiles, n);
        }
        char b[512];
        std::snprintf(b, sizeof b,
                      "\n    tiles=%zu per_device=%u/%u/%u/%u; submits returned before the first completion: %zu "
                      "(first completion at %llu us after the first submit, last submit returned at %llu us); "
                      "descriptors before the first completion: %llu (sum of min(tiles_d, depth) = %u); "
                      "sent behind an in-flight descriptor: %llu; waited for a credit: %llu; submits blocked: %llu",
                      tiles.size(), per_dev[0], per_dev[1], per_dev[2], per_dev[3], before, first_c_us,
                      returned_us.empty() ? 0ull : returned_us.back(),
                      s.descriptors_before_first_completion, expect_batch,
                      s.descriptors_behind_inflight, s.credit_waits, s.submits_blocked);
        detail += b;
        if (tiles.size() >= 2 && s.depth >= 2 && max_dev_tiles >= 2) {
          std::string why;
          if (s.descriptors_before_first_completion < 2)
            why += "; only " + std::to_string(s.descriptors_before_first_completion) + " descriptor(s) on the link before the first completion";
          if (s.descriptors_behind_inflight < 1)
            why += "; no descriptor was sent behind an in-flight one";
          if (s.submits_blocked != 0 && tiles.size() <= 64)
            why += "; " + std::to_string(s.submits_blocked) + " submit(s) blocked on a GEMM that fits the host queue";
          if (before < std::min<size_t>(tiles.size(), 2))
            why += "; fewer than two submits returned before the first completion";
          if (!why.empty()) {
            c = UNKNOWN;
            detail += "\n    SERIALIZED" + why;
          }
        }
      }
      report(p, c, submit_rc ? submit_rc : rc, detail);
    } else if (p.mode == 'Q') {
      // The host queue cut to one packed tile: once every operand slot of a
      // device holds a slab and every credit is in flight, the next submit
      // of that device has to wait -- for room, which the completion of the
      // OLDEST in-flight job of any device makes. A GEMM four times the
      // size, so that every device runs out of slots.
      fpga_async_runtime_close();
      setenv("SYSTOLIC_ASYNC_QUEUE_LIMIT", "1", 1);
      set_stage(o, p.stage);
      fpga_async_runtime_reset_stats();
      const int M4 = 2 * o.M, N4 = 2 * o.N;
      Gemm g = make_gemm(M4, N4, o.K, true);
      const std::vector<TileSpec> tiles = decompose(M4, N4, o.K, o.devices, cfg, false);
      const std::vector<unsigned> per_dev = tiles_per_device(tiles);
      int submit_rc = 0;
      const int rc = run_gemm(g, tiles, &submit_rc);
      const double wall = std::chrono::duration<double, std::milli>(clock_t_::now() - t0).count();
      fpga_async_stats_t s; fpga_async_runtime_get_stats(&s);
      unsetenv("SYSTOLIC_ASYNC_QUEUE_LIMIT");
      fpga_async_runtime_close();
      int first = -1;
      const int bad = rc == 0 ? mismatches_vs_ref(g, &first) : 0;
      Class c = submit_rc ? class_of_rc(submit_rc) : rc ? class_of_rc(rc) : bad ? MISMATCH : PASS;
      if (c == NOT_RUN) c = TRANSPORT;
      if (c == TRANSPORT) link_failed = true;
      std::string detail = stats_line(s, wall);
      char b[400];
      std::snprintf(b, sizeof b,
                    "\n    GEMM %dx%dx%d, tiles=%zu per_device=%u/%u/%u/%u; submits blocked for room: %llu (%llu us); "
                    "descriptors sent on a freed credit: %llu; most in flight per device %llu/%llu/%llu/%llu of depth %u",
                    M4, N4, o.K, tiles.size(), per_dev[0], per_dev[1], per_dev[2], per_dev[3],
                    s.submits_blocked, s.ns_submit_blocked / 1000, s.credit_waits,
                    s.max_inflight_per_device[0], s.max_inflight_per_device[1],
                    s.max_inflight_per_device[2], s.max_inflight_per_device[3], s.depth);
      detail += b;
      if (c == PASS) {
        std::string why;
        unsigned slots_exceeded = 0;
        for (unsigned n : per_dev) if (n > s.depth + 1) ++slots_exceeded;
        if (slots_exceeded > 0 && s.submits_blocked == 0)
          why += "; no submit blocked although a device had more tiles than operand slots";
        if (s.tiles_completed != tiles.size())
          why += "; completed " + std::to_string(s.tiles_completed) + " of " + std::to_string(tiles.size());
        for (int d = 0; d < 4; ++d)
          if (s.max_inflight_per_device[d] > s.depth)
            why += "; device " + std::to_string(d) + " had " + std::to_string(s.max_inflight_per_device[d]) + " in flight, over the credit";
        if (!why.empty()) {
          c = UNKNOWN;
          detail += "\n    BACKPRESSURE" + why;
        }
      }
      report(p, c, submit_rc ? submit_rc : rc, detail);
    } else if (p.mode == 'M') {
      // Three GEMMs of different shapes (so tile ids, k_counts and devices
      // overlap in different ways), submitted round-robin, waited for out
      // of order; each checked right after its own wait.
      const int M0 = o.M, N0 = o.N, K0 = o.K;
      const int M1 = o.M, N1 = o.N, K1 = 2 * o.K;
      const int M2 = std::min(o.M, 8), N2 = std::min(o.N, 8), K2 = o.K;
      Gemm gs[3] = {make_gemm(M0, N0, K0, true), make_gemm(M1, N1, K1, true), make_gemm(M2, N2, K2, true)};
      std::vector<TileSpec> ts[3] = {decompose(M0, N0, K0, o.devices, cfg, false),
                                     decompose(M1, N1, K1, o.devices, cfg, false),
                                     decompose(M2, N2, K2, o.devices, cfg, false)};
      fpga_gemm_t *hs[3] = {nullptr, nullptr, nullptr};
      int submit_rc = 0, rc = 0;
      std::string detail;
      for (int w = 0; w < 3; ++w) {
        hs[w] = fpga_gemm_begin(gs[w].M, gs[w].N, gs[w].K, static_cast<unsigned>(ts[w].size()));
        if (!hs[w] && rc == 0) rc = fpga_async_runtime_last_error();
      }
      int bad_total = 0;
      if (rc == 0) {
        std::vector<float> a, b;
        const size_t n = std::max({ts[0].size(), ts[1].size(), ts[2].size()});
        for (size_t i = 0; i < n; ++i)
          for (int w = 0; w < 3; ++w) {
            if (i >= ts[w].size()) continue;
            const TileSpec &t = ts[w][i];
            Gemm &g = gs[w];
            tile_operands(g, t, a, b);
            const int src = fpga_tile_submit(hs[w], t.tile_id, t.k_index, t.k_count, t.m, t.k, t.n,
                                             a.data(), b.data(),
                                             &g.C[static_cast<size_t>(t.row) * g.N + t.col], g.N,
                                             t.device, 0, est_cycles_of(t.device));
            if (src != 0 && submit_rc == 0) submit_rc = src;
          }
        // Waited for in the order 2, 0, 1: the last GEMM begun first.
        for (int w : {2, 0, 1}) {
          const int wrc = fpga_gemm_wait(hs[w]);
          if (wrc != 0 && rc == 0) rc = wrc;
          fpga_async_stats_t mid; fpga_async_runtime_get_stats(&mid);
          int first = -1;
          const int bad = wrc == 0 ? mismatches_vs_ref(gs[w], &first) : -1;
          if (bad > 0) bad_total += bad;
          char b2[240];
          std::snprintf(b2, sizeof b2, "%sgemm %d (%dx%dx%d, %zu tiles): wait rc=%d, %s checked right after its wait with %llu of %zu tiles of the session completed",
                        detail.empty() ? "" : "\n    ", w, gs[w].M, gs[w].N, gs[w].K, ts[w].size(), wrc,
                        bad == 0 ? "every element right" : bad < 0 ? "not checked" : (std::to_string(bad) + " mismatch(es)").c_str(),
                        mid.tiles_completed, ts[0].size() + ts[1].size() + ts[2].size());
          detail += b2;
        }
      }
      const double wall = std::chrono::duration<double, std::milli>(clock_t_::now() - t0).count();
      fpga_async_stats_t s; fpga_async_runtime_get_stats(&s);
      Class c = submit_rc ? class_of_rc(submit_rc) : rc ? class_of_rc(rc) : bad_total ? MISMATCH : PASS;
      if (c == NOT_RUN) c = TRANSPORT;
      if (c == TRANSPORT) link_failed = true;
      detail = stats_line(s, wall) + "\n    " + detail;
      report(p, c, submit_rc ? submit_rc : rc, detail);
    } else if (p.mode == 'P') {
      std::string detail;
      bool ok = true;
      auto expect = [&](const char *what, long long got, long long want) {
        char b[200];
        std::snprintf(b, sizeof b, "%s%s: %lld (expected %lld)", detail.empty() ? "" : "\n    ",
                      what, got, want);
        detail += b;
        if (got != want) ok = false;
      };
      Gemm g = make_gemm(o.M, o.N, o.K, true);
      const std::vector<TileSpec> tiles = decompose(o.M, o.N, o.K, o.devices, cfg, false);
      fpga_gemm_t *h = fpga_gemm_begin(g.M, g.N, g.K, static_cast<unsigned>(tiles.size()));
      int submit_rc = 0;
      if (!h) {
        expect("begin", fpga_async_runtime_last_error(), 0);
      } else {
        std::vector<float> a, b;
        double max_poll_us = 0, max_tile_poll_us = 0;
        std::vector<std::string> seen;   // the states tile 0 fold 0 was seen in, in order
        auto observe = [&]() {
          fpga_gemm_status_t st;
          const clock_t_::time_point t1 = clock_t_::now();
          fpga_gemm_poll(h, &st);
          max_poll_us = std::max(max_poll_us, std::chrono::duration<double, std::micro>(clock_t_::now() - t1).count());
          fpga_tile_status_t ts;
          const clock_t_::time_point t2 = clock_t_::now();
          fpga_tile_poll(h, tiles[0].tile_id, tiles[0].k_index, &ts);
          max_tile_poll_us = std::max(max_tile_poll_us, std::chrono::duration<double, std::micro>(clock_t_::now() - t2).count());
          const char *name = tile_state_name(ts.state);
          if (seen.empty() || seen.back() != name) seen.push_back(name);
          return st;
        };
        for (const TileSpec &t : tiles) {
          tile_operands(g, t, a, b);
          const int rc = fpga_tile_submit(h, t.tile_id, t.k_index, t.k_count, t.m, t.k, t.n,
                                          a.data(), b.data(),
                                          &g.C[static_cast<size_t>(t.row) * g.N + t.col], g.N,
                                          t.device, 0, est_cycles_of(t.device));
          if (rc != 0 && submit_rc == 0) submit_rc = rc;
          observe();   // polls interleaved with the submits: never blocking
        }
        fpga_gemm_status_t first = observe();
        expect("first poll after the submits: submitted", first.submitted, static_cast<long long>(tiles.size()));
        expect("first poll after the submits: done", first.done, 0);
        expect("first poll after the submits: completed", first.completed, 0);
        // A 1 ms wait cannot see 20 tiles through the co-simulation: -20,
        // and the GEMM stays open.
        expect("wait_for(1 ms)", fpga_gemm_wait_for(h, 1), -20);
        fpga_gemm_status_t after_timeout;
        expect("poll after the timed-out wait", fpga_gemm_poll(h, &after_timeout), 0);
        expect("submit after a wait", fpga_tile_submit(h, 99, 0, 1, 4, 1, 4, a.data(), b.data(), g.C.data(), g.N, 1, 0, 101), -17);
        // Poll until done, bounded.
        fpga_gemm_status_t st{};
        for (int i = 0; i < 600000; ++i) {
          st = observe();
          if (st.done) break;
          std::this_thread::sleep_for(std::chrono::microseconds(200));
        }
        expect("polled to done", st.done, 1);
        expect("polled: completed", st.completed, static_cast<long long>(tiles.size()));
        expect("polled: failed", st.failed, 0);
        expect("polled: in_flight + staged + queued at the end", st.in_flight + st.staged + st.queued + st.held, 0);
        expect("wait_for after done", fpga_gemm_wait_for(h, 1), 0);
        int first_bad = -1;
        expect("mismatches", mismatches_vs_ref(g, &first_bad), 0);
        expect("release after done", fpga_gemm_release(h), 0);
        expect("poll after release", fpga_gemm_poll(h, &st), -17);
        expect("release again", fpga_gemm_release(h), -17);
        char b2[400];
        std::snprintf(b2, sizeof b2, "\n    tile 0 fold 0 was seen: %s; longest fpga_gemm_poll %.1f us, longest fpga_tile_poll %.1f us",
                      [&] { std::string x; for (auto &n : seen) x += (x.empty() ? "" : " -> ") + n; return x; }().c_str(),
                      max_poll_us, max_tile_poll_us);
        detail += b2;
        if (max_poll_us > 5000 || max_tile_poll_us > 5000) {
          ok = false;
          detail += "\n    a poll took more than 5 ms: it blocked on the link";
        }
        if (seen.empty() || seen.back() != std::string("done")) {
          ok = false;
          detail += "\n    tile 0 fold 0 was never seen done";
        }
        // Release with a tile outstanding: refused, nothing released.
        Gemm g2 = make_gemm(8, 8, cfg.k4, true);
        const std::vector<TileSpec> t2 = decompose(8, 8, cfg.k4, {1, 2, 3}, cfg, false);
        fpga_gemm_t *h2 = fpga_gemm_begin(g2.M, g2.N, g2.K, static_cast<unsigned>(t2.size()));
        if (!h2) {
          expect("second begin", fpga_async_runtime_last_error(), 0);
        } else {
          for (const TileSpec &t : t2) {
            tile_operands(g2, t, a, b);
            const int rc = fpga_tile_submit(h2, t.tile_id, t.k_index, t.k_count, t.m, t.k, t.n,
                                            a.data(), b.data(),
                                            &g2.C[static_cast<size_t>(t.row) * g2.N + t.col], g2.N,
                                            t.device, 0, est_cycles_of(t.device));
            if (rc != 0 && submit_rc == 0) submit_rc = rc;
          }
          fpga_gemm_status_t s2;
          fpga_gemm_poll(h2, &s2);
          if (!s2.done)
            expect("release with tiles outstanding", fpga_gemm_release(h2), -21);
          else
            detail += "\n    (the second GEMM completed before the release test could run)";
          expect("wait_for(forever) of the second GEMM", fpga_gemm_wait_for(h2, FPGA_ASYNC_WAIT_FOREVER), 0);
          expect("its mismatches", mismatches_vs_ref(g2, &first_bad), 0);
          expect("its release", fpga_gemm_release(h2), 0);
        }
      }
      const double wall = std::chrono::duration<double, std::milli>(clock_t_::now() - t0).count();
      fpga_async_stats_t s; fpga_async_runtime_get_stats(&s);
      detail += "\n    " + stats_line(s, wall);
      Class c = submit_rc ? class_of_rc(submit_rc) : ok ? PASS : UNKNOWN;
      if (c == TRANSPORT || c == NOT_RUN) { link_failed = true; c = TRANSPORT; }
      report(p, c, submit_rc, detail);
    } else if (p.mode == 'F') {
      std::string detail;
      bool ok = true;
      auto expect = [&](const char *what, long long got, long long want) {
        char b[200];
        std::snprintf(b, sizeof b, "%s%s: %lld (expected %lld)", detail.empty() ? "" : "\n    ",
                      what, got, want);
        detail += b;
        if (got != want) ok = false;
      };
      // A fresh session whose 7th write fails: the header of the 4th
      // operand slab (each slab is a header write and a payload write).
      fpga_async_runtime_close();
      setenv("SYSTOLIC_DPTI_COSIM_FAIL_WRITE", "7", 1);
      set_stage(o, p.stage);
      fpga_async_runtime_reset_stats();
      Gemm g = make_gemm(o.M, o.N, o.K, true);
      const std::vector<float> c0 = g.C;
      const std::vector<TileSpec> tiles = decompose(o.M, o.N, o.K, o.devices, cfg, false);
      fpga_gemm_t *h = fpga_gemm_begin(g.M, g.N, g.K, static_cast<unsigned>(tiles.size()));
      if (!h) {
        expect("begin", fpga_async_runtime_last_error(), 0);
      } else {
        std::vector<float> a, b;
        unsigned accepted = 0, refused = 0;
        int first_refusal = 0;
        for (const TileSpec &t : tiles) {
          tile_operands(g, t, a, b);
          const int rc = fpga_tile_submit(h, t.tile_id, t.k_index, t.k_count, t.m, t.k, t.n,
                                          a.data(), b.data(),
                                          &g.C[static_cast<size_t>(t.row) * g.N + t.col], g.N,
                                          t.device, 0, est_cycles_of(t.device));
          if (rc == 0) ++accepted; else { ++refused; if (!first_refusal) first_refusal = rc; }
        }
        const int wrc = fpga_gemm_wait_for(h, FPGA_ASYNC_WAIT_FOREVER);
        expect("wait after the failure", wrc, -4);
        fpga_gemm_status_t st;
        expect("poll", fpga_gemm_poll(h, &st), 0);
        expect("link_failed", st.link_failed, 1);
        expect("completed (no result can have come back)", st.completed, 0);
        expect("failed == submitted (nothing lost)", st.failed, st.submitted);
        expect("submitted == accepted submits", st.submitted, accepted);
        if (refused) expect("refused submits after the failure: code", first_refusal, -14);
        unsigned failed_4 = 0, failed_14 = 0, other = 0;
        for (const TileSpec &t : tiles) {
          fpga_tile_status_t ts;
          fpga_tile_poll(h, t.tile_id, t.k_index, &ts);
          if (ts.state == FPGA_TILE_FAILED && ts.error == -4) ++failed_4;
          else if (ts.state == FPGA_TILE_FAILED && ts.error == -14) ++failed_14;
          else if (ts.state != FPGA_TILE_UNKNOWN) ++other;
        }
        expect("tiles polled failed with -4 (accepted before the failure)", failed_4, accepted);
        expect("tiles in any other state", other, 0);
        fpga_async_stats_t s; fpga_async_runtime_get_stats(&s);
        expect("bytes on the link: three slabs", static_cast<long long>(s.bytes_to_fpga), 3LL * (9 + cfg.slab));
        expect("bytes back", static_cast<long long>(s.bytes_from_fpga), 0);
        expect("release after the failure", fpga_gemm_release(h), 0);
        fpga_gemm_t *h2 = fpga_gemm_begin(8, 8, cfg.k4, 1);
        expect("a begin on the failed link is NULL", h2 == nullptr, 1);
        expect("its reason", fpga_async_runtime_last_error(), -14);
        int touched = 0;
        for (size_t i = 0; i < g.C.size(); ++i) if (g.C[i] != c0[i]) ++touched;
        expect("elements of C written by the failed GEMM", touched, 0);
      }
      // Recovery: close, lift the injection, run a GEMM.
      fpga_async_runtime_close();
      unsetenv("SYSTOLIC_DPTI_COSIM_FAIL_WRITE");
      set_stage(o, p.stage);
      fpga_async_runtime_reset_stats();
      {
        Gemm g3 = make_gemm(o.M, o.N, o.K, true);
        const std::vector<TileSpec> t3 = decompose(o.M, o.N, o.K, o.devices, cfg, false);
        int submit_rc = 0;
        const int rc = run_gemm(g3, t3, &submit_rc);
        expect("after close: a GEMM's submits", submit_rc, 0);
        expect("after close: its wait", rc, 0);
        int first = -1;
        if (rc == 0 && submit_rc == 0) expect("after close: its mismatches", mismatches_vs_ref(g3, &first), 0);
        if (rc != 0 && class_of_rc(rc) == TRANSPORT) link_failed = true;
      }
      const double wall = std::chrono::duration<double, std::milli>(clock_t_::now() - t0).count();
      fpga_async_stats_t s; fpga_async_runtime_get_stats(&s);
      detail += "\n    " + stats_line(s, wall);
      report(p, ok ? PASS : (link_failed ? TRANSPORT : UNKNOWN), ok ? 0 : -1, detail);
    } else if (p.mode == 'V') {
      std::string detail;
      bool ok = true;
      auto expect = [&](const char *what, int got, int want) {
        char b[160];
        std::snprintf(b, sizeof b, "%s%s: rc=%d (expected %d)", detail.empty() ? "" : "\n    ",
                      what, got, want);
        detail += b;
        if (got != want) ok = false;
      };
      std::vector<float> a(static_cast<size_t>(8) * (cfg.k8 + 1), 1.0f);
      std::vector<float> b(static_cast<size_t>(cfg.k8 + 1) * 8, 1.0f);
      std::vector<float> c(64, 0.0f);
      // Host-side refusals: nothing reaches the link.
      {
        fpga_gemm_t *h = fpga_gemm_begin(8, 8, cfg.k8 + 1, 1);
        if (!h) {
          expect("begin", fpga_async_runtime_last_error(), 0);
        } else {
          fpga_async_stats_t before; fpga_async_runtime_get_stats(&before);
          expect("k = K_MAX + 1 on device 0", fpga_tile_submit(h, 0, 0, 1, 8, cfg.k8 + 1, 8, a.data(), b.data(), c.data(), 8, 0, 0, 109), -10);
          expect("k = K_MAX + 1 on device 1", fpga_tile_submit(h, 0, 0, 1, 4, cfg.k4 + 1, 4, a.data(), b.data(), c.data(), 8, 1, 0, 101), -10);
          expect("device 4", fpga_tile_submit(h, 0, 0, 1, 4, 1, 4, a.data(), b.data(), c.data(), 8, 4, 0, 101), -11);
          expect("9x9 tile on device 0", fpga_tile_submit(h, 0, 0, 1, 9, 1, 9, a.data(), b.data(), c.data(), 9, 0, 0, 109), -11);
          expect("est_cycles = 0", fpga_tile_submit(h, 0, 0, 1, 4, 1, 4, a.data(), b.data(), c.data(), 8, 1, 0, 0), -15);
          expect("k_index >= k_count", fpga_tile_submit(h, 0, 2, 2, 4, 1, 4, a.data(), b.data(), c.data(), 8, 1, 0, 101), -17);
          fpga_async_stats_t after; fpga_async_runtime_get_stats(&after);
          expect("bytes those refusals put on the link", static_cast<int>(after.bytes_to_fpga - before.bytes_to_fpga), 0);
          expect("a valid fold 0/2", fpga_tile_submit(h, 0, 0, 2, 4, 1, 4, a.data(), b.data(), c.data(), 8, 1, 0, 101), 0);
          expect("fold 0/2 again", fpga_tile_submit(h, 0, 0, 2, 4, 1, 4, a.data(), b.data(), c.data(), 8, 1, 0, 101), -17);
          expect("fold 1 with k_count 3", fpga_tile_submit(h, 0, 1, 3, 4, 1, 4, a.data(), b.data(), c.data(), 8, 1, 0, 101), -17);
          expect("fold 1/2", fpga_tile_submit(h, 0, 1, 2, 4, 1, 4, a.data(), b.data(), c.data(), 8, 1, 0, 101), 0);
          expect("wait", fpga_gemm_wait(h), 0);
          expect("submit after wait", fpga_tile_submit(h, 0, 0, 1, 4, 1, 4, a.data(), b.data(), c.data(), 8, 1, 0, 101), -17);
          // The two valid folds: C = 0 + 1x1 + 1x1 on a 4x4 block of the 8-wide C.
          int wrong = 0;
          for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
              if (c[static_cast<size_t>(i) * 8 + j] != 2.0f) ++wrong;
          for (int i = 0; i < 8; ++i)
            for (int j = 0; j < 8; ++j)
              if ((i >= 4 || j >= 4) && c[static_cast<size_t>(i) * 8 + j] != 0.0f) ++wrong;
          expect("the two folds accumulated (elements wrong)", wrong, 0);
        }
      }
      // The bitstream's refusal: with SYSTOLIC_ASYNC_VIOLATE=1 the runtime
      // sends k = K_MAX + 1; the record comes back with status BAD_K, the
      // wait reports -18, and a GEMM after it runs normally.
      fpga_async_runtime_close();
      setenv("SYSTOLIC_ASYNC_VIOLATE", "1", 1);
      set_stage(o, p.stage);
      {
        fpga_gemm_t *h = fpga_gemm_begin(4, 4, cfg.k4 + 1, 1);
        if (!h) {
          expect("begin (violate)", fpga_async_runtime_last_error(), 0);
        } else {
          std::fill(c.begin(), c.end(), 0.0f);
          expect("k = K_MAX + 1 sent to device 1", fpga_tile_submit(h, 0, 0, 1, 4, cfg.k4 + 1, 4, a.data(), b.data(), c.data(), 8, 1, 0, 101), 0);
          expect("its wait", fpga_gemm_wait(h), -18);
          int touched = 0;
          for (float v : c) if (v != 0.0f) ++touched;
          expect("C untouched by the refused tile (elements)", touched, 0);
        }
      }
      setenv("SYSTOLIC_ASYNC_VIOLATE", "0", 1);
      {
        Gemm g = make_gemm(8, 8, cfg.k4, true);
        const std::vector<TileSpec> tiles = decompose(8, 8, cfg.k4, {1, 2, 3}, cfg, false);
        int submit_rc = 0;
        const int rc = run_gemm(g, tiles, &submit_rc);
        expect("a GEMM after the refusal: submit", submit_rc, 0);
        expect("a GEMM after the refusal: wait", rc, 0);
        int first = -1;
        if (rc == 0 && submit_rc == 0) expect("its mismatches", mismatches_vs_ref(g, &first), 0);
        if (rc != 0 && class_of_rc(rc) == TRANSPORT) link_failed = true;
      }
      fpga_async_stats_t s; fpga_async_runtime_get_stats(&s);
      const double wall = std::chrono::duration<double, std::milli>(clock_t_::now() - t0).count();
      detail += "\n    " + stats_line(s, wall);
      report(p, ok ? PASS : (link_failed ? TRANSPORT : SUBMISSION), ok ? 0 : -1, detail);
    }
  }

  fpga_async_runtime_close();

  const int failures = count[MISMATCH] + count[TRANSPORT] + count[SUBMISSION] + count[CONFIG] + count[UNKNOWN];
  if (failures == 0 && count[NOT_RUN] == 0) {
    std::printf("\nRUNTIME ASYNC: PASS\n");
    return 0;
  }
  std::string summary;
  for (Class c : {CONFIG, TRANSPORT, SUBMISSION, UNKNOWN, MISMATCH, NOT_RUN}) {
    if (count[c] == 0) continue;
    char buf[320];
    std::snprintf(buf, sizeof buf, "%s%s %d (first: rc=%d, %s)", summary.empty() ? "" : "; ",
                  class_name(c), count[c], first_rc[c], first_what[c].c_str());
    summary += buf;
  }
  std::printf("\nRUNTIME ASYNC: FAIL -- %s\n", summary.c_str());
  if (count[CONFIG]) return 2;
  if (count[TRANSPORT] || count[NOT_RUN]) return 3;
  if (count[SUBMISSION] || count[UNKNOWN]) return 4;
  return 1;
}
