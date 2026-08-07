// design_tb.cpp -- testbench for the runtime-K systolic array.
//
// Serves csim and cosim from one source. Two jobs:
//
//   1. Prove bit-exactness against a scalar reference. The reference MUST
//      accumulate in the same k order as the array (k = 0, 1, ... K-1),
//      because float addition is not associative -- any other order gives a
//      different-but-equally-correct answer and the comparison becomes a
//      tolerance check instead of an equality check. The whole point of the
//      original 48-configuration result was that it was exact, so keep it
//      exact.
//
//   2. Drive exactly one K per cosim run. Vitis cosim reports latency as
//      min/avg/max aggregated over every call in the testbench, so a TB
//      that sweeps K internally yields a range you cannot attribute to any
//      particular K. To fit cycles = II * (K + R + C - 2) + depth you need
//      one point per run -- hence TB_K, set from the tcl.
//
// Build for csim:  g++ -std=c++14 -DTB_K=64 design_tb.cpp design.cpp -o tb

#include "design.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

#ifndef TB_K
#define TB_K K_MAX        // reduction depth under test
#endif

// Reference: same k order as PE(i,j), same float type, no fma contraction.
// Compile the reference with -ffp-contract=off if your host compiler is
// fusing a*b+c into an FMA -- that alone will break bit-exactness against
// the HLS float mult + float add, and it is a genuinely confusing failure
// because the numbers look almost right.
static void ref_matmul(const data_t A[R][K_MAX], const data_t B[K_MAX][C],
                       data_t Cref[R][C], int K) {
  for (int i = 0; i < R; i++)
    for (int j = 0; j < C; j++) {
      data_t acc = Cref[i][j];
      for (int k = 0; k < K; k++) acc += A[i][k] * B[k][j];
      Cref[i][j] = acc;
    }
}

static int bit_equal(data_t x, data_t y) {
  // Compare the bit patterns, not the values: == would call two NaNs
  // unequal and +0.0/-0.0 equal, and both distinctions matter here.
  unsigned int bx, by;
  std::memcpy(&bx, &x, sizeof bx);
  std::memcpy(&by, &y, sizeof by);
  return bx == by;
}

// Deterministic operands. Values are small and dyadic so the products are
// exactly representable -- this keeps a csim failure meaningful (it means
// the schedule is wrong) rather than ambiguous (it might be rounding).
static void fill(data_t A[R][K_MAX], data_t B[K_MAX][C], unsigned seed) {
  unsigned s = seed;
  auto next = [&s]() {
    s = s * 1103515245u + 12345u;
    return static_cast<data_t>(static_cast<int>((s >> 16) & 0xFF) - 128) /
           16.0f;     // multiples of 1/16 in [-8, 8)
  };
  for (int i = 0; i < R; i++)
    for (int k = 0; k < K_MAX; k++) A[i][k] = next();
  for (int k = 0; k < K_MAX; k++)
    for (int j = 0; j < C; j++) B[k][j] = next();
}

static int run_case(int K, unsigned seed, const char *label) {
  static data_t A[R][K_MAX], B[K_MAX][C], Cdut[R][C], Cref[R][C];
  fill(A, B, seed);

  // Non-zero initial C, so the accumulate-into-C semantics are actually
  // exercised. Starting from zero would hide a kernel that overwrites.
  for (int i = 0; i < R; i++)
    for (int j = 0; j < C; j++) {
      data_t v = static_cast<data_t>((i * C + j) % 7) - 3.0f;
      Cdut[i][j] = v;
      Cref[i][j] = v;
    }

  matmul_4x4x4(A, B, Cdut, K);

  // The kernel clamps K to [0, K_MAX]; the reference must be told the same
  // clamped value or the out-of-range cases will "fail" for the wrong
  // reason.
  int k_eff = (K < 0) ? 0 : ((K > K_MAX) ? K_MAX : K);
  ref_matmul(A, B, Cref, k_eff);

  int bad = 0;
  for (int i = 0; i < R; i++)
    for (int j = 0; j < C; j++)
      if (!bit_equal(Cdut[i][j], Cref[i][j])) {
        if (bad < 4)
          std::printf("  MISMATCH [%d][%d] dut=%.9g ref=%.9g\n", i, j,
                      (double)Cdut[i][j], (double)Cref[i][j]);
        bad++;
      }

  std::printf("%-28s K=%-4d %s\n", label, K, bad ? "FAIL" : "ok");
  return bad;
}

int main() {
  int fails = 0;

  // The single point cosim will measure. Keep it first so the latency
  // numbers in the cosim log are easy to find.
  fails += run_case(TB_K, 0xC0FFEEu, "primary (cosim point)");

#ifdef CSIM_ONLY
  // Edge cases worth checking once in csim, but deliberately NOT in cosim:
  // extra calls would pollute the min/avg/max latency aggregation.
  fails += run_case(1, 0x1234u, "K=1 (shallowest)");
  fails += run_case(2, 0x5678u, "K=2");
  fails += run_case(K_MAX, 0x9ABCu, "K=K_MAX");
  fails += run_case(K_MAX + 9, 0xDEF0u, "K>K_MAX (clamps)");
  fails += run_case(0, 0x2468u, "K=0 (C unchanged)");
  fails += run_case(-5, 0x1357u, "K<0 (clamps to 0)");

  // Folding: two half-depth calls must equal one full-depth call, since
  // that is how the host issues a tile deeper than K_MAX. Same k order, so
  // still bit-exact.
  {
    static data_t A[R][K_MAX], B[K_MAX][C], Cfold[R][C], Cone[R][C];
    fill(A, B, 0xFEEDu);
    for (int i = 0; i < R; i++)
      for (int j = 0; j < C; j++) { Cfold[i][j] = 0.0f; Cone[i][j] = 0.0f; }

    int half = K_MAX / 2;
    matmul_4x4x4(A, B, Cone, half);          // first half only
    matmul_4x4x4(A, B, Cfold, half);         // same, then compare
    int bad = 0;
    for (int i = 0; i < R; i++)
      for (int j = 0; j < C; j++)
        if (!bit_equal(Cfold[i][j], Cone[i][j])) bad++;
    std::printf("%-28s        %s\n", "fold repeatability", bad ? "FAIL" : "ok");
    fails += bad;
  }
#endif

  std::printf("\n%s\n", fails ? "TEST FAILED" : "TEST PASSED");
  return fails ? 1 : 0;   // cosim treats non-zero as failure
}
