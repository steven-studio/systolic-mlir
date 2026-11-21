// REQUIRES: cfront-clang
//
// Outputs at non-affine addresses through systolic-cc's Generalized GEMM fallback, on by
// default (docs/GENERALIZED_GEMM_G3B.md, "Experimental: outputs at non-affine addresses"), on
// the software runtime model -- NOT the FPGA. The front end writes C at any integer expression
// of the counters (systolic-cfront --scatter-subscripts), the research tool verifies the
// nests (--admit-gather), and the pass lowers each by one of two tracks:
//
//   scatter_perm     C[(3 i + 1) % 16][j]: a bijection of I's range onto C's rows, inverted
//                    in closed form at compile time ((11 m + 5) mod 16): A is packed by the
//                    inverse and C is written as a view. Nothing is built at run time.
//   scatter_lookup   C[dp1[dp2[i]]][j], two levels of tables, with repeated targets: every
//                    target computed and checked at run time, the tiles into a zeroed
//                    matrix, and its cells added into C through a two-layer inverse map
//                    (bucket offsets per cell of C, sources sorted by cell), each cell written
//                    once.
//   scatter_coupled  C[(i * i + j) % 16][i > j ? (i ^ j) & 7 : j / 2], a coupled address of
//                    (i, j) with collisions, by the same scatter-add.
//
// Inputs/generalized_non_affine_scatter.json is the records file systolic-cc writes for this
// file (PROGRAM.systolic/10-nest-records.json) when it runs generalized-gemm-nest-report
// itself; --nest-records reads it instead, so this test does not need the research build.
//
// RUN: %systolic-cc --fp-reassociation --explain --nest-records %S/Inputs/generalized_non_affine_scatter.json %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// PLAN-DAG: generalized_non_affine_scatter.c:[[#]]:1: note: 'scatter_perm' is offloaded: 5 systolic tile invocation(s)
// PLAN-DAG: generalized_non_affine_scatter.c:[[#]]:1: note: 'scatter_lookup' is offloaded: 5 systolic tile invocation(s)
// PLAN-DAG: generalized_non_affine_scatter.c:[[#]]:1: note: 'scatter_coupled' is offloaded: 5 systolic tile invocation(s)
// RUN: %t | FileCheck %s --check-prefix=OUT
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t | FileCheck %s --check-prefix=MOCK
// RUN: FileCheck %s --check-prefix=REMARKS --input-file=%t.systolic/20-remarks.txt
// RUN: grep -c -e '--scatter-subscripts' %t.systolic/commands.txt | FileCheck %s --check-prefix=FRONT
//
// The fourth call's dp1 names row 20 of a caller's array with 32 rows (C does not enforce the
// bound written, 16): the shim's check of the targets sends that call to the function as
// written, on the CPU. Every result is bit for bit the host's.
// OUT: [systolic-cc] CPU Executed: GEMM #1
// OUT-NEXT: perm mismatches: 0 of 256
// OUT-NEXT: [systolic-cc] CPU Executed: GEMM #2
// OUT-NEXT: lookup mismatches: 0 of 256
// OUT-NEXT: [systolic-cc] CPU Executed: GEMM #3
// OUT-NEXT: coupled mismatches: 0 of 256
// OUT-NEXT: [systolic-cc] CPU Executed: GEMM #4
// OUT-NEXT: lookup out of range mismatches: 0 of 256
// MOCK: [systolic-cc] FPGA Executed: GEMM #1
// MOCK-NEXT: perm mismatches: 0 of 256
// MOCK-NEXT: [systolic-cc] FPGA Executed: GEMM #2
// MOCK-NEXT: lookup mismatches: 0 of 256
// MOCK-NEXT: [systolic-cc] FPGA Executed: GEMM #3
// MOCK-NEXT: coupled mismatches: 0 of 256
// MOCK-NEXT: [systolic-cc] CPU Executed: GEMM #4
// MOCK-NEXT: lookup out of range mismatches: 0 of 256
// REMARKS-DAG: remark: GEMM 16x8x12 of nest 0 of scatter_perm{{.*}} lowered from its record as 5 exact region(s) (inverse: C's rows (3 i + 1) mod 16, a bijection, inverted in closed form (11 m + 5) mod 16
// REMARKS-DAG: remark: GEMM 16x8x12 of nest 0 of scatter_lookup{{.*}} lowered from its record as 5 exact region(s) (scatter-add: C's address re-emitted at run time through read-only tables, with collisions
// REMARKS-DAG: remark: GEMM 16x8x12 of nest 0 of scatter_coupled{{.*}} lowered from its record as 5 exact region(s) (scatter-add:
// FRONT: 2
//
// --no-general-scatter: the front end takes no general subscript of a written array:
// scatter_lookup and scatter_coupled stay C, on the host, and are right; scatter_perm's
// E % C is the front end's --modulo-subscripts, as before.
// RUN: %systolic-cc --fp-reassociation --explain --no-general-scatter %s -o %t.off 2>&1 | FileCheck %s --check-prefix=OFF
// RUN: %t.off | FileCheck %s --check-prefix=OFFOUT
// RUN: not grep -e '--scatter-subscripts' %t.off.systolic/commands.txt
// OFF-DAG: note: 'scatter_lookup' is outside the supported C subset and is not translated
// OFF-DAG: note: 'scatter_coupled' is outside the supported C subset and is not translated
// OFFOUT: perm mismatches: 0 of 256
// OFFOUT: lookup mismatches: 0 of 256
// OFFOUT: coupled mismatches: 0 of 256
// OFFOUT: lookup out of range mismatches: 0 of 256

/* GEMMs whose output rows are reached through non-affine addresses. main
   checks each against a host loop on a 32-row C: rows past 16 must stay
   untouched unless the program writes them. */
#include <stdint.h>
#include <stdio.h>

void scatter_perm(float A[restrict 16][12], float B[restrict 12][8],
                  float C[restrict 16][8]) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 12; ++k)
        C[(3 * i + 1) % 16][j] += A[i][k] * B[k][j];
}

void scatter_lookup(float A[restrict 16][12], const int32_t dp1[restrict 16],
                    const int32_t dp2[restrict 16], float B[restrict 12][8],
                    float C[restrict 16][8]) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 12; ++k)
        C[dp1[dp2[i]]][j] += A[i][k] * B[k][j];
}

void scatter_coupled(float A[restrict 16][12], float B[restrict 12][8],
                     float C[restrict 16][8]) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 12; ++k)
        C[(i * i + j) % 16][i > j ? (i ^ j) & 7 : j / 2] += A[i][k] * B[k][j];
}

static float A[16][12], B[12][8], C[32][8], R[32][8];

static void reset(void) {
  for (int r = 0; r < 32; ++r)
    for (int j = 0; j < 8; ++j)
      C[r][j] = R[r][j] = (float)(r - j);
}

static int compare(const char *what) {
  int bad = 0;
  for (int r = 0; r < 32; ++r)
    for (int j = 0; j < 8; ++j)
      bad += C[r][j] != R[r][j];
  printf("%s mismatches: %d of 256\n", what, bad);
  return bad;
}

int main(void) {
  /* small integers: every sum is exact in any order, so results are bitwise */
  for (int i = 0; i < 16; ++i)
    for (int k = 0; k < 12; ++k)
      A[i][k] = (float)((i * 7 + k * 3) % 9 - 4);
  for (int k = 0; k < 12; ++k)
    for (int j = 0; j < 8; ++j)
      B[k][j] = (float)((k * 5 + j * 11) % 7 - 3);
  int bad = 0;

  reset();
  scatter_perm(A, B, C);
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 12; ++k) {
        /* one array read per product: the reference is no GEMM site */
        const float b = B[k][j];
        R[(3 * i + 1) % 16][j] += A[i][k] * b;
      }
  bad += compare("perm");

  /* two levels of token tables, with repeated targets */
  static const int32_t dp2[16] = {0, 1, 1, 2, 3, 5, 8, 13, 0, 1, 2, 3, 4, 5, 6, 7};
  static const int32_t dp1[16] = {7, 7, 2, 2, 9, 9, 9, 0, 15, 15, 1, 4, 4, 4, 6, 3};
  reset();
  scatter_lookup(A, dp1, dp2, B, C);
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 12; ++k) {
        const float b = B[k][j];
        R[dp1[dp2[i]]][j] += A[i][k] * b;
      }
  bad += compare("lookup");

  reset();
  scatter_coupled(A, B, C);
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 12; ++k) {
        const float b = B[k][j];
        R[(i * i + j) % 16][i > j ? (i ^ j) & 7 : j / 2] += A[i][k] * b;
      }
  bad += compare("coupled");

  /* one target past the bound written: row 20 of the caller's 32 (dp2 reads dp1[4]) */
  static const int32_t far1[16] = {7, 7, 2, 2, 20, 9, 9, 0, 15, 15, 1, 4, 4, 4, 6, 3};
  reset();
  scatter_lookup(A, far1, dp2, B, C);
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 12; ++k) {
        const float b = B[k][j];
        R[far1[dp2[i]]][j] += A[i][k] * b;
      }
  bad += compare("lookup out of range");
  return bad != 0;
}
