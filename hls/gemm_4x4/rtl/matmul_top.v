// 頂層 controller (4x4 版):UART <-> matmul_iface 橋接
module matmul_top (
    input  wire clk_pin100, // 板載 100MHz 晶振(原始輸入)
    input  wire btn_rst,    // Arty A7 按鈕,主動高電位(按下=1)
    input  wire uart_rx_pin,
    output wire uart_tx_pin,
    output wire led_done    // 完成指示燈
);
    // ---------------- 時脈產生:100MHz -> 20MHz ----------------
    wire clk;          // 20MHz,驅動全部內部邏輯
    wire mmcm_locked;

    clk_gen u_clk_gen (
        .clk_in100(clk_pin100),
        .rst_in(btn_rst),
        .clk_out20(clk),
        .locked(mmcm_locked)
    );

    // MMCM 還沒鎖定前,強制 reset 整個系統
    wire rst = btn_rst | ~mmcm_locked;

    // ---------------- UART (20MHz) ----------------
    wire [7:0] rx_data;
    wire       rx_valid;
    reg  [7:0] tx_data;
    reg        tx_start;
    wire       tx_busy;

    uart_rx #(.CLK_FREQ(20_000_000)) u_rx (
        .clk(clk), .rst(rst),
        .rx(uart_rx_pin),
        .data(rx_data), .valid(rx_valid)
    );
    uart_tx #(.CLK_FREQ(20_000_000)) u_tx (
        .clk(clk), .rst(rst),
        .data(tx_data), .start(tx_start),
        .tx(uart_tx_pin), .busy(tx_busy)
    );

    // ---------------- matmul_iface (4x4) ----------------
    reg  [511:0] arg0_flat, arg1_flat, arg2_in_flat;
    wire [511:0] arg2_out_flat;
    wire [15:0]  arg2_vld_flat;
    reg          ap_start;
    wire         ap_done, ap_idle, ap_ready;

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
    // 收 192 bytes (48 float) -> RUN -> 送 64 bytes (16 float)
    localparam RX_BYTES = 192;
    localparam TX_BYTES = 64;

    localparam S_RX   = 0, S_START = 1, S_WAIT = 2, S_TX = 3, S_TXWAIT = 4, S_DONE = 5;
    reg [2:0] state = S_RX;
    reg        tx_busy_seen = 0;

    reg [11:0] byte_cnt = 0;
    reg [2:0]  shift_pos = 0;
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
            tx_start <= 0;
            case (state)
                S_RX: begin
                    if (byte_cnt == 0 && shift_pos == 0) begin
                        done_led <= 0;  // 開始新一輪接收,熄燈
                    end
                    if (rx_valid) begin
                        word_buf <= {rx_data, word_buf[31:8]};
                        if (shift_pos == 3) begin
                            if (byte_cnt < 64) begin
                                arg0_flat[(byte_cnt/4)*32 +: 32] <= {rx_data, word_buf[31:8]};
                            end else if (byte_cnt < 128) begin
                                arg1_flat[((byte_cnt-64)/4)*32 +: 32] <= {rx_data, word_buf[31:8]};
                            end else begin
                                arg2_in_flat[((byte_cnt-128)/4)*32 +: 32] <= {rx_data, word_buf[31:8]};
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
                    // 自動回到接收狀態,準備下一輪矩陣運算
                    // 不需要外部 reset,可以連續處理多筆資料
                    state <= S_RX;
                    byte_cnt <= 0;
                    shift_pos <= 0;
                end
                default: state <= S_RX;
            endcase
        end
    end
endmodule
