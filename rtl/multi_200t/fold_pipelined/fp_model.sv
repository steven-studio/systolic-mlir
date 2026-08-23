/*
 * fp_mul / fp_add 的行為模型 —— 只給模擬用,不進合成。
 *
 * 為什麼需要它
 * ------------
 * 真的 fp_mul / fp_add 包的是 Xilinx IP,要跑 xsim 才有。單元測試
 * 想在 Verilator 裡三秒跑完,就需要一個行為一樣的替身。
 *
 * 它保證三件事,跟真的 IP 一樣:
 *   1. 保序:先進去的先出來
 *   2. 一進一出:每個 valid_in 恰好產生一個 valid_out
 *   3. 固定延遲 LAT 拍
 *
 * 為什麼算術是整數而不是浮點
 * --------------------------
 * 因為 PE 對算術是不可知的 —— 它只決定「哪個乘積加到哪一格、
 * 什麼時候發、什麼時候寫回」。加法器裡面是浮點還是整數,對這些
 * 控制決策沒有任何影響。
 *
 * 用整數換到兩件事:
 *   - 結果完全精確,測試可以要求逐位元相同,不必談 ulp
 *   - 不依賴 Verilator 的 shortreal 支援(它會把 shortreal 提升成
 *     double,使 $bitstoshortreal / $shortrealtobits 的位元語意
 *     失效,乘積會變成 0 —— 這個坑踩過一次了)
 *
 * 浮點的數值行為在別的地方驗:板上的 bit-exact 比對,以及既有的
 * conv2d ulp 掃描。這支測試負責的是控制路徑。
 *
 * LAT 是參數。用 LAT=3/5 跑一次、LAT=9/12 再跑一次,兩種都要通過。
 * 通過了就證明 PE 沒有把延遲寫死在邏輯裡。
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
            d_pipe[0] <= $signed(a) * $signed(b);
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
            d_pipe[0] <= $signed(a) + $signed(b);
            for (int i = 1; i < LAT; i++) begin
                v_pipe[i] <= v_pipe[i-1];
                d_pipe[i] <= d_pipe[i-1];
            end
        end
    end

    assign valid_out = v_pipe[LAT-1];
    assign result    = d_pipe[LAT-1];
endmodule
