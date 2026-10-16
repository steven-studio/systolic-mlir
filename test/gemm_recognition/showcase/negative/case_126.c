/*
 * Case ID: case_126
 * Name: a_column_couples_i_and_k
 * Set / category: negative / coupled_mapping
 * Mathematical class: gemm_of_skewed_view
 * C program valid: yes (every subscript in bounds, no undefined behaviour)
 *
 * Mathematical operation:
 *   for i in [0, 8), j in [0, 8), k in [0, 8) (loops outer to inner):
 *     C[i][j] += A[i][i + k] * B[k][j]
 *
 *   Not a dense GEMM of these operands because:
 *     - A has a subscript that couples loops of different coordinates (I, mixed:I+K)
 *
 *   C[i][j] += A[i][i+k] * B[k][j]: A is read along a skew, A'(I, K) = A[I][I+K]
 *   (a Hankel-like gather), and C += A' B.  A' is an affine view of A -- its
 *   dimension 1 combines both of A's roles; in elements A'(I, K) is at 16 I + K
 *   (row-major strides (16, 1)) -- and C's view is the identity (injective): a
 *   GEMM by docs/AFFINE_GEMM_MAPPING.md section 19 (G1, G2; Test 1's shape with
 *   B plain).  Re-decided 2026-10-16 (ISSUE-008; until then rejected: 'A' is not
 *   an affine strided view of A').  Recognized since; the lowering of a skewed
 *   view (a memref.reinterpret_cast) is not implemented yet: a FALSE NEGATIVE,
 *   refused at the lowering ('A has a skewed or merged view').
 *
 * Array dimensions: A: float[8][15]   B: float[8][8]   C: float[8][8]
 * Loop order (outer to inner): i [row I], j [column J], k [reduction K]
 * Index formulas: A: A[i][i + k]   B: B[k][j]   C: C[i][j]
 * Statement form: C[..] += A[..] * B[..]
 * Element type: float
 *
 * Mathematical GEMM: YES
 * Expected recognition (specification): ACCEPT
 *   basis: (since 2026-10-16, docs/AFFINE_GEMM_MAPPING.md section 19) a GEMM, or a batched GEMM, of general affine operand views -- a dimension combining both of an operand's roles (skewed: A[i][i + k]), both roles in one dimension (merged: C[4 i + j]), C reading batch loops in its role dimensions -- decided by enumeration (casespec.generalized_gemm_on_domain: roles by readers, every role one dense coordinate, (I, J, K) a bijection onto the box, C injective per batch)
 *   re-decided: 2026-10-16 (failures/issues/ISSUE-008, docs/AFFINE_GEMM_MAPPING.md section 19): REJECT (A' not a strided view of A) before; ACCEPT since -- a GEMM with a skewed view of A
 * Current implementation (commit 6e25528, predicted from its sources; measured by
 * scripts/run_showcase.py, never assumed):
 *   front end (systolic-cfront):             translated
 *   recognition (recognizeLogicalMatmul):    ACCEPT
 *   dense partition (getDenseLogicalGemm):   supported
 *   lowering (--affine-matmul-to-systolic):  unsupported
 *   driver (systolic-cc):                    stays_on_host
 *   rejection at: lowering
 *   reason: A has a skewed or merged view (a dimension combining both of its roles, or a role across several dimensions: docs/AFFINE_GEMM_MAPPING.md section 19), which is not lowered yet
 * Classification: FALSE NEGATIVE -- see failures/issues/ISSUE-008
 *   This nest IS a GEMM and the specification requires it to be recognized;
 *   the current recognizer refuses it.  The expectation is not lowered to match
 *   the implementation: the test fails until the recognizer is fixed.
 *   Lowering support: to be verified once recognized.
 *   FPGA execution support: to be verified once lowered.
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
 *     cc -std=c11 -O1 case_126.c -o case_126 && ./case_126
 *   Through the compiler (software runtime model, NOT the FPGA):
 *     build/bin/systolic-cc --runtime=soft --explain --verify -DGEMM_RECOGNITION_STATS \
 *       case_126.c -o case_126 && ./case_126
 *   On the board: the same with --runtime=board and SYSTOLIC_FPGA_CONFIG set.
 *   Stage by stage, results collected: scripts/run_showcase.py --case case_126
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

void case_126_kernel(float A[restrict 8][15], float B[restrict 8][8], float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        C[i][j] += A[i][i + k] * B[k][j];
}

/* -------------------------------------------------------------------- data */

static float A[8][15];
static float B[8][8];
static float C[8][8];
static float C_ref[8][8];   /* reference 1 */

/* small integers in [-3, 3]; the same formula as generator/casespec.py */
static float elem(long e, long seed) { return (float)((e * 37 + seed * 101) % 103 % 7 - 3); }
/* the product through a call: the references are not GEMM statements */
static float mul(float x, float y) { return x * y; }

static void init(void) {
  for (long e = 0; e < 120; ++e) (&A[0][0])[e] = elem(e, 11);
  for (long e = 0; e < 64; ++e) (&B[0][0])[e] = elem(e, 21);
  for (long e = 0; e < 64; ++e) (&C[0][0])[e] = elem(e, 31);
}

/* checksum: sum of C[e] * ((e % 97) + 1) over the flat array (exact in double) */
static double checksum(const float *p, long n) {
  double s = 0;
  for (long e = 0; e < n; ++e) s += (double)p[e] * (double)((e % 97) + 1);
  return s;
}
#define EXPECTED_CHECKSUM -916.0   /* final C, from the Python oracle */
#define INITIAL_CHECKSUM  38.0   /* C before the kernel runs */

/* ------------------------------------------------- reference 1: direct
 * The statement over the same loop domain, on flat arrays with explicit
 * row-major offsets.  It shares no index code with the kernel. */

static void reference_direct(const float *Af, const float *Bf, float *Cf) {
  for (long i = 0; i < 8; i += 1)
    for (long j = 0; j < 8; j += 1)
      for (long k = 0; k < 8; k += 1)
        Cf[(i) * 8 + (j)] = Cf[(i) * 8 + (j)] + mul(Af[(i) * 15 + (i + k)], Bf[(k) * 8 + (j)]);
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
  printf("[gemm_recognition] case_126 a_column_couples_i_and_k: negative / coupled_mapping, class gemm_of_skewed_view\n");
  if (checksum(&C[0][0], n) != INITIAL_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: initial data differs from the Python oracle\n");
    printf("RESULT case_126 FAIL (test bug)\n"); return 3;
  }
  reference_direct(&A[0][0], &B[0][0], &C_ref[0][0]);
  if (checksum(&C_ref[0][0], n) != EXPECTED_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: C reference checksum %.0f, Python oracle %.0f\n",
           checksum(&C_ref[0][0], n), EXPECTED_CHECKSUM);
    printf("RESULT case_126 FAIL (test bug)\n"); return 3;
  }
  printf("[gemm_recognition] references: checksum %.0f matches the Python oracle\n", EXPECTED_CHECKSUM);

#ifdef GEMM_RECOGNITION_STATS
  systolic_offload_stats_t before, after;
  systolic_offload_stats(&before);
#endif
  case_126_kernel(A, B, C);   /* the kernel under test */
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
    printf("RESULT case_126 PASS\n");
    return 0;
  }
  if (d >= 0)
    printf("[gemm_recognition] kernel: first mismatch at flat element %ld: got %g, reference %g\n",
           d, (double)(&C[0][0])[d], (double)(&C_ref[0][0])[d]);
  printf("[gemm_recognition] kernel: checksum %.0f, expected %.0f\n", got, EXPECTED_CHECKSUM);
  printf("RESULT case_126 FAIL\n");
  return 1;
}
