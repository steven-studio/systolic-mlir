// runtime_dispatch_test.cpp -- the runtime layer on its own:
//
//   host -> fpga_matmul_tiled_auto_scheduled -> DPTI -> FPGA -> result
//
// without the compiler in front of it. Each case is one invocation on one
// physical device of the 1x8x8 + 3x4x4 bitstream, checked element by element
// against an integer CPU reference. Run it against the board, or against the
// RTL co-simulation; test/e2e/run_e2e.sh runs it first on either backend.
//
//   runtime-dispatch-test                 run every case (opens the FPGA link)
//   runtime-dispatch-test --print-config  the configuration as the runtime
//                                         resolves it, KEY=VALUE; no link
//   runtime-dispatch-test --list          the cases a run would send; no link
//
// The configuration is the runtime's (fpga_runtime_get_config, the file in
// SYSTOLIC_FPGA_CONFIG): K_MAX_8X8 for device 0, K_MAX_4X4 for devices 1..3,
// two independent values, and the operand slab they imply. It must be given:
// without SYSTOLIC_FPGA_CONFIG the runtime assumes the archived bitstream
// (16 / 16), and nothing here can tell which bitstream the board holds -- the
// FPGA does not report its K_MAX -- so this test stops before the link
// instead.
//
// The cases follow each device's own capacity. Full tiles at K_MAX (8x8 on
// device 0, 4x4 on device 1); edge tiles, for the zero padding; C_in != 0,
// for the accumulation C += A x B that chained K invocations rely on; then,
// on devices 0, 1 and 3, k = 1, k = K_MAX - 1 (when that is a K at all:
// K_MAX >= 2) and k = K_MAX; and k = K_MAX + 1 on device 1 and device 0, which
// the runtime must refuse (-10) before anything reaches the link -- one
// invocation never runs deeper than the device holds; deeper reductions are
// the compiler's K-folding (run_e2e.sh's cases). The values are small
// integers, so every product and sum is exact in binary32 for any K up to
// FPGA_K_MAX_LIMIT (|C| <= 16376 * 64 + 80 < 2^24).
//
// Every case is reported as one of, with its raw return code:
//   PASS        rc = 0 and every element equals the integer reference
//   MISMATCH    rc = 0 and some element differs: a numerical failure
//   TRANSPORT   the DPTI link failed (rc -3, -4, -6, -7, -8)
//   SUBMISSION  the runtime refused or could not submit the invocation
//               (rc -5, -10, -11, -12, -13, -15)
//   CONFIG      the bitstream configuration is invalid (rc -16)
//   NOT RUN     rc -14: refused because an earlier call of this session
//               failed on the link -- a consequence of that failure, not a
//               failure of this case
// Exit status: 0 every case as expected; otherwise, in this order of
// precedence, 2 a configuration failure, 3 a transport failure (or a case not
// run after one), 4 a submission failure or an unexpected return code, 1 a
// numerical mismatch.

#include "fpga_matmul_tiled.h"

#include <algorithm>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace {

struct Case {
  std::string what;
  unsigned device;
  int m, k, n;
  bool nonzero_c_in;
};

// k = K_MAX + 1 on one device: must be refused with -10, nothing sent.
struct Refusal {
  std::string what;
  unsigned device;
  int p, k;
};

// Small integers, zeros and negatives included: exact in binary32, so the
// FPGA's accumulation order cannot change the result.
uint32_t g_lcg = 20261011u;
int next_value() {
  g_lcg = g_lcg * 1664525u + 1013904223u;
  return static_cast<int>((g_lcg >> 16) % 17) - 8;  // -8 .. 8
}

std::string shape(int m, int k, int n) {
  return std::to_string(m) + "x" + std::to_string(k) + "x" + std::to_string(n);
}

std::string where(unsigned dev) {
  return ", device " + std::to_string(dev) + (dev == 0 ? " (8x8)" : " (4x4)");
}

// The invocations a run sends, and the notes on boundaries that do not exist
// for this configuration (k = K_MAX - 1 = 0 when K_MAX = 1).
void make_cases(int k8, int k4, std::vector<Case> &cases,
                std::vector<std::string> &notes) {
  cases = {
      {shape(8, k8, 8) + " full tile, k = K_MAX" + where(0), 0, 8, k8, 8, false},
      {shape(4, k4, 4) + " full tile, k = K_MAX" + where(1), 1, 4, k4, 4, false},
      {shape(2, std::min(5, k4), 3) + " edge tile" + where(2), 2, 2,
       std::min(5, k4), 3, false},
      {shape(4, k4, 4) + " with C_in != 0" + where(3), 3, 4, k4, 4, true},
      {shape(5, std::min(7, k8), 6) + " edge tile with C_in != 0" + where(0),
       0, 5, std::min(7, k8), 6, true},
  };
  // k at the edges of each device's own capacity.
  for (unsigned dev : {0u, 1u, 3u}) {
    const int kmax = dev == 0 ? k8 : k4;
    const int p = dev == 0 ? 8 : 4;
    cases.push_back({shape(p, 1, p) + " k = 1" + where(dev), dev, p, 1, p, true});
    if (kmax >= 2)
      cases.push_back({shape(p - 1, kmax - 1, p) + " k = K_MAX - 1" + where(dev),
                       dev, p - 1, kmax - 1, p, false});
    else
      notes.push_back("k = K_MAX - 1 = 0" + where(dev) +
                      ": not a GEMM K (K_MAX = 1)");
    cases.push_back({shape(p, kmax, p - 1) + " k = K_MAX" + where(dev), dev, p,
                     kmax, p - 1, true});
  }
}

std::vector<Refusal> make_refusals(int k8, int k4) {
  std::vector<Refusal> r;
  for (unsigned dev : {1u, 0u}) {
    const int p = dev == 0 ? 8 : 4;
    const int k = (dev == 0 ? k8 : k4) + 1;
    r.push_back({shape(p, k, p) + " k = K_MAX + 1" + where(dev), dev, p, k});
  }
  return r;
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

// What a return code of fpga_matmul_tiled_auto_scheduled says
// (runtime/fpga_matmul_tiled.h); a code it does not list stays UNKNOWN.
Class class_of(int rc, int mismatches) {
  if (rc == 0)
    return mismatches == 0 ? PASS : MISMATCH;
  switch (rc) {
  case -16:
    return CONFIG;
  case -14:
    return NOT_RUN;
  case -3: case -4: case -6: case -7: case -8:
    return TRANSPORT;
  case -5: case -10: case -11: case -12: case -13: case -15:
    return SUBMISSION;
  default:
    return UNKNOWN;
  }
}

// The configuration has to be named; the runtime's built-in default is the
// archived bitstream's, which is not something a test of the board may
// assume. Prints why and returns false if it is not.
bool configuration_given(const char *prefix) {
  const char *path = std::getenv("SYSTOLIC_FPGA_CONFIG");
  if (path && *path)
    return true;
  std::printf("%sSYSTOLIC_FPGA_CONFIG is not set: name the fpga_config.txt of "
              "the bitstream on the FPGA (dma_top_build.tcl writes it next to "
              "the .bit; test/e2e/profiles/ has the archived one's). Without "
              "it the runtime would assume the archived bitstream's 16 / 16, "
              "and the FPGA does not report its K_MAX.\n",
              prefix);
  return false;
}

} // namespace

int main(int argc, char **argv) {
  // Line-buffered, so the report keeps its order with the runtime's own
  // messages on stderr when both go to one file.
  std::setvbuf(stdout, nullptr, _IOLBF, 0);

  const std::string mode = argc > 1 ? argv[1] : "";
  if (argc > 2 || (!mode.empty() && mode != "--print-config" && mode != "--list")) {
    std::fprintf(stderr,
                 "usage: runtime-dispatch-test [--print-config | --list]\n");
    return 2;
  }

  // ---- the configuration, as the runtime resolves it ---------------------
  if (!configuration_given(mode == "--print-config" ? "CONFIG_ERROR="
                                                     : "configuration failure: ")) {
    if (mode.empty())
      std::printf("RUNTIME DISPATCH: FAIL -- CONFIG\n");
    return 2;
  }
  fpga_runtime_config_t hw{};
  if (fpga_runtime_get_config(&hw) != 0) {
    if (mode == "--print-config")
      std::printf("CONFIG_ERROR=%s\n", hw.error);
    else
      std::printf("configuration failure: %s\n%s", hw.error,
                  mode.empty() ? "RUNTIME DISPATCH: FAIL -- CONFIG\n" : "");
    return 2;
  }
  const int k8 = static_cast<int>(hw.k_max_8x8);
  const int k4 = static_cast<int>(hw.k_max_4x4);

  if (mode == "--print-config") {
    std::printf("K_MAX_8X8=%u\nK_MAX_4X4=%u\nSLAB_BYTES=%u\nPIPELINE_DEPTH=%u\nSOURCE=%s\n",
                hw.k_max_8x8, hw.k_max_4x4, hw.slab_bytes, hw.pipeline_depth,
                hw.source);
    return 0;
  }

  std::vector<Case> cases;
  std::vector<std::string> notes;
  make_cases(k8, k4, cases, notes);
  const std::vector<Refusal> refusals = make_refusals(k8, k4);

  std::printf("configuration: K_MAX_8X8 = %d, K_MAX_4X4 = %d, %u-byte operand "
              "slab (%s)\n",
              k8, k4, hw.slab_bytes, hw.source);
  std::printf("  (the host's configuration: the FPGA does not report its "
              "K_MAX, so it is not independently verified)\n\n");

  if (mode == "--list") {
    for (const Case &c : cases)
      std::printf("run     %s\n", c.what.c_str());
    for (const std::string &n : notes)
      std::printf("skip    %s\n", n.c_str());
    for (const Refusal &r : refusals)
      std::printf("refuse  %s (expect rc=-10, nothing sent)\n", r.what.c_str());
    return 0;
  }

  int count[UNKNOWN + 1] = {};
  int first_class_rc[UNKNOWN + 1] = {};
  std::string first_class_case[UNKNOWN + 1];
  auto record = [&](Class c, int rc, const std::string &what) {
    if (count[c]++ == 0) {
      first_class_rc[c] = rc;
      first_class_case[c] = what;
    }
  };

  for (const Case &c : cases) {
    std::vector<float> A(static_cast<size_t>(c.m) * c.k);
    std::vector<float> B(static_cast<size_t>(c.k) * c.n);
    std::vector<float> C(static_cast<size_t>(c.m) * c.n);
    std::vector<int64_t> ref(C.size());

    for (float &v : A) v = static_cast<float>(next_value());
    for (float &v : B) v = static_cast<float>(next_value());
    for (size_t i = 0; i < C.size(); ++i) {
      C[i] = c.nonzero_c_in ? static_cast<float>(next_value() * 10) : 0.0f;
      ref[i] = static_cast<int64_t>(C[i]);
    }
    for (int i = 0; i < c.m; ++i)
      for (int j = 0; j < c.n; ++j)
        for (int k = 0; k < c.k; ++k)
          ref[i * c.n + j] += static_cast<int64_t>(A[i * c.k + k]) *
                              static_cast<int64_t>(B[k * c.n + j]);

    const int rc = fpga_matmul_tiled_auto_scheduled(
        c.m, c.k, c.n, A.data(), B.data(), C.data(), c.device,
        /*start_cycle=*/0, /*est_cycles=*/c.device == 0 ? 109 : 101);

    // Only a result that came back can be compared: C is not valid after a
    // failed call, and its elements are not counted as mismatches.
    int mismatches = 0;
    int first = -1;
    if (rc == 0) {
      for (size_t i = 0; i < C.size(); ++i) {
        if (C[i] != static_cast<float>(ref[i])) {
          if (first < 0) first = static_cast<int>(i);
          ++mismatches;
        }
      }
    }

    const Class cls = class_of(rc, mismatches);
    record(cls, rc, c.what);
    if (rc == 0)
      std::printf("%-62s rc=%d  mismatches=%d/%zu  %s\n", c.what.c_str(), rc,
                  mismatches, C.size(), class_name(cls));
    else
      std::printf("%-62s rc=%d  %s\n", c.what.c_str(), rc, class_name(cls));
    if (first >= 0)
      std::printf("    first mismatch C[%d][%d]: FPGA %.9g, CPU %" PRId64 "\n",
                  first / c.n, first % c.n, static_cast<double>(C[first]),
                  ref[first]);
  }
  for (const std::string &n : notes)
    std::printf("%-62s n/a, not run\n", n.c_str());

  // k = K_MAX + 1: refused with -10, nothing sent (the runtime checks k
  // before it looks at the link, so this holds after a link failure too).
  fpga_runtime_stats_t before;
  fpga_runtime_get_stats(&before);
  for (const Refusal &r : refusals) {
    std::vector<float> A(static_cast<size_t>(r.p) * r.k, 1.0f);
    std::vector<float> B(static_cast<size_t>(r.k) * r.p, 1.0f);
    std::vector<float> C(static_cast<size_t>(r.p) * r.p, 0.0f);
    const int rc = fpga_matmul_tiled_auto_scheduled(
        r.p, r.k, r.p, A.data(), B.data(), C.data(), r.device, 0, 101);
    fpga_runtime_stats_t after;
    fpga_runtime_get_stats(&after);
    const bool ok = rc == -10 && after.bytes_to_fpga == before.bytes_to_fpga &&
                    after.bytes_from_fpga == before.bytes_from_fpga;
    // Anything but the refusal is the runtime submitting (or failing to
    // refuse) an invocation it must not: a submission failure of this check.
    const Class cls = ok ? PASS : rc == -16 ? CONFIG : SUBMISSION;
    record(cls, rc, r.what);
    std::printf("%-62s rc=%d  %llu B sent  %s\n",
                (r.what + ": refused").c_str(), rc,
                after.bytes_to_fpga - before.bytes_to_fpga,
                ok ? "PASS" : class_name(cls));
  }

  // The refusals are counted as failed calls; the rest of the report is
  // about the invocations that ran.
  fpga_runtime_stats_t stats;
  fpga_runtime_get_stats(&stats);
  std::printf("\nruntime: %llu invocation(s) [dev0 %llu, dev1 %llu, dev2 %llu, "
              "dev3 %llu], %llu B to the FPGA, %llu B back\n",
              stats.invocations, stats.invocations_per_device[0],
              stats.invocations_per_device[1], stats.invocations_per_device[2],
              stats.invocations_per_device[3], stats.bytes_to_fpga,
              stats.bytes_from_fpga);
  fpga_runtime_close();

  const int failures = count[MISMATCH] + count[TRANSPORT] + count[SUBMISSION] +
                       count[CONFIG] + count[UNKNOWN];
  if (failures == 0 && count[NOT_RUN] == 0) {
    std::printf("RUNTIME DISPATCH: PASS\n");
    return 0;
  }
  std::string summary;
  for (Class c : {CONFIG, TRANSPORT, SUBMISSION, UNKNOWN, MISMATCH, NOT_RUN}) {
    if (count[c] == 0)
      continue;
    char buf[640];
    if (c == NOT_RUN)
      std::snprintf(buf, sizeof buf,
                    "%s%d case(s) NOT RUN (rc=-14: the link had already failed)",
                    summary.empty() ? "" : "; ", count[c]);
    else
      std::snprintf(buf, sizeof buf, "%s%s %d (first: rc=%d, %s)",
                    summary.empty() ? "" : "; ", class_name(c), count[c],
                    first_class_rc[c], first_class_case[c].c_str());
    summary += buf;
  }
  std::printf("RUNTIME DISPATCH: FAIL -- %s\n", summary.c_str());
  if (count[CONFIG]) return 2;
  if (count[TRANSPORT] || count[NOT_RUN]) return 3;
  if (count[SUBMISSION] || count[UNKNOWN]) return 4;
  return 1;
}
