// REQUIRES: cfront-clang
//
// Phase 5.4.3d-3b-6 (docs/AFFINE_GEMM_MAPPING.md section 27): the certified
// device-job lowering of recognized GEMMs, end to end on the software
// runtime model, compared bitwise with the CPU reference on integer data
// (the relaxed contract of section 6 is exact when every partial sum is
// exact). SYSTOLIC_CC_TEST_CERTIFIED_OFFLOAD_FIRST=1 is the test control
// under which the certified lowering takes the nests before the established
// fast path; the same program through the fast path gives the same output.
//
// RUN: env SYSTOLIC_CC_TEST_CERTIFIED_OFFLOAD_FIRST=1 %systolic-cc --fp-reassociation --verify --explain --keep-dir %t.keep %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// RUN: FileCheck %s --check-prefix=REMARKS --input-file=%t.keep/20-remarks.txt
// RUN: %t > %t.out 2> %t.err
// RUN: FileCheck %s --check-prefix=VERIFY --input-file=%t.err
// RUN: %host-cc -std=c11 -O0 -ffp-contract=off %s -o %t.cpu
// RUN: %t.cpu > %t.cpu.out
// RUN: grep -v '^\[systolic-cc\]' %t.out | diff %t.cpu.out -
//
// The established fast path on the same program: the same output.
// RUN: %systolic-cc --fp-reassociation --verify %s -o %t.fast > /dev/null 2>&1
// RUN: %t.fast > %t.fast.out 2> /dev/null
// RUN: grep -v '^\[systolic-cc\]' %t.fast.out | diff %t.cpu.out -
//
// Strict floating point (no --fp-reassociation): no GEMM is offloaded --
// the device's order of additions, its +0 start and its flush-to-zero are
// not the loop's -- and the program's loops run as written.
// RUN: env SYSTOLIC_CC_TEST_CERTIFIED_OFFLOAD_FIRST=1 %systolic-cc --verify --explain %s -o %t.strict 2>&1 | FileCheck %s --check-prefix=STRICT
// RUN: %t.strict > %t.strict.out 2> /dev/null
// RUN: grep -v '^\[systolic-cc\]' %t.strict.out | diff %t.cpu.out -

#include <stdio.h>

// The standard contiguous GEMM: ten invocations over the 1x8x8 + 3x4x4
// fleet.
void gemm16(float A[restrict 16][16], float B[restrict 16][16],
            float C[restrict 16][16]) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 16; ++j)
      for (int k = 0; k < 16; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

// K = 40 beyond k_max = 16: folds added into C in order.
void gemm_k40(float A[restrict 8][40], float B[restrict 40][8],
              float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 40; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

// A partial tile: M = 6 rows on the 8x8 array.
void partial(float A[restrict 6][8], float B[restrict 8][8],
             float C[restrict 6][8]) {
  for (int i = 0; i < 6; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

// Two independent nests in one function, with a statement between them.
void two_nests(float A[restrict 4][4], float B[restrict 4][4],
               float C[restrict 4][4], float D[restrict 4][4]) {
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      for (int k = 0; k < 4; ++k)
        C[i][j] += A[i][k] * B[k][j];
  D[0][0] += C[1][1];
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      for (int k = 0; k < 4; ++k)
        D[i][j] += A[i][k] * B[k][j];
}

// A sub-box of 8x8 arrays at constant offsets: static subviews.
void offsets(float A[restrict 8][8], float B[restrict 8][8],
             float C[restrict 8][8]) {
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      for (int k = 0; k < 4; ++k)
        C[i + 1][j + 2] += A[i + 1][k] * B[k][j + 2];
}

// A transposed factor (A read as (K, I)): outside the certified contract in
// this phase; the established fast path lowers it through its views.
void transposed(float A[restrict 4][4], float B[restrict 4][4],
                float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      for (int k = 0; k < 4; ++k)
        C[i][j] += A[k][i] * B[k][j];
}

// A triangular domain (k <= i): not a box, no device job in this phase and
// no fast plan; it runs on the host.
void triangular(float A[restrict 4][4], float B[restrict 4][4],
                float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      for (int k = 0; k <= i; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

// Arguments that may alias: the shared preflight refuses, host.
void may_alias(float (*A)[4], float (*B)[4], float (*C)[4]) {
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      for (int k = 0; k < 4; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

static float A16[16][16], B16[16][16], C16[16][16];
static float A40[8][40], B40[40][8], C8[8][8];
static float A6[6][8], B8[8][8], C6[6][8];
static float A4[4][4], B4[4][4], C4[4][4], D4[4][4];
static float A8[8][8], B8b[8][8], C8b[8][8];
static float T4[4][4], U4[4][4], V4[4][4], W4[4][4], X4[4][4], Y4[4][4];

int main(void) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 16; ++j) {
      A16[i][j] = (float)((i * 7 + j * 3) % 9 - 4);
      B16[i][j] = (float)((i * 5 + j * 11) % 7 - 3);
      C16[i][j] = (float)((i + 2 * j) % 5 - 2);
    }
  for (int i = 0; i < 8; ++i) {
    for (int k = 0; k < 40; ++k) A40[i][k] = (float)((i * 3 + k) % 7 - 3);
    for (int j = 0; j < 8; ++j) C8[i][j] = (float)((i - j) % 3);
  }
  for (int k = 0; k < 40; ++k)
    for (int j = 0; j < 8; ++j) B40[k][j] = (float)((k * 2 + j * 5) % 5 - 2);
  for (int i = 0; i < 6; ++i)
    for (int j = 0; j < 8; ++j) {
      A6[i][j] = (float)((i * 2 + j) % 5 - 2);
      C6[i][j] = (float)(j - i);
    }
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j) {
      B8[i][j] = (float)((i + 3 * j) % 7 - 3);
      A8[i][j] = (float)((i * 5 + j) % 6 - 2);
      B8b[i][j] = (float)((2 * i + j) % 5 - 2);
      C8b[i][j] = (float)(i * 8 + j);
    }
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j) {
      A4[i][j] = (float)(i + j - 2);
      B4[i][j] = (float)(i - j + 1);
      C4[i][j] = (float)(i * j);
      D4[i][j] = (float)(j - i);
      T4[i][j] = (float)((i * 3 + j) % 4 - 1);
      U4[i][j] = (float)((i + j * 2) % 3 - 1);
      V4[i][j] = 1.0f;
      W4[i][j] = (float)(i - 1);
      X4[i][j] = (float)(j - 2);
      Y4[i][j] = 2.0f;
    }

  gemm16(A16, B16, C16);
  gemm_k40(A40, B40, C8);
  partial(A6, B8, C6);
  two_nests(A4, B4, C4, D4);
  offsets(A8, B8b, C8b);
  transposed(T4, U4, V4);
  triangular(W4, X4, Y4);
  may_alias(A4, B4, D4);

  double s16 = 0, s8 = 0, s6 = 0, s4 = 0, sd = 0, so = 0, st = 0, sy = 0;
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 16; ++j) s16 += C16[i][j] * (i + 1) - j;
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j) {
      s8 += C8[i][j] * (j + 1) - i;
      so += C8b[i][j] * (i + 1);
    }
  for (int i = 0; i < 6; ++i)
    for (int j = 0; j < 8; ++j) s6 += C6[i][j] * (i + j + 1);
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j) {
      s4 += C4[i][j] * (i + 1);
      sd += D4[i][j] * (j + 1);
      st += V4[i][j] * (i + 2 * j + 1);
      sy += Y4[i][j] * (i + 1);
    }
  printf("%.1f %.1f %.1f %.1f %.1f %.1f %.1f %.1f %g %g %g\n", s16, s8, s6,
         s4, sd, so, st, sy, C16[3][5], C8[7][7], C8b[4][5]);
  return 0;
}

// The plan (the notes come from the pass's remarks, parsed by the driver):
//
// PLAN-DAG: note: 'gemm16' is offloaded: 10 systolic tile invocation(s)
// PLAN-DAG: note: 'gemm_k40' is offloaded: {{[0-9]+}} systolic tile invocation(s)
// PLAN-DAG: note: 'partial' is offloaded: {{[0-9]+}} systolic tile invocation(s)
// PLAN-DAG: note: 'two_nests' is offloaded: {{[0-9]+}} systolic tile invocation(s)
// PLAN-DAG: note: 'offsets' is offloaded: 1 systolic tile invocation(s)
// PLAN-DAG: note: 'transposed' is offloaded: {{[0-9]+}} systolic tile invocation(s)
// PLAN-DAG: note: 'triangular' stays on the host
// PLAN-DAG: note: 'may_alias' stays on the host
// PLAN:     wrote {{.*}} (6 function(s) offloaded, runtime: software model, NOT the FPGA
//
// Which path made each plan (20-remarks.txt of the kept compilation):
//
// REMARKS-DAG: remark: affine GEMM 16x16x16 lowered as 10 exact region(s) by the certified device-job lowering (docs/AFFINE_GEMM_MAPPING.md section 27) on 4 accelerator(s)
// REMARKS-DAG: remark: affine GEMM 8x8x40 lowered as {{[0-9]+}} exact region(s) by the certified device-job lowering
// REMARKS-DAG: remark: affine GEMM 6x8x8 lowered as {{[0-9]+}} exact region(s) by the certified device-job lowering
// REMARKS-DAG: remark: affine GEMM 4x4x4 lowered as 1 exact region(s) by the certified device-job lowering
// REMARKS-DAG: remark: certified device offload not applied: UNKNOWN: A is not read as (I, K)
// REMARKS-DAG: remark: certified device offload not applied: UNKNOWN: the logical domain is not a box
// REMARKS-DAG: remark: guarded MAC replacement not applied: UNKNOWN: A may alias C
//
// Every offloaded call equals the CPU reference bitwise:
//
// VERIFY-DAG: gemm16 ({{.*}}): verified: 256 elements equal the CPU reference (bitwise)
// VERIFY-DAG: gemm_k40 ({{.*}}): verified: 64 elements equal the CPU reference (bitwise)
// VERIFY-DAG: partial ({{.*}}): verified: 48 elements equal the CPU reference (bitwise)
// VERIFY-DAG: two_nests ({{.*}}): verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY-DAG: offsets ({{.*}}): verified: 40 elements equal the CPU reference (bitwise)
// VERIFY-DAG: transposed ({{.*}}): verified: 16 elements equal the CPU reference (bitwise)
// VERIFY-NOT: MISMATCH
//
// STRICT-DAG: note: 'gemm16' stays on the host: its GEMM is offloaded only with permission to reassociate floating-point sums (--fp-reassociation)
// STRICT-DAG: note: 'offsets' stays on the host: its GEMM is offloaded only with permission to reassociate floating-point sums (--fp-reassociation)
// STRICT:     wrote {{.*}} (0 function(s) offloaded
