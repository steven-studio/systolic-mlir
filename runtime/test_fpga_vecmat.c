#include "fpga_matmul_tiled.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define K 7
#define N 5

int main() {
    float x[K], A[K*N];

    unsigned seed = 33;
    for (int i = 0; i < K; i++) {
        seed = seed*1103515245+12345;
        x[i] = ((int)((seed>>16)%2000)-1000)/100.0f;
    }
    for (int i = 0; i < K*N; i++) {
        seed = seed*1103515245+12345;
        A[i] = ((int)((seed>>16)%2000)-1000)/100.0f;
    }

    double y_ref[N];
    for (int j = 0; j < N; j++) {
        double acc = 0.0;
        for (int k = 0; k < K; k++)
            acc += (double)x[k] * (double)A[k*N+j];
        y_ref[j] = acc;
    }

    float y_hw[N];
    printf("vecmat: x[%d] @ A[%dx%d] -> y[%d] (非對齊,雙向 zero-padding)\n", K, K, N, N);

    int rc = fpga_vecmat_tiled_auto(K, N, x, A, y_hw);
    if (rc != 0) { printf("FAILED, rc=%d\n", rc); return 1; }

    int errors = 0;
    for (int j = 0; j < N; j++) {
        float diff = fabsf(y_hw[j] - (float)y_ref[j]);
        const char *status = (diff < 1e-2f) ? "PASS" : "FAIL";
        if (diff >= 1e-2f) errors++;
        printf("[%s] idx=%d  hw=%10.4f  ref=%10.4f  diff=%.6f\n",
               status, j, y_hw[j], (float)y_ref[j], diff);
    }
    printf("\n%s: %d/%d\n", errors==0 ? "ALL PASS" : "SOME FAILED", N-errors, N);
    return errors==0 ? 0 : 1;
}
