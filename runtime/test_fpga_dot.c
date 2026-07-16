#include "fpga_matmul_tiled.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define K 9

int main() {
    float x[K], y[K];

    unsigned seed = 77;
    for (int i = 0; i < K; i++) {
        seed = seed*1103515245+12345;
        x[i] = ((int)((seed>>16)%2000)-1000)/100.0f;
    }
    for (int i = 0; i < K; i++) {
        seed = seed*1103515245+12345;
        y[i] = ((int)((seed>>16)%2000)-1000)/100.0f;
    }

    double ref = 0.0;
    for (int k = 0; k < K; k++)
        ref += (double)x[k] * (double)y[k];

    float result;
    printf("dot: x[%d] . y[%d] -> scalar (非對齊, 觸發 zero-padding)\n", K, K);

    int rc = fpga_dot_tiled_auto(K, x, y, &result);
    if (rc != 0) { printf("FAILED, rc=%d\n", rc); return 1; }

    float diff = fabsf(result - (float)ref);
    const char *status = (diff < 1e-2f) ? "PASS" : "FAIL";
    printf("[%s] hw=%12.4f  ref=%12.4f  diff=%.6f\n", status, result, (float)ref, diff);
    return diff < 1e-2f ? 0 : 1;
}
