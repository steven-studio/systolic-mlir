/*
 * Case ID: case_089
 * Name: singular_mapping
 * Set / category: negative / singular_mapping
 * Mathematical class: singular_mapping
 * C program valid: yes (every subscript in bounds, no undefined behaviour)
 *
 * Mathematical operation:
 *   for x in [0, 4), y in [0, 4), z in [0, 4) (loops outer to inner):
 *     C[x + z][y + z] += A[x + z][x + y + 2 * z] * B[x + y + 2 * z][y + z]
 *
 *   Not a dense GEMM of these operands because:
 *     - A is indexed by ['X', 'X']; the operand in that position of a GEMM is indexed by exactly ['I', 'K']
 *     - B is indexed by ['X', 'X']; the operand in that position of a GEMM is indexed by exactly ['J', 'K']
 *     - C is indexed by ['X', 'X']; the operand in that position of a GEMM is indexed by exactly ['I', 'J']
 *
 *   test/Systolic/affine_gemm_singular_mapping.mlir in C.  The recognizer reads
 *   I = x + z, J = y + z, K = x + y + 2z = I + J: T = [[1 0 1],[0 1 1],[1 1 2]] is
 *   singular.  K is determined by (I, J), so for a fixed output element nothing
 *   is summed over an independent reduction index; instead several (x, y, z)
 *   map to the same (I, J) and the same product is added repeatedly.  Not a GEMM:
 *   x, y and z are each read by A, B and C, so in every decomposition all three
 *   are batch loops and no loop is a row, column or reduction loop -- an
 *   element-wise product of views (docs/AFFINE_GEMM_MAPPING.md sections 19.1,
 *   19.2).  Expected rejection at recognition: '1 coordinate(s) are read by all
 *   three operands and none by two: an element-wise product'.  (Until 2026-10-16
 *   the coupled positional reading was kept and getDenseLogicalGemm refused it,
 *   'loop 0 drives I and K'; the reading is no longer restored, ISSUE-008.)
 *
 * Array dimensions: A: float[7][13]   B: float[13][7]   C: float[7][7]
 * Loop order (outer to inner): x [other], y [other], z [other]
 * Index formulas: A: A[x + z][x + y + 2 * z]   B: B[x + y + 2 * z][y + z]   C: C[x + z][y + z]
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
 *     cc -std=c11 -O1 case_089.c -o case_089 && ./case_089
 *   Through the compiler (software runtime model, NOT the FPGA):
 *     build/bin/systolic-cc --runtime=soft --explain --verify -DGEMM_RECOGNITION_STATS \
 *       case_089.c -o case_089 && ./case_089
 *   On the board: the same with --runtime=board and SYSTOLIC_FPGA_CONFIG set.
 *   Stage by stage, results collected: scripts/run_showcase.py --case case_089
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

void case_089_kernel(float A[restrict 7][13], float B[restrict 13][7], float C[restrict 7][7]) {
  for (int x = 0; x < 4; ++x)
    for (int y = 0; y < 4; ++y)
      for (int z = 0; z < 4; ++z)
        C[x + z][y + z] += A[x + z][x + y + 2 * z] * B[x + y + 2 * z][y + z];
}

/* -------------------------------------------------------------------- data */

static float A[7][13];
static float B[13][7];
static float C[7][7];
static float C_ref[7][7];   /* reference 1 */

/* small integers in [-3, 3]; the same formula as generator/casespec.py */
static float elem(long e, long seed) { return (float)((e * 37 + seed * 101) % 103 % 7 - 3); }
/* the product through a call: the references are not GEMM statements */
static float mul(float x, float y) { return x * y; }

static void init(void) {
  for (long e = 0; e < 91; ++e) (&A[0][0])[e] = elem(e, 11);
  for (long e = 0; e < 91; ++e) (&B[0][0])[e] = elem(e, 21);
  for (long e = 0; e < 49; ++e) (&C[0][0])[e] = elem(e, 31);
}

/* checksum: sum of C[e] * ((e % 97) + 1) over the flat array (exact in double) */
static double checksum(const float *p, long n) {
  double s = 0;
  for (long e = 0; e < n; ++e) s += (double)p[e] * (double)((e % 97) + 1);
  return s;
}
#define EXPECTED_CHECKSUM 473.0   /* final C, from the Python oracle */
#define INITIAL_CHECKSUM  -180.0   /* C before the kernel runs */

/* ------------------------------------------------- reference 1: direct
 * The statement over the same loop domain, on flat arrays with explicit
 * row-major offsets.  It shares no index code with the kernel. */

static void reference_direct(const float *Af, const float *Bf, float *Cf) {
  for (long x = 0; x < 4; x += 1)
    for (long y = 0; y < 4; y += 1)
      for (long z = 0; z < 4; z += 1)
        Cf[(x + z) * 7 + (y + z)] = Cf[(x + z) * 7 + (y + z)] + mul(Af[(x + z) * 13 + (x + y + 2 * z)], Bf[(x + y + 2 * z) * 7 + (y + z)]);
}

/* -------------------------------------------------------------------- main */

static long first_diff(const float *x, const float *y, long n) {
  for (long e = 0; e < n; ++e)
    if (x[e] != y[e]) return e;
  return -1;
}

int main(void) {
  const long n = 49;
  init();
  memcpy(C_ref, C, sizeof C_ref);
  printf("[gemm_recognition] case_089 singular_mapping: negative / singular_mapping, class singular_mapping\n");
  if (checksum(&C[0][0], n) != INITIAL_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: initial data differs from the Python oracle\n");
    printf("RESULT case_089 FAIL (test bug)\n"); return 3;
  }
  reference_direct(&A[0][0], &B[0][0], &C_ref[0][0]);
  if (checksum(&C_ref[0][0], n) != EXPECTED_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: C reference checksum %.0f, Python oracle %.0f\n",
           checksum(&C_ref[0][0], n), EXPECTED_CHECKSUM);
    printf("RESULT case_089 FAIL (test bug)\n"); return 3;
  }
  printf("[gemm_recognition] references: checksum %.0f matches the Python oracle\n", EXPECTED_CHECKSUM);

#ifdef GEMM_RECOGNITION_STATS
  systolic_offload_stats_t before, after;
  systolic_offload_stats(&before);
#endif
  case_089_kernel(A, B, C);   /* the kernel under test */
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
    printf("RESULT case_089 PASS\n");
    return 0;
  }
  if (d >= 0)
    printf("[gemm_recognition] kernel: first mismatch at flat element %ld: got %g, reference %g\n",
           d, (double)(&C[0][0])[d], (double)(&C_ref[0][0])[d]);
  printf("[gemm_recognition] kernel: checksum %.0f, expected %.0f\n", got, EXPECTED_CHECKSUM);
  printf("RESULT case_089 FAIL\n");
  return 1;
}
