/*
 * systolic_array_8x8_tile -- N 綁死成 8 的 systolic_array_tile。
 *
 * 這個模組只做一件事:把 N 綁成 8,讓還沒參數化的上層可以直接接。
 * 它本身沒有任何邏輯。
 *
 * ---------------------------------------------------------------
 * 這個檔案可能已經不需要存在了
 *
 * 它原本的理由寫在舊註解裡:「保留這個模組名是為了讓
 * systolic_uart_fold_top 不必更動」。那個理由現在失效了 ——
 * 上層正在為了移除 ctx 而重寫,本來就要動。
 *
 * 舊註解還寫著「週期 k_dim+118」。實測與雙錨定的模型是
 * 2(N-1)+105,N=8 時是 k_dim+119。那個 118 是更早期的數字,
 * 留著會誤導,所以不留。
 *
 * 刪掉之前先確認沒有人在用:
 *   grep -rn "systolic_array_8x8_tile" --include=*.sv --include=*.tcl .
 * 只剩這個檔案自己的話,就可以刪。
 * ---------------------------------------------------------------
 */
module systolic_array_8x8_tile #(
    parameter int DATA_W  = 32,

    /* 只是往下傳。這一層看不到 index —— 身分是陣列內部
     * 每個 PE 自己帶的,沒有拉到邊界上來。 */
    parameter int INDEX_W = 16
) (
    input  logic clk,
    input  logic rst,

    input  logic [DATA_W-1:0] a_in [0:7],
    input  logic [DATA_W-1:0] b_in [0:7],

    input  logic a_valid_in [0:7],
    input  logic b_valid_in [0:7],

    /* 一次交易發佈一片矩陣。c_valid_out 是一拍脈衝。
     * 舊版的 accum_ctx_in_a / accum_ctx_in_b / c_ctx_out 已經隨著
     * ctx 一起消失 —— 那兩個 context 從來沒有在時間上重疊過。 */
    output logic              c_valid_out,
    output logic [DATA_W-1:0] c_out [0:7][0:7]
);

    systolic_array_tile #(
        .N       (8),
        .DATA_W  (DATA_W),
        .INDEX_W (INDEX_W)
    ) u_arr (
        .clk         (clk),
        .rst         (rst),
        .a_in        (a_in),
        .b_in        (b_in),
        .a_valid_in  (a_valid_in),
        .b_valid_in  (b_valid_in),
        .c_valid_out (c_valid_out),
        .c_out       (c_out)
    );

endmodule