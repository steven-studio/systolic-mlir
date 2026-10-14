/*
 * Case ID: case_087
 * Name: coupled_i_plus_j
 * Set / category: negative / coupled_mapping
 * Mathematical class: gemm_nonrectangular_region
 * C program valid: yes (every subscript in bounds, no undefined behaviour)
 *
 * Mathematical operation:
 *   for i in [0, 8), j in [0, 8), k in [0, 8) (loops outer to inner):
 *     C[i + j][j] += A[i + j][k] * B[k][j]
 *
 *   Not a dense GEMM of these operands because:
 *     - A has a subscript that couples loops of different coordinates (mixed:I+J, K)
 *     - C has a subscript that couples loops of different coordinates (mixed:I+J, J)
 *
 *   C[i+j][j] += A[i+j][k] * B[k][j].  With r = i + j every computed element is
 *   C[r][j] += sum_k A[r][k] B[k][j], an element of the GEMM A B -- but the set of
 *   (r, j) computed is the parallelogram {j <= r < j + 8}, not a rectangle, so it
 *   is not one rectangular GEMM C' += A' B' of strided views.  The recognizer
 *   accepts the nest (I = i + j, J = j, K = k) and getDenseLogicalGemm refuses
 *   it: 'two logical coordinates are driven by the same loop (loop 1 drives I and
 *   J)' -- no box of (I, J, K) is a set of whole iterations.
 *
 * Array dimensions: A: float[15][8]   B: float[8][8]   C: float[15][8]
 * Loop order (outer to inner): i [row I], j [column J], k [reduction K]
 * Index formulas: A: A[i + j][k]   B: B[k][j]   C: C[i + j][j]
 * Statement form: C[..] += A[..] * B[..]
 * Element type: float
 *
 * Mathematical GEMM: NO
 * Expected recognition (specification): REJECT
 *   basis: the nest is not a dense GEMM of its operands (analyse(): see why_not_a_dense_gemm)
 * Current implementation (commit 6e25528, predicted from its sources; measured by
 * scripts/run_showcase.py, never assumed):
 *   front end (systolic-cfront):             translated
 *   recognition (recognizeLogicalMatmul):    ACCEPT
 *   dense partition (getDenseLogicalGemm):   unsupported
 *   lowering (--affine-matmul-to-systolic):  not_reached
 *   driver (systolic-cc):                    stays_on_host
 *   rejection at: dense_partition
 *   reason: two logical coordinates are driven by the same loop (loop 1 drives I and J)
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
 *     cc -std=c11 -O1 case_087.c -o case_087 && ./case_087
 *   Through the compiler (software runtime model, NOT the FPGA):
 *     build/bin/systolic-cc --runtime=soft --explain --verify -DGEMM_RECOGNITION_STATS \
 *       case_087.c -o case_087 && ./case_087
 *   On the board: the same with --runtime=board and SYSTOLIC_FPGA_CONFIG set.
 *   Stage by stage, results collected: scripts/run_showcase.py --case case_087
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

void case_087_kernel(float A[restrict 15][8], float B[restrict 8][8], float C[restrict 15][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        C[i + j][j] += A[i + j][k] * B[k][j];
}

/* -------------------------------------------------------------------- data */

static float A[15][8];
static float B[8][8];
static float C[15][8];
static float C_ref[15][8];   /* reference 1 */

/* small integers in [-3, 3]; the same formula as generator/casespec.py */
static float elem(long e, long seed) { return (float)((e * 37 + seed * 101) % 103 % 7 - 3); }
/* the product through a call: the references are not GEMM statements */
static float mul(float x, float y) { return x * y; }

static void init(void) {
  for (long e = 0; e < 120; ++e) (&A[0][0])[e] = elem(e, 11);
  for (long e = 0; e < 64; ++e) (&B[0][0])[e] = elem(e, 21);
  for (long e = 0; e < 120; ++e) (&C[0][0])[e] = elem(e, 31);
}

/* checksum: sum of C[e] * ((e % 97) + 1) over the flat array (exact in double) */
static double checksum(const float *p, long n) {
  double s = 0;
  for (long e = 0; e < n; ++e) s += (double)p[e] * (double)((e % 97) + 1);
  return s;
}
#define EXPECTED_CHECKSUM -2524.0   /* final C, from the Python oracle */
#define INITIAL_CHECKSUM  -442.0   /* C before the kernel runs */

/* ------------------------------------------------- reference 1: direct
 * The statement over the same loop domain, on flat arrays with explicit
 * row-major offsets.  It shares no index code with the kernel. */

static void reference_direct(const float *Af, const float *Bf, float *Cf) {
  for (long i = 0; i < 8; i += 1)
    for (long j = 0; j < 8; j += 1)
      for (long k = 0; k < 8; k += 1)
        Cf[(i + j) * 8 + (j)] = Cf[(i + j) * 8 + (j)] + mul(Af[(i + j) * 8 + (k)], Bf[(k) * 8 + (j)]);
}

/* -------------------------------------------------------------------- main */

static long first_diff(const float *x, const float *y, long n) {
  for (long e = 0; e < n; ++e)
    if (x[e] != y[e]) return e;
  return -1;
}

int main(void) {
  const long n = 120;
  init();
  memcpy(C_ref, C, sizeof C_ref);
  printf("[gemm_recognition] case_087 coupled_i_plus_j: negative / coupled_mapping, class gemm_nonrectangular_region\n");
  if (checksum(&C[0][0], n) != INITIAL_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: initial data differs from the Python oracle\n");
    printf("RESULT case_087 FAIL (test bug)\n"); return 3;
  }
  reference_direct(&A[0][0], &B[0][0], &C_ref[0][0]);
  if (checksum(&C_ref[0][0], n) != EXPECTED_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: C reference checksum %.0f, Python oracle %.0f\n",
           checksum(&C_ref[0][0], n), EXPECTED_CHECKSUM);
    printf("RESULT case_087 FAIL (test bug)\n"); return 3;
  }
  printf("[gemm_recognition] references: checksum %.0f matches the Python oracle\n", EXPECTED_CHECKSUM);

#ifdef GEMM_RECOGNITION_STATS
  systolic_offload_stats_t before, after;
  systolic_offload_stats(&before);
#endif
  case_087_kernel(A, B, C);   /* the kernel under test */
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
    printf("RESULT case_087 PASS\n");
    return 0;
  }
  if (d >= 0)
    printf("[gemm_recognition] kernel: first mismatch at flat element %ld: got %g, reference %g\n",
           d, (double)(&C[0][0])[d], (double)(&C_ref[0][0])[d]);
  printf("[gemm_recognition] kernel: checksum %.0f, expected %.0f\n", got, EXPECTED_CHECKSUM);
  printf("RESULT case_087 FAIL\n");
  return 1;
}
