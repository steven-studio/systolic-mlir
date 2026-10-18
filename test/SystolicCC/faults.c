// REQUIRES: cfront-clang
//
// What a compiled program does when the runtime under an offloaded call
// fails, and when it returns a wrong result without failing. The software
// runtime model injects both (SOFT_RUNTIME_FAULT); the two must never be
// confused. A failed invocation is not the end of the program: the call is
// redone on the CPU from the original C and the result is right. A wrong
// result the runtime does not report is a NUMERICAL MISMATCH (exit 4), and
// only --verify can see it.
//
// RUN: %systolic-cc --verify %s -o %t.verify > /dev/null 2>&1
// RUN: %systolic-cc %s -o %t.plain > /dev/null 2>&1
//
// No fault: correct.
// RUN: %t.verify 2>&1 | FileCheck %s --check-prefix=CLEAN
// CLEAN: verified: 256 elements equal the CPU reference
// CLEAN: checksum 5760
//
// Invocation 3 fails (rc -8, a transport timeout), after three tiles already
// accumulated into C: the CPU redoes the GEMM from the original C, the
// program goes on and its result is right, with or without --verify.
// RUN: env SOFT_RUNTIME_FAULT=fail SOFT_RUNTIME_FAULT_AT=3 %t.verify 2>&1 | FileCheck %s --check-prefix=FAIL
// RUN: env SOFT_RUNTIME_FAULT=fail SOFT_RUNTIME_FAULT_AT=3 %t.plain 2>&1 | FileCheck %s --check-prefix=FAIL
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 SOFT_RUNTIME_FAULT=fail SOFT_RUNTIME_FAULT_AT=3 %t.plain 2>&1 | FileCheck %s --check-prefix=FAIL
// FAIL-NOT: FPGA Executed
// FAIL:     [systolic-cc] CPU Executed: GEMM #1
// FAIL-NOT: MISMATCH
// FAIL:     checksum 5760
//
// The same program on the (mocked) FPGA without a fault.
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t.plain 2>&1 | FileCheck %s --check-prefix=MOCK
// MOCK: [systolic-cc] FPGA Executed: GEMM #1
// MOCK: checksum 5760
//
// Invocation 5 returns a wrong element without failing: --verify reports a
// numerical mismatch and stops.
// RUN: env SOFT_RUNTIME_FAULT=corrupt SOFT_RUNTIME_FAULT_AT=5 not %t.verify 2>&1 | FileCheck %s --check-prefix=CORRUPT
// CORRUPT:     matmul (faults.c:{{[0-9]+}}): first mismatch C[{{[0-9]+}}]: result {{.*}}, CPU reference
// CORRUPT:     matmul (faults.c:{{[0-9]+}}): NUMERICAL MISMATCH: 1 of 256 elements differ from the CPU reference (bitwise)
// CORRUPT-NOT: checksum
//
// Without --verify nothing can see it: the program finishes with the wrong
// result. (Only verification against a reference detects a wrong result the
// runtime does not report.)
// RUN: env SOFT_RUNTIME_FAULT=corrupt SOFT_RUNTIME_FAULT_AT=5 %t.plain 2>&1 | FileCheck %s --check-prefix=SILENT
// SILENT: checksum 5761

#include <stdio.h>

void matmul(float A[restrict 16][16], float B[restrict 16][16],
            float C[restrict 16][16]) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 16; ++j)
      for (int k = 0; k < 16; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

static float A[16][16], B[16][16], C[16][16];

int main(void) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 16; ++j) {
      A[i][j] = (float)(i % 3);
      B[i][j] = (float)(j % 4);
      C[i][j] = 0.0f;
    }
  matmul(A, B, C);
  double sum = 0;
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 16; ++j)
      sum += C[i][j];
  printf("checksum %.0f\n", sum);
  return 0;
}
