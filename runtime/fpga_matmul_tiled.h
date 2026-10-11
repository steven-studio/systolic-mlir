#ifndef FPGA_MATMUL_TILED_H
#define FPGA_MATMUL_TILED_H

#ifdef __cplusplus
extern "C" {
#endif

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


// Dot product: result = x[K] . y[K] (scalar output). Implemented as a
// 1xK @ Kx1 matmul via fpga_matmul_tiled_auto with M=N=1, doubly
// degenerate relative to fpga_vecmat_tiled_auto (M=1) and
// fpga_matvec_tiled_auto (N=1).
int fpga_dot_tiled_auto(int K, const float *x, const float *y,
                         float *result);

// 自動管理 UART 連線的版本:第一次呼叫時自動開啟 /dev/ttyUSB1,
// 之後重複使用同一個連線(不重複開關,避免每次呼叫都重新握手)
// 給 MLIR codegen 用,呼叫端不用管 fd 生命週期
int fpga_matmul_tiled_auto(int M, int K, int N,
                            const float *A, const float *B, float *C);

// Scheduled FPGA entry point: one invocation of one physical array.
//
// --lower-systolic-matmul-tile-to-fpga emits one call per scheduled
// systolic.matmul_tile; device_id / start_cycle / est_cycles are that
// tile's device and schedule.
//
//   C[M][N] += A[M][K] x B[K][N]       all row-major fp32, contiguous
//
// on the 1x8x8 + 3x4x4 bitstream: device 0 is the 8x8 (M, N <= 8),
// devices 1..3 the 4x4s (M, N <= 4), and 1 <= K <= 16. The tile is
// zero-padded to the array and to depth 16, staged as one 1024-byte slab,
// submitted as one DPTI descriptor, and the array's result tile is read
// back and added into C -- so invocations chained through C accumulate.
//
// Returns 0, or:
//   -10  K outside 1..16          -11  device_id unknown / tile too large
//   -12  null pointer             -13  slab packing failed
//    -3  DPTI session could not be opened
//   other negative values         DPTI MEM_WRITE / submit / read failure
int fpga_matmul_tiled_auto_scheduled(
    int M, int K, int N,
    const float *A, const float *B, float *C,
    unsigned device_id,
    unsigned start_cycle,
    unsigned est_cycles);

// What the scheduled entry point has done since the last reset. The bytes
// are counted at the FTDI layer, so they are what crossed the wire.
typedef struct {
    unsigned long long invocations;                /* results received */
    unsigned long long invocations_per_device[4];  /* by physical device id */
    unsigned long long bytes_to_fpga;              /* every byte written */
    unsigned long long bytes_from_fpga;            /* every byte read */
    unsigned long long failed_calls;
    int                first_error;                /* 0 = none */
} fpga_runtime_stats_t;

void fpga_runtime_get_stats(fpga_runtime_stats_t *out);
void fpga_runtime_reset_stats(void);

// Close the DPTI session the scheduled entry point opened on first use.
void fpga_runtime_close(void);


#ifdef __cplusplus
}  // extern "C"
#endif

#endif
