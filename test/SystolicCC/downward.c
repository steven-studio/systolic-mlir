// REQUIRES: cfront-clang
//
// GEMMs whose C loops count down, through the whole compiler: the front end
// turns a loop that counts down into the ordinal affine.for 0 to N with the
// counter LB - S * t, the recognizer reads the decreasing coordinate
// backwards, and the systolic arrays compute the same rows / columns / sums.
// '>' and '>=', steps that do and do not divide the distance to the bound,
// every coordinate counting down (I, J, the reduction K), a mixed nest, an
// upward control and a loop that never runs (an empty GEMM: open spec
// question SQ-2, stays on the host). Every case is checked against an
// independent CPU reference that spells out the visited indices (products
// through a call), with the invocations the runtime made.
//
// NO REAL HARDWARE HERE: the software runtime model computes every tile;
// SOFT_RUNTIME_MOCK_HARDWARE=1 makes it claim to be the FPGA.
//
// RUN: %systolic-cc --explain %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// PLAN-DAG: note: 'dn_i_ge' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'dn_i_gt' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'dn_i_ge_divisible' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'dn_i_gt_divisible' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'dn_j_gt_step3' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'dn_k' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'dn_all' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'dn_mixed' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'up_i_step2' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'dn_empty' stays on the host: no GEMM the systolic arrays can run was recognized in it (line {{[0-9]+}}: a loop of the nest has no iteration (an empty GEMM: M, N or K is 0))
// PLAN:     wrote {{.*}} (9 function(s) offloaded
//
// RUN: %t > %t.soft
// RUN: FileCheck %s --check-prefixes=SOFT,ALL --input-file=%t.soft --match-full-lines
// SOFT-NOT: FPGA
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t > %t.mock
// RUN: FileCheck %s --check-prefixes=MOCK,ALL --input-file=%t.mock --match-full-lines
//
// ALL:        downward loops test
// SOFT-NEXT:  [systolic-cc] CPU Executed: GEMM #1
// MOCK-NEXT:  [systolic-cc] FPGA Executed: GEMM #1
// ALL:        [case] dn_i_ge: equal to the reference, 2 invocation(s)
// ALL:        [case] dn_i_gt: equal to the reference, 2 invocation(s)
// ALL:        [case] dn_i_ge_divisible: equal to the reference, 2 invocation(s)
// ALL:        [case] dn_i_gt_divisible: equal to the reference, 2 invocation(s)
// ALL:        [case] dn_j_gt_step3: equal to the reference, 2 invocation(s)
// ALL:        [case] dn_k: equal to the reference, 1 invocation(s)
// ALL:        [case] dn_all: equal to the reference, 1 invocation(s)
// ALL:        [case] up_i_step2: equal to the reference, 2 invocation(s)
// ALL:        [case] dn_empty: equal to the reference, 0 invocation(s)
// ALL:        [case] dn_mixed: equal to the reference, 2 invocation(s)
// ALL:        PASS: 0 case(s) differ
//
// With --verify every offloaded call is also compared with the function as
// written, run on the CPU.
// RUN: %systolic-cc --verify %s -o %t.v > /dev/null 2>&1
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t.v 2>&1 >/dev/null | FileCheck %s --check-prefix=VERIFY
// VERIFY-COUNT-9: verified:
// VERIFY-NOT:     MISMATCH

#include <stdio.h>
#include <string.h>

#include "systolic_offload.h"

/* ---- GEMMs whose loops count down ---------------------------------------- */

/* i = 5, 3, 1 (>= 0, step 2 does not divide 5 - 0 + 1): M = 3. */
void dn_i_ge(float A[restrict 8][8], float B[restrict 8][8], float C[restrict 8][8]) {
  for (int i = 5; i >= 0; i -= 2)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

/* i = 5, 3, 1 again, with '>': i > 0 excludes 0, which is not reached anyway. */
void dn_i_gt(float A[restrict 8][8], float B[restrict 8][8], float C[restrict 8][8]) {
  for (int i = 5; i > 0; i -= 2)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

/* i = 6, 4, 2, 0 (the step divides the distance; '>=' includes 0): M = 4. */
void dn_i_ge_divisible(float A[restrict 8][8], float B[restrict 8][8], float C[restrict 8][8]) {
  for (int i = 6; i >= 0; i -= 2)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

/* i = 6, 4, 2 ('>' excludes 0): M = 3. */
void dn_i_gt_divisible(float A[restrict 8][8], float B[restrict 8][8], float C[restrict 8][8]) {
  for (int i = 6; i > 0; i = i - 2)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

/* j = 7, 4, 1 (> 0, step 3): N = 3. */
void dn_j_gt_step3(float A[restrict 8][8], float B[restrict 8][8], float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 7; j > 0; j -= 3)
      for (int k = 0; k < 8; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

/* The reduction counts down: k = 7 .. 0. */
void dn_k(float A[restrict 8][8], float B[restrict 8][8], float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 7; k >= 0; --k)
        C[i][j] += A[i][k] * B[k][j];
}

/* Every loop counts down. */
void dn_all(float A[restrict 8][8], float B[restrict 8][8], float C[restrict 8][8]) {
  for (int i = 7; i >= 0; i--)
    for (int j = 7; j >= 0; j--)
      for (int k = 7; k >= 0; k -= 1)
        C[i][j] += A[i][k] * B[k][j];
}

/* i counts up by 2 (0 .. 14), j down by 3 (7, 4, 1): M = 8, N = 3. */
void dn_mixed(float A[restrict 16][8], float B[restrict 8][8], float C[restrict 16][8]) {
  for (int i = 0; i < 16; i += 2)
    for (int j = 7; j >= 0; j -= 3)
      for (int k = 0; k < 8; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

/* The control: i = 1, 3, 5 counting up (the rows of dn_i_ge in the other order). */
void up_i_step2(float A[restrict 8][8], float B[restrict 8][8], float C[restrict 8][8]) {
  for (int i = 1; i < 6; i += 2)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

/* A loop counting down that runs zero times (i = 0, i > 0): an empty GEMM.
   Open spec question SQ-2: refused, stays on the host; C is unchanged. */
void dn_empty(float A[restrict 8][8], float B[restrict 8][8], float C[restrict 8][8]) {
  for (int i = 0; i > 0; --i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

/* ---- the reference ---------------------------------------------------------- */

/* C[r][c] += sum over the listed k of A[r][k] * B[k][c], for the listed rows
   and columns, each product through a call (not a GEMM statement); the index
   lists spell out what each loop above visits. */
static float mul(float a, float b) { return a * b; }
static void ref(const int *rows, int nr, const int *cols, int nc, const int *ks,
                int nk, const float *A, int lda, const float *B, int ldb,
                float *C, int ldc) {
  for (int r = 0; r < nr; ++r)
    for (int c = 0; c < nc; ++c)
      for (int q = 0; q < nk; ++q)
        C[rows[r] * ldc + cols[c]] = C[rows[r] * ldc + cols[c]] +
            mul(A[rows[r] * lda + ks[q]], B[ks[q] * ldb + cols[c]]);
}

static const int ALL8[] = {0, 1, 2, 3, 4, 5, 6, 7};
static const int DOWN8[] = {7, 6, 5, 4, 3, 2, 1, 0};
static const int R531[] = {5, 3, 1}, R6420[] = {6, 4, 2, 0}, R642[] = {6, 4, 2};
static const int C741[] = {7, 4, 1}, R135[] = {1, 3, 5};
static const int EVEN16[] = {0, 2, 4, 6, 8, 10, 12, 14};

static float A8[8][8], B8[8][8], C8[8][8], R8[8][8];
static float A16[16][8], C16[16][8], R16[16][8];
static int failures;

static unsigned long long invocations(void) {
  systolic_offload_stats_t s;
  systolic_offload_stats(&s);
  return s.per_device[0] + s.per_device[1] + s.per_device[2] + s.per_device[3];
}
static void fill(float *p, int n, int seed, int mod) {
  for (int e = 0; e < n; ++e)
    p[e] = (float)((e * (3 + seed % 5) + seed) % mod - mod / 2);
}
static void report(const char *name, const float *got, const float *want, int n,
                   unsigned long long inv) {
  int same = memcmp(got, want, n * sizeof *got) == 0;
  failures += !same;
  printf("[case] %s: %s, %llu invocation(s)\n", name,
         same ? "equal to the reference" : "DIFFERENT FROM THE REFERENCE", inv);
}

#define CASE8(name, rows, cols, ks)                                           \
  do {                                                                        \
    fill(&C8[0][0], 64, __LINE__, 5);                                         \
    memcpy(R8, C8, sizeof R8);                                                \
    unsigned long long before = invocations();                                \
    name(A8, B8, C8);                                                         \
    unsigned long long inv = invocations() - before;                          \
    ref(rows, (int)(sizeof rows / sizeof *rows), cols,                        \
        (int)(sizeof cols / sizeof *cols), ks, (int)(sizeof ks / sizeof *ks), \
        &A8[0][0], 8, &B8[0][0], 8, &R8[0][0], 8);                            \
    report(#name, &C8[0][0], &R8[0][0], 64, inv);                             \
  } while (0)

int main(void) {
  printf("downward loops test\n");
  fill(&A8[0][0], 64, 1, 7);
  fill(&B8[0][0], 64, 2, 5);
  fill(&A16[0][0], 128, 3, 7);
  CASE8(dn_i_ge, R531, ALL8, ALL8);
  CASE8(dn_i_gt, R531, ALL8, ALL8);
  CASE8(dn_i_ge_divisible, R6420, ALL8, ALL8);
  CASE8(dn_i_gt_divisible, R642, ALL8, ALL8);
  CASE8(dn_j_gt_step3, ALL8, C741, ALL8);
  CASE8(dn_k, ALL8, ALL8, DOWN8);
  CASE8(dn_all, DOWN8, DOWN8, DOWN8);
  CASE8(up_i_step2, R135, ALL8, ALL8);
  /* dn_empty: no row is visited -- the reference changes nothing */
  {
    fill(&C8[0][0], 64, __LINE__, 5);
    memcpy(R8, C8, sizeof R8);
    unsigned long long before = invocations();
    dn_empty(A8, B8, C8);
    report("dn_empty", &C8[0][0], &R8[0][0], 64, invocations() - before);
  }
  {
    fill(&C16[0][0], 128, __LINE__, 5);
    memcpy(R16, C16, sizeof R16);
    unsigned long long before = invocations();
    dn_mixed(A16, B8, C16);
    unsigned long long inv = invocations() - before;
    ref(EVEN16, 8, C741, 3, ALL8, 8, &A16[0][0], 8, &B8[0][0], 8, &R16[0][0], 8);
    report("dn_mixed", &C16[0][0], &R16[0][0], 128, inv);
  }
  printf("%s: %d case(s) differ\n", failures ? "FAIL" : "PASS", failures);
  return failures != 0;
}
