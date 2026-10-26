void __systolic_cc_gemm_executed(int);
#line 1 "/home/steven-studio/work/systolic-mlir/test/SystolicCC/affine_batched.c"
// REQUIRES: cfront-clang
//
// Batched GEMMs end to end (ISSUE-006, docs/AFFINE_GEMM_MAPPING.md section 2):
// the batch loop(s) are re-emitted around the systolic tiles, one GEMM per
// batch iteration on its own slice of each operand. Eight kernels, each
// compared with a reference that runs the same loops on the CPU (products
// through a call, so it is not offloaded itself):
//
//   bat          C[b] += A[b] B[b], b in [0, 2)                 the canonical batched GEMM
//   bat_mid      the batch loop in the middle of the nest        (i outside b)
//   bat_step     b = 1, 3, 5 of 7 slices                         a strided batch loop
//   bat_two      two batch coordinates p, q                       nested batch loops
//   bat_inner    A[i][p][k]: the batch dimension inside A        a dynamic offset in the middle
//   bat_tr_neg   A[b][k][i], C[b][i][7 - j]                      transposed and backwards within a batch
//   bat_down     for (b = 3; b >= 0; --b)                        a batch loop that counts down
//   bat_offset   C[b + 2] += A[b + 2] B[b + 2]                   an offset batch index
//   bat_tiles    C[b] += A[b] B[b], 16x16x16                      10 tiles per batch (slices of the batch's view)
//
// NO REAL HARDWARE HERE: the software runtime model computes every tile;
// SOFT_RUNTIME_MOCK_HARDWARE=1 makes it claim to be the FPGA.
//
// RUN: %systolic-cc --fp-reassociation --explain %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// PLAN-DAG: note: 'bat' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 2 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'bat_mid' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 3 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'bat_step' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 3 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'bat_two' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 6 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'bat_inner' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 2 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'bat_tr_neg' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 2 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'bat_down' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 4 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'bat_offset' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 3 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'bat_tiles' is offloaded: 10 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 20 invocation(s) per call (the loops around them repeat them)
// PLAN: wrote {{.*}} (9 function(s) offloaded
//
// RUN: %t > %t.soft
// RUN: FileCheck %s --check-prefix=ALL --input-file=%t.soft
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t > %t.mock
// RUN: FileCheck %s --check-prefix=ALL --input-file=%t.mock
// ALL: [case] bat: equal, offloaded (2)
// ALL: [case] bat_mid: equal, offloaded (3)
// ALL: [case] bat_step: equal, offloaded (3)
// ALL: [case] bat_two: equal, offloaded (6)
// ALL: [case] bat_inner: equal, offloaded (2)
// ALL: [case] bat_tr_neg: equal, offloaded (2)
// ALL: [case] bat_down: equal, offloaded (4)
// ALL: [case] bat_offset: equal, offloaded (3)
// ALL: [case] bat_tiles: equal, offloaded (20)
// ALL: PASS: 0 differ
//
// Every offloaded call also against the function as written, on the CPU:
// RUN: %systolic-cc --fp-reassociation --verify %s -o %t.v > /dev/null 2>&1
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t.v 2>&1 >/dev/null | FileCheck %s --check-prefix=VERIFY
// VERIFY-COUNT-9: verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY-NOT: MISMATCH
//
// Without the reassociation permission (the default) nothing is offloaded:
// RUN: %systolic-cc --explain %s -o %t.strict 2>&1 | FileCheck %s --check-prefix=STRICT
// STRICT: wrote {{.*}} (0 function(s) offloaded

#include <stdio.h>
#include <string.h>
#include "systolic_offload.h"


void bat(float A[restrict 2][4][4], float B[restrict 2][4][4], float C[restrict 2][4][4]);
#line 63 "/home/steven-studio/work/systolic-mlir/test/SystolicCC/affine_batched.c"
void bat__systolic_reference(float A[restrict 2][4][4], float B[restrict 2][4][4], float C[restrict 2][4][4]) {
  { for (int b = 0; b < 2; ++b) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[b][i][j] += A[b][i][k] * B[b][k][j]; __systolic_cc_gemm_executed(0); }
}

void bat_mid(float A[restrict 3][8][8], float B[restrict 3][8][8], float C[restrict 3][8][8]);
#line 67 "/home/steven-studio/work/systolic-mlir/test/SystolicCC/affine_batched.c"
void bat_mid__systolic_reference(float A[restrict 3][8][8], float B[restrict 3][8][8], float C[restrict 3][8][8]) {
  { for (int i = 0; i < 8; ++i) for (int b = 0; b < 3; ++b) for (int j = 0; j < 8; ++j) for (int k = 0; k < 8; ++k)
    C[b][i][j] += A[b][i][k] * B[b][k][j]; __systolic_cc_gemm_executed(0); }
}

void bat_step(float A[restrict 7][4][4], float B[restrict 7][4][4], float C[restrict 7][4][4]);
#line 71 "/home/steven-studio/work/systolic-mlir/test/SystolicCC/affine_batched.c"
void bat_step__systolic_reference(float A[restrict 7][4][4], float B[restrict 7][4][4], float C[restrict 7][4][4]) {
  { for (int b = 1; b < 7; b += 2) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[b][i][j] += A[b][i][k] * B[b][k][j]; __systolic_cc_gemm_executed(0); }
}

void bat_two(float A[restrict 2][3][4][4], float B[restrict 2][3][4][4], float C[restrict 2][3][4][4]);
#line 75 "/home/steven-studio/work/systolic-mlir/test/SystolicCC/affine_batched.c"
void bat_two__systolic_reference(float A[restrict 2][3][4][4], float B[restrict 2][3][4][4], float C[restrict 2][3][4][4]) {
  { for (int p = 0; p < 2; ++p) for (int q = 0; q < 3; ++q) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[p][q][i][j] += A[p][q][i][k] * B[p][q][k][j]; __systolic_cc_gemm_executed(0); }
}

void bat_inner(float A[restrict 4][2][4], float B[restrict 2][4][4], float C[restrict 2][4][4]);
#line 79 "/home/steven-studio/work/systolic-mlir/test/SystolicCC/affine_batched.c"
void bat_inner__systolic_reference(float A[restrict 4][2][4], float B[restrict 2][4][4], float C[restrict 2][4][4]) {
  { for (int p = 0; p < 2; ++p) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[p][i][j] += A[i][p][k] * B[p][k][j]; __systolic_cc_gemm_executed(0); }
}

void bat_tr_neg(float A[restrict 2][8][8], float B[restrict 2][8][8], float C[restrict 2][8][8]);
#line 83 "/home/steven-studio/work/systolic-mlir/test/SystolicCC/affine_batched.c"
void bat_tr_neg__systolic_reference(float A[restrict 2][8][8], float B[restrict 2][8][8], float C[restrict 2][8][8]) {
  { for (int b = 0; b < 2; ++b) for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) for (int k = 0; k < 8; ++k)
    C[b][i][7 - j] += A[b][k][i] * B[b][k][j]; __systolic_cc_gemm_executed(0); }
}

void bat_down(float A[restrict 4][4][4], float B[restrict 4][4][4], float C[restrict 4][4][4]);
#line 87 "/home/steven-studio/work/systolic-mlir/test/SystolicCC/affine_batched.c"
void bat_down__systolic_reference(float A[restrict 4][4][4], float B[restrict 4][4][4], float C[restrict 4][4][4]) {
  { for (int b = 3; b >= 0; --b) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[b][i][j] += A[b][i][k] * B[b][k][j]; __systolic_cc_gemm_executed(0); }
}

void bat_offset(float A[restrict 5][4][4], float B[restrict 5][4][4], float C[restrict 5][4][4]);
#line 91 "/home/steven-studio/work/systolic-mlir/test/SystolicCC/affine_batched.c"
void bat_offset__systolic_reference(float A[restrict 5][4][4], float B[restrict 5][4][4], float C[restrict 5][4][4]) {
  { for (int b = 0; b < 3; ++b) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[b + 2][i][j] += A[b + 2][i][k] * B[b + 2][k][j]; __systolic_cc_gemm_executed(0); }
}

void bat_tiles(float A[restrict 2][16][16], float B[restrict 2][16][16], float C[restrict 2][16][16]);
#line 95 "/home/steven-studio/work/systolic-mlir/test/SystolicCC/affine_batched.c"
void bat_tiles__systolic_reference(float A[restrict 2][16][16], float B[restrict 2][16][16], float C[restrict 2][16][16]) {
  { for (int b = 0; b < 2; ++b) for (int i = 0; i < 16; ++i) for (int j = 0; j < 16; ++j) for (int k = 0; k < 16; ++k)
    C[b][i][j] += A[b][i][k] * B[b][k][j]; __systolic_cc_gemm_executed(0); }
}
static float mul(float a, float b) { return a * b; }
static void bat_ref(float A[2][4][4], float B[2][4][4], float C[2][4][4]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[b][i][j] = C[b][i][j] + mul(A[b][i][k], B[b][k][j]);
}
static void bat_mid_ref(float A[3][8][8], float B[3][8][8], float C[3][8][8]) {
  for (int i = 0; i < 8; ++i) for (int b = 0; b < 3; ++b) for (int j = 0; j < 8; ++j) for (int k = 0; k < 8; ++k)
    C[b][i][j] = C[b][i][j] + mul(A[b][i][k], B[b][k][j]);
}
static void bat_step_ref(float A[7][4][4], float B[7][4][4], float C[7][4][4]) {
  for (int b = 1; b < 7; b += 2) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[b][i][j] = C[b][i][j] + mul(A[b][i][k], B[b][k][j]);
}
static void bat_two_ref(float A[2][3][4][4], float B[2][3][4][4], float C[2][3][4][4]) {
  for (int p = 0; p < 2; ++p) for (int q = 0; q < 3; ++q) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[p][q][i][j] = C[p][q][i][j] + mul(A[p][q][i][k], B[p][q][k][j]);
}
static void bat_inner_ref(float A[4][2][4], float B[2][4][4], float C[2][4][4]) {
  for (int p = 0; p < 2; ++p) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[p][i][j] = C[p][i][j] + mul(A[i][p][k], B[p][k][j]);
}
static void bat_tr_neg_ref(float A[2][8][8], float B[2][8][8], float C[2][8][8]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) for (int k = 0; k < 8; ++k)
    C[b][i][7 - j] = C[b][i][7 - j] + mul(A[b][k][i], B[b][k][j]);
}
static void bat_down_ref(float A[4][4][4], float B[4][4][4], float C[4][4][4]) {
  for (int b = 3; b >= 0; --b) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[b][i][j] = C[b][i][j] + mul(A[b][i][k], B[b][k][j]);
}
static void bat_offset_ref(float A[5][4][4], float B[5][4][4], float C[5][4][4]) {
  for (int b = 0; b < 3; ++b) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[b + 2][i][j] = C[b + 2][i][j] + mul(A[b + 2][i][k], B[b + 2][k][j]);
}
static void bat_tiles_ref(float A[2][16][16], float B[2][16][16], float C[2][16][16]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 16; ++i) for (int j = 0; j < 16; ++j) for (int k = 0; k < 16; ++k)
    C[b][i][j] = C[b][i][j] + mul(A[b][i][k], B[b][k][j]);
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
  RUN(bat, 2*4*4, 2*4*4, 2*4*4);
  RUN(bat_mid, 3*8*8, 3*8*8, 3*8*8);
  RUN(bat_step, 7*4*4, 7*4*4, 7*4*4);
  RUN(bat_two, 2*3*4*4, 2*3*4*4, 2*3*4*4);
  RUN(bat_inner, 4*2*4, 2*4*4, 2*4*4);
  RUN(bat_tr_neg, 2*8*8, 2*8*8, 2*8*8);
  RUN(bat_down, 4*4*4, 4*4*4, 4*4*4);
  RUN(bat_offset, 5*4*4, 5*4*4, 5*4*4);
  RUN(bat_tiles, 2*16*16, 2*16*16, 2*16*16);
  printf("%s: %d differ\n", differ ? "FAIL" : "PASS", differ); return differ;
}
