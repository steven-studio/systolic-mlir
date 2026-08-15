/* hw_test_new.c -- prove sys_matmul() against the live board.
 *
 * The packing is already known good (byte-identical to dump_ref_new.py), and
 * the first run confirmed the transport: K=1 came back BIT-EXACT, so frames
 * go out, the board computes, and 256 bytes come back correctly decoded.
 *
 * What that run did NOT establish is the accumulation order. K=1 passing
 * while every K>=8 failed is diagnostic rather than mysterious: float
 * addition is commutative but not associative, so any two orderings that
 * differ only in where C_init enters -- or in the direction of k -- agree
 * at K=1 and diverge from K=2 onward.
 *
 * Rather than guess which ordering the array uses, this file evaluates
 * several candidates and reports which one the hardware matches. One run
 * settles it, and the answer is needed again in the MLIR pass: chaining
 * K>64 as successive accumulating calls is only correct if we know where
 * C_init enters the sum.
 *
 * BUILD -- the -ffp-contract=off is load bearing:
 *
 *   gcc -O2 -ffp-contract=off -o hwtest \
 *       hw_test_new.c fpga_matmul_rk_new.c lib/fpga_matmul4x4.c -Ilib
 *
 * Without it gcc may fuse `acc + a*b` into a single FMA, which keeps more
 * intermediate precision than the FPGA's separate fmul and fadd. Every
 * candidate below would then be wrong in the same way, and the run would
 * report "no candidate matched" for a reason unrelated to ordering.
 */

#include "fpga_matmul_rk_new.h"
#include "fpga_matmul4x4.h"   /* fpga_uart_open_baud, fpga_uart_close */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BAUD 2000000

/* ------------------------------------------------------------------ */
/* Deterministic input                                                */
/* ------------------------------------------------------------------ */

/* A fixed LCG rather than rand(), so a failure reproduces across machines
 * and libc versions. Chasing a bit-level mismatch against input that
 * changes every run is not worth the misery. */
static uint32_t lcg_state;

static void lcg_seed(uint32_t s) { lcg_state = s ? s : 1u; }

static float lcg_float(void)
{
    lcg_state = lcg_state * 1664525u + 1013904223u;

    /* Roughly [-2, 2), kept clear of denormals and large exponents: the
     * point is to exercise the datapath, not IEEE corner cases. */
    const int32_t v = (int32_t)(lcg_state >> 8);

    return (float)v / (float)(1 << 22) - 2.0f;
}

/* ------------------------------------------------------------------ */
/* Candidate accumulation orders                                      */
/* ------------------------------------------------------------------ */

/* All accumulators are float, never double. Widening would disagree with
 * the array for the same reason FMA would. */

/* A: seed the accumulator with C_init, then add products with k ascending.
 * This is what the first version of this file assumed. */
static void ref_seed_cin(int K, const float *A, const float *B,
                         const float *C_init, float *C_out)
{
    for (int i = 0; i < SYS_R; i++) {
        for (int j = 0; j < SYS_C; j++) {
            float acc = C_init[i * SYS_C + j];

            for (int k = 0; k < K; k++)
                acc += A[i * K + k] * B[k * SYS_C + j];

            C_out[i * SYS_C + j] = acc;
        }
    }
}

/* B: accumulate products from zero with k ascending, add C_init last.
 * This is the natural shape if the kernel zeroes acc[][] in its init loop
 * and folds Cinout in on the way out. */
static void ref_add_cin_last(int K, const float *A, const float *B,
                             const float *C_init, float *C_out)
{
    for (int i = 0; i < SYS_R; i++) {
        for (int j = 0; j < SYS_C; j++) {
            float acc = 0.0f;

            for (int k = 0; k < K; k++)
                acc += A[i * K + k] * B[k * SYS_C + j];

            C_out[i * SYS_C + j] = acc + C_init[i * SYS_C + j];
        }
    }
}

/* C: as B but with k descending -- included only to rule it out. The array
 * feeds beat t with k = t - i - j, so k should ascend; if this is the one
 * that matches, the skew derivation is wrong somewhere. */
static void ref_desc_cin_last(int K, const float *A, const float *B,
                              const float *C_init, float *C_out)
{
    for (int i = 0; i < SYS_R; i++) {
        for (int j = 0; j < SYS_C; j++) {
            float acc = 0.0f;

            for (int k = K - 1; k >= 0; k--)
                acc += A[i * K + k] * B[k * SYS_C + j];

            C_out[i * SYS_C + j] = acc + C_init[i * SYS_C + j];
        }
    }
}

/* D: seed with C_init, k descending. The fourth corner, for completeness. */
static void ref_seed_cin_desc(int K, const float *A, const float *B,
                              const float *C_init, float *C_out)
{
    for (int i = 0; i < SYS_R; i++) {
        for (int j = 0; j < SYS_C; j++) {
            float acc = C_init[i * SYS_C + j];

            for (int k = K - 1; k >= 0; k--)
                acc += A[i * K + k] * B[k * SYS_C + j];

            C_out[i * SYS_C + j] = acc;
        }
    }
}

typedef void (*ref_fn)(int, const float *, const float *,
                       const float *, float *);

static const struct {
    const char *name;
    ref_fn      fn;
} CANDIDATES[] = {
    { "seed=Cin  k asc ", ref_seed_cin },
    { "seed=0    k asc ", ref_add_cin_last },
    { "seed=0    k desc", ref_desc_cin_last },
    { "seed=Cin  k desc", ref_seed_cin_desc },
};

#define N_CANDIDATES ((int)(sizeof(CANDIDATES) / sizeof(CANDIDATES[0])))

/* ------------------------------------------------------------------ */
/* Cases                                                              */
/* ------------------------------------------------------------------ */

/* Bit set per candidate that matched, so main() can intersect across cases.
 * A candidate only wins if it matches EVERY case -- at K=1 all four agree,
 * so a per-case verdict would be meaningless on its own. */
static int run_case(int fd, int dev, int K, const char *label, int *mask_out)
{
    static float A[SYS_R * SYS_K_MAX];
    static float B[SYS_K_MAX * SYS_C];
    static float Cin[SYS_R * SYS_C];
    static float got[SYS_R * SYS_C];
    static float want[SYS_R * SYS_C];

    lcg_seed((uint32_t)(K * 7919 + dev * 104729 + 1));

    for (int n = 0; n < SYS_R * K; n++)
        A[n] = lcg_float();

    for (int n = 0; n < K * SYS_C; n++)
        B[n] = lcg_float();

    for (int n = 0; n < SYS_R * SYS_C; n++)
        Cin[n] = lcg_float();

    const int rc = sys_matmul(fd, dev, K, A, B, Cin, got);

    if (rc != 0) {
        printf("  dev=%d K=%-3d %-12s TRANSPORT FAILED rc=%d\n",
               dev, K, label, rc);
        *mask_out = 0;
        return 1;
    }

    int mask = 0;

    for (int c = 0; c < N_CANDIDATES; c++) {
        CANDIDATES[c].fn(K, A, B, Cin, want);

        /* memcmp, not a tolerance loop: bit-exact means bit-exact, and the
         * raw comparison also catches a NaN that would slip past `==`. */
        if (memcmp(got, want, sizeof(want)) == 0)
            mask |= (1 << c);
    }

    *mask_out = mask;

    printf("  dev=%d K=%-3d %-12s ", dev, K, label);

    if (mask == 0) {
        printf("NO CANDIDATE MATCHED\n");
        return 1;
    }

    for (int c = 0; c < N_CANDIDATES; c++)
        if (mask & (1 << c))
            printf("[%s] ", CANDIDATES[c].name);

    printf("\n");

    return 0;
}

int main(int argc, char **argv)
{
    const char *port = (argc > 1) ? argv[1] : "/dev/ttyUSB2";

    const int fd = fpga_uart_open_baud(port, BAUD);

    if (fd < 0) {
        fprintf(stderr, "cannot open %s at %d\n", port, BAUD);
        return 1;
    }

    printf("port=%s baud=%d\n\n", port, BAUD);

    const struct { int dev, K; const char *label; } CASES[] = {
        { 0,  8, "regression" },
        { 1,  8, "regression" },
        { 0,  1, "min"        },
        { 0, 63, "non-pow2"   },
        { 0, 64, "K_MAX"      },
        { 1, 64, "K_MAX"      },
    };

    const int n_cases = (int)(sizeof(CASES) / sizeof(CASES[0]));

    int transport_fails = 0;
    int surviving = (1 << N_CANDIDATES) - 1;   /* all candidates alive */

    for (int t = 0; t < n_cases; t++) {
        int mask = 0;

        transport_fails += run_case(fd, CASES[t].dev, CASES[t].K,
                                    CASES[t].label, &mask);

        surviving &= mask;
    }

    fpga_uart_close(fd);

    printf("\n");

    if (transport_fails) {
        printf("TRANSPORT BROKEN on %d case(s) -- ordering is moot\n",
               transport_fails);
        return 1;
    }

    int n_surviving = 0;

    for (int c = 0; c < N_CANDIDATES; c++)
        if (surviving & (1 << c))
            n_surviving++;

    if (n_surviving == 0) {
        printf("transport OK, but NO ordering matched every case.\n"
               "The array is doing something none of the four candidates\n"
               "model -- read the init/drain loops in _rk/design.cpp.\n");
        return 1;
    }

    printf("transport OK on all %d cases.\n", n_cases);
    printf("ordering(s) consistent with every case:\n");

    for (int c = 0; c < N_CANDIDATES; c++)
        if (surviving & (1 << c))
            printf("    %s\n", CANDIDATES[c].name);

    if (n_surviving > 1)
        printf("\nMore than one survived -- the cases do not separate them.\n"
               "Harmless for correctness (they agree on this input) but the\n"
               "MLIR pass needs a single answer before chaining K>64.\n");

    return 0;
}
