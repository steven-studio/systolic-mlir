#include "fpga_matmul4x4.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define PORT "/dev/ttyUSB1"

int main() {
    // 跟 verify_matmul_float.py 完全相同的測資 (row-major flatten)
    float A[16] = {
        0.5f,     1.25f,   -2.75f,  3.1f,
        -1.6f,    2.333f,   0.001f, -4.9f,
        3.14159f, -0.5f,    1.1f,   2.2f,
        0.0001f,  7.7f,    -3.333f, 0.618f
    };
    float B[16] = {
        1.1f,   -2.2f,     3.3f,   -4.4f,
        0.05f,   0.15f,   -0.25f,  0.35f,
        -1.0f,   2.71828f, -3.5f,   0.9f,
        10.1f,  -0.01f,    0.001f, -100.5f
    };
    float C_init[16] = {
        0.1f,   -0.2f,   0.3f,   -0.4f,
        1.5f,   -1.5f,   2.5f,   -2.5f,
        0.0f,    100.0f, -100.0f, 0.5f,
        -0.001f, 0.002f, -0.003f, 42.42f
    };

    // 軟體參考答案 (雙精度計算後降回 float32,確保跟硬體同精度比較)
    double ref[16];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            double acc = (double)C_init[i*4+j];
            for (int k = 0; k < 4; k++) {
                acc += (double)A[i*4+k] * (double)B[k*4+j];
            }
            ref[i*4+j] = acc;
        }
    }

    printf("連線 %s ...\n", PORT);
    int fd = fpga_uart_open(PORT);
    if (fd < 0) {
        printf("開啟 UART 失敗\n");
        return 1;
    }
    printf("連線成功, fd=%d\n", fd);

    float C_out[16];
    int rc = fpga_matmul4x4(fd, A, B, C_init, C_out);
    fpga_uart_close(fd);

    if (rc != 0) {
        printf("fpga_matmul4x4 失敗, rc=%d\n", rc);
        return 1;
    }

    printf("\n=== 硬體結果 vs 軟體參考 ===\n");
    int errors = 0;
    for (int i = 0; i < 16; i++) {
        float ref32 = (float)ref[i];
        float diff = fabsf(C_out[i] - ref32);
        const char *status = (diff < 1e-3f) ? "PASS" : "FAIL";
        if (diff >= 1e-3f) errors++;
        printf("[%s] idx=%2d  hw=%12.6f  ref32=%12.6f  diff=%.8f\n",
               status, i, C_out[i], ref32, diff);
    }

    printf("\n");
    if (errors == 0) {
        printf("全部通過! 16/16 PASS (C runtime library 驗證成功)\n");
    } else {
        printf("%d/16 個結果不符\n", errors);
    }

    return errors == 0 ? 0 : 1;
}
