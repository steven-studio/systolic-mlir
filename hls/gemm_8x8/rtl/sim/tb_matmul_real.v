`timescale 1ns/1ps

module tb_matmul_real;
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
        .rst_n(rst_n),
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

    task send_float(input real val);
        reg [31:0] bits;
        begin
            bits = $shortrealtobits(val);
            send_byte(bits[7:0]);
            send_byte(bits[15:8]);
            send_byte(bits[23:16]);
            send_byte(bits[31:24]);
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
    reg [31:0] result_bits;
    real result_val, expected_val;
    real b_matrix [0:63];

    initial begin
        $dumpfile("tb_matmul_real.vcd");
        $dumpvars(0, tb_matmul_real);

        // 準備 B 矩陣的測試值(隨便選幾個容易驗證的浮點數)
        for (i = 0; i < 64; i = i + 1) begin
            b_matrix[i] = i * 1.5 + 0.25;
        end

        rst_n = 0;
        #(CLK_PERIOD*10);
        rst_n = 1;
        #(CLK_PERIOD*10);

        $display("[TB] 開始送 arg0(單位矩陣 I)...");
        for (i = 0; i < 8; i = i + 1) begin
            for (j = 0; j < 8; j = j + 1) begin
                if (i == j) send_float(1.0);
                else        send_float(0.0);
            end
        end

        $display("[TB] 開始送 arg1(B 矩陣)...");
        for (i = 0; i < 64; i = i + 1) begin
            send_float(b_matrix[i]);
        end

        $display("[TB] 開始送 arg2_in(基底,全部 0)...");
        for (i = 0; i < 64; i = i + 1) begin
            send_float(0.0);
        end

        $display("[TB] 傳送完畢,等待運算 + 接收結果...");
        wait (uart_rx_to_tb == 0);
        #(BIT_PERIOD/2);
        #(BIT_PERIOD);
        for (j = 0; j < 8; j = j + 1) begin
            first_byte[j] = uart_rx_to_tb;
            #(BIT_PERIOD);
        end

        for (i = 0; i < 64; i = i + 1) begin
            if (i == 0) begin
                rx_byte = first_byte;
            end else begin
                recv_byte(rx_byte);
            end
            result_bits[7:0] = rx_byte;
            recv_byte(rx_byte);
            result_bits[15:8] = rx_byte;
            recv_byte(rx_byte);
            result_bits[23:16] = rx_byte;
            recv_byte(rx_byte);
            result_bits[31:24] = rx_byte;

            result_val = $bitstoshortreal(result_bits);
            expected_val = b_matrix[i];

            if (result_val > expected_val + 0.001 || result_val < expected_val - 0.001) begin
                $display("[FAIL] word %0d: got %f, expected %f", i, result_val, expected_val);
            end else begin
                $display("[PASS] word %0d = %f", i, result_val);
            end
        end

        $display("[TB] 測試完成");
        #(CLK_PERIOD*100);
        $finish;
    end

    initial begin
        #200_000_000; // 200ms 逾時(真正的 IP 運算會需要更多時間)
        $display("[TB] TIMEOUT");
        $finish;
    end
endmodule
