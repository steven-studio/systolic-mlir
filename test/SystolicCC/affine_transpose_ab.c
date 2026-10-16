// REQUIRES: cfront-clang
//
// A and B transposed, alone or together, end to end (Phase 2 of 2026-10-16,
// docs/AFFINE_GEMM_MAPPING.md section 5): every kernel compiled by systolic-cc,
// its tiles run, the result compared with a reference that runs the same loops
// on the CPU (products through a call, so it is not offloaded itself):
//
//   nn            C[i][j] += A[i][k] B[k][j]                    neither transposed
//   tn            A[k][i]                                       A transposed
//   nt            B[j][k]                                       B transposed
//   tt            A[k][i], B[j][k]  (4 x 6 x 8)                  both
//   tt_offset     A[k + 2][i + 3], B[j + 1][k + 2]              both, offsets
//   tt_stride     A[2k][3i], B[2j][3k]                          both, strides
//   tt_negative   A[7 - k][3 - i], B[j][k]                      both, A backwards along K and I
//   nt_negative   B[7 - j][k]                                   B transposed and backwards
//   tt_tiles      both, 16 x 16 x 16                            ten tiles
//   tt_rect       both, 8 x 40 x 24                              22 tiles, rectangular
//   tt_batch      C[b][i][j] += A[k][i + b] B[j][k + b]         both, views moving with the batch (ISSUE-007)
//
// NO REAL HARDWARE HERE: the software runtime model computes every tile;
// SOFT_RUNTIME_MOCK_HARDWARE=1 makes it claim to be the FPGA.
//
// RUN: %systolic-cc --fp-reassociation --explain %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// PLAN-DAG: note: 'nn' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'tn' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'nt' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'tt' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'tt_offset' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'tt_stride' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'tt_negative' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'nt_negative' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'tt_tiles' is offloaded: 10 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'tt_rect' is offloaded: 22 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'tt_batch' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 2 invocation(s) per call (the loops around them repeat them)
// PLAN: wrote {{.*}} (11 function(s) offloaded
//
// RUN: %t > %t.soft
// RUN: FileCheck %s --check-prefix=ALL --input-file=%t.soft
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t > %t.mock
// RUN: FileCheck %s --check-prefix=ALL --input-file=%t.mock
// ALL: [case] nn: equal, offloaded (1)
// ALL: [case] tn: equal, offloaded (1)
// ALL: [case] nt: equal, offloaded (1)
// ALL: [case] tt: equal, offloaded (2)
// ALL: [case] tt_offset: equal, offloaded (2)
// ALL: [case] tt_stride: equal, offloaded (2)
// ALL: [case] tt_negative: equal, offloaded (2)
// ALL: [case] nt_negative: equal, offloaded (1)
// ALL: [case] tt_tiles: equal, offloaded (10)
// ALL: [case] tt_rect: equal, offloaded (22)
// ALL: [case] tt_batch: equal, offloaded (2)
// ALL: PASS: 0 differ
//
// Every offloaded call also against the function as written, on the CPU:
// RUN: %systolic-cc --fp-reassociation --verify %s -o %t.v > /dev/null 2>&1
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t.v 2>&1 >/dev/null | FileCheck %s --check-prefix=VERIFY
// VERIFY-COUNT-11: verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY-NOT: MISMATCH
//
// Without the reassociation permission (the default) nothing is offloaded:
// RUN: %systolic-cc --explain %s -o %t.strict 2>&1 | FileCheck %s --check-prefix=STRICT
// STRICT: wrote {{.*}} (0 function(s) offloaded

#include <stdio.h>
#include <string.h>
#include "systolic_offload.h"

void nn(float A[restrict 8][8], float B[restrict 8][8], float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) for (int k = 0; k < 8; ++k)
    C[i][j] += A[i][k] * B[k][j];
}
void tn(float A[restrict 8][8], float B[restrict 8][8], float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) for (int k = 0; k < 8; ++k)
    C[i][j] += A[k][i] * B[k][j];
}
void nt(float A[restrict 8][8], float B[restrict 8][8], float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) for (int k = 0; k < 8; ++k)
    C[i][j] += A[i][k] * B[j][k];
}
void tt(float A[restrict 8][4], float B[restrict 6][8], float C[restrict 4][6]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 6; ++j) for (int k = 0; k < 8; ++k)
    C[i][j] += A[k][i] * B[j][k];
}
void tt_offset(float A[restrict 10][7], float B[restrict 7][10], float C[restrict 4][6]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 6; ++j) for (int k = 0; k < 8; ++k)
    C[i][j] += A[k + 2][i + 3] * B[j + 1][k + 2];
}
void tt_stride(float A[restrict 15][10], float B[restrict 11][22], float C[restrict 4][6]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 6; ++j) for (int k = 0; k < 8; ++k)
    C[i][j] += A[2 * k][3 * i] * B[2 * j][3 * k];
}
void tt_negative(float A[restrict 8][4], float B[restrict 6][8], float C[restrict 4][6]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 6; ++j) for (int k = 0; k < 8; ++k)
    C[i][j] += A[7 - k][3 - i] * B[j][k];
}
void nt_negative(float A[restrict 8][8], float B[restrict 8][8], float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) for (int k = 0; k < 8; ++k)
    C[i][j] += A[i][k] * B[7 - j][k];
}
void tt_tiles(float A[restrict 16][16], float B[restrict 16][16], float C[restrict 16][16]) {
  for (int i = 0; i < 16; ++i) for (int j = 0; j < 16; ++j) for (int k = 0; k < 16; ++k)
    C[i][j] += A[k][i] * B[j][k];
}
void tt_rect(float A[restrict 24][8], float B[restrict 40][24], float C[restrict 8][40]) {
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 40; ++j) for (int k = 0; k < 24; ++k)
    C[i][j] += A[k][i] * B[j][k];
}
void tt_batch(float A[restrict 4][5], float B[restrict 4][5], float C[restrict 2][4][4]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[b][i][j] += A[k][i + b] * B[j][k + b];
}

static float mul(float a, float b) { return a * b; }
static void nn_ref(float A[8][8], float B[8][8], float C[8][8]) {
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) for (int k = 0; k < 8; ++k)
    C[i][j] = C[i][j] + mul(A[i][k], B[k][j]);
}
static void tn_ref(float A[8][8], float B[8][8], float C[8][8]) {
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) for (int k = 0; k < 8; ++k)
    C[i][j] = C[i][j] + mul(A[k][i], B[k][j]);
}
static void nt_ref(float A[8][8], float B[8][8], float C[8][8]) {
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) for (int k = 0; k < 8; ++k)
    C[i][j] = C[i][j] + mul(A[i][k], B[j][k]);
}
static void tt_ref(float A[8][4], float B[6][8], float C[4][6]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 6; ++j) for (int k = 0; k < 8; ++k)
    C[i][j] = C[i][j] + mul(A[k][i], B[j][k]);
}
static void tt_offset_ref(float A[10][7], float B[7][10], float C[4][6]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 6; ++j) for (int k = 0; k < 8; ++k)
    C[i][j] = C[i][j] + mul(A[k + 2][i + 3], B[j + 1][k + 2]);
}
static void tt_stride_ref(float A[15][10], float B[11][22], float C[4][6]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 6; ++j) for (int k = 0; k < 8; ++k)
    C[i][j] = C[i][j] + mul(A[2 * k][3 * i], B[2 * j][3 * k]);
}
static void tt_negative_ref(float A[8][4], float B[6][8], float C[4][6]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 6; ++j) for (int k = 0; k < 8; ++k)
    C[i][j] = C[i][j] + mul(A[7 - k][3 - i], B[j][k]);
}
static void nt_negative_ref(float A[8][8], float B[8][8], float C[8][8]) {
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) for (int k = 0; k < 8; ++k)
    C[i][j] = C[i][j] + mul(A[i][k], B[7 - j][k]);
}
static void tt_tiles_ref(float A[16][16], float B[16][16], float C[16][16]) {
  for (int i = 0; i < 16; ++i) for (int j = 0; j < 16; ++j) for (int k = 0; k < 16; ++k)
    C[i][j] = C[i][j] + mul(A[k][i], B[j][k]);
}
static void tt_rect_ref(float A[24][8], float B[40][24], float C[8][40]) {
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 40; ++j) for (int k = 0; k < 24; ++k)
    C[i][j] = C[i][j] + mul(A[k][i], B[j][k]);
}
static void tt_batch_ref(float A[4][5], float B[4][5], float C[2][4][4]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[b][i][j] = C[b][i][j] + mul(A[k][i + b], B[j][k + b]);
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
  RUN(nn, 8*8, 8*8, 8*8);
  RUN(tn, 8*8, 8*8, 8*8);
  RUN(nt, 8*8, 8*8, 8*8);
  RUN(tt, 8*4, 6*8, 4*6);
  RUN(tt_offset, 10*7, 7*10, 4*6);
  RUN(tt_stride, 15*10, 11*22, 4*6);
  RUN(tt_negative, 8*4, 6*8, 4*6);
  RUN(nt_negative, 8*8, 8*8, 8*8);
  RUN(tt_tiles, 16*16, 16*16, 16*16);
  RUN(tt_rect, 24*8, 40*24, 8*40);
  RUN(tt_batch, 4*5, 4*5, 2*4*4);
  printf("%s: %d differ\n", differ ? "FAIL" : "PASS", differ); return differ;
}
