/*
 * systolic_array_8x8 -- N 綁死成 8 的 systolic_array。
 *
 * 這個模組只做一件事:把 N 綁成 8,讓還沒參數化的上層可以直接接。
 * 它本身沒有任何邏輯。
 *
 * 舊版的 accum_ctx_in_a / accum_ctx_in_b / c_ctx_out 已經隨著 ctx
 * 一起消失 —— 那兩個 context 從來沒有在時間上重疊過。
 * 一次交易發佈一片矩陣,c_valid_out 是一拍脈衝。
 */
module systolic_array_8x8 #(
    parameter int DATA_W = 32
) (
    input  logic clk,
    input  logic rst,

    input  logic [DATA_W-1:0] a_in [0:7],
    input  logic [DATA_W-1:0] b_in [0:7],

    input  logic a_valid_in [0:7],
    input  logic b_valid_in [0:7],

    output logic              c_valid_out,
    output logic [DATA_W-1:0] c_out [0:7][0:7]
);

    systolic_array #(
        .N      (8),
        .DATA_W (DATA_W)
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