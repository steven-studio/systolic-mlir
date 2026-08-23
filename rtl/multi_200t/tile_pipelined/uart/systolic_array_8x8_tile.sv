/*
 * systolic_array_8x8_tile -- 8x8 的 systolic_array_tile。
 *
 * 保留這個模組名是為了讓 systolic_uart_fold_top 不必更動。上板驗證過的
 * 那份頂層（k_dim=16/32/64 皆 bit-exact、週期 k_dim+118）因此逐字不變,
 * 參數化的正確性條件就只剩「N=8 的行為與改動前相同」這一件事,而那是
 * tb_array_fold_kmax 直接可以檢查的。
 */
module systolic_array_8x8_tile #(
    parameter int DATA_W = 32
) (
    input  logic clk,
    input  logic rst,

    input  logic [DATA_W-1:0] a_in [0:7],
    input  logic [DATA_W-1:0] b_in [0:7],

    input  logic a_valid_in [0:7],
    input  logic b_valid_in [0:7],

    input  logic accum_ctx_in_a [0:7],
    input  logic accum_ctx_in_b [0:7],

    output logic              c_valid_out,
    output logic              c_ctx_out,
    output logic [DATA_W-1:0] c_out [0:7][0:7]
);

    systolic_array_tile #(
        .N      (8),
        .DATA_W (DATA_W)
    ) u_arr (
        .clk           (clk),
        .rst           (rst),
        .a_in          (a_in),
        .b_in          (b_in),
        .a_valid_in    (a_valid_in),
        .b_valid_in    (b_valid_in),
        .accum_ctx_in_a (accum_ctx_in_a),
        .accum_ctx_in_b (accum_ctx_in_b),
        .c_valid_out   (c_valid_out),
        .c_ctx_out     (c_ctx_out),
        .c_out         (c_out)
    );

endmodule
