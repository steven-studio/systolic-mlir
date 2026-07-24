#include "fpga_matmul_tiled.h"
#include "fpga_matmul4x4.h"
#include <stdlib.h>
#include <string.h>

static int ceil_div4(int x) { return (x + 3) / 4; }

// 從 (MxK) row-major 矩陣中取出以 (row0, col0) 為左上角的 4x4 子區塊
// 超出原矩陣範圍的部分補 0
static void extract_tile(const float *M_mat, int rows, int cols,
                          int row0, int col0, float tile[16]) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            int r = row0 + i, c = col0 + j;
            tile[i*4+j] = (r < rows && c < cols) ? M_mat[r*cols + c] : 0.0f;
        }
    }
}

// 把 4x4 tile 寫回 C(MxN),只寫有效範圍(超出的部分丟棄)
static void writeback_tile(float *C, int M, int N,
                            int row0, int col0, const float tile[16]) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            int r = row0 + i, c = col0 + j;
            if (r < M && c < N) C[r*N + c] = tile[i*4+j];
        }
    }
}

int fpga_matmul_tiled(int fd, int M, int K, int N,
                       const float *A, const float *B, float *C) {
    int M_tiles = ceil_div4(M);
    int K_tiles = ceil_div4(K);
    int N_tiles = ceil_div4(N);

    float zero16[16];
    memset(zero16, 0, sizeof(zero16));

    for (int it = 0; it < M_tiles; it++) {
        for (int jt = 0; jt < N_tiles; jt++) {
            float acc[16];
            memcpy(acc, zero16, sizeof(acc));  // 這個輸出 tile 的累加起點

            for (int kt = 0; kt < K_tiles; kt++) {
                float a_tile[16], b_tile[16], out_tile[16];
                extract_tile(A, M, K, it*4, kt*4, a_tile);
                extract_tile(B, K, N, kt*4, jt*4, b_tile);

                // acc = a_tile @ b_tile + acc  (硬體協定原生支援累加)
                int rc = fpga_matmul4x4(fd, a_tile, b_tile, acc, out_tile);
                if (rc != 0) return rc;
                memcpy(acc, out_tile, sizeof(acc));
            }

            writeback_tile(C, M, N, it*4, jt*4, acc);
        }
    }
    return 0;
}

static int g_fpga_fd = -2;  // -2 = 尚未嘗試開啟

int fpga_get_uart_fd(void) {
    if (g_fpga_fd == -2) {
        g_fpga_fd = fpga_uart_open("/dev/ttyUSB1");
    }
    return g_fpga_fd;  // 可能是負值(開啟失敗),呼叫端(MLIR 產生的 tile
                        // loop 裡對 fpga_matmul4x4 的呼叫)目前沒有檢查
                        // 這個回傳值 -- 跟既有的「runtime 呼叫的錯誤碼
                        // 沒有回傳給 MLIR」這個已知限制一致,不是新引入
                        // 的問題。
}

int fpga_matmul_tiled_auto(int M, int K, int N,
                            const float *A, const float *B, float *C) {
    int fd = fpga_get_uart_fd();
    if (fd < 0) return -3;  // 開啟失敗
    return fpga_matmul_tiled(fd, M, K, N, A, B, C);
}
