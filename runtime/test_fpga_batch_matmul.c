#include "fpga_matmul_tiled.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define BATCH 3
#define M 5
#define K 5
#define N 5

int main() {
    float A[BATCH*M*K], B[BATCH*K*N], C_init[BATCH*M*N];

    unsigned seed = 21;
    for (int i = 0; i < BATCH*M*K; i++) {
        seed = seed*1103515245+12345;
        A[i] = ((int)((seed>>16)%2000)-1000)/100.0f;
    }
    for (int i = 0; i < BATCH*K*N; i++) {
        seed = seed*1103515245+12345;
        B[i] = ((int)((seed>>16)%2000)-1000)/100.0f;
    }

    double Y_ref[BATCH*M*N];
    for (int b = 0; b < BATCH; b++)
        for (int i = 0; i < M; i++)
            for (int j = 0; j < N; j++) {
                double acc = 0.0;
                for (int k = 0; k < K; k++)
                    acc += (double)A[(b*M+i)*K+k] * (double)B[(b*K+k)*N+j];
                Y_ref[(b*M+i)*N+j] = acc;
            }

    float C_out[BATCH*M*N];
    printf("batch_matmul: batch=%d, %dx%dx%d each (非對齊 5x5x5, 會觸發 zero-padding)\n",
           BATCH, M, K, N);

    int rc = fpga_batch_matmul_tiled_auto(BATCH, M, K, N, A, B, C_out);
    if (rc != 0) { printf("FAILED, rc=%d\n", rc); return 1; }

    int total = BATCH*M*N, errors = 0;
    for (int i = 0; i < total; i++) {
        float diff = fabsf(C_out[i] - (float)Y_ref[i]);
        const char *status = (diff < 1e-2f) ? "PASS" : "FAIL";
        if (diff >= 1e-2f) errors++;
        printf("[%s] idx=%2d  hw=%10.4f  ref=%10.4f  diff=%.6f\n",
               status, i, C_out[i], (float)Y_ref[i], diff);
    }
    printf("\n%s: %d/%d\n", errors==0 ? "ALL PASS" : "SOME FAILED", total-errors, total);
    return errors==0 ? 0 : 1;
}
