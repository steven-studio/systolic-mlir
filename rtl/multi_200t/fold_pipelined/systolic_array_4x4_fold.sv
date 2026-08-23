/*
 * systolic_array_4x4_fold -- 4x4 的 systolic_array_fold。
 *
 * 先前這裡是一份獨立手寫的 4x4 陣列。它在 PE 從 dbg_acc_ctx0/1 改成
 * result_ctx0/1 之後就編不過了 -- 兩份必須一起改的檔案,終究會有一份
 * 沒被改到。現在兩者都是同一份參數化原始碼的實例。
 *
 * 注意:這個介面與舊版不同。舊版輸出的是 dbg_acc_ctx0/dbg_acc_ctx1
 * （每個 PE 的 16 個 bank 全部拉出來）,現在與 8x8 一致,輸出的是歸約
 * 完成的 c_out 加上 c_valid_out / c_ctx_out。
 */
module systolic_array_4x4_fold #(
    parameter int DATA_W = 32
) (
    input  logic clk,
    input  logic rst,

    input  logic [DATA_W-1:0] a_in [0:3],
    input  logic [DATA_W-1:0] b_in [0:3],

    input  logic a_valid_in [0:3],
    input  logic b_valid_in [0:3],

    input  logic fold_ctx_in_a [0:3],
    input  logic fold_ctx_in_b [0:3],

    output logic              c_valid_out,
    output logic              c_ctx_out,
    output logic [DATA_W-1:0] c_out [0:3][0:3]
);

    systolic_array_fold #(
        .N      (4),
        .DATA_W (DATA_W)
    ) u_arr (
        .clk           (clk),
        .rst           (rst),
        .a_in          (a_in),
        .b_in          (b_in),
        .a_valid_in    (a_valid_in),
        .b_valid_in    (b_valid_in),
        .fold_ctx_in_a (fold_ctx_in_a),
        .fold_ctx_in_b (fold_ctx_in_b),
        .c_valid_out   (c_valid_out),
        .c_ctx_out     (c_ctx_out),
        .c_out         (c_out)
    );

endmodule
