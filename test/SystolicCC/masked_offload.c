// REQUIRES: cfront-clang
//
// Phase 5.4.3d-3b-7 (docs/AFFINE_GEMM_MAPPING.md section 28): non-box
// iteration domains whose reduction fibers are complete run as dense device
// jobs into a zero temporary, then an exact output mask adds the selected
// elements into C; the other elements of C are never touched. Software
// runtime model, integer data (the relaxed contract is bitwise then), C
// nonzero before every call.
//
// The pass's default order reaches these nests through the guarded path
// (the fast path refuses non-box domains), so no test control is needed.
// RUN: %systolic-cc --fp-reassociation --verify --explain --keep-dir %t.keep %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// RUN: FileCheck %s --check-prefix=REMARKS --input-file=%t.keep/20-remarks.txt
// RUN: %t > %t.out 2> %t.err
// RUN: FileCheck %s --check-prefix=VERIFY --input-file=%t.err
// RUN: %host-cc -std=c11 -O0 -ffp-contract=off %s -o %t.cpu
// RUN: %t.cpu > %t.cpu.out
// RUN: grep -v '^\[systolic-cc\]' %t.out | diff %t.cpu.out -
//
// Strict floating point: nothing is offloaded.
// RUN: %systolic-cc --verify --explain %s -o %t.strict 2>&1 | FileCheck %s --check-prefix=STRICT
// RUN: %t.strict > %t.strict.out 2> /dev/null
// RUN: grep -v '^\[systolic-cc\]' %t.strict.out | diff %t.cpu.out -

#include <stdio.h>

// The upper triangle j >= i.
void upper(float A[restrict 8][8], float B[restrict 8][8],
           float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = i; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

// The strict lower triangle j < i.
void lower_strict(float A[restrict 8][8], float B[restrict 8][8],
                  float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < i; ++j)
      for (int k = 0; k < 8; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

// The anti-diagonal triangle i + j < 8.
void anti(float A[restrict 8][8], float B[restrict 8][8],
          float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8 - i; ++j)
      for (int k = 0; k < 8; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

// Nonzero bounds and constant offsets inside 8 x 8 arrays.
void offsets(float A[restrict 8][8], float B[restrict 8][8],
             float C[restrict 8][8]) {
  for (int i = 2; i < 6; ++i)
    for (int j = i; j < 7; ++j)
      for (int k = 1; k < 5; ++k)
        C[i + 1][j + 1] += A[i + 1][k] * B[k][j + 1];
}

// Two masked nests in one function.
void two_nests(float A[restrict 4][4], float B[restrict 4][4],
               float C[restrict 4][4], float D[restrict 4][4]) {
  for (int i = 0; i < 4; ++i)
    for (int j = i; j < 4; ++j)
      for (int k = 0; k < 4; ++k)
        C[i][j] += A[i][k] * B[k][j];
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < i; ++j)
      for (int k = 0; k < 4; ++k)
        D[i][j] += A[i][k] * B[k][j];
}

// A skewed triangle, C[i + j][j] for j < 4 - i, on an array parameter with a
// dynamic leading dimension: the dense box is tightened to the four rows the
// nest reads (review finding of 2026-10-29, section 28.2).
void skew_dynamic(float A[restrict][4], float B[restrict 4][4],
                  float C[restrict 8][4]) {
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4 - i; ++j)
      for (int k = 0; k < 4; ++k)
        C[i + j][j] += A[i + j][k] * B[k][j];
}

// K pinned to i (k = i only): every output's fiber is the single value i, a
// proper part of the K range, so the certified lowering refuses the mask;
// the fast path normalizes the parallelotope (x = U x', section 19.12) and
// lowers the nest as a batch loop over i of 1 x 4 x 1 tiles,
// C[i][0..4) += A[i][i] x B[i][0..4): one product per element, as written.
// Verified bitwise against the CPU reference below, with nonzero C.
void k_equals_i(float A[restrict 4][4], float B[restrict 4][4],
                float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      for (int k = i; k < i + 1; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

// A reduction bound depending on i: partial fibers, refused; runs on the host.
void k_from_i(float A[restrict 4][4], float B[restrict 4][4],
              float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      for (int k = 0; k <= i; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

static float A8[8][8], B8[8][8], CU[8][8], CL[8][8], CA[8][8], CO[8][8];
static float A4[4][4], B4[4][4], C4[4][4], D4[4][4], E4[4][4];
static float S4[4][4], T4[4][4], U8[8][4], P4[4][4];

int main(void) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j) {
      A8[i][j] = (float)(((i * 7 + j * 3) % 9 + 1) * ((i + j) % 2 ? -1 : 1));
      B8[i][j] = (float)(((i * 5 + j * 11) % 7 + 1) * ((i + 2 * j) % 3 ? 1 : -1));
      CU[i][j] = (float)((i + 2 * j) % 5 - 2);
      CL[i][j] = (float)(i * 8 + j);
      CA[i][j] = (float)(j - i);
      CO[i][j] = (float)(3 - i + j);
    }
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j) {
      A4[i][j] = (float)(i + j + 1);
      B4[i][j] = (float)(i - j - 1 == 0 ? 2 : i - j - 1);
      C4[i][j] = (float)(i * j - 2);
      D4[i][j] = (float)(j - i + 1);
      E4[i][j] = (float)(2 * i - j);
      S4[i][j] = (float)((i * 3 + j) % 5 + 1);
      P4[i][j] = (float)(3 * i - 2 * j + 1);
      T4[i][j] = (float)((i + 2 * j) % 3 - 2 == 0 ? 3 : (i + 2 * j) % 3 - 2);
    }
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 4; ++j)
      U8[i][j] = (float)(i - 2 * j);

  upper(A8, B8, CU);
  lower_strict(A8, B8, CL);
  anti(A8, B8, CA);
  offsets(A8, B8, CO);
  two_nests(A4, B4, C4, D4);
  skew_dynamic(S4, T4, U8);
  k_equals_i(A4, B4, P4);
  k_from_i(A4, B4, E4);

  double su = 0, sl = 0, sa = 0, so = 0, sc = 0, sd = 0, se = 0, sk = 0,
         sp = 0;
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j) {
      su += CU[i][j] * (i + 1);
      sl += CL[i][j] * (j + 1);
      sa += CA[i][j] * (i + j + 1);
      so += CO[i][j] * (2 * i + 1);
    }
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j) {
      sc += C4[i][j] * (i + 1);
      sd += D4[i][j] * (j + 1);
      se += E4[i][j];
      sp += P4[i][j] * (i + j + 1);
    }
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 4; ++j)
      sk += U8[i][j] * (i + 1);
  printf("%.1f %.1f %.1f %.1f %.1f %.1f %.1f %.1f %.1f %g %g %g %g %g\n", su,
         sl, sa, so, sc, sd, se, sk, sp, CU[7][0], CL[0][7], CA[7][7],
         CO[0][0], P4[3][3]);
  return 0;
}

// PLAN-DAG: note: 'upper' is offloaded: {{[0-9]+}} systolic tile invocation(s)
// PLAN-DAG: note: 'lower_strict' is offloaded: {{[0-9]+}} systolic tile invocation(s)
// PLAN-DAG: note: 'anti' is offloaded: {{[0-9]+}} systolic tile invocation(s)
// PLAN-DAG: note: 'offsets' is offloaded: {{[0-9]+}} systolic tile invocation(s)
// PLAN-DAG: note: 'two_nests' is offloaded: {{[0-9]+}} systolic tile invocation(s)
// PLAN-DAG: note: 'skew_dynamic' is offloaded: {{[0-9]+}} systolic tile invocation(s)
// PLAN-DAG: note: 'k_equals_i' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 4 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'k_from_i' stays on the host
// PLAN:     wrote {{.*}} (7 function(s) offloaded, runtime: software model, NOT the FPGA
//
// REMARKS-DAG: remark: affine GEMM 8x8x8 lowered as {{[0-9]+}} exact region(s) by the certified device-job lowering (docs/AFFINE_GEMM_MAPPING.md section 27) with an exact output mask over the non-box domain (section 28)
// REMARKS-DAG: remark: affine GEMM 4x5x4 lowered as {{[0-9]+}} exact region(s) by the certified device-job lowering (docs/AFFINE_GEMM_MAPPING.md section 27) with an exact output mask over the non-box domain (section 28)
// REMARKS-DAG: remark: affine GEMM 4x4x4 lowered as {{[0-9]+}} exact region(s) by the certified device-job lowering (docs/AFFINE_GEMM_MAPPING.md section 27) with an exact output mask over the non-box domain (section 28)
// REMARKS-DAG: remark: certified device offload not applied: UNKNOWN: the reduction fiber of an output coordinate is a proper part of the K range
// REMARKS-DAG: remark: affine GEMM 1x4x1 lowered as 1 exact region(s) on 4 accelerator(s): makespan = {{[0-9]+}} cycles (read + compute + write), 1 output block(s), batched: 4 batch iteration(s) of 1 batch loop(s)
//
// VERIFY-DAG: upper ({{.*}}): verified: 64 elements equal the CPU reference (bitwise)
// VERIFY-DAG: lower_strict ({{.*}}): verified: 64 elements equal the CPU reference (bitwise)
// VERIFY-DAG: anti ({{.*}}): verified: 64 elements equal the CPU reference (bitwise)
// VERIFY-DAG: offsets ({{.*}}): verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY-DAG: two_nests ({{.*}}): verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY-DAG: skew_dynamic ({{.*}}): verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY-DAG: k_equals_i ({{.*}}): verified: 16 elements equal the CPU reference (bitwise)
// VERIFY-NOT: MISMATCH
//
// STRICT-DAG: note: 'upper' stays on the host
// STRICT-DAG: note: 'two_nests' stays on the host
// STRICT:     wrote {{.*}} (0 function(s) offloaded
