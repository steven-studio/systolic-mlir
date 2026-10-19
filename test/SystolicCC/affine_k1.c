// REQUIRES: cfront-clang
//
// K = 1: a GEMM whose reduction has exactly one logical iteration (the
// requirement of 2026-10-18; SQ-4 decided 2026-10-15). C[i][j] += A[i][0] * B[0][j]
// is the outer product of A's column and B's row -- a valid GEMM with K = 1 --
// written three ways, each lowered as a 4 x 4 x 1 (or batched 1 x K) tile and
// compared bitwise with a CPU reference that runs the loops as written on
// integer-valued data with a nonzero initial C (the accumulation C += A B is
// checked, not C = A B):
//
//   k1_const      C[i][j] += A[i][0] * B[0][j]           constant reduction subscripts, no k loop
//   k1_loop       for (k = 2; k < 3) C[i][j] += A[i][k] * B[k][j]   a reduction loop of one iteration
//   k1_batched    C[b][i][j] += A[b][i][0] * B[b][0][j]  K = 1 per batch value
//   k1_rect       C[i][j] += A[i][3] * B[3][j], 8 x 5    M != N, the reduction index a constant 3
//   k1_oob        C[i][j] += A[i][4] * B[0][j], A 4 x 4  A[i][4] is out of bounds: refused (PROVEN_INVALID), stays on
//                                                        the host (compiled, not executed: the access is undefined)
//   k1_alias      A[i][j] += A[i][0] * B[0][j]           the output overlaps A: refused, host
//
// NO REAL HARDWARE HERE: the software runtime model computes every tile;
// SOFT_RUNTIME_MOCK_HARDWARE=1 makes it claim to be the FPGA.
//
// RUN: %systolic-cc --fp-reassociation --explain %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// PLAN-DAG: note: 'k1_const' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'k1_loop' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'k1_batched' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 2 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'k1_rect' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'k1_oob' stays on the host: {{.*}}A is accessed out of bounds along dimension 1)
// PLAN-DAG: note: 'k1_alias' stays on the host: {{.*}}A may overlap C
// PLAN: wrote {{.*}} (4 function(s) offloaded
//
// RUN: %t > %t.soft
// RUN: FileCheck %s --check-prefix=ALL --input-file=%t.soft
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t > %t.mock
// RUN: FileCheck %s --check-prefix=ALL --input-file=%t.mock
// ALL: [case] k1_const: equal, offloaded (1)
// ALL: [case] k1_loop: equal, offloaded (1)
// ALL: [case] k1_batched: equal, offloaded (2)
// ALL: [case] k1_rect: equal, offloaded (1)
// ALL: [case] k1_alias: equal, NOT OFFLOADED (0)
// ALL: PASS: 0 differ
//
// Every offloaded call also against the function as written, on the CPU:
// RUN: %systolic-cc --fp-reassociation --verify %s -o %t.v > /dev/null 2>&1
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t.v 2>&1 >/dev/null | FileCheck %s --check-prefix=VERIFY
// VERIFY-COUNT-4: verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY-NOT: MISMATCH

#include <stdio.h>
#include <string.h>

#include "systolic_offload.h"

void k1_const(float A[restrict 4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j)
    C[i][j] += A[i][0] * B[0][j];
}
void k1_loop(float A[restrict 4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 2; k < 3; ++k)
    C[i][j] += A[i][k] * B[k][j];
}
void k1_batched(float A[restrict 2][4][4], float B[restrict 2][4][4], float C[restrict 2][4][4]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j)
    C[b][i][j] += A[b][i][0] * B[b][0][j];
}
void k1_rect(float A[restrict 8][4], float B[restrict 4][5], float C[restrict 8][5]) {
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 5; ++j)
    C[i][j] += A[i][3] * B[3][j];
}
void k1_oob(float A[restrict 4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j)
    C[i][j] += A[i][4] * B[0][j];
}
void k1_alias(float A[4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j)
    A[i][j] += A[i][0] * B[0][j];
}

static float mul(float a, float b) { return a * b; }
static void k1_const_ref(float A[4][4], float B[4][4], float C[4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j)
    C[i][j] = C[i][j] + mul(A[i][0], B[0][j]);
}
static void k1_loop_ref(float A[4][4], float B[4][4], float C[4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 2; k < 3; ++k)
    C[i][j] = C[i][j] + mul(A[i][k], B[k][j]);
}
static void k1_batched_ref(float A[2][4][4], float B[2][4][4], float C[2][4][4]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j)
    C[b][i][j] = C[b][i][j] + mul(A[b][i][0], B[b][0][j]);
}
static void k1_rect_ref(float A[8][4], float B[4][5], float C[8][5]) {
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 5; ++j)
    C[i][j] = C[i][j] + mul(A[i][3], B[3][j]);
}
static void k1_alias_ref(float A[4][4], float B[4][4], float C[4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j)
    A[i][j] = A[i][j] + mul(A[i][0], B[0][j]);
}

static void fill(float *p, int n, int s) { for (int e = 0; e < n; ++e) p[e] = (float)((e * 7 + s) % 9 - 4); }
static int differ;
static unsigned long long inv(void) { systolic_offload_stats_t s; systolic_offload_stats(&s); return s.per_device[0]+s.per_device[1]+s.per_device[2]+s.per_device[3]; }
static void report(const char *n, const void *g, const void *w, size_t by, unsigned long long i) {
  int same = memcmp(g, w, by) == 0; differ += !same;
  printf("[case] %s: %s, %s (%llu)\n", n, same ? "equal" : "DIFFERENT", i ? "offloaded" : "NOT OFFLOADED", i);
}
#define RUN(name, NA, NB, NC, OUT) do { static float A[NA], B[NB], C[NC], RA[NA], RB[NB], RC[NC]; \
  fill(A, NA, 1); fill(B, NB, 2); fill(C, NC, 3); memcpy(RA, A, sizeof A); memcpy(RB, B, sizeof B); memcpy(RC, C, sizeof C); \
  unsigned long long before = inv(); name((void*)A,(void*)B,(void*)C); unsigned long long n = inv() - before; \
  name##_ref((void*)RA,(void*)RB,(void*)RC); \
  if (OUT == 0) report(#name, A, RA, sizeof A, n); else report(#name, C, RC, sizeof C, n); } while (0)
int main(void) {
  RUN(k1_const, 4*4, 4*4, 4*4, 1);
  RUN(k1_loop, 4*4, 4*4, 4*4, 1);
  RUN(k1_batched, 2*4*4, 2*4*4, 2*4*4, 1);
  RUN(k1_rect, 8*4, 4*5, 8*5, 1);
  RUN(k1_alias, 4*4, 4*4, 4*4, 0);
  printf("%s: %d differ\n", differ ? "FAIL" : "PASS", differ); return differ;
}
