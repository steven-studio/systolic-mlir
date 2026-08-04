// testbench.cpp -- same shape as the original hls/gemm_4x4/testbench.cpp,
// including the C_init accumulate semantics and 1e-3 tolerance.
#include <cstdio>
#include "design.h"

int main() {
  float A[R][K_DIM], B[K_DIM][C], Cbuf[R][C], Cref[R][C];

  for (int i = 0; i < R; i++)
    for (int j = 0; j < K_DIM; j++) A[i][j] = (float)(i + j) / 4.0f;
  for (int i = 0; i < K_DIM; i++)
    for (int j = 0; j < C; j++) B[i][j] = (float)(i - j) / 4.0f;
  for (int i = 0; i < R; i++)
    for (int j = 0; j < C; j++) { Cbuf[i][j] = 0.0f; Cref[i][j] = 0.0f; }

  for (int i = 0; i < R; i++)
    for (int j = 0; j < C; j++) {
      float s = 0.0f;
      for (int k = 0; k < K_DIM; k++) s += A[i][k] * B[k][j];
      Cref[i][j] = s;
    }

  matmul_4x4x4(A, B, Cbuf);

  int errors = 0;
  for (int i = 0; i < R; i++)
    for (int j = 0; j < C; j++) {
      float d = Cbuf[i][j] - Cref[i][j];
      if (d < 0) d = -d;
      if (d > 1e-3f) {
        printf("Mismatch (%d,%d): got %f expected %f\n", i, j, Cbuf[i][j], Cref[i][j]);
        errors++;
      }
    }
  printf("%dx%d K=%d, time steps = %d\n", R, C, K_DIM, TIME_STEPS);
  if (errors) { printf("FAIL: %d mismatches\n", errors); return 1; }
  printf("PASS\n");
  return 0;
}
