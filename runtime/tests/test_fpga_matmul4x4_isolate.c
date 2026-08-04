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

// 從鏈式測試裡實際觀察到會出錯的 C_init 快照 (依 idx=5 那一列附近的
// 觀察值反推出完整的 16 個元素的 C_init -- 這裡用一個近似但足夠測試
// 用的合成值: 把單一元素設成觀察到的量級, 其餘沿用原本 C_init0 的比例)
static void make_test_cinit(float out[16], float target_val, int target_idx) {
    static const float C_init0[16] = {
        0.1f,   -0.2f,   0.3f,   -0.4f,
        1.5f,   -1.5f,   2.5f,   -2.5f,
        0.0f,    100.0f, -100.0f, 0.5f,
        -0.001f, 0.002f, -0.003f, 42.42f
    };
    memcpy(out, C_init0, sizeof(float) * 16);
    out[target_idx] = target_val;
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

int main(int argc, char **argv) {
    int repeats = (argc > 1) ? atoi(argv[1]) : 10;

    printf("連線 %s ...\n", PORT);
    int fd = fpga_uart_open(PORT);
    if (fd < 0) { printf("開啟 UART 失敗\n"); return 1; }
    printf("連線成功, fd=%d\n\n", fd);

    // 這幾個值是從鏈式測試 log 裡 idx=5 (等同 flat index 5, 也就是 i=1,j=1)
    // 附近觀察到的量級, 挑幾個測試點, 每個獨立重複呼叫 repeats 次
    struct { float val; int idx; const char *label; } cases[] = {
        {825.978149f, 5, "iter210 對應值"},
        {829.899902f, 5, "iter211 對應值"},
        {833.821655f, 5, "iter212 對應值"},
        {3.558594f,   9, "iter23 對應值"},
        {-0.333271f, 10, "iter14 對應值"},
        {1.5f,        5, "原始 C_init0 基準值 (應該完全乾淨)"},
    };
    int n_cases = sizeof(cases) / sizeof(cases[0]);

    for (int c = 0; c < n_cases; c++) {
        float C_init[16];
        make_test_cinit(C_init, cases[c].val, cases[c].idx);
        double ref[16];
        compute_ref(C_init, ref);

        int n_bad = 0;
        long worst_ulp = 0;
        for (int r = 0; r < repeats; r++) {
            float C_out[16];
            int rc = fpga_matmul4x4(fd, A, B, C_init, C_out);
            if (rc != 0) { printf("  呼叫失敗 rc=%d\n", rc); continue; }
            long u = ulp_diff_f32(C_out[cases[c].idx], (float)ref[cases[c].idx]);
            if (u > worst_ulp) worst_ulp = u;
            if (u >= 100) n_bad++;
        }
        printf("[%s] C_init[%d]=%.6f -> %d/%d 次出錯 (>=100 ULP), 最大 ULP=%ld\n",
               cases[c].label, cases[c].idx, cases[c].val, n_bad, repeats, worst_ulp);
    }

    fpga_uart_close(fd);
    return 0;
}
