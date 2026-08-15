/* systolic_dispatch_new.h -- transport-neutral dispatch contract.
 *
 * This is the interface TileMatmulForFpgaPattern's generated IR calls
 * against. Exactly one of systolic_dispatch_uart_new.c or
 * systolic_dispatch_sim_new.c is linked in; that link choice is the only
 * difference between a build that drives the board and one that runs on the
 * host CPU. Neither the pass nor the generated object changes.
 *
 * Replaces systolic_dispatch_matmul4x4(), whose 4x4 shape was baked into
 * the pass's LLVMFunctionType. The array is 8x8 with K supplied at runtime,
 * so the tile shape moves into the signature rather than the name -- the
 * old name promised a geometry the hardware stopped having.
 *
 * SEMANTICS -- measured on hardware, not assumed:
 *
 *     C_out[i][j] = C_init[i][j] + sum_{k=0}^{K-1} A[i][k] * B[k][j]
 *
 * with a float32 accumulator SEEDED WITH C_init and k ASCENDING. That is
 * one of four orderings that agree at K=1 and disagree from K=2 on; the
 * others were ruled out against the live board. It matters because it is
 * what makes reductions deeper than SYS_K_MAX decomposable: feed one call's
 * C_out in as the next call's C_init and the chain is exact. Had the array
 * seeded from zero and folded C_init in at the end, that same chaining
 * would have added C_init once per chunk -- silently, and only for K>64.
 */

#ifndef SYSTOLIC_DISPATCH_NEW_H
#define SYSTOLIC_DISPATCH_NEW_H

/* Geometry of the deployed array. The pass must tile to exactly these. */
#define SYS_DISPATCH_R      8
#define SYS_DISPATCH_C      8
#define SYS_DISPATCH_K_MAX  64

/* Acquire a handle. Returns >= 0 on success, negative on failure.
 *
 * The UART backend opens and caches a port; the sim backend returns a
 * placeholder. Call once before the tile loop, not per tile -- reopening a
 * serial port per 8x8 tile would dominate everything else on the wire.
 *
 * UART backend honours two environment variables so that moving the board
 * to a different port never requires a rebuild:
 *
 *     SYSTOLIC_PORT   default "/dev/ttyUSB2"
 *     SYSTOLIC_BAUD   default 2000000
 *
 * The default was "/dev/ttyUSB1" in the code this replaces, which has not
 * been where the board enumerates for some time -- a wrong default here
 * fails as a timeout deep in a tile loop, so it is worth overridable.
 */
int systolic_dispatch_open(void);

/* One 8x8 output tile with a K-deep reduction.
 *
 *   K       in [1, SYS_DISPATCH_K_MAX]
 *   A       SYS_DISPATCH_R * K floats, row-major   A[i*K + k]
 *   B       K * SYS_DISPATCH_C floats, row-major   B[k*SYS_DISPATCH_C + j]
 *   C_init  64 floats, row-major
 *   C_out   64 floats, row-major; may alias C_init
 *
 * Callers pass the natural GEMM layouts. The UART backend transposes B onto
 * the wire; that convention stops at this boundary and never reaches the
 * pass.
 *
 * Returns 0 on success, negative on transport failure or invalid K. Both
 * backends reject K out of range identically, so a pass bug that emits
 * K=0 or K=65 surfaces in the sim build rather than as a hang on the board.
 */
int systolic_dispatch_matmul(int handle, int K,
                             const float *A, const float *B,
                             const float *C_init, float *C_out);

#endif  /* SYSTOLIC_DISPATCH_NEW_H */
