/* fpga_matmul_fold.c -- see fpga_matmul_fold.h for the wire format.
 *
 * write_full/read_full are duplicated from fpga_matmul_rk_new.c rather than
 * shared, for the same reason that file gives: they are static there, and
 * this file is meant to be droppable into a standalone test with no
 * link-order surprises.
 */

#include "fpga_matmul_fold.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

/* ------------------------------------------------------------------ */
/* Blocking I/O helpers                                               */
/* ------------------------------------------------------------------ */

static int write_full(int fd, const uint8_t *buf, size_t n)
{
    size_t off = 0;

    while (off < n) {
        ssize_t w = write(fd, buf + off, n - off);

        if (w < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (w == 0)
            return -1;

        off += (size_t)w;
    }
    return 0;
}

/* A zero return means the port hit its VTIME timeout with nothing pending,
 * i.e. the board never answered. That is the failure mode to care about
 * here: a request of the wrong length leaves the FPGA waiting for bytes
 * that never come, and it surfaces as a timeout rather than as bad data. */
static int read_full(int fd, uint8_t *buf, size_t n)
{
    size_t off = 0;

    while (off < n) {
        ssize_t r = read(fd, buf + off, n - off);

        if (r < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (r == 0)
            return -1;

        off += (size_t)r;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Connection                                                         */
/* ------------------------------------------------------------------ */

/* tcflush() discards only what the kernel already holds. Bytes still inside
 * the USB-serial adapter's own FIFO are delivered afterwards and are then
 * read as the head of the next response, shifting an otherwise-correct
 * reply. That is not hypothetical: a stale pair of bytes left over from an
 * earlier run put every float in a 512-byte response two bytes out, which
 * decodes as denormal garbage rather than as an obvious framing error.
 *
 * Read until the line has genuinely been quiet for one VTIME period.
 * VTIME is in deciseconds and the minimum useful value is 1, so this costs
 * ~100 ms; it therefore belongs at open time, not per invocation. */
static void drain_input(int fd)
{
    struct termios saved, tmp;

    if (tcgetattr(fd, &saved) != 0)
        return;

    tmp = saved;
    tmp.c_cc[VMIN]  = 0;
    tmp.c_cc[VTIME] = 1;
    if (tcsetattr(fd, TCSANOW, &tmp) != 0)
        return;

    tcflush(fd, TCIFLUSH);

    uint8_t junk[256];
    for (;;) {
        ssize_t r = read(fd, junk, sizeof(junk));
        if (r <= 0)
            break;
    }

    tcsetattr(fd, TCSANOW, &saved);
    tcflush(fd, TCIFLUSH);
}

static speed_t baud_constant(int baud)
{
    switch (baud) {
    case 9600:    return B9600;
    case 19200:   return B19200;
    case 38400:   return B38400;
    case 57600:   return B57600;
    case 115200:  return B115200;
    case 230400:  return B230400;
    case 460800:  return B460800;
    case 921600:  return B921600;
    case 1000000: return B1000000;
    case 2000000: return B2000000;
    default:      return 0;
    }
}

int fold_open(const char *port, int baud)
{
    if (!port)
        port = getenv("FOLD_UART_PORT");
    if (!port)
        port = "/dev/ttyUSB2";

    if (baud <= 0) {
        const char *e = getenv("FOLD_UART_BAUD");
        baud = e ? atoi(e) : 115200;
    }

    speed_t sp = baud_constant(baud);
    if (sp == 0)
        return -1;

    int fd = open(port, O_RDWR | O_NOCTTY);
    if (fd < 0)
        return -1;

    struct termios tio;
    if (tcgetattr(fd, &tio) != 0) {
        close(fd);
        return -1;
    }

    cfmakeraw(&tio);
    cfsetispeed(&tio, sp);
    cfsetospeed(&tio, sp);

    tio.c_cflag |= (CLOCAL | CREAD);
    tio.c_cflag &= (tcflag_t)~CRTSCTS;

    /* Byte-at-a-time reads with a 5 s inter-byte timeout. read_full loops,
     * so this bounds the wait per chunk rather than per transaction. The
     * whole 512-byte response takes ~44 ms at 115200, so 5 s only ever
     * fires when the board is not answering at all. */
    tio.c_cc[VMIN]  = 0;
    tio.c_cc[VTIME] = 50;

    if (tcsetattr(fd, TCSANOW, &tio) != 0) {
        close(fd);
        return -1;
    }

    tcflush(fd, TCIOFLUSH);

    /* Anything the adapter was still holding from a previous program. */
    drain_input(fd);

    return fd;
}

void fold_close(int fd)
{
    if (fd >= 0)
        close(fd);
}

int fold_default_k_max(void)
{
    const char *e = getenv("FOLD_K_MAX");
    int v = e ? atoi(e) : 64;

    if (v < 16 || (v % FOLD_WINDOW) != 0)
        return 64;
    return v;
}

size_t fold_request_bytes(int k_max)
{
    if (k_max <= 0)
        return 0;
    /* Two matrices per window, 8x8 floats each, k_max/8 windows:
     *   (k_max / 8) * 2 * 64 * 4  =  k_max * 64 */
    return (size_t)FOLD_HDR_BYTES + (size_t)k_max * 64u;
}

/* ------------------------------------------------------------------ */
/* Packing                                                            */
/* ------------------------------------------------------------------ */

int fold_pack_request(uint8_t *buf, int k_max, int k_dim,
                      const float *A, const float *B)
{
    if (!buf || !A || !B)
        return -3;
    if (k_max < 16 || (k_max % FOLD_WINDOW) != 0)
        return -3;
    if (k_dim < 1 || k_dim > k_max)
        return -3;

    uint8_t *p = buf;

    /* uint32_t on x86 is already little-endian, so the copy is the
     * encoding. Spelled as a memcpy of a sized type rather than four
     * shifts so the width cannot be got wrong. */
    const uint32_t k_le = (uint32_t)k_dim;
    memcpy(p, &k_le, sizeof(k_le));
    p += sizeof(k_le);

    const int windows = k_max / FOLD_WINDOW;
    const float zero = 0.0f;

    for (int w = 0; w < windows; w++) {
        /* A window: [row][local k]. Row-major over an 8x8 block, so the
         * inner index walks k -- the same order the caller's A[r][k]
         * already has, but only 8 of them, so it is not one memcpy. */
        for (int r = 0; r < FOLD_R; r++) {
            for (int off = 0; off < FOLD_WINDOW; off++) {
                const int lk = w * FOLD_WINDOW + off;
                const float *src =
                    (lk < k_dim) ? &A[(size_t)r * (size_t)k_dim + (size_t)lk]
                                 : &zero;
                memcpy(p, src, sizeof(float));
                p += sizeof(float);
            }
        }

        /* B window: [local k][col]. Note the loop nest -- k outer, col
         * inner. Swapping them yields an identical-looking index
         * expression while emitting the bytes transposed, and the mistake
         * is invisible whenever B happens to be symmetric. */
        for (int off = 0; off < FOLD_WINDOW; off++) {
            const int lk = w * FOLD_WINDOW + off;
            for (int c = 0; c < FOLD_C; c++) {
                const float *src =
                    (lk < k_dim) ? &B[(size_t)lk * FOLD_C + (size_t)c]
                                 : &zero;
                memcpy(p, src, sizeof(float));
                p += sizeof(float);
            }
        }
    }

    return (int)(p - buf);
}

/* ------------------------------------------------------------------ */
/* One invocation                                                     */
/* ------------------------------------------------------------------ */

int fold_invoke(int fd, int k_max, int k_dim,
                const float *A, const float *B,
                float *C_ctx0, float *C_ctx1)
{
    if (fd < 0)
        return -3;

    const size_t req_len = fold_request_bytes(k_max);
    if (req_len == 0)
        return -3;

    uint8_t *req = (uint8_t *)malloc(req_len);
    if (!req)
        return -3;

    int packed = fold_pack_request(req, k_max, k_dim, A, B);
    if (packed < 0 || (size_t)packed != req_len) {
        free(req);
        return -3;
    }

    /* Stale bytes from an aborted previous transaction would be consumed
     * as this request's header. */
    tcflush(fd, TCIFLUSH);

    if (write_full(fd, req, req_len) != 0) {
        free(req);
        return -1;
    }
    free(req);

    uint8_t rx[FOLD_TX_BYTES];
    if (read_full(fd, rx, sizeof(rx)) != 0)
        return -2;

    /* The board sends exactly FOLD_TX_BYTES. Anything still pending means
     * this reply was not the one belonging to this request, so every
     * subsequent invocation would silently read shifted data. Cheap to
     * check -- VTIME 0 with VMIN 0 returns whatever is buffered without
     * waiting -- and it converts a class of silent corruption into a
     * reported error. */
    {
        struct termios saved, tmp;
        if (tcgetattr(fd, &saved) == 0) {
            tmp = saved;
            tmp.c_cc[VMIN]  = 0;
            tmp.c_cc[VTIME] = 0;
            if (tcsetattr(fd, TCSANOW, &tmp) == 0) {
                uint8_t extra[8];
                ssize_t n = read(fd, extra, sizeof(extra));
                tcsetattr(fd, TCSANOW, &saved);
                if (n > 0) {
                    tcflush(fd, TCIFLUSH);
                    return -4;
                }
            } else {
                tcsetattr(fd, TCSANOW, &saved);
            }
        }
    }

    const size_t half = FOLD_R * FOLD_C * sizeof(float);
    if (C_ctx0)
        memcpy(C_ctx0, rx, half);
    if (C_ctx1)
        memcpy(C_ctx1, rx + half, half);

    return 0;
}

/* ------------------------------------------------------------------ */
/* One 8x8 tile, arbitrary K                                          */
/* ------------------------------------------------------------------ */

int fold_matmul_8x8(int fd, int k_max, int K,
                    const float *A, const float *B, float *C)
{
    if (!A || !B || !C || K < 1)
        return -3;
    if (k_max < 16 || (k_max % FOLD_WINDOW) != 0)
        return -3;

    const int n = FOLD_R * FOLD_C;

    for (int i = 0; i < n; i++)
        C[i] = 0.0f;

    float c0[FOLD_R * FOLD_C];
    float c1[FOLD_R * FOLD_C];

    /* A is 8 x K, so one invocation's slice A[:, base:base+kd] is strided
     * and has to be gathered. B is K x 8, so its slice is contiguous. */
    float *a_slice = (float *)malloc((size_t)FOLD_R * (size_t)k_max
                                     * sizeof(float));
    if (!a_slice)
        return -3;

    for (int base = 0; base < K; base += k_max) {
        const int kd = (K - base < k_max) ? (K - base) : k_max;

        for (int r = 0; r < FOLD_R; r++)
            memcpy(&a_slice[(size_t)r * (size_t)kd],
                   &A[(size_t)r * (size_t)K + (size_t)base],
                   (size_t)kd * sizeof(float));

        const float *b_slice = &B[(size_t)base * FOLD_C];

        int rc = fold_invoke(fd, k_max, kd, a_slice, b_slice, c0, c1);
        if (rc != 0) {
            free(a_slice);
            return rc;
        }

        /* The array has two accumulator contexts and no C_init, so the
         * cross-context and cross-invocation sums both happen here. */
        for (int i = 0; i < n; i++)
            C[i] += c0[i] + c1[i];
    }

    free(a_slice);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Arbitrary shape                                                    */
/* ------------------------------------------------------------------ */

int fold_matmul_tiled(int fd, int k_max, int M, int K, int N,
                      const float *A, const float *B, float *C)
{
    if (!A || !B || !C || M < 1 || K < 1 || N < 1)
        return -3;
    if (k_max < 16 || (k_max % FOLD_WINDOW) != 0)
        return -3;

    const int tilesM = (M + FOLD_R - 1) / FOLD_R;
    const int tilesN = (N + FOLD_C - 1) / FOLD_C;

    float *a_tile = (float *)calloc((size_t)FOLD_R * (size_t)K, sizeof(float));
    float *b_tile = (float *)calloc((size_t)K * FOLD_C, sizeof(float));
    if (!a_tile || !b_tile) {
        free(a_tile);
        free(b_tile);
        return -3;
    }

    float c_tile[FOLD_R * FOLD_C];
    int rc = 0;

    for (int ti = 0; ti < tilesM && rc == 0; ti++) {
        /* Gather this row-block of A, zero-padding the ragged edge. */
        memset(a_tile, 0, (size_t)FOLD_R * (size_t)K * sizeof(float));
        for (int r = 0; r < FOLD_R; r++) {
            const int gr = ti * FOLD_R + r;
            if (gr < M)
                memcpy(&a_tile[(size_t)r * (size_t)K],
                       &A[(size_t)gr * (size_t)K],
                       (size_t)K * sizeof(float));
        }

        for (int tj = 0; tj < tilesN && rc == 0; tj++) {
            memset(b_tile, 0, (size_t)K * FOLD_C * sizeof(float));
            for (int k = 0; k < K; k++)
                for (int c = 0; c < FOLD_C; c++) {
                    const int gc = tj * FOLD_C + c;
                    if (gc < N)
                        b_tile[(size_t)k * FOLD_C + (size_t)c] =
                            B[(size_t)k * (size_t)N + (size_t)gc];
                }

            rc = fold_matmul_8x8(fd, k_max, K, a_tile, b_tile, c_tile);
            if (rc != 0)
                break;

            for (int r = 0; r < FOLD_R; r++) {
                const int gr = ti * FOLD_R + r;
                if (gr >= M)
                    continue;
                for (int c = 0; c < FOLD_C; c++) {
                    const int gc = tj * FOLD_C + c;
                    if (gc < N)
                        C[(size_t)gr * (size_t)N + (size_t)gc] =
                            c_tile[r * FOLD_C + c];
                }
            }
        }
    }

    free(a_tile);
    free(b_tile);
    return rc;
}

/* ------------------------------------------------------------------ */
/* Default connection                                                 */
/* ------------------------------------------------------------------ */

static int g_fold_fd = -1;

int fold_matmul_tiled_auto(int M, int K, int N,
                           const float *A, const float *B, float *C)
{
    if (g_fold_fd < 0) {
        g_fold_fd = fold_open(NULL, 0);
        if (g_fold_fd < 0)
            return -3;
    }
    return fold_matmul_tiled(g_fold_fd, fold_default_k_max(),
                             M, K, N, A, B, C);
}
