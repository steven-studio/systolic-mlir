/*
 * tb_rx_contract -- top 層 RX 寫入路徑的 N 參數化契約測試(不需 FP IP)。
 *
 * 動機:N=4 上板 RX 0/128,而 full-system 模擬在 N=4 從未跑過。
 * 嫌疑最大的是 top 的 RX 位元切片一般化(a_lane/a_koff/b_koff/b_lane)。
 * 這一段在資料碰到 PE 之前就完成,所以用 stub IP 就能完整驗證:
 *
 *   UART 打一個完整 frame -> 檢查
 *     1. matrices_ready 有沒有升起(framing 收斂)
 *     2. k_dim 暫存器的值
 *     3. 兩顆 operand buffer 每個 bank 每個位址的內容 == 送進去的圖樣
 *     4. 主 FSM 有沒有走完 ST_FEED 進 ST_WAIT_RESULT(feed 邊界 k_dim+N-2)
 *
 * payload 用可辨識的 32-bit 圖樣(不是合法浮點也無妨 -- 這裡只驗
 * 儲存,不驗算術)。N=8 與 N=4 各一顆 DUT,同一套檢查:N=8 是
 * 已上板驗證組態的對照組,N=4 是嫌疑人。
 */
`timescale 1ns / 1ps

module rx_contract_one #(
    parameter int N     = 8,
    parameter int K_MAX = 16
)(
    input logic clk
);
    localparam int CLK_HZ       = 100_000_000;
    localparam int BAUD         = 10_000_000;
    localparam int CLKS_PER_BIT = CLK_HZ / BAUD;
    localparam time BIT_PERIOD  = 10ns * CLKS_PER_BIT;

    localparam logic [31:0] FRAME_START = 32'hA55A_C33C;
    localparam logic [31:0] FRAME_END   = 32'h5AA5_3CC3;

    logic rst;
    logic rx_line = 1'b1;
    logic tx_line;

    systolic_uart_tile_top #(
        .CLK_HZ (CLK_HZ),
        .BAUD   (BAUD),
        .K_MAX  (K_MAX),
        .N      (N),
        .DEBUG_MARKERS (1'b0),
        .CYCLE_COUNTER (1'b1)
    ) dut (
        .clk     (clk),
        .rst     (rst),
        .uart_rx (rx_line),
        .uart_tx (tx_line)
    );

    function automatic [31:0] pat(input int side, input int lane, input int k);
        pat = 32'hC0DE0000 ^ (side * 32'h100000) ^ (lane << 8) ^ k[7:0];
    endfunction

    task automatic send_byte(input logic [7:0] d);
        int i;
        begin
            rx_line = 1'b0;  #(BIT_PERIOD);
            for (i = 0; i < 8; i++) begin rx_line = d[i]; #(BIT_PERIOD); end
            rx_line = 1'b1;  #(BIT_PERIOD);
        end
    endtask

    task automatic send_word(input logic [31:0] w);
        begin
            send_byte(w[7:0]); send_byte(w[15:8]);
            send_byte(w[23:16]); send_byte(w[31:24]);
        end
    endtask

    logic saw_ready;
    always_ff @(posedge clk)
        if (rst)                       saw_ready <= 1'b0;
        else if (dut.matrices_ready)   saw_ready <= 1'b1;

    /* generate scope 的階層索引必須是常數(Verilator 限制),
     * 用 genvar 把每個 bank 的 mem 鏡射成平面陣列再檢查。 */
    logic [31:0] a_view [0:N-1][0:K_MAX-1];
    logic [31:0] b_view [0:N-1][0:K_MAX-1];
    generate
        for (genvar gl = 0; gl < N; gl++) begin : VIEW
            always_comb
                for (int kk = 0; kk < K_MAX; kk++) begin
                    a_view[gl][kk] = dut.u_a_buf.BANK[gl].mem[kk];
                    b_view[gl][kk] = dut.u_b_buf.BANK[gl].mem[kk];
                end
        end
    endgenerate

    int errors, checked;
    logic done = 1'b0;

    initial begin : RUN
        int w, r, c, k, lane;
        logic [31:0] got, exp;
        begin
            errors = 0; checked = 0;
            rst = 1'b1;  repeat (20) @(posedge clk);
            rst = 1'b0;  repeat (30) @(posedge clk);   // POR 16 拍

            /* wire format 與 test_uart_kmax.py 相同:
             * 每個 8-deep window:A[row][koff] row-major、B[koff][col] */
            send_word(FRAME_START);
            send_word(32'(K_MAX));                     // k_dim = K_MAX
            for (w = 0; w < K_MAX / 8; w++) begin
                for (r = 0; r < N; r++)                // A: lane 外圈
                    for (c = 0; c < 8; c++)
                        send_word(pat(0, r, w * 8 + c));
                for (r = 0; r < 8; r++)                // B: koff 外圈
                    for (c = 0; c < N; c++)
                        send_word(pat(1, c, w * 8 + r));
            end
            send_word(FRAME_END);

            repeat (50) @(posedge clk);

            // 1. framing
            if (!saw_ready) begin
                $display("  [N=%0d] FAIL: matrices_ready 從未升起 (rx_state=%0d rx_count=%0d hdr_done=%0b)",
                         N, dut.rx_state, dut.rx_count, dut.hdr_done);
                errors++;
            end
            checked++;

            // 2. k_dim
            if (int'(dut.k_dim) !== K_MAX) begin
                $display("  [N=%0d] FAIL: k_dim=%0d != %0d", N, dut.k_dim, K_MAX);
                errors++;
            end
            checked++;

            // 3. buffer 內容(這是位元切片的直接檢驗)
            for (lane = 0; lane < N; lane++)
                for (k = 0; k < K_MAX; k++) begin
                    got = a_view[lane][k];
                    exp = pat(0, lane, k);
                    if (got !== exp) begin
                        if (errors < 8)
                            $display("  [N=%0d] FAIL A bank%0d k%0d: %08x != %08x",
                                     N, lane, k, got, exp);
                        errors++;
                    end
                    checked++;
                    got = b_view[lane][k];
                    exp = pat(1, lane, k);
                    if (got !== exp) begin
                        if (errors < 8)
                            $display("  [N=%0d] FAIL B bank%0d k%0d: %08x != %08x",
                                     N, lane, k, got, exp);
                        errors++;
                    end
                    checked++;
                end

            // 4. feed 有沒有走完(stub IP 之下 PE 不會完成,
            //    但 ST_FEED -> ST_WAIT_RESULT 的邊界仍應發生)
            //    等待時間必須隨 K_MAX 縮放:feed 要 k_dim+N-2 拍。
            repeat (K_MAX + 300) @(posedge clk);
            if (int'(dut.state) !== 2) begin   // ST_WAIT_RESULT
                $display("  [N=%0d] FAIL: state=%0d,未達 ST_WAIT_RESULT(feed 邊界壞了?feed_t=%0d)",
                         N, dut.state, dut.feed_t);
                errors++;
            end
            checked++;

            done = 1'b1;
        end
    end
endmodule


module tb_rx_contract;
    logic clk = 1'b0;
    always #5 clk = ~clk;

    rx_contract_one #(.N(8), .K_MAX(16))  u8    (.clk(clk));
    rx_contract_one #(.N(4), .K_MAX(16))  u4    (.clk(clk));
    rx_contract_one #(.N(4), .K_MAX(32))  u4k32 (.clk(clk));
    rx_contract_one #(.N(4), .K_MAX(64))  u4k64 (.clk(clk));
    rx_contract_one #(.N(4), .K_MAX(256)) u4k256(.clk(clk));
    rx_contract_one #(.N(8), .K_MAX(32))  u8k32 (.clk(clk));

    initial begin
        wait (u8.done && u4.done && u4k32.done && u4k64.done &&
              u4k256.done && u8k32.done);
        @(posedge clk);

        $display("");
        $display("N=8 K=16  : errors=%0d checked=%0d", u8.errors, u8.checked);
        $display("N=4 K=16  : errors=%0d checked=%0d", u4.errors, u4.checked);
        $display("N=4 K=32  : errors=%0d checked=%0d", u4k32.errors, u4k32.checked);
        $display("N=4 K=64  : errors=%0d checked=%0d", u4k64.errors, u4k64.checked);
        $display("N=4 K=256 : errors=%0d checked=%0d", u4k256.errors, u4k256.checked);
        $display("N=8 K=32  : errors=%0d checked=%0d", u8k32.errors, u8k32.checked);
        if (u8.errors == 0 && u4.errors == 0 && u4k32.errors == 0 &&
            u4k64.errors == 0 && u4k256.errors == 0 && u8k32.errors == 0 &&
            u8.checked > 0 && u4.checked > 0)
            $display("PASS: RX write path + feed boundary correct at both N");
        else
            $display("FAIL");
        $finish;
    end

    initial begin
        #200_000_000;
        $display("TIMEOUT");
        $fatal(1, "watchdog");
    end
endmodule
