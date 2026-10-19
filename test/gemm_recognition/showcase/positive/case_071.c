/*
 * Case ID: case_071
 * Name: counter_names_row_col_depth
 * Set / category: positive / loop_syntax
 * Mathematical class: gemm
 * C program valid: yes (every subscript in bounds, no undefined behaviour)
 *
 * Mathematical operation:
 *   for row in [0, 6), col in [0, 5), depth in [0, 7) (loops outer to inner):
 *     C[row][col] += A[row][depth] * B[depth][col]
 *
 *   Logical GEMM: C'(I,J) += A'(I,K) * B'(K,J)   M=6 N=5 K=7
 *     I is enumerated by loop(s) row (6 values): extent 6
 *     J is enumerated by loop(s) col (5 values): extent 5
 *     K is enumerated by loop(s) depth (7 values): extent 7
 *     A'(I,K) = A[row][depth]   (I index 0..5 via 'row'; K index 0..6 via 'depth')
 *     B'(K,J) = B[depth][col]   (K index 0..6 via 'depth'; J index 0..4 via 'col')
 *     C'(I,J) = C[row][col]   (I index 0..5 via 'row'; J index 0..4 via 'col')
 *
 * Array dimensions: A: float[6][7]   B: float[7][5]   C: float[6][5]
 * Loop order (outer to inner): row [row I], col [column J], depth [reduction K]
 * Index formulas: A: A[row][depth]   B: B[depth][col]   C: C[row][col]
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
 *     cc -std=c11 -O1 case_071.c -o case_071 && ./case_071
 *   Through the compiler (software runtime model, NOT the FPGA):
 *     build/bin/systolic-cc --runtime=soft --explain --verify -DGEMM_RECOGNITION_STATS \
 *       case_071.c -o case_071 && ./case_071
 *   On the board: the same with --runtime=board and SYSTOLIC_FPGA_CONFIG set.
 *   Stage by stage, results collected: scripts/run_showcase.py --case case_071
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

void case_071_kernel(float A[restrict 6][7], float B[restrict 7][5], float C[restrict 6][5]) {
  for (int row = 0; row < 6; ++row)
    for (int col = 0; col < 5; ++col)
      for (int depth = 0; depth < 7; ++depth)
        C[row][col] += A[row][depth] * B[depth][col];
}

/* -------------------------------------------------------------------- data */

static float A[6][7];
static float B[7][5];
static float C[6][5];
static float C_ref[6][5];   /* reference 1 */
static float C_log[6][5];   /* reference 2 */

/* small integers in [-3, 3]; the same formula as generator/casespec.py */
static float elem(long e, long seed) { return (float)((e * 37 + seed * 101) % 103 % 7 - 3); }
/* the product through a call: the references are not GEMM statements */
static float mul(float x, float y) { return x * y; }

static void init(void) {
  for (long e = 0; e < 42; ++e) (&A[0][0])[e] = elem(e, 11);
  for (long e = 0; e < 35; ++e) (&B[0][0])[e] = elem(e, 21);
  for (long e = 0; e < 30; ++e) (&C[0][0])[e] = elem(e, 31);
}

/* checksum: sum of C[e] * ((e % 97) + 1) over the flat array (exact in double) */
static double checksum(const float *p, long n) {
  double s = 0;
  for (long e = 0; e < n; ++e) s += (double)p[e] * (double)((e % 97) + 1);
  return s;
}
#define EXPECTED_CHECKSUM -391.0   /* final C, from the Python oracle */
#define INITIAL_CHECKSUM  70.0   /* C before the kernel runs */

/* ------------------------------------------------- reference 1: direct
 * The statement over the same loop domain, on flat arrays with explicit
 * row-major offsets.  It shares no index code with the kernel. */

static void reference_direct(const float *Af, const float *Bf, float *Cf) {
  for (long row = 0; row < 6; row += 1)
    for (long col = 0; col < 5; col += 1)
      for (long depth = 0; depth < 7; depth += 1)
        Cf[(row) * 5 + (col)] = Cf[(row) * 5 + (col)] + mul(Af[(row) * 7 + (depth)], Bf[(depth) * 5 + (col)]);
}

/* ------------------------------------------------ reference 2: logical view
 * The nest as the GEMM it is: the logical operands gathered through each
 * operand's own index map, a plain GEMM on them, the result scattered back
 * through C's map.  Ordinals: each coordinate's loops, outer to inner,
 * numbered densely from 0. */

static void reference_logical(const float *Af, const float *Bf, float *Cf) {
  static float Av[6][7], Bv[7][5], Cv[6][5];
  {
    /* A'(I,K) = A[row][depth] */
    for (long row = 0; row < 6; row += 1)
      for (long depth = 0; depth < 7; depth += 1)
        Av[row][depth] = Af[(row) * 7 + (depth)];
    /* B'(K,J) = B[depth][col] */
    for (long depth = 0; depth < 7; depth += 1)
      for (long col = 0; col < 5; col += 1)
        Bv[depth][col] = Bf[(depth) * 5 + (col)];
    /* C'(I,J) = C[row][col] */
    for (long row = 0; row < 6; row += 1)
      for (long col = 0; col < 5; col += 1)
        Cv[row][col] = Cf[(row) * 5 + (col)];
    /* the GEMM C' += A' x B' (M=6, N=5, K=7) */
    for (long rep = 0; rep < 1; ++rep)
      for (long I = 0; I < 6; ++I)
        for (long J = 0; J < 5; ++J)
          for (long Kx = 0; Kx < 7; ++Kx)
            Cv[I][J] = Cv[I][J] + mul(Av[I][Kx], Bv[Kx][J]);
    /* scatter C' back through C's own view */
    for (long row = 0; row < 6; row += 1)
      for (long col = 0; col < 5; col += 1)
        Cf[(row) * 5 + (col)] = Cv[row][col];
  }
}

/* -------------------------------------------------------------------- main */

static long first_diff(const float *x, const float *y, long n) {
  for (long e = 0; e < n; ++e)
    if (x[e] != y[e]) return e;
  return -1;
}

int main(void) {
  const long n = 30;
  init();
  memcpy(C_ref, C, sizeof C_ref);
  memcpy(C_log, C, sizeof C_log);
  printf("[gemm_recognition] case_071 counter_names_row_col_depth: positive / loop_syntax, class gemm\n");
  if (checksum(&C[0][0], n) != INITIAL_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: initial data differs from the Python oracle\n");
    printf("RESULT case_071 FAIL (test bug)\n"); return 3;
  }
  reference_direct(&A[0][0], &B[0][0], &C_ref[0][0]);
  reference_logical(&A[0][0], &B[0][0], &C_log[0][0]);
  if (first_diff(&C_ref[0][0], &C_log[0][0], n) >= 0) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: the direct and logical-view references differ\n");
    printf("RESULT case_071 FAIL (test bug)\n"); return 3;
  }
  printf("[gemm_recognition] references: direct and logical-view agree (%ld elements)\n", n);
  if (checksum(&C_ref[0][0], n) != EXPECTED_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: C reference checksum %.0f, Python oracle %.0f\n",
           checksum(&C_ref[0][0], n), EXPECTED_CHECKSUM);
    printf("RESULT case_071 FAIL (test bug)\n"); return 3;
  }
  printf("[gemm_recognition] references: checksum %.0f matches the Python oracle\n", EXPECTED_CHECKSUM);

#ifdef GEMM_RECOGNITION_STATS
  systolic_offload_stats_t before, after;
  systolic_offload_stats(&before);
#endif
  case_071_kernel(A, B, C);   /* the kernel under test */
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
    printf("RESULT case_071 PASS\n");
    return 0;
  }
  if (d >= 0)
    printf("[gemm_recognition] kernel: first mismatch at flat element %ld: got %g, reference %g\n",
           d, (double)(&C[0][0])[d], (double)(&C_ref[0][0])[d]);
  printf("[gemm_recognition] kernel: checksum %.0f, expected %.0f\n", got, EXPECTED_CHECKSUM);
  printf("RESULT case_071 FAIL\n");
  return 1;
}
