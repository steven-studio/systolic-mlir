// REQUIRES: cfront-clang
//
// Exact non-box iteration domains end to end (docs/AFFINE_GEMM_MAPPING.md section 19.13,
// 2026-10-18): C nests whose loop bounds do not shift together, translated by the front end
// (bounds affine in the enclosing counters with any terms; conditions canonicalized), the
// compiler's exact normalization onto a box (the integer set compared with a parallelotope),
// repetition triangles hoisted around the GEMM, and the refusals that must stay. Each kernel
// is compared with a reference that runs the same loops on the CPU (products through a call,
// so the reference is not offloaded), bitwise, on integer-valued data:
//
//   ex_degenerate    i in [2, 25) step 2, j in [i, 4)          only i = 2 has points: the 1 x 2 x 4 GEMM
//   ex_rep_triangle  r1 in [0, 3), r0 in [r1, 2) between j, k   the 4 x 4 x 4 GEMM run 3 times (3 invocations)
//   ex_cond_minus    k in [i, ...) while k - i < 4              the condition canonicalized: k < i + 4
//   ex_cond_flipped  k in [i, ...) while i + 4 > k              the same, the comparison reversed
//   ex_k1_empty      e in [0, 2), k in [3 + e, 4)               only e = 0 has points: K = 1 (4 x 4 x 1)
//   ex_axa_views     e in [0, 2), k in [4e, 4): A[i][k] A[j][k]  A . A^T, two views of one array
//   ex_axa_same      the same domain: A[i][k] A[k][j]           A . A, one view
//   ex_det2          the same domain: C[2i][j]                  det T = 2: a stride-2 C view
//   ex_stepped       e in [0, 2), k in [8e, 8) step 2           stride-2 views, e = 0 only
//   ex_alias_c       the same domain: A[i][j] += A[i][k] B[k][j] the output is a factor: refused, host
//   ex_triangle      j in [i, 4)                                a genuine triangle: PROVEN_INVALID, host
//   ex_rep_reads     r in [i, 4) between i and j                row i repeated 4 - i times: UNKNOWN, host
//   ex_rep_prefix    r1 in [0, 2), r0 in [r1, 2) around k in [i, i + 4)   a repetition prefix around a shifted
//                                                              band: normalized below it, 3 invocations
//   ex_remainder     k1 in [0, 2), k0 in [0, 2 - k1): k = 2 k1 + k0   a strip-mined remainder: the 4 x 4 x 3 GEMM,
//                                                              not centrally symmetric, but the accesses do not
//                                                              tell the iterations apart: UNKNOWN (never
//                                                              PROVEN_INVALID), host
//   ex_coef2         k while 2 * k - i < 4                      outside the front end's subset: plain C
//   ex_oob           e in [0, 2), k in [4e, 4): A[i][k + 1]     out of bounds: PROVEN_INVALID (compiled, not run)
//
// NO REAL HARDWARE HERE: the software runtime model computes every tile;
// SOFT_RUNTIME_MOCK_HARDWARE=1 makes it claim to be the FPGA.
//
// RUN: %systolic-cc --fp-reassociation --explain %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// PLAN-DAG: note: 'ex_degenerate' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'ex_rep_triangle' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 3 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'ex_cond_minus' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'ex_cond_flipped' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'ex_k1_empty' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'ex_axa_views' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'ex_axa_same' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'ex_det2' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'ex_stepped' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'ex_alias_c' stays on the host: {{.*}}A may overlap C
// PLAN-DAG: note: 'ex_triangle' stays on the host: {{.*}}the iteration domain is not centrally symmetric
// PLAN-DAG: note: 'ex_rep_reads' stays on the host
// PLAN-DAG: note: 'ex_rep_prefix' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 3 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'ex_remainder' stays on the host: {{.*}}the loop bounds are not static
// PLAN-DAG: note: 'ex_coef2' is outside the supported C subset and is not translated: the loop condition must compare the counter, with coefficient 1 or -1, with an affine expression of the enclosing loop counters
// PLAN-DAG: note: 'ex_oob' stays on the host: {{.*}}A is accessed out of bounds along dimension 1
// PLAN: wrote {{.*}} (10 function(s) offloaded
//
// RUN: %t > %t.soft
// RUN: FileCheck %s --check-prefix=ALL --input-file=%t.soft
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t > %t.mock
// RUN: FileCheck %s --check-prefix=ALL --input-file=%t.mock
// ALL: [case] ex_degenerate: equal, offloaded (1)
// ALL: [case] ex_rep_triangle: equal, offloaded (3)
// ALL: [case] ex_cond_minus: equal, offloaded (1)
// ALL: [case] ex_cond_flipped: equal, offloaded (1)
// ALL: [case] ex_k1_empty: equal, offloaded (1)
// ALL: [case] ex_axa_views: equal, offloaded (1)
// ALL: [case] ex_axa_same: equal, offloaded (1)
// ALL: [case] ex_det2: equal, offloaded (1)
// ALL: [case] ex_stepped: equal, offloaded (1)
// ALL: [case] ex_alias_c: equal, NOT OFFLOADED (0)
// ALL: [case] ex_triangle: equal, NOT OFFLOADED (0)
// ALL: [case] ex_rep_reads: equal, NOT OFFLOADED (0)
// ALL: [case] ex_rep_prefix: equal, offloaded (3)
// ALL: [case] ex_remainder: equal, NOT OFFLOADED (0)
// ALL: [case] ex_coef2: equal, NOT OFFLOADED (0)
// ALL: PASS: 0 differ
//
// Every offloaded call also against the function as written, on the CPU:
// RUN: %systolic-cc --fp-reassociation --verify %s -o %t.v > /dev/null 2>&1
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t.v 2>&1 >/dev/null | FileCheck %s --check-prefix=VERIFY
// VERIFY-COUNT-10: verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY-NOT: MISMATCH

#include <stdio.h>
#include <string.h>

#include "systolic_offload.h"

void ex_degenerate(float A[restrict 4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int i = 2; i < 25; i += 2) for (int j = i; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] += A[i][k] * B[k][j];
}
void ex_rep_triangle(float A[restrict 4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int j = 0; j < 4; ++j)
    for (int r1 = 0; r1 <= 2; r1++)
      for (int r0 = r1; r0 <= 1; r0++)
        for (int k = 0; k < 4; ++k) for (int i = 0; i < 4; ++i)
          C[i][j] += A[i][k] * B[k][j];
}
void ex_cond_minus(float A[restrict 4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = i; k - i < 4; ++k)
    C[i][j] += A[i][k - i] * B[k - i][j];
}
void ex_cond_flipped(float A[restrict 4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = i; i + 4 > k; ++k)
    C[i][j] += A[i][k - i] * B[k - i][j];
}
void ex_k1_empty(float A[restrict 4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int e = 0; e < 2; ++e) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j)
    for (int k = 3 + e; k < 4; ++k)
      C[i][j] += A[i][k] * B[k][j];
}
void ex_axa_views(float A[restrict 4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int e = 0; e < 2; ++e) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j)
    for (int k = 4 * e; k < 4; ++k)
      C[i][j] += A[i][k] * A[j][k];
}
void ex_axa_same(float A[restrict 4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int e = 0; e < 2; ++e) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j)
    for (int k = 4 * e; k < 4; ++k)
      C[i][j] += A[i][k] * A[k][j];
}
void ex_det2(float A[restrict 4][4], float B[restrict 4][4], float C[restrict 8][4]) {
  for (int e = 0; e < 2; ++e) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j)
    for (int k = 4 * e; k < 4; ++k)
      C[2 * i][j] += A[i][k] * B[k][j];
}
void ex_stepped(float A[restrict 4][8], float B[restrict 8][4], float C[restrict 4][4]) {
  for (int e = 0; e < 2; ++e) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j)
    for (int k = 8 * e; k < 8; k += 2)
      C[i][j] += A[i][k] * B[k][j];
}
void ex_alias_c(float A[4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int e = 0; e < 2; ++e) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j)
    for (int k = 4 * e; k < 4; ++k)
      A[i][j] += A[i][k] * B[k][j];
}
void ex_triangle(float A[restrict 4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = i; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] += A[i][k] * B[k][j];
}
void ex_rep_reads(float A[restrict 4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i) for (int r = i; r < 4; ++r)
    for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
      C[i][j] += A[i][k] * B[k][j];
}
void ex_rep_prefix(float A[restrict 4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int r1 = 0; r1 < 2; ++r1) for (int r0 = r1; r0 < 2; ++r0)
    for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = i; k < i + 4; ++k)
      C[i][j] += A[i][k - i] * B[k - i][j];
}
void ex_remainder(float A[restrict 4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j)
    for (int k1 = 0; k1 < 2; ++k1) for (int k0 = 0; k0 < 2 - k1; ++k0)
      C[i][j] += A[i][2 * k1 + k0] * B[2 * k1 + k0][j];
}
void ex_coef2(float A[restrict 4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; 2 * k - i < 4; ++k)
    C[i][j] += A[i][k] * B[k][j];
}
void ex_oob(float A[restrict 4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int e = 0; e < 2; ++e) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j)
    for (int k = 4 * e; k < 4; ++k)
      C[i][j] += A[i][k + 1] * B[k][j];
}

static float mul(float a, float b) { return a * b; }
static void ex_degenerate_ref(float A[4][4], float B[4][4], float C[4][4]) {
  for (int i = 2; i < 25; i += 2) for (int j = i; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] = C[i][j] + mul(A[i][k], B[k][j]);
}
static void ex_rep_triangle_ref(float A[4][4], float B[4][4], float C[4][4]) {
  for (int j = 0; j < 4; ++j) for (int r1 = 0; r1 <= 2; r1++) for (int r0 = r1; r0 <= 1; r0++)
    for (int k = 0; k < 4; ++k) for (int i = 0; i < 4; ++i)
      C[i][j] = C[i][j] + mul(A[i][k], B[k][j]);
}
static void ex_cond_minus_ref(float A[4][4], float B[4][4], float C[4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = i; k - i < 4; ++k)
    C[i][j] = C[i][j] + mul(A[i][k - i], B[k - i][j]);
}
static void ex_cond_flipped_ref(float A[4][4], float B[4][4], float C[4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = i; i + 4 > k; ++k)
    C[i][j] = C[i][j] + mul(A[i][k - i], B[k - i][j]);
}
static void ex_k1_empty_ref(float A[4][4], float B[4][4], float C[4][4]) {
  for (int e = 0; e < 2; ++e) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j)
    for (int k = 3 + e; k < 4; ++k)
      C[i][j] = C[i][j] + mul(A[i][k], B[k][j]);
}
static void ex_axa_views_ref(float A[4][4], float B[4][4], float C[4][4]) {
  for (int e = 0; e < 2; ++e) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j)
    for (int k = 4 * e; k < 4; ++k)
      C[i][j] = C[i][j] + mul(A[i][k], A[j][k]);
}
static void ex_axa_same_ref(float A[4][4], float B[4][4], float C[4][4]) {
  for (int e = 0; e < 2; ++e) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j)
    for (int k = 4 * e; k < 4; ++k)
      C[i][j] = C[i][j] + mul(A[i][k], A[k][j]);
}
static void ex_det2_ref(float A[4][4], float B[4][4], float C[8][4]) {
  for (int e = 0; e < 2; ++e) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j)
    for (int k = 4 * e; k < 4; ++k)
      C[2 * i][j] = C[2 * i][j] + mul(A[i][k], B[k][j]);
}
static void ex_stepped_ref(float A[4][8], float B[8][4], float C[4][4]) {
  for (int e = 0; e < 2; ++e) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j)
    for (int k = 8 * e; k < 8; k += 2)
      C[i][j] = C[i][j] + mul(A[i][k], B[k][j]);
}
static void ex_alias_c_ref(float A[4][4], float B[4][4], float C[4][4]) {
  for (int e = 0; e < 2; ++e) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j)
    for (int k = 4 * e; k < 4; ++k)
      A[i][j] = A[i][j] + mul(A[i][k], B[k][j]);
}
static void ex_triangle_ref(float A[4][4], float B[4][4], float C[4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = i; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] = C[i][j] + mul(A[i][k], B[k][j]);
}
static void ex_rep_reads_ref(float A[4][4], float B[4][4], float C[4][4]) {
  for (int i = 0; i < 4; ++i) for (int r = i; r < 4; ++r)
    for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
      C[i][j] = C[i][j] + mul(A[i][k], B[k][j]);
}
static void ex_rep_prefix_ref(float A[4][4], float B[4][4], float C[4][4]) {
  for (int r1 = 0; r1 < 2; ++r1) for (int r0 = r1; r0 < 2; ++r0)
    for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = i; k < i + 4; ++k)
      C[i][j] = C[i][j] + mul(A[i][k - i], B[k - i][j]);
}
static void ex_remainder_ref(float A[4][4], float B[4][4], float C[4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j)
    for (int k1 = 0; k1 < 2; ++k1) for (int k0 = 0; k0 < 2 - k1; ++k0)
      C[i][j] = C[i][j] + mul(A[i][2 * k1 + k0], B[2 * k1 + k0][j]);
}
static void ex_coef2_ref(float A[4][4], float B[4][4], float C[4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; 2 * k - i < 4; ++k)
    C[i][j] = C[i][j] + mul(A[i][k], B[k][j]);
}

static void fill(float *p, int n, int s) { for (int e = 0; e < n; ++e) p[e] = (float)((e * 7 + s) % 9 - 4); }
static int differ;
static unsigned long long inv(void) { systolic_offload_stats_t s; systolic_offload_stats(&s); return s.per_device[0]+s.per_device[1]+s.per_device[2]+s.per_device[3]; }
static void report(const char *n, const void *g, const void *w, size_t by, unsigned long long i) {
  int same = memcmp(g, w, by) == 0; differ += !same;
  printf("[case] %s: %s, %s (%llu)\n", n, same ? "equal" : "DIFFERENT", i ? "offloaded" : "NOT OFFLOADED", i);
}
/* The kernel gets A, B, C; the reference the same data in separate arrays. The
   compared array is the one the kernel writes: C, or A for ex_alias_c. */
#define RUN(name, NA, NB, NC, OUT) do { static float A[NA], B[NB], C[NC], RA[NA], RB[NB], RC[NC]; \
  fill(A, NA, 1); fill(B, NB, 2); fill(C, NC, 3); memcpy(RA, A, sizeof A); memcpy(RB, B, sizeof B); memcpy(RC, C, sizeof C); \
  unsigned long long before = inv(); name((void*)A,(void*)B,(void*)C); unsigned long long n = inv() - before; \
  name##_ref((void*)RA,(void*)RB,(void*)RC); \
  if (OUT == 0) report(#name, A, RA, sizeof A, n); else report(#name, C, RC, sizeof C, n); } while (0)
int main(void) {
  RUN(ex_degenerate, 16, 16, 16, 1);
  RUN(ex_rep_triangle, 16, 16, 16, 1);
  RUN(ex_cond_minus, 16, 16, 16, 1);
  RUN(ex_cond_flipped, 16, 16, 16, 1);
  RUN(ex_k1_empty, 16, 16, 16, 1);
  RUN(ex_axa_views, 16, 16, 16, 1);
  RUN(ex_axa_same, 16, 16, 16, 1);
  RUN(ex_det2, 16, 16, 32, 1);
  RUN(ex_stepped, 32, 32, 16, 1);
  RUN(ex_alias_c, 16, 16, 16, 0);
  RUN(ex_triangle, 16, 16, 16, 1);
  RUN(ex_rep_reads, 16, 16, 16, 1);
  RUN(ex_rep_prefix, 16, 16, 16, 1);
  RUN(ex_remainder, 16, 16, 16, 1);
  RUN(ex_coef2, 16, 16, 16, 1);
  printf("%s: %d differ\n", differ ? "FAIL" : "PASS", differ); return differ;
}
