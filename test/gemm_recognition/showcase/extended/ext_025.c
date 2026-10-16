/*
 * Case ID: ext_025
 * Name: batch_loop_runs_once
 * Set / category: extended / batched
 * Mathematical class: not_gemm
 * C program valid: yes (every subscript in bounds, no undefined behaviour)
 *
 * Mathematical operation:
 *   for t in [1, 2), i in [0, 5) (loops outer to inner):
 *     C[t][i][0] += A[t][i][1] * B[t][1][0]
 *
 *   Not a dense GEMM of these operands because:
 *     - A is indexed by ['P', 'I', 'const']; the operand in that position of a GEMM is indexed by exactly ['I', 'K']
 *     - B is indexed by ['P', 'const', 'const']; the operand in that position of a GEMM is indexed by exactly ['J', 'K']
 *     - C is indexed by ['P', 'I', 'const']; the operand in that position of a GEMM is indexed by exactly ['I', 'J']
 *
 *   The positional form's prefix t runs once: a constant on the iteration
 *   domain, a fixed index, no batch coordinate -- the plain 5 x 1 x 1 GEMM
 *   C[1][:, 0] += A[1][:, 1] B[1][1][0] (SQ-4), not a batched vector times a
 *   scalar (SQ-5).  Refused until the review fix, offloaded since.
 *
 * Array dimensions: A: float[2][5][3]   B: float[2][3][2]   C: float[2][5][2]
 * Loop order (outer to inner): t [batch P], i [row I]
 * Index formulas: A: A[t][i][1]   B: B[t][1][0]   C: C[t][i][0]
 * Statement form: C[..] += A[..] * B[..]
 * Element type: float
 *
 * Mathematical GEMM: NO
 * Expected recognition (specification): ACCEPT
 *   basis: (P0.4, P0.5) not a GEMM as written, but its loops that run once are constants on the iteration domain, and there the nest is a dense GEMM of strided views (decided by enumerating the points: casespec.gemm_on_domain; docs/AFFINE_GEMM_MAPPING.md Lemma 0, section 3)
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
 *     cc -std=c11 -O1 ext_025.c -o ext_025 && ./ext_025
 *   Through the compiler (software runtime model, NOT the FPGA):
 *     build/bin/systolic-cc --runtime=soft --explain --verify -DGEMM_RECOGNITION_STATS \
 *       ext_025.c -o ext_025 && ./ext_025
 *   On the board: the same with --runtime=board and SYSTOLIC_FPGA_CONFIG set.
 *   Stage by stage, results collected: scripts/run_showcase.py --case ext_025
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

void ext_025_kernel(float A[restrict 2][5][3], float B[restrict 2][3][2], float C[restrict 2][5][2]) {
  for (int t = 1; t < 2; ++t)
    for (int i = 0; i < 5; ++i)
      C[t][i][0] += A[t][i][1] * B[t][1][0];
}

/* -------------------------------------------------------------------- data */

static float A[2][5][3];
static float B[2][3][2];
static float C[2][5][2];
static float C_ref[2][5][2];   /* reference 1 */

/* small integers in [-3, 3]; the same formula as generator/casespec.py */
static float elem(long e, long seed) { return (float)((e * 37 + seed * 101) % 103 % 7 - 3); }
/* the product through a call: the references are not GEMM statements */
static float mul(float x, float y) { return x * y; }

static void init(void) {
  for (long e = 0; e < 30; ++e) (&A[0][0][0])[e] = elem(e, 11);
  for (long e = 0; e < 12; ++e) (&B[0][0][0])[e] = elem(e, 21);
  for (long e = 0; e < 20; ++e) (&C[0][0][0])[e] = elem(e, 31);
}

/* checksum: sum of C[e] * ((e % 97) + 1) over the flat array (exact in double) */
static double checksum(const float *p, long n) {
  double s = 0;
  for (long e = 0; e < n; ++e) s += (double)p[e] * (double)((e % 97) + 1);
  return s;
}
#define EXPECTED_CHECKSUM -274.0   /* final C, from the Python oracle */
#define INITIAL_CHECKSUM  -115.0   /* C before the kernel runs */

/* ------------------------------------------------- reference 1: direct
 * The statement over the same loop domain, on flat arrays with explicit
 * row-major offsets.  It shares no index code with the kernel. */

static void reference_direct(const float *Af, const float *Bf, float *Cf) {
  for (long t = 1; t < 2; t += 1)
    for (long i = 0; i < 5; i += 1)
      Cf[(t) * 10 + (i) * 2 + (0)] = Cf[(t) * 10 + (i) * 2 + (0)] + mul(Af[(t) * 15 + (i) * 3 + (1)], Bf[(t) * 6 + (1) * 2 + (0)]);
}

/* -------------------------------------------------------------------- main */

static long first_diff(const float *x, const float *y, long n) {
  for (long e = 0; e < n; ++e)
    if (x[e] != y[e]) return e;
  return -1;
}

int main(void) {
  const long n = 20;
  init();
  memcpy(C_ref, C, sizeof C_ref);
  printf("[gemm_recognition] ext_025 batch_loop_runs_once: extended / batched, class not_gemm\n");
  if (checksum(&C[0][0][0], n) != INITIAL_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: initial data differs from the Python oracle\n");
    printf("RESULT ext_025 FAIL (test bug)\n"); return 3;
  }
  reference_direct(&A[0][0][0], &B[0][0][0], &C_ref[0][0][0]);
  if (checksum(&C_ref[0][0][0], n) != EXPECTED_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: C reference checksum %.0f, Python oracle %.0f\n",
           checksum(&C_ref[0][0][0], n), EXPECTED_CHECKSUM);
    printf("RESULT ext_025 FAIL (test bug)\n"); return 3;
  }
  printf("[gemm_recognition] references: checksum %.0f matches the Python oracle\n", EXPECTED_CHECKSUM);

#ifdef GEMM_RECOGNITION_STATS
  systolic_offload_stats_t before, after;
  systolic_offload_stats(&before);
#endif
  ext_025_kernel(A, B, C);   /* the kernel under test */
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
    printf("RESULT ext_025 PASS\n");
    return 0;
  }
  if (d >= 0)
    printf("[gemm_recognition] kernel: first mismatch at flat element %ld: got %g, reference %g\n",
           d, (double)(&C[0][0][0])[d], (double)(&C_ref[0][0][0])[d]);
  printf("[gemm_recognition] kernel: checksum %.0f, expected %.0f\n", got, EXPECTED_CHECKSUM);
  printf("RESULT ext_025 FAIL\n");
  return 1;
}
