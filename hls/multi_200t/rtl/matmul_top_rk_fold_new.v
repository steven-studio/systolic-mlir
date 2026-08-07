// matmul_top_rk_fold.v -- runtime-K controller that iterates folds on-chip.
//
// WHY THIS EXISTS
//
// matmul_top_rk.v handles exactly one fold per UART transaction: the host
// sends an 8x8 output block, the array computes it, the host reads it back.
// A tile larger than the array is decomposed on the host, so a 64x64x64 GEMM
// costs 64 round trips and re-sends the same A row-blocks and B column-blocks
// 8 times each. At 2 Mbaud that is ~278 KB and ~1.4 s, against ~142 us of
// actual compute -- four orders of magnitude of pure interconnect.
//
// This version walks the fold grid itself: A and B go up once, the FSM
// iterates (fi, fj), and only the C blocks come back. Same 64x64x64 becomes
// 48 KB, ~0.24 s. The array is untouched -- matmul_8x8x8 is byte-identical to
// the version in the known-good bitstream, and so is its wrapper's behaviour
// at MT=NT=1.
//
// PROTOCOL (changed -- host must be updated)
//   [1B  dev 0x00/0x01]
//   [4B  K,  little-endian, clamped to 1..K_MAX]
//   [4B  Mt, little-endian, clamped to 1..MT_MAX]
//   [4B  Nt, little-endian, clamped to 1..NT_MAX]
//   [A:  for fi in 0..Mt-1, for bank in 0..7, words 0..K-1]   Mt*8*K*4 bytes
//   [B:  for fj in 0..Nt-1, for bank in 0..7, words 0..K-1]   Nt*8*K*4 bytes
//   [C_init: for fi, for fj: 256 bytes]                       Mt*Nt*256 bytes
//   -> [C: for fi, for fj: 256 bytes]                         Mt*Nt*256 bytes
//
// A's bank index within fold fi is row fi*8+i, so "for fi, for bank" is just
// A in row-major order. B's bank j within fold fj is column fj*8+j, so B goes
// up column-major -- the same transpose the single-fold protocol already
// used, extended outward.
//
// At Mt=Nt=1 the payload after the header is byte-for-byte the old protocol
// and the computed result is identical; only the 8 header bytes are new. That
// is the regression test: run the existing single-fold vectors with Mt=Nt=1
// and require the same 6/6 bit-exact result before trying 4 folds.
//
// WHY NO DIVISION, ANYWHERE
//
// The linear fold index would be fi*Nt+fj, and the C-buffer address would
// need it. Nt is a runtime value, so that multiply -- like the w/K and w%K
// the single-fold controller already refuses to compute -- would synthesise
// real arithmetic on an address path. Instead the buffer is addressed by the
// concatenation {fi, fj} with fixed field widths. Entries above the runtime
// Mt*Nt simply go unused. Zero cost, and it does not care whether Mt or Nt is
// a power of two.
//
// THE C BUFFER IS ONE WRITE PORT AND ONE REGISTERED READ
//
// Deliberately, so it maps to block RAM when MT/NT grow. Reading it
// combinationally would build an (MT*NT):1 mux 2048 bits wide -- tolerable at
// 4 folds, hopeless at 64.
//
// A registered read costs a cycle of latency, and that cycle is why the load
// is split in two states rather than one. S_WAIT advances run_fi/run_fj on
// the beat it sees ap_done, so cbuf_raddr only carries the *next* fold's
// address from the following beat, and cbuf_q is only valid the beat after
// that. S_LOAD_A lets the address settle; S_LOAD_D captures. Collapsing them
// would feed the array the previous fold's C_init -- a plausible-looking
// wrong answer, not a visible failure. S_TXLD_A/S_TXLD_D are the same shape.
// Two cycles per fold against ~89 of compute.
//
// SIGNALS THAT MUST BE STABLE ACROSS ap_start..ap_done
//
// k_val, fold_i, fold_j. All three are sampled combinationally by the array
// wrapper, not latched. run_fi/run_fj only advance in S_WAIT after ap_done,
// so they are stable for the whole call; do not "optimise" that by advancing
// them earlier.

module matmul_top_rk_fold (
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

    // Fold capacity. Must match the arguments given to
    // gen_matmul_iface_fold_new.py when matmul_iface_rk_fold.v was produced,
    // or ab_waddr will be the wrong width and the operands will land in the
    // wrong RAM rows.
    localparam MT_MAX = 2;
    localparam NT_MAX = 2;
    localparam FI_W   = 1;              // clog2(MT_MAX)
    localparam FJ_W   = 1;              // clog2(NT_MAX)
    localparam FOLD_W = 1;              // max(FI_W, FJ_W), width in ab_waddr
    localparam FIJ_W  = FI_W + FJ_W;    // C-buffer address width
    localparam WADDR_W = BANK_W + FOLD_W + WORD_W;   // 10

    wire clk;
    wire mmcm_locked;

    // 埠名 clk_out20 是歷史遺留;CLKOUT0_DIVIDE_F 已是 25.0 -> 40 MHz。
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
    localparam S_DEV    = 0, S_RX     = 1, S_LOAD_A = 2, S_LOAD_D = 3,
               S_START  = 4, S_WAIT   = 5, S_TXLD_A = 6, S_TXLD_D = 7,
               S_TX     = 8, S_TXWAIT = 9, S_DONE   = 10;

    localparam PH_K  = 3'd0, PH_MT = 3'd1, PH_NT = 3'd2,
               PH_A  = 3'd3, PH_B  = 3'd4, PH_C  = 3'd5;

    reg [3:0]  state = S_DEV;
    reg [2:0]  phase = PH_K;
    reg        dev_sel = 0;
    reg        tx_busy_seen = 0;
    reg [2:0]  shift_pos = 0;
    reg [31:0] word_buf = 0;
    reg        done_led = 0;

    reg [WORD_W:0]   k_reg  = 7'd8;     // 已 clamp 的 K
    reg [FI_W:0]     mt_reg = 1;        // 已 clamp 的 Mt (1..MT_MAX)
    reg [FJ_W:0]     nt_reg = 1;        // 已 clamp 的 Nt (1..NT_MAX)

    reg [BANK_W-1:0] fill_bank = 0;
    reg [WORD_W-1:0] fill_word = 0;
    reg [FOLD_W-1:0] fill_fold = 0;

    reg [5:0]        c_idx = 0;         // C_init 的第幾個 word,0..63
    reg [FI_W-1:0]   c_fi  = 0;         // 正在接收哪一個 C 區塊
    reg [FJ_W-1:0]   c_fj  = 0;
    reg [FI_W-1:0]   run_fi = 0;        // 正在計算哪一個 fold
    reg [FJ_W-1:0]   run_fj = 0;
    reg [FI_W-1:0]   tx_fi  = 0;        // 正在回傳哪一個 C 區塊
    reg [FJ_W-1:0]   tx_fj  = 0;
    reg [7:0]        tx_cnt = 0;        // 0..255

    assign led_done = done_led;

    // 小端序組字,與單 fold 版逐位元相同
    wire [31:0] word_in   = {rx_data, word_buf[31:8]};
    wire        word_done = rx_valid && (shift_pos == 3);

    // 一個 bank 的最後一個 word;K=1 時 k_reg-1 = 0,條件仍然成立
    wire word_last = (fill_word == k_reg - 1'b1);
    wire bank_last = (fill_bank == N-1);
    // A 走 Mt 個 fold,B 走 Nt 個
    wire fold_last = (phase == PH_A) ? (fill_fold == mt_reg - 1'b1)
                                     : (fill_fold == nt_reg - 1'b1);

    // ---------------- 運算元寫入埠 ----------------
    // 組合邏輯:a_we/ab_waddr 必須在同一個 posedge 對 RAM 有效。用非阻塞
    // 指定會晚一拍,而那時計數器已經前進,寫到錯的位址。
    wire                 a_we     = word_done && (phase == PH_A);
    wire                 b_we     = word_done && (phase == PH_B);
    wire [WADDR_W-1:0]   ab_waddr = {fill_bank, fill_fold, fill_word};
    wire [31:0]          ab_wdata = word_in;

    wire a_we0 = a_we && ~dev_sel;
    wire a_we1 = a_we &&  dev_sel;
    wire b_we0 = b_we && ~dev_sel;
    wire b_we1 = b_we &&  dev_sel;

    // ---------------- C 緩衝 ----------------
    // 一個寫入埠、一個註冊過的讀取埠 —— 見檔頭。
    reg [2047:0] c_buf [0:(1<<FIJ_W)-1];
    reg [2047:0] c_fill;                // 正在從 UART 組裝的區塊
    reg [2047:0] c_in_reg;              // 餵給陣列的區塊
    reg [2047:0] tx_blk;                // 正在回傳的區塊

    wire         ap_done_sel;
    wire [2047:0] arg2_out_sel;

    // C_init 的最後一個 word 剛剛到:c_fill 的非阻塞指定還沒生效,所以最高
    // 那個 word 要直接從 word_in 取。
    wire rx_c_last = (state == S_RX) && word_done && (phase == PH_C)
                     && (c_idx == N*N-1);
    wire compute_done = (state == S_WAIT) && ap_done_sel;

    wire              cbuf_we    = rx_c_last | compute_done;
    wire [FIJ_W-1:0]  cbuf_waddr = rx_c_last ? {c_fi, c_fj} : {run_fi, run_fj};
    wire [2047:0]     cbuf_wdata = rx_c_last ? {word_in, c_fill[2015:0]}
                                             : arg2_out_sel;

    // 讀取位址:計算階段用 run_*,回傳階段用 tx_*
    wire [FIJ_W-1:0]  cbuf_raddr = (state == S_TXLD_A || state == S_TXLD_D)
                                       ? {tx_fi, tx_fj} : {run_fi, run_fj};
    reg  [2047:0]     cbuf_q;

    always @(posedge clk) begin
        if (cbuf_we) c_buf[cbuf_waddr] <= cbuf_wdata;
        cbuf_q <= c_buf[cbuf_raddr];
    end

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
        .fold_i(run_fi), .fold_j(run_fj),
        .a_we(a_we0), .b_we(b_we0),
        .ab_waddr(ab_waddr), .ab_wdata(ab_wdata),
        .arg2_in_flat(c_in_reg),
        .arg2_out_flat(arg2_out_flat0), .arg2_vld_flat(arg2_vld_flat0)
    );

    matmul_iface_rk u_iface1 (
        .ap_clk(clk), .ap_rst(rst),
        .ap_start(ap_start1),
        .ap_done(ap_done1), .ap_idle(ap_idle1), .ap_ready(ap_ready1),
        .k_val({25'd0, k_reg}),
        .fold_i(run_fi), .fold_j(run_fj),
        .a_we(a_we1), .b_we(b_we1),
        .ab_waddr(ab_waddr), .ab_wdata(ab_wdata),
        .arg2_in_flat(c_in_reg),
        .arg2_out_flat(arg2_out_flat1), .arg2_vld_flat(arg2_vld_flat1)
    );

    assign ap_done_sel  = dev_sel ? ap_done1       : ap_done0;
    assign arg2_out_sel = dev_sel ? arg2_out_flat1 : arg2_out_flat0;

    // 最後一個 fold / 最後一個回傳區塊
    wire run_last = (run_fi == mt_reg - 1'b1) && (run_fj == nt_reg - 1'b1);
    wire tx_last  = (tx_fi  == mt_reg - 1'b1) && (tx_fj  == nt_reg - 1'b1);
    wire c_last   = (c_fi   == mt_reg - 1'b1) && (c_fj   == nt_reg - 1'b1);

    // ---------------- FSM ----------------
    always @(posedge clk) begin
        if (rst) begin
            state <= S_DEV;   phase <= PH_K;
            dev_sel <= 0;     shift_pos <= 0;
            fill_bank <= 0;   fill_word <= 0;   fill_fold <= 0;
            c_idx <= 0;       c_fi <= 0;        c_fj <= 0;
            run_fi <= 0;      run_fj <= 0;
            tx_fi <= 0;       tx_fj <= 0;       tx_cnt <= 0;
            k_reg <= 7'd8;    mt_reg <= 1;      nt_reg <= 1;
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
                        fill_bank <= 0;  fill_word <= 0;  fill_fold <= 0;
                        c_idx     <= 0;  c_fi <= 0;       c_fj <= 0;
                        run_fi    <= 0;  run_fj <= 0;
                        tx_fi     <= 0;  tx_fj <= 0;
                        state     <= S_RX;
                    end
                end

                S_RX: begin
                    if (rx_valid) begin
                        word_buf <= word_in;
                        if (shift_pos == 3) begin
                            shift_pos <= 0;
                            case (phase)

                                // clamp 到 1..K_MAX。0 會讓 PH_A 永遠等不到
                                // 最後一個 word,FSM 卡死到重置為止;超過
                                // K_MAX 會寫爆 RAM 的位址空間。
                                PH_K: begin
                                    if (word_in == 0)
                                        k_reg <= 7'd1;
                                    else if (word_in > K_MAX)
                                        k_reg <= K_MAX[WORD_W:0];
                                    else
                                        k_reg <= word_in[WORD_W:0];
                                    phase <= PH_MT;
                                end

                                // Mt/Nt 同樣要 clamp,理由相同:0 會卡死
                                // fold 迴圈,超過 MT_MAX 會索引到不存在的
                                // RAM 區塊。
                                PH_MT: begin
                                    if (word_in == 0)
                                        mt_reg <= 1;
                                    else if (word_in > MT_MAX)
                                        mt_reg <= MT_MAX[FI_W:0];
                                    else
                                        mt_reg <= word_in[FI_W:0];
                                    phase <= PH_NT;
                                end

                                PH_NT: begin
                                    if (word_in == 0)
                                        nt_reg <= 1;
                                    else if (word_in > NT_MAX)
                                        nt_reg <= NT_MAX[FJ_W:0];
                                    else
                                        nt_reg <= word_in[FJ_W:0];
                                    phase     <= PH_A;
                                    fill_bank <= 0;
                                    fill_word <= 0;
                                    fill_fold <= 0;
                                end

                                // 寫入已由組合的 a_we 在這個 posedge 完成,
                                // 這裡只推進計數:word -> bank -> fold
                                PH_A: begin
                                    if (word_last) begin
                                        fill_word <= 0;
                                        if (bank_last) begin
                                            fill_bank <= 0;
                                            if (fold_last) begin
                                                fill_fold <= 0;
                                                phase     <= PH_B;
                                            end else begin
                                                fill_fold <= fill_fold + 1'b1;
                                            end
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
                                            if (fold_last) begin
                                                fill_fold <= 0;
                                                phase     <= PH_C;
                                                c_idx     <= 0;
                                                c_fi      <= 0;
                                                c_fj      <= 0;
                                            end else begin
                                                fill_fold <= fill_fold + 1'b1;
                                            end
                                        end else begin
                                            fill_bank <= fill_bank + 1'b1;
                                        end
                                    end else begin
                                        fill_word <= fill_word + 1'b1;
                                    end
                                end

                                // 整塊寫進 c_buf 由 cbuf_we/rx_c_last 處理
                                PH_C: begin
                                    c_fill[c_idx*32 +: 32] <= word_in;
                                    if (c_idx == N*N-1) begin
                                        c_idx <= 0;
                                        if (c_last) begin
                                            c_fi   <= 0;  c_fj <= 0;
                                            run_fi <= 0;  run_fj <= 0;
                                            state  <= S_LOAD_A;
                                        end else if (c_fj == nt_reg - 1'b1) begin
                                            c_fj <= 0;
                                            c_fi <= c_fi + 1'b1;
                                        end else begin
                                            c_fj <= c_fj + 1'b1;
                                        end
                                    end else begin
                                        c_idx <= c_idx + 1'b1;
                                    end
                                end

                                default: ;
                            endcase
                        end else begin
                            shift_pos <= shift_pos + 1'b1;
                        end
                    end
                end

                // 位址剛換,先讓 cbuf_raddr 穩定一拍 —— cbuf_q 這時還是上一
                // 個 fold 的值,不能用。見檔頭。
                S_LOAD_A: state <= S_LOAD_D;

                S_LOAD_D: begin
                    c_in_reg <= cbuf_q;
                    state    <= S_START;
                end

                S_START: begin
                    if (dev_sel) ap_start1 <= 1;
                    else         ap_start0 <= 1;
                    state <= S_WAIT;
                end

                S_WAIT: begin
                    ap_start0 <= 0;  ap_start1 <= 0;
                    if (ap_done_sel) begin
                        // 結果由 cbuf_we/compute_done 寫進 c_buf[{run_fi,run_fj}]
                        if (run_last) begin
                            run_fi <= 0;  run_fj <= 0;
                            tx_fi  <= 0;  tx_fj  <= 0;
                            tx_cnt <= 0;
                            state  <= S_TXLD_A;
                        end else begin
                            if (run_fj == nt_reg - 1'b1) begin
                                run_fj <= 0;
                                run_fi <= run_fi + 1'b1;
                            end else begin
                                run_fj <= run_fj + 1'b1;
                            end
                            state <= S_LOAD_A;
                        end
                    end
                end

                // 同樣先讓 {tx_fi,tx_fj} 穩定一拍再抓 cbuf_q
                S_TXLD_A: state <= S_TXLD_D;

                S_TXLD_D: begin
                    tx_blk <= cbuf_q;
                    tx_cnt <= 0;
                    state  <= S_TX;
                end

                S_TX: begin
                    if (!tx_busy) begin
                        // (n/4)*32 + (n%4)*8 == n*8 for all n; 展開式與單 fold
                        // 版等價,這裡取簡短形式。
                        tx_data      <= tx_blk[tx_cnt*8 +: 8];
                        tx_start     <= 1;
                        tx_busy_seen <= 0;
                        state        <= S_TXWAIT;
                    end
                end

                S_TXWAIT: begin
                    if (tx_busy) tx_busy_seen <= 1;
                    if (tx_busy_seen && !tx_busy) begin
                        if (tx_cnt == 8'd255) begin
                            if (tx_last) begin
                                state <= S_DONE;
                            end else begin
                                if (tx_fj == nt_reg - 1'b1) begin
                                    tx_fj <= 0;
                                    tx_fi <= tx_fi + 1'b1;
                                end else begin
                                    tx_fj <= tx_fj + 1'b1;
                                end
                                state <= S_TXLD_A;
                            end
                        end else begin
                            tx_cnt <= tx_cnt + 1'b1;
                            state  <= S_TX;
                        end
                    end
                end

                S_DONE: begin
                    done_led <= 1;
                    state    <= S_DEV;
                end

                default: state <= S_DEV;
            endcase
        end
    end

endmodule
