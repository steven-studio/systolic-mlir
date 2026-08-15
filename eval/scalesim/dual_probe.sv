// 只為了量資源。功能無意義:兩個陣列吃同樣的輸入,輸出 XOR 起來
// 避免被最佳化掉。不要拿去燒板子。
module dual_probe #(parameter int K_MAX = 64) (
    input  logic clk, rst,
    input  logic [31:0] a_in [0:7],
    input  logic [31:0] b_in [0:7],
    input  logic        a_valid_in [0:7],
    input  logic        b_valid_in [0:7],
    input  logic        fold_ctx_in_a [0:7],
    input  logic        fold_ctx_in_b [0:7],
    output logic [31:0] c_xor
);
    logic [31:0] c8 [0:7][0:7];  logic v8, x8;
    logic [31:0] c4 [0:3][0:3];  logic v4, x4;
    logic [31:0] a4 [0:3], b4 [0:3];
    logic a4v [0:3], b4v [0:3], a4c [0:3], b4c [0:3];
    always_comb for (int i = 0; i < 4; i++) begin
        a4[i]=a_in[i]; b4[i]=b_in[i];
        a4v[i]=a_valid_in[i]; b4v[i]=b_valid_in[i];
        a4c[i]=fold_ctx_in_a[i]; b4c[i]=fold_ctx_in_b[i];
    end
    systolic_array_8x8_fold u8 (.clk,.rst,.a_in,.b_in,.a_valid_in,.b_valid_in,
        .fold_ctx_in_a,.fold_ctx_in_b,.c_valid_out(v8),.c_ctx_out(x8),.c_out(c8));
    systolic_array_4x4_fold u4 (.clk,.rst,.a_in(a4),.b_in(b4),
        .a_valid_in(a4v),.b_valid_in(b4v),.fold_ctx_in_a(a4c),.fold_ctx_in_b(b4c),
        .c_valid_out(v4),.c_ctx_out(x4),.c_out(c4));
    always_ff @(posedge clk) c_xor <= c8[0][0] ^ c4[0][0] ^ {30'd0, v8, v4};
endmodule
