/* run_fold_shape.c -- run one GEMM shape on the fold array and check it.
 *
 *   ./bin/run_fold_shape M K N [seed]
 *   ./bin/run_fold_shape 8 65 8
 *   ./bin/run_fold_shape 24 100 16
 *   FOLD_K_MAX=64 FOLD_UART_PORT=/dev/ttyUSB2 ./bin/run_fold_shape 8 128 8
 *
 * The fold counterpart of run_shape.c, which speaks the old 4x4 protocol
 * and cannot drive this bitstream. See fpga_matmul_fold.h for why the two
 * are not interchangeable.
 *
 * Stimulus is small integers so every product and partial sum is exactly
 * representable in float32. The hardware's summation order differs from
 * the host reference -- 16 rotating accumulator banks, then a linear
 * reduction, then a host-side sum across contexts and invocations -- so a
 * floating-point stimulus would legitimately differ in the last ulp.
 * Integers remove that ambiguity and let this demand bit-exactness.
 */

#include "fpga_matmul_fold.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static float a_val(int r, int k, unsigned seed)
{
    return (float)((int)(((unsigned)(r * 31 + k * 17 + seed)) % 7u) - 3);
}

static float b_val(int k, int c, unsigned seed)
{
    return (float)((int)(((unsigned)(k * 13 + c * 7 + seed)) % 5u) - 2);
}

int main(int argc, char **argv)
{
    if (argc < 4) {
        fprintf(stderr,
                "usage: %s M K N [seed]\n"
                "  env: FOLD_K_MAX (default 64)\n"
                "       FOLD_UART_PORT (default /dev/ttyUSB2)\n"
                "       FOLD_UART_BAUD (default 115200)\n",
                argv[0]);
        return 2;
    }

    const int M = atoi(argv[1]);
    const int K = atoi(argv[2]);
    const int N = atoi(argv[3]);
    const unsigned seed = (argc > 4) ? (unsigned)atoi(argv[4]) : 0u;

    if (M < 1 || K < 1 || N < 1) {
        fprintf(stderr, "M, K, N must all be >= 1\n");
        return 2;
    }

    const int k_max = fold_default_k_max();
    const int n_inv_per_tile = (K + k_max - 1) / k_max;
    const int tiles = ((M + FOLD_R - 1) / FOLD_R)
                    * ((N + FOLD_C - 1) / FOLD_C);

    printf("shape        : %d x %d x %d\n", M, K, N);
    printf("K_MAX        : %d\n", k_max);
    printf("output tiles : %d  (%dx%d each)\n", tiles, FOLD_R, FOLD_C);
    printf("invocations  : %d per tile, %d total\n",
           n_inv_per_tile, n_inv_per_tile * tiles);
    printf("request size : %zu bytes\n", fold_request_bytes(k_max));
    printf("\n");

    float *A = (float *)malloc((size_t)M * (size_t)K * sizeof(float));
    float *B = (float *)malloc((size_t)K * (size_t)N * sizeof(float));
    float *C = (float *)malloc((size_t)M * (size_t)N * sizeof(float));
    float *R = (float *)malloc((size_t)M * (size_t)N * sizeof(float));
    if (!A || !B || !C || !R) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    for (int i = 0; i < M; i++)
        for (int k = 0; k < K; k++)
            A[(size_t)i * K + k] = a_val(i, k, seed);

    for (int k = 0; k < K; k++)
        for (int j = 0; j < N; j++)
            B[(size_t)k * N + j] = b_val(k, j, seed);

    for (int i = 0; i < M; i++)
        for (int j = 0; j < N; j++) {
            float acc = 0.0f;
            for (int k = 0; k < K; k++)
                acc += A[(size_t)i * K + k] * B[(size_t)k * N + j];
            R[(size_t)i * N + j] = acc;
        }

    int fd = fold_open(NULL, 0);
    if (fd < 0) {
        fprintf(stderr, "cannot open UART. Set FOLD_UART_PORT if the board "
                        "is not on /dev/ttyUSB2.\n");
        return 1;
    }

    int rc = fold_matmul_tiled(fd, k_max, M, K, N, A, B, C);
    fold_close(fd);

    if (rc != 0) {
        fprintf(stderr, "fold_matmul_tiled failed: %d\n", rc);
        switch (rc) {
        case -1:
            fprintf(stderr, "  write failed -- port went away?\n");
            break;
        case -2:
            fprintf(stderr,
                "  read timed out. Three usual causes:\n"
                "   1. no reset after programming: k_dim loads K_MAX on\n"
                "      reset only, so press BTNC before the first request;\n"
                "   2. FOLD_K_MAX (%d) disagrees with the loaded bitstream,\n"
                "      so the request length is wrong and the board is still\n"
                "      waiting for more bytes;\n"
                "   3. the bitstream predates the k_dim header, in which\n"
                "      case it read our 4 header bytes as payload and is\n"
                "      short by exactly 4.\n", k_max);
            break;
        case -3:
            fprintf(stderr, "  invalid argument\n");
            break;
        default:
            break;
        }
        return 1;
    }

    int mismatches = 0;
    float worst = 0.0f;

    for (int i = 0; i < M * N; i++) {
        const float d = C[i] - R[i];
        const float ad = d < 0 ? -d : d;
        if (ad > worst)
            worst = ad;
        if (C[i] != R[i])
            mismatches++;
    }

    printf("mismatches   : %d / %d\n", mismatches, M * N);
    printf("max |error|  : %g\n", (double)worst);

    if (mismatches == 0) {
        printf("\nPASS: %dx%dx%d bit-exact on hardware "
               "(%d invocation(s) per tile).\n",
               M, K, N, n_inv_per_tile);
    } else {
        printf("\nFAIL\n");
        const int lim = (M * N < 16) ? M * N : 16;
        for (int i = 0, shown = 0; i < M * N && shown < lim; i++)
            if (C[i] != R[i]) {
                printf("  C[%d][%d] got=%g exp=%g\n",
                       i / N, i % N, (double)C[i], (double)R[i]);
                shown++;
            }
    }

    free(A); free(B); free(C); free(R);
    return mismatches == 0 ? 0 : 1;
}
