// runtime_dispatch_test.cpp -- the runtime layer on its own:
//
//   host -> fpga_matmul_tiled_auto_scheduled -> DPTI -> FPGA -> result
//
// without the compiler in front of it. Each case is one invocation on one
// physical device of the 1x8x8 + 3x4x4 bitstream, checked element by element
// against an integer CPU reference. Run it against the board, or against the
// RTL co-simulation (test/e2e/run_e2e.sh --backend cosim --runtime-only).
//
// The full tiles (8x8x16 on device 0, 4x4x16 on device 1) are the shapes the
// 2026-10-09 board run sent; the edge tiles exercise the zero padding, and the
// C_in cases the accumulation C += A x B that chained K invocations rely on.

#include "fpga_matmul_tiled.h"

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

struct Case {
  const char *what;
  unsigned device;
  int m, k, n;
  bool nonzero_c_in;
};

const Case kCases[] = {
    {"8x8x16 full tile, device 0 (8x8)", 0, 8, 16, 8, false},
    {"4x4x16 full tile, device 1 (4x4 #0)", 1, 4, 16, 4, false},
    {"2x5x3 edge tile, device 2 (4x4 #1)", 2, 2, 5, 3, false},
    {"4x16x4 with C_in != 0, device 3 (4x4 #2)", 3, 4, 16, 4, true},
    {"5x7x6 edge tile with C_in != 0, device 0 (8x8)", 0, 5, 7, 6, true},
};

// Small integers, zeros and negatives included: exact in binary32, so the
// FPGA's accumulation order cannot change the result.
uint32_t g_lcg = 20261011u;
int next_value() {
  g_lcg = g_lcg * 1664525u + 1013904223u;
  return static_cast<int>((g_lcg >> 16) % 17) - 8;  // -8 .. 8
}

} // namespace

int main() {
  int failed = 0;

  for (const Case &c : kCases) {
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
    std::printf("%-48s rc=%d  mismatches=%d/%zu  %s\n", c.what, rc, mismatches,
                C.size(), ok ? "PASS" : "FAIL");
    if (first >= 0)
      std::printf("    first mismatch C[%d][%d]: FPGA %.9g, CPU %" PRId64 "\n",
                  first / c.n, first % c.n, static_cast<double>(C[first]),
                  ref[first]);
    if (!ok) ++failed;
  }

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
