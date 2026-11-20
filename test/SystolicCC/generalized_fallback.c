// REQUIRES: cfront-clang
//
// systolic-cc's Generalized GEMM fallback (docs/GENERALIZED_GEMM_SCOPE.md, "The fallback in
// systolic-cc"), on the software runtime model -- NOT the FPGA. gemm_plain is a GEMM the
// standard path lowers; gemm_perm reads A's rows in the order (5 i) mod 16, which has no
// affine view: the standard path finds no GEMM in it, and the fallback lowers it from its
// record by tables. Inputs/generalized_fallback.json is the records file systolic-cc writes
// for this file (PROGRAM.systolic/10-nest-records.json) when it runs
// generalized-gemm-nest-report itself; --nest-records reads it instead, so this test does not
// need the research build.
//
// RUN: %systolic-cc --fp-reassociation --explain --nest-records %S/Inputs/generalized_fallback.json %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// PLAN-DAG: generalized_fallback.c:[[#]]:1: note: 'gemm_perm' is offloaded: 5 systolic tile invocation(s)
// PLAN-DAG: generalized_fallback.c:[[#]]:1: note: 'gemm_plain' is offloaded: 5 systolic tile invocation(s)
// RUN: %t | FileCheck %s --check-prefix=OUT
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t | FileCheck %s --check-prefix=MOCK
// RUN: FileCheck %s --check-prefix=REMARKS --input-file=%t.systolic/20-remarks.txt
// RUN: grep -c 'generalized-fallback=true' %t.systolic/commands.txt | FileCheck %s --check-prefix=TWICE
//
// The two GEMMs run their tiles (the model reports CPU); the host loop in main is no GEMM.
// OUT-DAG: [systolic-cc] CPU Executed: GEMM #1
// OUT-DAG: [systolic-cc] CPU Executed: GEMM #2
// OUT-NOT: GEMM #3
// OUT: gemm_perm mismatches: 0 of 128
// OUT: gemm_plain mismatches: 0 of 128
// MOCK-DAG: [systolic-cc] FPGA Executed: GEMM #1
// MOCK-DAG: [systolic-cc] FPGA Executed: GEMM #2
// MOCK: gemm_perm mismatches: 0 of 128
// REMARKS: remark: GEMM 16x8x12 of nest 0 of gemm_perm{{.*}} lowered from its record as 5 exact region(s) (tables:
// TWICE: 2
//
// --no-generalized-gemm: as before the fallback; gemm_perm stays on the host and is right.
// RUN: %systolic-cc --fp-reassociation --explain --no-generalized-gemm %s -o %t.off 2>&1 | FileCheck %s --check-prefix=OFF
// RUN: %t.off | FileCheck %s --check-prefix=OFFOUT
// RUN: not grep -e 'generalized-fallback' -e 'static-extents' %t.off.systolic/commands.txt
// OFF: note: 'gemm_perm' is outside the supported C subset and is not translated
// OFF: note: 'gemm_plain' is offloaded: 5 systolic tile invocation(s)
// OFFOUT: gemm_perm mismatches: 0 of 128
// OFFOUT: gemm_plain mismatches: 0 of 128
//
// No tool and no records: the fallback is skipped, said with --explain, and the program is
// compiled as without it.
// RUN: env PATH=/usr/bin:/bin SYSTOLIC_NEST_REPORT=%t.none %systolic-cc --fp-reassociation --explain %s -o %t.skip 2>&1 | FileCheck %s --check-prefix=SKIP
// RUN: %t.skip | FileCheck %s --check-prefix=OFFOUT
// SKIP: note: the Generalized GEMM fallback is skipped: generalized-gemm-nest-report was not found
// SKIP: note: 'gemm_perm' stays on the host: no GEMM the systolic arrays can run was recognized in it
//
// RUN: not %systolic-cc --no-generalized-gemm --nest-records %S/Inputs/generalized_fallback.json %s -o %t.bad 2>&1 | FileCheck %s --check-prefix=BAD
// BAD: --nest-records is read by the Generalized GEMM fallback, which --no-generalized-gemm turns off

/* Generalized GEMM fallback demo: C += A[(5 i) mod 16][k] * B[k][j].
   The row order (5 i) mod 16 has no affine view, so the affine GEMM
   recognizer finds no GEMM; the research records verify it and the
   fallback lowers it by compile-time tables. gemm_plain is an ordinary
   GEMM the standard path takes. main checks both against a host loop. */
#include <stdio.h>

void gemm_perm(float A[restrict 16][12], float B[restrict 12][8],
               float C[restrict 16][8]) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 12; ++k)
        C[i][j] += A[(5 * i) % 16][k] * B[k][j];
}

void gemm_plain(float A[restrict 16][12], float B[restrict 12][8],
                float C[restrict 16][8]) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 12; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

static float A[16][12], B[12][8], C1[16][8], C2[16][8], R1[16][8], R2[16][8];

int main(void) {
  /* small integers: every sum is exact in any order, so results are bitwise */
  for (int i = 0; i < 16; ++i)
    for (int k = 0; k < 12; ++k)
      A[i][k] = (float)((i * 7 + k * 3) % 9 - 4);
  for (int k = 0; k < 12; ++k)
    for (int j = 0; j < 8; ++j)
      B[k][j] = (float)((k * 5 + j * 11) % 7 - 3);
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 8; ++j) {
      C1[i][j] = C2[i][j] = R1[i][j] = R2[i][j] = (float)(i - j);
    }
  gemm_perm(A, B, C1);
  gemm_plain(A, B, C2);
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 12; ++k) {
        /* one array read per product: the reference is no GEMM site */
        const float b = B[k][j];
        R1[i][j] += A[(5 * i) % 16][k] * b;
        R2[i][j] += A[i][k] * b;
      }
  int bad1 = 0, bad2 = 0;
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 8; ++j) {
      bad1 += C1[i][j] != R1[i][j];
      bad2 += C2[i][j] != R2[i][j];
    }
  printf("gemm_perm mismatches: %d of 128\n", bad1);
  printf("gemm_plain mismatches: %d of 128\n", bad2);
  return bad1 || bad2;
}
