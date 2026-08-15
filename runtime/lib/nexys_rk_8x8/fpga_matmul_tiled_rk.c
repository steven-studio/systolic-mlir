/* fpga_matmul_tiled_rk.c -- 這個檔為什麼存在、為什麼不改 4x4 路徑,
 * 見 fpga_matmul_tiled_rk.h。 */

#include <stdlib.h>
#include <string.h>

#include "fpga_matmul_tiled_rk.h"
#include "fpga_matmul_rk_new.h"
#include "fpga_ctx.h"
#include "fpga_signal.h"

static long g_tiles_last = 0;

long fpga_rk_tiles_last(void) { return g_tiles_last; }

static int ceil_div(int x, int d) { return (x + d - 1) / d; }

/* A 的列塊:A 的第 [r0, r0+SYS_R) 列、第 [k0, k0+kb) 行。
 * 超過 M 的列補零 -- 補零是精確的,不會擾動結果。 */
static void gather_a(const float *A, int M, int K,
                     int r0, int k0, int kb, float *tile)
{
    memset(tile, 0, (size_t)SYS_R * (size_t)kb * sizeof(float));
    for (int i = 0; i < SYS_R; i++) {
        const int gr = r0 + i;
        if (gr >= M) continue;
        memcpy(&tile[(size_t)i * (size_t)kb],
               &A[(size_t)gr * (size_t)K + (size_t)k0],
               (size_t)kb * sizeof(float));
    }
}

/* B 的行塊:第 [k0, k0+kb) 列、第 [c0, c0+SYS_C) 行。
 * 跨距不連續,所以不像 A 可以 memcpy。 */
static void gather_b(const float *B, int K, int N,
                     int k0, int c0, int kb, float *tile)
{
    (void)K;
    for (int k = 0; k < kb; k++) {
        for (int j = 0; j < SYS_C; j++) {
            const int gc = c0 + j;
            tile[(size_t)k * SYS_C + (size_t)j] =
                (gc < N) ? B[(size_t)(k0 + k) * (size_t)N + (size_t)gc] : 0.0f;
        }
    }
}

int fpga_matmul_tiled_rk(int fd, int dev, int k_max,
                         int M, int K, int N,
                         const float *A, const float *B, float *C)
{
    g_tiles_last = 0;

    if (!A || !B || !C || M < 1 || K < 1 || N < 1) return -3;
    if (fd < 0) return -3;
    if (k_max <= 0 || k_max > SYS_K_MAX) k_max = SYS_K_MAX;

    const int tilesM = ceil_div(M, SYS_R);
    const int tilesN = ceil_div(N, SYS_C);

    /* 最大塊是 SYS_K_MAX,所以有界且放得進堆疊:8*64 與 64*8 個 float,
     * 各 2 KB。 */
    float a_tile[SYS_R * SYS_K_MAX];
    float b_tile[SYS_K_MAX * SYS_C];
    float acc[SYS_R * SYS_C];
    float out[SYS_R * SYS_C];

    for (int ti = 0; ti < tilesM; ti++) {
        for (int tj = 0; tj < tilesN; tj++) {

            memset(acc, 0, sizeof(acc));

            /* k 遞增,一塊一塊來,running sum 由 C_init 帶著走。
             * 順序就是逐位元相符的來源 -- 不要重排這些塊,也不要並行。 */
            for (int k0 = 0; k0 < K; k0 += k_max) {

                if (fpga_stop_requested()) return -5;

                const int kb = (K - k0 < k_max) ? (K - k0) : k_max;

                gather_a(A, M, K, ti * SYS_R, k0, kb, a_tile);
                gather_b(B, K, N, k0, tj * SYS_C, kb, b_tile);

                g_tiles_last++;
                int rc = sys_matmul(fd, dev, kb, a_tile, b_tile, acc, out);
                if (rc != 0) return rc;

                memcpy(acc, out, sizeof(acc));
            }

            for (int i = 0; i < SYS_R; i++) {
                const int gr = ti * SYS_R + i;
                if (gr >= M) continue;
                for (int j = 0; j < SYS_C; j++) {
                    const int gc = tj * SYS_C + j;
                    if (gc < N)
                        C[(size_t)gr * (size_t)N + (size_t)gc] =
                            acc[i * SYS_C + j];
                }
            }
        }
    }
    return 0;
}

int fpga_matmul_tiled_rk_auto(int M, int K, int N,
                              const float *A, const float *B, float *C)
{
    fpga_ctx_t *ctx = fpga_ctx_default();
    if (!ctx) return -3;
    return fpga_matmul_tiled_rk(fpga_ctx_fd(ctx), 0, 0, M, K, N, A, B, C);
}
