/* fpga_matmul_rk_new.h -- runtime-K wire layer for the 8x8 systolic array.
 *
 * Replaces fpga_matmul4x4()'s protocol entirely. That function speaks a
 * 192-byte format (three 4x4 float matrices, no header) that no bitstream
 * on this board has accepted since the dual-instance rewrite. It is not
 * "the same protocol with K missing" -- the matrix shape, the framing and
 * the reply length all differ, so there is nothing to preserve.
 *
 * The wire format below is transcribed from test_rk_new.py, which is the
 * executable specification: it is the thing that produced 6/6 BIT-EXACT
 * against the live bitstream. Any disagreement between this file and that
 * script means this file is wrong.
 *
 *   [dev(1)] [K(4, LE u32)] [A(8*K*4)] [B(K*8*4)] [C_init(256)]  -> 256 back
 *
 * A and C_init go out in the caller's row-major order, so on x86 they are a
 * straight memcpy. B does NOT: it is transposed on the wire.
 *
 *   A       bank i = row i,    words A[i][0..K-1]
 *   B       bank j = column j, words B[0..K-1][j]      <-- transposed
 *   C_init  row-major
 *
 * The transpose is not arbitrary. Bank j feeds column j of the array, and
 * that column needs its own k-sequence contiguously -- the wire order is
 * the mirror of ARRAY_PARTITION variable=B complete dim=2 in the kernel.
 *
 * This is invisible at K=1, where B is 1x8 and both orders emit the same
 * eight floats. A K=1-only test therefore proves nothing about B's layout;
 * that is exactly how it went unnoticed once already.
 *
 * Everything is little-endian float32. On a big-endian host this file is
 * wrong and would need byte swapping; that is deliberate, since the host is
 * x86 and a portable version would obscure that A is a plain copy.
 */

#ifndef FPGA_MATMUL_RK_NEW_H
#define FPGA_MATMUL_RK_NEW_H

#include <stddef.h>
#include <stdint.h>

/* Geometry of the deployed array. R and C are fixed by the bitstream; K is
 * a runtime argument bounded by K_MAX. These must track hls_rk.cfg. */
#define SYS_R      8
#define SYS_C      8
#define SYS_K_MAX  64

/* Largest request: 1 + 4 + 8*64*4 + 64*8*4 + 256 */
#define SYS_TX_MAX (1 + 4 + SYS_R * SYS_K_MAX * 4 + SYS_K_MAX * SYS_C * 4 \
                    + SYS_R * SYS_C * 4)
#define SYS_RX_LEN (SYS_R * SYS_C * 4)

/* Serialise one transaction into buf. Returns the number of bytes written,
 * or -1 if K is out of range. Exposed separately from the I/O so the packing
 * can be diffed against the Python reference without touching hardware --
 * see dump_ref_new.py. buf must be at least SYS_TX_MAX bytes.
 *
 * Argument layouts are the natural GEMM ones -- the packer does the
 * transpose, so callers never see the wire convention:
 *
 *   A      points at SYS_R * K floats, row-major  (A[i*K + k])
 *   B      points at K * SYS_C floats, row-major  (B[k*SYS_C + j])
 *   C_init points at SYS_R * SYS_C floats         (C[i*SYS_C + j])
 */
int sys_pack_request(uint8_t *buf, int dev, int K,
                     const float *A, const float *B, const float *C_init);

/* C_out = A[8][K] @ B[K][8] + C_init, computed on the array.
 * Returns 0 on success, -1 on write failure, -2 on read failure/timeout,
 * -3 if K is out of range.
 *
 * dev selects which of the two array instances handles the request. Both
 * share one UART, so alternating devices buys nothing while the link is the
 * bottleneck -- pass 0 unless you are specifically testing instance 1.
 *
 * K must be in [1, SYS_K_MAX]. Deeper reductions are the caller's job:
 * issue successive calls feeding C_out back in as C_init, which the kernel
 * accumulates in place.
 */
int sys_matmul(int fd, int dev, int K,
               const float *A, const float *B,
               const float *C_init, float *C_out);

#endif  /* FPGA_MATMUL_RK_NEW_H */