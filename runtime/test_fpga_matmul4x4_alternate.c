#include "fpga_matmul4x4.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#define PORT "/dev/ttyUSB1"

static const float A[16] = {
    0.5f,     1.25f,   -2.75f,  3.1f,
    -1.6f,    2.333f,   0.001f, -4.9f,
    3.14159f, -0.5f,    1.1f,   2.2f,
    0.0001f,  7.7f,    -3.333f, 0.618f
};
static const float B[16] = {
    1.1f,   -2.2f,     3.3f,   -4.4f,
    0.05f,   0.15f,   -0.25f,  0.35f,
    -1.0f,   2.71828f, -3.5f,   0.9f,
    10.1f,  -0.01f,    0.001f, -100.5f
};

static long ulp_diff_f32(float a, float b) {
    int32_t ai, bi;
    memcpy(&ai, &a, sizeof(ai));
    memcpy(&bi, &b, sizeof(bi));
    int64_t ai64 = (ai < 0) ? (int64_t)0x80000000LL - ai : ai;
    int64_t bi64 = (bi < 0) ? (int64_t)0x80000000LL - bi : bi;
    int64_t d = ai64 - bi64;
    return (long)(d < 0 ? -d : d);
}

static void compute_ref(const float *Cin, double *ref) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            double acc = (double)Cin[i*4+j];
            for (int k = 0; k < 4; k++) {
                acc += (double)A[i*4+k] * (double)B[k*4+j];
            }
            ref[i*4+j] = acc;
        }
    }
}

static const float C_init_X[16] = {
    0.1f,   -0.2f,   0.3f,   -0.4f,
    1.5f,   825.978149f,   2.5f,   -2.5f,
    0.0f,    100.0f, -100.0f, 0.5f,
    -0.001f, 0.002f, -0.003f, 42.42f
};
static const float C_init_Y[16] = {
    0.1f,   -0.2f,   0.3f,   -0.4f,
    1.5f,   3.558594f,   2.5f,   -2.5f,
    0.0f,    100.0f, -100.0f, 0.5f,
    -0.001f, 0.002f, -0.003f, 42.42f
};

int main(int argc, char **argv) {
    int n_iters = (argc > 1) ? atoi(argv[1]) : 200;

    printf("連線 %s ...\n", PORT);
    int fd = fpga_uart_open(PORT);
    if (fd < 0) { printf("開啟 UART 失敗\n"); return 1; }
    printf("連線成功, fd=%d\n\n", fd);

    double refX[16], refY[16];
    compute_ref(C_init_X, refX);
    compute_ref(C_init_Y, refY);

    printf("=== 在兩個固定值之間交替呼叫 %d 次 (每次輸入都跟前一次不同) ===\n", n_iters);
    int n_bad = 0;
    long worst_ulp = 0;
    for (int i = 0; i < n_iters; i++) {
        const float *cin = (i % 2 == 0) ? C_init_X : C_init_Y;
        const double *ref = (i % 2 == 0) ? refX : refY;
        float C_out[16];
        int rc = fpga_matmul4x4(fd, A, B, cin, C_out);
        if (rc != 0) { printf("  [iter %d] 呼叫失敗 rc=%d\n", i, rc); continue; }
        long u = ulp_diff_f32(C_out[5], (float)ref[5]);
        if (u > worst_ulp) worst_ulp = u;
        if (u >= 100) {
            n_bad++;
            if (n_bad <= 20)
                printf("  [iter %5d] GLITCH (輸入=%s) hw=%.6f ref=%.6f ulp=%ld\n",
                       i, (i%2==0)?"X":"Y", C_out[5], (float)ref[5], u);
        }
    }
    printf("\n結果: %d/%d 次呼叫出現 glitch (%.3f%%), 最大 ULP=%ld\n",
           n_bad, n_iters, 100.0 * n_bad / n_iters, worst_ulp);

    fpga_uart_close(fd);
    return 0;
}
