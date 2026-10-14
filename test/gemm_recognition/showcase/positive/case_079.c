/*
 * Case ID: case_079
 * Name: kji_origins_offsets_strides
 * Set / category: positive / combination
 * Mathematical class: gemm
 * C program valid: yes (every subscript in bounds, no undefined behaviour)
 *
 * Mathematical operation:
 *   for k in [1, 9), j in [2, 10), i in [0, 8) (loops outer to inner):
 *     C[2 * i + 1][3 * j] += A[2 * i + 1][k + 2] * B[k + 2][3 * j]
 *
 *   Logical GEMM: C'(I,J) += A'(I,K) * B'(K,J)   M=8 N=8 K=8
 *     I is enumerated by loop(s) i (8 values): extent 8
 *     J is enumerated by loop(s) j (8 values): extent 8
 *     K is enumerated by loop(s) k (8 values): extent 8
 *     A'(I,K) = A[2 * i + 1][k + 2]   (I index 1..15 via '2 * i + 1'; K index 3..10 via 'k + 2')
 *     B'(K,J) = B[k + 2][3 * j]   (K index 3..10 via 'k + 2'; J index 6..27 via '3 * j')
 *     C'(I,J) = C[2 * i + 1][3 * j]   (I index 1..15 via '2 * i + 1'; J index 6..27 via '3 * j')
 *
 * Array dimensions: A: float[16][11]   B: float[11][28]   C: float[16][28]
 * Loop order (outer to inner): k [reduction K], j [column J], i [row I]
 * Index formulas: A: A[2 * i + 1][k + 2]   B: B[k + 2][3 * j]   C: C[2 * i + 1][3 * j]
 * Statement form: C[..] += A[..] * B[..]
 * Element type: float
 *
 * Mathematical GEMM: YES
 * Expected recognition (specification): ACCEPT
 *   basis: a dense GEMM with affine operand views: include/Systolic/AffineMatmulPartitioning.h ('loops in any order, steps, nonzero origins, access offsets and strides, and coordinates split over several loops ... are all of this form'); include/Systolic/AffineMatmulAnalysis.h (F_A = phiA o (P,I,K), F_B = phiB o (P,K,J), F_C = phiC o (P,I,J): one factor map per operand); the task statement's index family c_i*i + c_j*j + c_k*k + c_0 per operand
 * Current implementation (commit 6e25528, predicted from its sources; measured by
 * scripts/run_showcase.py, never assumed):
 *   front end (systolic-cfront):             translated
 *   recognition (recognizeLogicalMatmul):    ACCEPT
 *   dense partition (getDenseLogicalGemm):   supported
 *   lowering (--affine-matmul-to-systolic):  supported
 *   driver (systolic-cc):                    offloaded
 * Classification: TRUE POSITIVE
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
 *     cc -std=c11 -O1 case_079.c -o case_079 && ./case_079
 *   Through the compiler (software runtime model, NOT the FPGA):
 *     build/bin/systolic-cc --runtime=soft --explain --verify -DGEMM_RECOGNITION_STATS \
 *       case_079.c -o case_079 && ./case_079
 *   On the board: the same with --runtime=board and SYSTOLIC_FPGA_CONFIG set.
 *   Stage by stage, results collected: scripts/run_showcase.py --case case_079
 * Exit status: 0 = PASS (the kernel equals every reference), 1 = FAIL, 3 = the
 * references disagree among themselves (a bug in this test, not in the compiler).
 *
 * Loops k, j, i with origins 1 and 2; I = 2i + 1 (odd rows), J = 3j (j from 2:
 * columns 6, 9, ..., 27), K = k + 2 (k from 1: 3..10).
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

void case_079_kernel(float A[restrict 16][11], float B[restrict 11][28], float C[restrict 16][28]) {
  for (int k = 1; k < 9; ++k)
    for (int j = 2; j < 10; ++j)
      for (int i = 0; i < 8; ++i)
        C[2 * i + 1][3 * j] += A[2 * i + 1][k + 2] * B[k + 2][3 * j];
}

/* -------------------------------------------------------------------- data */

static float A[16][11];
static float B[11][28];
static float C[16][28];
static float C_ref[16][28];   /* reference 1 */
static float C_log[16][28];   /* reference 2 */

/* small integers in [-3, 3]; the same formula as generator/casespec.py */
static float elem(long e, long seed) { return (float)((e * 37 + seed * 101) % 103 % 7 - 3); }
/* the product through a call: the references are not GEMM statements */
static float mul(float x, float y) { return x * y; }

static void init(void) {
  for (long e = 0; e < 176; ++e) (&A[0][0])[e] = elem(e, 11);
  for (long e = 0; e < 308; ++e) (&B[0][0])[e] = elem(e, 21);
  for (long e = 0; e < 448; ++e) (&C[0][0])[e] = elem(e, 31);
}

/* checksum: sum of C[e] * ((e % 97) + 1) over the flat array (exact in double) */
static double checksum(const float *p, long n) {
  double s = 0;
  for (long e = 0; e < n; ++e) s += (double)p[e] * (double)((e % 97) + 1);
  return s;
}
#define EXPECTED_CHECKSUM -2974.0   /* final C, from the Python oracle */
#define INITIAL_CHECKSUM  -1627.0   /* C before the kernel runs */

/* ------------------------------------------------- reference 1: direct
 * The statement over the same loop domain, on flat arrays with explicit
 * row-major offsets.  It shares no index code with the kernel. */

static void reference_direct(const float *Af, const float *Bf, float *Cf) {
  for (long k = 1; k < 9; k += 1)
    for (long j = 2; j < 10; j += 1)
      for (long i = 0; i < 8; i += 1)
        Cf[(2 * i + 1) * 28 + (3 * j)] = Cf[(2 * i + 1) * 28 + (3 * j)] + mul(Af[(2 * i + 1) * 11 + (k + 2)], Bf[(k + 2) * 28 + (3 * j)]);
}

/* ------------------------------------------------ reference 2: logical view
 * The nest as the GEMM it is: the logical operands gathered through each
 * operand's own index map, a plain GEMM on them, the result scattered back
 * through C's map.  Ordinals: each coordinate's loops, outer to inner,
 * numbered densely from 0. */

static void reference_logical(const float *Af, const float *Bf, float *Cf) {
  static float Av[8][8], Bv[8][8], Cv[8][8];
  {
    /* A'(I,K) = A[2 * i + 1][k + 2] */
    for (long i = 0; i < 8; i += 1)
      for (long k = 1; k < 9; k += 1)
        Av[i][((k - 1) / 1) * 1] = Af[(2 * i + 1) * 11 + (k + 2)];
    /* B'(K,J) = B[k + 2][3 * j] */
    for (long k = 1; k < 9; k += 1)
      for (long j = 2; j < 10; j += 1)
        Bv[((k - 1) / 1) * 1][((j - 2) / 1) * 1] = Bf[(k + 2) * 28 + (3 * j)];
    /* C'(I,J) = C[2 * i + 1][3 * j] */
    for (long i = 0; i < 8; i += 1)
      for (long j = 2; j < 10; j += 1)
        Cv[i][((j - 2) / 1) * 1] = Cf[(2 * i + 1) * 28 + (3 * j)];
    /* the GEMM C' += A' x B' (M=8, N=8, K=8) */
    for (long rep = 0; rep < 1; ++rep)
      for (long I = 0; I < 8; ++I)
        for (long J = 0; J < 8; ++J)
          for (long Kx = 0; Kx < 8; ++Kx)
            Cv[I][J] = Cv[I][J] + mul(Av[I][Kx], Bv[Kx][J]);
    /* scatter C' back through C's own view */
    for (long i = 0; i < 8; i += 1)
      for (long j = 2; j < 10; j += 1)
        Cf[(2 * i + 1) * 28 + (3 * j)] = Cv[i][((j - 2) / 1) * 1];
  }
}

/* -------------------------------------------------------------------- main */

static long first_diff(const float *x, const float *y, long n) {
  for (long e = 0; e < n; ++e)
    if (x[e] != y[e]) return e;
  return -1;
}

int main(void) {
  const long n = 448;
  init();
  memcpy(C_ref, C, sizeof C_ref);
  memcpy(C_log, C, sizeof C_log);
  printf("[gemm_recognition] case_079 kji_origins_offsets_strides: positive / combination, class gemm\n");
  if (checksum(&C[0][0], n) != INITIAL_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: initial data differs from the Python oracle\n");
    printf("RESULT case_079 FAIL (test bug)\n"); return 3;
  }
  reference_direct(&A[0][0], &B[0][0], &C_ref[0][0]);
  reference_logical(&A[0][0], &B[0][0], &C_log[0][0]);
  if (first_diff(&C_ref[0][0], &C_log[0][0], n) >= 0) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: the direct and logical-view references differ\n");
    printf("RESULT case_079 FAIL (test bug)\n"); return 3;
  }
  printf("[gemm_recognition] references: direct and logical-view agree (%ld elements)\n", n);
  if (checksum(&C_ref[0][0], n) != EXPECTED_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: C reference checksum %.0f, Python oracle %.0f\n",
           checksum(&C_ref[0][0], n), EXPECTED_CHECKSUM);
    printf("RESULT case_079 FAIL (test bug)\n"); return 3;
  }
  printf("[gemm_recognition] references: checksum %.0f matches the Python oracle\n", EXPECTED_CHECKSUM);

#ifdef GEMM_RECOGNITION_STATS
  systolic_offload_stats_t before, after;
  systolic_offload_stats(&before);
#endif
  case_079_kernel(A, B, C);   /* the kernel under test */
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
    printf("RESULT case_079 PASS\n");
    return 0;
  }
  if (d >= 0)
    printf("[gemm_recognition] kernel: first mismatch at flat element %ld: got %g, reference %g\n",
           d, (double)(&C[0][0])[d], (double)(&C_ref[0][0])[d]);
  printf("[gemm_recognition] kernel: checksum %.0f, expected %.0f\n", got, EXPECTED_CHECKSUM);
  printf("RESULT case_079 FAIL\n");
  return 1;
}
