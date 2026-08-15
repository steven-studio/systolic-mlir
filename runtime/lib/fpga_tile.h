#ifndef FPGA_TILE_H
#define FPGA_TILE_H

// Dimension- and device-generic tile transaction, for boards whose array is
// not 4x4 and/or which host more than one array.
//
// The existing fpga_matmul4x4 / fpga_matmul_tiled pair is left untouched.
// That path carries the 48/48 bit-exact result on the Arty and is the only
// thing the MLIR codegen entry points reach today; re-parametrising it in
// place would put that evidence at risk for no gain, since the two paths
// differ only in constants.
//
// Protocol, per matmul_top_dual.v:
//     dev <  0 : no device byte      (single-array boards, e.g. the 4x4 Arty)
//     dev >= 0 : one leading byte selecting the array
//     then 3 * dim * dim floats  (A, B, C_init), little-endian
//     back    dim * dim floats   (C)

#ifdef __cplusplus
extern "C" {
#endif

// Largest array this transaction buffer can carry. 16x16 needs
// 1 + 3*256*4 = 3073 bytes; raising it costs only stack.
#define FPGA_TILE_MAX_DIM 16

// One tile. Returns 0, or -1 write failed, -2 read timed out.
int fpga_matmul_tile(int fd, int dim, int dev,
                     const float *A, const float *B,
                     const float *Cin, float *Cout);

// Whole GEMM, tiled at dim x dim x dim. ndev > 1 spreads output tiles over
// the arrays round-robin; ndev <= 1 sends no device byte at all.
//
// Note the round robin buys nothing on a UART: the protocol is blocking, and
// at 115200 one 8x8 tile is ~89 ms of transfer against ~0.28 us of compute,
// so a second array can only ever overlap the compute. It is here because the
// arrays exist and the dispatch has to name one, not as a speed-up.
int fpga_matmul_tiled_dim(int fd, int dim, int ndev,
                          int M, int K, int N,
                          const float *A, const float *B, float *C);

#ifdef __cplusplus
}
#endif

// Drop-in for fpga_matmul_tiled(fd, ...) that takes its geometry from the
// environment instead of hard-coding 4x4.
//
// It exists because fpga_conv2d_im2col.c calls the tiled matmul directly
// rather than through fpga_ctx, so the ctx-level dim/ndev choice never
// reached the conv path -- it kept sending 192-byte 4x4 packets to a board
// waiting for 769-byte 8x8 ones, which surfaces as a read timeout (rc=-2).
//
// FPGA_DIM (default 4) and FPGA_NDEV (default 1) are read once. Those
// defaults route straight back to fpga_matmul_tiled, so the validated 4x4
// path is byte-for-byte unchanged.
int fpga_tile_dispatch(int fd, int M, int K, int N,
                       const float *A, const float *B, float *C);

#endif  // FPGA_TILE_H
