// REQUIRES: cfront-clang
//
// A GEMM whose row loop has an affine condition and counts down, exactly as
// written in C:  for (int i = 100; 2 * i - 3 > 0; i--)  -- i = 100, 99, ...,
// 2 (99 iterations; 2 * 1 - 3 = -1 ends it). Through the whole compiler: the
// front end (the condition is i > 1; the ordinal loop 0 to 99 with i = 100 -
// t), GEMM recognition, the dense partition (I read backwards: rows 2 .. 100),
// the fleet partition and lowering, the offload, and the result checked
// against a reference that runs the same loop on the CPU (products through a
// call, so it is not offloaded itself) and counts the rows it visits.
//
// NO REAL HARDWARE HERE: the software runtime model computes every tile;
// SOFT_RUNTIME_MOCK_HARDWARE=1 makes it claim to be the FPGA.
//
// RUN: %systolic-cc --explain %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// PLAN: note: 'affine_condition' is offloaded: 29 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN: wrote {{.*}} (1 function(s) offloaded
//
// RUN: %t > %t.soft
// RUN: FileCheck %s --check-prefixes=SOFT,ALL --input-file=%t.soft --match-full-lines
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t > %t.mock
// RUN: FileCheck %s --check-prefixes=MOCK,ALL --input-file=%t.mock --match-full-lines
// SOFT-NOT: FPGA
// ALL:       affine condition test
// SOFT-NEXT: [systolic-cc] CPU Executed: GEMM #1
// MOCK-NEXT: [systolic-cc] FPGA Executed: GEMM #1
// ALL-NEXT:  reference: 99 row(s), first 100, last 2
// ALL-NEXT:  [case] affine_condition: equal to the reference, 29 invocation(s)
// ALL-NEXT:  PASS: 0 case(s) differ
//
// With --verify the offloaded call is also compared with the function as
// written, run on the CPU.
// RUN: %systolic-cc --verify %s -o %t.v > /dev/null 2>&1
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t.v 2>&1 >/dev/null | FileCheck %s --check-prefix=VERIFY
// VERIFY: verified: 808 elements equal the CPU reference (bitwise)
// VERIFY-NOT: MISMATCH

#include <stdio.h>
#include <string.h>

#include "systolic_offload.h"

void affine_condition(float A[restrict 101][8], float B[restrict 8][8],
                      float C[restrict 101][8]) {
  for (int i = 100; 2 * i - 3 > 0; i--)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

/* The same loop on the CPU: products through a call (not a GEMM statement);
   it also reports which rows the condition lets through. */
static float mul(float a, float b) { return a * b; }
static void reference(float A[101][8], float B[8][8], float C[101][8]) {
  int rows = 0, first = -1, last = -1;
  for (int i = 100; 2 * i - 3 > 0; i--) {
    if (first < 0)
      first = i;
    last = i;
    ++rows;
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        C[i][j] = C[i][j] + mul(A[i][k], B[k][j]);
  }
  printf("reference: %d row(s), first %d, last %d\n", rows, first, last);
}

static float A[101][8], B[8][8], C[101][8], R[101][8];

static unsigned long long invocations(void) {
  systolic_offload_stats_t s;
  systolic_offload_stats(&s);
  return s.per_device[0] + s.per_device[1] + s.per_device[2] + s.per_device[3];
}

int main(void) {
  printf("affine condition test\n");
  for (int e = 0; e < 101 * 8; ++e) {
    (&A[0][0])[e] = (float)(e % 7 - 3);
    (&C[0][0])[e] = (float)(e % 5 - 2);
  }
  for (int e = 0; e < 64; ++e)
    (&B[0][0])[e] = (float)(e % 5 - 2);
  memcpy(R, C, sizeof R);
  unsigned long long before = invocations();
  affine_condition(A, B, C);
  unsigned long long inv = invocations() - before;
  reference(A, B, R);
  int same = memcmp(C, R, sizeof C) == 0;
  printf("[case] affine_condition: %s, %llu invocation(s)\n",
         same ? "equal to the reference" : "DIFFERENT FROM THE REFERENCE", inv);
  printf("%s: %d case(s) differ\n", same ? "PASS" : "FAIL", !same);
  return !same;
}
