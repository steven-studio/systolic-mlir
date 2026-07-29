// hls/gemm_4x4/testbench.cpp
#include <cstdio>
#include <cstring>

void matmul_4x4x4(float arg0[4][4], float arg1[4][4], float arg2[4][4]);

int main() {
  float A[4][4], B[4][4], C[4][4], C_ref[4][4];

  // 初始化測試資料
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++) {
      A[i][j] = (float)(i + j) / 4.0f;
      B[i][j] = (float)(i - j) / 4.0f;
      C[i][j] = 0.0f;
    }

  // Reference(naive C++)計算
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++) {
      float acc = 0.0f;
      for (int k = 0; k < 4; k++)
        acc += A[i][k] * B[k][j];
      C_ref[i][j] = acc;
    }

  // 呼叫 HLS design
  matmul_4x4x4(A, B, C);

  // 比對
  int errors = 0;
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++) {
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
