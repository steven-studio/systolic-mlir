/*
 * Case ID: ext_027
 * Name: test2_on_a_box
 * Set / category: extended / generalized_views
 * Mathematical class: elementwise_product_of_views
 * C program valid: yes (every subscript in bounds, no undefined behaviour)
 *
 * Mathematical operation:
 *   for i in [2, 10), j in [0, 6), k in [0, 2) (loops outer to inner):
 *     C[i + j - 2 * k][i + 2 * j + k] += A[i + j - 2 * k][i + j + k] * B[i + j + k][i + 2 * j + k]
 *
 *   Not a dense GEMM of these operands because:
 *     - A is indexed by ['X', 'X']; the operand in that position of a GEMM is indexed by exactly ['I', 'K']
 *     - B is indexed by ['X', 'X']; the operand in that position of a GEMM is indexed by exactly ['J', 'K']
 *     - C is indexed by ['X', 'X']; the operand in that position of a GEMM is indexed by exactly ['I', 'J']
 *
 *   Test 2, C[i+j-2k][i+2j+k] += A[i+j-2k][i+j+k] * B[i+j+k][i+2j+k] on the
 *   box i in [2, 10), j in [0, 6), k in [0, 2): every loop is read by all
 *   three operands, so on a box all three are batch loops and no loop is a
 *   row, column or reduction loop -- an element-wise product of views, not
 *   a GEMM (docs/AFFINE_GEMM_MAPPING.md section 19.6: 96 points, 87 outputs,
 *   9 written twice).  It is a GEMM only on the lattice parallelepiped D*
 *   (a different domain, section 19.6; test/Systolic/affine_gemm_generalized.mlir
 *   @test2_reparametrized).  Expected rejection at recognition (element-wise);
 *   the nest runs on the host and its result is checked.
 *
 * Array dimensions: A: float[15][16]   B: float[16][21]   C: float[15][21]
 * Loop order (outer to inner): i [other], j [other], k [other]
 * Index formulas: A: A[i + j - 2 * k][i + j + k]   B: B[i + j + k][i + 2 * j + k]   C: C[i + j - 2 * k][i + 2 * j + k]
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
 *   reason: 1 coordinate(s) are read by all three operands and none by two: an element-wise product
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
 *     cc -std=c11 -O1 ext_027.c -o ext_027 && ./ext_027
 *   Through the compiler (software runtime model, NOT the FPGA):
 *     build/bin/systolic-cc --runtime=soft --explain --verify -DGEMM_RECOGNITION_STATS \
 *       ext_027.c -o ext_027 && ./ext_027
 *   On the board: the same with --runtime=board and SYSTOLIC_FPGA_CONFIG set.
 *   Stage by stage, results collected: scripts/run_showcase.py --case ext_027
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

void ext_027_kernel(float A[restrict 15][16], float B[restrict 16][21], float C[restrict 15][21]) {
  for (int i = 2; i < 10; ++i)
    for (int j = 0; j < 6; ++j)
      for (int k = 0; k < 2; ++k)
        C[i + j - 2 * k][i + 2 * j + k] += A[i + j - 2 * k][i + j + k] * B[i + j + k][i + 2 * j + k];
}

/* -------------------------------------------------------------------- data */

static float A[15][16];
static float B[16][21];
static float C[15][21];
static float C_ref[15][21];   /* reference 1 */

/* small integers in [-3, 3]; the same formula as generator/casespec.py */
static float elem(long e, long seed) { return (float)((e * 37 + seed * 101) % 103 % 7 - 3); }
/* the product through a call: the references are not GEMM statements */
static float mul(float x, float y) { return x * y; }

static void init(void) {
  for (long e = 0; e < 240; ++e) (&A[0][0])[e] = elem(e, 11);
  for (long e = 0; e < 336; ++e) (&B[0][0])[e] = elem(e, 21);
  for (long e = 0; e < 315; ++e) (&C[0][0])[e] = elem(e, 31);
}

/* checksum: sum of C[e] * ((e % 97) + 1) over the flat array (exact in double) */
static double checksum(const float *p, long n) {
  double s = 0;
  for (long e = 0; e < n; ++e) s += (double)p[e] * (double)((e % 97) + 1);
  return s;
}
#define EXPECTED_CHECKSUM -1779.0   /* final C, from the Python oracle */
#define INITIAL_CHECKSUM  -1377.0   /* C before the kernel runs */

/* ------------------------------------------------- reference 1: direct
 * The statement over the same loop domain, on flat arrays with explicit
 * row-major offsets.  It shares no index code with the kernel. */

static void reference_direct(const float *Af, const float *Bf, float *Cf) {
  for (long i = 2; i < 10; i += 1)
    for (long j = 0; j < 6; j += 1)
      for (long k = 0; k < 2; k += 1)
        Cf[(i + j - 2 * k) * 21 + (i + 2 * j + k)] = Cf[(i + j - 2 * k) * 21 + (i + 2 * j + k)] + mul(Af[(i + j - 2 * k) * 16 + (i + j + k)], Bf[(i + j + k) * 21 + (i + 2 * j + k)]);
}

/* -------------------------------------------------------------------- main */

static long first_diff(const float *x, const float *y, long n) {
  for (long e = 0; e < n; ++e)
    if (x[e] != y[e]) return e;
  return -1;
}

int main(void) {
  const long n = 315;
  init();
  memcpy(C_ref, C, sizeof C_ref);
  printf("[gemm_recognition] ext_027 test2_on_a_box: extended / generalized_views, class elementwise_product_of_views\n");
  if (checksum(&C[0][0], n) != INITIAL_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: initial data differs from the Python oracle\n");
    printf("RESULT ext_027 FAIL (test bug)\n"); return 3;
  }
  reference_direct(&A[0][0], &B[0][0], &C_ref[0][0]);
  if (checksum(&C_ref[0][0], n) != EXPECTED_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: C reference checksum %.0f, Python oracle %.0f\n",
           checksum(&C_ref[0][0], n), EXPECTED_CHECKSUM);
    printf("RESULT ext_027 FAIL (test bug)\n"); return 3;
  }
  printf("[gemm_recognition] references: checksum %.0f matches the Python oracle\n", EXPECTED_CHECKSUM);

#ifdef GEMM_RECOGNITION_STATS
  systolic_offload_stats_t before, after;
  systolic_offload_stats(&before);
#endif
  ext_027_kernel(A, B, C);   /* the kernel under test */
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
    printf("RESULT ext_027 PASS\n");
    return 0;
  }
  if (d >= 0)
    printf("[gemm_recognition] kernel: first mismatch at flat element %ld: got %g, reference %g\n",
           d, (double)(&C[0][0])[d], (double)(&C_ref[0][0])[d]);
  printf("[gemm_recognition] kernel: checksum %.0f, expected %.0f\n", got, EXPECTED_CHECKSUM);
  printf("RESULT ext_027 FAIL\n");
  return 1;
}
