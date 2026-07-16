#include "fpga_matmul_tiled.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define M 5
#define K 7

int main() {
    float A[M*K], x[K];

    unsigned seed = 55;
    for (int i = 0; i < M*K; i++) {
        seed = seed*1103515245+12345;
        A[i] = ((int)((seed>>16)%2000)-1000)/100.0f;
    }
    for (int i = 0; i < K; i++) {
        seed = seed*1103515245+12345;
        x[i] = ((int)((seed>>16)%2000)-1000)/100.0f;
    }

    double y_ref[M];
    for (int i = 0; i < M; i++) {
        double acc = 0.0;
        for (int k = 0; k < K; k++)
            acc += (double)A[i*K+k] * (double)x[k];
        y_ref[i] = acc;
    }

    float y_hw[M];
    printf("matvec: A[%dx%d] @ x[%d] -> y[%d] (非對齊,雙向 zero-padding)\n", M, K, K, M);

    int rc = fpga_matvec_tiled_auto(M, K, A, x, y_hw);
    if (rc != 0) { printf("FAILED, rc=%d\n", rc); return 1; }

    int errors = 0;
    for (int i = 0; i < M; i++) {
        float diff = fabsf(y_hw[i] - (float)y_ref[i]);
        const char *status = (diff < 1e-2f) ? "PASS" : "FAIL";
        if (diff >= 1e-2f) errors++;
        printf("[%s] idx=%d  hw=%10.4f  ref=%10.4f  diff=%.6f\n",
               status, i, y_hw[i], (float)y_ref[i], diff);
    }
    printf("\n%s: %d/%d\n", errors==0 ? "ALL PASS" : "SOME FAILED", M-errors, M);
    return errors==0 ? 0 : 1;
}
