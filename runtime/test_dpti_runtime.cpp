#include "dpti_runtime.h"

#include <cmath>
#include <cstdio>

static void fill_matrix(
    float *x,
    int rows,
    int cols)
{
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            x[i * cols + j] =
                static_cast<float>((i + 1) * (j + 1));
        }
    }
}

static void software_matmul(
    const float *A,
    const float *B,
    float *C,
    int M,
    int K,
    int N)
{
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            float acc = 0.0f;

            for (int k = 0; k < K; ++k)
                acc += A[i * K + k] * B[k * N + j];

            C[i * N + j] = acc;
        }
    }
}

int main()
{
    constexpr int M = 4;
    constexpr int K = 4;
    constexpr int N = 4;

    float A[M * K];
    float B[K * N];
    float C_hw[M * N];
    float C_ref[M * N];

    fill_matrix(A, M, K);
    fill_matrix(B, K, N);

    software_matmul(
        A,
        B,
        C_ref,
        M,
        K,
        N);

    DptiDevice dev{-1, 0};

    constexpr const char *PORT = "/dev/ttyUSB2";

    int rc = dpti_device_open(
        &dev,
        PORT);

    if (rc != 0) {
        std::fprintf(
            stderr,
            "DPTI: device open failed rc=%d\n",
            rc);
        return 1;
    }

    DptiJob job{};

    job.job_id      = 1;
    job.device_id   = 0;

    job.m           = M;
    job.n           = N;
    job.k           = K;

    job.start_cycle = 0;

    /*
     * Current calibrated 8x8 hardware model:
     *
     *   T = K + 2(N-1) + H
     *
     * with H = 95.
     *
     * This smoke test uses a 4x4 logical GEMM routed through the
     * existing tiled runtime, so this field is ABI validation only.
     */
    job.est_cycles =
        K + 2 * (8 - 1) + 95;

    job.a_base = 0;
    job.b_base = 0;
    job.c_base = 0;

    std::printf(
        "==============================================\n"
        " DPTI Runtime Smoke Test\n"
        "==============================================\n");

    std::printf(
        " job_id       = %u\n"
        " device_id    = %u\n"
        " shape        = %ux%ux%u\n"
        " start_cycle  = %u\n"
        " est_cycles   = %u\n",
        job.job_id,
        job.device_id,
        job.m,
        job.k,
        job.n,
        job.start_cycle,
        job.est_cycles);

    rc = dpti_execute_matmul(
        &dev,
        &job,
        A,
        B,
        C_hw);

    dpti_device_close(&dev);

    if (rc != 0) {
        std::fprintf(
            stderr,
            "DPTI: FPGA execution failed rc=%d\n",
            rc);
        return 1;
    }

    bool pass = true;

    for (int i = 0; i < M * N; ++i) {
        if (std::fabs(C_hw[i] - C_ref[i]) > 1e-3f) {
            std::fprintf(
                stderr,
                "FAIL C[%d]: hw=%f ref=%f\n",
                i,
                C_hw[i],
                C_ref[i]);

            pass = false;
        }
    }

    if (!pass) {
        std::printf("\nDPTI: FAIL\n");
        return 1;
    }

    std::printf("\nDPTI: PASS\n");

    return 0;
}
