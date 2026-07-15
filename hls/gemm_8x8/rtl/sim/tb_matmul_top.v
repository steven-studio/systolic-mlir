`timescale 1ns/1ps

module tb_matmul_top;
    localparam CLK_PERIOD = 10;      // 100MHz
    localparam BAUD_RATE  = 115200;
    localparam BIT_PERIOD = 1_000_000_000 / BAUD_RATE; // ns per bit (~8681 ns)

    reg clk = 0;
    reg rst_n = 0;
    reg uart_tx_from_tb = 1;  // 接到 DUT 的 uart_rx_pin
    wire uart_rx_to_tb;       // 接到 DUT 的 uart_tx_pin

    always #(CLK_PERIOD/2) clk = ~clk;

    matmul_top dut (
        .clk(clk),
        .rst_n(rst_n),
        .uart_rx_pin(uart_tx_from_tb),
        .uart_tx_pin(uart_rx_to_tb),
        .led_done()
    );

    // ---------- task: 送一個 byte 出去 (bit-bang UART TX) ----------
    task send_byte(input [7:0] b);
        integer i;
        begin
            uart_tx_from_tb = 0; // start bit
            #(BIT_PERIOD);
            for (i = 0; i < 8; i = i + 1) begin
                uart_tx_from_tb = b[i];
                #(BIT_PERIOD);
            end
            uart_tx_from_tb = 1; // stop bit
            #(BIT_PERIOD);
        end
    endtask

    // ---------- task: 收一個 byte (bit-bang UART RX) ----------
    task recv_byte(output [7:0] b);
        integer i;
        begin
            $display("[RECV] t=%0t waiting for negedge...", $time);
            @(negedge uart_rx_to_tb); // 等 start bit
            $display("[RECV] t=%0t negedge detected (start bit begins)", $time);
            #(BIT_PERIOD/2);
            $display("[RECV] t=%0t mid start-bit, pin=%b", $time, uart_rx_to_tb);
            #(BIT_PERIOD);
            for (i = 0; i < 8; i = i + 1) begin
                b[i] = uart_rx_to_tb;
                $display("[RECV] t=%0t sampled bit%0d = %b", $time, i, uart_rx_to_tb);
                #(BIT_PERIOD);
            end
        end
    endtask

    integer i;
    reg [7:0] rx_byte;
    reg [31:0] result_word;
    reg [7:0] first_byte;
    integer j;

    initial begin
        $dumpfile("tb_matmul_top.vcd");
        $dumpvars(0, tb_matmul_top);

        rst_n = 0;
        #(CLK_PERIOD*10);
        rst_n = 1;
        #(CLK_PERIOD*10);

        $display("[TB] 開始送 768 bytes (arg0=64 arg1=64 arg2_in=64 個 float)...");

        // arg0: 送 64 個 float,值分別是 1.0, 2.0, ... 用簡單整數代替方便看
        for (i = 0; i < 64; i = i + 1) begin
            send_byte(i[7:0]);       // byte0 (LSB)
            send_byte(8'h00);
            send_byte(8'h00);
            send_byte(8'h00);        // byte3 (MSB) -- little-endian 32-bit word
        end
        // arg1: 送 64 個 float,值用 100+i
        for (i = 0; i < 64; i = i + 1) begin
            send_byte((100+i));
            send_byte(8'h00);
            send_byte(8'h00);
            send_byte(8'h00);
        end
        // arg2_in: 送 64 個 float,值用 200+i (dummy IP 會對這個 +1)
        for (i = 0; i < 64; i = i + 1) begin
            send_byte(i[7:0]);  // 改用 0~63,避免超過 8-bit 範圍
            send_byte(8'h00);
            send_byte(8'h00);
            send_byte(8'h00);
        end

        $display("[TB] 傳送完畢,等待運算 + 接收 256 bytes 結果...");

        // 同步點:等待 DUT 真正開始傳送第一個 byte 的 start bit,
        // 避免 recv_byte 的 @(negedge) 錯過已經發生的邊緣
        wait (uart_rx_to_tb == 0);
        $display("[TB] t=%0t 偵測到第一個 start bit,開始接收", $time);
        #(BIT_PERIOD/2);
        #(BIT_PERIOD);
        for (j = 0; j < 8; j = j + 1) begin
            first_byte[j] = uart_rx_to_tb;
            #(BIT_PERIOD);
        end
        $display("[DEBUG] arg0_flat[31:0]   = %0d (expect 0)", dut.arg0_flat[31:0]);
        $display("[DEBUG] arg1_flat[31:0]   = %0d (expect 100)", dut.arg1_flat[31:0]);
        $display("[DEBUG] arg2_in_flat[31:0]  word0 = %0d (expect 200)", dut.arg2_in_flat[31:0]);
        $display("[DEBUG] arg2_in_flat[63:32] word1 = %0d (expect 201)", dut.arg2_in_flat[63:32]);
        $display("[DEBUG] arg2_in_flat[2015:1984] word62 = %0d (expect 262)", dut.arg2_in_flat[2015:1984]);
        $display("[DEBUG] arg2_in_flat[2047:2016] word63 = %0d (expect 263)", dut.arg2_in_flat[2047:2016]);
        $monitor("[MON] t=%0t state=%0d byte_cnt=%0d tx_data=0x%02h tx_start=%b tx_busy=%b tx_busy_seen=%b uart_tx_pin=%b",
                  $time, dut.state, dut.byte_cnt, dut.tx_data, dut.tx_start, dut.tx_busy, dut.tx_busy_seen, dut.uart_tx_pin);

        for (i = 0; i < 64; i = i + 1) begin
            if (i == 0) begin
                rx_byte = first_byte; // 第一個 byte 已在同步階段手動接收完成
            end else begin
                recv_byte(rx_byte);
            end
            if (i < 3) $display("[RAWBYTE] word%0d byte0 = 0x%02h", i, rx_byte);
            result_word[7:0] = rx_byte;
            recv_byte(rx_byte);
            if (i < 3) $display("[RAWBYTE] word%0d byte1 = 0x%02h", i, rx_byte);
            result_word[15:8] = rx_byte;
            recv_byte(rx_byte);
            if (i < 3) $display("[RAWBYTE] word%0d byte2 = 0x%02h", i, rx_byte);
            result_word[23:16] = rx_byte;
            recv_byte(rx_byte);
            if (i < 3) $display("[RAWBYTE] word%0d byte3 = 0x%02h", i, rx_byte);
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

    // 逾時保護,避免模擬卡死
    initial begin
        #100_000_000; // 100ms 模擬時間上限(115200 baud 下 768+256 bytes 實際需要約 89ms)
        $display("[TB] TIMEOUT -- 模擬超時,FSM 可能卡住");
        $finish;
    end
endmodule
