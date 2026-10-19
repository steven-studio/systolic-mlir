/*
 * Case ID: case_100
 * Name: double_precision
 * Set / category: negative / element_type
 * Mathematical class: gemm
 * C program valid: yes (every subscript in bounds, no undefined behaviour)
 *
 * Mathematical operation:
 *   for i in [0, 4), j in [0, 4), k in [0, 4) (loops outer to inner):
 *     C[i][j] += A[i][k] * B[k][j]
 *
 *   Logical GEMM: C'(I,J) += A'(I,K) * B'(K,J)   M=4 N=4 K=4
 *     I is enumerated by loop(s) i (4 values): extent 4
 *     J is enumerated by loop(s) j (4 values): extent 4
 *     K is enumerated by loop(s) k (4 values): extent 4
 *     A'(I,K) = A[i][k]   (I index 0..3 via 'i'; K index 0..3 via 'k')
 *     B'(K,J) = B[k][j]   (K index 0..3 via 'k'; J index 0..3 via 'j')
 *     C'(I,J) = C[i][j]   (I index 0..3 via 'i'; J index 0..3 via 'j')
 *     tags: double
 *
 *   A valid GEMM in double precision.  Recognized, dense, and the lowering pass
 *   itself would lower it; systolic-cc keeps it on the host because the FPGA
 *   datapath is fp32: 'the FPGA datapath is fp32 and it uses double arrays'.
 *   Legal, unsupported by the driver's policy.
 *
 * Array dimensions: A: double[4][4]   B: double[4][4]   C: double[4][4]
 * Loop order (outer to inner): i [row I], j [column J], k [reduction K]
 * Index formulas: A: A[i][k]   B: B[k][j]   C: C[i][j]
 * Statement form: C[..] += A[..] * B[..]
 * Element type: double
 *
 * Mathematical GEMM: YES
 * Expected recognition (specification): OUT_OF_SCOPE
 *   basis: tools/systolic-cc/systolic-cc.c: 'the FPGA datapath is fp32 and it uses double arrays'; test/SystolicCC/program.c matmul_f64
 * Current implementation (commit 6e25528, predicted from its sources; measured by
 * scripts/run_showcase.py, never assumed):
 *   front end (systolic-cfront):             translated
 *   recognition (recognizeLogicalMatmul):    ACCEPT
 *   dense partition (getDenseLogicalGemm):   supported
 *   lowering (--affine-matmul-to-systolic):  supported
 *   driver (systolic-cc):                    stays_on_host
 *   rejection at: driver
 *   reason: the FPGA datapath is fp32 and it uses double arrays
 * Classification: DOCUMENTED LIMITATION
 *   A mathematically valid GEMM that the specification excludes explicitly;
 *   'refused' must never be read as 'not a GEMM'.
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
 *     cc -std=c11 -O1 case_100.c -o case_100 && ./case_100
 *   Through the compiler (software runtime model, NOT the FPGA):
 *     build/bin/systolic-cc --runtime=soft --explain --verify -DGEMM_RECOGNITION_STATS \
 *       case_100.c -o case_100 && ./case_100
 *   On the board: the same with --runtime=board and SYSTOLIC_FPGA_CONFIG set.
 *   Stage by stage, results collected: scripts/run_showcase.py --case case_100
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

void case_100_kernel(double A[restrict 4][4], double B[restrict 4][4], double C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      for (int k = 0; k < 4; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

/* -------------------------------------------------------------------- data */

static double A[4][4];
static double B[4][4];
static double C[4][4];
static double C_ref[4][4];   /* reference 1 */
static double C_log[4][4];   /* reference 2 */

/* small integers in [-3, 3]; the same formula as generator/casespec.py */
static double elem(long e, long seed) { return (double)((e * 37 + seed * 101) % 103 % 7 - 3); }
/* the product through a call: the references are not GEMM statements */
static double mul(double x, double y) { return x * y; }

static void init(void) {
  for (long e = 0; e < 16; ++e) (&A[0][0])[e] = elem(e, 11);
  for (long e = 0; e < 16; ++e) (&B[0][0])[e] = elem(e, 21);
  for (long e = 0; e < 16; ++e) (&C[0][0])[e] = elem(e, 31);
}

/* checksum: sum of C[e] * ((e % 97) + 1) over the flat array (exact in double) */
static double checksum(const double *p, long n) {
  double s = 0;
  for (long e = 0; e < n; ++e) s += (double)p[e] * (double)((e % 97) + 1);
  return s;
}
#define EXPECTED_CHECKSUM -54.0   /* final C, from the Python oracle */
#define INITIAL_CHECKSUM  -99.0   /* C before the kernel runs */

/* ------------------------------------------------- reference 1: direct
 * The statement over the same loop domain, on flat arrays with explicit
 * row-major offsets.  It shares no index code with the kernel. */

static void reference_direct(const double *Af, const double *Bf, double *Cf) {
  for (long i = 0; i < 4; i += 1)
    for (long j = 0; j < 4; j += 1)
      for (long k = 0; k < 4; k += 1)
        Cf[(i) * 4 + (j)] = Cf[(i) * 4 + (j)] + mul(Af[(i) * 4 + (k)], Bf[(k) * 4 + (j)]);
}

/* ------------------------------------------------ reference 2: logical view
 * The nest as the GEMM it is: the logical operands gathered through each
 * operand's own index map, a plain GEMM on them, the result scattered back
 * through C's map.  Ordinals: each coordinate's loops, outer to inner,
 * numbered densely from 0. */

static void reference_logical(const double *Af, const double *Bf, double *Cf) {
  static double Av[4][4], Bv[4][4], Cv[4][4];
  {
    /* A'(I,K) = A[i][k] */
    for (long i = 0; i < 4; i += 1)
      for (long k = 0; k < 4; k += 1)
        Av[i][k] = Af[(i) * 4 + (k)];
    /* B'(K,J) = B[k][j] */
    for (long k = 0; k < 4; k += 1)
      for (long j = 0; j < 4; j += 1)
        Bv[k][j] = Bf[(k) * 4 + (j)];
    /* C'(I,J) = C[i][j] */
    for (long i = 0; i < 4; i += 1)
      for (long j = 0; j < 4; j += 1)
        Cv[i][j] = Cf[(i) * 4 + (j)];
    /* the GEMM C' += A' x B' (M=4, N=4, K=4) */
    for (long rep = 0; rep < 1; ++rep)
      for (long I = 0; I < 4; ++I)
        for (long J = 0; J < 4; ++J)
          for (long Kx = 0; Kx < 4; ++Kx)
            Cv[I][J] = Cv[I][J] + mul(Av[I][Kx], Bv[Kx][J]);
    /* scatter C' back through C's own view */
    for (long i = 0; i < 4; i += 1)
      for (long j = 0; j < 4; j += 1)
        Cf[(i) * 4 + (j)] = Cv[i][j];
  }
}

/* -------------------------------------------------------------------- main */

static long first_diff(const double *x, const double *y, long n) {
  for (long e = 0; e < n; ++e)
    if (x[e] != y[e]) return e;
  return -1;
}

int main(void) {
  const long n = 16;
  init();
  memcpy(C_ref, C, sizeof C_ref);
  memcpy(C_log, C, sizeof C_log);
  printf("[gemm_recognition] case_100 double_precision: negative / element_type, class gemm\n");
  if (checksum(&C[0][0], n) != INITIAL_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: initial data differs from the Python oracle\n");
    printf("RESULT case_100 FAIL (test bug)\n"); return 3;
  }
  reference_direct(&A[0][0], &B[0][0], &C_ref[0][0]);
  reference_logical(&A[0][0], &B[0][0], &C_log[0][0]);
  if (first_diff(&C_ref[0][0], &C_log[0][0], n) >= 0) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: the direct and logical-view references differ\n");
    printf("RESULT case_100 FAIL (test bug)\n"); return 3;
  }
  printf("[gemm_recognition] references: direct and logical-view agree (%ld elements)\n", n);
  if (checksum(&C_ref[0][0], n) != EXPECTED_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: C reference checksum %.0f, Python oracle %.0f\n",
           checksum(&C_ref[0][0], n), EXPECTED_CHECKSUM);
    printf("RESULT case_100 FAIL (test bug)\n"); return 3;
  }
  printf("[gemm_recognition] references: checksum %.0f matches the Python oracle\n", EXPECTED_CHECKSUM);

#ifdef GEMM_RECOGNITION_STATS
  systolic_offload_stats_t before, after;
  systolic_offload_stats(&before);
#endif
  case_100_kernel(A, B, C);   /* the kernel under test */
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
    printf("RESULT case_100 PASS\n");
    return 0;
  }
  if (d >= 0)
    printf("[gemm_recognition] kernel: first mismatch at flat element %ld: got %g, reference %g\n",
           d, (double)(&C[0][0])[d], (double)(&C_ref[0][0])[d]);
  printf("[gemm_recognition] kernel: checksum %.0f, expected %.0f\n", got, EXPECTED_CHECKSUM);
  printf("RESULT case_100 FAIL\n");
  return 1;
}
