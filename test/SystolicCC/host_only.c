// REQUIRES: cfront-clang
//
// A program with nothing to offload compiles and runs as plain C; the
// compiler says why each function stays on the host (--explain). The GEMM
// it did not recognize reports that the CPU ran it; the axpy loop is not a
// GEMM and reports nothing.
//
// RUN: %systolic-cc --explain %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// RUN: %t | FileCheck %s --check-prefix=OUT --match-full-lines

#include <stdio.h>

// PLAN-DAG: host_only.c:[[@LINE+1]]:1: note: 'axpy' stays on the host: no GEMM the systolic arrays can run was recognized in it
void axpy(float y[restrict 8], float x[restrict 8], float a) {
  for (int i = 0; i < 8; ++i)
    y[i] += a * x[i];
}

// A scalar accumulator is outside the subset (local variables).
// PLAN-DAG: host_only.c:[[@LINE+5]]:7: note: 'dot_gemm' is outside the supported C subset and is not translated: local variables are not supported (only for-loop counters)
void dot_gemm(float A[restrict 4][4], float B[restrict 4][4],
              float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j) {
      float acc = 0.0f;
      for (int k = 0; k < 4; ++k)
        acc += A[i][k] * B[k][j];
      C[i][j] = acc;
    }
}

// PLAN: wrote {{.*}} (0 function(s) offloaded
// OUT:      [systolic-cc] CPU Executed: GEMM #1
// OUT-NEXT: 13 12
// OUT-EMPTY:

int main(void) {
  float y[8] = {1, 1, 1, 1, 1, 1, 1, 1}, x[8] = {0, 1, 2, 3, 4, 5, 6, 7};
  float A[4][4] = {{1, 2}, {3, 4}}, B[4][4] = {{1, 0}, {0, 1}}, C[4][4];
  axpy(y, x, 2.0f);
  dot_gemm(A, B, C);
  printf("%g %g\n", y[6], C[1][0] * 4);
  return 0;
}
