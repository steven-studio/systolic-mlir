#ifndef FPGA_MATMUL_TILED_RK_H
#define FPGA_MATMUL_TILED_RK_H

/* fpga_matmul_tiled_rk.h -- 任意形狀 GEMM,跑在 8x8 runtime-K 陣列上。
 *
 * bitstream:  hls/multi_200t/vivado/baseline_40mhz/
 *             matmul_8x8x8_rk_40mhz_K1to64.bit
 * wire layer: fpga_matmul_rk_new.h  ([dev][K][A][B][C_init] -> 256 B)
 *
 * 為什麼需要這個檔
 * fpga_matmul_rk_new.c 只做一筆交易:A[8][K] @ B[K][8],K <= SYS_K_MAX。
 * 它上面的東西 -- fpga_matmul_tiled.c,以及因此 fpga_conv2d_im2col.c --
 * 全部寫死 4x4。所以 conv2d 根本碰不到 8x8 陣列。這個檔就是缺的那圈迴圈。
 *
 * 為什麼不直接把 fpga_matmul_tiled.c 參數化
 * 那條路徑承載著 48/48 逐位元的 conv2d 掃描結果。為了省一個檔而原地改它,
 * 是拿證據去冒險。fpga_tile.h 對雙陣列路徑已經講過同樣的理由。
 *
 * 逐位元相符是設計出來的,不是碰巧
 * runtime/harness/run_conv2d_sweep.py 的正確性判準是 reference_conv2d_f32:
 * fp32、im2col、k 遞增且循序。硬體以 C_init 起始 fp32 累加器、k 遞增
 * (實測,不是假設 -- 見 systolic_dispatch_new.h)。這圈迴圈把 C_out 當成
 * 下一塊的 C_init,且 k 塊依遞增順序取,所以整體加總順序就是
 * k = 0,1,...,K-1,與 4x4 路徑相同。同順序、同 fp32 捨入、同位元。
 *
 * M 與 N 的畸零邊界補零是精確的(x + 0.0f == x),補出來的通道不會擾動
 * 有效通道。
 *
 * 換來什麼
 * conv_sweep_003 (M=36, K=75, N=8) 的交易數:
 *     4x4:  ceil(36/4) * ceil(8/4) * ceil(75/4)  = 9 * 2 * 19 = 342
 *     8x8:  ceil(36/8) * ceil(8/8) * ceil(75/64) = 5 * 1 *  2 =  10
 * 342 正是成本模型為該列記錄的 predicted_tiles。少 34 倍有意義,因為
 * rc=-2 是每次往返都在冒的風險,而 conv_sweep_003 就是
 * sweep_nexys_8x8.log 裡逾時的那一列。
 */

#ifdef __cplusplus
extern "C" {
#endif

/* C(MxN) = A(MxK) @ B(KxN),全部 row-major float32。
 *
 * k_max <= 0 表示 SYS_K_MAX。傳更小的值只用於研究切塊的影響 --
 * 結果逐位元相同(已在主機端驗證)。
 * dev 選陣列實例;除非特意測 instance 1,傳 0。
 *
 * 回傳 0,或 -1 寫入失敗、-2 讀取逾時、-3 參數錯誤、
 * -5 於 tile 邊界被中斷(fpga_stop_requested)。
 */
int fpga_matmul_tiled_rk(int fd, int dev, int k_max,
                         int M, int K, int N,
                         const float *A, const float *B, float *C);

/* 同上,走 process 預設 context,第一次呼叫時開啟。 */
int fpga_matmul_tiled_rk_auto(int M, int K, int N,
                              const float *A, const float *B, float *C);

/* 最近一次呼叫發出的交易數,成功與否皆計。
 *
 * 這就是用來填 measured_tiles 那一欄的東西 -- 每一份 conv2d 掃描 CSV
 * 都把它留白,成本模型的 predicted_tiles 從來沒有跟實跑的計數對照過。
 */
long fpga_rk_tiles_last(void);

#ifdef __cplusplus
}
#endif

#endif  /* FPGA_MATMUL_TILED_RK_H */
