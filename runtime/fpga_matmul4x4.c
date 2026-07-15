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
    if (fd < 0) return -1;

    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) { close(fd); return -1; }

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

    if (tcsetattr(fd, TCSANOW, &tty) != 0) { close(fd); return -1; }
    tcflush(fd, TCIOFLUSH);
    return fd;
}

void fpga_uart_close(int fd) {
    if (fd >= 0) close(fd);
}

// 確保寫滿 n bytes
static int write_full(int fd, const uint8_t *buf, size_t n) {
    size_t off = 0;
    while (off < n) {
        ssize_t w = write(fd, buf + off, n - off);
        if (w <= 0) return -1;
        off += (size_t)w;
    }
    return 0;
}

// 確保讀滿 n bytes,逾時則失敗
static int read_full(int fd, uint8_t *buf, size_t n) {
    size_t off = 0;
    while (off < n) {
        ssize_t r = read(fd, buf + off, n - off);
        if (r < 0) {
            if (errno == EAGAIN || errno == EINTR) continue;
            return -1;
        }
        if (r == 0) return -1;  // VTIME 逾時,read 回傳 0
        off += (size_t)r;
    }
    return 0;
}

static double ts_diff_ms(struct timespec a, struct timespec b) {
    return (b.tv_sec - a.tv_sec) * 1000.0 + (b.tv_nsec - a.tv_nsec) / 1e6;
}

int fpga_matmul4x4(int fd, const float A[16], const float B[16],
                    const float C_init[16], float C_out[16]) {
    uint8_t tx[192];
    memcpy(tx,        A,      64);
    memcpy(tx + 64,   B,      64);
    memcpy(tx + 128,  C_init, 64);

    // FPGA_MATMUL_TIMING 環境變數控制是否印出每次 tile 呼叫的計時訊息,
    // 平常跑其他測試/正式流程不設這個變數就不會印,不會洗版。
    static int timing_enabled = -1;
    if (timing_enabled == -1) {
        timing_enabled = getenv("FPGA_MATMUL_TIMING") != NULL;
    }

    struct timespec t_start, t_after_tx, t_after_rx;
    if (timing_enabled) clock_gettime(CLOCK_MONOTONIC, &t_start);

    if (write_full(fd, tx, 192) != 0) return -1;

    if (timing_enabled) clock_gettime(CLOCK_MONOTONIC, &t_after_tx);

    uint8_t rx[64];
    if (read_full(fd, rx, 64) != 0) return -2;

    if (timing_enabled) {
        clock_gettime(CLOCK_MONOTONIC, &t_after_rx);
        double tx_ms = ts_diff_ms(t_start, t_after_tx);
        double rx_ms = ts_diff_ms(t_after_tx, t_after_rx);
        double total_ms = ts_diff_ms(t_start, t_after_rx);
        fprintf(stderr, "[tile] tx=%.3fms rx_wait_compute=%.3fms total=%.3fms\n",
                tx_ms, rx_ms, total_ms);
    }

    memcpy(C_out, rx, 64);
    return 0;
}
