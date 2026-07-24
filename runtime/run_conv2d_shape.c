// run_conv2d_shape.c
//
// Generic hardware test driver for fpga_conv2d_im2col_padded_auto,
// modeled directly on run_shape.c (the existing matmul driver): reads
// all conv2d parameters from argv, reads X and Kernel from binary files,
// calls the runtime function, writes Y to a binary file. One compiled
// binary drives every row of the sweep CSV -- no recompilation per shape.
//
// Usage (18 numeric/path args after the program name):
//   ./run_conv2d_shape N H W Cin Kh Kw Cout strideH strideW dilH dilW \
//       padTop padBottom padLeft padRight X.bin K.bin Y.bin

#include <stdio.h>
#include <stdlib.h>
#include "fpga_conv2d_im2col.h"

static float *read_floats(const char *path, size_t count) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); exit(1); }
    float *buf = malloc(sizeof(float) * count);
    size_t got = fread(buf, sizeof(float), count, f);
    if (got != count) {
        fprintf(stderr, "%s: expected %zu floats, got %zu\n", path, count, got);
        exit(1);
    }
    fclose(f);
    return buf;
}

static void write_floats(const char *path, const float *buf, size_t count) {
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(1); }
    fwrite(buf, sizeof(float), count, f);
    fclose(f);
}

int main(int argc, char **argv) {
    // argv[0] = program name, argv[1..15] = 15 numeric params,
    // argv[16..18] = X.bin, K.bin, Y.bin -> argc must be 19.
    if (argc != 19) {
        fprintf(stderr,
            "usage: %s N H W Cin Kh Kw Cout strideH strideW dilH dilW "
            "padTop padBottom padLeft padRight X.bin K.bin Y.bin\n"
            "(got %d args, expected 18)\n", argv[0], argc - 1);
        return 1;
    }

    int N        = atoi(argv[1]);
    int H        = atoi(argv[2]);
    int W        = atoi(argv[3]);
    int Cin      = atoi(argv[4]);
    int Kh       = atoi(argv[5]);
    int Kw       = atoi(argv[6]);
    int Cout     = atoi(argv[7]);
    int strideH  = atoi(argv[8]);
    int strideW  = atoi(argv[9]);
    int dilH     = atoi(argv[10]);
    int dilW     = atoi(argv[11]);
    int padTop   = atoi(argv[12]);
    int padBottom= atoi(argv[13]);
    int padLeft  = atoi(argv[14]);
    int padRight = atoi(argv[15]);
    const char *xPath = argv[16];
    const char *kPath = argv[17];
    const char *yPath = argv[18];

    int Hpadded = H + padTop + padBottom;
    int Wpadded = W + padLeft + padRight;
    int effKh = dilH * (Kh - 1) + 1;
    int effKw = dilW * (Kw - 1) + 1;
    int Hout = (Hpadded - effKh) / strideH + 1;
    int Wout = (Wpadded - effKw) / strideW + 1;
    if (Hout <= 0 || Wout <= 0) {
        fprintf(stderr, "invalid shape: Hout=%d Wout=%d\n", Hout, Wout);
        return 1;
    }

    size_t xCount = (size_t)N * H * W * Cin;
    size_t kCount = (size_t)Kh * Kw * Cin * Cout;
    size_t yCount = (size_t)N * Hout * Wout * Cout;

    float *X = read_floats(xPath, xCount);
    float *K = read_floats(kPath, kCount);
    float *Y = malloc(sizeof(float) * yCount);
    if (!Y) { fprintf(stderr, "alloc failed\n"); return 1; }

    int rc = fpga_conv2d_im2col_padded_auto(
        N, H, W, Cin, Kh, Kw, Cout, strideH, strideW, dilH, dilW,
        padTop, padBottom, padLeft, padRight, X, K, Y);
    if (rc != 0) {
        fprintf(stderr, "fpga_conv2d_im2col_padded_auto failed, rc=%d\n", rc);
        free(X); free(K); free(Y);
        return 1;
    }

    write_floats(yPath, Y, yCount);
    free(X); free(K); free(Y);
    return 0;
}
