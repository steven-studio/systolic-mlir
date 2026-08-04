#include "fpga_conv2d_im2col.h"
#include "fpga_matmul4x4.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define PORT "/dev/ttyUSB1"

// 6x6 單通道輸入, 3x3 單通道卷積核, stride=1, valid padding -> 4x4 輸出
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
    float Y_hw[64];  // Hout*Wout*COUT, generously sized
    double Y_ref[64];

    // software reference conv2d
    for (int oy = 0; oy < Hout; oy++)
        for (int ox = 0; ox < Wout; ox++)
            for (int oc = 0; oc < COUT; oc++) {
                double acc = 0.0;
                for (int ky = 0; ky < KH; ky++)
                    for (int kx = 0; kx < KW; kx++)
                        for (int c = 0; c < CIN; c++) {
                            int iy = oy*STRIDE+ky, ix = ox*STRIDE+kx;
                            acc += (double)X[(iy*W+ix)*CIN+c] *
                                   (double)Kernel[(ky*KW+kx)*CIN*COUT + c*COUT + oc];
                        }
                Y_ref[(oy*Wout+ox)*COUT+oc] = acc;
            }

    printf("conv2d: input %dx%dx%d, kernel %dx%dx%dx%d, stride=%d -> output %dx%dx%d\n",
           H, W, CIN, KH, KW, CIN, COUT, STRIDE, Hout, Wout, COUT);
    printf("(implemented via im2col -> matmul on real FPGA hardware)\n\n");

    int fd = fpga_uart_open(PORT);
    if (fd < 0) { printf("open UART failed\n"); return 1; }

    int rc = fpga_conv2d_im2col(fd, H, W, CIN, KH, KW, COUT, STRIDE, X, Kernel, Y_hw);
    fpga_uart_close(fd);
    if (rc != 0) { printf("fpga_conv2d_im2col failed, rc=%d\n", rc); return 1; }

    int errors = 0;
    for (int i = 0; i < Hout*Wout*COUT; i++) {
        float ref32 = (float)Y_ref[i];
        float diff = fabsf(Y_hw[i] - ref32);
        const char *status = (diff < 1e-2f) ? "PASS" : "FAIL";
        if (diff >= 1e-2f) errors++;
        printf("[%s] idx=%2d  hw=%10.4f  ref=%10.4f  diff=%.6f\n",
               status, i, Y_hw[i], ref32, diff);
    }
    printf("\n%s: %d/%d\n", errors == 0 ? "ALL PASS" : "SOME FAILED",
           Hout*Wout*COUT - errors, Hout*Wout*COUT);
    return errors == 0 ? 0 : 1;
}
