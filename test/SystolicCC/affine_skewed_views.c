// REQUIRES: cfront-clang
//
// Skewed, merged and flat operand views end to end (docs/AFFINE_GEMM_MAPPING.md
// section 19, Phase D: section 19.11). A dimension of an operand may combine
// both of the operand's GEMM roles (A[i][i + k]: a skew), a role may fill one
// flat dimension together with the other (C[4i + j]), or a one-dimensional
// operand may be read along a skew (A[i + k], a Hankel view): the element
// address is still affine in (row, column), and the lowering builds the view
// as one memref.reinterpret_cast. Each kernel is compared with a reference
// that runs the same loops on the CPU (products through a call, so the
// reference is not offloaded itself), bitwise, on integer-valued data:
//
//   sv_test1         C[i][j] += A[i][i+k] B[j+k][j]           Test 1 of the request (ext_026)
//   sv_flat          C[4i+j] += A[4i+k] B[4k+j]               one-dimensional arrays, the C idiom
//   sv_skewed_c      C[2i+j][j] += A[2i+3][k+1] B[k+2][3j+2]  the seed of case_088 (a skewed output view)
//   sv_hankel        C[i][j] += A[i+k] B[k][j]                a Hankel view: A' repeats elements
//   sv_skew_a        C[i][j] += A[i][i+k] B[k][j]             case_126
//   sv_skew_transposed  C[i][j] += A[i+k][i] B[j][j+k]        both factors skewed and stored roles-swapped
//   sv_skew_negative C[i][j] += A[i][6-i-k] B[k][j]           a skew read backwards (stride -1 along K)
//   sv_batch_skew    C[b][i][j] += A[b][i][i+k] B[b][j+k][j]  Test 1 per batch value
//   sv_tiles         Test 1 at 16 x 16 x 16                   several tiles over the skewed views
//   sv_test3         C[i][j] += A[i][j+k] B[i+k][j]           Test 3: a batch of dot products (SQ-5, ext_028)
//   sv_case087       C[i+j][j] += A[i+j][k] B[k][j]           a batched GEMV (C's window moves with j)
//   sv_case127       C[i+k][j] += A[i][k] B[k][j]             batched rank-1 updates, overlapping C windows
//   sv_test2_dstar   Test 2 on the lattice domain D*, written over (a, J', b): the reference runs
//                    the ORIGINAL Test 2 statement over the points of D* (section 19.6)
//   sv_test2_box     Test 2 on a box: not a GEMM (element-wise), stays on the host
//   sv_axa_same_skew C[i][j] += A[i][i+k] A[k][k+j]           one array as both factors through ONE skewed view
//   sv_axa_rev_skew  C[i][j] += A[i][i+3-k] A[3-k][3-k+j]     the same, K read downward by both: one view once K is
//                                                           enumerated downward (the lowering tries the orientations)
//   sv_axa_rows_rev  C[i][j] += A[3-i][k] A[k][j]             one tensor (A with its rows reversed) serves both factors;
//                                                           C is written through a reversed view
//   sv_once_const    C[2][0] += A[1][0] B[0][0] in two loops of one iteration: the 1 x 1 x 1 GEMM
//
// NO REAL HARDWARE HERE: the software runtime model computes every tile;
// SOFT_RUNTIME_MOCK_HARDWARE=1 makes it claim to be the FPGA.
//
// RUN: %systolic-cc --fp-reassociation --explain %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// PLAN-DAG: note: 'sv_test1' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'sv_flat' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'sv_skewed_c' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'sv_hankel' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'sv_skew_a' is offloaded: {{[0-9]+}} systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'sv_skew_transposed' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'sv_skew_negative' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'sv_batch_skew' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 2 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'sv_tiles' is offloaded: {{[0-9]+}} systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'sv_test3' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 16 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'sv_case087' is offloaded: {{[0-9]+}} systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; {{[0-9]+}} invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'sv_case127' is offloaded: {{[0-9]+}} systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; {{[0-9]+}} invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'sv_test2_dstar' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'sv_test2_box' stays on the host
// PLAN-DAG: note: 'sv_axa_same_skew' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'sv_axa_rev_skew' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'sv_axa_rows_rev' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'sv_once_const' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN: wrote {{.*}} (17 function(s) offloaded
//
// RUN: %t > %t.soft
// RUN: FileCheck %s --check-prefix=ALL --input-file=%t.soft
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t > %t.mock
// RUN: FileCheck %s --check-prefix=ALL --input-file=%t.mock
// ALL: [case] sv_test1: equal, offloaded (1)
// ALL: [case] sv_flat: equal, offloaded (1)
// ALL: [case] sv_skewed_c: equal, offloaded (1)
// ALL: [case] sv_hankel: equal, offloaded (1)
// ALL: [case] sv_skew_a: equal, offloaded ({{[0-9]+}})
// ALL: [case] sv_skew_transposed: equal, offloaded (1)
// ALL: [case] sv_skew_negative: equal, offloaded (1)
// ALL: [case] sv_batch_skew: equal, offloaded (2)
// ALL: [case] sv_tiles: equal, offloaded ({{[0-9]+}})
// ALL: [case] sv_test3: equal, offloaded (16)
// ALL: [case] sv_case087: equal, offloaded ({{[0-9]+}})
// ALL: [case] sv_case127: equal, offloaded ({{[0-9]+}})
// ALL: [case] sv_test2_dstar: equal, offloaded (1)
// ALL: [case] sv_test2_box: equal, NOT OFFLOADED (0)
// ALL: [case] sv_axa_same_skew: equal, offloaded (1)
// ALL: [case] sv_axa_rev_skew: equal, offloaded (1)
// ALL: [case] sv_axa_rows_rev: equal, offloaded (1)
// ALL: [case] sv_once_const: equal, offloaded (1)
// ALL: PASS: 0 differ
//
// Every offloaded call also against the function as written, on the CPU:
// RUN: %systolic-cc --fp-reassociation --verify %s -o %t.v > /dev/null 2>&1
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t.v 2>&1 >/dev/null | FileCheck %s --check-prefix=VERIFY
// VERIFY-COUNT-17: verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY-NOT: MISMATCH
//
// Without the reassociation permission (the default) nothing is offloaded:
// RUN: %systolic-cc --explain %s -o %t.strict 2>&1 | FileCheck %s --check-prefix=STRICT
// STRICT: wrote {{.*}} (0 function(s) offloaded

#include <stdio.h>
#include <string.h>
#include "systolic_offload.h"

void sv_test1(float A[restrict 4][7], float B[restrict 7][4], float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] += A[i][i + k] * B[j + k][j];
}
void sv_flat(float A[restrict 16], float B[restrict 16], float C[restrict 16]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[4 * i + j] += A[4 * i + k] * B[4 * k + j];
}
void sv_skewed_c(float A[restrict 10][5], float B[restrict 6][12], float C[restrict 10][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[2 * i + j][j] += A[2 * i + 3][k + 1] * B[k + 2][3 * j + 2];
}
void sv_hankel(float A[restrict 7], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] += A[i + k] * B[k][j];
}
void sv_skew_a(float A[restrict 8][15], float B[restrict 8][8], float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) for (int k = 0; k < 8; ++k)
    C[i][j] += A[i][i + k] * B[k][j];
}
void sv_skew_transposed(float A[restrict 7][4], float B[restrict 4][7], float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] += A[i + k][i] * B[j][j + k];
}
void sv_skew_negative(float A[restrict 4][7], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] += A[i][6 - i - k] * B[k][j];
}
void sv_batch_skew(float A[restrict 2][4][7], float B[restrict 2][7][4], float C[restrict 2][4][4]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[b][i][j] += A[b][i][i + k] * B[b][j + k][j];
}
void sv_tiles(float A[restrict 16][31], float B[restrict 31][16], float C[restrict 16][16]) {
  for (int i = 0; i < 16; ++i) for (int j = 0; j < 16; ++j) for (int k = 0; k < 16; ++k)
    C[i][j] += A[i][i + k] * B[j + k][j];
}
void sv_test3(float A[restrict 4][7], float B[restrict 7][4], float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] += A[i][j + k] * B[i + k][j];
}
void sv_case087(float A[restrict 15][8], float B[restrict 8][8], float C[restrict 15][8]) {
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) for (int k = 0; k < 8; ++k)
    C[i + j][j] += A[i + j][k] * B[k][j];
}
void sv_case127(float A[restrict 8][8], float B[restrict 8][8], float C[restrict 15][8]) {
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) for (int k = 0; k < 8; ++k)
    C[i + k][j] += A[i][k] * B[k][j];
}
/* Test 2 over D* = {x0 + a u_I + J' u_J + b d}, (a, J', b) in [0,2)^3, with u_I = (1,0,-1),
   u_J = (-1,1,0), d = (5,-3,1), x0 = (5,-1,1): on D* the subscripts are Ic = 2 + 3a, Jc = 4 + J',
   Ka = Kb = 5 + 3b (section 19.6), which is this nest. */
void sv_test2_dstar(float A[restrict 9][11], float B[restrict 11][6], float C[restrict 9][6]) {
  for (int a = 0; a < 2; ++a) for (int jj = 0; jj < 2; ++jj) for (int b = 0; b < 2; ++b)
    C[3 * a + 2][jj + 4] += A[3 * a + 2][3 * b + 5] * B[3 * b + 5][jj + 4];
}
void sv_test2_box(float A[restrict 15][16], float B[restrict 16][21], float C[restrict 15][21]) {
  for (int i = 2; i < 10; ++i) for (int j = 0; j < 6; ++j) for (int k = 0; k < 2; ++k)
    C[i + j - 2 * k][i + 2 * j + k] += A[i + j - 2 * k][i + j + k] * B[i + j + k][i + 2 * j + k];
}

static float mul(float a, float b) { return a * b; }
void sv_axa_same_skew(float A[restrict 4][7], float B[restrict 4][7], float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] += A[i][i + k] * A[k][k + j];
}
void sv_axa_rev_skew(float A[restrict 4][7], float B[restrict 4][7], float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] += A[i][i + 3 - k] * A[3 - k][3 - k + j];
}
void sv_axa_rows_rev(float A[restrict 4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] += A[3 - i][k] * A[k][j];
}
void sv_once_const(float A[restrict 4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int k = 1; k < 2; ++k) for (int i = 0; i < 1; ++i)
    C[2][0] += A[1][0] * B[0][0];
}
static void sv_test1_ref(float A[4][7], float B[7][4], float C[4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] = C[i][j] + mul(A[i][i + k], B[j + k][j]);
}
static void sv_flat_ref(float A[16], float B[16], float C[16]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[4 * i + j] = C[4 * i + j] + mul(A[4 * i + k], B[4 * k + j]);
}
static void sv_skewed_c_ref(float A[10][5], float B[6][12], float C[10][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[2 * i + j][j] = C[2 * i + j][j] + mul(A[2 * i + 3][k + 1], B[k + 2][3 * j + 2]);
}
static void sv_hankel_ref(float A[7], float B[4][4], float C[4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] = C[i][j] + mul(A[i + k], B[k][j]);
}
static void sv_skew_a_ref(float A[8][15], float B[8][8], float C[8][8]) {
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) for (int k = 0; k < 8; ++k)
    C[i][j] = C[i][j] + mul(A[i][i + k], B[k][j]);
}
static void sv_skew_transposed_ref(float A[7][4], float B[4][7], float C[4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] = C[i][j] + mul(A[i + k][i], B[j][j + k]);
}
static void sv_skew_negative_ref(float A[4][7], float B[4][4], float C[4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] = C[i][j] + mul(A[i][6 - i - k], B[k][j]);
}
static void sv_batch_skew_ref(float A[2][4][7], float B[2][7][4], float C[2][4][4]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[b][i][j] = C[b][i][j] + mul(A[b][i][i + k], B[b][j + k][j]);
}
static void sv_tiles_ref(float A[16][31], float B[31][16], float C[16][16]) {
  for (int i = 0; i < 16; ++i) for (int j = 0; j < 16; ++j) for (int k = 0; k < 16; ++k)
    C[i][j] = C[i][j] + mul(A[i][i + k], B[j + k][j]);
}
static void sv_test3_ref(float A[4][7], float B[7][4], float C[4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] = C[i][j] + mul(A[i][j + k], B[i + k][j]);
}
static void sv_case087_ref(float A[15][8], float B[8][8], float C[15][8]) {
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) for (int k = 0; k < 8; ++k)
    C[i + j][j] = C[i + j][j] + mul(A[i + j][k], B[k][j]);
}
static void sv_case127_ref(float A[8][8], float B[8][8], float C[15][8]) {
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) for (int k = 0; k < 8; ++k)
    C[i + k][j] = C[i + k][j] + mul(A[i][k], B[k][j]);
}
/* The reference of sv_test2_dstar is the ORIGINAL statement of Test 2 run over the points
   of D*: (i, j, k) = x0 + a u_I + J' u_J + b d. Equality checks the reparametrization. */
static void sv_test2_dstar_ref(float A[9][11], float B[11][6], float C[9][6]) {
  for (int a = 0; a < 2; ++a) for (int jj = 0; jj < 2; ++jj) for (int b = 0; b < 2; ++b) {
    int i = 5 + a - jj + 5 * b, j = -1 + jj - 3 * b, k = 1 - a + b;
    C[i + j - 2 * k][i + 2 * j + k] = C[i + j - 2 * k][i + 2 * j + k]
        + mul(A[i + j - 2 * k][i + j + k], B[i + j + k][i + 2 * j + k]);
  }
}
static void sv_test2_box_ref(float A[15][16], float B[16][21], float C[15][21]) {
  for (int i = 2; i < 10; ++i) for (int j = 0; j < 6; ++j) for (int k = 0; k < 2; ++k)
    C[i + j - 2 * k][i + 2 * j + k] = C[i + j - 2 * k][i + 2 * j + k]
        + mul(A[i + j - 2 * k][i + j + k], B[i + j + k][i + 2 * j + k]);
}

static void sv_axa_same_skew_ref(float A[4][7], float B[4][7], float C[4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] = C[i][j] + mul(A[i][i + k], A[k][k + j]);
}
static void sv_axa_rev_skew_ref(float A[4][7], float B[4][7], float C[4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] = C[i][j] + mul(A[i][i + 3 - k], A[3 - k][3 - k + j]);
}
static void sv_axa_rows_rev_ref(float A[4][4], float B[4][4], float C[4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
    C[i][j] = C[i][j] + mul(A[3 - i][k], A[k][j]);
}
static void sv_once_const_ref(float A[4][4], float B[4][4], float C[4][4]) {
  for (int k = 1; k < 2; ++k) for (int i = 0; i < 1; ++i)
    C[2][0] = C[2][0] + mul(A[1][0], B[0][0]);
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
  RUN(sv_test1, 4*7, 7*4, 4*4);
  RUN(sv_flat, 16, 16, 16);
  RUN(sv_skewed_c, 10*5, 6*12, 10*4);
  RUN(sv_hankel, 7, 4*4, 4*4);
  RUN(sv_skew_a, 8*15, 8*8, 8*8);
  RUN(sv_skew_transposed, 7*4, 4*7, 4*4);
  RUN(sv_skew_negative, 4*7, 4*4, 4*4);
  RUN(sv_batch_skew, 2*4*7, 2*7*4, 2*4*4);
  RUN(sv_tiles, 16*31, 31*16, 16*16);
  RUN(sv_test3, 4*7, 7*4, 4*4);
  RUN(sv_case087, 15*8, 8*8, 15*8);
  RUN(sv_case127, 8*8, 8*8, 15*8);
  RUN(sv_test2_dstar, 9*11, 11*6, 9*6);
  RUN(sv_test2_box, 15*16, 16*21, 15*21);
  RUN(sv_axa_same_skew, 4*7, 4*7, 4*4);
  RUN(sv_axa_rev_skew, 4*7, 4*7, 4*4);
  RUN(sv_axa_rows_rev, 4*4, 4*4, 4*4);
  RUN(sv_once_const, 4*4, 4*4, 4*4);
  printf("%s: %d differ\n", differ ? "FAIL" : "PASS", differ); return differ;
}
