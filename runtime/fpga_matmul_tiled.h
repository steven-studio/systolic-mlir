#ifndef FPGA_MATMUL_TILED_H
#define FPGA_MATMUL_TILED_H

// C(MxN) = A(MxK) @ B(KxN), 全部 row-major float32
// 內部自動 zero-pad 到 4 的倍數,拆成 4x4x4 tile 逐塊送去 FPGA 累加
// 回傳 0 成功,非 0 表示 UART 逾時或協定錯誤
int fpga_matmul_tiled(int fd, int M, int K, int N,
                       const float *A, const float *B, float *C);


// Batched matmul: C[b] = A[b] @ B[b] + C_init[b] for b in [0, batch).
// A is (batch, M, K), B is (batch, K, N), C is (batch, M, N), all
// row-major float32. Each batch element is dispatched independently
// via fpga_matmul_tiled_auto -- no data dependency between batches,
// so this is simply a sequential loop (see Section: Limitations,
// "single accelerator, sequential dispatch" for why it is not
// currently parallelized).
int fpga_batch_matmul_tiled_auto(int batch, int M, int K, int N,
                                  const float *A, const float *B, float *C);


// Vector-matrix multiply: y[N] = x[K] @ A[K,N] (no accumulator input,
// matching linalg.vecmat's semantics -- torch-mlir emits this for
// nn.Linear applied to a rank-1, unbatched input). Implemented as a
// 1xK @ KxN matmul via fpga_matmul_tiled_auto, reusing all existing
// tiling and zero-padding logic with M=1.
int fpga_vecmat_tiled_auto(int K, int N, const float *x, const float *A,
                            float *y);


// Matrix-vector multiply: y[M] = A[M,K] @ x[K] (no accumulator input,
// matching linalg.matvec's semantics). Implemented as an MxK @ Kx1
// matmul via fpga_matmul_tiled_auto, reusing all existing tiling and
// zero-padding logic with N=1 -- the mirror image of
// fpga_vecmat_tiled_auto's M=1.
int fpga_matvec_tiled_auto(int M, int K, const float *A, const float *x,
                            float *y);

#endif

// 自動管理 UART 連線的版本:第一次呼叫時自動開啟 /dev/ttyUSB1,
// 之後重複使用同一個連線(不重複開關,避免每次呼叫都重新握手)
// 給 MLIR codegen 用,呼叫端不用管 fd 生命週期
int fpga_matmul_tiled_auto(int M, int K, int N,
                            const float *A, const float *B, float *C);
