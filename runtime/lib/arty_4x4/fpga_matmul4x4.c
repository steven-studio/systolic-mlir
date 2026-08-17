#include "fpga_matmul4x4.h"
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <glob.h>
#include <stdlib.h>

static speed_t baud_const(int baud) {
    // Only the rates the FPGA side can divide cleanly are listed. The UART
    // modules run on a 20 MHz clock, so CLKS_PER_BIT = 20e6/baud must be an
    // integer: 115200 gives 173 (+0.35%, fine), 1 M gives 20, 2 M gives 10.
    switch (baud) {
        case 9600:    return B9600;
        case 115200:  return B115200;
        case 230400:  return B230400;
        case 460800:  return B460800;
        case 500000:  return B500000;
        case 921600:  return B921600;
        case 1000000: return B1000000;
        case 2000000: return B2000000;
        default:      return B115200;
    }
}

/* 依序嘗試候選埠,回傳第一個開得起來的。
 *
 * /dev/ttyUSB<N> 的編號由 OS 列舉時決定,重燒、換孔、接第二塊板都會變動,
 * 因此不能寫死,也不該要求使用者每次以環境變數指定。優先順序:
 *   1. FPGA_UART_PORT      -- 保留手動覆寫,平常不需要
 *   2. 呼叫端指定的 port
 *   3. /dev/serial/by-id/  -- udev 依 USB 序號建立,重新列舉也不會變
 *   4. /dev/ttyUSB* / ttyACM*
 * 全部失敗時列出試過哪些,以及當下實際存在哪些。 */
static int uart_try_open(const char *p)
{
    if (!p || !*p)
        return -1;
    return open(p, O_RDWR | O_NOCTTY);
}

static int uart_open_any(const char *port)
{
    static const char *pats[] = {
        "/dev/serial/by-id/*Digilent*",
        "/dev/serial/by-id/*FTDI*",
        "/dev/ttyUSB*",
        "/dev/ttyACM*",
    };
    const char *env = getenv("FPGA_UART_PORT");
    int fd;

    if ((fd = uart_try_open(env))  >= 0) return fd;
    if ((fd = uart_try_open(port)) >= 0) return fd;

    for (size_t i = 0; i < sizeof(pats) / sizeof(pats[0]); i++) {
        glob_t g;
        if (glob(pats[i], 0, NULL, &g) == 0) {
            for (size_t j = 0; j < g.gl_pathc; j++) {
                fd = uart_try_open(g.gl_pathv[j]);
                if (fd >= 0) { globfree(&g); return fd; }
            }
        }
        globfree(&g);
    }

    fprintf(stderr,
        "fpga_uart_open: 找不到可用的序列埠。\n"
        "  FPGA_UART_PORT = %s,呼叫端指定 = %s\n"
        "  另已嘗試 /dev/serial/by-id/、/dev/ttyUSB*、/dev/ttyACM*\n"
        "  目前實際存在:",
        env ? env : "(未設)", port ? port : "(無)");
    {
        glob_t g; int any = 0;
        if (glob("/dev/ttyUSB*", 0, NULL, &g) == 0)
            for (size_t j = 0; j < g.gl_pathc; j++) {
                fprintf(stderr, " %s", g.gl_pathv[j]); any = 1;
            }
        globfree(&g);
        if (!any)
            fprintf(stderr, " (沒有 /dev/ttyUSB*,板子可能沒接或尚未燒錄)");
        fprintf(stderr, "\n");
    }
    return -1;
}

int fpga_uart_open_baud(const char *port, int baud) {
    int fd = uart_open_any(port);
    if (fd < 0) return -1;

    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) { perror("fpga_uart_open: tcgetattr"); close(fd); return -1; }

    speed_t sp = baud_const(baud);
    cfsetospeed(&tty, sp);
    cfsetispeed(&tty, sp);
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

int fpga_uart_open(const char *port) {
    // Historical entry point: the Arty bitstream is 115200.
    return fpga_uart_open_baud(port, 115200);
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