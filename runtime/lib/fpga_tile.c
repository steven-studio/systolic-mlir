#include "fpga_tile.h"

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "fpga_signal.h"
#include "fpga_matmul_tiled.h"

#include <stdlib.h>

static int write_full_t(int fd, const uint8_t *buf, size_t n) {
    size_t off = 0;
    while (off < n) {
        ssize_t w = write(fd, buf + off, n - off);
        if (w < 0) {
            // With a handler installed, SIGINT surfaces here rather than
            // killing the process; finishing the packet keeps the link in
            // sync so the next run does not read our tail as its head.
            if (errno == EINTR) continue;
            return -1;
        }
        off += (size_t)w;
    }
    return 0;
}

static int read_full_t(int fd, uint8_t *buf, size_t n) {
    size_t off = 0;
    while (off < n) {
        ssize_t r = read(fd, buf + off, n - off);
        if (r < 0) {
            if (errno == EINTR) continue;
            return -2;
        }
        if (r == 0) return -2;          // VTIME expired
        off += (size_t)r;
    }
    return 0;
}

int fpga_matmul_tile(int fd, int dim, int dev,
                     const float *A, const float *B,
                     const float *Cin, float *Cout) {
    if (dim <= 0 || dim > FPGA_TILE_MAX_DIM) return -1;

    const size_t elems = (size_t)dim * dim;
    const size_t mat   = elems * sizeof(float);
    uint8_t tx[1 + 3 * FPGA_TILE_MAX_DIM * FPGA_TILE_MAX_DIM * sizeof(float)];
    uint8_t rx[FPGA_TILE_MAX_DIM * FPGA_TILE_MAX_DIM * sizeof(float)];

    size_t off = 0;
    if (dev >= 0) tx[off++] = (uint8_t)dev;
    memcpy(tx + off, A,   mat); off += mat;
    memcpy(tx + off, B,   mat); off += mat;
    memcpy(tx + off, Cin, mat); off += mat;

    int rc = write_full_t(fd, tx, off);
    if (rc != 0) return rc;

    rc = read_full_t(fd, rx, mat);
    if (rc != 0) return rc;

    memcpy(Cout, rx, mat);
    return 0;
}

static int ceil_div(int x, int d) { return (x + d - 1) / d; }

// Copy a dim x dim block out of a row-major matrix, zero-padding past the
// edge. Adding 0.0f is exact, so the padding never perturbs the result.
static void extract(const float *src, int rows, int cols,
                    int r0, int c0, int dim, float *tile) {
    for (int i = 0; i < dim; i++)
        for (int j = 0; j < dim; j++) {
            int r = r0 + i, c = c0 + j;
            tile[i * dim + j] = (r < rows && c < cols) ? src[r * cols + c] : 0.0f;
        }
}

static void writeback(float *C, int M, int N,
                      int r0, int c0, int dim, const float *tile) {
    for (int i = 0; i < dim; i++)
        for (int j = 0; j < dim; j++) {
            int r = r0 + i, c = c0 + j;
            if (r < M && c < N) C[r * N + c] = tile[i * dim + j];
        }
}

int fpga_matmul_tiled_dim(int fd, int dim, int ndev,
                          int M, int K, int N,
                          const float *A, const float *B, float *C) {
    if (dim <= 0 || dim > FPGA_TILE_MAX_DIM) return -1;

    const int MT = ceil_div(M, dim), KT = ceil_div(K, dim), NT = ceil_div(N, dim);
    const size_t elems = (size_t)dim * dim;

    float a_tile[FPGA_TILE_MAX_DIM * FPGA_TILE_MAX_DIM];
    float b_tile[FPGA_TILE_MAX_DIM * FPGA_TILE_MAX_DIM];
    float acc[FPGA_TILE_MAX_DIM * FPGA_TILE_MAX_DIM];
    float out[FPGA_TILE_MAX_DIM * FPGA_TILE_MAX_DIM];

    long tile_no = 0;
    for (int it = 0; it < MT; it++) {
        for (int jt = 0; jt < NT; jt++) {
            memset(acc, 0, elems * sizeof(float));

            // The whole K chain for one output tile stays on one array. It
            // need not -- the running sum travels as C_init, so any array
            // could pick it up -- but keeping it put makes a mismatch easy
            // to attribute to a specific array.
            int dev = (ndev > 1) ? (int)(tile_no % ndev) : -1;

            for (int kt = 0; kt < KT; kt++) {
                if (fpga_stop_requested()) return -5;
                extract(A, M, K, it * dim, kt * dim, dim, a_tile);
                extract(B, K, N, kt * dim, jt * dim, dim, b_tile);
                int rc = fpga_matmul_tile(fd, dim, dev, a_tile, b_tile, acc, out);
                if (rc != 0) return rc;
                memcpy(acc, out, elems * sizeof(float));
            }
            writeback(C, M, N, it * dim, jt * dim, dim, acc);
            tile_no++;
        }
    }
    return 0;
}

static int env_int_once(const char *name, int fallback) {
    const char *v = getenv(name);
    if (!v || !*v) return fallback;
    char *end = NULL;
    long n = strtol(v, &end, 10);
    return (end == v) ? fallback : (int)n;
}

int fpga_tile_dispatch(int fd, int M, int K, int N,
                       const float *A, const float *B, float *C) {
    static int dim = -1, ndev = -1;
    if (dim < 0) {
        dim  = env_int_once("FPGA_DIM", 4);
        ndev = env_int_once("FPGA_NDEV", 1);
    }
    if (dim == 4 && ndev <= 1)
        return fpga_matmul_tiled(fd, M, K, N, A, B, C);
    return fpga_matmul_tiled_dim(fd, dim, ndev, M, K, N, A, B, C);
}
