// REQUIRES: cfront-clang
//
// GEMMs at any depth of a C loop nest, through the whole compiler: 3 to 8
// loops, I/J/K on loops at different levels, coordinates split over two
// loops (tiled nests), loops that repeat the GEMM outside it or between its
// loops (moved outward: a legal interchange), one inside its reduction (not
// legal: stays on the CPU), and nests that are not GEMMs. Every case is
// checked against an independent CPU reference (products through a call,
// repetitions applied explicitly), with the invocations the runtime made.
//
// NO REAL HARDWARE HERE: the software runtime model computes every tile.
// SOFT_RUNTIME_MOCK_HARDWARE=1 makes it claim to be the FPGA (a test
// stand-in for a successful board run); SOFT_RUNTIME_FAULT=fail makes one
// invocation fail.
//
// RUN: %systolic-cc --explain %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// PLAN-DAG: note: 'd3' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'd4_outer' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 2 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'd4_mid' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 3 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'd4_illegal' stays on the host: no GEMM the systolic arrays can run was recognized in it (line {{[0-9]+}}: loop 3 of the nest repeats the accumulation inside it, and moving it outward is not a legal loop interchange (it would reorder dependent iterations))
// PLAN-DAG: note: 'd4_splitk' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'd5_tiled' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'd6_tiled' is offloaded: 20 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'd7' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 6 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'd8' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 4 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'ng_ewise5' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// PLAN-DAG: note: 'ng_batched' stays on the host: no GEMM the systolic arrays can run was recognized in it (line {{[0-9]+}}: batched GEMM (a partition prefix P* of 1 coordinates) is not supported)
// PLAN-DAG: note: 'ng_skew' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// PLAN-DAG: note: 'ng_matvec4' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// PLAN:     wrote {{.*}} (8 function(s) offloaded
//
// The software model is not the FPGA: every GEMM reports CPU; the
// invocations are the same.
// RUN: %t > %t.soft
// RUN: FileCheck %s --check-prefixes=SOFT,ALL --input-file=%t.soft --match-full-lines
// SOFT-NOT: FPGA
//
// The FPGA (mocked) runs every offloaded GEMM.
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t > %t.mock
// RUN: FileCheck %s --check-prefixes=MOCK,ALL --input-file=%t.mock --match-full-lines
//
// ALL:        depth test
// SOFT-NEXT:  [systolic-cc] CPU Executed: GEMM #1
// MOCK-NEXT:  [systolic-cc] FPGA Executed: GEMM #1
// ALL-NEXT:   [case] d3: equal to the reference, 1 invocation(s)
// SOFT-NEXT:  [systolic-cc] CPU Executed: GEMM #2
// SOFT-NEXT:  [systolic-cc] CPU Executed: GEMM #3
// MOCK-NEXT:  [systolic-cc] FPGA Executed: GEMM #2
// MOCK-NEXT:  [systolic-cc] FPGA Executed: GEMM #3
// ALL-NEXT:   [case] d4_outer: equal to the reference, 2 invocation(s)
// SOFT-NEXT:  [systolic-cc] CPU Executed: GEMM #4
// MOCK-NEXT:  [systolic-cc] FPGA Executed: GEMM #4
// ALL-NEXT:   [case] d4_mid: equal to the reference, 3 invocation(s)
// ALL-NEXT:   [systolic-cc] CPU Executed: GEMM #5
// ALL-NEXT:   [case] d4_illegal: equal to the reference, 0 invocation(s)
// SOFT-NEXT:  [systolic-cc] CPU Executed: GEMM #6
// MOCK-NEXT:  [systolic-cc] FPGA Executed: GEMM #6
// ALL-NEXT:   [case] d4_splitk: equal to the reference, 1 invocation(s)
// SOFT-NEXT:  [systolic-cc] CPU Executed: GEMM #7
// MOCK-NEXT:  [systolic-cc] FPGA Executed: GEMM #7
// ALL-NEXT:   [case] d5_tiled: equal to the reference, 1 invocation(s)
// SOFT-NEXT:  [systolic-cc] CPU Executed: GEMM #8
// MOCK-NEXT:  [systolic-cc] FPGA Executed: GEMM #8
// ALL-NEXT:   [case] d6_tiled: equal to the reference, 20 invocation(s)
// SOFT-NEXT:  [systolic-cc] CPU Executed: GEMM #9
// SOFT-NEXT:  [systolic-cc] CPU Executed: GEMM #10
// MOCK-NEXT:  [systolic-cc] FPGA Executed: GEMM #9
// MOCK-NEXT:  [systolic-cc] FPGA Executed: GEMM #10
// ALL-NEXT:   [case] d7: equal to the reference, 6 invocation(s)
// SOFT-NEXT:  [systolic-cc] CPU Executed: GEMM #11
// SOFT-NEXT:  [systolic-cc] CPU Executed: GEMM #12
// MOCK-NEXT:  [systolic-cc] FPGA Executed: GEMM #11
// MOCK-NEXT:  [systolic-cc] FPGA Executed: GEMM #12
// ALL-NEXT:   [case] d8: equal to the reference, 4 invocation(s)
// ALL-NEXT:   [case] ng_ewise5: equal to the reference, 0 invocation(s)
// ALL-NEXT:   [systolic-cc] CPU Executed: GEMM #13
// ALL-NEXT:   [case] ng_batched: equal to the reference, 0 invocation(s)
// ALL-NEXT:   [systolic-cc] CPU Executed: GEMM #14
// ALL-NEXT:   [case] ng_skew: equal to the reference, 0 invocation(s)
// ALL-NEXT:   [case] ng_matvec4: equal to the reference, 0 invocation(s)
// ALL-NEXT:   PASS: 0 case(s) differ
// ALL-EMPTY:
//
// Invocation 30 fails -- the third of d7's six (0-27 are the cases
// before it): the two tiles already accumulated into C are undone,
// d7 runs on the CPU -- both of its executions -- with the right result,
// and the program goes on on the FPGA.
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 SOFT_RUNTIME_FAULT=fail SOFT_RUNTIME_FAULT_AT=30 %t > %t.f30
// RUN: FileCheck %s --check-prefix=F30 --input-file=%t.f30 --match-full-lines
// F30:      [case] d6_tiled: equal to the reference, 20 invocation(s)
// F30-NEXT: [systolic-cc] CPU Executed: GEMM #9
// F30-NEXT: [systolic-cc] CPU Executed: GEMM #10
// F30-NEXT: [case] d7: equal to the reference, 5 invocation(s)
// F30-NEXT: [systolic-cc] FPGA Executed: GEMM #11
// F30-NEXT: [systolic-cc] FPGA Executed: GEMM #12
// F30:      PASS: 0 case(s) differ
//
// d4_mid's second repetition fails (invocation 4) after the first one
// accumulated into C: undone, recomputed on the CPU.
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 SOFT_RUNTIME_FAULT=fail SOFT_RUNTIME_FAULT_AT=4 %t > %t.f4
// RUN: FileCheck %s --check-prefix=F4 --input-file=%t.f4 --match-full-lines
// F4:      [systolic-cc] CPU Executed: GEMM #4
// F4-NEXT: [case] d4_mid: equal to the reference, 2 invocation(s)
// F4:      PASS: 0 case(s) differ
//
// With --verify every offloaded call is also compared with the function as
// written, run on the CPU.
// RUN: %systolic-cc --verify %s -o %t.v > /dev/null 2>&1
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t.v 2>&1 >/dev/null | FileCheck %s --check-prefix=VERIFY
// VERIFY-COUNT-8: verified:
// VERIFY-NOT:     MISMATCH

#include <stdio.h>
#include <string.h>

#include "systolic_offload.h"

/* ---- GEMMs, from 3 to 8 loops ---------------------------------------- */

/* 3 loops. */
void d3(float A[restrict 8][8], float B[restrict 8][8], float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

/* 4 loops: an outer loop runs the GEMM twice. */
void d4_outer(float A[restrict 8][8], float B[restrict 8][8], float C[restrict 8][8]) {
  for (int r = 0; r < 2; ++r)
    for (int i = 0; i < 8; ++i)
      for (int j = 0; j < 8; ++j)
        for (int k = 0; k < 8; ++k)
          C[i][j] += A[i][k] * B[k][j];
}

/* 4 loops: a repeating loop between J and K (I, J on loops 1, 2; K on 4):
   moving it outward is a legal interchange. */
void d4_mid(float A[restrict 8][8], float B[restrict 8][8], float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int r = 0; r < 3; ++r)
        for (int k = 0; k < 8; ++k)
          C[i][j] += A[i][k] * B[k][j];
}

/* 4 loops: a repeating loop inside the reduction: moving it outward would
   reorder dependent iterations -- not legal, stays on the CPU. */
void d4_illegal(float A[restrict 8][8], float B[restrict 8][8], float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        for (int r = 0; r < 2; ++r)
          C[i][j] += A[i][k] * B[k][j];
}

/* 4 loops: K over two loops, K = 8 * k0 + k1. */
void d4_splitk(float A[restrict 8][16], float B[restrict 16][8], float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k0 = 0; k0 < 2; ++k0)
        for (int k1 = 0; k1 < 8; ++k1)
          C[i][j] += A[i][8 * k0 + k1] * B[8 * k0 + k1][j];
}

/* 5 loops: I and J tiled, I = 4 * i0 + i1, J = 4 * j0 + j1. */
void d5_tiled(float A[restrict 8][8], float B[restrict 8][8], float C[restrict 8][8]) {
  for (int i0 = 0; i0 < 2; ++i0)
    for (int j0 = 0; j0 < 2; ++j0)
      for (int i1 = 0; i1 < 4; ++i1)
        for (int j1 = 0; j1 < 4; ++j1)
          for (int k = 0; k < 8; ++k)
            C[4 * i0 + i1][4 * j0 + j1] += A[4 * i0 + i1][k] * B[k][4 * j0 + j1];
}

/* 6 loops: I, J, K all tiled. */
void d6_tiled(float A[restrict 16][24], float B[restrict 24][16], float C[restrict 16][16]) {
  for (int i0 = 0; i0 < 2; ++i0)
    for (int j0 = 0; j0 < 4; ++j0)
      for (int k0 = 0; k0 < 3; ++k0)
        for (int i1 = 0; i1 < 8; ++i1)
          for (int j1 = 0; j1 < 4; ++j1)
            for (int k1 = 0; k1 < 8; ++k1)
              C[8 * i0 + i1][4 * j0 + j1] +=
                  A[8 * i0 + i1][8 * k0 + k1] * B[8 * k0 + k1][4 * j0 + j1];
}

/* 7 loops: r0 outside (2 executions), r1 between J and I1 (3 repetitions
   moved outward), I = 4 * i0 + i1, K = 8 * k0 + k1. */
void d7(float A[restrict 8][16], float B[restrict 16][8], float C[restrict 8][8]) {
  for (int r0 = 0; r0 < 2; ++r0)
    for (int i0 = 0; i0 < 2; ++i0)
      for (int j = 0; j < 8; ++j)
        for (int r1 = 0; r1 < 3; ++r1)
          for (int i1 = 0; i1 < 4; ++i1)
            for (int k0 = 0; k0 < 2; ++k0)
              for (int k1 = 0; k1 < 8; ++k1)
                C[4 * i0 + i1][j] += A[4 * i0 + i1][8 * k0 + k1] * B[8 * k0 + k1][j];
}

/* 8 loops: r0 outside, r1 between J0 and K0; I, J, K tiled. */
void d8(float A[restrict 8][8], float B[restrict 8][8], float C[restrict 8][8]) {
  for (int r0 = 0; r0 < 2; ++r0)
    for (int i0 = 0; i0 < 2; ++i0)
      for (int j0 = 0; j0 < 2; ++j0)
        for (int r1 = 0; r1 < 2; ++r1)
          for (int k0 = 0; k0 < 2; ++k0)
            for (int i1 = 0; i1 < 4; ++i1)
              for (int j1 = 0; j1 < 4; ++j1)
                for (int k1 = 0; k1 < 4; ++k1)
                  C[4 * i0 + i1][4 * j0 + j1] +=
                      A[4 * i0 + i1][4 * k0 + k1] * B[4 * k0 + k1][4 * j0 + j1];
}

/* ---- not GEMMs the systolic arrays run ------------------------------- */

/* 5 loops, element-wise: no reduction, not a GEMM. */
void ng_ewise5(float X[restrict 2][2][2][2][2], float Y[restrict 2][2][2][2][2]) {
  for (int a = 0; a < 2; ++a)
    for (int b = 0; b < 2; ++b)
      for (int c = 0; c < 2; ++c)
        for (int d = 0; d < 2; ++d)
          for (int e = 0; e < 2; ++e)
            X[a][b][c][d][e] += Y[a][b][c][d][e] * Y[a][b][c][d][e];
}

/* 4 loops, batched (rank-3 arrays): recognized, not lowered. */
void ng_batched(float A[restrict 2][4][4], float B[restrict 2][4][4], float C[restrict 2][4][4]) {
  for (int b = 0; b < 2; ++b)
    for (int i = 0; i < 4; ++i)
      for (int j = 0; j < 4; ++j)
        for (int k = 0; k < 4; ++k)
          C[b][i][j] += A[b][i][k] * B[b][k][j];
}

/* 3 loops, A[i + j][k] * B[k][2 * j] (in bounds): not a GEMM of A and B. */
void ng_skew(float A[restrict 15][8], float B[restrict 8][16], float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        C[i][j] += A[i + j][k] * B[k][2 * j];
}

/* 4 loops around a matrix-vector product: two dimensions only. */
void ng_matvec4(float M[restrict 8][8], float x[restrict 8], float y[restrict 8]) {
  for (int r = 0; r < 2; ++r)
    for (int s = 0; s < 2; ++s)
      for (int i = 0; i < 8; ++i)
        for (int j = 0; j < 8; ++j)
          y[i] += M[i][j] * x[j];
}

/* ---- the independent reference ---------------------------------------- */

/* C[i][j] += times * sum_k A(i, k) * B(k, j), with A(i, k) = A[i * lda + k]
   etc.; each product through a call (not a GEMM statement). */
static float mul(float a, float b) { return a * b; }
static void ref(int M, int N, int K, const float *A, int lda, const float *B,
                int ldb, float *C, int ldc, int times) {
  for (int t = 0; t < times; ++t)
    for (int i = 0; i < M; ++i)
      for (int j = 0; j < N; ++j)
        for (int k = 0; k < K; ++k)
          C[i * ldc + j] = C[i * ldc + j] + mul(A[i * lda + k], B[k * ldb + j]);
}

static float A8[8][8], B8[8][8], A16[8][16], B16[16][8], Ab[16][24], Bb[24][16];
static float C8[8][8], R8[8][8], C16[16][16], R16[16][16];
static float X5[2][2][2][2][2], Y5[2][2][2][2][2], R5[2][2][2][2][2];
static float A3[2][4][4], B3[2][4][4], C3[2][4][4], R3[2][4][4];
static float As[15][8], Bs[8][16], M8[8][8], x8[8], y8[8], ry8[8];

static int failures;
static unsigned long long invocations(void) {
  systolic_offload_stats_t s;
  systolic_offload_stats(&s);
  return s.per_device[0] + s.per_device[1] + s.per_device[2] + s.per_device[3];
}
static void fill(float *p, int n, int seed, int mod) {
  for (int e = 0; e < n; ++e)
    p[e] = (float)((e * (3 + seed % 5) + seed) % mod - mod / 2);
}
static void report(const char *name, const float *got, const float *want,
                   int n, unsigned long long inv) {
  int same = memcmp(got, want, n * sizeof *got) == 0;
  failures += !same;
  printf("[case] %s: %s, %llu invocation(s)\n", name,
         same ? "equal to the reference" : "DIFFERENT FROM THE REFERENCE", inv);
}

#define GEMM8(name, call, times)                                              \
  do {                                                                        \
    fill(&C8[0][0], 64, __LINE__, 5);                                         \
    memcpy(R8, C8, sizeof R8);                                                \
    unsigned long long before = invocations();                                \
    call;                                                                     \
    unsigned long long inv = invocations() - before;                          \
    ref(8, 8, 8, &A8[0][0], 8, &B8[0][0], 8, &R8[0][0], 8, times);            \
    report(name, &C8[0][0], &R8[0][0], 64, inv);                              \
  } while (0)

int main(void) {
  printf("depth test\n");
  fill(&A8[0][0], 64, 1, 7);
  fill(&B8[0][0], 64, 2, 7);
  fill(&A16[0][0], 128, 3, 7);
  fill(&B16[0][0], 128, 4, 7);
  fill(&Ab[0][0], 384, 5, 5);
  fill(&Bb[0][0], 384, 6, 5);

  GEMM8("d3", d3(A8, B8, C8), 1);
  GEMM8("d4_outer", d4_outer(A8, B8, C8), 2);
  GEMM8("d4_mid", d4_mid(A8, B8, C8), 3);
  GEMM8("d4_illegal", d4_illegal(A8, B8, C8), 2);
  {
    fill(&C8[0][0], 64, 11, 5);
    memcpy(R8, C8, sizeof R8);
    unsigned long long before = invocations();
    d4_splitk(A16, B16, C8);
    unsigned long long inv = invocations() - before;
    ref(8, 8, 16, &A16[0][0], 16, &B16[0][0], 8, &R8[0][0], 8, 1);
    report("d4_splitk", &C8[0][0], &R8[0][0], 64, inv);
  }
  GEMM8("d5_tiled", d5_tiled(A8, B8, C8), 1);
  {
    fill(&C16[0][0], 256, 12, 5);
    memcpy(R16, C16, sizeof R16);
    unsigned long long before = invocations();
    d6_tiled(Ab, Bb, C16);
    unsigned long long inv = invocations() - before;
    ref(16, 16, 24, &Ab[0][0], 24, &Bb[0][0], 16, &R16[0][0], 16, 1);
    report("d6_tiled", &C16[0][0], &R16[0][0], 256, inv);
  }
  {
    fill(&C8[0][0], 64, 13, 5);
    memcpy(R8, C8, sizeof R8);
    unsigned long long before = invocations();
    d7(A16, B16, C8);
    unsigned long long inv = invocations() - before;
    ref(8, 8, 16, &A16[0][0], 16, &B16[0][0], 8, &R8[0][0], 8, 6);
    report("d7", &C8[0][0], &R8[0][0], 64, inv);
  }
  GEMM8("d8", d8(A8, B8, C8), 4);

  {
    fill(&X5[0][0][0][0][0], 32, 14, 5);
    fill(&Y5[0][0][0][0][0], 32, 15, 5);
    memcpy(R5, X5, sizeof R5);
    unsigned long long before = invocations();
    ng_ewise5(X5, Y5);
    unsigned long long inv = invocations() - before;
    for (int e = 0; e < 32; ++e)
      (&R5[0][0][0][0][0])[e] = (&R5[0][0][0][0][0])[e] +
          mul((&Y5[0][0][0][0][0])[e], (&Y5[0][0][0][0][0])[e]);
    report("ng_ewise5", &X5[0][0][0][0][0], &R5[0][0][0][0][0], 32, inv);
  }
  {
    fill(&A3[0][0][0], 32, 16, 5);
    fill(&B3[0][0][0], 32, 17, 5);
    fill(&C3[0][0][0], 32, 18, 5);
    memcpy(R3, C3, sizeof R3);
    unsigned long long before = invocations();
    ng_batched(A3, B3, C3);
    unsigned long long inv = invocations() - before;
    for (int b = 0; b < 2; ++b)
      ref(4, 4, 4, &A3[b][0][0], 4, &B3[b][0][0], 4, &R3[b][0][0], 4, 1);
    report("ng_batched", &C3[0][0][0], &R3[0][0][0], 32, inv);
  }
  {
    fill(&As[0][0], 120, 19, 5);
    fill(&Bs[0][0], 128, 20, 5);
    fill(&C8[0][0], 64, 21, 5);
    memcpy(R8, C8, sizeof R8);
    unsigned long long before = invocations();
    ng_skew(As, Bs, C8);
    unsigned long long inv = invocations() - before;
    for (int i = 0; i < 8; ++i)
      for (int j = 0; j < 8; ++j)
        for (int k = 0; k < 8; ++k)
          R8[i][j] = R8[i][j] + mul(As[i + j][k], Bs[k][2 * j]);
    report("ng_skew", &C8[0][0], &R8[0][0], 64, inv);
  }
  {
    fill(&M8[0][0], 64, 22, 5);
    fill(x8, 8, 23, 5);
    fill(y8, 8, 24, 5);
    memcpy(ry8, y8, sizeof ry8);
    unsigned long long before = invocations();
    ng_matvec4(M8, x8, y8);
    unsigned long long inv = invocations() - before;
    for (int t = 0; t < 4; ++t)
      for (int i = 0; i < 8; ++i)
        for (int j = 0; j < 8; ++j)
          ry8[i] = ry8[i] + mul(M8[i][j], x8[j]);
    report("ng_matvec4", y8, ry8, 8, inv);
  }
  printf("%s: %d case(s) differ\n", failures ? "FAIL" : "PASS", failures);
  return failures != 0;
}
