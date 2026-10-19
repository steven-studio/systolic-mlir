// REQUIRES: cfront-clang
//
// One array as both factors of the product, end to end (docs/AFFINE_GEMM_MAPPING.md
// section 7, 19.12, 2026-10-18). When the two factors read the same elements in the
// same arrangement one read-only tensor serves both; otherwise each factor is its
// own read-only view of the shared base and the lowering stages every tile from its
// own view (C is written at its own view). Each kernel is compared
// with a reference that runs the same loops on the CPU (products through a call, so
// the reference is not offloaded), bitwise, on integer-valued data:
//
//   sf_same          C[i][j] += A[i][k] A[k][j]           one view, one tensor (A . A)
//   sf_gram_right    C[i][j] += A[i][k] A[j][k]           A . A^T: the whole memref and its transpose
//   sf_gram_left     C[i][j] += A[k][i] A[k][j]           A^T . A
//   sf_skew_one      C[i][j] += A[i][i+k] A[k][j]         a skew read by the first factor only
//   sf_stride2       C[i][j] += A[i][2k] A[2k][j]         a stride-2 view against its transpose
//   sf_offset        C[i][j] += A[i][k] A[k][j+1]         showcase ext_007: an offset view (8 x 8 x 8)
//   sf_rev_rows      C[i][j] += A[3-i][k] A[k][j]         the rows reversed against the whole memref
//   sf_c_is_a        A[i][j] += A[i][k] B[k][j]           the output is the shared base: refused, host
//
// NO REAL HARDWARE HERE: the software runtime model computes every tile;
// SOFT_RUNTIME_MOCK_HARDWARE=1 makes it claim to be the FPGA.
//
// RUN: %systolic-cc --fp-reassociation --explain %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// PLAN-DAG: note: 'sf_same' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'sf_gram_right' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'sf_gram_left' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'sf_skew_one' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'sf_stride2' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'sf_offset' is offloaded: {{[0-9]+}} systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'sf_rev_rows' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'sf_c_is_a' stays on the host
// PLAN: wrote {{.*}} (7 function(s) offloaded
//
// RUN: %t > %t.soft
// RUN: FileCheck %s --check-prefix=ALL --input-file=%t.soft
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t > %t.mock
// RUN: FileCheck %s --check-prefix=ALL --input-file=%t.mock
// ALL: [case] sf_same: equal, offloaded (1)
// ALL: [case] sf_gram_right: equal, offloaded (1)
// ALL: [case] sf_gram_left: equal, offloaded (1)
// ALL: [case] sf_skew_one: equal, offloaded (1)
// ALL: [case] sf_stride2: equal, offloaded (1)
// ALL: [case] sf_offset: equal, offloaded ({{[0-9]+}})
// ALL: [case] sf_rev_rows: equal, offloaded (1)
// ALL: [case] sf_c_is_a: equal, NOT OFFLOADED (0)
// ALL: PASS: 0 differ
//
// Every offloaded call also against the function as written, on the CPU:
// RUN: %systolic-cc --fp-reassociation --verify %s -o %t.v > /dev/null 2>&1
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t.v 2>&1 >/dev/null | FileCheck %s --check-prefix=VERIFY
// VERIFY-COUNT-7: verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY-NOT: MISMATCH

#include <stdio.h>
#include <string.h>

#include "systolic_offload.h"

void sf_same(float A[restrict 4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] += A[i][k] * A[k][j];
}
void sf_gram_right(float A[restrict 4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] += A[i][k] * A[j][k];
}
void sf_gram_left(float A[restrict 4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] += A[k][i] * A[k][j];
}
void sf_skew_one(float A[restrict 4][7], float B[restrict 4][7], float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] += A[i][i + k] * A[k][j];
}
void sf_stride2(float A[restrict 8][8], float B[restrict 8][8], float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] += A[i][2 * k] * A[2 * k][j];
}
void sf_offset(float A[restrict 8][9], float B[restrict 8][9], float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) for (int k = 0; k < 8; ++k)
    C[i][j] += A[i][k] * A[k][j + 1];
}
void sf_rev_rows(float A[restrict 4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] += A[3 - i][k] * A[k][j];
}
void sf_c_is_a(float A[4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    A[i][j] += A[i][k] * B[k][j];
}

static float mul(float a, float b) { return a * b; }
static void sf_same_ref(float A[4][4], float B[4][4], float C[4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] = C[i][j] + mul(A[i][k], A[k][j]);
}
static void sf_gram_right_ref(float A[4][4], float B[4][4], float C[4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] = C[i][j] + mul(A[i][k], A[j][k]);
}
static void sf_gram_left_ref(float A[4][4], float B[4][4], float C[4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] = C[i][j] + mul(A[k][i], A[k][j]);
}
static void sf_skew_one_ref(float A[4][7], float B[4][7], float C[4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] = C[i][j] + mul(A[i][i + k], A[k][j]);
}
static void sf_stride2_ref(float A[8][8], float B[8][8], float C[4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] = C[i][j] + mul(A[i][2 * k], A[2 * k][j]);
}
static void sf_offset_ref(float A[8][9], float B[8][9], float C[8][8]) {
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) for (int k = 0; k < 8; ++k)
    C[i][j] = C[i][j] + mul(A[i][k], A[k][j + 1]);
}
static void sf_rev_rows_ref(float A[4][4], float B[4][4], float C[4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] = C[i][j] + mul(A[3 - i][k], A[k][j]);
}
static void sf_c_is_a_ref(float A[4][4], float B[4][4], float C[4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    A[i][j] = A[i][j] + mul(A[i][k], B[k][j]);
}

static void fill(float *p, int n, int s) { for (int e = 0; e < n; ++e) p[e] = (float)((e * 7 + s) % 9 - 4); }
static int differ;
static unsigned long long inv(void) { systolic_offload_stats_t s; systolic_offload_stats(&s); return s.per_device[0]+s.per_device[1]+s.per_device[2]+s.per_device[3]; }
static void report(const char *n, const void *g, const void *w, size_t by, unsigned long long i) {
  int same = memcmp(g, w, by) == 0; differ += !same;
  printf("[case] %s: %s, %s (%llu)\n", n, same ? "equal" : "DIFFERENT", i ? "offloaded" : "NOT OFFLOADED", i);
}
/* The kernel gets A, B, C; the reference the same data in separate arrays. The
   compared array is the one the kernel writes: C, or A for sf_c_is_a. */
#define RUN(name, NA, NB, NC, OUT) do { static float A[NA], B[NB], C[NC], RA[NA], RB[NB], RC[NC]; \
  fill(A, NA, 1); fill(B, NB, 2); fill(C, NC, 3); memcpy(RA, A, sizeof A); memcpy(RB, B, sizeof B); memcpy(RC, C, sizeof C); \
  unsigned long long before = inv(); name((void*)A,(void*)B,(void*)C); unsigned long long n = inv() - before; \
  name##_ref((void*)RA,(void*)RB,(void*)RC); \
  if (OUT == 0) report(#name, A, RA, sizeof A, n); else report(#name, C, RC, sizeof C, n); } while (0)
int main(void) {
  RUN(sf_same, 4*4, 4*4, 4*4, 1);
  RUN(sf_gram_right, 4*4, 4*4, 4*4, 1);
  RUN(sf_gram_left, 4*4, 4*4, 4*4, 1);
  RUN(sf_skew_one, 4*7, 4*7, 4*4, 1);
  RUN(sf_stride2, 8*8, 8*8, 4*4, 1);
  RUN(sf_offset, 8*9, 8*9, 8*8, 1);
  RUN(sf_rev_rows, 4*4, 4*4, 4*4, 1);
  RUN(sf_c_is_a, 4*4, 4*4, 4*4, 0);
  printf("%s: %d differ\n", differ ? "FAIL" : "PASS", differ); return differ;
}
