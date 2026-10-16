/*
 * Case ID: ext_016
 * Name: batch_dependent_a_row_tiles
 * Set / category: extended / batched
 * Mathematical class: not_gemm
 * C program valid: yes (every subscript in bounds, no undefined behaviour)
 *
 * Mathematical operation:
 *   for i in [1, 17), j in [0, 4), k in [0, 16) (loops outer to inner):
 *     C[8 * i][j] += A[4 * i - j][5 * k] * B[5 * k][j]
 *
 *   Not a dense GEMM of these operands because:
 *     - A has a subscript that couples loops of different coordinates (mixed:I+P, K)
 *     - B is indexed by ['K', 'P']; the operand in that position of a GEMM is indexed by exactly ['J', 'K']
 *     - C is indexed by ['I', 'P']; the operand in that position of a GEMM is indexed by exactly ['I', 'J']
 *
 *   ext_015 with i in [1, 17) and k in [0, 16): a 16x1x16 product per j,
 *   several tiles per batch value, each a slice of the moving view.
 *   Offloaded since ISSUE-007.
 *
 * Array dimensions: A: float[65][76]   B: float[76][4]   C: float[129][4]
 * Loop order (outer to inner): i [row I], j [batch P], k [reduction K]
 * Index formulas: A: A[4 * i - j][5 * k]   B: B[5 * k][j]   C: C[8 * i][j]
 * Statement form: C[..] += A[..] * B[..]
 * Element type: float
 *
 * Mathematical GEMM: NO
 * Expected recognition (specification): ACCEPT
 *   basis: (supported since 2026-10-16, ISSUE-007) a batched GEMM whose factors read the batch coordinate(s) through their own expressions -- in dimensions of their own or as affine offsets of role dimensions (A[4i - j][5k]) -- while C reads every batch coordinate alone: per batch value an ordinary GEMM of strided views whose offsets are affine in the batch counters (docs/AFFINE_GEMM_MAPPING.md section 16)
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
 *     cc -std=c11 -O1 ext_016.c -o ext_016 && ./ext_016
 *   Through the compiler (software runtime model, NOT the FPGA):
 *     build/bin/systolic-cc --runtime=soft --explain --verify -DGEMM_RECOGNITION_STATS \
 *       ext_016.c -o ext_016 && ./ext_016
 *   On the board: the same with --runtime=board and SYSTOLIC_FPGA_CONFIG set.
 *   Stage by stage, results collected: scripts/run_showcase.py --case ext_016
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

void ext_016_kernel(float A[restrict 65][76], float B[restrict 76][4], float C[restrict 129][4]) {
  for (int i = 1; i < 17; ++i)
    for (int j = 0; j < 4; ++j)
      for (int k = 0; k < 16; ++k)
        C[8 * i][j] += A[4 * i - j][5 * k] * B[5 * k][j];
}

/* -------------------------------------------------------------------- data */

static float A[65][76];
static float B[76][4];
static float C[129][4];
static float C_ref[129][4];   /* reference 1 */

/* small integers in [-3, 3]; the same formula as generator/casespec.py */
static float elem(long e, long seed) { return (float)((e * 37 + seed * 101) % 103 % 7 - 3); }
/* the product through a call: the references are not GEMM statements */
static float mul(float x, float y) { return x * y; }

static void init(void) {
  for (long e = 0; e < 4940; ++e) (&A[0][0])[e] = elem(e, 11);
  for (long e = 0; e < 304; ++e) (&B[0][0])[e] = elem(e, 21);
  for (long e = 0; e < 516; ++e) (&C[0][0])[e] = elem(e, 31);
}

/* checksum: sum of C[e] * ((e % 97) + 1) over the flat array (exact in double) */
static double checksum(const float *p, long n) {
  double s = 0;
  for (long e = 0; e < n; ++e) s += (double)p[e] * (double)((e % 97) + 1);
  return s;
}
#define EXPECTED_CHECKSUM 2490.0   /* final C, from the Python oracle */
#define INITIAL_CHECKSUM  -1355.0   /* C before the kernel runs */

/* ------------------------------------------------- reference 1: direct
 * The statement over the same loop domain, on flat arrays with explicit
 * row-major offsets.  It shares no index code with the kernel. */

static void reference_direct(const float *Af, const float *Bf, float *Cf) {
  for (long i = 1; i < 17; i += 1)
    for (long j = 0; j < 4; j += 1)
      for (long k = 0; k < 16; k += 1)
        Cf[(8 * i) * 4 + (j)] = Cf[(8 * i) * 4 + (j)] + mul(Af[(4 * i - j) * 76 + (5 * k)], Bf[(5 * k) * 4 + (j)]);
}

/* -------------------------------------------------------------------- main */

static long first_diff(const float *x, const float *y, long n) {
  for (long e = 0; e < n; ++e)
    if (x[e] != y[e]) return e;
  return -1;
}

int main(void) {
  const long n = 516;
  init();
  memcpy(C_ref, C, sizeof C_ref);
  printf("[gemm_recognition] ext_016 batch_dependent_a_row_tiles: extended / batched, class not_gemm\n");
  if (checksum(&C[0][0], n) != INITIAL_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: initial data differs from the Python oracle\n");
    printf("RESULT ext_016 FAIL (test bug)\n"); return 3;
  }
  reference_direct(&A[0][0], &B[0][0], &C_ref[0][0]);
  if (checksum(&C_ref[0][0], n) != EXPECTED_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: C reference checksum %.0f, Python oracle %.0f\n",
           checksum(&C_ref[0][0], n), EXPECTED_CHECKSUM);
    printf("RESULT ext_016 FAIL (test bug)\n"); return 3;
  }
  printf("[gemm_recognition] references: checksum %.0f matches the Python oracle\n", EXPECTED_CHECKSUM);

#ifdef GEMM_RECOGNITION_STATS
  systolic_offload_stats_t before, after;
  systolic_offload_stats(&before);
#endif
  ext_016_kernel(A, B, C);   /* the kernel under test */
#ifdef GEMM_RECOGNITION_STATS
  systolic_offload_stats(&after);
  printf("[gemm_recognition] invocations: %llu (backend %s, hardware=%d, failed=%llu)\n",
         (after.per_device[0] + after.per_device[1] + after.per_device[2] + after.per_device[3]) -
         (before.per_device[0] + before.per_device[1] + before.per_device[2] + before.per_device[3]),
         after.backend, after.is_hardware, after.failed - before.failed);
#endif

  long d = first_diff(&C[0][0], &C_ref[0][0], n);
  double got = checksum(&C[0][0], n);
  if (d < 0 && got == EXPECTED_CHECKSUM) {
    printf("[gemm_recognition] kernel: equal to the reference (%ld elements, exact), checksum %.0f\n", n, got);
    printf("RESULT ext_016 PASS\n");
    return 0;
  }
  if (d >= 0)
    printf("[gemm_recognition] kernel: first mismatch at flat element %ld: got %g, reference %g\n",
           d, (double)(&C[0][0])[d], (double)(&C_ref[0][0])[d]);
  printf("[gemm_recognition] kernel: checksum %.0f, expected %.0f\n", got, EXPECTED_CHECKSUM);
  printf("RESULT ext_016 FAIL\n");
  return 1;
}
