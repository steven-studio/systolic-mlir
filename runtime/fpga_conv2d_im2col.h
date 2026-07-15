#ifndef FPGA_CONV2D_IM2COL_H
#define FPGA_CONV2D_IM2COL_H

// Y(Hout x Wout x Cout) = conv2d(X(H x W x Cin), K(Kh x Kw x Cin x Cout))
// valid padding only (no zero-padding around input), stride given explicitly.
// All tensors row-major float32, channel-last layout.
int fpga_conv2d_im2col(int fd,
                        int H, int W, int Cin,
                        int Kh, int Kw, int Cout,
                        int stride,
                        const float *X, const float *Kernel, float *Y);


// Auto-managed UART connection version, for MLIR codegen call sites.
// Reuses the same persistent connection as fpga_matmul_tiled_auto
// (declared in fpga_matmul_tiled.h) if it has already been opened.
int fpga_conv2d_im2col_auto(int H, int W, int Cin,
                             int Kh, int Kw, int Cout,
                             int stride,
                             const float *X, const float *Kernel, float *Y);

// General version: batch size N > 1, independent strideH/strideW,
// independent dilationH/dilationW. X is (N, H, W, Cin), Kernel is
// (Kh, Kw, Cin, Cout), Y is (N, Hout, Wout, Cout), all row-major
// channel-last float32. Hout/Wout account for dilation:
//   Hout = (H - (dilationH*(Kh-1)+1)) / strideH + 1
//   Wout = (W - (dilationW*(Kw-1)+1)) / strideW + 1
int fpga_conv2d_im2col_general_auto(int N, int H, int W, int Cin,
                                     int Kh, int Kw, int Cout,
                                     int strideH, int strideW,
                                     int dilationH, int dilationW,
                                     const float *X, const float *Kernel,
                                     float *Y);

#endif
