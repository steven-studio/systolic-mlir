/*
 * Case ID: case_095
 * Name: reduction_index_in_output
 * Set / category: negative / reduction_mismatch
 * Mathematical class: batched_scaled_vectors_by_demotion
 * C program valid: yes (every subscript in bounds, no undefined behaviour)
 *
 * Mathematical operation:
 *   for i in [0, 8), j in [0, 8), k in [0, 8) (loops outer to inner):
 *     C[i][k] += A[i][k] * B[k][j]
 *
 *   Not a dense GEMM of these operands because:
 *     - C is indexed by ['I', 'K']; the operand in that position of a GEMM is indexed by exactly ['I', 'J']
 *
 *   C[i][k] += A[i][k] * B[k][j]: the output is indexed by the would-be
 *   reduction index k and the loop j is summed away: C[i][k] += A[i][k] * (sum_j
 *   B[k][j]) -- A scaled column-wise by the row sums of B.  By the readers
 *   (docs/AFFINE_GEMM_MAPPING.md section 19.2) k is read by all three operands (a
 *   batch loop), i by A and C (I), j by B only (a broadcast): the canonical
 *   decomposition fails.  With j demoted to a batch loop it is a batched GEMM:
 *   per (k, j) the 8 x 1 x 1 tile C[I][k] += A[I][k] B[k][j] (a vector scaled by
 *   a scalar), the j batches accumulating into one column in program order.
 *   Mathematically valid; the demotion is not implemented: a documented
 *   limitation.  With a single-iteration j the canonical decomposition is that
 *   batched GEMM and it is offloaded (SQ-5, decided 2026-10-16).  Expected
 *   refusal at recognition: 'a coordinate is read by the second factor only' (j).
 *   Re-decided 2026-10-16 (ISSUE-008; REJECT before).
 *
 * Array dimensions: A: float[8][8]   B: float[8][8]   C: float[8][8]
 * Loop order (outer to inner): i [row I], j [column J], k [reduction K]
 * Index formulas: A: A[i][k]   B: B[k][j]   C: C[i][k]
 * Statement form: C[..] += A[..] * B[..]
 * Element type: float
 *
 * Mathematical GEMM: YES
 * Expected recognition (specification): OUT_OF_SCOPE
 *   basis: (docs/AFFINE_GEMM_MAPPING.md section 19.2) the canonical decomposition (the roles by readers) fails -- a broadcast, two coordinates in one role, a role that is not one dense coordinate, C not injective per batch -- but with more loops as batch loops the nest is a batched GEMM of smaller tiles; the compiler constructs only the canonical decomposition: a documented limitation
 *   re-decided: 2026-10-16 (failures/issues/ISSUE-008, docs/AFFINE_GEMM_MAPPING.md section 19.2): REJECT (not a GEMM) before; OUT_OF_SCOPE since -- a batched GEMM once the broadcast loop is demoted to a batch loop, which the compiler does not construct
 * Current implementation (commit 6e25528, predicted from its sources; measured by
 * scripts/run_showcase.py, never assumed):
 *   front end (systolic-cfront):             translated
 *   recognition (recognizeLogicalMatmul):    REJECT
 *   dense partition (getDenseLogicalGemm):   not_reached
 *   lowering (--affine-matmul-to-systolic):  not_reached
 *   driver (systolic-cc):                    stays_on_host
 *   rejection at: recognition
 *   reason: a coordinate is read by the second factor only: the other operands would be broadcast along it
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
 *     cc -std=c11 -O1 case_095.c -o case_095 && ./case_095
 *   Through the compiler (software runtime model, NOT the FPGA):
 *     build/bin/systolic-cc --runtime=soft --explain --verify -DGEMM_RECOGNITION_STATS \
 *       case_095.c -o case_095 && ./case_095
 *   On the board: the same with --runtime=board and SYSTOLIC_FPGA_CONFIG set.
 *   Stage by stage, results collected: scripts/run_showcase.py --case case_095
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

void case_095_kernel(float A[restrict 8][8], float B[restrict 8][8], float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        C[i][k] += A[i][k] * B[k][j];
}

/* -------------------------------------------------------------------- data */

static float A[8][8];
static float B[8][8];
static float C[8][8];
static float C_ref[8][8];   /* reference 1 */

/* small integers in [-3, 3]; the same formula as generator/casespec.py */
static float elem(long e, long seed) { return (float)((e * 37 + seed * 101) % 103 % 7 - 3); }
/* the product through a call: the references are not GEMM statements */
static float mul(float x, float y) { return x * y; }

static void init(void) {
  for (long e = 0; e < 64; ++e) (&A[0][0])[e] = elem(e, 11);
  for (long e = 0; e < 64; ++e) (&B[0][0])[e] = elem(e, 21);
  for (long e = 0; e < 64; ++e) (&C[0][0])[e] = elem(e, 31);
}

/* checksum: sum of C[e] * ((e % 97) + 1) over the flat array (exact in double) */
static double checksum(const float *p, long n) {
  double s = 0;
  for (long e = 0; e < n; ++e) s += (double)p[e] * (double)((e % 97) + 1);
  return s;
}
#define EXPECTED_CHECKSUM 122.0   /* final C, from the Python oracle */
#define INITIAL_CHECKSUM  38.0   /* C before the kernel runs */

/* ------------------------------------------------- reference 1: direct
 * The statement over the same loop domain, on flat arrays with explicit
 * row-major offsets.  It shares no index code with the kernel. */

static void reference_direct(const float *Af, const float *Bf, float *Cf) {
  for (long i = 0; i < 8; i += 1)
    for (long j = 0; j < 8; j += 1)
      for (long k = 0; k < 8; k += 1)
        Cf[(i) * 8 + (k)] = Cf[(i) * 8 + (k)] + mul(Af[(i) * 8 + (k)], Bf[(k) * 8 + (j)]);
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
  printf("[gemm_recognition] case_095 reduction_index_in_output: negative / reduction_mismatch, class batched_scaled_vectors_by_demotion\n");
  if (checksum(&C[0][0], n) != INITIAL_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: initial data differs from the Python oracle\n");
    printf("RESULT case_095 FAIL (test bug)\n"); return 3;
  }
  reference_direct(&A[0][0], &B[0][0], &C_ref[0][0]);
  if (checksum(&C_ref[0][0], n) != EXPECTED_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: C reference checksum %.0f, Python oracle %.0f\n",
           checksum(&C_ref[0][0], n), EXPECTED_CHECKSUM);
    printf("RESULT case_095 FAIL (test bug)\n"); return 3;
  }
  printf("[gemm_recognition] references: checksum %.0f matches the Python oracle\n", EXPECTED_CHECKSUM);

#ifdef GEMM_RECOGNITION_STATS
  systolic_offload_stats_t before, after;
  systolic_offload_stats(&before);
#endif
  case_095_kernel(A, B, C);   /* the kernel under test */
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
    printf("RESULT case_095 PASS\n");
    return 0;
  }
  if (d >= 0)
    printf("[gemm_recognition] kernel: first mismatch at flat element %ld: got %g, reference %g\n",
           d, (double)(&C[0][0])[d], (double)(&C_ref[0][0])[d]);
  printf("[gemm_recognition] kernel: checksum %.0f, expected %.0f\n", got, EXPECTED_CHECKSUM);
  printf("RESULT case_095 FAIL\n");
  return 1;
}
