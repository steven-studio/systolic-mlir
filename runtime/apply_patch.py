#!/usr/bin/env python3
"""把 baud / dim / ndev 加進 fpga_ctx，並讓 fpga_uart_open 接受 baud。
   在 runtime/ 底下執行。"""
import re, sys, os

def edit(path, subs, required=True):
    if not os.path.exists(path):
        print(f"  !! 找不到 {path}"); return False
    s = open(path).read(); miss=[]
    for a,b in subs:
        if a in s: s = s.replace(a,b,1)
        else: miss.append(a)
    if miss and required:
        print(f"  !! {path} 有 {len(miss)} 處沒對上：")
        for m in miss: print("     ", m.splitlines()[0][:70])
        return False
    open(path,'w').write(s)
    print(f"  ok {path}")
    return True

ok = True

# ---- 1. fpga_uart_open 接受 baud -------------------------------------
ok &= edit('lib/fpga_matmul4x4.c', [
 ('int fpga_uart_open(const char *port) {',
  '''static speed_t baud_const(int baud) {
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

int fpga_uart_open_baud(const char *port, int baud) {'''),
 ('    cfsetospeed(&tty, B1000000);\n    cfsetispeed(&tty, B1000000);',
  '    speed_t sp = baud_const(baud);\n    cfsetospeed(&tty, sp);\n    cfsetispeed(&tty, sp);'),
])

# 舊簽名保留成薄包裝，避免動到已驗證的呼叫端
s = open('lib/fpga_matmul4x4.c').read()
if 'int fpga_uart_open(const char *port)' not in s:
    s = s.replace('void fpga_uart_close(int fd) {',
'''int fpga_uart_open(const char *port) {
    // Historical entry point: the Arty bitstream is 115200.
    return fpga_uart_open_baud(port, 115200);
}

void fpga_uart_close(int fd) {''', 1)
    open('lib/fpga_matmul4x4.c','w').write(s)
    print("  ok lib/fpga_matmul4x4.c (加回 fpga_uart_open 包裝)")

ok &= edit('lib/fpga_matmul4x4.h', [
 ('int fpga_uart_open(const char *port);',
  'int fpga_uart_open(const char *port);\nint fpga_uart_open_baud(const char *port, int baud);'),
])

# ---- 2. fpga_ctx 加 baud / dim / ndev --------------------------------
ok &= edit('lib/fpga_ctx.c', [
 ('struct fpga_ctx {\n    int  fd;\n    char port[128];',
  'struct fpga_ctx {\n    int  fd;\n    char port[128];\n    int  baud;   // FPGA_UART_BAUD\n    int  dim;    // FPGA_DIM  -- array edge, 4 or 8\n    int  ndev;   // FPGA_NDEV -- arrays on the board; >1 adds a device byte'),
 ('    int fd = fpga_uart_open(port);',
  '''    int baud = env_int("FPGA_UART_BAUD", 115200);
    int fd = fpga_uart_open_baud(port, baud);'''),
 ('    ctx->fd = fd;',
  '''    ctx->fd   = fd;
    ctx->baud = baud;
    ctx->dim  = env_int("FPGA_DIM", 4);
    ctx->ndev = env_int("FPGA_NDEV", 1);'''),
])

# env_int helper + include
s = open('lib/fpga_ctx.c').read()
if 'static int env_int' not in s:
    s = s.replace('#define DEFAULT_PORT',
'''static int env_int(const char *name, int fallback) {
    const char *v = getenv(name);
    if (!v || !*v) return fallback;
    char *end = NULL;
    long n = strtol(v, &end, 10);
    return (end == v) ? fallback : (int)n;
}

#define DEFAULT_PORT''', 1)
if '#include "fpga_tile.h"' not in s:
    s = s.replace('#include "fpga_conv2d_im2col.h"',
                  '#include "fpga_conv2d_im2col.h"\n#include "fpga_tile.h"', 1)
open('lib/fpga_ctx.c','w').write(s)
print("  ok lib/fpga_ctx.c (env_int + include)")

# ---- 3. ctx 的 matmul 走 dim-generic 版本 ----------------------------
ok &= edit('lib/fpga_ctx.c', [
 ('    return fpga_matmul_tiled(ctx->fd, M, K, N, A, B, C);',
  '''    // dim 4 且單陣列時走原本已驗證的路徑，其餘走 dim-generic 版本。
    if (ctx->dim == 4 && ctx->ndev <= 1)
        return fpga_matmul_tiled(ctx->fd, M, K, N, A, B, C);
    return fpga_matmul_tiled_dim(ctx->fd, ctx->dim, ctx->ndev, M, K, N, A, B, C);'''),
])

ok &= edit('lib/fpga_ctx.h', [
 ('int fpga_ctx_fd(const fpga_ctx_t *ctx);',
  'int fpga_ctx_fd(const fpga_ctx_t *ctx);\nint fpga_ctx_dim(const fpga_ctx_t *ctx);\nint fpga_ctx_ndev(const fpga_ctx_t *ctx);\nint fpga_ctx_baud(const fpga_ctx_t *ctx);'),
])
s = open('lib/fpga_ctx.c').read()
if 'int fpga_ctx_dim(' not in s:
    s = s.replace('int fpga_ctx_fd(const fpga_ctx_t *ctx) { return ctx ? ctx->fd : -1; }',
'''int fpga_ctx_fd(const fpga_ctx_t *ctx) { return ctx ? ctx->fd : -1; }
int fpga_ctx_dim(const fpga_ctx_t *ctx) { return ctx ? ctx->dim : 0; }
int fpga_ctx_ndev(const fpga_ctx_t *ctx) { return ctx ? ctx->ndev : 0; }
int fpga_ctx_baud(const fpga_ctx_t *ctx) { return ctx ? ctx->baud : 0; }''', 1)
    open('lib/fpga_ctx.c','w').write(s)
    print("  ok lib/fpga_ctx.c (accessors)")

print("\n完成" if ok else "\n有項目未對上，見上方訊息")
sys.exit(0 if ok else 1)
