/*
 * Case ID: ext_021
 * Name: a_and_b_transposed_batch_views
 * Set / category: extended / batched
 * Mathematical class: not_gemm
 * C program valid: yes (every subscript in bounds, no undefined behaviour)
 *
 * Mathematical operation:
 *   for b in [0, 2), i in [0, 4), j in [0, 4), k in [0, 4) (loops outer to inner):
 *     C[b][i][j] += A[k][i + b] * B[j][k + b]
 *
 *   Not a dense GEMM of these operands because:
 *     - A has a subscript that couples loops of different coordinates (K, mixed:I+P)
 *     - B has a subscript that couples loops of different coordinates (J, mixed:K+P)
 *
 *   C[b][i][j] += A[k][i + b] B[j][k + b]: both factors transposed, both
 *   views moving with the batch loop b (ISSUE-007): per b,
 *   A'(I, K) = A(K, I + b) and B'(K, J) = B(J, K + b).  Offloaded.
 *
 * Array dimensions: A: float[4][5]   B: float[4][5]   C: float[2][4][4]
 * Loop order (outer to inner): b [batch P], i [row I], j [column J], k [reduction K]
 * Index formulas: A: A[k][i + b]   B: B[j][k + b]   C: C[b][i][j]
 * Statement form: C[..] += A[..] * B[..]
 * Element type: float
 *
 * Mathematical GEMM: YES
 * Expected recognition (specification): ACCEPT
 *   basis: (supported since 2026-10-16, ISSUE-007) a batched GEMM whose operands read the batch coordinate(s) through their own expressions -- in dimensions of their own or as affine offsets of role dimensions (A[4i - j][5k], C[i + k][j] since section 19.3): per batch value an ordinary GEMM of strided views whose offsets are affine in the batch counters, C injective per batch (docs/AFFINE_GEMM_MAPPING.md sections 16, 19)
 * Current implementation (commit 6e25528 with the fix of ISSUE-007, predicted from its sources; measured by
 * scripts/run_showcase.py, never assumed):
 *   front end (systolic-cfront):             translated
 *   recognition (recognizeLogicalMatmul):    ACCEPT
 *   dense partition (getDenseLogicalGemm):   supported
 *   lowering (--affine-matmul-to-systolic):  supported
 *   driver (systolic-cc):                    offloaded
 * Classification: TRUE POSITIVE -- was a FALSE NEGATIVE of failures/issues/ISSUE-007 (fixed)
 *
 * Independent CPU reference: yes --
 *   (1) direct evaluation of the statement over the loop domain on flat arrays
 *       with explicit row-major offsets, the product through a function call;
 *   (2) a checksum of the expected result computed independently in Python
 *       (generator/casespec.py) at generation time: EXPECTED_CHECKSUM below.
 * The references never call the kernel or any compiler-generated code and share
 * no index code with it.  Data: small integers in [-3, 3], C starts non-zero;
 * every partial sum is an integer far below 2^24, so the float results are
 * exact and compared for exact equality -- no tolerance.
 *
 * How to run:
 *   CPU only (the kernel as written vs the references):
 *     cc -std=c11 -O1 ext_021.c -o ext_021 && ./ext_021
 *   Through the compiler (software runtime model, NOT the FPGA):
 *     build/bin/systolic-cc --runtime=soft --explain --verify -DGEMM_RECOGNITION_STATS \
 *       ext_021.c -o ext_021 && ./ext_021
 *   On the board: the same with --runtime=board and SYSTOLIC_FPGA_CONFIG set.
 *   Stage by stage, results collected: scripts/run_showcase.py --case ext_021
 * Exit status: 0 = PASS (the kernel equals every reference), 1 = FAIL, 3 = the
 * references disagree among themselves (a bug in this test, not in the compiler).
 */

#include <stdio.h>
#include <string.h>

#ifdef GEMM_RECOGNITION_STATS
#include "systolic_offload.h"   /* invocation counts (available when systolic-cc links the runtime) */
#endif

/* ------------------------------------------------------------------ kernel
 * The loop nest under test.  It is the only function the C front end is
 * meant to translate; everything below it is host code (int return values,
 * pointers, calls) and stays on the CPU. */

void ext_021_kernel(float A[restrict 4][5], float B[restrict 4][5], float C[restrict 2][4][4]) {
  for (int b = 0; b < 2; ++b)
    for (int i = 0; i < 4; ++i)
      for (int j = 0; j < 4; ++j)
        for (int k = 0; k < 4; ++k)
          C[b][i][j] += A[k][i + b] * B[j][k + b];
}

/* -------------------------------------------------------------------- data */

static float A[4][5];
static float B[4][5];
static float C[2][4][4];
static float C_ref[2][4][4];   /* reference 1 */

/* small integers in [-3, 3]; the same formula as generator/casespec.py */
static float elem(long e, long seed) { return (float)((e * 37 + seed * 101) % 103 % 7 - 3); }
/* the product through a call: the references are not GEMM statements */
static float mul(float x, float y) { return x * y; }

static void init(void) {
  for (long e = 0; e < 20; ++e) (&A[0][0])[e] = elem(e, 11);
  for (long e = 0; e < 20; ++e) (&B[0][0])[e] = elem(e, 21);
  for (long e = 0; e < 32; ++e) (&C[0][0][0])[e] = elem(e, 31);
}

/* checksum: sum of C[e] * ((e % 97) + 1) over the flat array (exact in double) */
static double checksum(const float *p, long n) {
  double s = 0;
  for (long e = 0; e < n; ++e) s += (double)p[e] * (double)((e % 97) + 1);
  return s;
}
#define EXPECTED_CHECKSUM 559.0   /* final C, from the Python oracle */
#define INITIAL_CHECKSUM  197.0   /* C before the kernel runs */

/* ------------------------------------------------- reference 1: direct
 * The statement over the same loop domain, on flat arrays with explicit
 * row-major offsets.  It shares no index code with the kernel. */

static void reference_direct(const float *Af, const float *Bf, float *Cf) {
  for (long b = 0; b < 2; b += 1)
    for (long i = 0; i < 4; i += 1)
      for (long j = 0; j < 4; j += 1)
        for (long k = 0; k < 4; k += 1)
          Cf[(b) * 16 + (i) * 4 + (j)] = Cf[(b) * 16 + (i) * 4 + (j)] + mul(Af[(k) * 5 + (i + b)], Bf[(j) * 5 + (k + b)]);
}

/* -------------------------------------------------------------------- main */

static long first_diff(const float *x, const float *y, long n) {
  for (long e = 0; e < n; ++e)
    if (x[e] != y[e]) return e;
  return -1;
}

int main(void) {
  const long n = 32;
  init();
  memcpy(C_ref, C, sizeof C_ref);
  printf("[gemm_recognition] ext_021 a_and_b_transposed_batch_views: extended / batched, class not_gemm\n");
  if (checksum(&C[0][0][0], n) != INITIAL_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: initial data differs from the Python oracle\n");
    printf("RESULT ext_021 FAIL (test bug)\n"); return 3;
  }
  reference_direct(&A[0][0], &B[0][0], &C_ref[0][0][0]);
  if (checksum(&C_ref[0][0][0], n) != EXPECTED_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: C reference checksum %.0f, Python oracle %.0f\n",
           checksum(&C_ref[0][0][0], n), EXPECTED_CHECKSUM);
    printf("RESULT ext_021 FAIL (test bug)\n"); return 3;
  }
  printf("[gemm_recognition] references: checksum %.0f matches the Python oracle\n", EXPECTED_CHECKSUM);

#ifdef GEMM_RECOGNITION_STATS
  systolic_offload_stats_t before, after;
  systolic_offload_stats(&before);
#endif
  ext_021_kernel(A, B, C);   /* the kernel under test */
#ifdef GEMM_RECOGNITION_STATS
  systolic_offload_stats(&after);
  printf("[gemm_recognition] invocations: %llu (backend %s, hardware=%d, failed=%llu)\n",
         (after.per_device[0] + after.per_device[1] + after.per_device[2] + after.per_device[3]) -
         (before.per_device[0] + before.per_device[1] + before.per_device[2] + before.per_device[3]),
         after.backend, after.is_hardware, after.failed - before.failed);
#endif

  long d = first_diff(&C[0][0][0], &C_ref[0][0][0], n);
  double got = checksum(&C[0][0][0], n);
  if (d < 0 && got == EXPECTED_CHECKSUM) {
    printf("[gemm_recognition] kernel: equal to the reference (%ld elements, exact), checksum %.0f\n", n, got);
    printf("RESULT ext_021 PASS\n");
    return 0;
  }
  if (d >= 0)
    printf("[gemm_recognition] kernel: first mismatch at flat element %ld: got %g, reference %g\n",
           d, (double)(&C[0][0][0])[d], (double)(&C_ref[0][0][0])[d]);
  printf("[gemm_recognition] kernel: checksum %.0f, expected %.0f\n", got, EXPECTED_CHECKSUM);
  printf("RESULT ext_021 FAIL\n");
  return 1;
}
