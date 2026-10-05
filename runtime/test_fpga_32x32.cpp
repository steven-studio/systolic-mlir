#include "fpga_matmul_tiled.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

static void printMatrix(
    const char *name,
    const std::vector<float> &X,
    int rows,
    int cols) {

    std::printf("\n%s:\n", name);

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j)
            std::printf("%8.1f ", X[i * cols + j]);
        std::printf("\n");
    }
}

int main() {
    constexpr int M = 32;
    constexpr int K = 32;
    constexpr int N = 32;

    std::vector<float> A(M * K);
    std::vector<float> B(K * N, 0.0f);
    std::vector<float> C(M * N, 0.0f);
    std::vector<float> Ref(M * N, 0.0f);

    // A =
    // [   1    2 ...   32 ]
    // [  33   34 ...   64 ]
    // ...
    // [ 993  994 ... 1024 ]
    for (int i = 0; i < M * K; ++i)
        A[i] = static_cast<float>(i + 1);

    // B = I_32.
    for (int i = 0; i < K; ++i)
        B[i * N + i] = 1.0f;

    // CPU reference.
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            float sum = 0.0f;
            for (int k = 0; k < K; ++k)
                sum += A[i * K + k] * B[k * N + j];
            Ref[i * N + j] = sum;
        }
    }

    std::printf("=== FPGA 32x32 GEMM E2E ===\n");
    std::printf("M=%d K=%d N=%d\n", M, K, N);
    std::printf("A = sequential 1..1024\n");
    std::printf("B = 32x32 identity\n");
    std::printf("Expected: C = A\n");

    std::printf("\nCalling FPGA...\n");

    int rc = fpga_matmul_tiled_auto(
        M, K, N,
        A.data(),
        B.data(),
        C.data());

    if (rc != 0) {
        std::fprintf(
            stderr,
            "ERROR: fpga_matmul_tiled_auto failed, rc=%d\n",
            rc);
        return 1;
    }

    printMatrix("FPGA result C (32x32)", C, M, N);

    float maxAbsError = 0.0f;
    int badCount = 0;

    for (int i = 0; i < M * N; ++i) {
        float err = std::fabs(C[i] - Ref[i]);
        maxAbsError = std::max(maxAbsError, err);

        if (err > 1.0e-4f)
            ++badCount;
    }

    std::printf("\n========================================\n");
    std::printf("max_abs_error = %.8f\n", maxAbsError);
    std::printf("bad_elements  = %d / %d\n", badCount, M * N);

    if (badCount == 0) {
        std::printf("RESULT: PASS\n");
        std::printf("FPGA computed the complete 32x32 GEMM correctly.\n");
        return 0;
    }

    std::printf("RESULT: FAIL\n");
    return 1;
}
