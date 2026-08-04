#ifndef FPGA_CTX_H
#define FPGA_CTX_H

// One accelerator connection and everything that belongs to it.
//
// WHY
// The state this replaces was scattered: fpga_matmul_tiled.c held the UART
// fd in a file-static (g_fpga_fd) initialised lazily on first use, the port
// path was a string literal inside that function, and the reliability
// counters were a separate file-static in fpga_matmul4x4_reliable.c. None of
// it could be inspected, reset, or duplicated, and the port could only be
// changed by editing and recompiling -- which mattered every time the board
// came back as a different /dev/ttyUSB* after a reboot.
//
// WHAT DOES NOT CHANGE
// The *_auto entry points stay exactly as they are. Conv2DToFpga and
// TileMatmulForFpga bake their signatures into an LLVMFunctionType, so the
// generated calls have nowhere to put a handle. Those functions now forward
// to a process-wide default context: still one global, but it is a single
// pointer to a struct rather than several independent globals, and the
// default context is created through the same path as any other.
//
// Signal handling stays out of here on purpose. A handler may only touch a
// volatile sig_atomic_t at file scope, so fpga_signal.c keeps its own flag.

#include "fpga_matmul4x4_reliable.h"   // fpga_reliable_stats_t

#ifdef __cplusplus
extern "C" {
#endif

typedef struct fpga_ctx fpga_ctx_t;

// port == NULL falls back to $FPGA_UART_PORT, then to the historical
// /dev/ttyUSB1. Returns NULL if the port cannot be opened.
fpga_ctx_t *fpga_ctx_open(const char *port);
void        fpga_ctx_close(fpga_ctx_t *ctx);

// The lazily-created process default, used by the *_auto entry points.
// Returns NULL if the port cannot be opened; callers must check.
fpga_ctx_t *fpga_ctx_default(void);

// Frees the default context. Not required for one-shot drivers; useful for
// a long-running host that wants to reopen after unplugging the board.
void fpga_ctx_reset_default(void);

int fpga_ctx_fd(const fpga_ctx_t *ctx);
const char *fpga_ctx_port(const fpga_ctx_t *ctx);

const fpga_reliable_stats_t *fpga_ctx_stats(const fpga_ctx_t *ctx);
void fpga_ctx_clear_stats(fpga_ctx_t *ctx);

// --- operations -----------------------------------------------------
// Return codes are unchanged from the fd-taking functions they wrap:
//   0 ok, -1 write failed, -2 read timed out, -3 no connection,
//   -4 三次回應互不相同 (verify mode), -5 中斷於 tile 邊界.

int fpga_ctx_matmul_tiled(fpga_ctx_t *ctx, int M, int K, int N,
                          const float *A, const float *B, float *C);

int fpga_ctx_conv2d_im2col_general(fpga_ctx_t *ctx,
                                   int N, int H, int W, int Cin,
                                   int Kh, int Kw, int Cout,
                                   int strideH, int strideW,
                                   int dilationH, int dilationW,
                                   const float *X, const float *Kernel,
                                   float *Y);

int fpga_ctx_conv2d_im2col_padded(fpga_ctx_t *ctx,
                                  int N, int H, int W, int Cin,
                                  int Kh, int Kw, int Cout,
                                  int strideH, int strideW,
                                  int dilationH, int dilationW,
                                  int padTop, int padBottom,
                                  int padLeft, int padRight,
                                  const float *X, const float *Kernel,
                                  float *Y);

#ifdef __cplusplus
}
#endif

#endif  // FPGA_CTX_H
