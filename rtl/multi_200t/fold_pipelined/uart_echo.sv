`timescale 1ns / 1ps

/*
 * ============================================================
 * uart_echo -- 序列鏈路的最小診斷 bitstream
 * ============================================================
 *
 * 與 systolic_uart_tile_top 用完全相同的 uart_rx / uart_tx、相同的
 * 埠名(所以相同的腳位約束)、相同的 POR。沒有陣列、沒有 framing、
 * 沒有浮點 IP。行為只有兩件事:
 *
 *   1. 心跳:每 ~1.3 秒自動送一個 0x55('U',bit pattern 01010101,
 *      示波器上最好認)。**不需要收到任何東西就會送。**
 *   2. 回音:每收到一個 byte,回傳該 byte XOR 0x20(大小寫翻轉,
 *      證明資料真的進過 FPGA,不是線路迴授)。
 *
 * 判讀矩陣:
 *
 *   心跳有、回音有  -> 序列鏈路完全正常,問題在 tile 設計內部
 *                      -> 燒 DEBUG_MARKERS=1 看 breadcrumb
 *   心跳有、回音無  -> FPGA->PC 方向好,PC->FPGA 方向斷
 *                      (uart_rx 腳位、cable、port 的 TX 側)
 *   心跳無          -> FPGA->PC 方向斷(uart_tx 腳位、port 選錯、
 *                      clk/rst 根本沒起來)
 *
 * 讀法:
 *   picocom -b 115200 /dev/ttyUSB2        # 或 minicom / screen
 *   看 'U' 每秒出現;打字看回音(a -> A)
 * ============================================================
 */
module uart_echo #(
    parameter int CLK_HZ = 100_000_000,
    parameter int BAUD   = 115200
) (
    input  logic clk,
    input  logic rst,

    input  logic uart_rx,
    output logic uart_tx
);

    /* 與 tile top 相同的組態後自動 reset。 */
    logic [15:0] por_sr = '0;
    wire         por_rst = ~por_sr[15];
    always_ff @(posedge clk)
        por_sr <= {por_sr[14:0], 1'b1};

    wire rst_i = rst | por_rst;

    logic [7:0] rx_byte;
    logic       rx_valid;

    logic [7:0] tx_byte;
    logic       tx_start;
    logic       tx_busy;

    uart_rx #(.CLK_HZ(CLK_HZ), .BAUD(BAUD)) u_rx (
        .clk(clk), .rst(rst_i), .rx(uart_rx),
        .data_out(rx_byte), .valid(rx_valid));

    uart_tx #(.CLK_HZ(CLK_HZ), .BAUD(BAUD)) u_tx (
        .clk(clk), .rst(rst_i), .start(tx_start),
        .data_in(tx_byte), .tx(uart_tx), .busy(tx_busy));

    /* 心跳週期:2^27 個 100 MHz 時脈 ~= 1.34 s。 */
    logic [26:0] beat_cnt;
    wire         beat = (beat_cnt == '0);

    /* 收到的 byte 暫存一拍等 TX 空閒。深度 1 就夠 -- 人手打字與
     * 心跳都遠慢於一個 byte 的傳輸時間。 */
    logic       pend_v;
    logic [7:0] pend_b;

    always_ff @(posedge clk) begin
        if (rst_i) begin
            beat_cnt <= 27'd1;
            pend_v   <= 1'b0;
            pend_b   <= 8'h00;
            tx_start <= 1'b0;
            tx_byte  <= 8'h00;
        end
        else begin
            beat_cnt <= beat_cnt + 1'b1;
            tx_start <= 1'b0;

            if (rx_valid) begin
                pend_v <= 1'b1;
                pend_b <= rx_byte ^ 8'h20;   // a->A:證明經過邏輯
            end

            if (!tx_busy && !tx_start) begin
                if (pend_v) begin
                    tx_byte  <= pend_b;
                    tx_start <= 1'b1;
                    pend_v   <= 1'b0;
                end
                else if (beat) begin
                    tx_byte  <= 8'h55;       // 'U'
                    tx_start <= 1'b1;
                end
            end
        end
    end

endmodule
