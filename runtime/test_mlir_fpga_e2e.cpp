#include "fpga_matmul_tiled.h"

#include <cmath>
#include <cstdio>

int main() {
    constexpr int M = 4;
    constexpr int K = 4;
    constexpr int N = 4;

    const float A[M * K] = {
         1.0f, 2.0f, 3.0f, 4.0f,
         5.0f, 6.0f, 7.0f, 8.0f,
         9.0f, 10.0f, 11.0f, 12.0f,
         13.0f, 14.0f, 15.0f, 16.0f
    };

    const float B[K * N] = {
         1.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 1.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 1.0f, 0.0f,
         0.0f, 0.0f, 0.0f, 1.0f
    };

    float C[M * N] = {};

    std::printf("=== MLIR -> FPGA 4x4 E2E ===\n\n");

    std::printf("A:\n");
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < K; ++j)
            std::printf("%8.3f ", A[i * K + j]);
        std::printf("\n");
    }

    std::printf("\nB:\n");
    for (int i = 0; i < K; ++i) {
        for (int j = 0; j < N; ++j)
            std::printf("%8.3f ", B[i * N + j]);
        std::printf("\n");
    }

    std::printf("\nCalling FPGA...\n");

    int rc = fpga_matmul_tiled_auto(
        M, K, N,
        A, B, C);

    if (rc != 0) {
        std::fprintf(
            stderr,
            "ERROR: fpga_matmul_tiled_auto failed, rc=%d\n",
            rc);
        return 1;
    }

    std::printf("\nC = A @ B:\n");

    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j)
            std::printf("%8.3f ", C[i * N + j]);
        std::printf("\n");
    }

    return 0;
}
