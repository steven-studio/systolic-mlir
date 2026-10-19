// REQUIRES: cfront-clang
//
// The one deviation of an offloaded assignment C = A * B from the loop as
// written (docs/AFFINE_GEMM_MAPPING.md sections 6 and 20): the systolic tiles
// sum from +0 and the host writes 0 + (0 + a b) into the zeroed C, so a
// product that is -0 (a zero factor times a negative one) comes back as +0.
// Numerically equal, bitwise different -- the reason the strict default keeps
// every GEMM on the host and --fp-reassociation is the program's explicit
// permission. Here A holds zeros and B negative values: bitwise --verify
// reports the sign-of-zero elements as mismatches, --verify-rtol accepts them,
// and the program itself counts them (every difference is a zero against a
// zero).
//
// RUN: %systolic-cc --fp-reassociation --explain %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// PLAN: note: 'signzero_assign' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// RUN: %t | FileCheck %s --check-prefix=OUT
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t | FileCheck %s --check-prefix=OUT
// OUT: [case] signzero_assign: offloaded (1), 16 elements: 0 differ in value, 6 differ only in the sign of a zero (offload +0, loop -0)
// OUT: PASS
//
// RUN: %systolic-cc --fp-reassociation --verify %s -o %t.v > /dev/null 2>&1
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t.v > /dev/null 2> %t.bitwise || true
// RUN: FileCheck %s --check-prefix=BITWISE --input-file=%t.bitwise
// BITWISE: signzero_assign ({{.*}}): first mismatch C[{{[0-9]+}}]: result 0, CPU reference -0
// BITWISE: NUMERICAL MISMATCH: 6 of 16 elements differ from the CPU reference (bitwise)
// RUN: %systolic-cc --fp-reassociation --verify --verify-rtol 1e-6 %s -o %t.r > /dev/null 2>&1
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t.r 2>&1 >/dev/null | FileCheck %s --check-prefix=RTOL
// RTOL: signzero_assign ({{.*}}): verified: 16 elements equal the CPU reference (relative tolerance 1e-06)
// RTOL-NOT: MISMATCH

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "systolic_offload.h"

void signzero_assign(float A[restrict 4], float B[restrict 4], float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) C[i][j] = A[i] * B[j];
}

static unsigned long long inv(void) { systolic_offload_stats_t s; systolic_offload_stats(&s); return s.per_device[0]+s.per_device[1]+s.per_device[2]+s.per_device[3]; }
int main(void) {
  static float A[4] = {1, 0, -2, 0}, B[4] = {-1, -3, 2, -4}, C[4][4], R[4][4];
  for (int e = 0; e < 16; ++e) (&C[0][0])[e] = (float)(e % 5 - 2);
  memcpy(R, C, sizeof R);
  unsigned long long before = inv();
  signzero_assign(A, B, C);
  unsigned long long n = inv() - before;
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) R[i][j] = A[i] * B[j];
  int value = 0, sign = 0;
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) {
    if (C[i][j] != R[i][j]) ++value;
    else if (memcmp(&C[i][j], &R[i][j], sizeof(float)) != 0) ++sign;
  }
  printf("[case] signzero_assign: %s (%llu), 16 elements: %d differ in value, %d differ only in the sign of a zero (offload +0, loop -0)\n",
         n ? "offloaded" : "NOT OFFLOADED", n, value, sign);
  printf("%s\n", value == 0 && n ? "PASS" : "FAIL");
  return value != 0 || !n;
}
