/*
 * fp_mul / fp_add 的行為模型 —— 只給模擬用,不進合成。
 *
 * 為什麼需要它:真的 fp_mul/fp_add 包的是 Xilinx IP,要跑 xsim
 * 才有。單元測試想在 Verilator 裡三秒跑完,就需要一個行為一樣
 * 的替身。
 *
 * 它保證三件事,跟真的 IP 一樣:
 *   1. 保序:先進去的先出來
 *   2. 一進一出:每個 valid_in 恰好產生一個 valid_out
 *   3. 固定延遲 LAT 拍
 *
 * 算術用 IEEE-754 單精度($bitstoshortreal / $shortrealtobits),
 * 與硬體同格式。
 *
 * Verilator 會警告 shortreal 被提升成 real(雙精度)。那不影響
 * 正確性:兩個 float32 相乘或相加,精確結果所需的位數都在 double
 * 的 53 位以內,所以「double 算完再捨入到 float32」與「直接用
 * float32 算」結果相同,沒有雙重捨入誤差。下面的 lint_off 只是
 * 讓警告不要蓋掉真正的問題。
 *
 * LAT 是參數,所以你可以用 LAT=3 跑快一點、LAT=12 跑真實值 ——
 * 兩種都要通過。通過了就證明你的 PE 沒有把延遲寫死在邏輯裡,
 * 那正是這次重寫要達成的性質之一。
 */

module fp_mul #(parameter int LAT = 9) (
    input  logic        clk,
    input  logic        rst,
    input  logic        valid_in,
    input  logic [31:0] a,
    input  logic [31:0] b,
    output logic        valid_out,
    output logic [31:0] result
);
    logic        v_pipe [0:LAT-1];
    logic [31:0] d_pipe [0:LAT-1];

    always_ff @(posedge clk) begin
        if (rst) begin
            for (int i = 0; i < LAT; i++) begin
                v_pipe[i] <= 1'b0;
                d_pipe[i] <= 32'd0;
            end
        end
        else begin
            v_pipe[0] <= valid_in;
            /* verilator lint_off SHORTREAL */
            /* verilator lint_off WIDTH */
            d_pipe[0] <= $shortrealtobits($bitstoshortreal(a) * $bitstoshortreal(b));
            /* verilator lint_on WIDTH */
            /* verilator lint_on SHORTREAL */
            for (int i = 1; i < LAT; i++) begin
                v_pipe[i] <= v_pipe[i-1];
                d_pipe[i] <= d_pipe[i-1];
            end
        end
    end

    assign valid_out = v_pipe[LAT-1];
    assign result    = d_pipe[LAT-1];
endmodule


module fp_add #(parameter int LAT = 12) (
    input  logic        clk,
    input  logic        rst,
    input  logic        valid_in,
    input  logic [31:0] a,
    input  logic [31:0] b,
    output logic        valid_out,
    output logic [31:0] result
);
    logic        v_pipe [0:LAT-1];
    logic [31:0] d_pipe [0:LAT-1];

    always_ff @(posedge clk) begin
        if (rst) begin
            for (int i = 0; i < LAT; i++) begin
                v_pipe[i] <= 1'b0;
                d_pipe[i] <= 32'd0;
            end
        end
        else begin
            v_pipe[0] <= valid_in;
            /* verilator lint_off SHORTREAL */
            /* verilator lint_off WIDTH */
            d_pipe[0] <= $shortrealtobits($bitstoshortreal(a) + $bitstoshortreal(b));
            /* verilator lint_on WIDTH */
            /* verilator lint_on SHORTREAL */
            for (int i = 1; i < LAT; i++) begin
                v_pipe[i] <= v_pipe[i-1];
                d_pipe[i] <= d_pipe[i-1];
            end
        end
    end

    assign valid_out = v_pipe[LAT-1];
    assign result    = d_pipe[LAT-1];
endmodule
