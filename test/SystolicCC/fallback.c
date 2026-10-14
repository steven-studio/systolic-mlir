// REQUIRES: cfront-clang
//
// Which unit computed each GEMM, as the compiled program reports it on
// stdout: "[systolic-cc] FPGA Executed: GEMM #N" when the FPGA confirmed the
// result, "[systolic-cc] CPU Executed: GEMM #N" otherwise -- a GEMM the
// compiler did not recognize, or a recognized one whose FPGA run failed and
// was redone on the CPU from the original C. N numbers the executions from 1;
// plain loops and the program's own output carry no label.
//
// NO REAL HARDWARE HERE: the software runtime model computes every offloaded
// GEMM. SOFT_RUNTIME_MOCK_HARDWARE=1 makes it claim to be the FPGA (a test
// stand-in for a successful board run), SOFT_RUNTIME_FAULT=fail makes one
// invocation fail (a stand-in for a transport failure). Whatever happens,
// the program's results equal those of the same file compiled by clang alone
// (an independent CPU build) and the in-program reference below.
//
// RUN: %systolic-cc --explain %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// PLAN-DAG: fallback.c:[[#FPGA_GEMM:]]:1: note: 'fpga_gemm' is offloaded: 10 systolic tile invocation(s)
// PLAN-DAG: fallback.c:[[#TWO_NESTS:]]:1: note: 'two_nests' is offloaded: 2 systolic tile invocation(s)
//
// RUN: %host-cc -std=c11 %s -o %t.cpu
// RUN: %t.cpu > %t.cpu.out
// RUN: FileCheck %s --check-prefix=PROGRAM --input-file=%t.cpu.out
//
// The software model is not the FPGA: every GEMM reports CPU.
// RUN: %t > %t.soft
// RUN: FileCheck %s --check-prefix=SOFT --input-file=%t.soft
// RUN: grep -v '^\[systolic-cc\]' %t.soft | diff %t.cpu.out -
// SOFT-NOT:  FPGA
//
// The FPGA (mocked) runs the recognized GEMMs; cpu_gemm stays on the CPU.
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t > %t.mock
// RUN: FileCheck %s --check-prefix=MOCK --input-file=%t.mock --match-full-lines
// RUN: grep -v '^\[systolic-cc\]' %t.mock | diff %t.cpu.out -
//
// The first invocation of the first fpga_gemm call fails: that call is
// redone on the CPU from the original (non-zero) C; the program goes on and
// the later calls still use the FPGA.
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 SOFT_RUNTIME_FAULT=fail SOFT_RUNTIME_FAULT_AT=0 %t > %t.f0
// RUN: FileCheck %s --check-prefix=F0 --input-file=%t.f0 --match-full-lines
// RUN: grep -v '^\[systolic-cc\]' %t.f0 | diff %t.cpu.out -
//
// The last invocation of the first call fails, after nine tiles already
// accumulated into C: no partial or doubled accumulation survives.
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 SOFT_RUNTIME_FAULT=fail SOFT_RUNTIME_FAULT_AT=9 %t > %t.f9
// RUN: FileCheck %s --check-prefix=F0 --input-file=%t.f9 --match-full-lines
// RUN: grep -v '^\[systolic-cc\]' %t.f9 | diff %t.cpu.out -
//
// two_nests fails (invocation 10): both of its GEMMs are redone on the CPU.
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 SOFT_RUNTIME_FAULT=fail SOFT_RUNTIME_FAULT_AT=10 %t > %t.f10
// RUN: FileCheck %s --check-prefix=F10 --input-file=%t.f10 --match-full-lines
// RUN: grep -v '^\[systolic-cc\]' %t.f10 | diff %t.cpu.out -
//
// With --verify every result is also checked against the CPU reference, on
// failure too: three offloaded calls (cpu_gemm is not offloaded); the last
// fpga_gemm call fails at invocation 12.
// RUN: %systolic-cc --verify %s -o %t.v > /dev/null 2>&1
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 SOFT_RUNTIME_FAULT=fail SOFT_RUNTIME_FAULT_AT=12 %t.v > %t.v.out 2> %t.v.err
// RUN: FileCheck %s --check-prefix=F12 --input-file=%t.v.out --match-full-lines
// RUN: grep -v '^\[systolic-cc\]' %t.v.out | diff %t.cpu.out -
// RUN: FileCheck %s --check-prefix=VERIFY --input-file=%t.v.err
// VERIFY-COUNT-3: verified:
// VERIFY-NOT:     MISMATCH
//
// PROGRAM:      start
// PROGRAM-NEXT: initialized
// PROGRAM-NEXT: after the first GEMM
// PROGRAM-NEXT: checksums 429.0 576.0 811.0
// PROGRAM-NEXT: reference: equal
// PROGRAM-NEXT: done
//
// SOFT:      start
// SOFT-NEXT: initialized
// SOFT-NEXT: [systolic-cc] CPU Executed: GEMM #1
// SOFT-NEXT: after the first GEMM
// SOFT-NEXT: [systolic-cc] CPU Executed: GEMM #2
// SOFT-NEXT: [systolic-cc] CPU Executed: GEMM #3
// SOFT-NEXT: [systolic-cc] CPU Executed: GEMM #4
// SOFT-NEXT: [systolic-cc] CPU Executed: GEMM #5
// SOFT-NEXT: checksums
//
// MOCK:      start
// MOCK-NEXT: initialized
// MOCK-NEXT: [systolic-cc] FPGA Executed: GEMM #1
// MOCK-NEXT: after the first GEMM
// MOCK-NEXT: [systolic-cc] CPU Executed: GEMM #2
// MOCK-NEXT: [systolic-cc] FPGA Executed: GEMM #3
// MOCK-NEXT: [systolic-cc] FPGA Executed: GEMM #4
// MOCK-NEXT: [systolic-cc] FPGA Executed: GEMM #5
// MOCK-NEXT: checksums 429.0 576.0 811.0
// MOCK-NEXT: reference: equal
// MOCK-NEXT: done
// MOCK-EMPTY:
//
// F0:      start
// F0-NEXT: initialized
// F0-NEXT: [systolic-cc] CPU Executed: GEMM #1
// F0-NEXT: after the first GEMM
// F0-NEXT: [systolic-cc] CPU Executed: GEMM #2
// F0-NEXT: [systolic-cc] FPGA Executed: GEMM #3
// F0-NEXT: [systolic-cc] FPGA Executed: GEMM #4
// F0-NEXT: [systolic-cc] FPGA Executed: GEMM #5
// F0-NEXT: checksums 429.0 576.0 811.0
// F0-NEXT: reference: equal
// F0-NEXT: done
// F0-EMPTY:
//
// F10:      start
// F10-NEXT: initialized
// F10-NEXT: [systolic-cc] FPGA Executed: GEMM #1
// F10-NEXT: after the first GEMM
// F10-NEXT: [systolic-cc] CPU Executed: GEMM #2
// F10-NEXT: [systolic-cc] CPU Executed: GEMM #3
// F10-NEXT: [systolic-cc] CPU Executed: GEMM #4
// F10-NEXT: [systolic-cc] FPGA Executed: GEMM #5
// F10-NEXT: checksums 429.0 576.0 811.0
// F10-NEXT: reference: equal
// F10-NEXT: done
// F10-EMPTY:
//
// F12:      start
// F12-NEXT: initialized
// F12-NEXT: [systolic-cc] FPGA Executed: GEMM #1
// F12-NEXT: after the first GEMM
// F12-NEXT: [systolic-cc] CPU Executed: GEMM #2
// F12-NEXT: [systolic-cc] FPGA Executed: GEMM #3
// F12-NEXT: [systolic-cc] FPGA Executed: GEMM #4
// F12-NEXT: [systolic-cc] CPU Executed: GEMM #5
// F12-NEXT: checksums 429.0 576.0 811.0
// F12-NEXT: reference: equal
// F12-NEXT: done
// F12-EMPTY:

#include <stdio.h>

// Recognized: 10 tile invocations.
void fpga_gemm(float A[restrict 16][16], float B[restrict 16][16],
               float C[restrict 16][16]) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 16; ++j)
      for (int k = 0; k < 16; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

// A GEMM the compiler does not recognize (a scalar accumulator).
void cpu_gemm(float A[restrict 8][8], float B[restrict 8][8],
              float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j) {
      float acc = C[i][j];
      for (int k = 0; k < 8; ++k)
        acc += A[i][k] * B[k][j];
      C[i][j] = acc;
    }
}

// Two GEMMs and a plain loop in one function: two labels, none for the loop.
void two_nests(float A[restrict 8][8], float B[restrict 8][8],
               float D[restrict 8][8], float E[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      D[i][j] = 0.0f;
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        D[i][j] += A[i][k] * B[k][j];
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        E[i][j] += B[i][k] * A[k][j];
}

// The in-program reference: a multiply-add through a call is not a GEMM
// statement, so these loops are not labelled.
static float mac(float c, float a, float b) { return c + a * b; }

static void reference(int n, int lda, const float *A, const float *B,
                      float *C) {
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j)
      for (int k = 0; k < n; ++k)
        C[i * lda + j] = mac(C[i * lda + j], A[i * lda + k], B[k * lda + j]);
}

static float A[16][16], B[16][16], C[16][16], RC[16][16];
static float a[8][8], b[8][8], c[8][8], d[8][8], e[8][8];
static float rc[8][8], rd[8][8], re[8][8];

int main(void) {
  printf("start\n");
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 16; ++j) {
      A[i][j] = (float)((i + 2 * j) % 5 - 2);
      B[i][j] = (float)((3 * i + j) % 7 - 3);
      C[i][j] = RC[i][j] = (float)((i * j) % 3);   // non-zero: C += A x B
    }
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j) {
      a[i][j] = A[i][j];
      b[i][j] = B[j][i];
      c[i][j] = rc[i][j] = 2.0f;
      d[i][j] = rd[i][j] = 7.0f;   // cleared by two_nests
      e[i][j] = re[i][j] = (float)(i - j);
    }
  printf("initialized\n");

  fpga_gemm(A, B, C);
  printf("after the first GEMM\n");
  cpu_gemm(a, b, c);
  two_nests(a, b, d, e);
  fpga_gemm(A, B, C);   // accumulates again into the result of the first

  reference(16, 16, &A[0][0], &B[0][0], &RC[0][0]);
  reference(16, 16, &A[0][0], &B[0][0], &RC[0][0]);
  reference(8, 8, &a[0][0], &b[0][0], &rc[0][0]);
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      rd[i][j] = 0.0f;
  reference(8, 8, &a[0][0], &b[0][0], &rd[0][0]);
  reference(8, 8, &b[0][0], &a[0][0], &re[0][0]);

  double s16 = 0, s8 = 0, se = 0;
  int equal = 1;
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 16; ++j) {
      s16 += C[i][j] * (i + 1);
      equal &= C[i][j] == RC[i][j];
    }
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j) {
      s8 += (c[i][j] - d[i][j]) * (j + 1);
      se += e[i][j] * (i - j);
      equal &= c[i][j] == rc[i][j] && d[i][j] == rd[i][j] &&
               e[i][j] == re[i][j];
    }
  printf("checksums %.1f %.1f %.1f\n", s16, s8, se);
  printf("reference: %s\n", equal ? "equal" : "DIFFERENT");
  printf("done\n");
  return 0;
}
