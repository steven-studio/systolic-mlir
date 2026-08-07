// matmul_top_rk.v -- runtime-K 版頂層 controller。
//
// 與 matmul_top_dual.v 的差異只有兩處,其餘逐位元相同:
//
//   1. 實例化 matmul_iface_rk 而不是 matmul_iface。A/B 不再是 2048-bit
//      的展平向量,而是寫進 iface 內部的 16 顆 RAM。
//   2. 協定多了 K,而且 payload 長度隨 K 變。
//
// 協定
//   [1B dev 0x00/0x01]
//   [4B K, 小端序, clamp 到 1..64]
//   [A: bank 0 的 word 0..K-1, bank 1 的 0..K-1, ... bank 7]   32K bytes
//   [B: 同樣的順序,bank j = B 的第 j 行]                       32K bytes
//   [C_init 256B]
//   -> [C 256B]
//
// K=64 時一次交易是 1 + 4 + 2048 + 2048 + 256 = 4357 bytes 出、256 回。
// 對照固定 K=8 要做同樣的事:8 次呼叫 x (769+256) = 8200 bytes。
// 1.78x —— 省下來的是重複搬了八次的 C_init 和 C。
//
// 為什麼不用除法算 bank/word
// 位址是 {bank[2:0], word[5:0]},而 word 要在 K 處回捲。K 是執行期的值,
// 所以 w/K 和 w%K 都會合成出除法器。改用兩個計數器:word 數到 K-1 就
// 歸零並讓 bank 加一。零成本,而且不受 K 是不是 2 的冪次影響。
//
// 寫入脈衝為什麼是組合邏輯而不是暫存
// a_we/b_we/ab_waddr/ab_wdata 必須在同一個 posedge 對 RAM 有效。如果用
// `a_we <= 1` 就會晚一拍,而那時 fill_bank/fill_word 已經前進,寫到錯的
// 位址。組合邏輯讓 RAM 在這一拍看到舊的(正確的)計數值,計數器同時取
// 新值 —— 這是唯一對的順序。
//
// K 為什麼要 clamp
// K=0 會讓 PH_A 永遠等不到最後一個 word,FSM 卡死到重置為止。收到就夾。

module matmul_top_rk (
    input  wire clk_pin100,
    input  wire btn_rst,
    input  wire uart_rx_pin,
    output wire uart_tx_pin,
    output wire led_done
);

    localparam N      = 8;
    localparam K_MAX  = 64;
    localparam WORD_W = 6;              // clog2(K_MAX)
    localparam BANK_W = 3;              // clog2(N)

    wire clk;
    wire mmcm_locked;

    // 埠名 clk_out20 是歷史遺留;CLKOUT0_DIVIDE_F 已是 25.0 -> 40 MHz。
    // 如果這版的 WNS 變負,先把它改成 30.0 (33.3 MHz) 再說,不要動邏輯。
    clk_gen u_clk_gen (
        .clk_in100(clk_pin100), .rst_in(btn_rst),
        .clk_out20(clk), .locked(mmcm_locked)
    );

    wire rst = btn_rst | ~mmcm_locked;

    wire [7:0] rx_data;  wire rx_valid;
    reg  [7:0] tx_data;  reg  tx_start;  wire tx_busy;

    uart_rx #(.CLK_FREQ(40_000_000), .BAUD_RATE(2_000_000)) u_rx (
        .clk(clk), .rst(rst), .rx(uart_rx_pin),
        .data(rx_data), .valid(rx_valid)
    );
    uart_tx #(.CLK_FREQ(40_000_000), .BAUD_RATE(2_000_000)) u_tx (
        .clk(clk), .rst(rst), .data(tx_data), .start(tx_start),
        .tx(uart_tx_pin), .busy(tx_busy)
    );

    // ---------------- 狀態 ----------------
    localparam S_DEV = 0, S_RX = 1, S_START = 2, S_WAIT = 3,
               S_TX  = 4, S_TXWAIT = 5, S_DONE = 6;

    localparam PH_K = 2'd0, PH_A = 2'd1, PH_B = 2'd2, PH_C = 2'd3;

    reg [2:0]  state = S_DEV;
    reg [1:0]  phase = PH_K;
    reg        dev_sel = 0;
    reg        tx_busy_seen = 0;
    reg [2:0]  shift_pos = 0;
    reg [31:0] word_buf = 0;
    reg        done_led = 0;

    reg [WORD_W:0] k_reg = 7'd8;      // 實際使用的 K,已 clamp
    reg [BANK_W-1:0] fill_bank = 0;
    reg [WORD_W-1:0] fill_word = 0;
    reg [5:0]        c_idx = 0;         // C_init 的第幾個 word,0..63
    reg [7:0]        tx_cnt = 0;        // 0..255

    assign led_done = done_led;

    // 小端序組字,與舊版逐位元相同
    wire [31:0] word_in   = {rx_data, word_buf[31:8]};
    wire        word_done = rx_valid && (shift_pos == 3);

    // 一個 bank 的最後一個 word;K=1 時 k_reg-1 = 0,條件仍然成立
    wire word_last = (fill_word == k_reg - 1'b1);
    wire bank_last = (fill_bank == N-1);

    // ---------------- 運算元寫入埠 ----------------
    // 組合邏輯,見檔頭說明
    wire                    a_we     = word_done && (phase == PH_A);
    wire                    b_we     = word_done && (phase == PH_B);
    wire [BANK_W+WORD_W-1:0] ab_waddr = {fill_bank, fill_word};
    wire [31:0]             ab_wdata = word_in;

    wire a_we0 = a_we && ~dev_sel;
    wire a_we1 = a_we &&  dev_sel;
    wire b_we0 = b_we && ~dev_sel;
    wire b_we1 = b_we &&  dev_sel;

    // ---------------- C 緩衝(仍然是展平的) ----------------
    reg [2047:0] c_in_flat;

    reg          ap_start0, ap_start1;
    wire         ap_done0, ap_idle0, ap_ready0;
    wire         ap_done1, ap_idle1, ap_ready1;
    wire [2047:0] arg2_out_flat0, arg2_out_flat1;
    wire [63:0]  arg2_vld_flat0, arg2_vld_flat1;

    matmul_iface_rk u_iface0 (
        .ap_clk(clk), .ap_rst(rst),
        .ap_start(ap_start0),
        .ap_done(ap_done0), .ap_idle(ap_idle0), .ap_ready(ap_ready0),
        .k_val({25'd0, k_reg}),
        .a_we(a_we0), .b_we(b_we0),
        .ab_waddr(ab_waddr), .ab_wdata(ab_wdata),
        .arg2_in_flat(c_in_flat),
        .arg2_out_flat(arg2_out_flat0), .arg2_vld_flat(arg2_vld_flat0)
    );

    matmul_iface_rk u_iface1 (
        .ap_clk(clk), .ap_rst(rst),
        .ap_start(ap_start1),
        .ap_done(ap_done1), .ap_idle(ap_idle1), .ap_ready(ap_ready1),
        .k_val({26'd0, k_reg}),
        .a_we(a_we1), .b_we(b_we1),
        .ab_waddr(ab_waddr), .ab_wdata(ab_wdata),
        .arg2_in_flat(c_in_flat),
        .arg2_out_flat(arg2_out_flat1), .arg2_vld_flat(arg2_vld_flat1)
    );

    wire          ap_done_sel  = dev_sel ? ap_done1       : ap_done0;
    wire [2047:0] arg2_out_sel = dev_sel ? arg2_out_flat1 : arg2_out_flat0;

    // ---------------- FSM ----------------
    always @(posedge clk) begin
        if (rst) begin
            state <= S_DEV;   phase <= PH_K;
            dev_sel <= 0;     shift_pos <= 0;
            fill_bank <= 0;   fill_word <= 0;
            c_idx <= 0;       tx_cnt <= 0;
            k_reg <= 7'd8;
            ap_start0 <= 0;   ap_start1 <= 0;
            tx_start <= 0;    done_led <= 0;
        end else begin
            tx_start <= 0;
            case (state)

                S_DEV: begin
                    if (rx_valid) begin
                        dev_sel   <= rx_data[0];
                        done_led  <= 0;
                        shift_pos <= 0;
                        phase     <= PH_K;
                        fill_bank <= 0;
                        fill_word <= 0;
                        c_idx     <= 0;
                        state     <= S_RX;
                    end
                end

                S_RX: begin
                    if (rx_valid) begin
                        word_buf <= word_in;
                        if (shift_pos == 3) begin
                            shift_pos <= 0;
                            case (phase)

                                PH_K: begin
                                    // clamp 到 1..K_MAX。0 會卡死,超過
                                    // K_MAX 會寫爆 RAM 的位址空間。
                                    if (word_in == 0)
                                        k_reg <= 7'd1;
                                    else if (word_in > K_MAX)
                                        k_reg <= K_MAX[WORD_W:0];
                                    else
                                        k_reg <= word_in[WORD_W:0];
                                    phase     <= PH_A;
                                    fill_bank <= 0;
                                    fill_word <= 0;
                                end

                                PH_A: begin
                                    // 寫入已由組合的 a_we 在這個 posedge
                                    // 完成,這裡只推進計數
                                    if (word_last) begin
                                        fill_word <= 0;
                                        if (bank_last) begin
                                            fill_bank <= 0;
                                            phase     <= PH_B;
                                        end else begin
                                            fill_bank <= fill_bank + 1'b1;
                                        end
                                    end else begin
                                        fill_word <= fill_word + 1'b1;
                                    end
                                end

                                PH_B: begin
                                    if (word_last) begin
                                        fill_word <= 0;
                                        if (bank_last) begin
                                            fill_bank <= 0;
                                            phase     <= PH_C;
                                            c_idx     <= 0;
                                        end else begin
                                            fill_bank <= fill_bank + 1'b1;
                                        end
                                    end else begin
                                        fill_word <= fill_word + 1'b1;
                                    end
                                end

                                PH_C: begin
                                    c_in_flat[c_idx*32 +: 32] <= word_in;
                                    if (c_idx == N*N-1) state <= S_START;
                                    else                 c_idx <= c_idx + 1'b1;
                                end

                            endcase
                        end else begin
                            shift_pos <= shift_pos + 1'b1;
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
                        state  <= S_TX;
                        tx_cnt <= 0;
                    end
                end

                S_TX: begin
                    if (!tx_busy) begin
                        tx_data <= arg2_out_sel[(tx_cnt/4)*32 + (tx_cnt%4)*8 +: 8];
                        tx_start <= 1;
                        tx_busy_seen <= 0;
                        state <= S_TXWAIT;
                    end
                end

                S_TXWAIT: begin
                    if (tx_busy) tx_busy_seen <= 1;
                    if (tx_busy_seen && !tx_busy) begin
                        if (tx_cnt == 8'd255) state <= S_DONE;
                        else begin tx_cnt <= tx_cnt + 1'b1; state <= S_TX; end
                    end
                end

                S_DONE: begin
                    done_led <= 1;
                    state <= S_DEV;
                end

                default: state <= S_DEV;
            endcase
        end
    end

endmodule
