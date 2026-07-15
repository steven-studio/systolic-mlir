#include "fpga_conv2d_im2col.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define H 6
#define W 6
#define CIN 1
#define KH 3
#define KW 3
#define COUT 1
#define STRIDE 1

int main() {
    float X[H*W*CIN], Kernel[KH*KW*CIN*COUT];
    srand(7);
    for (int i = 0; i < H*W*CIN; i++)
        X[i] = ((int)(rand() % 2000) - 1000) / 100.0f;
    for (int i = 0; i < KH*KW*CIN*COUT; i++)
        Kernel[i] = ((int)(rand() % 2000) - 1000) / 100.0f;

    int Hout = (H - KH) / STRIDE + 1;
    int Wout = (W - KW) / STRIDE + 1;
    float Y_hw[64];
    double Y_ref[64];

    for (int oy = 0; oy < Hout; oy++)
        for (int ox = 0; ox < Wout; ox++) {
            double acc = 0.0;
            for (int ky = 0; ky < KH; ky++)
                for (int kx = 0; kx < KW; kx++)
                    for (int c = 0; c < CIN; c++) {
                        int iy = oy*STRIDE+ky, ix = ox*STRIDE+kx;
                        acc += (double)X[(iy*W+ix)*CIN+c] * (double)Kernel[(ky*KW+kx)*CIN+c];
                    }
            Y_ref[oy*Wout+ox] = acc;
        }

    printf("Testing fpga_conv2d_im2col_auto (no explicit fd management)...\n");
    int rc = fpga_conv2d_im2col_auto(H, W, CIN, KH, KW, COUT, STRIDE, X, Kernel, Y_hw);
    if (rc != 0) { printf("FAILED, rc=%d\n", rc); return 1; }

    int errors = 0;
    for (int i = 0; i < Hout*Wout; i++) {
        float diff = fabsf(Y_hw[i] - (float)Y_ref[i]);
        if (diff >= 1e-2f) errors++;
    }
    printf("%s: %d/%d\n", errors == 0 ? "ALL PASS" : "SOME FAILED", Hout*Wout - errors, Hout*Wout);
    return errors == 0 ? 0 : 1;
}
