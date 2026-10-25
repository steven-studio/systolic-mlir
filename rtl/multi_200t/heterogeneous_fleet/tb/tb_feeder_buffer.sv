`timescale 1ns/1ps

/* feeder + operand buffer 的最小驗證。不含 UART、不含浮點 IP,
 * 所以 xvlog/xelab/xsim 直接跑,幾秒鐘一次迭代。
 *
 * 驗的是契約本身,不是舊程式碼:灌入已知資料後,第 r 列在
 * gk = feed_t - r 界內時,必須交出 mem[r][gk],且 valid 與資料同拍。
 *
 * 重構是否保行為由 tb_operand_buffer_equiv 負責 —— 契約測試答不了
 * 那個問題,因為契約寫錯時它會跟著一起通過。兩個都要跑。 */
module tb_feeder_buffer;

    localparam int K_MAX  = 32;
    localparam int K_W    = 5;
    localparam int FEED_W = 8;
    localparam int KDIM_W = 8;

    logic clk = 0;
    always #5 clk = ~clk;

    logic rst    = 1'b1;
    logic enable = 1'b0;
    logic [FEED_W-1:0] feed_t = '0;
    logic [KDIM_W-1:0] k_dim  = K_MAX;

    logic           wr    = 1'b0;
    logic [2:0]     wsel  = '0;
    logic [K_W-1:0] waddr = '0;
    logic [31:0]    wdata = '0;

    logic [K_W-1:0] a_raddr [0:7];
    logic [K_W-1:0] b_raddr [0:7];
    wire  [31:0]    a_rdata [0:7];
    wire  [31:0]    b_rdata [0:7];

    logic [31:0] a_in [0:7];
    logic [31:0] b_in [0:7];
    logic a_valid [0:7];
    logic b_valid [0:7];
    logic ctx_a   [0:7];
    logic ctx_b   [0:7];

    systolic_operand_buffer #(.K_MAX(K_MAX), .K_W(K_W)) u_buf_a (
        .clk(clk), .wr(wr), .wsel(wsel), .waddr(waddr), .wdata(wdata),
        .raddr(a_raddr), .rdata(a_rdata));

    systolic_operand_buffer #(.K_MAX(K_MAX), .K_W(K_W)) u_buf_b (
        .clk(clk), .wr(1'b0), .wsel('0), .waddr('0), .wdata('0),
        .raddr(b_raddr), .rdata(b_rdata));

    systolic_tile_feeder #(.K_W(K_W), .FEED_W(FEED_W), .KDIM_W(KDIM_W)) u_feeder (
        .clk(clk), .rst(rst), .enable(enable),
        .feed_t(feed_t), .k_dim(k_dim),
        .a_rdata(a_rdata), .b_rdata(b_rdata),
        .a_raddr(a_raddr), .b_raddr(b_raddr),
        .a_in(a_in), .b_in(b_in),
        .a_valid_in(a_valid), .b_valid_in(b_valid),
        .accum_ctx_in_a(ctx_a), .accum_ctx_in_b(ctx_b));

    logic [31:0] gold [0:7][0:K_MAX-1];

    function automatic logic [31:0] val(int r, int k);
        return 32'hA000_0000 | (r << 16) | k;
    endfunction

    int errors = 0;
    int checked = 0;

    logic [FEED_W-1:0] feed_t_d;
    always_ff @(posedge clk) feed_t_d <= feed_t;

    initial begin
        repeat (4) @(posedge clk);
        rst = 0;
        @(negedge clk);

        for (int r = 0; r < 8; r++)
          for (int k = 0; k < K_MAX; k++) begin
            wr = 1; wsel = r[2:0]; waddr = k[K_W-1:0]; wdata = val(r,k);
            gold[r][k] = val(r,k);
            @(negedge clk);
          end
        wr = 0;
        @(negedge clk);

        enable = 1;
        for (int t = 0; t < K_MAX + 12; t++) begin
            feed_t = t[FEED_W-1:0];
            @(posedge clk);
            #1;
            for (int r = 0; r < 8; r++) begin
                int gk;
                gk = int'(feed_t_d) - r;
                if (a_valid[r]) begin
                    checked++;
                    if (gk < 0 || gk >= K_MAX)
                        begin $display("t=%0d r=%0d valid 但 gk=%0d 超界", t, r, gk); errors++; end
                    else if (a_in[r] !== gold[r][gk])
                        begin $display("t=%0d r=%0d gk=%0d 期望 %08h 實得 %08h",
                                       t, r, gk, gold[r][gk], a_in[r]); errors++; end
                end
            end
            @(negedge clk);
        end
        enable = 0;

        /* 防 vacuous pass:valid 若從未出現,checked==0、errors==0
         * 也會印 PASS。零次檢查本身就是 FAIL。 */
        if (checked == 0) begin
            $display("FAIL 前置條件:checked == 0,從未觀察到任何 valid 資料");
            errors++;
        end

        $display("checked = %0d", checked);
        if (errors == 0) $display("PASS: feeder + buffer 契約正確");
        else             $display("FAIL: %0d 處錯誤", errors);
        $finish;
    end

endmodule