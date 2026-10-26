void __systolic_cc_gemm_executed(int);
#line 1 "/home/steven-studio/work/systolic-mlir/test/SystolicCC/affine_mapping.c"
// REQUIRES: cfront-clang
//
// The affine GEMM mapping end to end (docs/AFFINE_GEMM_MAPPING.md): five
// kernels whose loops and subscripts are not the canonical
// C[i][j] += A[i][k] * B[k][j], each offloaded and its result compared with a
// reference that runs the same loops on the CPU (products through a call, so
// it is not offloaded itself):
//
//   expressions  C[4i][2j+1] += A[2i+1][3k] * B[6k+2][j]: six different
//                subscripts, each operand its own strided view
//   runs_once    a loop that runs once (t = 3) inside the nest, in I, in J and
//                in A's and B's views: equal on the iteration domain to
//                C[i+3][j+3] += A[i+6][k] * B[k][j+2] (refused before
//                2026-10-15: the subscripts were compared as expressions)
//   steps        for (i = 5; i >= 0; i -= 2), for (j = 20; 3 * j - 7 > 20; --j),
//                for (k = 14; k >= 0; k -= 2): values 5 3 1 / 20..10 / 14..0,
//                not ordinals
//   split        I split over i0 and i1 = 1, 3, 5, 7 (step 2), strided
//                differently in A (8 i0 + i1) and C (16 i0 + 2 i1 + 3)
//   near_max     long counters 9223372036854775800 .. 9223372036854775806
//
// NO REAL HARDWARE HERE: the software runtime model computes every tile;
// SOFT_RUNTIME_MOCK_HARDWARE=1 makes it claim to be the FPGA.
//
// RUN: %systolic-cc --fp-reassociation --explain %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// PLAN-DAG: note: 'expressions' is offloaded:
// PLAN-DAG: note: 'runs_once' is offloaded:
// PLAN-DAG: note: 'steps' is offloaded:
// PLAN-DAG: note: 'split' is offloaded:
// PLAN-DAG: note: 'near_max' is offloaded:
// PLAN: wrote {{.*}} (5 function(s) offloaded
//
// RUN: %t > %t.soft
// RUN: FileCheck %s --check-prefix=ALL --input-file=%t.soft
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t > %t.mock
// RUN: FileCheck %s --check-prefix=ALL --input-file=%t.mock
// ALL: [case] expressions: equal to the reference, offloaded
// ALL: [case] runs_once: equal to the reference, offloaded
// ALL: reference steps: rows 3 (5..1), columns 11 (20..10), reductions 8 (14..0)
// ALL: [case] steps: equal to the reference, offloaded
// ALL: [case] split: equal to the reference, offloaded
// ALL: [case] near_max: equal to the reference, offloaded
// ALL: PASS: 0 case(s) differ
//
// With --verify every offloaded call is also compared with the function as
// written, run on the CPU.
// RUN: %systolic-cc --fp-reassociation --verify %s -o %t.v > /dev/null 2>&1
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t.v 2>&1 >/dev/null | FileCheck %s --check-prefix=VERIFY
// VERIFY-COUNT-5: verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY-NOT: MISMATCH

#include <stdio.h>
#include <string.h>

#include "systolic_offload.h"


void expressions(float A[restrict 8][10], float B[restrict 21][4],
                 float C[restrict 13][8]);
#line 57 "/home/steven-studio/work/systolic-mlir/test/SystolicCC/affine_mapping.c"
void expressions__systolic_reference(float A[restrict 8][10], float B[restrict 21][4],
                 float C[restrict 13][8]) {
  { for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      for (int k = 0; k < 4; ++k)
        C[4 * i][2 * j + 1] += A[2 * i + 1][3 * k] * B[6 * k + 2][j]; __systolic_cc_gemm_executed(0); }
}


void runs_once(float A[restrict 14][8], float B[restrict 8][10],
          float C[restrict 11][11]);
#line 65 "/home/steven-studio/work/systolic-mlir/test/SystolicCC/affine_mapping.c"
void runs_once__systolic_reference(float A[restrict 14][8], float B[restrict 8][10],
          float C[restrict 11][11]) {
  { for (int i = 0; i < 8; ++i)
    for (int t = 3; t < 4; ++t)
      for (int j = 0; j < 8; ++j)
        for (int k = 0; k < 8; ++k)
          C[i + t][j + t] += A[i + 2 * t][k] * B[k][j - t + 5]; __systolic_cc_gemm_executed(0); }
}


void steps(float A[restrict 6][15], float B[restrict 15][11],
           float C[restrict 6][11]);
#line 74 "/home/steven-studio/work/systolic-mlir/test/SystolicCC/affine_mapping.c"
void steps__systolic_reference(float A[restrict 6][15], float B[restrict 15][11],
           float C[restrict 6][11]) {
  { for (int i = 5; i >= 0; i -= 2)
    for (int j = 20; 3 * j - 7 > 20; --j)
      for (int k = 14; k >= 0; k -= 2)
        C[i][j - 10] += A[i][k] * B[k][j - 10]; __systolic_cc_gemm_executed(0); }
}


void split(float A[restrict 16][8], float B[restrict 8][8],
           float C[restrict 34][8]);
#line 82 "/home/steven-studio/work/systolic-mlir/test/SystolicCC/affine_mapping.c"
void split__systolic_reference(float A[restrict 16][8], float B[restrict 8][8],
           float C[restrict 34][8]) {
  { for (int i0 = 0; i0 < 2; ++i0)
    for (int j = 0; j < 8; ++j)
      for (int i1 = 1; i1 < 8; i1 += 2)
        for (int k = 0; k < 8; ++k)
          C[16 * i0 + 2 * i1 + 3][j] += A[8 * i0 + i1][k] * B[k][j]; __systolic_cc_gemm_executed(0); }
}


void near_max(float A[restrict 7][8], float B[restrict 8][8],
              float C[restrict 7][8]);
#line 91 "/home/steven-studio/work/systolic-mlir/test/SystolicCC/affine_mapping.c"
void near_max__systolic_reference(float A[restrict 7][8], float B[restrict 8][8],
              float C[restrict 7][8]) {
  { for (long i = 9223372036854775800L; i < 9223372036854775807L; ++i)
    for (long j = 0; j < 8; ++j)
      for (long k = 0; k < 8; ++k)
        C[i - 9223372036854775800L][j] +=
            A[i - 9223372036854775800L][k] * B[k][j]; __systolic_cc_gemm_executed(0); }
}

/* The same loops on the CPU, products through a call (not a GEMM statement). */
static float mul(float a, float b) { return a * b; }

static void expressions_ref(float A[8][10], float B[21][4], float C[13][8]) {
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      for (int k = 0; k < 4; ++k)
        C[4 * i][2 * j + 1] =
            C[4 * i][2 * j + 1] + mul(A[2 * i + 1][3 * k], B[6 * k + 2][j]);
}

static void runs_once_ref(float A[14][8], float B[8][10], float C[11][11]) {
  for (int i = 0; i < 8; ++i)
    for (int t = 3; t < 4; ++t)
      for (int j = 0; j < 8; ++j)
        for (int k = 0; k < 8; ++k)
          C[i + t][j + t] =
              C[i + t][j + t] + mul(A[i + 2 * t][k], B[k][j - t + 5]);
}

static void steps_ref(float A[6][15], float B[15][11], float C[6][11]) {
  int rows = 0, cols = 0, reds = 0, i_first = -1, i_last = -1, j_first = -1,
      j_last = -1, k_first = -1, k_last = -1;
  for (int i = 5; i >= 0; i -= 2) {
    ++rows, i_first = i_first < 0 ? i : i_first, i_last = i;
    for (int j = 20; 3 * j - 7 > 20; --j) {
      if (rows == 1)
        ++cols, j_first = j_first < 0 ? j : j_first, j_last = j;
      for (int k = 14; k >= 0; k -= 2) {
        if (rows == 1 && cols == 1)
          ++reds, k_first = k_first < 0 ? k : k_first, k_last = k;
        C[i][j - 10] = C[i][j - 10] + mul(A[i][k], B[k][j - 10]);
      }
    }
  }
  printf("reference steps: rows %d (%d..%d), columns %d (%d..%d), "
         "reductions %d (%d..%d)\n",
         rows, i_first, i_last, cols, j_first, j_last, reds, k_first, k_last);
}

static void split_ref(float A[16][8], float B[8][8], float C[34][8]) {
  for (int i0 = 0; i0 < 2; ++i0)
    for (int j = 0; j < 8; ++j)
      for (int i1 = 1; i1 < 8; i1 += 2)
        for (int k = 0; k < 8; ++k)
          C[16 * i0 + 2 * i1 + 3][j] =
              C[16 * i0 + 2 * i1 + 3][j] + mul(A[8 * i0 + i1][k], B[k][j]);
}

static void near_max_ref(float A[7][8], float B[8][8], float C[7][8]) {
  for (long i = 9223372036854775800L; i < 9223372036854775807L; ++i)
    for (long j = 0; j < 8; ++j)
      for (long k = 0; k < 8; ++k)
        C[i - 9223372036854775800L][j] =
            C[i - 9223372036854775800L][j] +
            mul(A[i - 9223372036854775800L][k], B[k][j]);
}

static unsigned long long invocations(void) {
  systolic_offload_stats_t s;
  systolic_offload_stats(&s);
  return s.per_device[0] + s.per_device[1] + s.per_device[2] + s.per_device[3];
}

/* Small integers: every partial sum is exact in float, so the reassociated
   systolic sum equals the loop's bit for bit. */
static void fill(float *p, int n, int seed) {
  for (int e = 0; e < n; ++e)
    p[e] = (float)((e * 7 + seed) % 9 - 4);
}

static int differ;

static void report(const char *name, const void *got, const void *want,
                   size_t bytes, unsigned long long inv) {
  int same = memcmp(got, want, bytes) == 0;
  differ += !same;
  printf("[case] %s: %s, %s\n", name,
         same ? "equal to the reference" : "DIFFERENT FROM THE REFERENCE",
         inv ? "offloaded" : "NOT OFFLOADED");
}

#define RUN_CASE(name, a, b, c)                                                \
  do {                                                                         \
    static float A[sizeof(a) / sizeof(float)], B[sizeof(b) / sizeof(float)],   \
        C[sizeof(c) / sizeof(float)], R[sizeof(c) / sizeof(float)];            \
    fill(A, sizeof A / sizeof(float), 1);                                      \
    fill(B, sizeof B / sizeof(float), 2);                                      \
    fill(C, sizeof C / sizeof(float), 3);                                      \
    memcpy(R, C, sizeof C);                                                    \
    unsigned long long before = invocations();                                 \
    name((void *)A, (void *)B, (void *)C);                                     \
    unsigned long long inv = invocations() - before;                           \
    name##_ref((void *)A, (void *)B, (void *)R);                               \
    report(#name, C, R, sizeof C, inv);                                        \
  } while (0)

int main(void) {
  float expressions_a[8][10], expressions_b[21][4], expressions_c[13][8];
  float runs_once_a[14][8], runs_once_b[8][10], runs_once_c[11][11];
  float steps_a[6][15], steps_b[15][11], steps_c[6][11];
  float split_a[16][8], split_b[8][8], split_c[34][8];
  float near_max_a[7][8], near_max_b[8][8], near_max_c[7][8];
  printf("affine mapping test\n");
  RUN_CASE(expressions, expressions_a, expressions_b, expressions_c);
  RUN_CASE(runs_once, runs_once_a, runs_once_b, runs_once_c);
  RUN_CASE(steps, steps_a, steps_b, steps_c);
  RUN_CASE(split, split_a, split_b, split_c);
  RUN_CASE(near_max, near_max_a, near_max_b, near_max_c);
  printf("%s: %d case(s) differ\n", differ ? "FAIL" : "PASS", differ);
  return differ != 0;
}
