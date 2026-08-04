#include "fpga_matmul4x4.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#define PORT "/dev/ttyUSB1"

// 跟 test_fpga_matmul4x4.c 完全相同的測資
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
static const float C_init0[16] = {
    0.1f,   -0.2f,   0.3f,   -0.4f,
    1.5f,   -1.5f,   2.5f,   -2.5f,
    0.0f,    100.0f, -100.0f, 0.5f,
    -0.001f, 0.002f, -0.003f, 42.42f
};

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

static int max_diff_idx(const float *C_out, const double *ref, float *max_diff_out) {
    int worst = -1;
    float worst_diff = -1.0f;
    for (int i = 0; i < 16; i++) {
        float diff = fabsf(C_out[i] - (float)ref[i]);
        if (diff > worst_diff) { worst_diff = diff; worst = i; }
    }
    *max_diff_out = worst_diff;
    return worst;
}

// ULP 距離,跟 debug_single_row.py 的 ulp_diff() 邏輯一致,
// 才能跟之前 conv2d sweep 的誤差量級放在同一把尺上比較。
static long ulp_diff_f32(float a, float b) {
    int32_t ai, bi;
    memcpy(&ai, &a, sizeof(ai));
    memcpy(&bi, &b, sizeof(bi));
    int64_t ai64 = (ai < 0) ? (int64_t)0x80000000LL - ai : ai;
    int64_t bi64 = (bi < 0) ? (int64_t)0x80000000LL - bi : bi;
    int64_t d = ai64 - bi64;
    return (long)(d < 0 ? -d : d);
}

static int max_ulp_idx(const float *C_out, const double *ref, long *max_ulp_out) {
    int worst = -1;
    long worst_ulp = -1;
    for (int i = 0; i < 16; i++) {
        long u = ulp_diff_f32(C_out[i], (float)ref[i]);
        if (u > worst_ulp) { worst_ulp = u; worst = i; }
    }
    *max_ulp_out = worst_ulp;
    return worst;
}

// 模式 1: 每次呼叫都用完全相同、固定的 A, B, C_init0。
// 理論上每次都應該得到完全一樣的結果 (bit-for-bit, 因為輸入完全沒變)。
// 任何一次結果跟第一次不一樣,或跟參考值差太多,都是硬體/協定層面的 glitch。
static void run_fixed_input_test(int fd, int n_iters) {
    printf("\n=== 模式 1: 固定輸入,連續呼叫 %d 次 ===\n", n_iters);
    double ref[16];
    compute_ref(C_init0, ref);

    int n_bad = 0;
    float worst_overall = 0.0f;
    int worst_iter = -1;

    for (int iter = 0; iter < n_iters; iter++) {
        float C_out[16];
        int rc = fpga_matmul4x4(fd, A, B, C_init0, C_out);
        if (rc != 0) {
            printf("  [iter %5d] fpga_matmul4x4 失敗, rc=%d\n", iter, rc);
            n_bad++;
            continue;
        }
        float worst_diff;
        int worst_i = max_diff_idx(C_out, ref, &worst_diff);
        if (worst_diff >= 1e-3f) {
            n_bad++;
            if (worst_diff > worst_overall) { worst_overall = worst_diff; worst_iter = iter; }
            if (n_bad <= 20) {  // 只印前 20 個,避免洗版
                printf("  [iter %5d] GLITCH idx=%2d hw=%.6f ref=%.6f diff=%.6f\n",
                       iter, worst_i, C_out[worst_i], (float)ref[worst_i], worst_diff);
            }
        }
    }

    printf("\n結果: %d/%d 次呼叫出現 glitch (%.3f%%)\n",
           n_bad, n_iters, 100.0 * n_bad / n_iters);
    if (worst_iter >= 0) {
        printf("最嚴重的一次在 iter=%d, diff=%.6f\n", worst_iter, worst_overall);
    }
}

// 模式 2: 模擬真實的 conv2d/matmul_tiled 使用情境 -- 把上一次的 C_out
// 當作下一次呼叫的 C_init (累加鏈),A、B 保持不變。
// 這模擬的是 fpga_matmul_tiled.c 裡 K_tiles 迴圈的真實資料流。
//
// 重要修正: 參考值改成每一步都用 float32 捨入 (ref_acc_f32),
// 模擬「假設硬體是完美 IEEE-754 float32」該有的累積結果 --
// 而不是全程用 double 精度、只在比較時才轉 float32。
// 後者會讓正常的、預期中的逐步捨入誤差被誤判成硬體 bug。
static void run_chained_test(int fd, int n_iters) {
    printf("\n=== 模式 2: 累加鏈 (模擬 K_tiles 迴圈),連續 %d 次 ===\n", n_iters);

    float C_acc[16];
    float ref_acc_f32[16];   // 「完美硬體」該有的結果: 每步都用 float32 捨入
    memcpy(C_acc, C_init0, sizeof(C_acc));
    memcpy(ref_acc_f32, C_init0, sizeof(ref_acc_f32));

    int n_bad = 0;
    for (int iter = 0; iter < n_iters; iter++) {
        // 「完美硬體」參考: 用 double 算這一步的乘加,但立刻捨入回 float32,
        // 這樣才是跟硬體同一種精度演化路徑的公平比較。
        float ref_step_f32[16];
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                double acc = (double)ref_acc_f32[i*4+j];
                for (int k = 0; k < 4; k++) {
                    acc += (double)A[i*4+k] * (double)B[k*4+j];
                }
                ref_step_f32[i*4+j] = (float)acc;  // 每步捨入一次,模擬硬體行為
            }
        }

        float C_out[16];
        int rc = fpga_matmul4x4(fd, A, B, C_acc, C_out);
        if (rc != 0) {
            printf("  [iter %5d] fpga_matmul4x4 失敗, rc=%d\n", iter, rc);
            n_bad++;
            break;
        }

        long worst_ulp = 0;
        int worst_i = -1;
        for (int i = 0; i < 16; i++) {
            long u = ulp_diff_f32(C_out[i], ref_step_f32[i]);
            if (u > worst_ulp) { worst_ulp = u; worst_i = i; }
        }
        // 現在兩邊都是「每步捨入一次的 float32 鏈」,理論上應該幾乎完全一致
        // (頂多 1~2 ULP 的正常浮點捨入模式差異),用比較嚴格的門檻。
        if (worst_ulp >= 20) {
            n_bad++;
            if (n_bad <= 30) {
                printf("  [iter %5d] GLITCH idx=%2d hw=%.6f ref_f32=%.6f ulp=%ld\n",
                       iter, worst_i, C_out[worst_i], ref_step_f32[worst_i], worst_ulp);
            }
        }

        memcpy(C_acc, C_out, sizeof(C_acc));
        memcpy(ref_acc_f32, ref_step_f32, sizeof(ref_acc_f32));
    }

    printf("\n結果: %d/%d 次呼叫出現 glitch (>=20 ULP, %.3f%%)\n",
           n_bad, n_iters, 100.0 * n_bad / n_iters);
    printf("最終累加值 (硬體) vs (逐步 float32 捨入參考) 的差距:\n");
    for (int i = 0; i < 4; i++) {
        printf("  ");
        for (int j = 0; j < 4; j++) {
            printf("%12.4f ", C_acc[i*4+j] - ref_acc_f32[i*4+j]);
        }
        printf("\n");
    }
}

int main(int argc, char **argv) {
    int n_iters = (argc > 1) ? atoi(argv[1]) : 500;

    printf("連線 %s ...\n", PORT);
    int fd = fpga_uart_open(PORT);
    if (fd < 0) {
        printf("開啟 UART 失敗\n");
        return 1;
    }
    printf("連線成功, fd=%d\n", fd);

    run_fixed_input_test(fd, n_iters);
    run_chained_test(fd, n_iters);

    fpga_uart_close(fd);
    return 0;
}
