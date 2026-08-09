#include <cstdio>
#include "design.h"

int main() {
    static data_t A[R][K_MAX] = {};
    static data_t B[K_MAX][C] = {};
    static data_t Cmat[R][C] = {};

    for (int i = 0; i < R; ++i) {
        for (int k = 0; k < K_MAX; ++k) {
            A[i][k] = 1.0f;
        }
    }

    for (int k = 0; k < K_MAX; ++k) {
        for (int j = 0; j < C; ++j) {
            B[k][j] = 1.0f;
        }
    }

    printf("=== K=64 continuous-fold test ===\n");

    matmul_8x8x8(A, B, Cmat, 64);

    printf("=== done ===\n");
    printf("C[0][0] = %f\n", (double)Cmat[0][0]);

    return 0;
}
