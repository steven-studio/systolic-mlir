// REQUIRES: cfront-clang
//
// Phase 5.4.3d-3b-8 M1 (docs/AFFINE_GEMM_MAPPING.md section 29): reduction
// fibers that vary with the output coordinate. Under the option (the
// test-only environment variable mirrors the pass option
// common-fiber-offload; the pass's default order is unchanged) each output
// block of the tile plan runs its common reduction fiber on the device and
// the remaining contributions run on the host; every product of the source
// is accumulated exactly once. Software runtime model, integer data (the
// relaxed contract is bitwise then), C nonzero before every call.
//
// RUN: env SYSTOLIC_CC_TEST_COMMON_FIBER_OFFLOAD=1 %systolic-cc --fp-reassociation --verify --explain --keep-dir %t.keep %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// RUN: FileCheck %s --check-prefix=REMARKS --input-file=%t.keep/20-remarks.txt
// RUN: %t > %t.out 2> %t.err
// RUN: FileCheck %s --check-prefix=VERIFY --input-file=%t.err
// RUN: %host-cc -std=c11 -O0 -ffp-contract=off %s -o %t.cpu
// RUN: %t.cpu > %t.cpu.out
// RUN: grep -v '^\[systolic-cc\]' %t.out | diff %t.cpu.out -
//
// Without the option these nests stay on the host, as before this phase.
// RUN: %systolic-cc --fp-reassociation --explain %s -o %t.off 2>&1 | FileCheck %s --check-prefix=OFF
// RUN: %t.off > %t.off.out 2> /dev/null
// RUN: grep -v '^\[systolic-cc\]' %t.off.out | diff %t.cpu.out -
//
// Strict floating point: nothing is offloaded.
// RUN: env SYSTOLIC_CC_TEST_COMMON_FIBER_OFFLOAD=1 %systolic-cc --verify --explain %s -o %t.strict 2>&1 | FileCheck %s --check-prefix=STRICT
// RUN: %t.strict > %t.strict.out 2> /dev/null
// RUN: grep -v '^\[systolic-cc\]' %t.strict.out | diff %t.cpu.out -

#include <stdio.h>

// Case A: 0 <= k <= i.
void k_le_i(float A[restrict 8][8], float B[restrict 8][8],
            float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k <= i; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

// Case B: 0 <= k <= i + j (K up to 14 inside 16).
void k_le_i_plus_j(float A[restrict 8][16], float B[restrict 16][8],
                   float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k <= i + j; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

// Case C: k >= j.
void k_ge_j(float A[restrict 8][8], float B[restrict 8][8],
            float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = j; k < 8; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

// Case D: j <= k <= i. On the 8 x 8 output every block of the plan holds a
// cell with an empty fiber, so no block has a common fiber: UNKNOWN, host.
void j_le_k_le_i(float A[restrict 8][8], float B[restrict 8][8],
                 float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = j; k <= i; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

// Nonzero lower bounds and shifted coordinates: C[i+1][j+2] over
// i in [2, 7), j in [1, 6), k in [1, i].
void shifted(float A[restrict 8][8], float B[restrict 8][8],
             float C[restrict 8][8]) {
  for (int i = 2; i < 7; ++i)
    for (int j = 1; j < 6; ++j)
      for (int k = 1; k <= i; ++k)
        C[i + 1][j + 2] += A[i + 1][k] * B[k][j + 2];
}

static float A8[8][8], B8[8][8], A16[8][16], B16[16][8];
static float CA[8][8], CB[8][8], CC[8][8], CD[8][8], CS[8][8];

int main(void) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j) {
      A8[i][j] = (float)(((i * 7 + j * 3) % 9 + 1) * ((i + j) % 2 ? -1 : 1));
      B8[i][j] = (float)(((i * 5 + j * 11) % 7 + 1) * ((i + 2 * j) % 3 ? 1 : -1));
      CA[i][j] = (float)((i + 2 * j) % 5 - 2);
      CB[i][j] = (float)(i * 8 + j);
      CC[i][j] = (float)(j - i);
      CD[i][j] = (float)(3 - i + j);
      CS[i][j] = (float)(2 * i - j + 1);
    }
  for (int i = 0; i < 8; ++i)
    for (int k = 0; k < 16; ++k)
      A16[i][k] = (float)(((i * 3 + k) % 7 + 1) * ((i + k) % 2 ? 1 : -1));
  for (int k = 0; k < 16; ++k)
    for (int j = 0; j < 8; ++j)
      B16[k][j] = (float)(((k * 2 + j * 5) % 6 + 1) * ((k + j) % 3 ? -1 : 1));

  k_le_i(A8, B8, CA);
  k_le_i_plus_j(A16, B16, CB);
  k_ge_j(A8, B8, CC);
  j_le_k_le_i(A8, B8, CD);
  shifted(A8, B8, CS);

  double sa = 0, sb = 0, sc = 0, sd = 0, ss = 0;
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j) {
      sa += CA[i][j] * (i + 1);
      sb += CB[i][j] * (j + 1);
      sc += CC[i][j] * (i + j + 1);
      sd += CD[i][j] * (2 * i + 1);
      ss += CS[i][j] * (i - j);
    }
  printf("%.1f %.1f %.1f %.1f %.1f %g %g %g %g\n", sa, sb, sc, sd, ss,
         CA[7][0], CB[7][7], CC[0][7], CS[3][3]);
  return 0;
}

// PLAN-DAG: note: 'k_le_i' is offloaded: {{[0-9]+}} systolic tile invocation(s)
// PLAN-DAG: note: 'k_le_i_plus_j' is offloaded: {{[0-9]+}} systolic tile invocation(s)
// PLAN-DAG: note: 'k_ge_j' is offloaded: {{[0-9]+}} systolic tile invocation(s)
// PLAN-DAG: note: 'j_le_k_le_i' stays on the host
// PLAN-DAG: note: 'shifted' is offloaded: {{[0-9]+}} systolic tile invocation(s)
// PLAN:     wrote {{.*}} (4 function(s) offloaded, runtime: software model, NOT the FPGA
//
// REMARKS-DAG: remark: affine GEMM 8x8x8 lowered as {{[0-9]+}} exact region(s) by the certified device-job lowering (docs/AFFINE_GEMM_MAPPING.md section 27) with the common reduction fibers of {{[0-9]+}} output block(s) on the device ({{[0-9]+}} of 288 MACs) and the remainder on the host ({{[0-9]+}} MACs in {{[0-9]+}} guarded nest(s)) (section 29)
// REMARKS-DAG: remark: affine GEMM 8x8x15 lowered as {{[0-9]+}} exact region(s) by the certified device-job lowering (docs/AFFINE_GEMM_MAPPING.md section 27) with the common reduction fibers of {{[0-9]+}} output block(s) on the device ({{[0-9]+}} of 512 MACs) and the remainder on the host
// REMARKS-DAG: remark: certified device offload not applied: UNKNOWN: no output block has a common reduction fiber: nothing for the device
//
// VERIFY-DAG: k_le_i ({{.*}}): verified: 64 elements equal the CPU reference (bitwise)
// VERIFY-DAG: k_le_i_plus_j ({{.*}}): verified: 64 elements equal the CPU reference (bitwise)
// VERIFY-DAG: k_ge_j ({{.*}}): verified: 64 elements equal the CPU reference (bitwise)
// VERIFY-DAG: shifted ({{.*}}): verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY-NOT: MISMATCH
//
// OFF-DAG: note: 'k_le_i' stays on the host
// OFF-DAG: note: 'k_le_i_plus_j' stays on the host
// OFF-DAG: note: 'k_ge_j' stays on the host
// OFF-DAG: note: 'shifted' stays on the host
// OFF:     wrote {{.*}} (0 function(s) offloaded
//
// STRICT-DAG: note: 'k_le_i' stays on the host
// STRICT:     wrote {{.*}} (0 function(s) offloaded
