/* e2e_main_new.c -- driver for the lowered MLIR gemm.
 *
 * Calls the compiled _mlir_ciface_gemm and checks the result bit-exactly
 * against a CPU reference.
 *
 * WHY BIT-EXACT IS THE RIGHT BAR HERE
 *
 * The tiled computation and a plain sequential reference produce identical
 * float32 results, and that is not a coincidence worth glossing over. Each
 * dispatch seeds its accumulator with the running C value and sums k
 * ascending within its chunk; chunk 1 then seeds from chunk 0's output. So
 * for a given (i, j) the additions happen in exactly the order
 *
 *     C[i][j], then k = 0, 1, 2, ... K-1
 *
 * which is what the loop below does. K is never zero-padded, so no spurious
 * +0.0 terms enter. M and N padding produces whole rows and columns that the
 * writeback discards, so it cannot perturb a valid element either.
 *
 * A tolerance would therefore not be "safer" -- it would mask exactly the
 * ordering and layout bugs this test exists to find.
 *
 * BUILD -- -ffp-contract=off is load bearing, as everywhere else in this
 * project: an FMA in the reference carries more intermediate precision than
 * the FPGA's separate fmul and fadd, and the test would fail for a reason
 * that has nothing to do with the code under test.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define M 17
#define K 100
#define N 9

/* MLIR's C interface for a rank-2 memref. Field order and count are fixed by
 * the LLVM lowering, not by us -- a mismatch here corrupts the pointer the
 * kernel dereferences, which tends to present as a segfault inside generated
 * code rather than as a wrong answer. */
typedef struct {
    float   *allocated;
    float   *aligned;
    int64_t  offset;
    int64_t  sizes[2];
    int64_t  strides[2];
} MemRef2D;

/* Returning a memref means the C interface takes the result descriptor as a
 * hidden first argument. */
extern void _mlir_ciface_gemm(MemRef2D *result,
                              MemRef2D *a, MemRef2D *b, MemRef2D *c);

static void init_memref(MemRef2D *m, float *data, int64_t rows, int64_t cols)
{
    m->allocated = data;
    m->aligned   = data;
    m->offset    = 0;
    m->sizes[0]  = rows;
    m->sizes[1]  = cols;
    m->strides[0] = cols;   /* row-major, contiguous */
    m->strides[1] = 1;
}

/* Fixed LCG rather than rand(): a bit-level failure that only reproduces
 * every other run is not worth debugging. */
static uint32_t lcg_state = 12345u;

static float lcg_float(void)
{
    lcg_state = lcg_state * 1664525u + 1013904223u;

    const int32_t v = (int32_t)(lcg_state >> 8);

    return (float)v / (float)(1 << 22) - 2.0f;
}

int main(void)
{
    static float A[M * K], B[K * N], C[M * N], Cin_copy[M * N];
    static float out[M * N], want[M * N];

    for (int n = 0; n < M * K; n++) A[n] = lcg_float();
    for (int n = 0; n < K * N; n++) B[n] = lcg_float();

    /* Non-zero C on the way in, deliberately. linalg.matmul accumulates into
     * its output operand; a previous revision of the pass zeroed its
     * accumulator and overwrote C, which is indistinguishable from correct
     * whenever C starts at zero. Seeding it with real values is what makes
     * this test able to tell the difference. */
    for (int n = 0; n < M * N; n++) C[n] = lcg_float();

    memcpy(Cin_copy, C, sizeof(C));

    MemRef2D ma, mb, mc, mres;

    init_memref(&ma, A, M, K);
    init_memref(&mb, B, K, N);
    init_memref(&mc, C, M, N);
    memset(&mres, 0, sizeof(mres));

    _mlir_ciface_gemm(&mres, &ma, &mb, &mc);

    /* The result descriptor points at whatever buffer the lowered code chose
     * -- it may or may not be the same storage as C, depending on how
     * bufferization resolved the in-place update. Read through the returned
     * pointer rather than assuming. */
    for (int i = 0; i < M; i++)
        for (int j = 0; j < N; j++)
            out[i * N + j] =
                mres.aligned[mres.offset + i * mres.strides[0]
                             + j * mres.strides[1]];

    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            float acc = Cin_copy[i * N + j];

            for (int k = 0; k < K; k++)
                acc += A[i * K + k] * B[k * N + j];

            want[i * N + j] = acc;
        }
    }

    int bad = 0;

    for (int n = 0; n < M * N; n++) {
        if (memcmp(&out[n], &want[n], sizeof(float)) != 0) {
            if (bad < 5)
                printf("  [%d][%d] got=%.9g want=%.9g\n",
                       n / N, n % N, (double)out[n], (double)want[n]);
            bad++;
        }
    }

    printf("\n%dx%dx%d, %d tiles dispatched, %d/%d elements mismatched\n",
           M, K, N,
           ((M + 7) / 8) * ((N + 7) / 8) * ((K + 63) / 64),
           bad, M * N);

    printf("%s\n", bad ? "FAILED" : "BIT-EXACT");

    return bad ? 1 : 0;
}
