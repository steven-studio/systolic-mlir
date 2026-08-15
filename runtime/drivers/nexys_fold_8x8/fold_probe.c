/* fold_probe.c -- one invocation, everything dumped. For when run_fold_shape
 * comes back wrong and the question is what the board actually said.
 *
 *   ./bin/fold_probe            # k_dim = 8, A = B = 1.0  -> every C is 8.0
 *   ./bin/fold_probe 4          # k_dim = 4              -> every C is 4.0
 *   ./bin/fold_probe 64
 *
 * All-ones stimulus on purpose: every element of both contexts has the same
 * expected value, so a wrong answer is readable at a glance instead of
 * needing a reference matrix to compare against. With k_dim = 8 only window
 * 0 is fed, so ctx1 must be exactly zero -- that alone distinguishes several
 * failure modes.
 *
 *   ctx0 = 8.0 everywhere, ctx1 = 0.0 everywhere   correct
 *   ctx0 = ctx1 = 64.0                             k_dim ignored, full K_MAX
 *   ctx0 = 0.0                                     reduction ran before the
 *                                                  MACs landed (the drain
 *                                                  bug), or banks not cleared
 *   leading 0xA1..0xA5                             DEBUG_MARKERS is on
 *   garbage with sane values 4 bytes in            bitstream predates the
 *                                                  k_dim header
 */

#include "fpga_matmul_fold.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    const int k_max = fold_default_k_max();
    const int k_dim = (argc > 1) ? atoi(argv[1]) : 8;

    if (k_dim < 1 || k_dim > k_max) {
        fprintf(stderr, "k_dim must be in [1, %d]\n", k_max);
        return 2;
    }

    float *A = (float *)malloc((size_t)FOLD_R * (size_t)k_dim * sizeof(float));
    float *B = (float *)malloc((size_t)k_dim * FOLD_C * sizeof(float));
    if (!A || !B)
        return 1;

    for (int i = 0; i < FOLD_R * k_dim; i++) A[i] = 1.0f;
    for (int i = 0; i < k_dim * FOLD_C; i++) B[i] = 1.0f;

    /* Expected, per the RTL's own context rule: step lk lands in
     * context (lk >> 3) & 1. */
    int n0 = 0, n1 = 0;
    for (int lk = 0; lk < k_dim; lk++) {
        if ((lk >> 3) & 1)
            n1++;
        else
            n0++;
    }

    printf("K_MAX        : %d\n", k_max);
    printf("k_dim        : %d\n", k_dim);
    printf("request      : %zu bytes\n", fold_request_bytes(k_max));
    printf("expected ctx0: %d.0 in every element\n", n0);
    printf("expected ctx1: %d.0 in every element\n", n1);
    printf("\n");

    int fd = fold_open(NULL, 0);
    if (fd < 0) {
        fprintf(stderr, "cannot open UART (set FOLD_UART_PORT)\n");
        return 1;
    }

    float c0[FOLD_R * FOLD_C];
    float c1[FOLD_R * FOLD_C];
    memset(c0, 0, sizeof(c0));
    memset(c1, 0, sizeof(c1));

    int rc = fold_invoke(fd, k_max, k_dim, A, B, c0, c1);
    fold_close(fd);

    if (rc != 0) {
        fprintf(stderr, "fold_invoke failed: %d\n", rc);
        return 1;
    }

    /* Raw bytes first, before any float interpretation: a marker prefix or
     * a byte-shift is visible here and nowhere else. */
    const unsigned char *raw0 = (const unsigned char *)c0;
    const unsigned char *raw1 = (const unsigned char *)c1;

    printf("first 32 response bytes:\n  ");
    for (int i = 0; i < 32; i++) {
        printf("%02X ", raw0[i]);
        if (i % 16 == 15) printf("\n  ");
    }
    printf("\nbytes 256..271 (start of ctx1):\n  ");
    for (int i = 0; i < 16; i++)
        printf("%02X ", raw1[i]);
    printf("\n\n");

    printf("ctx0:\n");
    for (int r = 0; r < FOLD_R; r++) {
        printf("  ");
        for (int c = 0; c < FOLD_C; c++)
            printf("%12g", (double)c0[r * FOLD_C + c]);
        printf("\n");
    }

    printf("\nctx1:\n");
    for (int r = 0; r < FOLD_R; r++) {
        printf("  ");
        for (int c = 0; c < FOLD_C; c++)
            printf("%12g", (double)c1[r * FOLD_C + c]);
        printf("\n");
    }

    int bad0 = 0, bad1 = 0;
    for (int i = 0; i < FOLD_R * FOLD_C; i++) {
        if (c0[i] != (float)n0) bad0++;
        if (c1[i] != (float)n1) bad1++;
    }

    printf("\nctx0 wrong: %d / 64      ctx1 wrong: %d / 64\n", bad0, bad1);
    if (bad0 == 0 && bad1 == 0)
        printf("PASS\n");

    free(A);
    free(B);
    return (bad0 || bad1) ? 1 : 0;
}
