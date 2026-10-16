// REQUIRES: cfront-clang
//
// Batch-dependent operand views end to end (ISSUE-007,
// docs/AFFINE_GEMM_MAPPING.md section 16): a loop read by all three operands
// is a batch loop, the factors may read it as an affine offset of a role's
// dimension or through their own expression, C reads it alone. Each kernel is
// compared with a reference that runs the same loops on the CPU (products
// through a call, so it is not offloaded itself):
//
//   bv_target      C[8i][j] += A[4i - j][5k] B[5k][j], i in [1, 5)   the Phase 1 target (A's row moves with j)
//   bv_tiles       the same, i in [1, 17), k in [0, 16)              several tiles per batch value
//   bv_b_offset    C[b] += A[b] B[k][j + b]                          B's columns move with b
//   bv_per_operand A[b + 1], B[b], C[b]                              each operand its own batch index
//   bv_two         A[p][i + q][k], B[k + p][j + q], C[p][q]          two batch coordinates, offsets in both factors
//   bv_c_shift     C[b + 1] += A[b] B[b]                             C's batch index shifted
//   bv_neg         C[b][3 - i][j] += A[2b + 3 - i][k] B[k][j + b]    a backwards view with a batch offset
//   bv_j_in_k      C[i][j] += A[i][k + j] B[k + j][j]                j read by all three: a batch loop, N = 1 per j
//   bv_i_in_k      C[i][j] += A[i][k + i] B[k + i][j]                i a batch loop, M = 1 per i
//   bv_const_prefix C[t][i][0] += A[t][i][1] B[t][1][0], t in [1, 2) t runs once: a fixed index, a plain 5 x 1 x 1 GEMM
//
// The last three were refused before the review fixes: the positional reading
// K = k + j, J = j had loop j drive two coordinates ("two logical coordinates
// are driven by the same loop"), and a prefix constant on the domain counted
// as a batch coordinate (a batched vector-times-scalar, SQ-5).
//
// NO REAL HARDWARE HERE: the software runtime model computes every tile;
// SOFT_RUNTIME_MOCK_HARDWARE=1 makes it claim to be the FPGA.
//
// RUN: %systolic-cc --fp-reassociation --explain %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// PLAN-DAG: note: 'bv_target' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 4 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'bv_tiles' is offloaded: {{[0-9]+}} systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; {{[0-9]+}} invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'bv_b_offset' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 2 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'bv_per_operand' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 2 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'bv_two' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 6 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'bv_c_shift' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 2 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'bv_neg' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 2 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'bv_j_in_k' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 4 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'bv_i_in_k' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 4 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'bv_const_prefix' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN: wrote {{.*}} (10 function(s) offloaded
//
// RUN: %t > %t.soft
// RUN: FileCheck %s --check-prefix=ALL --input-file=%t.soft
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t > %t.mock
// RUN: FileCheck %s --check-prefix=ALL --input-file=%t.mock
// ALL: [case] bv_target: equal, offloaded (4)
// ALL: [case] bv_tiles: equal, offloaded ({{[0-9]+}})
// ALL: [case] bv_b_offset: equal, offloaded (2)
// ALL: [case] bv_per_operand: equal, offloaded (2)
// ALL: [case] bv_two: equal, offloaded (6)
// ALL: [case] bv_c_shift: equal, offloaded (2)
// ALL: [case] bv_neg: equal, offloaded (2)
// ALL: [case] bv_j_in_k: equal, offloaded (4)
// ALL: [case] bv_i_in_k: equal, offloaded (4)
// ALL: [case] bv_const_prefix: equal, offloaded (2)
// ALL: PASS: 0 differ
//
// Every offloaded call also against the function as written, on the CPU:
// RUN: %systolic-cc --fp-reassociation --verify %s -o %t.v > /dev/null 2>&1
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t.v 2>&1 >/dev/null | FileCheck %s --check-prefix=VERIFY
// VERIFY-COUNT-10: verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY-NOT: MISMATCH
//
// Without the reassociation permission (the default) nothing is offloaded:
// RUN: %systolic-cc --explain %s -o %t.strict 2>&1 | FileCheck %s --check-prefix=STRICT
// STRICT: wrote {{.*}} (0 function(s) offloaded

#include <stdio.h>
#include <string.h>
#include "systolic_offload.h"

void bv_target(float A[restrict 17][16], float B[restrict 16][4], float C[restrict 33][4]) {
  for (int i = 1; i < 5; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[8 * i][j] += A[4 * i - j][5 * k] * B[5 * k][j];
}
void bv_tiles(float A[restrict 65][76], float B[restrict 76][4], float C[restrict 129][4]) {
  for (int i = 1; i < 17; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 16; ++k)
    C[8 * i][j] += A[4 * i - j][5 * k] * B[5 * k][j];
}
void bv_b_offset(float A[restrict 2][4][4], float B[restrict 4][5], float C[restrict 2][4][4]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[b][i][j] += A[b][i][k] * B[k][j + b];
}
void bv_per_operand(float A[restrict 3][4][4], float B[restrict 2][4][4], float C[restrict 2][4][4]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[b][i][j] += A[b + 1][i][k] * B[b][k][j];
}
void bv_two(float A[restrict 2][6][4], float B[restrict 5][6], float C[restrict 2][3][4][4]) {
  for (int p = 0; p < 2; ++p) for (int q = 0; q < 3; ++q)
    for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
      C[p][q][i][j] += A[p][i + q][k] * B[k + p][j + q];
}
void bv_c_shift(float A[restrict 2][4][4], float B[restrict 2][4][4], float C[restrict 3][4][4]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[b + 1][i][j] += A[b][i][k] * B[b][k][j];
}
void bv_neg(float A[restrict 6][4], float B[restrict 4][5], float C[restrict 2][4][4]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[b][3 - i][j] += A[2 * b + 3 - i][k] * B[k][j + b];
}
void bv_j_in_k(float A[restrict 4][8], float B[restrict 8][4], float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] += A[i][k + j] * B[k + j][j];
}
void bv_i_in_k(float A[restrict 4][8], float B[restrict 8][4], float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] += A[i][k + i] * B[k + i][j];
}
void bv_const_prefix(float A[restrict 2][5][3], float B[restrict 2][3][2], float C[restrict 2][5][2]) {
  for (int t = 1; t < 2; ++t) for (int i = 0; i < 5; ++i)
    C[t][i][0] += A[t][i][1] * B[t][1][0];
}

static float mul(float a, float b) { return a * b; }
static void bv_target_ref(float A[17][16], float B[16][4], float C[33][4]) {
  for (int i = 1; i < 5; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[8 * i][j] = C[8 * i][j] + mul(A[4 * i - j][5 * k], B[5 * k][j]);
}
static void bv_tiles_ref(float A[65][76], float B[76][4], float C[129][4]) {
  for (int i = 1; i < 17; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 16; ++k)
    C[8 * i][j] = C[8 * i][j] + mul(A[4 * i - j][5 * k], B[5 * k][j]);
}
static void bv_b_offset_ref(float A[2][4][4], float B[4][5], float C[2][4][4]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[b][i][j] = C[b][i][j] + mul(A[b][i][k], B[k][j + b]);
}
static void bv_per_operand_ref(float A[3][4][4], float B[2][4][4], float C[2][4][4]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[b][i][j] = C[b][i][j] + mul(A[b + 1][i][k], B[b][k][j]);
}
static void bv_two_ref(float A[2][6][4], float B[5][6], float C[2][3][4][4]) {
  for (int p = 0; p < 2; ++p) for (int q = 0; q < 3; ++q)
    for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
      C[p][q][i][j] = C[p][q][i][j] + mul(A[p][i + q][k], B[k + p][j + q]);
}
static void bv_c_shift_ref(float A[2][4][4], float B[2][4][4], float C[3][4][4]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[b + 1][i][j] = C[b + 1][i][j] + mul(A[b][i][k], B[b][k][j]);
}
static void bv_neg_ref(float A[6][4], float B[4][5], float C[2][4][4]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[b][3 - i][j] = C[b][3 - i][j] + mul(A[2 * b + 3 - i][k], B[k][j + b]);
}
static void bv_j_in_k_ref(float A[4][8], float B[8][4], float C[4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] = C[i][j] + mul(A[i][k + j], B[k + j][j]);
}
static void bv_i_in_k_ref(float A[4][8], float B[8][4], float C[4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] = C[i][j] + mul(A[i][k + i], B[k + i][j]);
}
static void bv_const_prefix_ref(float A[2][5][3], float B[2][3][2], float C[2][5][2]) {
  for (int t = 1; t < 2; ++t) for (int i = 0; i < 5; ++i)
    C[t][i][0] = C[t][i][0] + mul(A[t][i][1], B[t][1][0]);
}

static void fill(float *p, int n, int s) { for (int e = 0; e < n; ++e) p[e] = (float)((e * 7 + s) % 9 - 4); }
static int differ;
static unsigned long long inv(void) { systolic_offload_stats_t s; systolic_offload_stats(&s); return s.per_device[0]+s.per_device[1]+s.per_device[2]+s.per_device[3]; }
static void report(const char *n, const void *g, const void *w, size_t by, unsigned long long i) {
  int same = memcmp(g, w, by) == 0; differ += !same;
  printf("[case] %s: %s, %s (%llu)\n", n, same ? "equal" : "DIFFERENT", i ? "offloaded" : "NOT OFFLOADED", i);
}
#define RUN(name, NA, NB, NC) do { static float A[NA], B[NB], C[NC], R[NC]; \
  fill(A, NA, 1); fill(B, NB, 2); fill(C, NC, 3); memcpy(R, C, sizeof C); \
  unsigned long long before = inv(); name((void*)A,(void*)B,(void*)C); unsigned long long n = inv() - before; \
  name##_ref((void*)A,(void*)B,(void*)R); report(#name, C, R, sizeof C, n); } while (0)
int main(void) {
  RUN(bv_target, 17*16, 16*4, 33*4);
  RUN(bv_tiles, 65*76, 76*4, 129*4);
  RUN(bv_b_offset, 2*4*4, 4*5, 2*4*4);
  RUN(bv_per_operand, 3*4*4, 2*4*4, 2*4*4);
  RUN(bv_two, 2*6*4, 5*6, 2*3*4*4);
  RUN(bv_c_shift, 2*4*4, 2*4*4, 3*4*4);
  RUN(bv_neg, 6*4, 4*5, 2*4*4);
  RUN(bv_j_in_k, 4*8, 8*4, 4*4);
  RUN(bv_i_in_k, 4*8, 8*4, 4*4);
  RUN(bv_const_prefix, 2*5*3, 2*3*2, 2*5*2);
  printf("%s: %d differ\n", differ ? "FAIL" : "PASS", differ); return differ;
}
