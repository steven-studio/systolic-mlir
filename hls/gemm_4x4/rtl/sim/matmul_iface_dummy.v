// 模擬用的假 matmul_iface (4x4版),收到 ap_start 後固定 5 個 cycle 完成
// 輸出值 = 輸入 arg2_in 直接加 1(方便驗證資料有沒有正確傳遞)
module matmul_iface (
    input  wire        ap_clk,
    input  wire        ap_rst,
    input  wire        ap_start,
    output reg         ap_done,
    output wire        ap_idle,
    output wire        ap_ready,
    input  wire [511:0] arg0_flat,
    input  wire [511:0] arg1_flat,
    input  wire [511:0] arg2_in_flat,
    output reg  [511:0] arg2_out_flat,
    output wire [15:0]   arg2_vld_flat
);
    reg [3:0] cnt = 0;
    reg running = 0;
    assign ap_idle = !running;
    assign ap_ready = 1;
    assign arg2_vld_flat = 16'hFFFF;

    integer k;
    always @(posedge ap_clk) begin
        if (ap_rst) begin
            running <= 0;
            ap_done <= 0;
            cnt <= 0;
        end else if (ap_start && !running) begin
            running <= 1;
            cnt <= 0;
            ap_done <= 0;
        end else if (running) begin
            if (cnt == 4) begin
                for (k = 0; k < 16; k = k + 1) begin
                    arg2_out_flat[k*32 +: 32] <= arg2_in_flat[k*32 +: 32] + 32'd1;
                end
                ap_done <= 1;
                running <= 0;
            end else begin
                cnt <= cnt + 1;
            end
        end else begin
            ap_done <= 0;
        end
    end
endmodule
