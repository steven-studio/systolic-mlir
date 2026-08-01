#include "fpga_matmul4x4.h"
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

int fpga_uart_open(const char *port) {
    int fd = open(port, O_RDWR | O_NOCTTY);
    if (fd < 0) { perror("fpga_uart_open: open"); return -1; }

    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) { perror("fpga_uart_open: tcgetattr"); close(fd); return -1; }

    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CREAD | CLOCAL;
    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO;
    tty.c_lflag &= ~ECHOE;
    tty.c_lflag &= ~ISIG;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
    tty.c_oflag &= ~OPOST;
    tty.c_oflag &= ~ONLCR;
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 50;  // 5 秒逾時 (單位 0.1s)

    if (tcsetattr(fd, TCSANOW, &tty) != 0) { perror("fpga_uart_open: tcsetattr"); close(fd); return -1; }
    tcflush(fd, TCIOFLUSH);
    return fd;
}

void fpga_uart_close(int fd) {
    if (fd >= 0) close(fd);
}

// 確保寫滿 n bytes
static int write_full(int fd, const uint8_t *buf, size_t n)
{
    size_t off = 0;

    while (off < n) {
        ssize_t w = write(fd, buf + off, n - off);

        if (w < 0) {
            /*
             * Ctrl+C 只中斷 system call，不中斷目前的 UART 封包。
             * 完成本次 transaction 後，再由 main() 檢查
             * stop_requested 並退出。
             */
            if (errno == EINTR)
                continue;

            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue;

            return -1;
        }

        /*
         * 對一般 blocking UART，write() 通常不會回傳 0。
         * 若真的發生，視為寫入失敗，避免無限迴圈。
         */
        if (w == 0)
            return -1;

        off += (size_t)w;
    }

    return 0;
}

// 確保讀滿 n bytes,逾時則失敗
static int read_full(int fd, uint8_t *buf, size_t n)
{
    size_t off = 0;

    while (off < n) {
        ssize_t r = read(fd, buf + off, n - off);

        if (r < 0) {
            /*
             * 收到 Ctrl+C 時，繼續完成這次 UART transaction，
             * 避免 FPGA 與 host 的固定長度封包失步。
             */
            if (errno == EINTR)
                continue;

            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue;

            return -1;
        }

        /*
         * VMIN=0、VTIME=50：
         * 5 秒內沒有收到任何資料時，read() 回傳 0。
         */
        if (r == 0)
            return -1;

        off += (size_t)r;
    }

    return 0;
}

static double ts_diff_ms(struct timespec a, struct timespec b) {
    return (b.tv_sec - a.tv_sec) * 1000.0 + (b.tv_nsec - a.tv_nsec) / 1e6;
}

#define MATMUL_DIM       4
#define MATRIX_ELEMENTS  (MATMUL_DIM * MATMUL_DIM)
#define MATRIX_BYTES     (MATRIX_ELEMENTS * sizeof(float))
#define REQUEST_BYTES    (3 * MATRIX_BYTES)

int fpga_matmul4x4(int fd,
                   const float A[MATRIX_ELEMENTS],
                   const float B[MATRIX_ELEMENTS],
                   const float C_init[MATRIX_ELEMENTS],
                   float C_out[MATRIX_ELEMENTS])
{
    uint8_t tx[REQUEST_BYTES];

    memcpy(tx,                    A,      MATRIX_BYTES);
    memcpy(tx + MATRIX_BYTES,     B,      MATRIX_BYTES);
    memcpy(tx + 2 * MATRIX_BYTES, C_init, MATRIX_BYTES);

    /*
     * FPGA_MATMUL_TIMING 環境變數控制是否印出每次 tile 呼叫的計時訊息。
     */
    static int timing_enabled = -1;

    if (timing_enabled == -1)
        timing_enabled = getenv("FPGA_MATMUL_TIMING") != NULL;

    struct timespec t_start;
    struct timespec t_after_tx;
    struct timespec t_after_rx;

    if (timing_enabled)
        clock_gettime(CLOCK_MONOTONIC, &t_start);

    if (write_full(fd, tx, sizeof(tx)) != 0)
        return -1;

    if (timing_enabled)
        clock_gettime(CLOCK_MONOTONIC, &t_after_tx);

    uint8_t rx[MATRIX_BYTES];

    if (read_full(fd, rx, sizeof(rx)) != 0)
        return -2;

    if (timing_enabled) {
        clock_gettime(CLOCK_MONOTONIC, &t_after_rx);

        double tx_ms =
            ts_diff_ms(t_start, t_after_tx);

        double rx_ms =
            ts_diff_ms(t_after_tx, t_after_rx);

        double total_ms =
            ts_diff_ms(t_start, t_after_rx);

        fprintf(
            stderr,
            "[tile] tx=%.3fms "
            "rx_wait_compute=%.3fms "
            "total=%.3fms\n",
            tx_ms,
            rx_ms,
            total_ms
        );
    }

    memcpy(C_out, rx, sizeof(rx));

    /*
     * Optional diagnostic delay.
     */
    static long delay_us = -1;

    if (delay_us == -1) {
        const char *env = getenv("FPGA_MATMUL_DELAY_US");
        delay_us = env ? atol(env) : 0;
    }

    if (delay_us > 0)
        usleep((useconds_t)delay_us);

    return 0;
}