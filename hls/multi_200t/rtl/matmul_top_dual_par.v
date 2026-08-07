// matmul_top_dual_par.v -- 頂層 controller,兩顆 8x8 陣列可真正同時運算。
//
// 為什麼有這個檔案
// 原本的 matmul_top_dual.v 把 arg0/arg1/arg2_in 三組緩衝同時餵給兩顆陣列,
// 但 ap_start 只打給 dev_sel 選中的那一顆。128 顆 PE 裡永遠只有 64 顆在動,
// DSP 用掉 640/740 (86%) 卻只換到單陣列的吞吐量。硬體是真的 —— 綜合報告
// 數得出 128 個 fadd 和 128 個 fmul —— 只是有一半從來沒被啟動過。
//
// 這個版本給每顆陣列各自的 A/B/C_init,同時拉起兩個 ap_start,等兩顆都
// done 之後回傳兩份 C。
//
// 協定 (byte 0 = 模式)
//   0x00  單陣列,dev0   [768B A,B,Cinit] -> [256B C]
//   0x01  單陣列,dev1   [768B A,B,Cinit] -> [256B C]
//   0x02  雙陣列並行     [768B dev0][768B dev1] -> [256B C0][256B C1]
//
// 0x00/0x01 的行為與舊版逐位元組相同,所以現有的 test_dual_8x8.py 不必改
// 就能繼續當回歸測試 —— 新路徑壞掉時,舊路徑仍然是可信的對照組。
//
// 端到端不會變快,這點要先講明
// UART 在 115200 baud 下約 11.5 KB/s。算兩塊 tile,循序要送 2x769 byte,
// 並行要送 1x1537 byte —— 傳輸量幾乎一樣,省下的只有 28 個週期的計算,
// 而計算佔整體時間不到 0.001%。這個改動買到的是「128 顆 PE 同時在動」
// 和 2x 的峰值 GFLOP/s,不是牆鐘時間。要動牆鐘時間得換掉 UART。
//
// 資源代價
// 緩衝從 3x2048 bit 變成 6x2048 bit,多 6144 個 FF。dual 目前用 60,512 個
// slice register (22.6%),加上去約 24.9%,還很寬鬆。DSP 完全不變。

module matmul_top_dual_par (
    input  wire clk_pin100,
    input  wire btn_rst,
    input  wire uart_rx_pin,
    output wire uart_tx_pin,
    output wire led_done
);

    wire clk;
    wire mmcm_locked;

    // 埠名還叫 clk_out20 是歷史遺留;CLKOUT0_DIVIDE_F 已經是 25.0,
    // 實際輸出 40 MHz。改埠名要動 clk_gen.v 和所有引用處,不值得為了
    // 名稱冒險,留註解說明即可。
    clk_gen u_clk_gen (
        .clk_in100(clk_pin100), .rst_in(btn_rst),
        .clk_out20(clk), .locked(mmcm_locked)
    );

    wire rst = btn_rst | ~mmcm_locked;

    wire [7:0] rx_data;  wire rx_valid;
    reg  [7:0] tx_data;  reg  tx_start;  wire tx_busy;

    uart_rx #(.CLK_FREQ(40_000_000)) u_rx (
        .clk(clk), .rst(rst), .rx(uart_rx_pin),
        .data(rx_data), .valid(rx_valid)
    );
    uart_tx #(.CLK_FREQ(40_000_000)) u_tx (
        .clk(clk), .rst(rst), .data(tx_data), .start(tx_start),
        .tx(uart_tx_pin), .busy(tx_busy)
    );

    // ---------------- 每顆陣列各自的緩衝 ----------------
    // 舊版共用一組;要並行就必須拆開,否則兩顆算的是同一件事。
    reg [2047:0] a0_flat, b0_flat, c0_in_flat;
    reg [2047:0] a1_flat, b1_flat, c1_in_flat;

    reg          ap_start0, ap_start1;
    wire         ap_done0, ap_idle0, ap_ready0;
    wire         ap_done1, ap_idle1, ap_ready1;
    wire [2047:0] arg2_out_flat0, arg2_out_flat1;
    wire [63:0]  arg2_vld_flat0, arg2_vld_flat1;

    matmul_iface u_iface0 (
        .ap_clk(clk), .ap_rst(rst),
        .ap_start(ap_start0),
        .ap_done(ap_done0), .ap_idle(ap_idle0), .ap_ready(ap_ready0),
        .arg0_flat(a0_flat), .arg1_flat(b0_flat),
        .arg2_in_flat(c0_in_flat),
        .arg2_out_flat(arg2_out_flat0), .arg2_vld_flat(arg2_vld_flat0)
    );

    matmul_iface u_iface1 (
        .ap_clk(clk), .ap_rst(rst),
        .ap_start(ap_start1),
        .ap_done(ap_done1), .ap_idle(ap_idle1), .ap_ready(ap_ready1),
        .arg0_flat(a1_flat), .arg1_flat(b1_flat),
        .arg2_in_flat(c1_in_flat),
        .arg2_out_flat(arg2_out_flat1), .arg2_vld_flat(arg2_vld_flat1)
    );

    // ---------------- Controller FSM ----------------
    localparam MODE_DEV0 = 2'd0, MODE_DEV1 = 2'd1, MODE_DUAL = 2'd2;

    localparam TILE_BYTES = 768;   // 一份 A + B + C_init
    localparam OUT_BYTES  = 256;   // 一份 C

    localparam S_MODE = 6, S_RX = 0, S_START = 1, S_WAIT = 2,
               S_TX = 3, S_TXWAIT = 4, S_DONE = 5;

    reg [2:0]  state = S_MODE;
    reg [1:0]  mode  = MODE_DEV0;
    reg        tx_busy_seen = 0;
    reg [11:0] byte_cnt = 0;
    reg [2:0]  shift_pos = 0;
    reg [31:0] word_buf = 0;
    reg        done_led = 0;

    // 兩顆不一定在同一拍 done —— 同時啟動、延遲相同,理論上會,但
    // 依賴那個假設沒有好處。各自 latch,兩個都到齊才往下走。
    reg done0_seen, done1_seen;

    assign led_done = done_led;

    // 這一輪要收多少、要送多少,由模式決定
    wire [11:0] rx_total = (mode == MODE_DUAL) ? (TILE_BYTES*2) : TILE_BYTES;
    wire [11:0] tx_total = (mode == MODE_DUAL) ? (OUT_BYTES*2)  : OUT_BYTES;

    // 收進來的位元組屬於哪一顆、在該顆的哪個位移
    wire        rx_second = (mode == MODE_DUAL) && (byte_cnt >= TILE_BYTES);
    wire        rx_tgt1   = (mode == MODE_DUAL) ? rx_second : (mode == MODE_DEV1);
    wire [11:0] rx_off    = rx_second ? (byte_cnt - TILE_BYTES) : byte_cnt;

    // 送出去的位元組來自哪一顆、在該顆的哪個位移
    wire        tx_second = (mode == MODE_DUAL) && (byte_cnt >= OUT_BYTES);
    wire [11:0] tx_off    = tx_second ? (byte_cnt - OUT_BYTES) : byte_cnt;
    wire [2047:0] tx_src  = (mode == MODE_DUAL)
                          ? (tx_second ? arg2_out_flat1 : arg2_out_flat0)
                          : ((mode == MODE_DEV1) ? arg2_out_flat1 : arg2_out_flat0);

    // 組字是小端序,與舊版逐位元相同,不要改
    wire [31:0] word_in = {rx_data, word_buf[31:8]};

    always @(posedge clk) begin
        if (rst) begin
            state <= S_MODE;  mode <= MODE_DEV0;
            byte_cnt <= 0;    shift_pos <= 0;
            ap_start0 <= 0;   ap_start1 <= 0;
            tx_start <= 0;    done_led <= 0;
            done0_seen <= 0;  done1_seen <= 0;
        end else begin
            tx_start <= 0;
            case (state)

                S_MODE: begin                  // 第 1 個 byte = 模式
                    if (rx_valid) begin
                        // 只認 0/1/2;其他值一律當 dev0,避免收到雜訊時
                        // 進到一個收不完的狀態卡死。
                        mode      <= (rx_data[1:0] == 2'd2) ? MODE_DUAL
                                   : (rx_data[0]   == 1'b1) ? MODE_DEV1
                                   : MODE_DEV0;
                        done_led   <= 0;
                        byte_cnt   <= 0;
                        shift_pos  <= 0;
                        done0_seen <= 0;
                        done1_seen <= 0;
                        state      <= S_RX;
                    end
                end

                S_RX: begin
                    if (rx_valid) begin
                        word_buf <= word_in;
                        if (shift_pos == 3) begin
                            if (rx_off < 256) begin
                                if (rx_tgt1) a1_flat[(rx_off/4)*32 +: 32] <= word_in;
                                else         a0_flat[(rx_off/4)*32 +: 32] <= word_in;
                            end else if (rx_off < 512) begin
                                if (rx_tgt1) b1_flat[((rx_off-256)/4)*32 +: 32] <= word_in;
                                else         b0_flat[((rx_off-256)/4)*32 +: 32] <= word_in;
                            end else begin
                                if (rx_tgt1) c1_in_flat[((rx_off-512)/4)*32 +: 32] <= word_in;
                                else         c0_in_flat[((rx_off-512)/4)*32 +: 32] <= word_in;
                            end
                            shift_pos <= 0;
                        end else begin
                            shift_pos <= shift_pos + 1;
                        end
                        byte_cnt <= byte_cnt + 1;
                        if (byte_cnt == rx_total - 1) begin
                            state    <= S_START;
                            byte_cnt <= 0;
                        end
                    end
                end

                S_START: begin
                    // 並行模式下兩個 ap_start 同一拍拉起 —— 這一行就是
                    // 整個改動的重點,其餘都是為了餵飽它。
                    ap_start0 <= (mode != MODE_DEV1);
                    ap_start1 <= (mode != MODE_DEV0);
                    state <= S_WAIT;
                end

                S_WAIT: begin
                    ap_start0 <= 0;  ap_start1 <= 0;
                    if (ap_done0) done0_seen <= 1;
                    if (ap_done1) done1_seen <= 1;

                    // 每個模式只等自己啟動過的那些。等一顆沒啟動的陣列
                    // 會永遠等下去,因為它的 ap_done 不會來。
                    if ((mode == MODE_DEV0 && (done0_seen || ap_done0)) ||
                        (mode == MODE_DEV1 && (done1_seen || ap_done1)) ||
                        (mode == MODE_DUAL && (done0_seen || ap_done0)
                                           && (done1_seen || ap_done1))) begin
                        state    <= S_TX;
                        byte_cnt <= 0;
                    end
                end

                S_TX: begin
                    if (!tx_busy) begin
                        tx_data <= tx_src[(tx_off/4)*32 + (tx_off%4)*8 +: 8];
                        tx_start <= 1;
                        tx_busy_seen <= 0;
                        state <= S_TXWAIT;
                    end
                end

                S_TXWAIT: begin
                    if (tx_busy) tx_busy_seen <= 1;
                    if (tx_busy_seen && !tx_busy) begin
                        if (byte_cnt == tx_total - 1) state <= S_DONE;
                        else begin byte_cnt <= byte_cnt + 1; state <= S_TX; end
                    end
                end

                S_DONE: begin
                    done_led <= 1;
                    state <= S_MODE;
                end

                default: state <= S_MODE;
            endcase
        end
    end

endmodule
