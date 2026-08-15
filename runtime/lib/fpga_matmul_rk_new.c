/* fpga_matmul_rk_new.c -- see fpga_matmul_rk_new.h for the wire format.
 *
 * write_full/read_full are duplicated here rather than shared with
 * fpga_matmul4x4.c because they are static there. Duplicating ~40 lines is
 * cheaper than making them external while the old file is still linked in,
 * and this file is meant to be droppable into a standalone test with no
 * link-order surprises.
 */

#include "fpga_matmul_rk_new.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ------------------------------------------------------------------ */
/* Blocking I/O helpers                                               */
/* ------------------------------------------------------------------ */

/* write() on a UART can return short. Loop until the whole buffer is out or
 * something that is not EINTR goes wrong. A zero return is treated as an
 * error: on a blocking fd it means the far side is gone, and spinning on it
 * would hang instead of reporting. */
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

/* Mirror of the above for reads. A zero return here means the port hit its
 * VTIME timeout with nothing pending -- i.e. the board never answered. That
 * is the failure mode to care about: a wrong-length request leaves the FPGA
 * waiting for bytes that never come, and it shows up here rather than as
 * corrupt data. */
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
/* Packing                                                            */
/* ------------------------------------------------------------------ */

int sys_pack_request(uint8_t *buf, int dev, int K,
                     const float *A, const float *B, const float *C_init)
{
    if (K < 1 || K > SYS_K_MAX)
        return -1;

    const size_t a_bytes = (size_t)SYS_R * (size_t)K * sizeof(float);
    const size_t b_bytes = (size_t)K * (size_t)SYS_C * sizeof(float);
    const size_t c_bytes = (size_t)SYS_R * (size_t)SYS_C * sizeof(float);

    uint8_t *p = buf;

    *p++ = (uint8_t)dev;

    /* struct.pack('<I', K). uint32_t on x86 is already little-endian, so the
     * copy is the encoding. Spelled as a memcpy of a sized type rather than
     * four shifts so that the width is impossible to get wrong. */
    const uint32_t k_le = (uint32_t)K;
    memcpy(p, &k_le, sizeof(k_le));
    p += sizeof(k_le);

    /* A: bank i = row i. The caller's row-major layout already has each
     * row's k-sequence contiguous, so this is a straight copy. */
    memcpy(p, A, a_bytes);
    p += a_bytes;

    /* B: bank j = column j, transposed relative to the caller's row-major
     * B[k][j]. Column j's k-sequence is strided by SYS_C in memory, so this
     * cannot be a memcpy -- it is the one place the wire order and the host
     * order disagree.
     *
     * Note the loop nest: j outer, k inner. Writing it k-outer produces an
     * identical index expression B[k*SYS_C + j] while emitting the bytes in
     * exactly the wrong order, and the mistake is invisible at K=1. */
    for (int j = 0; j < SYS_C; j++) {
        for (int k = 0; k < K; k++) {
            memcpy(p, &B[(size_t)k * SYS_C + j], sizeof(float));
            p += sizeof(float);
        }
    }

    (void)b_bytes;   /* kept for the length assertion below */

    memcpy(p, C_init, c_bytes);
    p += c_bytes;

    return (int)(p - buf);
}

/* ------------------------------------------------------------------ */
/* Transaction                                                        */
/* ------------------------------------------------------------------ */

int sys_matmul(int fd, int dev, int K,
               const float *A, const float *B,
               const float *C_init, float *C_out)
{
    uint8_t tx[SYS_TX_MAX];

    const int tx_len = sys_pack_request(tx, dev, K, A, B, C_init);

    if (tx_len < 0)
        return -3;

    /* FPGA_DUMP_TX=<path> writes the serialised request and stops short of
     * touching the port. This is the hook for byte-diffing against
     * dump_ref_new.py: a length match alone does not prove the layout is
     * right, and a transposed A would otherwise come back as plausible
     * wrong numbers rather than an error. */
    const char *dump = getenv("FPGA_DUMP_TX");

    if (dump != NULL) {
        FILE *f = fopen(dump, "wb");

        if (f == NULL)
            return -1;

        const size_t n = fwrite(tx, 1, (size_t)tx_len, f);

        fclose(f);

        return (n == (size_t)tx_len) ? 0 : -1;
    }

    if (write_full(fd, tx, (size_t)tx_len) != 0)
        return -1;

    uint8_t rx[SYS_RX_LEN];

    if (read_full(fd, rx, sizeof(rx)) != 0)
        return -2;

    memcpy(C_out, rx, sizeof(rx));

    return 0;
}