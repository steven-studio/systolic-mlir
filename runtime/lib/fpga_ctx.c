#include "fpga_ctx.h"

#include "fpga_matmul4x4.h"        // fpga_uart_open / fpga_uart_close
#include "fpga_matmul_tiled.h"     // fpga_matmul_tiled(fd, ...)
#include "fpga_conv2d_im2col.h"
#include "fpga_tile.h"    // fd-taking conv variants

#include <stdlib.h>
#include <string.h>

static int env_int(const char *name, int fallback) {
    const char *v = getenv(name);
    if (!v || !*v) return fallback;
    char *end = NULL;
    long n = strtol(v, &end, 10);
    return (end == v) ? fallback : (int)n;
}

#define DEFAULT_PORT "/dev/ttyUSB1"

struct fpga_ctx {
    int  fd;
    char port[128];
    int  baud;   // FPGA_UART_BAUD
    int  dim;    // FPGA_DIM  -- array edge, 4 or 8
    int  ndev;   // FPGA_NDEV -- arrays on the board; >1 adds a device byte
    // Owned here rather than in fpga_matmul4x4_reliable.c so two contexts
    // (a board and a simulator, say) do not share one set of counters.
    fpga_reliable_stats_t stats;
};

static fpga_ctx_t *g_default = NULL;

fpga_ctx_t *fpga_ctx_open(const char *port) {
    if (!port || !*port) {
        const char *env = getenv("FPGA_UART_PORT");
        port = (env && *env) ? env : DEFAULT_PORT;
    }

    int baud = env_int("FPGA_UART_BAUD", 115200);
    int fd = fpga_uart_open_baud(port, baud);
    if (fd < 0)
        return NULL;

    fpga_ctx_t *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) {
        fpga_uart_close(fd);
        return NULL;
    }
    ctx->fd   = fd;
    ctx->baud = baud;
    ctx->dim  = env_int("FPGA_DIM", 4);
    ctx->ndev = env_int("FPGA_NDEV", 1);
    // Truncation is fine: the string is only ever reported back to the user.
    strncpy(ctx->port, port, sizeof(ctx->port) - 1);
    return ctx;
}

void fpga_ctx_close(fpga_ctx_t *ctx) {
    if (!ctx)
        return;
    if (ctx->fd >= 0)
        fpga_uart_close(ctx->fd);
    if (ctx == g_default)
        g_default = NULL;
    free(ctx);
}

fpga_ctx_t *fpga_ctx_default(void) {
    // Opened on first use, not at startup: a host process that never touches
    // the accelerator should not fail because no board is plugged in.
    if (!g_default)
        g_default = fpga_ctx_open(NULL);
    return g_default;
}

void fpga_ctx_reset_default(void) {
    if (g_default)
        fpga_ctx_close(g_default);   // clears g_default itself
}

int fpga_ctx_fd(const fpga_ctx_t *ctx) { return ctx ? ctx->fd : -1; }
int fpga_ctx_dim(const fpga_ctx_t *ctx) { return ctx ? ctx->dim : 0; }
int fpga_ctx_ndev(const fpga_ctx_t *ctx) { return ctx ? ctx->ndev : 0; }
int fpga_ctx_baud(const fpga_ctx_t *ctx) { return ctx ? ctx->baud : 0; }
const char *fpga_ctx_port(const fpga_ctx_t *ctx) { return ctx ? ctx->port : ""; }

const fpga_reliable_stats_t *fpga_ctx_stats(const fpga_ctx_t *ctx) {
    return ctx ? &ctx->stats : NULL;
}

void fpga_ctx_clear_stats(fpga_ctx_t *ctx) {
    if (ctx)
        memset(&ctx->stats, 0, sizeof(ctx->stats));
}

// --- operations -----------------------------------------------------

int fpga_ctx_matmul_tiled(fpga_ctx_t *ctx, int M, int K, int N,
                          const float *A, const float *B, float *C) {
    if (!ctx || ctx->fd < 0)
        return -3;
    // dim 4 且單陣列時走原本已驗證的路徑，其餘走 dim-generic 版本。
    if (ctx->dim == 4 && ctx->ndev <= 1)
        return fpga_matmul_tiled(ctx->fd, M, K, N, A, B, C);
    return fpga_matmul_tiled_dim(ctx->fd, ctx->dim, ctx->ndev, M, K, N, A, B, C);
}

int fpga_ctx_conv2d_im2col_general(fpga_ctx_t *ctx,
                                   int N, int H, int W, int Cin,
                                   int Kh, int Kw, int Cout,
                                   int strideH, int strideW,
                                   int dilationH, int dilationW,
                                   const float *X, const float *Kernel,
                                   float *Y) {
    if (!ctx || ctx->fd < 0)
        return -3;
    return fpga_conv2d_im2col_general(ctx->fd, N, H, W, Cin, Kh, Kw, Cout,
                                      strideH, strideW, dilationH, dilationW,
                                      X, Kernel, Y);
}

int fpga_ctx_conv2d_im2col_padded(fpga_ctx_t *ctx,
                                  int N, int H, int W, int Cin,
                                  int Kh, int Kw, int Cout,
                                  int strideH, int strideW,
                                  int dilationH, int dilationW,
                                  int padTop, int padBottom,
                                  int padLeft, int padRight,
                                  const float *X, const float *Kernel,
                                  float *Y) {
    if (!ctx || ctx->fd < 0)
        return -3;
    return fpga_conv2d_im2col_padded(ctx->fd, N, H, W, Cin, Kh, Kw, Cout,
                                     strideH, strideW, dilationH, dilationW,
                                     padTop, padBottom, padLeft, padRight,
                                     X, Kernel, Y);
}

int fpga_conv2d_im2col_general_auto(int N, int H, int W, int Cin,
                                    int Kh, int Kw, int Cout,
                                    int strideH, int strideW,
                                    int dilationH, int dilationW,
                                    const float *X, const float *Kernel,
                                    float *Y) {
    return fpga_ctx_conv2d_im2col_general(fpga_ctx_default(),
        N, H, W, Cin, Kh, Kw, Cout, strideH, strideW,
        dilationH, dilationW, X, Kernel, Y);
}

int fpga_conv2d_im2col_padded_auto(int N, int H, int W, int Cin,
                                   int Kh, int Kw, int Cout,
                                   int strideH, int strideW,
                                   int dilationH, int dilationW,
                                   int padTop, int padBottom,
                                   int padLeft, int padRight,
                                   const float *X, const float *Kernel,
                                   float *Y) {
    return fpga_ctx_conv2d_im2col_padded(fpga_ctx_default(),
        N, H, W, Cin, Kh, Kw, Cout, strideH, strideW, dilationH, dilationW,
        padTop, padBottom, padLeft, padRight, X, Kernel, Y);
}

int fpga_matmul_tiled_auto(int M, int K, int N,
                           const float *A, const float *B, float *C) {
    return fpga_ctx_matmul_tiled(fpga_ctx_default(), M, K, N, A, B, C);
}
