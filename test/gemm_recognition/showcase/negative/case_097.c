/*
 * Case ID: case_097
 * Name: no_accumulation
 * Set / category: negative / reduction_mismatch
 * Mathematical class: assignment_last_write
 * C program valid: yes (every subscript in bounds, no undefined behaviour)
 *
 * Mathematical operation:
 *   for i in [0, 8), j in [0, 8), k in [0, 8) (loops outer to inner):
 *     C[i][j] = A[i][k] * B[k][j]
 *
 *   Not a dense GEMM of these operands because:
 *     - the statement overwrites C (C = A * B, no accumulation) and some element of C is written more than once: only the last product survives, not the GEMM of A and B (docs/AFFINE_GEMM_MAPPING.md section 20)
 *
 *   C[i][j] = A[i][k] * B[k][j] overwrites C in every k iteration; the last one
 *   wins: C[i][j] = A[i][7] B[7][j], the outer product of A's last column and B's
 *   last row (a K = 1 GEMM of those two vectors, not the GEMM of A and B).
 *   Until 2026-10-21 rejected at recognition (docs/AFFINE_GEMM_MAPPING.md section
 *   20): k runs eight times and is not a subscript of C, so the assignment writes
 *   every element eight times.  Since 2026-10-21 (docs section 20.5, last write
 *   wins, on by default with --fp-reassociation) the lowering pass first cuts k,
 *   which C does not read, to its last iteration k = 7: on this box only those
 *   writes survive, so the cut nest C[i][j] = A[i][7] * B[7][j] has the program's
 *   result, writes every element once and is the K = 1 GEMM into a zeroed C
 *   (8 x 8 x 1), offloaded.  The data keep their zeros: a product of a zero and a
 *   negative factor is -0 in the program as written and comes back from the tiles
 *   as +0 (the tiles sum from +0: the signed-zero deviation of docs section 6,
 *   accepted for this case).  The checks are numerical (-0 == +0, any other
 *   difference fails); the elements that differ in the sign of zero only are
 *   counted and reported, and --verify compares numerically (runtime_check
 *   'signed_zero').
 *
 * Array dimensions: A: float[8][8]   B: float[8][8]   C: float[8][8]
 * Loop order (outer to inner): i [row I], j [column J], k [reduction K]
 * Index formulas: A: A[i][k]   B: B[k][j]   C: C[i][j]
 * Statement form: C[..] = A[..] * B[..]
 * Element type: float
 *
 * Mathematical GEMM: YES
 * Expected recognition (specification): ACCEPT
 *   basis: (since 2026-10-21, docs/AFFINE_GEMM_MAPPING.md section 20.5) last write wins: in an assignment C = A * B on a box as written (constant bounds, not empty, A and B other arrays than C), every loop of two or more iterations that C does not read rewrites the same elements of C, so only its last iteration's writes survive; the nest cut to those last iterations has the same result (casespec.last_write_cut, checked against the last write of every element by enumeration) and is accepted when it is an assignment the rules accept -- every element written once, the K = 1 GEMM into a zeroed C (offloaded with --fp-reassociation, as every assignment; strict floating point keeps it on the host as written); the signed zeros of docs section 6 apply (a -0 product comes back as +0); the cut nest: a dense GEMM with affine operand views: include/Systolic/AffineMatmulPartitioning.h ('loops in any order, steps, nonzero origins, access offsets and strides, and coordinates split over several loops ... are all of this form'); include/Systolic/AffineMatmulAnalysis.h (F_A = phiA o (P,I,K), F_B = phiB o (P,K,J), F_C = phiC o (P,I,J): one factor map per operand); the task statement's index family c_i*i + c_j*j + c_k*k + c_0 per operand; (supported since 2026-10-19) the assignment C = A * B whose every element of C is written exactly once is the K = 1 GEMM into a zeroed C: the lowering zeroes C's logical window and the tiles accumulate into it (docs/AFFINE_GEMM_MAPPING.md section 20)
 *   re-decided: RE-DECIDED 2026-10-21 (docs section 20.5, last write wins; the signed-zero deviation of section 6 accepted): ACCEPT -- the reduction loop k, which C does not read, is cut to its last iteration and the cut nest C[i][j] = A[i][7] * B[7][j] is the K = 1 GEMM into a zeroed C, offloaded; numerically equal, the -0 products come back as +0. REJECT before (not_gemm_overwrite: the assignment writes every element of C eight times; rejected at recognition)
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
 *   (2) a checksum of the expected result computed independently in Python
 *       (generator/casespec.py) at generation time: EXPECTED_CHECKSUM below.
 * The references never call the kernel or any compiler-generated code and share
 * no index code with it.  Data: small integers in [-3, 3], C starts non-zero;
 * every partial sum is an integer far below 2^24, so the float results are
 * exact and compared numerically (x == y) -- no tolerance; -0 equals +0. This
 * case keeps its zero factors: the offloaded assignment returns each -0 product
 * as +0 (tiles sum from +0: the deviation of docs/AFFINE_GEMM_MAPPING.md sections
 * 6 and 20, accepted for this case), so an element may differ from the reference
 * in the sign of zero only. Such elements are counted and reported (a -0 result
 * where the reference holds +0 fails), every other difference fails, and
 * systolic-cc --verify compares numerically here (scripts/stages.py verify_flags).
 *
 * How to run:
 *   CPU only (the kernel as written vs the references):
 *     cc -std=c11 -O1 case_097.c -o case_097 && ./case_097
 *   Through the compiler (software runtime model, NOT the FPGA):
 *     build/bin/systolic-cc --runtime=soft --explain --verify -DGEMM_RECOGNITION_STATS \
 *       case_097.c -o case_097 && ./case_097
 *   On the board: the same with --runtime=board and SYSTOLIC_FPGA_CONFIG set.
 *   Stage by stage, results collected: scripts/run_showcase.py --case case_097
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

void case_097_kernel(float A[restrict 8][8], float B[restrict 8][8], float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        C[i][j] = A[i][k] * B[k][j];
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
#define EXPECTED_CHECKSUM -4.0   /* final C, from the Python oracle */
#define INITIAL_CHECKSUM  38.0   /* C before the kernel runs */

/* ------------------------------------------------- reference 1: direct
 * The statement over the same loop domain, on flat arrays with explicit
 * row-major offsets.  It shares no index code with the kernel. */

static void reference_direct(const float *Af, const float *Bf, float *Cf) {
  for (long i = 0; i < 8; i += 1)
    for (long j = 0; j < 8; j += 1)
      for (long k = 0; k < 8; k += 1)
        Cf[(i) * 8 + (j)] = mul(Af[(i) * 8 + (k)], Bf[(k) * 8 + (j)]);
}

/* -------------------------------------------------------------------- main */

static long first_diff(const float *x, const float *y, long n) {
  for (long e = 0; e < n; ++e)
    if (x[e] != y[e]) return e;
  return -1;
}

/* elements numerically equal to the reference but not bitwise: only a zero of
   the other sign (-0 == +0); *wrong_way counts a -0 result where the reference
   holds +0 (the tiles sum from +0 and never make that one) */
static long sign_of_zero_only(const float *x, const float *y, long n, long *wrong_way) {
  const float pz = 0;
  long count = 0;
  *wrong_way = 0;
  for (long e = 0; e < n; ++e)
    if (x[e] == y[e] && memcmp(&x[e], &y[e], sizeof x[e]) != 0) {
      ++count;
      if (memcmp(&x[e], &pz, sizeof pz) != 0) ++*wrong_way;   /* x is -0 */
    }
  return count;
}
#define REFERENCE_NEGATIVE_ZEROS 8   /* -0 elements of the final C, from the Python oracle */

int main(void) {
  const long n = 64;
  init();
  memcpy(C_ref, C, sizeof C_ref);
  printf("[gemm_recognition] case_097 no_accumulation: negative / reduction_mismatch, class assignment_last_write\n");
  if (checksum(&C[0][0], n) != INITIAL_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: initial data differs from the Python oracle\n");
    printf("RESULT case_097 FAIL (test bug)\n"); return 3;
  }
  reference_direct(&A[0][0], &B[0][0], &C_ref[0][0]);
  if (checksum(&C_ref[0][0], n) != EXPECTED_CHECKSUM) {
    printf("[gemm_recognition] REFERENCE INCONSISTENT: C reference checksum %.0f, Python oracle %.0f\n",
           checksum(&C_ref[0][0], n), EXPECTED_CHECKSUM);
    printf("RESULT case_097 FAIL (test bug)\n"); return 3;
  }
  printf("[gemm_recognition] references: checksum %.0f matches the Python oracle\n", EXPECTED_CHECKSUM);
  {
    const float pz = 0;
    long nz = 0;
    for (long e = 0; e < n; ++e)
      if ((&C_ref[0][0])[e] == 0 && memcmp(&(&C_ref[0][0])[e], &pz, sizeof pz) != 0) ++nz;
    if (nz != REFERENCE_NEGATIVE_ZEROS) {
      printf("[gemm_recognition] REFERENCE INCONSISTENT: the reference holds %ld negative zeros, the Python oracle %d\n", nz, REFERENCE_NEGATIVE_ZEROS);
      printf("RESULT case_097 FAIL (test bug)\n"); return 3;
    }
    printf("[gemm_recognition] references: %ld negative zero(s), as the Python oracle says\n", nz);
  }

#ifdef GEMM_RECOGNITION_STATS
  systolic_offload_stats_t before, after;
  systolic_offload_stats(&before);
#endif
  case_097_kernel(A, B, C);   /* the kernel under test */
#ifdef GEMM_RECOGNITION_STATS
  systolic_offload_stats(&after);
  printf("[gemm_recognition] invocations: %llu (backend %s, hardware=%d, failed=%llu)\n",
         (after.per_device[0] + after.per_device[1] + after.per_device[2] + after.per_device[3]) -
         (before.per_device[0] + before.per_device[1] + before.per_device[2] + before.per_device[3]),
         after.backend, after.is_hardware, after.failed - before.failed);
#endif

  long d = first_diff(&C[0][0], &C_ref[0][0], n);
  double got = checksum(&C[0][0], n);
  long wrong_way, sz = sign_of_zero_only(&C[0][0], &C_ref[0][0], n, &wrong_way);
  printf("[gemm_recognition] kernel: %ld element(s) differ from the reference in the sign of zero only (+0 for -0, accepted: docs sections 6 and 20), %ld the other way\n", sz, wrong_way);
  if (d < 0 && got == EXPECTED_CHECKSUM && wrong_way == 0) {
    printf("[gemm_recognition] kernel: numerically equal to the reference (%ld elements, exact; %ld signed zero(s) differ), checksum %.0f\n", n, sz, got);
    printf("RESULT case_097 PASS\n");
    return 0;
  }
  if (d >= 0)
    printf("[gemm_recognition] kernel: first mismatch at flat element %ld: got %g, reference %g\n",
           d, (double)(&C[0][0])[d], (double)(&C_ref[0][0])[d]);
  printf("[gemm_recognition] kernel: checksum %.0f, expected %.0f\n", got, EXPECTED_CHECKSUM);
  printf("RESULT case_097 FAIL\n");
  return 1;
}
