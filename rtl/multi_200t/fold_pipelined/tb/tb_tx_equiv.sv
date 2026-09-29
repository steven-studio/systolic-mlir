/*
 * tb_tx_equiv -- TX streaming 重構的等價證明。
 *
 * golden = 舊 top 的 TX FSM + tx_word/tx_byte mux,逐字複製(僅把
 *          state==ST_SEND 換成 send_level 輸入)。
 * DUT    = systolic_tx_source + uart_tx_streamer。
 *
 * 兩側掛相同的 uart_tx stub(busy N 拍的快速模型,不跑真實鮑率),
 * 餵完全相同的刺激(C0/C1/cyc、交易觸發、debug_set 事件脈衝),
 * 捕捉各自送出的 byte 序列,邊跑邊逐 byte 比對。
 *
 * 比對的是「byte 序列」不是逐 cycle -- byte 之間的間隔 host 看不
 * 見(UART 線速才是瓶頸),鎖序列不鎖時序,streamer 內部才有重構
 * 自由。
 *
 * 三個 pair:
 *   p_clean  DEBUG_MARKERS=0, CYCLE_COUNTER=1  (板上組態)
 *   p_dbg    DEBUG_MARKERS=1, CYCLE_COUNTER=1  (除錯組態)
 *   p_neg    負向控制:DUT 側第 100 個 byte 故意打壞,tb 必須抓到
 *
 * PASS 條件:
 *   p_clean/p_dbg errors==0、兩側 byte 數相等、checked 符合預期
 *   p_neg errors>0(證明 harness 抓得到錯)
 *   任何 pair checked==0 -> 直接 FAIL(vacuous-pass 防呆)
 */
`timescale 1ns / 1ps

/* ------------------------------------------------------------------ */
/* uart_tx 快速模型:start 後 busy N 拍,並回報捕捉到的 byte          */
/* ------------------------------------------------------------------ */
module uart_tx_stub #(
    parameter int BUSY_CYCLES = 6
)(
    input  logic       clk,
    input  logic       rst,
    input  logic       start,
    input  logic [7:0] data_in,
    output logic       busy,
    output logic [7:0] cap_byte,
    output logic       cap_valid
);
    integer cnt;

    always_ff @(posedge clk) begin
        if (rst) begin
            busy      <= 1'b0;
            cnt       <= 0;
            cap_valid <= 1'b0;
            cap_byte  <= 8'h00;
        end
        else begin
            cap_valid <= 1'b0;

            if (!busy) begin
                if (start) begin
                    busy      <= 1'b1;
                    cnt       <= BUSY_CYCLES;
                    cap_byte  <= data_in;
                    cap_valid <= 1'b1;
                end
            end
            else begin
                cnt <= cnt - 1;
                if (cnt == 1)
                    busy <= 1'b0;
            end
        end
    end
endmodule


/* ------------------------------------------------------------------ */
/* golden:舊 TX 邏輯逐字複製                                          */
/* ------------------------------------------------------------------ */
module tx_golden #(
    parameter bit DEBUG_MARKERS = 1'b0,
    parameter bit CYCLE_COUNTER = 1'b0
)(
    input  logic        clk,
    input  logic        rst,
    input  logic        send_level,      // == (state == ST_SEND)
    input  logic [4:0]  debug_pending,
    output logic [4:0]  debug_accept,
    input  logic [31:0] C0 [0:7][0:7],
    input  logic [31:0] C1 [0:7][0:7],
    input  logic [31:0] cyc_latched,
    output logic        tx_all_done,
    output logic        tx_start,
    output logic [7:0]  tx_byte,
    input  logic        tx_busy
);
    localparam int TX_BYTES       = 512;
    localparam int TX_TOTAL_BYTES = TX_BYTES + (CYCLE_COUNTER ? 4 : 0);
    localparam int TX_LAST        = TX_TOTAL_BYTES - 1;

    logic [9:0]  tx_count;
    logic [31:0] tx_word;
    logic        tx_send_started;
    logic        debug_tx_active;
    logic [7:0]  debug_tx_byte;

    /* ---- 逐字搬自 systolic_uart_tile_top ---- */
    always_comb begin
        if (tx_count < 256) begin
            tx_word = C0[tx_count[7:5]][tx_count[4:2]];
        end
        else if (tx_count < 512) begin
            tx_word = C1[(tx_count - 256) >> 5][((tx_count - 256) >> 2) & 7];
        end
        else begin
            tx_word = cyc_latched;
        end

        if (debug_tx_active) begin
            tx_byte = debug_tx_byte;
        end
        else begin
            case (tx_count[1:0])
                2'd0:    tx_byte = tx_word[7:0];
                2'd1:    tx_byte = tx_word[15:8];
                2'd2:    tx_byte = tx_word[23:16];
                default: tx_byte = tx_word[31:24];
            endcase
        end
    end

    typedef enum logic [1:0] {
        TX_IDLE,
        TX_START,
        TX_WAIT_BUSY,
        TX_WAIT_DONE
    } tx_state_t;

    tx_state_t tx_state;

    always_ff @(posedge clk) begin
        if (rst) begin
            tx_count        <= 10'd0;
            tx_start        <= 1'b0;
            tx_state        <= TX_IDLE;
            tx_all_done     <= 1'b0;
            debug_tx_active <= 1'b0;
            debug_tx_byte   <= 8'h00;
            debug_accept    <= 5'b0;
            tx_send_started <= 1'b0;
        end
        else begin
            tx_start     <= 1'b0;
            tx_all_done  <= 1'b0;
            debug_accept <= 5'b0;

            case (tx_state)

                TX_IDLE: begin
                    if (DEBUG_MARKERS && debug_pending[0]) begin
                        debug_tx_active <= 1'b1;
                        debug_tx_byte   <= 8'hA1;
                        debug_accept[0] <= 1'b1;
                        tx_state        <= TX_START;
                    end
                    else if (DEBUG_MARKERS && debug_pending[1]) begin
                        debug_tx_active <= 1'b1;
                        debug_tx_byte   <= 8'hA2;
                        debug_accept[1] <= 1'b1;
                        tx_state        <= TX_START;
                    end
                    else if (DEBUG_MARKERS && debug_pending[2]) begin
                        debug_tx_active <= 1'b1;
                        debug_tx_byte   <= 8'hA3;
                        debug_accept[2] <= 1'b1;
                        tx_state        <= TX_START;
                    end
                    else if (DEBUG_MARKERS && debug_pending[3]) begin
                        debug_tx_active <= 1'b1;
                        debug_tx_byte   <= 8'hA4;
                        debug_accept[3] <= 1'b1;
                        tx_state        <= TX_START;
                    end
                    else if (DEBUG_MARKERS && debug_pending[4]) begin
                        debug_tx_active <= 1'b1;
                        debug_tx_byte   <= 8'hA5;
                        debug_accept[4] <= 1'b1;
                        tx_state        <= TX_START;
                    end
                    else if (send_level && !tx_send_started) begin
                        debug_tx_active <= 1'b0;
                        tx_send_started <= 1'b1;
                        tx_count        <= 10'd0;
                        tx_state        <= TX_START;
                    end

                    if (!send_level)
                        tx_send_started <= 1'b0;
                end

                TX_START: begin
                    if (!tx_busy) begin
                        tx_start <= 1'b1;
                        tx_state <= TX_WAIT_BUSY;
                    end
                end

                TX_WAIT_BUSY: begin
                    if (tx_busy)
                        tx_state <= TX_WAIT_DONE;
                end

                TX_WAIT_DONE: begin
                    if (!tx_busy) begin
                        if (debug_tx_active) begin
                            debug_tx_active <= 1'b0;
                            tx_state        <= TX_IDLE;
                        end
                        else if (tx_count == TX_LAST[9:0]) begin
                            tx_count    <= 10'd0;
                            tx_state    <= TX_IDLE;
                            tx_all_done <= 1'b1;
                        end
                        else begin
                            tx_count <= tx_count + 1'b1;
                            tx_state <= TX_START;
                        end
                    end
                end

                default: tx_state <= TX_IDLE;

            endcase
        end
    end
endmodule


/* ------------------------------------------------------------------ */
/* pair:golden 與 DUT 並排,同刺激,序列比對                           */
/* ------------------------------------------------------------------ */
module tx_equiv_pair #(
    parameter bit DEBUG_MARKERS = 1'b0,
    parameter bit CYCLE_COUNTER = 1'b1,
    parameter bit NEGATIVE_CTRL = 1'b0
)(
    input  logic        clk,
    input  logic        rst,
    input  logic        trigger,     // 1-cycle pulse:開始一筆交易
    input  logic [4:0]  dbg_set,     // breadcrumb 事件脈衝
    input  logic [31:0] C0 [0:7][0:7],
    input  logic [31:0] C1 [0:7][0:7],
    input  logic [31:0] cyc_latched
);
    /* ---- golden 側:pending 暫存器 + 主 FSM 模擬 + 舊 FSM + stub ---- */
    logic [4:0] g_pending;
    logic [4:0] g_accept;
    logic       g_send, g_done;
    logic       g_start, g_busy;
    logic [7:0] g_byte;
    logic [7:0] g_cap;
    logic       g_capv;

    always_ff @(posedge clk) begin
        if (rst) begin
            g_pending <= 5'b0;
            g_send    <= 1'b0;
        end
        else begin
            // 與 top 相同的 pending 更新式
            g_pending <= (g_pending & ~g_accept) | dbg_set;
            // 主 FSM 模擬:ST_SEND 進入 <- trigger,離開 <- all_done
            if (trigger)     g_send <= 1'b1;
            else if (g_done) g_send <= 1'b0;
        end
    end

    tx_golden #(
        .DEBUG_MARKERS (DEBUG_MARKERS),
        .CYCLE_COUNTER (CYCLE_COUNTER)
    ) u_gold (
        .clk           (clk),
        .rst           (rst),
        .send_level    (g_send),
        .debug_pending (g_pending),
        .debug_accept  (g_accept),
        .C0            (C0),
        .C1            (C1),
        .cyc_latched   (cyc_latched),
        .tx_all_done   (g_done),
        .tx_start      (g_start),
        .tx_byte       (g_byte),
        .tx_busy       (g_busy)
    );

    uart_tx_stub u_gstub (
        .clk       (clk),
        .rst       (rst),
        .start     (g_start),
        .data_in   (g_byte),
        .busy      (g_busy),
        .cap_byte  (g_cap),
        .cap_valid (g_capv)
    );

    /* ---- DUT 側:同構,換上 source + streamer ---- */
    logic [4:0] n_pending;
    logic [4:0] n_accept;
    logic       n_send, n_done;
    logic       n_start, n_busy;
    logic [7:0] n_byte;
    logic [7:0] n_cap;
    logic       n_capv;
    logic       m_valid, m_ready;
    logic [7:0] m_data;

    always_ff @(posedge clk) begin
        if (rst) begin
            n_pending <= 5'b0;
            n_send    <= 1'b0;
        end
        else begin
            n_pending <= (n_pending & ~n_accept) | dbg_set;
            if (trigger)     n_send <= 1'b1;
            else if (n_done) n_send <= 1'b0;
        end
    end

    systolic_tx_source #(
        .DEBUG_MARKERS (DEBUG_MARKERS),
        .CYCLE_COUNTER (CYCLE_COUNTER)
    ) u_src (
        .clk           (clk),
        .rst           (rst),
        .send_go       (n_send),
        .all_done      (n_done),
        .debug_pending (n_pending),
        .debug_accept  (n_accept),
        .C0            (C0),
        .C1            (C1),
        .cyc_latched   (cyc_latched),
        .m_valid       (m_valid),
        .m_data        (m_data),
        .m_ready       (m_ready)
    );

    uart_tx_streamer u_str (
        .clk      (clk),
        .rst      (rst),
        .s_valid  (m_valid),
        .s_data   (m_data),
        .s_ready  (m_ready),
        .tx_start (n_start),
        .tx_data  (n_byte),
        .tx_busy  (n_busy)
    );

    uart_tx_stub u_nstub (
        .clk       (clk),
        .rst       (rst),
        .start     (n_start),
        .data_in   (n_byte),
        .busy      (n_busy),
        .cap_byte  (n_cap),
        .cap_valid (n_capv)
    );

    /* ---- 捕捉 + 逐 byte 比對 ---- */
    logic [7:0] gbuf [0:16383];
    logic [7:0] nbuf [0:16383];
    integer gcnt, ncnt, idx, errors, checked;

    always_ff @(posedge clk) begin
        if (rst) begin
            gcnt <= 0;
            ncnt <= 0;
        end
        else begin
            if (g_capv) begin
                gbuf[gcnt] <= g_cap;
                gcnt       <= gcnt + 1;
            end
            if (n_capv) begin
                // 負向控制:第 100 個 byte 故意打壞
                nbuf[ncnt] <= (NEGATIVE_CTRL && ncnt == 100)
                              ? (n_cap ^ 8'h01) : n_cap;
                ncnt       <= ncnt + 1;
            end
        end
    end

    always_ff @(posedge clk) begin
        if (rst) begin
            idx     <= 0;
            errors  <= 0;
            checked <= 0;
        end
        else if (idx < gcnt && idx < ncnt) begin
            if (gbuf[idx] !== nbuf[idx]) begin
                errors <= errors + 1;
                if (!NEGATIVE_CTRL)
                    $display("  [%m] MISMATCH byte %0d: gold=%02x new=%02x",
                             idx, gbuf[idx], nbuf[idx]);
            end
            checked <= checked + 1;
            idx     <= idx + 1;
        end
    end
endmodule


/* ------------------------------------------------------------------ */
/* top                                                                */
/* ------------------------------------------------------------------ */
module tb_tx_equiv;

    logic clk = 1'b0;
    always #5 clk = ~clk;

    logic rst;
    logic trigger;
    logic [4:0] dbg_set;

    logic [31:0] C0 [0:7][0:7];
    logic [31:0] C1 [0:7][0:7];
    logic [31:0] cyc;

    integer i, r, c;
    integer data_txns;
    integer fails;

    tx_equiv_pair #(.DEBUG_MARKERS(1'b0), .CYCLE_COUNTER(1'b1),
                    .NEGATIVE_CTRL(1'b0)) p_clean
        (.clk(clk), .rst(rst), .trigger(trigger), .dbg_set(dbg_set),
         .C0(C0), .C1(C1), .cyc_latched(cyc));

    tx_equiv_pair #(.DEBUG_MARKERS(1'b1), .CYCLE_COUNTER(1'b1),
                    .NEGATIVE_CTRL(1'b0)) p_dbg
        (.clk(clk), .rst(rst), .trigger(trigger), .dbg_set(dbg_set),
         .C0(C0), .C1(C1), .cyc_latched(cyc));

    tx_equiv_pair #(.DEBUG_MARKERS(1'b0), .CYCLE_COUNTER(1'b1),
                    .NEGATIVE_CTRL(1'b1)) p_neg
        (.clk(clk), .rst(rst), .trigger(trigger), .dbg_set(dbg_set),
         .C0(C0), .C1(C1), .cyc_latched(cyc));

    task automatic randomize_data;
        begin
            for (r = 0; r < 8; r = r + 1)
                for (c = 0; c < 8; c = c + 1) begin
                    C0[r][c] = $urandom();
                    C1[r][c] = $urandom();
                end
            cyc = $urandom();
        end
    endtask

    // 一筆交易:換資料 -> 觸發 -> 視情況在 burst 中途丟 marker ->
    // 等到綽綽有餘(516 bytes * ~12 cycles/byte ≈ 6.2k)
    task automatic one_txn(input integer mid_marker);
        begin
            randomize_data();
            @(posedge clk); trigger <= 1'b1;
            @(posedge clk); trigger <= 1'b0;
            if (mid_marker != 0) begin
                repeat (500) @(posedge clk);
                dbg_set <= 5'b00100;      // A3,burst 中途:必須排隊
                @(posedge clk);
                dbg_set <= 5'b0;
            end
            repeat (10000) @(posedge clk);
            data_txns = data_txns + 1;
        end
    endtask

    task automatic report(input integer pair_errors,
                          input integer pair_checked,
                          input integer pair_g,
                          input integer pair_n,
                          input integer expect_min,
                          inout integer fail_acc);
        begin
            if (pair_errors != 0)  fail_acc = fail_acc + 1;
            if (pair_g != pair_n)  fail_acc = fail_acc + 1;
            if (pair_checked < expect_min) fail_acc = fail_acc + 1;
            $display("    errors=%0d checked=%0d gold_bytes=%0d new_bytes=%0d",
                     pair_errors, pair_checked, pair_g, pair_n);
        end
    endtask

    initial begin
        rst       = 1'b1;
        trigger   = 1'b0;
        dbg_set   = 5'b0;
        data_txns = 0;
        fails     = 0;
        randomize_data();

        repeat (10) @(posedge clk);
        rst = 1'b0;
        repeat (5) @(posedge clk);

        // T1: 單筆交易
        $display("T1: single transaction");
        one_txn(0);

        // T2: 連續兩筆(rearm 語意)
        $display("T2: back-to-back transactions");
        one_txn(0);
        one_txn(0);

        // T3: idle 時的 markers(p_dbg 送 A1/A2/A5,其餘 pair 無動作)
        $display("T3: markers while idle");
        dbg_set <= 5'b10011;
        @(posedge clk);
        dbg_set <= 5'b0;
        repeat (500) @(posedge clk);

        // T4: burst 中途 marker(必須等整包送完才出現)
        $display("T4: marker arriving mid-burst");
        one_txn(1);

        // T5: 隨機 soak
        $display("T5: random soak");
        for (i = 0; i < 3; i = i + 1)
            one_txn(i % 2);

        /* ---- scoreboard ---- */
        $display("");
        $display("== p_clean (DEBUG_MARKERS=0, CYCLE_COUNTER=1) ==");
        report(p_clean.errors, p_clean.checked,
               p_clean.gcnt, p_clean.ncnt, data_txns * 516, fails);

        $display("== p_dbg   (DEBUG_MARKERS=1, CYCLE_COUNTER=1) ==");
        report(p_dbg.errors, p_dbg.checked,
               p_dbg.gcnt, p_dbg.ncnt, data_txns * 516 + 4, fails);

        $display("== p_neg   (negative control) ==");
        $display("    errors=%0d (expect >0)", p_neg.errors);
        if (p_neg.errors == 0) begin
            $display("    negative control FAILED to fail -- harness is blind!");
            fails = fails + 1;
        end

        // vacuous-pass 防呆
        if (p_clean.checked == 0 || p_dbg.checked == 0) begin
            $display("    checked==0 -- vacuous run");
            fails = fails + 1;
        end

        $display("");
        if (fails == 0)
            $display("PASS: TX byte sequences identical across %0d transactions",
                     data_txns);
        else
            $display("FAIL: %0d check(s) failed", fails);

        $finish;
    end

endmodule
