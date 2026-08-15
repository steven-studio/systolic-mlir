#include "fpga_conv2d_im2col.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

// batch=2, 8x8 輸入, 3x3 kernel, strideH=1 strideW=2 (不對稱),
// dilationH=2 dilationW=1 (有 dilation)
#define N 2
#define H 8
#define W 8
#define CIN 1
#define KH 3
#define KW 3
#define COUT 1
#define STRIDEH 1
#define STRIDEW 2
#define DILH 2
#define DILW 1

int main() {
    int effKh = DILH*(KH-1)+1, effKw = DILW*(KW-1)+1;
    int Hout = (H - effKh) / STRIDEH + 1;
    int Wout = (W - effKw) / STRIDEW + 1;

    float X[N*H*W*CIN], Kernel[KH*KW*CIN*COUT];
    unsigned seed = 11;
    for (int i = 0; i < N*H*W*CIN; i++) {
        seed = seed*1103515245+12345;
        X[i] = ((int)((seed>>16)%2000)-1000)/100.0f;
    }
    for (int i = 0; i < KH*KW*CIN*COUT; i++) {
        seed = seed*1103515245+12345;
        Kernel[i] = ((int)((seed>>16)%2000)-1000)/100.0f;
    }

    float *Y_hw = malloc((size_t)N*Hout*Wout*COUT*sizeof(float));
    double *Y_ref = malloc((size_t)N*Hout*Wout*COUT*sizeof(double));

    for (int n = 0; n < N; n++)
        for (int oy = 0; oy < Hout; oy++)
            for (int ox = 0; ox < Wout; ox++) {
                double acc = 0.0;
                for (int ky = 0; ky < KH; ky++)
                    for (int kx = 0; kx < KW; kx++)
                        for (int c = 0; c < CIN; c++) {
                            int iy = oy*STRIDEH + ky*DILH;
                            int ix = ox*STRIDEW + kx*DILW;
                            acc += (double)X[((size_t)n*H+iy)*W*CIN + ix*CIN+c]
                                 * (double)Kernel[(ky*KW+kx)*CIN+c];
                        }
                Y_ref[((size_t)n*Hout+oy)*Wout+ox] = acc;
            }

    printf("conv2d general: N=%d, %dx%dx%d input, %dx%dx%dx%d kernel, "
           "stride=(%d,%d), dilation=(%d,%d) -> %dx%dx%d output x%d batches\n",
           N, H, W, CIN, KH, KW, CIN, COUT, STRIDEH, STRIDEW, DILH, DILW,
           Hout, Wout, COUT, N);

    int rc = fpga_conv2d_im2col_general_auto(N, H, W, CIN, KH, KW, COUT,
                                              STRIDEH, STRIDEW, DILH, DILW,
                                              X, Kernel, Y_hw);
    if (rc != 0) { printf("FAILED, rc=%d\n", rc); return 1; }

    int total = N*Hout*Wout*COUT, errors = 0;
    for (int i = 0; i < total; i++) {
        float diff = fabsf(Y_hw[i] - (float)Y_ref[i]);
        const char *status = (diff < 1e-2f) ? "PASS" : "FAIL";
        if (diff >= 1e-2f) errors++;
        printf("[%s] idx=%2d  hw=%10.4f  ref=%10.4f  diff=%.6f\n",
               status, i, Y_hw[i], (float)Y_ref[i], diff);
    }
    printf("\n%s: %d/%d\n", errors==0 ? "ALL PASS" : "SOME FAILED", total-errors, total);
    free(Y_hw); free(Y_ref);
    return errors==0 ? 0 : 1;
}
