`timescale 1ns/1ps
// 雙裝置協定測試 v2:常駐 UART RX 監聽器(修 v1 首 byte 漏接)。
// T1: dev=0x00; T2: dev=0x01(期間 device0 不得動作)。
// C[i][j] = 2*(j+1) + 0.25*i,定點精確,=== 硬比。
module tb_matmul_top_dual;
    localparam CLK_PERIOD = 10;
    localparam BIT_PERIOD = 1_000_000_000 / 115200;

    reg clk = 0;  reg rst = 0;
    reg  uart_tx_from_tb = 1;
    wire uart_rx_to_tb;
    always #(CLK_PERIOD/2) clk = ~clk;

    matmul_top_dual dut (
        .clk_pin100(clk), .btn_rst(rst),
        .uart_rx_pin(uart_tx_from_tb), .uart_tx_pin(uart_rx_to_tb),
        .led_done()
    );

    // ---- 常駐 RX 監聽器:開機即聽,永不漏 byte ----
    reg [7:0] rx_buf [0:511];
    integer   rx_cnt = 0;
    reg [7:0] rxb;
    integer   kk;
    initial begin
        forever begin
            @(negedge uart_rx_to_tb);
            #(BIT_PERIOD/2); #(BIT_PERIOD);
            for (kk = 0; kk < 8; kk = kk + 1) begin
                rxb[kk] = uart_rx_to_tb; #(BIT_PERIOD);
            end
            rx_buf[rx_cnt] = rxb;
            rx_cnt = rx_cnt + 1;
        end
    end

    // ---- 測試向量(同 v1)----
    function [31:0] a_word;  input integer m; begin a_word = 32'h3F800000; end endfunction
    function [31:0] b_word;  input integer m; begin
        case (m % 4)
            0: b_word = 32'h3F000000;  1: b_word = 32'h3F800000;
            2: b_word = 32'h3FC00000;  3: b_word = 32'h40000000;
        endcase
    end endfunction
    function [31:0] c_word;  input integer m; begin
        case (m / 4)
            0: c_word = 32'h00000000;  1: c_word = 32'h3E800000;
            2: c_word = 32'h3F000000;  3: c_word = 32'h3F400000;
        endcase
    end endfunction
    function [31:0] exp_word; input integer m; begin
        case (m)
            0:  exp_word=32'h40000000;  1:  exp_word=32'h40800000;
            2:  exp_word=32'h40C00000;  3:  exp_word=32'h41000000;
            4:  exp_word=32'h40100000;  5:  exp_word=32'h40880000;
            6:  exp_word=32'h40C80000;  7:  exp_word=32'h41040000;
            8:  exp_word=32'h40200000;  9:  exp_word=32'h40900000;
            10: exp_word=32'h40D00000;  11: exp_word=32'h41080000;
            12: exp_word=32'h40300000;  13: exp_word=32'h40980000;
            14: exp_word=32'h40D80000;  15: exp_word=32'h410C0000;
        endcase
    end endfunction

    task send_byte(input [7:0] b);
        integer i; begin
            uart_tx_from_tb = 0; #(BIT_PERIOD);
            for (i = 0; i < 8; i = i + 1) begin uart_tx_from_tb = b[i]; #(BIT_PERIOD); end
            uart_tx_from_tb = 1; #(BIT_PERIOD);
        end
    endtask
    task send_word(input [31:0] w);
        begin send_byte(w[7:0]); send_byte(w[15:8]); send_byte(w[23:16]); send_byte(w[31:24]); end
    endtask

    // ---- device0 隔離監看 ----
    reg watch_dev0 = 0;
    reg dev0_violated = 0;
    always @(posedge clk)
        if (watch_dev0 && (dut.ap_start0 || dut.ap_done0)) dev0_violated <= 1;

    integer m, errors, base;
    reg [31:0] rw;

    task run_transaction(input [7:0] dev);
        begin
            $display("[TB] === transaction: device 0x%02x ===", dev);
            base = rx_cnt;
            send_byte(dev);
            for (m = 0; m < 16; m = m + 1) send_word(a_word(m));
            for (m = 0; m < 16; m = m + 1) send_word(b_word(m));
            for (m = 0; m < 16; m = m + 1) send_word(c_word(m));
            wait (rx_cnt == base + 64);          // 監聽器收滿 64 bytes
            for (m = 0; m < 16; m = m + 1) begin
                rw = {rx_buf[base+m*4+3], rx_buf[base+m*4+2],
                      rx_buf[base+m*4+1], rx_buf[base+m*4]};
                if (rw !== exp_word(m)) begin
                    $display("[FAIL] dev%0d word %0d: got %08x, expected %08x",
                             dev[0], m, rw, exp_word(m));
                    errors = errors + 1;
                end
            end
            $display("[TB] device 0x%02x done, %0d errors so far", dev, errors);
        end
    endtask

    initial begin
        errors = 0;
        rst = 1; #(CLK_PERIOD*20); rst = 0;
        #(CLK_PERIOD*200);

        run_transaction(8'h00);

        watch_dev0 = 1;
        run_transaction(8'h01);
        watch_dev0 = 0;
        if (dev0_violated) begin
            $display("[FAIL] device0 activated during device1 transaction");
            errors = errors + 1;
        end

        if (errors == 0) $display("[TB] ALL PASS");
        else             $display("[TB] %0d FAILURES", errors);
        $finish;
    end

    initial begin
        #120_000_000;
        $display("[TB] TIMEOUT"); $finish;
    end
endmodule
