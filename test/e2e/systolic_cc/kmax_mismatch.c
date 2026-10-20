// kmax_mismatch.c -- GEMMs whose K is folded deeper than the bitstream holds:
// built by systolic-cc for K_MAX_8X8 = K_MAX_4X4 = 32 and run against a
// 16 / 16 bitstream (test/e2e/run_cosim_regressions.sh section kmax). Each
// output tile's folds are k = 32 first and the remainder last, so the
// runtime refuses every fold but the last at submit (k > K_MAX, -10): with
// the asynchronous runtime the accepted last fold must not wait for ever for
// the folds before it -- the GEMM fails, the shim restores C and runs it on
// the CPU. The output must equal the program compiled by clang alone.

#include <stdio.h>

// 8x8x40 on the 8x8: folds 32 + 8.
void gemm40(float A[restrict 8][40], float B[restrict 40][8], float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 40; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

// 8x8x72: folds 32 + 32 + 8.
void gemm72(float A[restrict 8][72], float B[restrict 72][8], float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 72; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

// 12x12x40: 8x8 and 4x4 output tiles on every array.
void mixed40(float A[restrict 12][40], float B[restrict 40][12], float C[restrict 12][12]) {
  for (int i = 0; i < 12; ++i)
    for (int j = 0; j < 12; ++j)
      for (int k = 0; k < 40; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

// The output written in reverse row order, 8x8x40.
void rr40(float A[restrict 8][40], float B[restrict 40][8], float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 40; ++k)
        C[7 - i][j] += A[i][k] * B[k][j];
}

static float A[12][72], B[72][12], C8[8][8], C12[12][12];

static void fill(void) {
  for (int i = 0; i < 12; ++i)
    for (int k = 0; k < 72; ++k)
      A[i][k] = (float)((i * 5 + k * 3) % 7 - 3);
  for (int k = 0; k < 72; ++k)
    for (int j = 0; j < 12; ++j)
      B[k][j] = (float)((k + j * 7) % 5 - 2);
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      C8[i][j] = (float)((i + j) % 3 - 1);
  for (int i = 0; i < 12; ++i)
    for (int j = 0; j < 12; ++j)
      C12[i][j] = (float)((2 * i + j) % 5 - 2);
}

static double sum(const float *p, int n) {
  double s = 0;
  for (int i = 0; i < n; ++i)
    s += p[i] * (double)(i + 1);
  return s;
}

int main(void) {
  float A40[8][40], B40[40][8], A72[8][72], B72[72][8], A12[12][40], B12[40][12];
  fill();
  for (int i = 0; i < 12; ++i)
    for (int k = 0; k < 72; ++k) {
      if (i < 8 && k < 40) A40[i][k] = A[i][k];
      if (i < 8) A72[i][k] = A[i][k];
      if (k < 40) A12[i][k] = A[i][k];
    }
  for (int k = 0; k < 72; ++k)
    for (int j = 0; j < 12; ++j) {
      if (k < 40 && j < 8) B40[k][j] = B[k][j];
      if (j < 8) B72[k][j] = B[k][j];
      if (k < 40) B12[k][j] = B[k][j];
    }
  gemm40(A40, B40, C8);
  printf("gemm40 %.1f\n", sum(&C8[0][0], 64));
  gemm72(A72, B72, C8);
  printf("gemm72 %.1f\n", sum(&C8[0][0], 64));
  mixed40(A12, B12, C12);
  printf("mixed40 %.1f\n", sum(&C12[0][0], 144));
  rr40(A40, B40, C8);
  printf("rr40 %.1f\n", sum(&C8[0][0], 64));
  return 0;
}
