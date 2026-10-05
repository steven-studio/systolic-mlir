#include "dpti_fpga_transport.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

namespace {

constexpr int HW_N = 8;
constexpr int HW_K_MAX = 16;

constexpr size_t FRAME_START_BYTES = 4;
constexpr size_t HEADER_BYTES = 4;
constexpr size_t FRAME_END_BYTES = 4;

constexpr size_t RX_BYTES =
    static_cast<size_t>(HW_K_MAX) * 8 * HW_N;

constexpr size_t TX_BYTES =
    sizeof(float) * HW_N * HW_N;

/*
 * These byte orders match systolic_uart_top.sv:
 *
 *   FRAME_START = 32'hA55A_C33C
 *   sync_next   = {rx_byte, sync_sr[31:8]}
 *
 * Therefore the UART wire order is:
 *
 *   START: 3C C3 5A A5
 *   END:   C3 3C A5 5A
 */
static const uint8_t kFrameStart[FRAME_START_BYTES] = {
    0x3C, 0xC3, 0x5A, 0xA5
};

static const uint8_t kFrameEnd[FRAME_END_BYTES] = {
    0xC3, 0x3C, 0xA5, 0x5A
};

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

        off += static_cast<size_t>(w);
    }

    return 0;
}

static int read_full(int fd, uint8_t *buf, size_t n)
{
    size_t off = 0;

    while (off < n) {
        ssize_t r = read(fd, buf + off, n - off);

        fprintf(stderr,
                "[DPTI TRANSPORT] read(fd=%d, want=%zu) -> %zd, errno=%d (%s)\\n",
                fd, n - off, r, errno, strerror(errno));

        if (r < 0) {
            if (errno == EINTR || errno == EAGAIN)
                continue;
            return -1;
        }

        if (r == 0) {
            fprintf(stderr,
                    "[DPTI TRANSPORT] TIMEOUT: received %zu / %zu bytes\\n",
                    off, n);
            return -1;
        }

        off += static_cast<size_t>(r);

        fprintf(stderr,
                "[DPTI TRANSPORT] total received = %zu / %zu bytes\\n",
                off, n);
    }

    return 0;
}

static void put_u32_le(uint8_t *p, uint32_t v)
{
    p[0] = static_cast<uint8_t>(v);
    p[1] = static_cast<uint8_t>(v >> 8);
    p[2] = static_cast<uint8_t>(v >> 16);
    p[3] = static_cast<uint8_t>(v >> 24);
}

/*
 * Current RTL payload layout:
 *
 *   For every k-window of 8:
 *
 *     A: N floats
 *     B: N floats
 *
 * With N=8 and K_MAX=16:
 *
 *     window 0: A[*,0..7] + B[*,0..7]
 *     window 1: A[*,8..15] + B[*,8..15]
 *
 * Each word is transmitted little-endian, matching the existing
 * FPGA float UART protocol.
 */
static void pack_payload(
    uint8_t *payload,
    const float *A,
    const float *B,
    int M,
    int K,
    int N)
{
    size_t off = 0;

    for (int win = 0; win < HW_K_MAX / 8; ++win) {
        const int k0 = win * 8;

        for (int lane = 0; lane < HW_N; ++lane) {
            for (int koff = 0; koff < 8; ++koff) {
                const int k = k0 + koff;

                float value = 0.0f;

                if (lane < M && k < K)
                    value = A[lane * K + k];

                memcpy(payload + off, &value, sizeof(float));
                off += sizeof(float);
            }
        }

        for (int lane = 0; lane < HW_N; ++lane) {
            for (int koff = 0; koff < 8; ++koff) {
                const int k = k0 + koff;

                float value = 0.0f;

                if (k < K && lane < N)
                    value = B[k * N + lane];

                memcpy(payload + off, &value, sizeof(float));
                off += sizeof(float);
            }
        }
    }
}

static void unpack_result(
    const uint8_t *rx,
    float *C,
    int M,
    int N)
{
    /*
     * RTL result stream is an N x N matrix.
     * Only the valid M x N region is copied back.
     */
    float tmp[HW_N * HW_N];

    memcpy(tmp, rx, sizeof(tmp));

    for (int i = 0; i < M && i < HW_N; ++i) {
        for (int j = 0; j < N && j < HW_N; ++j)
            C[i * N + j] = tmp[i * HW_N + j];
    }
}

} // namespace

extern "C" int dpti_fpga_transport_matmul(
    int fd,
    int M,
    int K,
    int N,
    const float *A,
    const float *B,
    float *C)
{
    if (fd < 0 || !A || !B || !C)
        return -1;

    if (M <= 0 || K <= 0 || N <= 0)
        return -2;

    if (M > HW_N || N > HW_N || K > HW_K_MAX)
        return -3;

    uint8_t payload[RX_BYTES];
    uint8_t header[HEADER_BYTES];
    uint8_t rx[TX_BYTES];

    memset(payload, 0, sizeof(payload));

    pack_payload(payload, A, B, M, K, N);

    put_u32_le(header, static_cast<uint32_t>(K));

    /*
     * FRAME_START | k_dim | payload | FRAME_END
     */
    if (write_full(fd, kFrameStart, FRAME_START_BYTES) != 0)
        return -4;

    if (write_full(fd, header, HEADER_BYTES) != 0)
        return -5;

    if (write_full(fd, payload, RX_BYTES) != 0)
        return -6;

    if (write_full(fd, kFrameEnd, FRAME_END_BYTES) != 0)
        return -7;

    if (read_full(fd, rx, TX_BYTES) != 0)
        return -8;

    unpack_result(rx, C, M, N);

    return 0;
}
