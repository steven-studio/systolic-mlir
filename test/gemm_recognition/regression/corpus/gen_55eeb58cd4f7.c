/*
 * Case ID: gen_55eeb58cd4f7
 * Name: gen_positive_kji_1x1x1_repeat_inside_K
 * Set / category: generated / mutation_repeat_inside_K
 * Mathematical class: gemm
 * C program valid: yes (every subscript in bounds, no undefined behaviour)
 *
 * Mathematical operation:
 *   for k in [0, 1), j in [0, 1), i in [2, 3), rr in [0, 2) (loops outer to inner):
 *     C[i + 2][j] += A[i + 2][k] * B[k][j]
 *
 *   Logical GEMM: C'(I,J) += A'(I,K) * B'(K,J)   M=1 N=1 K=1, repeated 2 times
 *     I is enumerated by loop(s) i (1 values): extent 1
 *     J is enumerated by loop(s) j (1 values): extent 1
 *     K is enumerated by loop(s) k (1 values): extent 1
 *     A'(I,K) = A[i + 2][k]   (I index 4..4 via 'i + 2'; K index 0..0 via 'k')
 *     B'(K,J) = B[k][j]   (K index 0..0 via 'k'; J index 0..0 via 'j')
 *     C'(I,J) = C[i + 2][j]   (I index 4..4 via 'i + 2'; J index 0..0 via 'j')
 *     tags: repeated_2
 *
 * Array dimensions: A: float[5][1]   B: float[1][1]   C: float[5][1]
 * Loop order (outer to inner): k [reduction K], j [column J], i [row I], rr [repetition]
 * Index formulas: A: A[i + 2][k]   B: B[k][j]   C: C[i + 2][j]
 * Statement form: C[..] += A[..] * B[..]
 * Element type: float
 *
 * Mathematical GEMM: YES
 * Expected recognition (specification): ACCEPT
 *   basis: a dense GEMM with affine operand views: include/Systolic/AffineMatmulPartitioning.h ('loops in any order, steps, nonzero origins, access offsets and strides, and coordinates split over several loops ... are all of this form'); include/Systolic/AffineMatmulAnalysis.h (F_A = phiA o (P,I,K), F_B = phiB o (P,K,J), F_C = phiC o (P,I,J): one factor map per operand); the task statement's index family c_i*i + c_j*j + c_k*k + c_0 per operand
 * Current implementation (commit 6e25528, predicted from its sources; measured by
 * scripts/run_showcase.py, never assumed):
 *   front end (systolic-cfront):             translated
 *   recognition (recognizeLogicalMatmul):    ACCEPT (after the lowering pass hoists the repetition loop outward)
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
 *     cc -std=c11 -O1 gen_55eeb58cd4f7.c -o gen_55eeb58cd4f7 && ./gen_55eeb58cd4f7
 *   Through the compiler (software runtime model, NOT the FPGA):
 *     build/bin/systolic-cc --runtime=soft --explain --verify -DGEMM_RECOGNITION_STATS \
 *       gen_55eeb58cd4f7.c -o gen_55eeb58cd4f7 && ./gen_55eeb58cd4f7
 *   On the board: the same with --runtime=board and SYSTOLIC_FPGA_CONFIG set.
 *   Stage by stage, results collected: scripts/run_showcase.py --case gen_55eeb58cd4f7
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

void gen_55eeb58cd4f7_kernel(float A[restrict 5][1], float B[restrict 1][1], float C[restrict 5][1]) {
  for (int k = 0; k < 1; ++k)
    for (int j = 0; j < 1; ++j)
      for (int i = 2; i < 3; ++i)
        for (int rr = 0; rr < 2; ++rr)
          C[i + 2][j] += A[i + 2][k] * B[k][j];
}

/* -------------------------------------------------------------------- data */

static float A[5][1];
static float B[1][1];
static float C[5][1];
static float C_ref[5][1];   /* reference 1 */
static float C_log[5][1];   /* reference 2 */

/* small integers in [-3, 3]; the same formula as generator/casespec.py */
static float elem(long e, long seed) { return (float)((e * 37 + seed * 101) % 103 % 7 - 3); }
/* the product through a call: the references are not GEMM statements */
static float mul(float x, float y) { return x * y; }

static void init(void) {
  for (long e = 0; e < 5; ++e) (&A[0][0])[e] = elem(e, 224);
  for (long e = 0; e < 1; ++e) (&B[0][0])[e] = elem(e, 234);
  for (long e = 0; e < 5; ++e) (&C[0][0])[e] = elem(e, 244);
}

/* checksum: sum of C[e] * ((e % 97) + 1) over the flat array (exact in double) */
static double checksum(const float *p, long n) {
  double s = 0;
  for (long e = 0; e < n; ++e) s += (double)p[e] * (double)((e % 97) + 1);
  return s;
}
#define EXPECTED_CHECKSUM -38.0   /* final C, from the Python oracle */
#define INITIAL_CHECKSUM  -18.0   /* C before the kernel runs */

/* ------------------------------------------------- reference 1: direct
 * The statement over the same loop domain, on flat arrays with explicit
 * row-major offsets.  It shares no index code with the kernel. */

static void reference_direct(const float *Af, const float *Bf, float *Cf) {
  for (long k = 0; k < 1; k += 1)
    for (long j = 0; j < 1; j += 1)
      for (long i = 2; i < 3; i += 1)
        for (long rr = 0; rr < 2; rr += 1)
          Cf[(i + 2) + (j)] = Cf[(i + 2) + (j)] + mul(Af[(i + 2) + (k)], Bf[(k) + (j)]);
}

/* ------------------------------------------------ reference 2: logical view
 * The nest as the GEMM it is: the logical operands gathered through each
 * operand's own index map, a plain GEMM on them, the result scattered back
 * through C's map.  Ordinals: each coordinate's loops, outer to inner,
 * numbered densely from 0. */

static void reference_logical(const float *Af, const float *Bf, float *Cf) {
  static float Av[1][1], Bv[1][1], Cv[1][1];
  {
    /* A'(I,K) = A[i + 2][k] */
    for (long i = 2; i < 3; i += 1)
      for (long k = 0; k < 1; k += 1)
        Av[((i - 2) / 1) * 1][k] = Af[(i + 2) + (k)];
    /* B'(K,J) = B[k][j] */
    for (long k = 0; k < 1; k += 1)
      for (long j = 0; j < 1; j += 1)
        Bv[k][j] = Bf[(k) + (j)];
    /* C'(I,J) = C[i + 2][j] */
    for (long i = 2; i < 3; i += 1)
      for (long j = 0; j < 1; j += 1)
        Cv[((i - 2) / 1) * 1][j] = Cf[(i + 2) + (j)];
    /* the GEMM C' += A' x B' (M=1, N=1, K=1), repeated 2 times */
    for (long rep = 0; rep < 2; ++rep)
      for (long I = 0; I < 1; ++I)
        for (long J = 0; J < 1; ++J)
          for (long Kx = 0; Kx < 1; ++Kx)
            Cv[I][J] = Cv[I][J] + mul(Av[I][Kx], Bv[Kx][J]);
    /* scatter C' back through C's own view */
    for (long i = 2; i < 3; i += 1)
      for (long j = 0; j < 1; j += 1)
        Cf[(i + 2) + (j)] = Cv[((i - 2) / 1) * 1][j];
  }
}

/* -------------------------------------------------------------------- main */

static long first_diff(const float *x, const float *y, long n) {
  for (long e = 0; e < n; ++e)
    if (x[e] != y[e]) return e;
  return -1;
}

int main(void) {
  const long n = 5;
  init();
  memcpy(C_ref, C, sizeof C_ref);
  memcpy(C_log, C, sizeof C_log);
  printf("[gemm_recognition] gen_55eeb58cd4f7 gen_positive_kji_1x1x1_repeat_inside_K: generated / mutation_repeat_inside_K, class gemm\n");
  if (checksum(&C[0][0], n) != INITIAL_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: initial data differs from the Python oracle\n");
    printf("RESULT gen_55eeb58cd4f7 FAIL (test bug)\n"); return 3;
  }
  reference_direct(&A[0][0], &B[0][0], &C_ref[0][0]);
  reference_logical(&A[0][0], &B[0][0], &C_log[0][0]);
  if (first_diff(&C_ref[0][0], &C_log[0][0], n) >= 0) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: the direct and logical-view references differ\n");
    printf("RESULT gen_55eeb58cd4f7 FAIL (test bug)\n"); return 3;
  }
  printf("[gemm_recognition] references: direct and logical-view agree (%ld elements)\n", n);
  if (checksum(&C_ref[0][0], n) != EXPECTED_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: C reference checksum %.0f, Python oracle %.0f\n",
           checksum(&C_ref[0][0], n), EXPECTED_CHECKSUM);
    printf("RESULT gen_55eeb58cd4f7 FAIL (test bug)\n"); return 3;
  }
  printf("[gemm_recognition] references: checksum %.0f matches the Python oracle\n", EXPECTED_CHECKSUM);

#ifdef GEMM_RECOGNITION_STATS
  systolic_offload_stats_t before, after;
  systolic_offload_stats(&before);
#endif
  gen_55eeb58cd4f7_kernel(A, B, C);   /* the kernel under test */
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
    printf("RESULT gen_55eeb58cd4f7 PASS\n");
    return 0;
  }
  if (d >= 0)
    printf("[gemm_recognition] kernel: first mismatch at flat element %ld: got %g, reference %g\n",
           d, (double)(&C[0][0])[d], (double)(&C_ref[0][0])[d]);
  printf("[gemm_recognition] kernel: checksum %.0f, expected %.0f\n", got, EXPECTED_CHECKSUM);
  printf("RESULT gen_55eeb58cd4f7 FAIL\n");
  return 1;
}
