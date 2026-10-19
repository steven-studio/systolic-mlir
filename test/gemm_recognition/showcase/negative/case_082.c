/*
 * Case ID: case_082
 * Name: elementwise_3d
 * Set / category: negative / elementwise
 * Mathematical class: elementwise_product
 * C program valid: yes (every subscript in bounds, no undefined behaviour)
 *
 * Mathematical operation:
 *   for i in [0, 4), j in [0, 4), k in [0, 4) (loops outer to inner):
 *     C[i][j][k] += A[i][j][k] * B[i][j][k]
 *
 *   Not a dense GEMM of these operands because:
 *     - A is indexed by ['X', 'X', 'X']; the operand in that position of a GEMM is indexed by exactly ['I', 'K']
 *     - B is indexed by ['X', 'X', 'X']; the operand in that position of a GEMM is indexed by exactly ['J', 'K']
 *     - C is indexed by ['X', 'X', 'X']; the operand in that position of a GEMM is indexed by exactly ['I', 'J']
 *
 *   C[i][j][k] += A[i][j][k] * B[i][j][k]: an element-wise product of rank-3
 *   arrays; every loop appears in every operand, nothing is reduced.  Not a GEMM.
 *   Expected rejection: with three subscripts the recognizer reads the leading
 *   one as a batch coordinate and needs A = (P, I, K), B = (P, K, J), C = (P, I, J)
 *   with I, J, K distinct; here I = J = K.
 *
 * Array dimensions: A: float[4][4][4]   B: float[4][4][4]   C: float[4][4][4]
 * Loop order (outer to inner): i [other], j [other], k [other]
 * Index formulas: A: A[i][j][k]   B: B[i][j][k]   C: C[i][j][k]
 * Statement form: C[..] += A[..] * B[..]
 * Element type: float
 *
 * Mathematical GEMM: NO
 * Expected recognition (specification): REJECT
 *   basis: the nest is not a dense GEMM of its operands (analyse(): see why_not_a_dense_gemm)
 * Current implementation (commit 6e25528, predicted from its sources; measured by
 * scripts/run_showcase.py, never assumed):
 *   front end (systolic-cfront):             translated
 *   recognition (recognizeLogicalMatmul):    REJECT
 *   dense partition (getDenseLogicalGemm):   not_reached
 *   lowering (--affine-matmul-to-systolic):  not_reached
 *   driver (systolic-cc):                    stays_on_host
 *   rejection at: recognition
 *   reason: 3 coordinate(s) are read by all three operands and none by two: an element-wise product
 * Classification: TRUE NEGATIVE
 *   Not a dense GEMM of these operands; the compiler must keep refusing it.
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
 *     cc -std=c11 -O1 case_082.c -o case_082 && ./case_082
 *   Through the compiler (software runtime model, NOT the FPGA):
 *     build/bin/systolic-cc --runtime=soft --explain --verify -DGEMM_RECOGNITION_STATS \
 *       case_082.c -o case_082 && ./case_082
 *   On the board: the same with --runtime=board and SYSTOLIC_FPGA_CONFIG set.
 *   Stage by stage, results collected: scripts/run_showcase.py --case case_082
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

void case_082_kernel(float A[restrict 4][4][4], float B[restrict 4][4][4], float C[restrict 4][4][4]) {
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      for (int k = 0; k < 4; ++k)
        C[i][j][k] += A[i][j][k] * B[i][j][k];
}

/* -------------------------------------------------------------------- data */

static float A[4][4][4];
static float B[4][4][4];
static float C[4][4][4];
static float C_ref[4][4][4];   /* reference 1 */

/* small integers in [-3, 3]; the same formula as generator/casespec.py */
static float elem(long e, long seed) { return (float)((e * 37 + seed * 101) % 103 % 7 - 3); }
/* the product through a call: the references are not GEMM statements */
static float mul(float x, float y) { return x * y; }

static void init(void) {
  for (long e = 0; e < 64; ++e) (&A[0][0][0])[e] = elem(e, 11);
  for (long e = 0; e < 64; ++e) (&B[0][0][0])[e] = elem(e, 21);
  for (long e = 0; e < 64; ++e) (&C[0][0][0])[e] = elem(e, 31);
}

/* checksum: sum of C[e] * ((e % 97) + 1) over the flat array (exact in double) */
static double checksum(const float *p, long n) {
  double s = 0;
  for (long e = 0; e < n; ++e) s += (double)p[e] * (double)((e % 97) + 1);
  return s;
}
#define EXPECTED_CHECKSUM 2243.0   /* final C, from the Python oracle */
#define INITIAL_CHECKSUM  38.0   /* C before the kernel runs */

/* ------------------------------------------------- reference 1: direct
 * The statement over the same loop domain, on flat arrays with explicit
 * row-major offsets.  It shares no index code with the kernel. */

static void reference_direct(const float *Af, const float *Bf, float *Cf) {
  for (long i = 0; i < 4; i += 1)
    for (long j = 0; j < 4; j += 1)
      for (long k = 0; k < 4; k += 1)
        Cf[(i) * 16 + (j) * 4 + (k)] = Cf[(i) * 16 + (j) * 4 + (k)] + mul(Af[(i) * 16 + (j) * 4 + (k)], Bf[(i) * 16 + (j) * 4 + (k)]);
}

/* -------------------------------------------------------------------- main */

static long first_diff(const float *x, const float *y, long n) {
  for (long e = 0; e < n; ++e)
    if (x[e] != y[e]) return e;
  return -1;
}

int main(void) {
  const long n = 64;
  init();
  memcpy(C_ref, C, sizeof C_ref);
  printf("[gemm_recognition] case_082 elementwise_3d: negative / elementwise, class elementwise_product\n");
  if (checksum(&C[0][0][0], n) != INITIAL_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: initial data differs from the Python oracle\n");
    printf("RESULT case_082 FAIL (test bug)\n"); return 3;
  }
  reference_direct(&A[0][0][0], &B[0][0][0], &C_ref[0][0][0]);
  if (checksum(&C_ref[0][0][0], n) != EXPECTED_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: C reference checksum %.0f, Python oracle %.0f\n",
           checksum(&C_ref[0][0][0], n), EXPECTED_CHECKSUM);
    printf("RESULT case_082 FAIL (test bug)\n"); return 3;
  }
  printf("[gemm_recognition] references: checksum %.0f matches the Python oracle\n", EXPECTED_CHECKSUM);

#ifdef GEMM_RECOGNITION_STATS
  systolic_offload_stats_t before, after;
  systolic_offload_stats(&before);
#endif
  case_082_kernel(A, B, C);   /* the kernel under test */
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
    printf("RESULT case_082 PASS\n");
    return 0;
  }
  if (d >= 0)
    printf("[gemm_recognition] kernel: first mismatch at flat element %ld: got %g, reference %g\n",
           d, (double)(&C[0][0][0])[d], (double)(&C_ref[0][0][0])[d]);
  printf("[gemm_recognition] kernel: checksum %.0f, expected %.0f\n", got, EXPECTED_CHECKSUM);
  printf("RESULT case_082 FAIL\n");
  return 1;
}
