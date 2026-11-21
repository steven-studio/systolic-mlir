// REQUIRES: cfront-clang
//
// One-level gathers through systolic-cc's Generalized GEMM fallback, on by default
// (docs/GENERALIZED_GEMM_G3B.md, "Experimental: one-level gathers"), on the software runtime
// model -- NOT the FPGA. gemm_gather is C[i][j] += A[dp[i]][k] * B[k][j], dp a read-only
// row index with repeated entries (MoE token ids): the front end reads A at the gathered row
// (systolic-cfront --gather-subscripts), the research tool verifies the nest
// (--admit-gather), and the pass packs A_packed[i][k] = A[dp[i]][k] before the dense tiles.
// Inputs/generalized_gather.json is the records file systolic-cc writes for this file
// (PROGRAM.systolic/10-nest-records.json) when it runs generalized-gemm-nest-report itself;
// --nest-records reads it instead, so this test does not need the research build.
//
// RUN: %systolic-cc --fp-reassociation --explain --nest-records %S/Inputs/generalized_gather.json %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// PLAN: generalized_gather.c:[[#]]:1: note: 'gemm_gather' is offloaded: 5 systolic tile invocation(s)
// RUN: %t | FileCheck %s --check-prefix=OUT
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t | FileCheck %s --check-prefix=MOCK
// RUN: FileCheck %s --check-prefix=REMARKS --input-file=%t.systolic/20-remarks.txt
// RUN: grep -c -e '--gather-subscripts' %t.systolic/commands.txt | FileCheck %s --check-prefix=FRONT
//
// The first call's dp holds rows of A only: the tiles run (the model reports CPU). The
// second call's dp names row 40 of a caller's array with 48 rows (C does not enforce the
// bound written, 32): the shim's check of dp sends that call to the function as written,
// on the CPU, and both results are bit for bit the host's.
// OUT: [systolic-cc] CPU Executed: GEMM #1
// OUT-NEXT: in range mismatches: 0 of 128
// OUT-NEXT: [systolic-cc] CPU Executed: GEMM #2
// OUT-NEXT: out of range mismatches: 0 of 128
// OUT-NOT: GEMM #3
// MOCK: [systolic-cc] FPGA Executed: GEMM #1
// MOCK-NEXT: in range mismatches: 0 of 128
// MOCK-NEXT: [systolic-cc] CPU Executed: GEMM #2
// MOCK-NEXT: out of range mismatches: 0 of 128
// REMARKS: remark: GEMM 16x8x12 of nest 0 of gemm_gather{{.*}} lowered from its record as 5 exact region(s) (formulas: {{.*}}; A or B packed by the record's gather before the tiles)
// FRONT: 2
//
// --no-admit-gather: the front end takes no integer array, so gemm_gather stays C, on the
// host, and is right.
// RUN: %systolic-cc --fp-reassociation --explain --no-admit-gather %s -o %t.off 2>&1 | FileCheck %s --check-prefix=OFF
// RUN: %t.off | FileCheck %s --check-prefix=OFFOUT
// RUN: not grep -e '--gather-subscripts' -e '--admit-gather' %t.off.systolic/commands.txt
// OFF: note: 'gemm_gather' is outside the supported C subset and is not translated
// OFFOUT: in range mismatches: 0 of 128
// OFFOUT: out of range mismatches: 0 of 128

/* C[i][j] += A[dp[i]][k] * B[k][j]: the rows of A in the order dp gives,
   with repeats, as a mixture-of-experts layer reads its tokens. main checks
   two calls against a host loop: one whose dp stays below the bound written
   for A, one whose dp goes past it into the caller's larger array. */
#include <stdint.h>
#include <stdio.h>

void gemm_gather(float A[restrict 32][12], const int32_t dp[restrict 16],
                 float B[restrict 12][8], float C[restrict 16][8]) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 12; ++k)
        C[i][j] += A[dp[i]][k] * B[k][j];
}

static float A[48][12], B[12][8], C[16][8], R[16][8];

static int check(const int32_t dp[16], const char *what) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 8; ++j)
      C[i][j] = R[i][j] = (float)(i - j);
  gemm_gather(A, dp, B, C);
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 12; ++k) {
        /* one array read per product: the reference is no GEMM site */
        const float b = B[k][j];
        R[i][j] += A[dp[i]][k] * b;
      }
  int bad = 0;
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 8; ++j)
      bad += C[i][j] != R[i][j];
  printf("%s mismatches: %d of 128\n", what, bad);
  return bad;
}

int main(void) {
  /* small integers: every sum is exact in any order, so results are bitwise */
  for (int r = 0; r < 48; ++r)
    for (int k = 0; k < 12; ++k)
      A[r][k] = (float)((r * 7 + k * 3) % 9 - 4);
  for (int k = 0; k < 12; ++k)
    for (int j = 0; j < 8; ++j)
      B[k][j] = (float)((k * 5 + j * 11) % 7 - 3);
  /* token ids with repeats, all rows of A[32] */
  static const int32_t in_range[16] = {3, 3, 17, 0, 31, 8, 8, 8, 25, 1, 17, 30, 2, 2, 19, 3};
  /* one id past the bound written: row 40 of the caller's 48 */
  static const int32_t out_of_range[16] = {3, 3, 17, 0, 31, 8, 8, 8, 25, 1, 17, 30, 2, 40, 19, 3};
  int bad = check(in_range, "in range");
  bad += check(out_of_range, "out of range");
  return bad != 0;
}
