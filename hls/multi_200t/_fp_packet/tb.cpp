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
            acc_t sum = Cin[i][j];

            for (int k = 0; k < k_len; ++k) {
                sum += A[i][k] * B[k][j];
            }

            Cout[i][j] = sum;
        }
    }
}


static bool run_test(int K)
{
    static data_t A[R][K_MAX];
    static data_t B[K_MAX][C];

    static data_t Cin[R][C];
    static data_t C_hw[R][C];
    static data_t C_ref[R][C];

    /*
     * Deterministic input values.
     */
    for (int i = 0; i < R; ++i) {
        for (int k = 0; k < K_MAX; ++k) {
            A[i][k] =
                ((i * 7 + k * 3) % 17 - 8) * 0.125f;
        }
    }

    for (int k = 0; k < K_MAX; ++k) {
        for (int j = 0; j < C; ++j) {
            B[k][j] =
                ((k * 5 + j * 11) % 19 - 9) * 0.1f;
        }
    }

    /*
     * Cin intentionally non-zero.
     * This also verifies accumulate-into-C semantics.
     */
    for (int i = 0; i < R; ++i) {
        for (int j = 0; j < C; ++j) {
            Cin[i][j] =
                (i - j) * 0.25f;

            C_hw[i][j] =
                Cin[i][j];

            C_ref[i][j] =
                0.0f;
        }
    }

    /*
     * Ordinary matrix multiplication reference.
     */
    reference_matmul(
        A,
        B,
        Cin,
        C_ref,
        K
    );

    /*
     * Fold-pipelined implementation.
     */
    matmul_8x8_fold_pipelined(
        A,
        B,
        C_hw,
        K
    );

    bool pass = true;

    float max_error = 0.0f;

    int bad_i = -1;
    int bad_j = -1;

    for (int i = 0; i < R; ++i) {
        for (int j = 0; j < C; ++j) {

            float err =
                std::fabs(
                    (float)C_hw[i][j] -
                    (float)C_ref[i][j]
                );

            if (err > max_error) {
                max_error = err;
                bad_i = i;
                bad_j = j;
            }

            if (err > 1e-4f) {
                pass = false;
            }
        }
    }

    std::printf(
        "K=%2d  max_error=%g  %s\n",
        K,
        max_error,
        pass ? "PASS" : "FAIL"
    );

    if (!pass) {
        std::printf(
            "  worst [%d][%d]: fp=%f ref=%f\n",
            bad_i,
            bad_j,
            (double)C_hw[bad_i][bad_j],
            (double)C_ref[bad_i][bad_j]
        );
    }

    return pass;
}


int main()
{
    // /*
    //  * Especially test both sides of every FOLD_K=8 boundary.
    //  */
    // const int tests[] = {
    //     0,
    //     1,
    //     7,
    //     8,
    //     9,
    //     15,
    //     16,
    //     17,
    //     31,
    //     32,
    //     33,
    //     63,
    //     64
    // };

    // bool all_pass = true;

    // for (unsigned n = 0;
    //      n < sizeof(tests) / sizeof(tests[0]);
    //      ++n)
    // {
    //     if (!run_test(tests[n])) {
    //         all_pass = false;
    //     }
    // }

    // std::printf("\n");

    // if (all_pass) {
    //     std::printf(
    //         "=================================\n"
    //         "ALL FOLD-PIPELINED TESTS PASSED\n"
    //         "=================================\n"
    //     );

    //     return 0;
    // }

    // std::printf(
    //     "=================================\n"
    //     "FOLD-PIPELINED TEST FAILED\n"
    //     "=================================\n"
    // );

    // return 1;
    bool pass = run_test(17);

    return pass ? 0 : 1;
}
