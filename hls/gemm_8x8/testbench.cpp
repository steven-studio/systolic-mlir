// hls/gemm_8x8/testbench.cpp
#include <cstdio>
#include <cstring>

void matmul_8x8x8(float arg0[8][8], float arg1[8][8], float arg2[8][8]);

int main() {
  float A[8][8], B[8][8], C[8][8], C_ref[8][8];

  // 初始化測試資料
  for (int i = 0; i < 8; i++)
    for (int j = 0; j < 8; j++) {
      A[i][j] = (float)(i + j) / 8.0f;
      B[i][j] = (float)(i - j) / 8.0f;
      C[i][j] = 0.0f;
    }

  // Reference(naive C++)計算
  for (int i = 0; i < 8; i++)
    for (int j = 0; j < 8; j++) {
      float acc = 0.0f;
      for (int k = 0; k < 8; k++)
        acc += A[i][k] * B[k][j];
      C_ref[i][j] = acc;
    }

  // 呼叫 HLS design
  matmul_8x8x8(A, B, C);

  // 比對
  int errors = 0;
  for (int i = 0; i < 8; i++)
    for (int j = 0; j < 8; j++) {
      float diff = C[i][j] - C_ref[i][j];
      if (diff < 0) diff = -diff;
      if (diff > 1e-3f) {
        printf("Mismatch at (%d,%d): got %f, expected %f\n", i, j, C[i][j], C_ref[i][j]);
        errors++;
      }
    }

  if (errors == 0) {
    printf("PASS\n");
    return 0;
  } else {
    printf("FAIL: %d mismatches\n", errors);
    return 1;
  }
}
