// Reliability wrapper around fpga_matmul4x4.
//
// WHY THIS EXISTS
// The UART protocol is fixed-length and bare: 48 bytes out (A, B, C_init),
// 16 bytes back (the result). There is no framing header, no length field,
// no sequence number and no checksum. That leaves two failure modes:
//
//   dropped byte : read_full() blocks until VTIME (5 s) and returns -2.
//                  Detected -- but the fd is now out of sync and whatever
//                  arrived late is still sitting in the kernel buffer, so
//                  every subsequent transaction on that fd is garbage.
//   flipped bit  : the byte count is correct and the content is wrong.
//                  NOT detectable host-side. Silently becomes the result.
//
// A full 48-config conv2d sweep (tens of thousands of tile transactions)
// produced exactly one bad tile, and it was the second kind: byte count
// correct, 32 output elements wrong, which is two complete output tiles --
// consistent with one corrupted response poisoning an accumulator that is
// carried across the whole kt loop in fpga_matmul_tiled.
//
// WHAT THIS FIXES AND WHAT IT DOES NOT
//   fixed  : dropped bytes. Flush both directions, then resend. The board
//            is request/response and stateless per transaction (the running
//            accumulator is passed in as C_init every time), so resending
//            an identical request is idempotent and safe.
//   partly : flipped bits, but only when FPGA_MATMUL_VERIFY is set, which
//            sends every request twice and compares the two responses
//            bitwise. That costs 2x the transactions, so it is opt-in and
//            meant for verification sweeps, not production runs.
//   NOT    : flipped bits in the default path. Detecting those requires the
//            board to send a checksum, which means changing the HLS/RTL,
//            re-synthesising and re-flashing. Left as future work.
//
// fpga_matmul4x4() itself is deliberately untouched -- it is the already
// validated transport. This file only wraps it.

#include "fpga_matmul4x4_reliable.h"
#include "fpga_matmul4x4.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#define MATRIX_ELEMENTS 16
#define MATRIX_BYTES    (MATRIX_ELEMENTS * sizeof(float))

// Time to let the line settle after a flush before resending. A dropped
// byte usually means the board is still mid-transmission, so draining and
// immediately resending can just resync onto the tail of the old response.
#define RESYNC_SETTLE_US 20000  // 20 ms

static fpga_reliable_stats_t g_stats;

static int env_int(const char *name, int fallback) {
    const char *v = getenv(name);
    if (!v || !*v) return fallback;
    char *end = NULL;
    long n = strtol(v, &end, 10);
    if (end == v) return fallback;
    return (int)n;
}

static int max_attempts(void) {
    static int n = -1;
    if (n == -1) {
        n = env_int("FPGA_MATMUL_RETRIES", 3);
        if (n < 1) n = 1;
    }
    return n;
}

static int verify_enabled(void) {
    static int v = -1;
    if (v == -1) v = (getenv("FPGA_MATMUL_VERIFY") != NULL);
    return v;
}

// Drain anything stale in both directions and give the line a moment to
// go quiet, so the next request starts from a known state.
static void resync(int fd) {
    tcflush(fd, TCIOFLUSH);
    usleep(RESYNC_SETTLE_US);
    tcflush(fd, TCIOFLUSH);
    g_stats.resyncs++;
}

// One transaction with resync-and-retry on transport failure.
static int attempt(int fd, const float *A, const float *B,
                   const float *C_init, float *out) {
    int rc = -1;
    for (int i = 0; i < max_attempts(); i++) {
        if (i > 0) {
            resync(fd);
            g_stats.retries++;
        }
        rc = fpga_matmul4x4(fd, A, B, C_init, out);
        if (rc == 0) return 0;
        g_stats.transport_failures++;
    }
    return rc;
}

int fpga_matmul4x4_reliable(int fd,
                            const float A[16],
                            const float B[16],
                            const float C_init[16],
                            float C_out[16]) {
    g_stats.transactions++;

    int rc = attempt(fd, A, B, C_init, C_out);
    if (rc != 0) return rc;

    if (!verify_enabled()) return 0;

    // Opt-in double-send. The board is stateless per transaction, so a
    // second identical request must produce a bit-identical response.
    // Any difference means at least one of the two was corrupted.
    float second[MATRIX_ELEMENTS];
    rc = attempt(fd, A, B, C_init, second);
    if (rc != 0) return rc;

    if (memcmp(C_out, second, MATRIX_BYTES) == 0) return 0;

    g_stats.verify_mismatches++;

    // Tie-break with a third sample. Two matching out of three is taken
    // as correct; three different answers means something is badly wrong
    // and we refuse to guess.
    float third[MATRIX_ELEMENTS];
    rc = attempt(fd, A, B, C_init, third);
    if (rc != 0) return rc;

    if (memcmp(third, C_out, MATRIX_BYTES) == 0) {
        g_stats.verify_recovered++;
        return 0;
    }
    if (memcmp(third, second, MATRIX_BYTES) == 0) {
        memcpy(C_out, second, MATRIX_BYTES);
        g_stats.verify_recovered++;
        return 0;
    }

    g_stats.verify_unrecovered++;
    return -4;  // three mutually different responses
}

fpga_reliable_stats_t fpga_reliable_get_stats(void) { return g_stats; }

void fpga_reliable_reset_stats(void) { memset(&g_stats, 0, sizeof(g_stats)); }

void fpga_reliable_print_stats(FILE *f) {
    fprintf(f,
            "[uart] transactions=%lu resyncs=%lu retries=%lu "
            "transport_failures=%lu verify_mismatches=%lu "
            "recovered=%lu unrecovered=%lu\n",
            g_stats.transactions, g_stats.resyncs, g_stats.retries,
            g_stats.transport_failures, g_stats.verify_mismatches,
            g_stats.verify_recovered, g_stats.verify_unrecovered);
}
