#include "fpga_conv2d_im2col.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define N 1
#define CIN 1
#define H 6
#define W 6
#define COUT 1
#define KH 3
#define KW 3
#define STRIDEH 1
#define STRIDEW 1
#define DILH 1
#define DILW 1

int main() {
    int Hout = (H - KH) / STRIDEH + 1;
    int Wout = (W - KW) / STRIDEW + 1;

    float X[N*CIN*H*W], Kernel[COUT*CIN*KH*KW];
    unsigned seed = 99;
    for (int i = 0; i < N*CIN*H*W; i++) {
        seed = seed*1103515245+12345;
        X[i] = ((int)((seed>>16)%2000)-1000)/100.0f;
    }
    for (int i = 0; i < COUT*CIN*KH*KW; i++) {
        seed = seed*1103515245+12345;
        Kernel[i] = ((int)((seed>>16)%2000)-1000)/100.0f;
    }

    // software reference, matching PyTorch's NCHW/FCHW semantics directly
    double Y_ref[COUT*Hout*Wout];
    for (int oc = 0; oc < COUT; oc++)
        for (int oy = 0; oy < Hout; oy++)
            for (int ox = 0; ox < Wout; ox++) {
                double acc = 0.0;
                for (int c = 0; c < CIN; c++)
                    for (int ky = 0; ky < KH; ky++)
                        for (int kx = 0; kx < KW; kx++) {
                            int iy = oy*STRIDEH + ky*DILH;
                            int ix = ox*STRIDEW + kx*DILW;
                            double xval = X[(c*H+iy)*W+ix];
                            double kval = Kernel[((oc*CIN+c)*KH+ky)*KW+kx];
                            acc += xval * kval;
                        }
                Y_ref[(oc*Hout+oy)*Wout+ox] = acc;
            }

    float Y_hw[COUT*Hout*Wout];
    printf("conv2d NCHW/FCHW: %dx%dx%dx%d input, %dx%dx%dx%d kernel -> %dx%dx%dx%d output\n",
           N, CIN, H, W, COUT, CIN, KH, KW, N, COUT, Hout, Wout);

    int rc = fpga_conv2d_im2col_nchw_general_auto(N, CIN, H, W, COUT, KH, KW,
                                                   STRIDEH, STRIDEW, DILH, DILW,
                                                   X, Kernel, Y_hw);
    if (rc != 0) { printf("FAILED, rc=%d\n", rc); return 1; }

    int total = COUT*Hout*Wout, errors = 0;
    for (int i = 0; i < total; i++) {
        float diff = fabsf(Y_hw[i] - (float)Y_ref[i]);
        const char *status = (diff < 1e-2f) ? "PASS" : "FAIL";
        if (diff >= 1e-2f) errors++;
        printf("[%s] idx=%2d  hw=%10.4f  ref=%10.4f  diff=%.6f\n",
               status, i, Y_hw[i], (float)Y_ref[i], diff);
    }
    printf("\n%s: %d/%d\n", errors==0 ? "ALL PASS" : "SOME FAILED", total-errors, total);
    return errors==0 ? 0 : 1;
}
