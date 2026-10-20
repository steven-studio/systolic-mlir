// REQUIRES: cfront-clang
//
// Calls whose integer parameters put an accessed row before the array's
// first element: C[r0 + i][j] with r0 < 0 on a pointer into the middle of
// the caller's array -- legal C. The front end records each array's run-time
// row range (leading_min_expr, leading_extent_expr in the interface); the
// caller-side shim snapshots, restores and checks rows [0, extent) of what
// the pointer addresses, and the kernel's views start at row 0. So a call
// whose minimum row is negative -- for any array, read or written -- runs on
// the CPU, as the function is written; a minimum of 0 or more stays on the
// FPGA. Checked on the software runtime model (NOT the FPGA), in both
// lowerings:
//   - with the hardware mocked: which calls run on the (mocked) FPGA and
//     which on the CPU, and every row of the caller's arrays equal to the
//     program compiled by clang alone;
//   - with an invocation failing (SOFT_RUNTIME_FAULT): the call is redone on
//     the CPU from the original arrays, and rows before the pointer must not
//     keep the kernel's partial sums or be added to twice;
//   - with --verify: the calls that ran on the FPGA are verified, the others
//     say so on stderr, and nothing writes outside the shim's copies.
//
// RUN: %systolic-cc --fp-reassociation --explain %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// RUN: %host-cc -std=c11 %s -o %t.cpu
// RUN: %t.cpu > %t.cpu.out
//
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t > %t.mock
// RUN: FileCheck %s --check-prefix=MOCK --input-file=%t.mock
// RUN: grep -v '^\[systolic-cc\]' %t.mock | diff %t.cpu.out -
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 SOFT_RUNTIME_FAULT=fail SOFT_RUNTIME_FAULT_AT=0 %t | grep -v '^\[systolic-cc\]' | diff %t.cpu.out -
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 SOFT_RUNTIME_FAULT=fail SOFT_RUNTIME_FAULT_AT=1 %t | grep -v '^\[systolic-cc\]' | diff %t.cpu.out -
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 SOFT_RUNTIME_FAULT=fail SOFT_RUNTIME_FAULT_AT=3 %t | grep -v '^\[systolic-cc\]' | diff %t.cpu.out -
//
// RUN: %systolic-cc --submission async --fp-reassociation %s -o %t.a > /dev/null 2>&1
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t.a > %t.a.mock
// RUN: FileCheck %s --check-prefix=MOCK --input-file=%t.a.mock
// RUN: grep -v '^\[systolic-cc\]' %t.a.mock | diff %t.cpu.out -
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 SOFT_RUNTIME_FAULT=fail SOFT_RUNTIME_FAULT_AT=1 %t.a | grep -v '^\[systolic-cc\]' | diff %t.cpu.out -
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 SOFT_RUNTIME_FAULT=fail SOFT_RUNTIME_FAULT_AT=2 %t.a | grep -v '^\[systolic-cc\]' | diff %t.cpu.out -
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 SOFT_RUNTIME_FAULT=fail SOFT_RUNTIME_FAULT_AT=3 %t.a | grep -v '^\[systolic-cc\]' | diff %t.cpu.out -
//
// RUN: %systolic-cc --fp-reassociation --verify %s -o %t.v > /dev/null 2>&1
// RUN: %t.v > %t.v.out 2> %t.v.err
// RUN: FileCheck %s --check-prefix=VERIFY --input-file=%t.v.err
// RUN: not grep -i mismatch %t.v.err
// RUN: grep -v '^\[systolic-cc\]' %t.v.out | diff %t.cpu.out -
// RUN: %systolic-cc --submission async --fp-reassociation --verify %s -o %t.av > /dev/null 2>&1
// RUN: %t.av > %t.av.out 2> %t.av.err
// RUN: FileCheck %s --check-prefix=VERIFY --input-file=%t.av.err
// RUN: not grep -i mismatch %t.av.err
// RUN: grep -v '^\[systolic-cc\]' %t.av.out | diff %t.cpu.out -

#include <stdio.h>

// The output at a run-time row offset: rows r0 .. r0 + 15.
void gemm_rows(float A[restrict 16][16], float B[restrict 16][8],
               float C[restrict 48][8], int r0) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 16; ++k)
        C[r0 + i][j] += A[i][k] * B[k][j];
}

// An input read at a run-time row offset (the output from row 0).
void gemm_arows(float A[restrict 48][16], float B[restrict 16][8],
                float C[restrict 16][8], int r0) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 16; ++k)
        C[i][j] += A[r0 + i][k] * B[k][j];
}

// A negative coefficient on the parameter: rows -s .. 15 - s.
void gemm_back(float A[restrict 16][16], float B[restrict 16][8],
               float C[restrict 48][8], int s) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 16; ++k)
        C[i - s][j] += A[i][k] * B[k][j];
}

// A static access and an offset one to the same array: the minimum is
// min(0, r0 + 16), the extent max(16, r0 + 32).
void gemm_mixed(float A[restrict 16][16], float B[restrict 16][8],
                float C[restrict 48][8], int r0) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 16; ++k)
        C[i][j] += A[i][k] * B[k][j];
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 16; ++k)
        C[r0 + i + 16][j] += A[i][k] * B[k][j];
}

// Two parameters in one subscript: the minimum is r0 + r1.
void gemm_two(float A[restrict 16][16], float B[restrict 16][8],
              float C[restrict 48][8], int r0, int r1) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 16; ++k)
        C[r0 + r1 + i][j] += A[i][k] * B[k][j];
}

#define ROWS 80
static float A[16][16], B[16][8], C16[16][8];
static float bigC[ROWS][8];   // the output views point into its middle
static float bigA[ROWS][16];  // and so do the input views

static void reset(void) {
  for (int i = 0; i < 16; ++i)
    for (int k = 0; k < 16; ++k)
      A[i][k] = (float)((i * 3 + k) % 7 - 3);
  for (int k = 0; k < 16; ++k)
    for (int j = 0; j < 8; ++j)
      B[k][j] = (float)((k * 5 + j * 3) % 5 - 2);
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 8; ++j)
      C16[i][j] = (float)((i + j) % 3 - 1);
  for (int i = 0; i < ROWS; ++i)
    for (int j = 0; j < 8; ++j)
      bigC[i][j] = (float)((i * 7 + j) % 5 - 2);
  for (int i = 0; i < ROWS; ++i)
    for (int k = 0; k < 16; ++k)
      bigA[i][k] = (float)((i + 2 * k) % 9 - 4);
}

// Every element of the caller's arrays, each weighted by its position.
static double sum(const float *p, int n) {
  double s = 0;
  for (int i = 0; i < n; ++i)
    s += p[i] * (double)(i % 13 + 1) * (double)(i / 13 + 1);
  return s;
}

static void show(const char *what) {
  printf("%s: %.1f %.1f\n", what, sum(&bigC[0][0], ROWS * 8), sum(&C16[0][0], 16 * 8));
}

int main(void) {
  reset(); gemm_rows(A, B, &bigC[32], -4);   show("rows -4");    // min -4
  reset(); gemm_rows(A, B, &bigC[32], -16);  show("rows -16");   // extent 0
  reset(); gemm_rows(A, B, &bigC[32], -24);  show("rows -24");   // extent < 0
  reset(); gemm_rows(A, B, &bigC[32], 0);    show("rows 0");     // min 0
  reset(); gemm_rows(A, B, &bigC[32], 5);    show("rows 5");
  reset(); gemm_arows(&bigA[32], B, C16, -6); show("arows -6");  // an input before its pointer
  reset(); gemm_arows(&bigA[32], B, C16, 3);  show("arows 3");
  reset(); gemm_back(A, B, &bigC[32], 3);    show("back 3");     // min -3
  reset(); gemm_back(A, B, &bigC[32], -2);   show("back -2");    // min 2
  reset(); gemm_mixed(A, B, &bigC[32], -20); show("mixed -20");  // min(0, -4)
  reset(); gemm_mixed(A, B, &bigC[32], -16); show("mixed -16");  // min(0, 0)
  reset(); gemm_two(A, B, &bigC[32], 4, -6); show("two 4 -6");   // min -2
  reset(); gemm_two(A, B, &bigC[32], 6, -6); show("two 6 -6");   // min 0
  return 0;
}

// PLAN-DAG: negoffset.c:{{[0-9]+}}:1: note: 'gemm_rows' is offloaded: 5 systolic tile invocation(s)
// PLAN-DAG: negoffset.c:{{[0-9]+}}:1: note: 'gemm_arows' is offloaded: 5 systolic tile invocation(s)
// PLAN-DAG: negoffset.c:{{[0-9]+}}:1: note: 'gemm_back' is offloaded: 5 systolic tile invocation(s)
// PLAN-DAG: negoffset.c:{{[0-9]+}}:1: note: 'gemm_mixed' is offloaded: 10 systolic tile invocation(s)
// PLAN-DAG: negoffset.c:{{[0-9]+}}:1: note: 'gemm_two' is offloaded: 5 systolic tile invocation(s)
// PLAN:     wrote {{.*}} (5 function(s) offloaded
//
// MOCK:      [systolic-cc] CPU Executed: GEMM #1
// MOCK-NEXT: rows -4:
// MOCK-NEXT: [systolic-cc] CPU Executed: GEMM #2
// MOCK-NEXT: rows -16:
// MOCK-NEXT: [systolic-cc] CPU Executed: GEMM #3
// MOCK-NEXT: rows -24:
// MOCK-NEXT: [systolic-cc] FPGA Executed: GEMM #4
// MOCK-NEXT: rows 0:
// MOCK-NEXT: [systolic-cc] FPGA Executed: GEMM #5
// MOCK-NEXT: rows 5:
// MOCK-NEXT: [systolic-cc] CPU Executed: GEMM #6
// MOCK-NEXT: arows -6:
// MOCK-NEXT: [systolic-cc] FPGA Executed: GEMM #7
// MOCK-NEXT: arows 3:
// MOCK-NEXT: [systolic-cc] CPU Executed: GEMM #8
// MOCK-NEXT: back 3:
// MOCK-NEXT: [systolic-cc] FPGA Executed: GEMM #9
// MOCK-NEXT: back -2:
// MOCK-NEXT: [systolic-cc] CPU Executed: GEMM #10
// MOCK-NEXT: [systolic-cc] CPU Executed: GEMM #11
// MOCK-NEXT: mixed -20:
// MOCK-NEXT: [systolic-cc] FPGA Executed: GEMM #12
// MOCK-NEXT: [systolic-cc] FPGA Executed: GEMM #13
// MOCK-NEXT: mixed -16:
// MOCK-NEXT: [systolic-cc] CPU Executed: GEMM #14
// MOCK-NEXT: two 4 -6:
// MOCK-NEXT: [systolic-cc] FPGA Executed: GEMM #15
// MOCK-NEXT: two 6 -6:
//
// VERIFY:      gemm_rows (negoffset.c:{{[0-9]+}}): ran on the CPU: at these arguments an access reaches a row before the first element of an argument array; nothing to compare
// VERIFY-NEXT: gemm_rows (negoffset.c:{{[0-9]+}}): ran on the CPU:
// VERIFY-NEXT: gemm_rows (negoffset.c:{{[0-9]+}}): ran on the CPU:
// VERIFY-NEXT: gemm_rows (negoffset.c:{{[0-9]+}}): verified: 128 elements equal the CPU reference (bitwise)
// VERIFY-NEXT: gemm_rows (negoffset.c:{{[0-9]+}}): verified: 168 elements equal the CPU reference (bitwise)
// VERIFY-NEXT: gemm_arows (negoffset.c:{{[0-9]+}}): ran on the CPU:
// VERIFY-NEXT: gemm_arows (negoffset.c:{{[0-9]+}}): verified: 128 elements equal the CPU reference (bitwise)
// VERIFY-NEXT: gemm_back (negoffset.c:{{[0-9]+}}): ran on the CPU:
// VERIFY-NEXT: gemm_back (negoffset.c:{{[0-9]+}}): verified: 144 elements equal the CPU reference (bitwise)
// VERIFY-NEXT: gemm_mixed (negoffset.c:{{[0-9]+}}): ran on the CPU:
// VERIFY-NEXT: gemm_mixed (negoffset.c:{{[0-9]+}}): verified: 128 elements equal the CPU reference (bitwise)
// VERIFY-NEXT: gemm_two (negoffset.c:{{[0-9]+}}): ran on the CPU:
// VERIFY-NEXT: gemm_two (negoffset.c:{{[0-9]+}}): verified: 128 elements equal the CPU reference (bitwise)
