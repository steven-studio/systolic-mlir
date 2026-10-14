/*
 * Case ID: case_083
 * Name: matrix_vector
 * Set / category: negative / matrix_vector
 * Mathematical class: matrix_vector
 * C program valid: yes (every subscript in bounds, no undefined behaviour)
 *
 * Mathematical operation:
 *   for i in [0, 8), j in [0, 8) (loops outer to inner):
 *     y[i] += M[i][j] * x[j]
 *
 *   Not a dense GEMM of these operands because:
 *     - x is indexed by ['K']; the operand in that position of a GEMM is indexed by exactly ['J', 'K']
 *     - y is indexed by ['I']; the operand in that position of a GEMM is indexed by exactly ['I', 'J']
 *
 *   y[i] += M[i][j] * x[j]: a matrix-vector product (GEMV).  It is the N = 1
 *   special case of a GEMM in the mathematical sense (y and x as 8x1 matrices),
 *   but as written it has two index dimensions and rank-1 operands.
 *   Expected rejection: only two loops; the recognizer looks for three nested
 *   affine.for ops and rank-2 (or batched rank-3) operands.
 *
 * Array dimensions: M: float[8][8]   x: float[8]   y: float[8]
 * Loop order (outer to inner): i [row I], j [reduction K]
 * Index formulas: A: M[i][j]   B: x[j]   C: y[i]
 * Statement form: C[..] += A[..] * B[..]
 * Element type: float
 *
 * Mathematical GEMM: NO
 * Expected recognition (specification): OUT_OF_SCOPE
 *   basis: AffineMatmulAnalysis.cpp collectAffineMatmulCandidates expects a three-deep affine.for nest with rank-2 (or batched rank-3) operands; a GEMV has two index dimensions
 * Current implementation (commit 6e25528, predicted from its sources; measured by
 * scripts/run_showcase.py, never assumed):
 *   front end (systolic-cfront):             translated
 *   recognition (recognizeLogicalMatmul):    REJECT
 *   dense partition (getDenseLogicalGemm):   not_reached
 *   lowering (--affine-matmul-to-systolic):  not_reached
 *   driver (systolic-cc):                    stays_on_host
 *   rejection at: recognition
 *   reason: fewer than three loop counters are used by the accesses: no three-deep affine.for candidate whose loops are all GEMM dimensions
 * Classification: DOCUMENTED LIMITATION
 *   A mathematically valid GEMM that the specification excludes explicitly;
 *   'refused' must never be read as 'not a GEMM'.
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
 *     cc -std=c11 -O1 case_083.c -o case_083 && ./case_083
 *   Through the compiler (software runtime model, NOT the FPGA):
 *     build/bin/systolic-cc --runtime=soft --explain --verify -DGEMM_RECOGNITION_STATS \
 *       case_083.c -o case_083 && ./case_083
 *   On the board: the same with --runtime=board and SYSTOLIC_FPGA_CONFIG set.
 *   Stage by stage, results collected: scripts/run_showcase.py --case case_083
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

void case_083_kernel(float M[restrict 8][8], float x[restrict 8], float y[restrict 8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      y[i] += M[i][j] * x[j];
}

/* -------------------------------------------------------------------- data */

static float M[8][8];
static float x[8];
static float y[8];
static float C_ref[8];   /* reference 1 */

/* small integers in [-3, 3]; the same formula as generator/casespec.py */
static float elem(long e, long seed) { return (float)((e * 37 + seed * 101) % 103 % 7 - 3); }
/* the product through a call: the references are not GEMM statements */
static float mul(float x, float y) { return x * y; }

static void init(void) {
  for (long e = 0; e < 64; ++e) (&M[0][0])[e] = elem(e, 11);
  for (long e = 0; e < 8; ++e) (&x[0])[e] = elem(e, 21);
  for (long e = 0; e < 8; ++e) (&y[0])[e] = elem(e, 31);
}

/* checksum: sum of C[e] * ((e % 97) + 1) over the flat array (exact in double) */
static double checksum(const float *p, long n) {
  double s = 0;
  for (long e = 0; e < n; ++e) s += (double)p[e] * (double)((e % 97) + 1);
  return s;
}
#define EXPECTED_CHECKSUM -363.0   /* final C, from the Python oracle */
#define INITIAL_CHECKSUM  -8.0   /* C before the kernel runs */

/* ------------------------------------------------- reference 1: direct
 * The statement over the same loop domain, on flat arrays with explicit
 * row-major offsets.  It shares no index code with the kernel. */

static void reference_direct(const float *Mf, const float *xf, float *yf) {
  for (long i = 0; i < 8; i += 1)
    for (long j = 0; j < 8; j += 1)
      yf[(i)] = yf[(i)] + mul(Mf[(i) * 8 + (j)], xf[(j)]);
}

/* -------------------------------------------------------------------- main */

static long first_diff(const float *x, const float *y, long n) {
  for (long e = 0; e < n; ++e)
    if (x[e] != y[e]) return e;
  return -1;
}

int main(void) {
  const long n = 8;
  init();
  memcpy(C_ref, y, sizeof C_ref);
  printf("[gemm_recognition] case_083 matrix_vector: negative / matrix_vector, class matrix_vector\n");
  if (checksum(&y[0], n) != INITIAL_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: initial data differs from the Python oracle\n");
    printf("RESULT case_083 FAIL (test bug)\n"); return 3;
  }
  reference_direct(&M[0][0], &x[0], &C_ref[0]);
  if (checksum(&C_ref[0], n) != EXPECTED_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: C reference checksum %.0f, Python oracle %.0f\n",
           checksum(&C_ref[0], n), EXPECTED_CHECKSUM);
    printf("RESULT case_083 FAIL (test bug)\n"); return 3;
  }
  printf("[gemm_recognition] references: checksum %.0f matches the Python oracle\n", EXPECTED_CHECKSUM);

#ifdef GEMM_RECOGNITION_STATS
  systolic_offload_stats_t before, after;
  systolic_offload_stats(&before);
#endif
  case_083_kernel(M, x, y);   /* the kernel under test */
#ifdef GEMM_RECOGNITION_STATS
  systolic_offload_stats(&after);
  printf("[gemm_recognition] invocations: %llu (backend %s, hardware=%d, failed=%llu)\n",
         (after.per_device[0] + after.per_device[1] + after.per_device[2] + after.per_device[3]) -
         (before.per_device[0] + before.per_device[1] + before.per_device[2] + before.per_device[3]),
         after.backend, after.is_hardware, after.failed - before.failed);
#endif

  long d = first_diff(&y[0], &C_ref[0], n);
  double got = checksum(&y[0], n);
  if (d < 0 && got == EXPECTED_CHECKSUM) {
    printf("[gemm_recognition] kernel: equal to the reference (%ld elements, exact), checksum %.0f\n", n, got);
    printf("RESULT case_083 PASS\n");
    return 0;
  }
  if (d >= 0)
    printf("[gemm_recognition] kernel: first mismatch at flat element %ld: got %g, reference %g\n",
           d, (double)(&y[0])[d], (double)(&C_ref[0])[d]);
  printf("[gemm_recognition] kernel: checksum %.0f, expected %.0f\n", got, EXPECTED_CHECKSUM);
  printf("RESULT case_083 FAIL\n");
  return 1;
}
