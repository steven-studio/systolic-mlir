#include "fpga_conv2d_im2col.h"
#include "fpga_matmul_tiled.h"
#include <stdlib.h>
#include <string.h>

int fpga_conv2d_im2col(int fd,
                        int H, int W, int Cin,
                        int Kh, int Kw, int Cout,
                        int stride,
                        const float *X, const float *Kernel, float *Y) {
    int Hout = (H - Kh) / stride + 1;
    int Wout = (W - Kw) / stride + 1;
    if (Hout <= 0 || Wout <= 0) return -1;

    int M = Hout * Wout;          // rows of X_col
    int Kdim = Kh * Kw * Cin;     // cols of X_col == rows of K_mat
    int N = Cout;                 // cols of K_mat

    float *Xcol = malloc((size_t)M * Kdim * sizeof(float));
    float *Kmat = malloc((size_t)Kdim * N * sizeof(float));
    float *Ycol = malloc((size_t)M * N * sizeof(float));
    if (!Xcol || !Kmat || !Ycol) {
        free(Xcol); free(Kmat); free(Ycol);
        return -2;
    }

    // --- im2col: unfold X into Xcol ---
    // Xcol[(oy*Wout+ox), (ky*Kw+kx)*Cin + c] = X[(oy*stride+ky)*W*Cin + (ox*stride+kx)*Cin + c]
    for (int oy = 0; oy < Hout; oy++) {
        for (int ox = 0; ox < Wout; ox++) {
            int row = oy * Wout + ox;
            for (int ky = 0; ky < Kh; ky++) {
                for (int kx = 0; kx < Kw; kx++) {
                    for (int c = 0; c < Cin; c++) {
                        int iy = oy * stride + ky;
                        int ix = ox * stride + kx;
                        int col = (ky * Kw + kx) * Cin + c;
                        Xcol[(size_t)row * Kdim + col] =
                            X[((size_t)iy * W + ix) * Cin + c];
                    }
                }
            }
        }
    }

    // --- reshape kernel: K(Kh,Kw,Cin,Cout) is already exactly
    //     the (Kdim x Cout) layout we need, just a flat copy ---
    memcpy(Kmat, Kernel, (size_t)Kdim * N * sizeof(float));

    // --- Ycol = Xcol @ Kmat, reusing the already-validated tiled runtime ---
    int rc = fpga_matmul_tiled(fd, M, Kdim, N, Xcol, Kmat, Ycol);

    // --- Ycol (M x N) is already Y in (Hout*Wout, Cout) layout == Y flattened ---
    memcpy(Y, Ycol, (size_t)M * N * sizeof(float));

    free(Xcol); free(Kmat); free(Ycol);
    return rc;
}

// Shares the same lazily-opened global fd as fpga_matmul_tiled_auto.
// Declared extern here rather than duplicated, since it is defined in
// fpga_matmul_tiled.c and that translation unit is always linked
// alongside this one.
extern int fpga_matmul_tiled_auto(int M, int K, int N,
                                   const float *A, const float *B, float *C);

// im2col + matmul, but routed through fpga_matmul_tiled_auto instead of
// fpga_matmul_tiled(fd, ...) directly, so MLIR-generated call sites don't
// need to manage a UART file descriptor themselves.
int fpga_conv2d_im2col_auto(int H, int W, int Cin,
                             int Kh, int Kw, int Cout,
                             int stride,
                             const float *X, const float *Kernel, float *Y) {
    int Hout = (H - Kh) / stride + 1;
    int Wout = (W - Kw) / stride + 1;
    if (Hout <= 0 || Wout <= 0) return -1;

    int M = Hout * Wout;
    int Kdim = Kh * Kw * Cin;
    int N = Cout;

    float *Xcol = malloc((size_t)M * Kdim * sizeof(float));
    float *Kmat = malloc((size_t)Kdim * N * sizeof(float));
    float *Ycol = malloc((size_t)M * N * sizeof(float));
    if (!Xcol || !Kmat || !Ycol) {
        free(Xcol); free(Kmat); free(Ycol);
        return -2;
    }

    for (int oy = 0; oy < Hout; oy++) {
        for (int ox = 0; ox < Wout; ox++) {
            int row = oy * Wout + ox;
            for (int ky = 0; ky < Kh; ky++) {
                for (int kx = 0; kx < Kw; kx++) {
                    for (int c = 0; c < Cin; c++) {
                        int iy = oy * stride + ky;
                        int ix = ox * stride + kx;
                        int col = (ky * Kw + kx) * Cin + c;
                        Xcol[(size_t)row * Kdim + col] =
                            X[((size_t)iy * W + ix) * Cin + c];
                    }
                }
            }
        }
    }
    memcpy(Kmat, Kernel, (size_t)Kdim * N * sizeof(float));

    int rc = fpga_matmul_tiled_auto(M, Kdim, N, Xcol, Kmat, Ycol);

    memcpy(Y, Ycol, (size_t)M * N * sizeof(float));
    free(Xcol); free(Kmat); free(Ycol);
    return rc;
}

int fpga_conv2d_im2col_general_auto(int N, int H, int W, int Cin,
                                     int Kh, int Kw, int Cout,
                                     int strideH, int strideW,
                                     int dilationH, int dilationW,
                                     const float *X, const float *Kernel,
                                     float *Y) {
    int effKh = dilationH * (Kh - 1) + 1;
    int effKw = dilationW * (Kw - 1) + 1;
    int Hout = (H - effKh) / strideH + 1;
    int Wout = (W - effKw) / strideW + 1;
    if (Hout <= 0 || Wout <= 0) return -1;

    int M = Hout * Wout;
    int Kdim = Kh * Kw * Cin;
    int Ncols = Cout;

    float *Xcol = malloc((size_t)M * Kdim * sizeof(float));
    float *Kmat = malloc((size_t)Kdim * Ncols * sizeof(float));
    float *Ycol = malloc((size_t)M * Ncols * sizeof(float));
    if (!Xcol || !Kmat || !Ycol) {
        free(Xcol); free(Kmat); free(Ycol);
        return -2;
    }
    memcpy(Kmat, Kernel, (size_t)Kdim * Ncols * sizeof(float));

    // Process each batch image independently: unfold, matmul, write
    // back into the corresponding batch slice of Y. This is a simple
    // sequential loop over N -- no data dependency between batch
    // elements, so this is also where future concurrent dispatch
    // (Section: Limitations, "single accelerator, sequential dispatch")
    // would slot in without changing this function's interface.
    for (int n = 0; n < N; n++) {
        const float *Xn = X + (size_t)n * H * W * Cin;
        float *Yn = Y + (size_t)n * Hout * Wout * Cout;

        for (int oy = 0; oy < Hout; oy++) {
            for (int ox = 0; ox < Wout; ox++) {
                int row = oy * Wout + ox;
                for (int ky = 0; ky < Kh; ky++) {
                    for (int kx = 0; kx < Kw; kx++) {
                        for (int c = 0; c < Cin; c++) {
                            int iy = oy * strideH + ky * dilationH;
                            int ix = ox * strideW + kx * dilationW;
                            int col = (ky * Kw + kx) * Cin + c;
                            Xcol[(size_t)row * Kdim + col] =
                                Xn[((size_t)iy * W + ix) * Cin + c];
                        }
                    }
                }
            }
        }

        int rc = fpga_matmul_tiled_auto(M, Kdim, Ncols, Xcol, Kmat, Ycol);
        if (rc != 0) {
            free(Xcol); free(Kmat); free(Ycol);
            return rc;
        }
        memcpy(Yn, Ycol, (size_t)M * Ncols * sizeof(float));
    }

    free(Xcol); free(Kmat); free(Ycol);
    return 0;
}
