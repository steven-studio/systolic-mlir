/* fpga_matmul_fold.h -- wire layer for the K_MAX-parameterised 8x8 FP32
 * fold array (hls/multi_200t/_fp_beat/rtl_fp/systolic_uart_fold_top.sv).
 *
 * This is a THIRD protocol, not a variant of the two already in runtime/.
 * None of them interoperate, and picking the wrong one fails as a read
 * timeout deep inside a tile loop, so the differences are spelled out:
 *
 *   fpga_matmul4x4.h      192 B, three 4x4 matrices, no header.
 *                         Targets the old 4x4 bitstream. Nothing on this
 *                         board has accepted it since the 8x8 rewrite.
 *
 *   fpga_matmul_rk_new.h  [dev][K][A][B][C_init] -> 256 B.
 *                         Targets the HLS 8x8 runtime-K bitstream. Carries
 *                         C_init, so the array accumulates in hardware.
 *
 *   THIS FILE             [k_dim][A/B payload] -> 512 B.
 *                         Targets the hand-written fold RTL. No C_init:
 *                         the array returns two accumulator contexts and
 *                         the host sums them.
 *
 * WIRE FORMAT
 *
 *   request  = [ k_dim : 4 B little-endian ] [ payload : k_max * 64 B ]
 *   response = [ C_ctx0 : 256 B ] [ C_ctx1 : 256 B ]
 *
 * The payload is A and B interleaved one 8-deep k window at a time, and is
 * always full length: operand storage and the RX framing are sized by
 * k_max, and positions at k >= k_dim are never read by the feeder.
 *
 *   A[:, 0:8]  B[0:8, :]  A[:, 8:16]  B[8:16, :]  ...
 *
 * Each 8x8 matrix is float32 little-endian row-major. Within window w:
 *
 *   A window holds A[row][k] for k = 8w .. 8w+7,  indexed [row][k-8w]
 *   B window holds B[k][col] for k = 8w .. 8w+7,  indexed [k-8w][col]
 *
 * TWO CONTEXTS, NOT ONE RESULT
 *
 * The hardware has two accumulator contexts. Even-numbered k windows
 * accumulate into ctx0 and odd-numbered into ctx1, so a single invocation
 * returns two partial matrices whose sum is that invocation's result. The
 * context index is derived from the LOCAL k of the invocation, not from the
 * position of those steps in a larger reduction.
 *
 * K_MAX IS NOT A COMPILE-TIME CONSTANT HERE
 *
 * k_max is a synthesis-time property of whichever bitstream is loaded, so
 * it is a runtime argument throughout this file. Hard-coding it would make
 * the host silently disagree with the board after a rebuild at a different
 * capacity -- which manifests as a read timeout, since the request length
 * depends on it.
 *
 * A bitstream that predates the k_dim header also shows up as a timeout: it
 * consumes the 4 header bytes as payload and then waits for 4 more that
 * never arrive.
 *
 * Return codes follow the rest of runtime/:
 *   0 ok, -1 write failed, -2 read failed/timed out, -3 invalid argument.
 */

#ifndef FPGA_MATMUL_FOLD_H
#define FPGA_MATMUL_FOLD_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Fixed by the bitstream's PE grid. Unlike k_max these are not swept. */
#define FOLD_R          8
#define FOLD_C          8

/* Depth of one accumulator-context window. Fixed in the RTL: the context
 * bit is (k >> 3) & 1. */
#define FOLD_WINDOW     8

#define FOLD_HDR_BYTES  4
#define FOLD_TX_BYTES   (2 * FOLD_R * FOLD_C * (int)sizeof(float))  /* 512 */

/* Bytes on the wire for one request at this capacity. */
size_t fold_request_bytes(int k_max);

/* Open a port in raw mode. port == NULL falls back to $FOLD_UART_PORT then
 * "/dev/ttyUSB2"; baud <= 0 falls back to $FOLD_UART_BAUD then 115200.
 * Returns a file descriptor, or -1. */
int fold_open(const char *port, int baud);
void fold_close(int fd);

/* k_max read from $FOLD_K_MAX, default 64. The board cannot be asked, so
 * this has to be told to the host somehow; an environment variable keeps it
 * out of every call site. */
int fold_default_k_max(void);

/* Serialise one invocation into buf, which must hold fold_request_bytes().
 * A is FOLD_R x k_dim row-major, B is k_dim x FOLD_C row-major. Positions
 * beyond k_dim are zero-filled. Returns bytes written, or -3.
 *
 * Exposed separately from the I/O so the packing can be diffed against the
 * Python reference without touching hardware. */
int fold_pack_request(uint8_t *buf, int k_max, int k_dim,
                      const float *A, const float *B);

/* One invocation. C_ctx0 and C_ctx1 each receive FOLD_R*FOLD_C floats;
 * either may be NULL if only the sum is wanted via fold_matmul_8x8. */
int fold_invoke(int fd, int k_max, int k_dim,
                const float *A, const float *B,
                float *C_ctx0, float *C_ctx1);

/* One 8x8 output tile with an arbitrary reduction depth.
 *
 *   C[8][8] = A[8][K] @ B[K][8]
 *
 * Issued as ceil(K / k_max) invocations, each at its true depth -- the
 * remainder is not padded up to capacity -- with the partial sums added on
 * the host. C is overwritten, not accumulated into. */
int fold_matmul_8x8(int fd, int k_max, int K,
                    const float *A, const float *B, float *C);

/* Arbitrary shape. M and N are zero-padded up to multiples of 8 and the
 * output is tiled; each 8x8 output tile runs the sequence above. A is
 * M x K row-major, B is K x N row-major, C is M x N row-major. */
int fold_matmul_tiled(int fd, int k_max, int M, int K, int N,
                      const float *A, const float *B, float *C);

/* Convenience wrapper: opens (and caches) a default connection and uses
 * fold_default_k_max(). Mirrors the *_auto entry points elsewhere in
 * runtime/ so generated code has something with no handle to pass. */
int fold_matmul_tiled_auto(int M, int K, int N,
                           const float *A, const float *B, float *C);

#ifdef __cplusplus
}
#endif

#endif  /* FPGA_MATMUL_FOLD_H */
