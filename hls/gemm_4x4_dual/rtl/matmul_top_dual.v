
// 頂層 controller (2x 4x4 版):UART <-> 兩份 matmul_iface

// 協定 = 單陣列版前面加 1 個裝置選擇 byte:

//   [1B dev (0x00/0x01)] [192B A,B,Cinit] -> [64B C]

// 緩衝共用:arg*_flat 同時餵兩顆陣列(輸入 fanout 免費),

// ap_start 只打給被選中那顆,done/C 回讀用 dev_sel mux。

// 第二顆陣列的成本 = 陣列本身(~5.7k LUT + 16 DSP),iface 不長大。

module matmul_top_dual (

    input  wire clk_pin100,

    input  wire btn_rst,

    input  wire uart_rx_pin,

    output wire uart_tx_pin,

    output wire led_done

);

    wire clk;

    wire mmcm_locked;

    clk_gen u_clk_gen (

        .clk_in100(clk_pin100), .rst_in(btn_rst),

        .clk_out20(clk), .locked(mmcm_locked)

    );

    wire rst = btn_rst | ~mmcm_locked;

    wire [7:0] rx_data;  wire rx_valid;

    reg  [7:0] tx_data;  reg  tx_start;  wire tx_busy;

    uart_rx #(.CLK_FREQ(20_000_000)) u_rx (

        .clk(clk), .rst(rst), .rx(uart_rx_pin),

        .data(rx_data), .valid(rx_valid)

    );

    uart_tx #(.CLK_FREQ(20_000_000)) u_tx (

        .clk(clk), .rst(rst), .data(tx_data), .start(tx_start),

        .tx(uart_tx_pin), .busy(tx_busy)

    );

    // ---------------- 兩份 matmul_iface,共用輸入緩衝 ----------------

    reg  [511:0] arg0_flat, arg1_flat, arg2_in_flat;

    reg          dev_sel;

    reg          ap_start0, ap_start1;

    wire         ap_done0, ap_idle0, ap_ready0;

    wire         ap_done1, ap_idle1, ap_ready1;

    wire [511:0] arg2_out_flat0, arg2_out_flat1;

    wire [15:0]  arg2_vld_flat0, arg2_vld_flat1;

    matmul_iface u_iface0 (

        .ap_clk(clk), .ap_rst(rst),

        .ap_start(ap_start0),

        .ap_done(ap_done0), .ap_idle(ap_idle0), .ap_ready(ap_ready0),

        .arg0_flat(arg0_flat), .arg1_flat(arg1_flat),

        .arg2_in_flat(arg2_in_flat),

        .arg2_out_flat(arg2_out_flat0), .arg2_vld_flat(arg2_vld_flat0)

    );

    matmul_iface u_iface1 (

        .ap_clk(clk), .ap_rst(rst),

        .ap_start(ap_start1),

        .ap_done(ap_done1), .ap_idle(ap_idle1), .ap_ready(ap_ready1),

        .arg0_flat(arg0_flat), .arg1_flat(arg1_flat),

        .arg2_in_flat(arg2_in_flat),

        .arg2_out_flat(arg2_out_flat1), .arg2_vld_flat(arg2_vld_flat1)

    );

    wire         ap_done_sel   = dev_sel ? ap_done1       : ap_done0;

    wire [511:0] arg2_out_sel  = dev_sel ? arg2_out_flat1 : arg2_out_flat0;

    // ---------------- Controller FSM ----------------

    localparam RX_BYTES = 192;

    localparam TX_BYTES = 64;

    localparam S_DEV = 6, S_RX = 0, S_START = 1, S_WAIT = 2,

               S_TX = 3, S_TXWAIT = 4, S_DONE = 5;

    reg [2:0]  state = S_DEV;

    reg        tx_busy_seen = 0;

    reg [11:0] byte_cnt = 0;

    reg [2:0]  shift_pos = 0;

    reg [31:0] word_buf = 0;

    reg done_led = 0;

    assign led_done = done_led;

    always @(posedge clk) begin

        if (rst) begin

            state <= S_DEV;  dev_sel <= 0;

            byte_cnt <= 0;   shift_pos <= 0;

            ap_start0 <= 0;  ap_start1 <= 0;

            tx_start <= 0;   done_led <= 0;

        end else begin

            tx_start <= 0;

            case (state)

                S_DEV: begin                       // 交易第 1 個 byte = 裝置選擇

                    if (rx_valid) begin

                        dev_sel   <= rx_data[0];

                        done_led  <= 0;

                        byte_cnt  <= 0;

                        shift_pos <= 0;

                        state     <= S_RX;

                    end

                end

                S_RX: begin

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

                    if (dev_sel) ap_start1 <= 1;

                    else         ap_start0 <= 1;

                    state <= S_WAIT;

                end

                S_WAIT: begin

                    ap_start0 <= 0;  ap_start1 <= 0;

                    if (ap_done_sel) begin

                        state <= S_TX;

                        byte_cnt <= 0;

                    end

                end

                S_TX: begin

                    if (!tx_busy) begin

                        tx_data <= arg2_out_sel[(byte_cnt/4)*32 + (byte_cnt%4)*8 +: 8];

                        tx_start <= 1;

                        tx_busy_seen <= 0;

                        state <= S_TXWAIT;

                    end

                end

                S_TXWAIT: begin

                    if (tx_busy) tx_busy_seen <= 1;

                    if (tx_busy_seen && !tx_busy) begin

                        if (byte_cnt == TX_BYTES-1) state <= S_DONE;

                        else begin byte_cnt <= byte_cnt + 1; state <= S_TX; end

                    end

                end

                S_DONE: begin

                    done_led <= 1;

                    state <= S_DEV;                // 下一輪從裝置 byte 開始

                end

                default: state <= S_DEV;

            endcase

        end

    end

endmodule

