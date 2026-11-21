// REQUIRES: cfront-clang
//
// Outputs whose address reads the inner counter k, through systolic-cc's Generalized GEMM
// fallback, on by default (docs/GENERALIZED_GEMM_G3B.md, "Experimental: outputs whose address
// reads k"), on the software runtime model -- NOT the FPGA. When C's address reads k, the
// products of one (i, j) no longer meet in one cell: the research verifies the fold over the
// pointwise family (P = i, j, k), and the pass computes every product A[i][k] * B[k][j] once,
// by a K = 1 GEMM per k on the systolic tiles, then adds the products into C through a
// two-layer inverse map (bucket offsets per cell of C, sources (k, i, j) sorted by cell), each
// cell written once. No closed-form inverse exists for these addresses, with or without a
// table:
//
//   k_square   C[i * i + k][j]               a polynomial, with collisions
//   k_cubic    C[(i * i * i + 2 * k) % 16][j] a cubic modulo 16, with collisions
//   k_lookup   C[dp[i + k]][j]               a table read at i + k, with repeated targets
//
// Inputs/generalized_k_scatter.json is the records file systolic-cc writes for this file
// (PROGRAM.systolic/10-nest-records.json) when it runs generalized-gemm-nest-report itself;
// --nest-records reads it instead, so this test does not need the research build.
//
// RUN: %systolic-cc --fp-reassociation --explain --nest-records %S/Inputs/generalized_k_scatter.json %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// PLAN-DAG: generalized_k_scatter.c:[[#]]:1: note: 'k_square' is offloaded: 12 systolic tile invocation(s)
// PLAN-DAG: generalized_k_scatter.c:[[#]]:1: note: 'k_cubic' is offloaded: 12 systolic tile invocation(s)
// PLAN-DAG: generalized_k_scatter.c:[[#]]:1: note: 'k_lookup' is offloaded: 12 systolic tile invocation(s)
// RUN: %t | FileCheck %s --check-prefix=OUT
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t | FileCheck %s --check-prefix=MOCK
// RUN: FileCheck %s --check-prefix=REMARKS --input-file=%t.systolic/20-remarks.txt
//
// The fourth call's dp names row 40 of a caller's array with 64 rows (C does not enforce the
// bound written, 16): the shim's check of the targets sends that call to the function as
// written, on the CPU. Every result is bit for bit the host's.
// OUT: [systolic-cc] CPU Executed: GEMM #1
// OUT-NEXT: square mismatches: 0 of 512
// OUT-NEXT: [systolic-cc] CPU Executed: GEMM #2
// OUT-NEXT: cubic mismatches: 0 of 512
// OUT-NEXT: [systolic-cc] CPU Executed: GEMM #3
// OUT-NEXT: lookup mismatches: 0 of 512
// OUT-NEXT: [systolic-cc] CPU Executed: GEMM #4
// OUT-NEXT: lookup out of range mismatches: 0 of 512
// MOCK: [systolic-cc] FPGA Executed: GEMM #1
// MOCK-NEXT: square mismatches: 0 of 512
// MOCK-NEXT: [systolic-cc] FPGA Executed: GEMM #2
// MOCK-NEXT: cubic mismatches: 0 of 512
// MOCK-NEXT: [systolic-cc] FPGA Executed: GEMM #3
// MOCK-NEXT: lookup mismatches: 0 of 512
// MOCK-NEXT: [systolic-cc] CPU Executed: GEMM #4
// MOCK-NEXT: lookup out of range mismatches: 0 of 512
// REMARKS-DAG: remark: GEMM 8x8x12 of nest 0 of k_square{{.*}} lowered from its record as 12 exact region(s) (product scatter-add: every product A[i][k] * B[k][j] by a K = 1 GEMM per k, C's address of (i, j, k) re-emitted at run time, with collisions
// REMARKS-DAG: remark: GEMM 8x8x12 of nest 0 of k_cubic{{.*}} lowered from its record as 12 exact region(s) (product scatter-add:
// REMARKS-DAG: remark: GEMM 8x8x12 of nest 0 of k_lookup{{.*}} lowered from its record as 12 exact region(s) (product scatter-add: {{.*}} through read-only tables, with collisions

/* GEMM-like nests whose output row reads k. main checks each against a
   host loop on a 64-row C: rows the program does not write stay as they
   are. */
#include <stdint.h>
#include <stdio.h>

void k_square(float A[restrict 8][12], float B[restrict 12][8],
              float C[restrict 64][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 12; ++k)
        C[i * i + k][j] += A[i][k] * B[k][j];
}

void k_cubic(float A[restrict 8][12], float B[restrict 12][8],
             float C[restrict 16][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 12; ++k)
        C[(i * i * i + 2 * k) % 16][j] += A[i][k] * B[k][j];
}

void k_lookup(float A[restrict 8][12], const int32_t dp[restrict 32],
              float B[restrict 12][8], float C[restrict 16][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 12; ++k)
        C[dp[i + k]][j] += A[i][k] * B[k][j];
}

static float A[8][12], B[12][8], C[64][8], R[64][8];

static void reset(void) {
  for (int r = 0; r < 64; ++r)
    for (int j = 0; j < 8; ++j)
      C[r][j] = R[r][j] = (float)(r - j);
}

static int compare(const char *what) {
  int bad = 0;
  for (int r = 0; r < 64; ++r)
    for (int j = 0; j < 8; ++j)
      bad += C[r][j] != R[r][j];
  printf("%s mismatches: %d of 512\n", what, bad);
  return bad;
}

int main(void) {
  /* small integers: every sum is exact in any order, so results are bitwise */
  for (int i = 0; i < 8; ++i)
    for (int k = 0; k < 12; ++k)
      A[i][k] = (float)((i * 7 + k * 3) % 9 - 4);
  for (int k = 0; k < 12; ++k)
    for (int j = 0; j < 8; ++j)
      B[k][j] = (float)((k * 5 + j * 11) % 7 - 3);
  int bad = 0;

  reset();
  k_square(A, B, C);
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 12; ++k) {
        /* one array read per product: the reference is no GEMM site */
        const float b = B[k][j];
        R[i * i + k][j] += A[i][k] * b;
      }
  bad += compare("square");

  reset();
  k_cubic(A, B, C);
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 12; ++k) {
        const float b = B[k][j];
        R[(i * i * i + 2 * k) % 16][j] += A[i][k] * b;
      }
  bad += compare("cubic");

  /* token ids with repeats, all rows of C[16] */
  int32_t dp[32];
  for (int x = 0; x < 32; ++x)
    dp[x] = (x * 5) % 7 + (x % 3) * 3;
  reset();
  k_lookup(A, dp, B, C);
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 12; ++k) {
        const float b = B[k][j];
        R[dp[i + k]][j] += A[i][k] * b;
      }
  bad += compare("lookup");

  /* one target past the bound written: row 40 of the caller's 64 */
  dp[9] = 40;
  reset();
  k_lookup(A, dp, B, C);
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 12; ++k) {
        const float b = B[k][j];
        R[dp[i + k]][j] += A[i][k] * b;
      }
  bad += compare("lookup out of range");
  return bad != 0;
}
