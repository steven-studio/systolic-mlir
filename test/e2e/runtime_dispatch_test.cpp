// runtime_dispatch_test.cpp -- the runtime layer on its own:
//
//   host -> fpga_matmul_tiled_auto_scheduled -> DPTI -> FPGA -> result
//
// without the compiler in front of it. Each case is one invocation on one
// physical device of the 1x8x8 + 3x4x4 bitstream, checked element by element
// against an integer CPU reference. Run it against the board, or against the
// RTL co-simulation; test/e2e/run_e2e.sh runs it first on either backend.
//
// The cases follow the bitstream's reduction capacity, K_MAX_8X8 on device 0
// and K_MAX_4X4 on devices 1..3 (fpga_runtime_get_config: the file in
// SYSTOLIC_FPGA_CONFIG, 16 and 16 without it). At 16 / 16 the first five are
// the cases this test always ran: the full tiles (8x8x16 on device 0, 4x4x16
// on device 1) are the shapes the 2026-10-09 board run sent; the edge tiles
// exercise the zero padding, and the C_in cases the accumulation C += A x B
// that chained K invocations rely on. The rest put k at the edges of each
// device's own capacity -- 1, K_MAX - 1, K_MAX -- and check that k = K_MAX + 1
// is refused before anything reaches the link.

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

std::vector<Case> make_cases(int k8, int k4) {
  std::vector<Case> cases = {
      {shape(8, k8, 8) + " full tile, device 0 (8x8)", 0, 8, k8, 8, false},
      {shape(4, k4, 4) + " full tile, device 1 (4x4 #0)", 1, 4, k4, 4, false},
      {shape(2, std::min(5, k4), 3) + " edge tile, device 2 (4x4 #1)", 2, 2,
       std::min(5, k4), 3, false},
      {shape(4, k4, 4) + " with C_in != 0, device 3 (4x4 #2)", 3, 4, k4, 4,
       true},
      {shape(5, std::min(7, k8), 6) + " edge tile with C_in != 0, device 0 (8x8)",
       0, 5, std::min(7, k8), 6, true},
  };
  // k at the edges of each device's own capacity.
  for (unsigned dev : {0u, 1u, 3u}) {
    const int kmax = dev == 0 ? k8 : k4;
    const int p = dev == 0 ? 8 : 4;
    const std::string where = ", device " + std::to_string(dev) +
                              (dev == 0 ? " (8x8)" : " (4x4)");
    cases.push_back({shape(p, 1, p) + " k = 1" + where, dev, p, 1, p, true});
    if (kmax > 2)
      cases.push_back({shape(p - 1, kmax - 1, p) + " k = K_MAX - 1" + where,
                       dev, p - 1, kmax - 1, p, false});
    cases.push_back({shape(p, kmax, p - 1) + " k = K_MAX" + where, dev, p,
                     kmax, p - 1, true});
  }
  return cases;
}

} // namespace

int main() {
  fpga_runtime_config_t hw{};
  if (fpga_runtime_get_config(&hw) != 0) {
    std::printf("%s\nRUNTIME DISPATCH: FAIL\n", hw.error);
    return 1;
  }
  const int k8 = static_cast<int>(hw.k_max_8x8);
  const int k4 = static_cast<int>(hw.k_max_4x4);
  std::printf("bitstream: K_MAX_8X8 = %d, K_MAX_4X4 = %d, %u-byte operand slab "
              "(%s)\n\n",
              k8, k4, hw.slab_bytes, hw.source);

  int failed = 0;

  for (const Case &c : make_cases(k8, k4)) {
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

    const bool ok = rc == 0 && mismatches == 0;
    std::printf("%-56s rc=%d  mismatches=%d/%zu  %s\n", c.what.c_str(), rc,
                mismatches, C.size(), ok ? "PASS" : "FAIL");
    if (first >= 0)
      std::printf("    first mismatch C[%d][%d]: FPGA %.9g, CPU %" PRId64 "\n",
                  first / c.n, first % c.n, static_cast<double>(C[first]),
                  ref[first]);
    if (!ok) ++failed;
  }

  // k = K_MAX + 1 on a 4x4 and on the 8x8: refused with -10, nothing sent.
  fpga_runtime_stats_t before;
  fpga_runtime_get_stats(&before);
  for (unsigned dev : {1u, 0u}) {
    const int k = (dev == 0 ? k8 : k4) + 1;
    const int p = dev == 0 ? 8 : 4;
    std::vector<float> A(static_cast<size_t>(p) * k, 1.0f);
    std::vector<float> B(static_cast<size_t>(k) * p, 1.0f);
    std::vector<float> C(static_cast<size_t>(p) * p, 0.0f);
    const int rc = fpga_matmul_tiled_auto_scheduled(
        p, k, p, A.data(), B.data(), C.data(), dev, 0, 101);
    fpga_runtime_stats_t after;
    fpga_runtime_get_stats(&after);
    const bool ok = rc == -10 && after.bytes_to_fpga == before.bytes_to_fpga &&
                    after.bytes_from_fpga == before.bytes_from_fpga;
    std::printf("%-56s rc=%d  %llu B sent  %s\n",
                (shape(p, k, p) + " k = K_MAX + 1, device " +
                 std::to_string(dev) + ": refused")
                    .c_str(),
                rc, after.bytes_to_fpga - before.bytes_to_fpga,
                ok ? "PASS" : "FAIL");
    if (!ok) ++failed;
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

  std::printf("%s\n", failed ? "RUNTIME DISPATCH: FAIL" : "RUNTIME DISPATCH: PASS");
  return failed ? 1 : 0;
}
