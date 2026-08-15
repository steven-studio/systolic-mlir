#include <cstdio>
#include <cmath>
#include "design.h"

static void reference_matmul(
    data_t A[R][K_MAX],
    data_t B[K_MAX][C],
    data_t Cin[R][C],
    data_t Cout[R][C],
    int K)
{
    int k_len =
        (K < 0) ? 0 :
        ((K > K_MAX) ? K_MAX : K);

    for (int i = 0; i < R; ++i) {
        for (int j = 0; j < C; ++j) {

            acc_t sum =
                Cin[i][j];

            for (int k = 0; k < k_len; ++k) {
                sum +=
                    A[i][k] * B[k][j];
            }

            Cout[i][j] =
                sum;
        }
    }
}


static bool run_test(int K)
{
    static data_t A[R][K_MAX];
    static data_t B[K_MAX][C];

    static data_t Cin[R][C];
    static data_t C_fp[R][C];
    static data_t C_ref[R][C];

    for (int i = 0; i < R; ++i) {
        for (int k = 0; k < K_MAX; ++k) {
            A[i][k] =
                ((i * 7 + k * 3) % 17 - 8)
                * 0.125f;
        }
    }

    for (int k = 0; k < K_MAX; ++k) {
        for (int j = 0; j < C; ++j) {
            B[k][j] =
                ((k * 5 + j * 11) % 19 - 9)
                * 0.1f;
        }
    }

    for (int i = 0; i < R; ++i) {
        for (int j = 0; j < C; ++j) {

            Cin[i][j] =
                (i - j) * 0.25f;

            C_fp[i][j] =
                Cin[i][j];

            C_ref[i][j] =
                0.0f;
        }
    }

    reference_matmul(
        A,
        B,
        Cin,
        C_ref,
        K
    );

    matmul_8x8_fold_pipelined(
        A,
        B,
        C_fp,
        K
    );

    bool pass = true;
    float max_error = 0.0f;

    for (int i = 0; i < R; ++i) {
        for (int j = 0; j < C; ++j) {

            float err =
                std::fabs(
                    (float)C_fp[i][j] -
                    (float)C_ref[i][j]
                );

            if (err > max_error)
                max_error = err;

            if (err > 1e-4f)
                pass = false;
        }
    }

    std::printf(
        "\nK=%d max_error=%g %s\n",
        K,
        max_error,
        pass ? "PASS" : "FAIL"
    );

    return pass;
}


int main()
{
    const int tests[] = {
        0,
        1,
        7,
        8,
        9,
        15,
        16,
        17,
        31,
        32,
        33,
        63,
        64
    };

    bool pass = true;

    for (int K : tests) {

        printf("\n==============================\n");
        printf("Testing K=%d\n", K);
        printf("==============================\n");

        if (!run_test(K))
            pass = false;
    }

    if (pass) {
        printf("\n=================================\n");
        printf("ALL FOLD-BEAT TESTS PASSED\n");
        printf("=================================\n");
        return 0;
    }

    printf("\nFAIL\n");
    return 1;
}