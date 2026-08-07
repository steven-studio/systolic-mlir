/* systolic_dispatch_sim_new.c -- host-CPU backend for systolic_dispatch_new.h.
 *
 * Exports the same two symbols as systolic_dispatch_uart_new.c and is
 * mutually exclusive with it at link time. No serial port, no board.
 *
 * The arithmetic here is not "a reasonable reference" -- it reproduces the
 * array's accumulation order exactly: float32 accumulator seeded with
 * C_init, k ascending. That ordering was measured against the hardware
 * rather than assumed, and three plausible alternatives were ruled out. A
 * reference that merely computed the same mathematical quantity in a
 * different order would agree to a few ULP and disagree bit-for-bit, so
 * every sim-vs-board comparison would need a tolerance -- and a tolerance
 * would hide precisely the layout and ordering bugs this backend exists to
 * catch before anyone touches a board.
 *
 * BUILD NOTE: compile with -ffp-contract=off. Otherwise the compiler may
 * fuse `acc + a*b` into an FMA, which carries more intermediate precision
 * than the FPGA's separate fmul and fadd, and this backend stops matching
 * hardware for a reason that has nothing to do with the code.
 */

#include "systolic_dispatch_new.h"

#include <stdio.h>

int systolic_dispatch_open(void)
{
    /* Nothing to open. Return a non-negative placeholder so callers that
     * test for a negative "failed to open" never see a false failure. */
    return 0;
}

int systolic_dispatch_matmul(int handle, int K,
                             const float *A, const float *B,
                             const float *C_init, float *C_out)
{
    (void)handle;

    /* Same range check the UART path applies. Enforcing it here is the
     * point of the sim backend: a pass that emits K=0 or K=65 should fail
     * loudly on a laptop, not hang waiting for bytes the board is still
     * expecting. */
    if (K < 1 || K > SYS_DISPATCH_K_MAX) {
        fprintf(stderr, "systolic(sim): K=%d out of range [1, %d]\n",
                K, SYS_DISPATCH_K_MAX);
        return -3;
    }

    for (int i = 0; i < SYS_DISPATCH_R; i++) {
        for (int j = 0; j < SYS_DISPATCH_C; j++) {
            /* Seeded with C_init, NOT zero -- see the header. C_out may
             * alias C_init, so read the seed before any store to C_out.
             * Reading it here rather than hoisting keeps that true even if
             * the loop is later restructured. */
            float acc = C_init[i * SYS_DISPATCH_C + j];

            for (int k = 0; k < K; k++)
                acc += A[i * K + k] * B[k * SYS_DISPATCH_C + j];

            C_out[i * SYS_DISPATCH_C + j] = acc;
        }
    }

    return 0;
}
