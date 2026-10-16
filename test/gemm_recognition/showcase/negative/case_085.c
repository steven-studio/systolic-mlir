/*
 * Case ID: case_085
 * Name: batched_gemm
 * Set / category: negative / batched
 * Mathematical class: gemm_batched
 * C program valid: yes (every subscript in bounds, no undefined behaviour)
 *
 * Mathematical operation:
 *   for b in [0, 2), i in [0, 4), j in [0, 4), k in [0, 4) (loops outer to inner):
 *     C[b][i][j] += A[b][i][k] * B[b][k][j]
 *
 *   Logical GEMM: C'(I,J) += A'(I,K) * B'(K,J)   M=4 N=4 K=4, one per value of batch coordinate b
 *     I is enumerated by loop(s) i (4 values): extent 4
 *     J is enumerated by loop(s) j (4 values): extent 4
 *     K is enumerated by loop(s) k (4 values): extent 4
 *     A'(I,K) = A[b][i][k]   (I index 0..3 via 'i'; K index 0..3 via 'k')
 *     B'(K,J) = B[b][k][j]   (K index 0..3 via 'k'; J index 0..3 via 'j')
 *     C'(I,J) = C[b][i][j]   (I index 0..3 via 'i'; J index 0..3 via 'j')
 *     tags: batched
 *
 *   A batched GEMM: for each b, C[b] += A[b] B[b] (two 4x4x4 GEMMs).  This IS a
 *   valid GEMM (a partition coordinate P = b in front of I, J, K).  A documented
 *   limitation until 2026-10-15; offloaded since ISSUE-006: the batch loop is
 *   re-emitted around the tiles, two invocations per call.
 *
 * Array dimensions: A: float[2][4][4]   B: float[2][4][4]   C: float[2][4][4]
 * Loop order (outer to inner): b [batch P], i [row I], j [column J], k [reduction K]
 * Index formulas: A: A[b][i][k]   B: B[b][k][j]   C: C[b][i][j]
 * Statement form: C[..] += A[..] * B[..]
 * Element type: float
 *
 * Mathematical GEMM: YES
 * Expected recognition (specification): ACCEPT
 *   basis: (supported since 2026-10-15, ISSUE-006) a batched GEMM, every operand carrying the batch coordinate(s) through one and the same expression: the batch loops are re-emitted around the tiles, one GEMM per batch iteration on its own slice of each operand (docs/AFFINE_GEMM_MAPPING.md section 2; AffineMatmulToSystolic.cpp)
 * Current implementation (commit 6e25528 with the fix of ISSUE-006, predicted from its sources; measured by
 * scripts/run_showcase.py, never assumed):
 *   front end (systolic-cfront):             translated
 *   recognition (recognizeLogicalMatmul):    ACCEPT
 *   dense partition (getDenseLogicalGemm):   supported
 *   lowering (--affine-matmul-to-systolic):  supported
 *   driver (systolic-cc):                    offloaded
 * Classification: TRUE POSITIVE -- was a FALSE NEGATIVE of failures/issues/ISSUE-006 (fixed)
 *
 * Independent CPU reference: yes --
 *   (1) direct evaluation of the statement over the loop domain on flat arrays
 *       with explicit row-major offsets, the product through a function call;
 *   (2) the logical GEMM view: gather A', B' through each operand's own index
 *       map, plain GEMM, scatter into C' (must agree with (1));
 *   (3) a checksum of the expected result computed independently in Python
 *       (generator/casespec.py) at generation time: EXPECTED_CHECKSUM below.
 * The references never call the kernel or any compiler-generated code and share
 * no index code with it.  Data: small integers in [-3, 3], C starts non-zero;
 * every partial sum is an integer far below 2^24, so the float results are
 * exact and compared for exact equality -- no tolerance.
 *
 * How to run:
 *   CPU only (the kernel as written vs the references):
 *     cc -std=c11 -O1 case_085.c -o case_085 && ./case_085
 *   Through the compiler (software runtime model, NOT the FPGA):
 *     build/bin/systolic-cc --runtime=soft --explain --verify -DGEMM_RECOGNITION_STATS \
 *       case_085.c -o case_085 && ./case_085
 *   On the board: the same with --runtime=board and SYSTOLIC_FPGA_CONFIG set.
 *   Stage by stage, results collected: scripts/run_showcase.py --case case_085
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

void case_085_kernel(float A[restrict 2][4][4], float B[restrict 2][4][4], float C[restrict 2][4][4]) {
  for (int b = 0; b < 2; ++b)
    for (int i = 0; i < 4; ++i)
      for (int j = 0; j < 4; ++j)
        for (int k = 0; k < 4; ++k)
          C[b][i][j] += A[b][i][k] * B[b][k][j];
}

/* -------------------------------------------------------------------- data */

static float A[2][4][4];
static float B[2][4][4];
static float C[2][4][4];
static float C_ref[2][4][4];   /* reference 1 */
static float C_log[2][4][4];   /* reference 2 */

/* small integers in [-3, 3]; the same formula as generator/casespec.py */
static float elem(long e, long seed) { return (float)((e * 37 + seed * 101) % 103 % 7 - 3); }
/* the product through a call: the references are not GEMM statements */
static float mul(float x, float y) { return x * y; }

static void init(void) {
  for (long e = 0; e < 32; ++e) (&A[0][0][0])[e] = elem(e, 11);
  for (long e = 0; e < 32; ++e) (&B[0][0][0])[e] = elem(e, 21);
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
          Cf[(b) * 16 + (i) * 4 + (j)] = Cf[(b) * 16 + (i) * 4 + (j)] + mul(Af[(b) * 16 + (i) * 4 + (k)], Bf[(b) * 16 + (k) * 4 + (j)]);
}

/* ------------------------------------------------ reference 2: logical view
 * The nest as the GEMM it is: the logical operands gathered through each
 * operand's own index map, a plain GEMM on them, the result scattered back
 * through C's map.  Ordinals: each coordinate's loops, outer to inner,
 * numbered densely from 0. */

static void reference_logical(const float *Af, const float *Bf, float *Cf) {
  static float Av[4][4], Bv[4][4], Cv[4][4];
  /* one logical GEMM per value of the batch coordinate(s) b */
  for (long b = 0; b < 2; b += 1)
    {
      /* A'(I,K) = A[b][i][k] */
      for (long i = 0; i < 4; i += 1)
        for (long k = 0; k < 4; k += 1)
          Av[i][k] = Af[(b) * 16 + (i) * 4 + (k)];
      /* B'(K,J) = B[b][k][j] */
      for (long k = 0; k < 4; k += 1)
        for (long j = 0; j < 4; j += 1)
          Bv[k][j] = Bf[(b) * 16 + (k) * 4 + (j)];
      /* C'(I,J) = C[b][i][j] */
      for (long i = 0; i < 4; i += 1)
        for (long j = 0; j < 4; j += 1)
          Cv[i][j] = Cf[(b) * 16 + (i) * 4 + (j)];
      /* the GEMM C' += A' x B' (M=4, N=4, K=4) */
      for (long rep = 0; rep < 1; ++rep)
        for (long I = 0; I < 4; ++I)
          for (long J = 0; J < 4; ++J)
            for (long Kx = 0; Kx < 4; ++Kx)
              Cv[I][J] = Cv[I][J] + mul(Av[I][Kx], Bv[Kx][J]);
      /* scatter C' back through C's own view */
      for (long i = 0; i < 4; i += 1)
        for (long j = 0; j < 4; j += 1)
          Cf[(b) * 16 + (i) * 4 + (j)] = Cv[i][j];
    }
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
  memcpy(C_log, C, sizeof C_log);
  printf("[gemm_recognition] case_085 batched_gemm: negative / batched, class gemm_batched\n");
  if (checksum(&C[0][0][0], n) != INITIAL_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: initial data differs from the Python oracle\n");
    printf("RESULT case_085 FAIL (test bug)\n"); return 3;
  }
  reference_direct(&A[0][0][0], &B[0][0][0], &C_ref[0][0][0]);
  reference_logical(&A[0][0][0], &B[0][0][0], &C_log[0][0][0]);
  if (first_diff(&C_ref[0][0][0], &C_log[0][0][0], n) >= 0) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: the direct and logical-view references differ\n");
    printf("RESULT case_085 FAIL (test bug)\n"); return 3;
  }
  printf("[gemm_recognition] references: direct and logical-view agree (%ld elements)\n", n);
  if (checksum(&C_ref[0][0][0], n) != EXPECTED_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: C reference checksum %.0f, Python oracle %.0f\n",
           checksum(&C_ref[0][0][0], n), EXPECTED_CHECKSUM);
    printf("RESULT case_085 FAIL (test bug)\n"); return 3;
  }
  printf("[gemm_recognition] references: checksum %.0f matches the Python oracle\n", EXPECTED_CHECKSUM);

#ifdef GEMM_RECOGNITION_STATS
  systolic_offload_stats_t before, after;
  systolic_offload_stats(&before);
#endif
  case_085_kernel(A, B, C);   /* the kernel under test */
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
    printf("RESULT case_085 PASS\n");
    return 0;
  }
  if (d >= 0)
    printf("[gemm_recognition] kernel: first mismatch at flat element %ld: got %g, reference %g\n",
           d, (double)(&C[0][0][0])[d], (double)(&C_ref[0][0][0])[d]);
  printf("[gemm_recognition] kernel: checksum %.0f, expected %.0f\n", got, EXPECTED_CHECKSUM);
  printf("RESULT case_085 FAIL\n");
  return 1;
}
