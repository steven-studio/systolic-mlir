#include "fpga_matmul4x4.h"

#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#define PORT "/dev/ttyUSB1"

static volatile sig_atomic_t stop_requested = 0;

static void handle_sigint(int signo)
{
    (void)signo;
    stop_requested = 1;
}

static int install_signal_handlers(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);

    /*
     * 不設定 SA_RESTART。
     *
     * fpga_matmul4x4.c 內的 read_full() 遇到 EINTR 時會繼續，
     * 因此目前的 UART transaction 仍會完整完成。
     * main() 會在 transaction 結束後檢查 stop_requested 並退出。
     */
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) != 0) {
        perror("sigaction(SIGINT)");
        return -1;
    }

    return 0;
}

int main(void)
{
    if (install_signal_handlers() != 0)
        return 1;

    /*
     * 與 verify_matmul_float.py 相同的測試資料。
     * 所有矩陣皆以 row-major 順序展平。
     */
    const float A[16] = {
         0.5f,      1.25f,   -2.75f,   3.1f,
        -1.6f,      2.333f,   0.001f, -4.9f,
         3.14159f, -0.5f,     1.1f,    2.2f,
         0.0001f,   7.7f,    -3.333f,  0.618f
    };

    const float B[16] = {
         1.1f,  -2.2f,      3.3f,   -4.4f,
         0.05f,  0.15f,    -0.25f,   0.35f,
        -1.0f,   2.71828f, -3.5f,    0.9f,
        10.1f,  -0.01f,     0.001f, -100.5f
    };

    const float C_init[16] = {
         0.1f,   -0.2f,     0.3f,    -0.4f,
         1.5f,   -1.5f,     2.5f,    -2.5f,
         0.0f,  100.0f,  -100.0f,     0.5f,
        -0.001f,  0.002f,   -0.003f, 42.42f
    };

    /*
     * 使用 double 計算軟體參考答案，
     * 最後再轉回 float32 與硬體結果比較。
     */
    double reference[16];

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            double accumulator = (double)C_init[i * 4 + j];

            for (int k = 0; k < 4; ++k) {
                accumulator +=
                    (double)A[i * 4 + k] *
                    (double)B[k * 4 + j];
            }

            reference[i * 4 + j] = accumulator;
        }
    }

    if (stop_requested) {
        fprintf(stderr, "\n收到 Ctrl+C，停止執行。\n");
        return 130;
    }

    printf("連線 %s ...\n", PORT);

    int fd = fpga_uart_open(PORT);

    if (fd < 0) {
        fprintf(stderr, "開啟 UART 失敗\n");
        return 1;
    }

    printf("連線成功, fd=%d\n", fd);

    float C_out[16];

    int rc = fpga_matmul4x4(
        fd,
        A,
        B,
        C_init,
        C_out
    );

    /*
     * 無論成功或失敗，都先關閉 UART。
     */
    fpga_uart_close(fd);

    /*
     * 舊版 runtime 在 EINTR 時會繼續完成 transaction，
     * 因此 Ctrl+C 會在這裡被處理。
     */
    if (stop_requested) {
        fprintf(stderr, "\n收到 Ctrl+C，停止執行。\n");
        return 130;
    }

    /*
     * 舊版 fpga_matmul4x4() 回傳值：
     *   0  成功
     *  -1  UART write 失敗
     *  -2  UART read 失敗或逾時
     */
    if (rc != 0) {
        fprintf(
            stderr,
            "fpga_matmul4x4 失敗, rc=%d\n",
            rc
        );
        return 1;
    }

    printf("\n=== 硬體結果 vs 軟體參考 ===\n");

    int errors = 0;

    for (int i = 0; i < 16; ++i) {
        float reference_f32 = (float)reference[i];
        float difference = fabsf(C_out[i] - reference_f32);

        const char *status =
            difference < 1e-3f ? "PASS" : "FAIL";

        if (difference >= 1e-3f)
            ++errors;

        printf(
            "[%s] idx=%2d  hw=%12.6f  "
            "ref32=%12.6f  diff=%.8f\n",
            status,
            i,
            C_out[i],
            reference_f32,
            difference
        );
    }

    printf("\n");

    if (errors == 0) {
        printf(
            "全部通過! 16/16 PASS "
            "(C runtime library 驗證成功)\n"
        );
        return 0;
    }

    printf("%d/16 個結果不符\n", errors);
    return 1;
}