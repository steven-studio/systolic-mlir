#include "fpga_matmul_tiled.h"
#include "fpga_matmul4x4.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#define PORT "/dev/ttyUSB1"
#define M 6
#define K 6
#define N 6

int main() {
    float A[M*K], B[K*N], C[M*N];

    // 用有小數、有正負號的固定亂數(seed 固定,方便重現)
    srand(42);
    for (int i = 0; i < M*K; i++) A[i] = ((rand() % 2000) - 1000) / 100.0f;
    for (int i = 0; i < K*N; i++) B[i] = ((rand() % 2000) - 1000) / 100.0f;

    // 軟體參考答案 (雙精度計算, C_init = 0)
    double ref[M*N];
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            double acc = 0.0;
            for (int k = 0; k < K; k++) {
                acc += (double)A[i*K+k] * (double)B[k*N+j];
            }
            ref[i*N+j] = acc;
        }
    }

    printf("矩陣大小: %dx%d @ %dx%d = %dx%d (非 4 的倍數,會觸發 zero-padding)\n", M, K, K, N, M, N);
    printf("連線 %s ...\n", PORT);
    int fd = fpga_uart_open(PORT);
    if (fd < 0) { printf("開啟 UART 失敗\n"); return 1; }
    printf("連線成功\n\n");

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    int rc = fpga_matmul_tiled(fd, M, K, N, A, B, C);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    fpga_uart_close(fd);

    if (rc != 0) {
        printf("fpga_matmul_tiled 失敗, rc=%d\n", rc);
        return 1;
    }

    double elapsed = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;
    printf("完成,耗時 %.3f 秒 (共 %d 個 4x4 tile)\n\n", elapsed, 2*2*2);

    printf("=== 硬體結果 vs 軟體參考 ===\n");
    int errors = 0;
    for (int i = 0; i < M*N; i++) {
        float ref32 = (float)ref[i];
        float diff = fabsf(C[i] - ref32);
        const char *status = (diff < 1e-2f) ? "PASS" : "FAIL";
        if (diff >= 1e-2f) errors++;
        printf("[%s] idx=%2d (r%d,c%d)  hw=%12.6f  ref32=%12.6f  diff=%.6f\n",
               status, i, i/N, i%N, C[i], ref32, diff);
    }

    printf("\n");
    if (errors == 0) {
        printf("全部通過! %d/%d PASS (tiling 演算法驗證成功,含 zero-padding)\n", M*N, M*N);
    } else {
        printf("%d/%d 個結果不符\n", errors, M*N);
    }

    return errors == 0 ? 0 : 1;
}
