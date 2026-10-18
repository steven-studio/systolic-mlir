// REQUIRES: cfront-clang
//
// Loop nests whose iteration domain is a constant-extent parallelotope, not a box
// (docs/AFFINE_GEMM_MAPPING.md section 19.12, 2026-10-18), end to end: the C front
// end translates loop bounds that are affine in the enclosing counters when the
// bounds shift together (for (int k = i; k < i + 4; ++k) becomes affine.for with
// affine_map bounds), the compiler maps the domain onto a box (x = U x') and lowers
// the GEMM on the box. Each kernel is compared with a reference that runs the same
// loops on the CPU (products through a call, so the reference is not offloaded),
// bitwise, on integer-valued data:
//
//   dn_shift_k   k in [i, i+4):   C[i][j] += A[i][k-i] B[k-i][j]       the reduction shifted by i
//   dn_shift_j   j in [i, i+4):   C[i][j-i] += A[i][k] B[k][j-i]       an output coordinate shifted
//   dn_skew_c    j in [i, i+4):   C[i][j] += A[i][k] B[k][j-i]         the shift read by C: a skewed C view (4 x 7)
//   dn_chain     j in [i, i+4), k in [j, j+2):
//                                 C[i][j-i][k-j] += A[i][k-j] B[j-i][k-j]   two chained shifts; batched, K = 1 per batch
//   dn_down      i counts down, k in [2i+1, 2i+4]:
//                                 C[i][j] += A[i][k-2i-1] B[k-2i-1][j]  a shift by a counting-down counter, <=
//   dn_step2     k in [2i, 2i+8) step 2:
//                                 C[i][j] += A[i][k-2i] B[k-2i][j]      a shifted loop of step 2: stride-2 views
//   dn_tri       k in [i, 8):     a triangle: the trip count varies, outside the front end's subset, plain C
//   dn_oob       k in [i, i+4) with A[i][k] on a 4 x 4 A: column k reaches 6, PROVEN_INVALID,
//                                 stays on the host (compiled, not run: the reads are undefined in C)
//
// NO REAL HARDWARE HERE: the software runtime model computes every tile;
// SOFT_RUNTIME_MOCK_HARDWARE=1 makes it claim to be the FPGA.
//
// RUN: %systolic-cc --fp-reassociation --explain %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// PLAN-DAG: note: 'dn_shift_k' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'dn_shift_j' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'dn_skew_c' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'dn_chain' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 2 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'dn_down' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'dn_step2' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'dn_tri' is outside the supported C subset and is not translated: the loop's trip count depends on an enclosing loop counter
// PLAN-DAG: note: 'dn_oob' stays on the host: {{.*}}A is accessed out of bounds along dimension 1
// PLAN: wrote {{.*}} (6 function(s) offloaded
//
// RUN: %t > %t.soft
// RUN: FileCheck %s --check-prefix=ALL --input-file=%t.soft
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t > %t.mock
// RUN: FileCheck %s --check-prefix=ALL --input-file=%t.mock
// ALL: [case] dn_shift_k: equal, offloaded (1)
// ALL: [case] dn_shift_j: equal, offloaded (1)
// ALL: [case] dn_skew_c: equal, offloaded (1)
// ALL: [case] dn_chain: equal, offloaded (2)
// ALL: [case] dn_down: equal, offloaded (1)
// ALL: [case] dn_step2: equal, offloaded (1)
// ALL: [case] dn_tri: equal, NOT OFFLOADED (0)
// ALL: PASS: 0 differ
//
// Every offloaded call also against the function as written, on the CPU:
// RUN: %systolic-cc --fp-reassociation --verify %s -o %t.v > /dev/null 2>&1
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t.v 2>&1 >/dev/null | FileCheck %s --check-prefix=VERIFY
// VERIFY-COUNT-6: verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY-NOT: MISMATCH

#include <stdio.h>
#include <string.h>

#include "systolic_offload.h"

void dn_shift_k(float A[restrict 4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = i; k < i + 4; ++k)
    C[i][j] += A[i][k - i] * B[k - i][j];
}
void dn_shift_j(float A[restrict 4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = i; j < i + 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j - i] += A[i][k] * B[k][j - i];
}
void dn_skew_c(float A[restrict 4][4], float B[restrict 4][4], float C[restrict 4][7]) {
  for (int i = 0; i < 4; ++i) for (int j = i; j < i + 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] += A[i][k] * B[k][j - i];
}
void dn_chain(float A[restrict 4][2], float B[restrict 4][2], float C[restrict 4][4][2]) {
  for (int i = 0; i < 4; ++i) for (int j = i; j < i + 4; ++j) for (int k = j; k < j + 2; ++k)
    C[i][j - i][k - j] += A[i][k - j] * B[j - i][k - j];
}
void dn_down(float A[restrict 4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int i = 3; i >= 0; --i) for (int j = 0; j < 4; ++j) for (int k = 2 * i + 1; k <= 2 * i + 4; ++k)
    C[i][j] += A[i][k - 2 * i - 1] * B[k - 2 * i - 1][j];
}
void dn_step2(float A[restrict 4][8], float B[restrict 8][4], float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 2 * i; k < 2 * i + 8; k += 2)
    C[i][j] += A[i][k - 2 * i] * B[k - 2 * i][j];
}
void dn_tri(float A[restrict 4][8], float B[restrict 8][4], float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = i; k < 8; ++k)
    C[i][j] += A[i][k] * B[k][j];
}
void dn_oob(float A[restrict 4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = i; k < i + 4; ++k)
    C[i][j] += A[i][k] * B[k][j];
}

static float mul(float a, float b) { return a * b; }
static void dn_shift_k_ref(float A[4][4], float B[4][4], float C[4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = i; k < i + 4; ++k)
    C[i][j] = C[i][j] + mul(A[i][k - i], B[k - i][j]);
}
static void dn_shift_j_ref(float A[4][4], float B[4][4], float C[4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = i; j < i + 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j - i] = C[i][j - i] + mul(A[i][k], B[k][j - i]);
}
static void dn_skew_c_ref(float A[4][4], float B[4][4], float C[4][7]) {
  for (int i = 0; i < 4; ++i) for (int j = i; j < i + 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] = C[i][j] + mul(A[i][k], B[k][j - i]);
}
static void dn_chain_ref(float A[4][2], float B[4][2], float C[4][4][2]) {
  for (int i = 0; i < 4; ++i) for (int j = i; j < i + 4; ++j) for (int k = j; k < j + 2; ++k)
    C[i][j - i][k - j] = C[i][j - i][k - j] + mul(A[i][k - j], B[j - i][k - j]);
}
static void dn_down_ref(float A[4][4], float B[4][4], float C[4][4]) {
  for (int i = 3; i >= 0; --i) for (int j = 0; j < 4; ++j) for (int k = 2 * i + 1; k <= 2 * i + 4; ++k)
    C[i][j] = C[i][j] + mul(A[i][k - 2 * i - 1], B[k - 2 * i - 1][j]);
}
static void dn_step2_ref(float A[4][8], float B[8][4], float C[4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 2 * i; k < 2 * i + 8; k += 2)
    C[i][j] = C[i][j] + mul(A[i][k - 2 * i], B[k - 2 * i][j]);
}
static void dn_tri_ref(float A[4][8], float B[8][4], float C[4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = i; k < 8; ++k)
    C[i][j] = C[i][j] + mul(A[i][k], B[k][j]);
}

static void fill(float *p, int n, int s) { for (int e = 0; e < n; ++e) p[e] = (float)((e * 7 + s) % 9 - 4); }
static int differ;
static unsigned long long inv(void) { systolic_offload_stats_t s; systolic_offload_stats(&s); return s.per_device[0]+s.per_device[1]+s.per_device[2]+s.per_device[3]; }
static void report(const char *n, const void *g, const void *w, size_t by, unsigned long long i) {
  int same = memcmp(g, w, by) == 0; differ += !same;
  printf("[case] %s: %s, %s (%llu)\n", n, same ? "equal" : "DIFFERENT", i ? "offloaded" : "NOT OFFLOADED", i);
}
/* The kernel gets A, B, C; the reference the same data in separate arrays; C is compared. */
#define RUN(name, NA, NB, NC) do { static float A[NA], B[NB], C[NC], RA[NA], RB[NB], RC[NC]; \
  fill(A, NA, 1); fill(B, NB, 2); fill(C, NC, 3); memcpy(RA, A, sizeof A); memcpy(RB, B, sizeof B); memcpy(RC, C, sizeof C); \
  unsigned long long before = inv(); name((void*)A,(void*)B,(void*)C); unsigned long long n = inv() - before; \
  name##_ref((void*)RA,(void*)RB,(void*)RC); report(#name, C, RC, sizeof C, n); } while (0)
int main(void) {
  RUN(dn_shift_k, 4*4, 4*4, 4*4);
  RUN(dn_shift_j, 4*4, 4*4, 4*4);
  RUN(dn_skew_c, 4*4, 4*4, 4*7);
  RUN(dn_chain, 4*2, 4*2, 4*4*2);
  RUN(dn_down, 4*4, 4*4, 4*4);
  RUN(dn_step2, 4*8, 8*4, 4*4);
  RUN(dn_tri, 4*8, 8*4, 4*4);
  printf("%s: %d differ\n", differ ? "FAIL" : "PASS", differ); return differ;
}
