#include "fpga_matmul4x4.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

#define PORT "/dev/ttyUSB2"

static void print_matrix(const char *name, const float *M) {
    printf("\n%s =\n", name);

    for (int i = 0; i < 4; ++i) {
        printf("[ ");
        for (int j = 0; j < 4; ++j) {
            printf("%10.5f", M[i * 4 + j]);
            if (j != 3)
                printf(" ");
        }
        printf(" ]\n");
    }
}

static void software_matmul(
    const float *A,
    const float *B,
    const float *C_init,
    float *C_ref) {

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            double acc = C_init[i * 4 + j];

            for (int k = 0; k < 4; ++k) {
                acc +=
                    static_cast<double>(A[i * 4 + k]) *
                    static_cast<double>(B[k * 4 + j]);
            }

            C_ref[i * 4 + j] =
                static_cast<float>(acc);
        }
    }
}

int main() {

    // ------------------------------------------------------------
    // Same 4x4 GEMM used by the existing FPGA runtime test.
    //
    // C = A @ B + C_init
    // ------------------------------------------------------------

    const float A[16] = {
         0.5f,     1.25f,   -2.75f,   3.1f,
        -1.6f,     2.333f,   0.001f, -4.9f,
         3.14159f,-0.5f,     1.1f,    2.2f,
         0.0001f,  7.7f,    -3.333f,  0.618f
    };

    const float B[16] = {
         1.1f,   -2.2f,    3.3f,    -4.4f,
         0.05f,   0.15f,  -0.25f,    0.35f,
        -1.0f,    2.71828f,-3.5f,     0.9f,
        10.1f,   -0.01f,    0.001f, -100.5f
    };

    const float C_init[16] = {
         0.1f,   -0.2f,    0.3f,   -0.4f,
         1.5f,   -1.5f,     2.5f,   -2.5f,
         0.0f,  100.0f,  -100.0f,    0.5f,
        -0.001f, 0.002f,  -0.003f,  42.42f
    };

    float C_ref[16];
    float C_hw[16];

    software_matmul(A, B, C_init, C_ref);

    print_matrix("A", A);
    print_matrix("B", B);
    print_matrix("C_init", C_init);

    // ------------------------------------------------------------
    // Real FPGA execution.
    // ------------------------------------------------------------

    printf("\n========================================\n");
    printf(" FPGA E2E 4x4 execution\n");
    printf("========================================\n");

    printf("Opening UART: %s\n", PORT);

    int fd = fpga_uart_open(PORT);

    if (fd < 0) {
        fprintf(stderr,
                "ERROR: fpga_uart_open(%s) failed\n",
                PORT);
        return 1;
    }

    printf("UART connected: fd=%d\n", fd);

    printf("\nDispatching scheduled 4x4 tile to FPGA...\n");
    printf("geometry    = 4x4\n");
    printf("M           = 4\n");
    printf("K           = 4\n");
    printf("N           = 4\n");
    printf("dataflow    = output_stationary\n");
    printf("operation   = C = A @ B + C_init\n");

    int rc =
        fpga_matmul4x4(
            fd,
            A,
            B,
            C_init,
            C_hw);

    fpga_uart_close(fd);

    if (rc != 0) {
        fprintf(stderr,
                "\nERROR: fpga_matmul4x4 failed, rc=%d\n",
                rc);
        return 1;
    }

    // ------------------------------------------------------------
    // Print actual FPGA result.
    // ------------------------------------------------------------

    print_matrix("C_ref (software)", C_ref);
    print_matrix("C_hw (FPGA)", C_hw);

    // ------------------------------------------------------------
    // Compare.
    // ------------------------------------------------------------

    printf("\n========================================\n");
    printf(" Verification\n");
    printf("========================================\n");

    int errors = 0;

    for (int i = 0; i < 16; ++i) {

        float diff =
            std::fabs(C_hw[i] - C_ref[i]);

        if (diff >= 1e-3f) {
            ++errors;

            printf(
                "[FAIL] idx=%2d "
                "hw=%12.6f "
                "ref=%12.6f "
                "diff=%g\n",
                i,
                C_hw[i],
                C_ref[i],
                diff);
        }
    }

    if (errors == 0) {

        printf(
            "\nPASS: FPGA 4x4 matrix multiplication "
            "16/16 elements match.\n");

        printf("\nFirst FPGA 4x4 multiplication completed.\n");

        return 0;
    }

    printf(
        "\nFAIL: %d / 16 elements mismatch.\n",
        errors);

    return 1;
}
