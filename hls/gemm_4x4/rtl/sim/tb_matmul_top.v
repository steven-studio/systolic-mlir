`timescale 1ns/1ps

module tb_matmul_top;
    localparam CLK_PERIOD = 10;
    localparam BAUD_RATE  = 115200;
    localparam BIT_PERIOD = 1_000_000_000 / BAUD_RATE;

    reg clk = 0;
    reg rst_n = 0;
    reg uart_tx_from_tb = 1;
    wire uart_rx_to_tb;

    always #(CLK_PERIOD/2) clk = ~clk;

    matmul_top dut (
        .clk(clk),
        .btn_rst(rst_n),
        .uart_rx_pin(uart_tx_from_tb),
        .uart_tx_pin(uart_rx_to_tb),
        .led_done()
    );

    task send_byte(input [7:0] b);
        integer i;
        begin
            uart_tx_from_tb = 0;
            #(BIT_PERIOD);
            for (i = 0; i < 8; i = i + 1) begin
                uart_tx_from_tb = b[i];
                #(BIT_PERIOD);
            end
            uart_tx_from_tb = 1;
            #(BIT_PERIOD);
        end
    endtask

    task recv_byte(output [7:0] b);
        integer i;
        begin
            @(negedge uart_rx_to_tb);
            #(BIT_PERIOD/2);
            #(BIT_PERIOD);
            for (i = 0; i < 8; i = i + 1) begin
                b[i] = uart_rx_to_tb;
                #(BIT_PERIOD);
            end
        end
    endtask

    integer i, j;
    reg [7:0] rx_byte, first_byte;
    reg [31:0] result_word;

    initial begin
        $dumpfile("tb_matmul_top.vcd");
        $dumpvars(0, tb_matmul_top);

        rst_n = 1;   // 注意:4x4 是主動高電位,rst_n=1 代表 reset 中
        #(CLK_PERIOD*10);
        rst_n = 0;   // 放開 reset
        #(CLK_PERIOD*10);

        $display("[TB] 開始送 192 bytes (arg0=16 arg1=16 arg2_in=16 個 float)...");

        for (i = 0; i < 16; i = i + 1) begin
            send_byte(i[7:0]);
            send_byte(8'h00);
            send_byte(8'h00);
            send_byte(8'h00);
        end
        for (i = 0; i < 16; i = i + 1) begin
            send_byte((100+i));
            send_byte(8'h00);
            send_byte(8'h00);
            send_byte(8'h00);
        end
        for (i = 0; i < 16; i = i + 1) begin
            send_byte(i[7:0]);
            send_byte(8'h00);
            send_byte(8'h00);
            send_byte(8'h00);
        end

        $display("[TB] 傳送完畢,等待運算 + 接收 64 bytes 結果...");

        wait (uart_rx_to_tb == 0);
        #(BIT_PERIOD/2);
        #(BIT_PERIOD);
        for (j = 0; j < 8; j = j + 1) begin
            first_byte[j] = uart_rx_to_tb;
            #(BIT_PERIOD);
        end

        for (i = 0; i < 16; i = i + 1) begin
            if (i == 0) begin
                rx_byte = first_byte;
            end else begin
                recv_byte(rx_byte);
            end
            result_word[7:0] = rx_byte;
            recv_byte(rx_byte);
            result_word[15:8] = rx_byte;
            recv_byte(rx_byte);
            result_word[23:16] = rx_byte;
            recv_byte(rx_byte);
            result_word[31:24] = rx_byte;

            if (result_word !== (i + 1)) begin
                $display("[FAIL] word %0d: got %0d, expected %0d", i, result_word, i+1);
            end else begin
                $display("[PASS] word %0d = %0d", i, result_word);
            end
        end

        $display("[TB] 測試完成");
        #(CLK_PERIOD*100);
        $finish;
    end

    initial begin
        #30_000_000; // 30ms 逾時(192+64=256 bytes ≈ 22ms @ 115200 baud)
        $display("[TB] TIMEOUT");
        $finish;
    end
endmodule
