/*
 * Case ID: case_088
 * Name: skewed_output_seed
 * Set / category: negative / coupled_mapping
 * Mathematical class: gemm_values_skewed_placement
 * C program valid: yes (every subscript in bounds, no undefined behaviour)
 *
 * Mathematical operation:
 *   for i in [0, 4), j in [0, 4), k in [0, 4) (loops outer to inner):
 *     C[2 * i + j][j] += A[2 * i + 3][k + 1] * B[k + 2][3 * j + 2]
 *
 *   Not a dense GEMM of these operands because:
 *     - C has a subscript that couples loops of different coordinates (mixed:I+J, J)
 *
 *   The user's seed A[2*i+3][k+1], B[k+2][3*j+2], C[2*i+j][j], with 4x4x4 loops.
 *   P[i][j] = sum_k A[2i+3][k+1] B[k+2][3j+2] is a GEMM of two strided views
 *   (A rows 3,5,7,9 x cols 1..4; B rows 2..5 x cols 2,5,8,11) -- note the K views
 *   of A (k+1) and B (k+2) differ, which is fine mathematically.  Its value P[i][j]
 *   is accumulated into C[2i+j][j]: C's view (I, J) -> (2I + J, J) is affine and
 *   injective (rank [lamI lamJ] = 2), so the nest is the GEMM C' += A' B' with
 *   that skewed view C' (docs/AFFINE_GEMM_MAPPING.md section 19: G1, G2).
 *   Re-decided 2026-10-16 (ISSUE-008; until then rejected: not a strided
 *   sub-matrix of C).  Recognized since; the lowering of a skewed view (a
 *   memref.reinterpret_cast) is not implemented yet: a FALSE NEGATIVE, refused
 *   at the lowering ('C has a skewed or merged view').
 *
 * Array dimensions: A: float[10][5]   B: float[6][12]   C: float[10][4]
 * Loop order (outer to inner): i [row I], j [column J], k [reduction K]
 * Index formulas: A: A[2 * i + 3][k + 1]   B: B[k + 2][3 * j + 2]   C: C[2 * i + j][j]
 * Statement form: C[..] += A[..] * B[..]
 * Element type: float
 *
 * Mathematical GEMM: YES
 * Expected recognition (specification): ACCEPT
 *   basis: (since 2026-10-16, docs/AFFINE_GEMM_MAPPING.md section 19) a GEMM, or a batched GEMM, of general affine operand views -- a dimension combining both of an operand's roles (skewed: A[i][i + k]), both roles in one dimension (merged: C[4 i + j]), C reading batch loops in its role dimensions -- decided by enumeration (casespec.generalized_gemm_on_domain: roles by readers, every role one dense coordinate, (I, J, K) a bijection onto the box, C injective per batch)
 *   re-decided: 2026-10-16 (failures/issues/ISSUE-008, docs/AFFINE_GEMM_MAPPING.md section 19): REJECT (not a strided sub-matrix of C) before; ACCEPT since -- a GEMM with a skewed view of C
 * Current implementation (commit 6e25528, predicted from its sources; measured by
 * scripts/run_showcase.py, never assumed):
 *   front end (systolic-cfront):             translated
 *   recognition (recognizeLogicalMatmul):    ACCEPT
 *   dense partition (getDenseLogicalGemm):   supported
 *   lowering (--affine-matmul-to-systolic):  unsupported
 *   driver (systolic-cc):                    stays_on_host
 *   rejection at: lowering
 *   reason: C has a skewed or merged view (a dimension combining both of its roles, or a role across several dimensions: docs/AFFINE_GEMM_MAPPING.md section 19), which is not lowered yet
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
 *     cc -std=c11 -O1 case_088.c -o case_088 && ./case_088
 *   Through the compiler (software runtime model, NOT the FPGA):
 *     build/bin/systolic-cc --runtime=soft --explain --verify -DGEMM_RECOGNITION_STATS \
 *       case_088.c -o case_088 && ./case_088
 *   On the board: the same with --runtime=board and SYSTOLIC_FPGA_CONFIG set.
 *   Stage by stage, results collected: scripts/run_showcase.py --case case_088
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

void case_088_kernel(float A[restrict 10][5], float B[restrict 6][12], float C[restrict 10][4]) {
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      for (int k = 0; k < 4; ++k)
        C[2 * i + j][j] += A[2 * i + 3][k + 1] * B[k + 2][3 * j + 2];
}

/* -------------------------------------------------------------------- data */

static float A[10][5];
static float B[6][12];
static float C[10][4];
static float C_ref[10][4];   /* reference 1 */

/* small integers in [-3, 3]; the same formula as generator/casespec.py */
static float elem(long e, long seed) { return (float)((e * 37 + seed * 101) % 103 % 7 - 3); }
/* the product through a call: the references are not GEMM statements */
static float mul(float x, float y) { return x * y; }

static void init(void) {
  for (long e = 0; e < 50; ++e) (&A[0][0])[e] = elem(e, 11);
  for (long e = 0; e < 72; ++e) (&B[0][0])[e] = elem(e, 21);
  for (long e = 0; e < 40; ++e) (&C[0][0])[e] = elem(e, 31);
}

/* checksum: sum of C[e] * ((e % 97) + 1) over the flat array (exact in double) */
static double checksum(const float *p, long n) {
  double s = 0;
  for (long e = 0; e < n; ++e) s += (double)p[e] * (double)((e % 97) + 1);
  return s;
}
#define EXPECTED_CHECKSUM 119.0   /* final C, from the Python oracle */
#define INITIAL_CHECKSUM  51.0   /* C before the kernel runs */

/* ------------------------------------------------- reference 1: direct
 * The statement over the same loop domain, on flat arrays with explicit
 * row-major offsets.  It shares no index code with the kernel. */

static void reference_direct(const float *Af, const float *Bf, float *Cf) {
  for (long i = 0; i < 4; i += 1)
    for (long j = 0; j < 4; j += 1)
      for (long k = 0; k < 4; k += 1)
        Cf[(2 * i + j) * 4 + (j)] = Cf[(2 * i + j) * 4 + (j)] + mul(Af[(2 * i + 3) * 5 + (k + 1)], Bf[(k + 2) * 12 + (3 * j + 2)]);
}

/* -------------------------------------------------------------------- main */

static long first_diff(const float *x, const float *y, long n) {
  for (long e = 0; e < n; ++e)
    if (x[e] != y[e]) return e;
  return -1;
}

int main(void) {
  const long n = 40;
  init();
  memcpy(C_ref, C, sizeof C_ref);
  printf("[gemm_recognition] case_088 skewed_output_seed: negative / coupled_mapping, class gemm_values_skewed_placement\n");
  if (checksum(&C[0][0], n) != INITIAL_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: initial data differs from the Python oracle\n");
    printf("RESULT case_088 FAIL (test bug)\n"); return 3;
  }
  reference_direct(&A[0][0], &B[0][0], &C_ref[0][0]);
  if (checksum(&C_ref[0][0], n) != EXPECTED_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: C reference checksum %.0f, Python oracle %.0f\n",
           checksum(&C_ref[0][0], n), EXPECTED_CHECKSUM);
    printf("RESULT case_088 FAIL (test bug)\n"); return 3;
  }
  printf("[gemm_recognition] references: checksum %.0f matches the Python oracle\n", EXPECTED_CHECKSUM);

#ifdef GEMM_RECOGNITION_STATS
  systolic_offload_stats_t before, after;
  systolic_offload_stats(&before);
#endif
  case_088_kernel(A, B, C);   /* the kernel under test */
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
    printf("RESULT case_088 PASS\n");
    return 0;
  }
  if (d >= 0)
    printf("[gemm_recognition] kernel: first mismatch at flat element %ld: got %g, reference %g\n",
           d, (double)(&C[0][0])[d], (double)(&C_ref[0][0])[d]);
  printf("[gemm_recognition] kernel: checksum %.0f, expected %.0f\n", got, EXPECTED_CHECKSUM);
  printf("RESULT case_088 FAIL\n");
  return 1;
}
