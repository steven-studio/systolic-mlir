// 頂層 controller:UART <-> matmul_iface 橋接
module matmul_top (
    input  wire clk,        // 100MHz 板載晶振
    input  wire btn_rst,    // Arty A7 按鈕,主動高電位(按下=1)
    input  wire uart_rx_pin,
    output wire uart_tx_pin,
    output wire led_done    // 完成指示燈
);
    wire rst = btn_rst;     // 直接使用,按下按鈕 = reset

    // ---------------- UART ----------------
    wire [7:0] rx_data;
    wire       rx_valid;
    reg  [7:0] tx_data;
    reg        tx_start;
    wire       tx_busy;

    uart_rx u_rx (
        .clk(clk), .rst(rst),
        .rx(uart_rx_pin),
        .data(rx_data), .valid(rx_valid)
    );
    uart_tx u_tx (
        .clk(clk), .rst(rst),
        .data(tx_data), .start(tx_start),
        .tx(uart_tx_pin), .busy(tx_busy)
    );

    // ---------------- matmul_iface ----------------
    reg  [2047:0] arg0_flat, arg1_flat, arg2_in_flat;
    wire [2047:0] arg2_out_flat;
    wire [63:0]   arg2_vld_flat;
    reg           ap_start;
    wire          ap_done, ap_idle, ap_ready;

    matmul_iface u_iface (
        .ap_clk(clk), .ap_rst(rst),
        .ap_start(ap_start),
        .ap_done(ap_done), .ap_idle(ap_idle), .ap_ready(ap_ready),
        .arg0_flat(arg0_flat),
        .arg1_flat(arg1_flat),
        .arg2_in_flat(arg2_in_flat),
        .arg2_out_flat(arg2_out_flat),
        .arg2_vld_flat(arg2_vld_flat)
    );

    // ---------------- Controller FSM ----------------
    // 收 768 bytes (192 float) -> RUN -> 送 256 bytes (64 float)
    localparam RX_BYTES = 768;
    localparam TX_BYTES = 256;

    localparam S_RX   = 0, S_START = 1, S_WAIT = 2, S_TX = 3, S_TXWAIT = 4, S_DONE = 5;
    reg [2:0] state = S_RX;
    reg        tx_busy_seen = 0;

    reg [11:0] byte_cnt = 0;   // 0..767 for RX, 0..255 for TX
    reg [2:0]  shift_pos = 0;  // 目前收到第幾個 byte (0~3) 組成一個 32-bit word
    reg [31:0] word_buf = 0;

    reg done_led = 0;
    assign led_done = done_led;

    always @(posedge clk) begin
        if (rst) begin
            state <= S_RX;
            byte_cnt <= 0;
            shift_pos <= 0;
            ap_start <= 0;
            tx_start <= 0;
            done_led <= 0;
        end else begin
            tx_start <= 0; // 預設 pulse 一個 cycle
            case (state)
                S_RX: begin
                    if (rx_valid) begin
                        word_buf <= {rx_data, word_buf[31:8]}; // little-endian 組字
                        if (shift_pos == 3) begin
                            // 一個 float 收齊,寫進對應 flat vector 的位置
                            if (byte_cnt < 256) begin
                                arg0_flat[(byte_cnt/4)*32 +: 32] <= {rx_data, word_buf[31:8]};
                            end else if (byte_cnt < 512) begin
                                arg1_flat[((byte_cnt-256)/4)*32 +: 32] <= {rx_data, word_buf[31:8]};
                            end else begin
                                arg2_in_flat[((byte_cnt-512)/4)*32 +: 32] <= {rx_data, word_buf[31:8]};
                            end
                            shift_pos <= 0;
                        end else begin
                            shift_pos <= shift_pos + 1;
                        end
                        byte_cnt <= byte_cnt + 1;
                        if (byte_cnt == RX_BYTES-1) begin
                            state <= S_START;
                            byte_cnt <= 0;
                        end
                    end
                end
                S_START: begin
                    ap_start <= 1;
                    state <= S_WAIT;
                end
                S_WAIT: begin
                    ap_start <= 0;
                    if (ap_done) begin
                        state <= S_TX;
                        byte_cnt <= 0;
                    end
                end
                S_TX: begin
                    if (!tx_busy) begin
                        tx_data <= arg2_out_flat[(byte_cnt/4)*32 + (byte_cnt%4)*8 +: 8];
                        tx_start <= 1;
                        tx_busy_seen <= 0;
                        state <= S_TXWAIT;
                    end
                end
                S_TXWAIT: begin
                    if (tx_busy) tx_busy_seen <= 1;
                    if (tx_busy_seen && !tx_busy) begin
                        if (byte_cnt == TX_BYTES-1) begin
                            state <= S_DONE;
                        end else begin
                            byte_cnt <= byte_cnt + 1;
                            state <= S_TX;
                        end
                    end
                end
                S_DONE: begin
                    done_led <= 1;
                    // 停在這裡,等下次 rst 重新開始
                end
                default: state <= S_RX;
            endcase
        end
    end
endmodule
