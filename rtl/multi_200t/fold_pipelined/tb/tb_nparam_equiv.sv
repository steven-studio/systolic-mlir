/*
 * tb_nparam_equiv -- N 參數化的等價證明。
 *
 * Part 1  N=8 逐 cycle 等價:
 *   golden = repo 現行版 feeder + operand buffer(逐字複製、僅改名,
 *   8 寫死)。DUT = 參數化版,N=8。兩條鏈(buffer -> feeder)餵完全
 *   相同的寫入與 feed 刺激,每一拍比對餵進陣列的所有訊號:
 *   a_in/b_in、valid、accumulator context、以及 raddr 本身。
 *   參數化若正確,N=8 就是恆等變換 -- 一個 bit 都不准動。
 *
 * Part 2  N=4 契約測試:
 *   沒有 golden(舊程式從不支援 N=4),改用 tb 內獨立實作的行為
 *   參考模型(程序式、逐元素,與 RTL 風格刻意不同):鏡射寫入,
 *   逐拍推導「上一拍位址的同步讀 + 延後一拍的 valid/ctx」,比對
 *   DUT 輸出。涵蓋 k_dim = 8/16/24/32 與 skew 邊界。
 *
 * Part 3  N=4 TX 序列化:
 *   tx_source(N=4) + streamer + stub,已知 C0/C1 圖樣,比對送出的
 *   132 bytes(2*64 + 4 cycle counter)與軟體算出的期望序列。
 *
 * 負向控制:Part 1 另掛一組 DUT,其 B 側某一筆寫入資料被故意
 * 打壞 -- 比對器必須抓到,否則整個 harness 視為瞎的,直接 FAIL。
 * 任何部分 checked==0 也直接 FAIL(vacuous-pass 防呆)。
 */
`timescale 1ns / 1ps

module tb_nparam_equiv;

    localparam int KMAX  = 32;
    localparam int KW    = $clog2(KMAX);   // 5
    localparam int FEEDW = 16;
    localparam int KDIMW = 32;

    logic clk = 1'b0;
    always #5 clk = ~clk;
    logic rst;

    /* ---------------- 共用刺激 ---------------- */
    logic             wrA, wrB;
    logic [2:0]       wsel;          // N=8 側用滿 3 bits
    logic [KW-1:0]    waddr;
    logic [31:0]      wdata;

    logic             enable;
    logic [FEEDW-1:0] feed_t;
    logic [KDIMW-1:0] k_dim;

    integer errors1, checked1;
    integer errors1n;                // 負向控制組的錯誤數
    integer errors2, checked2;
    integer errors3, checked3;
    integer i, k, t, b;
    integer fails;

    function automatic [31:0] pat (input integer side,
                                   input integer bank,
                                   input integer kk);
        pat = 32'hC0DE0000 ^ (side * 32'h10000) ^ (bank << 8) ^ kk[7:0];
    endfunction

    /* ================= Part 1: N=8 golden vs param ================= */

    // golden 鏈
    wire [KW-1:0] g_a_raddr [0:7];
    wire [KW-1:0] g_b_raddr [0:7];
    wire [31:0]   g_a_rdata [0:7];
    wire [31:0]   g_b_rdata [0:7];
    wire [31:0]   g_a_in [0:7];
    wire [31:0]   g_b_in [0:7];
    wire          g_a_v [0:7];
    wire          g_b_v [0:7];
    wire          g_ca [0:7];
    wire          g_cb [0:7];

    golden_operand_buffer #(.K_MAX(KMAX), .K_W(KW)) g_abuf (
        .clk(clk), .wr(wrA), .wsel(wsel), .waddr(waddr), .wdata(wdata),
        .raddr(g_a_raddr), .rdata(g_a_rdata));
    golden_operand_buffer #(.K_MAX(KMAX), .K_W(KW)) g_bbuf (
        .clk(clk), .wr(wrB), .wsel(wsel), .waddr(waddr), .wdata(wdata),
        .raddr(g_b_raddr), .rdata(g_b_rdata));

    golden_tile_feeder #(.K_W(KW), .FEED_W(FEEDW), .KDIM_W(KDIMW)) g_feed (
        .clk(clk), .rst(rst), .enable(enable),
        .feed_t(feed_t), .k_dim(k_dim),
        .a_rdata(g_a_rdata), .b_rdata(g_b_rdata),
        .a_raddr(g_a_raddr), .b_raddr(g_b_raddr),
        .a_in(g_a_in), .b_in(g_b_in),
        .a_valid_in(g_a_v), .b_valid_in(g_b_v),
        .accum_ctx_in_a(g_ca), .accum_ctx_in_b(g_cb));

    // 參數化鏈,N=8
    wire [KW-1:0] p_a_raddr [0:7];
    wire [KW-1:0] p_b_raddr [0:7];
    wire [31:0]   p_a_rdata [0:7];
    wire [31:0]   p_b_rdata [0:7];
    wire [31:0]   p_a_in [0:7];
    wire [31:0]   p_b_in [0:7];
    wire          p_a_v [0:7];
    wire          p_b_v [0:7];
    wire          p_ca [0:7];
    wire          p_cb [0:7];

    systolic_operand_buffer #(.K_MAX(KMAX), .K_W(KW), .N_BANKS(8)) p_abuf (
        .clk(clk), .wr(wrA), .wsel(wsel), .waddr(waddr), .wdata(wdata),
        .raddr(p_a_raddr), .rdata(p_a_rdata));
    systolic_operand_buffer #(.K_MAX(KMAX), .K_W(KW), .N_BANKS(8)) p_bbuf (
        .clk(clk), .wr(wrB), .wsel(wsel), .waddr(waddr), .wdata(wdata),
        .raddr(p_b_raddr), .rdata(p_b_rdata));

    systolic_tile_feeder #(.N(8), .K_W(KW), .FEED_W(FEEDW), .KDIM_W(KDIMW)) p_feed (
        .clk(clk), .rst(rst), .enable(enable),
        .feed_t(feed_t), .k_dim(k_dim),
        .a_rdata(p_a_rdata), .b_rdata(p_b_rdata),
        .a_raddr(p_a_raddr), .b_raddr(p_b_raddr),
        .a_in(p_a_in), .b_in(p_b_in),
        .a_valid_in(p_a_v), .b_valid_in(p_b_v),
        .accum_ctx_in_a(p_ca), .accum_ctx_in_b(p_cb));

    // 負向控制:同一條參數化鏈,但 B 側寫入資料在 waddr==7 時打壞
    wire [31:0] nb_wdata = (waddr == KW'(7)) ? (wdata ^ 32'h1) : wdata;
    wire [KW-1:0] n_a_raddr [0:7];
    wire [KW-1:0] n_b_raddr [0:7];
    wire [31:0]   n_a_rdata [0:7];
    wire [31:0]   n_b_rdata [0:7];
    wire [31:0]   n_a_in [0:7];
    wire [31:0]   n_b_in [0:7];
    wire          n_a_v [0:7];
    wire          n_b_v [0:7];
    wire          n_ca [0:7];
    wire          n_cb [0:7];

    systolic_operand_buffer #(.K_MAX(KMAX), .K_W(KW), .N_BANKS(8)) n_abuf (
        .clk(clk), .wr(wrA), .wsel(wsel), .waddr(waddr), .wdata(wdata),
        .raddr(n_a_raddr), .rdata(n_a_rdata));
    systolic_operand_buffer #(.K_MAX(KMAX), .K_W(KW), .N_BANKS(8)) n_bbuf (
        .clk(clk), .wr(wrB), .wsel(wsel), .waddr(waddr), .wdata(nb_wdata),
        .raddr(n_b_raddr), .rdata(n_b_rdata));

    systolic_tile_feeder #(.N(8), .K_W(KW), .FEED_W(FEEDW), .KDIM_W(KDIMW)) n_feed (
        .clk(clk), .rst(rst), .enable(enable),
        .feed_t(feed_t), .k_dim(k_dim),
        .a_rdata(n_a_rdata), .b_rdata(n_b_rdata),
        .a_raddr(n_a_raddr), .b_raddr(n_b_raddr),
        .a_in(n_a_in), .b_in(n_b_in),
        .a_valid_in(n_a_v), .b_valid_in(n_b_v),
        .accum_ctx_in_a(n_ca), .accum_ctx_in_b(n_cb));

    logic cmp1_en;

    always_ff @(posedge clk) begin
        if (rst) begin
            errors1  <= 0;
            checked1 <= 0;
            errors1n <= 0;
        end
        else if (cmp1_en) begin
            checked1 <= checked1 + 1;
            for (int j = 0; j < 8; j++) begin
                if (g_a_raddr[j] !== p_a_raddr[j] ||
                    g_b_raddr[j] !== p_b_raddr[j] ||
                    g_a_in[j]    !== p_a_in[j]    ||
                    g_b_in[j]    !== p_b_in[j]    ||
                    g_a_v[j]     !== p_a_v[j]     ||
                    g_b_v[j]     !== p_b_v[j]     ||
                    g_ca[j]      !== p_ca[j]      ||
                    g_cb[j]      !== p_cb[j]) begin
                    errors1 <= errors1 + 1;
                    $display("  P1 MISMATCH lane %0d t=%0d: a %08x/%08x v %b/%b",
                             j, feed_t, g_a_in[j], p_a_in[j], g_a_v[j], p_a_v[j]);
                end
                // 負向組:只比 b_in(壞的那筆會流到這裡)
                if (g_b_in[j] !== n_b_in[j])
                    errors1n <= errors1n + 1;
            end
        end
    end

    /* ================= Part 2: N=4 vs 行為參考 ================= */

    wire [KW-1:0] q_a_raddr [0:3];
    wire [KW-1:0] q_b_raddr [0:3];
    wire [31:0]   q_a_rdata [0:3];
    wire [31:0]   q_b_rdata [0:3];
    wire [31:0]   q_a_in [0:3];
    wire [31:0]   q_b_in [0:3];
    wire          q_a_v [0:3];
    wire          q_b_v [0:3];
    wire          q_ca [0:3];
    wire          q_cb [0:3];

    // N=4 側的寫入只在 wsel < 4 時鏡射(刺激共用 3-bit wsel)
    wire wrA4 = wrA && (wsel < 3'd4);
    wire wrB4 = wrB && (wsel < 3'd4);

    systolic_operand_buffer #(.K_MAX(KMAX), .K_W(KW), .N_BANKS(4)) q_abuf (
        .clk(clk), .wr(wrA4), .wsel(wsel[1:0]), .waddr(waddr), .wdata(wdata),
        .raddr(q_a_raddr), .rdata(q_a_rdata));
    systolic_operand_buffer #(.K_MAX(KMAX), .K_W(KW), .N_BANKS(4)) q_bbuf (
        .clk(clk), .wr(wrB4), .wsel(wsel[1:0]), .waddr(waddr), .wdata(wdata),
        .raddr(q_b_raddr), .rdata(q_b_rdata));

    systolic_tile_feeder #(.N(4), .K_W(KW), .FEED_W(FEEDW), .KDIM_W(KDIMW)) q_feed (
        .clk(clk), .rst(rst), .enable(enable),
        .feed_t(feed_t), .k_dim(k_dim),
        .a_rdata(q_a_rdata), .b_rdata(q_b_rdata),
        .a_raddr(q_a_raddr), .b_raddr(q_b_raddr),
        .a_in(q_a_in), .b_in(q_b_in),
        .a_valid_in(q_a_v), .b_valid_in(q_b_v),
        .accum_ctx_in_a(q_ca), .accum_ctx_in_b(q_cb));

    // 行為參考:鏡射記憶體 + 「上一拍」期望(同步讀 + 延後一拍 valid)
    logic [31:0] ref_amem [0:3][0:KMAX-1];
    logic [31:0] ref_bmem [0:3][0:KMAX-1];

    logic        exp_av [0:3];
    logic        exp_bv [0:3];
    logic        exp_ca [0:3];
    logic        exp_cb [0:3];
    logic [31:0] exp_ain [0:3];
    logic [31:0] exp_bin [0:3];

    logic cmp2_en;
    integer gk;

    always_ff @(posedge clk) begin
        // 鏡射寫入
        if (wrA4) ref_amem[wsel[1:0]][waddr] <= wdata;
        if (wrB4) ref_bmem[wsel[1:0]][waddr] <= wdata;

        if (rst) begin
            errors2  <= 0;
            checked2 <= 0;
            for (int j = 0; j < 4; j++) begin
                exp_av[j] <= 1'b0; exp_bv[j] <= 1'b0;
                exp_ca[j] <= 1'b0; exp_cb[j] <= 1'b0;
            end
        end
        else begin
            // 先比對「上一拍推導出的期望」
            if (cmp2_en) begin
                checked2 <= checked2 + 1;
                for (int j = 0; j < 4; j++) begin
                    if (q_a_v[j] !== exp_av[j] || q_b_v[j] !== exp_bv[j]) begin
                        errors2 <= errors2 + 1;
                        $display("  P2 valid MISMATCH lane %0d t=%0d: a %b/%b b %b/%b",
                                 j, feed_t, q_a_v[j], exp_av[j], q_b_v[j], exp_bv[j]);
                    end
                    else begin
                        if (exp_av[j] && q_a_in[j] !== exp_ain[j]) begin
                            errors2 <= errors2 + 1;
                            $display("  P2 a_in MISMATCH lane %0d: %08x/%08x",
                                     j, q_a_in[j], exp_ain[j]);
                        end
                        if (exp_bv[j] && q_b_in[j] !== exp_bin[j]) begin
                            errors2 <= errors2 + 1;
                            $display("  P2 b_in MISMATCH lane %0d: %08x/%08x",
                                     j, q_b_in[j], exp_bin[j]);
                        end
                        if (exp_av[j] && q_ca[j] !== exp_ca[j]) begin
                            errors2 <= errors2 + 1;
                            $display("  P2 ctx_a MISMATCH lane %0d", j);
                        end
                        if (exp_bv[j] && q_cb[j] !== exp_cb[j]) begin
                            errors2 <= errors2 + 1;
                            $display("  P2 ctx_b MISMATCH lane %0d", j);
                        end
                    end
                end
            end

            // 再推導「這一拍 -> 下一拍」的期望(獨立實作,程序式)
            for (int j = 0; j < 4; j++) begin
                gk = enable ? (int'(feed_t) - j) : -1;
                if (enable && gk >= 0 && gk < int'(k_dim)) begin
                    exp_av[j]  <= 1'b1;
                    exp_bv[j]  <= 1'b1;
                    exp_ca[j]  <= ((gk >> 3) & 1) ? 1'b1 : 1'b0;
                    exp_cb[j]  <= ((gk >> 3) & 1) ? 1'b1 : 1'b0;
                    exp_ain[j] <= ref_amem[j][gk[KW-1:0]];
                    exp_bin[j] <= ref_bmem[j][gk[KW-1:0]];
                end
                else begin
                    exp_av[j] <= 1'b0;
                    exp_bv[j] <= 1'b0;
                end
            end
        end
    end

    /* ================= Part 3: N=4 TX 序列化 ================= */

    logic [31:0] C0s [0:3][0:3];
    logic [31:0] C1s [0:3][0:3];
    logic [31:0] cycv;

    logic        s_send, s_done;
    logic [4:0]  s_pending;
    logic [4:0]  s_accept;
    logic        s_mv, s_mr;
    logic [7:0]  s_md;
    logic        s_start, s_busy;
    logic [7:0]  s_byte;
    logic [7:0]  s_cap;
    logic        s_capv;
    logic        s_trig;

    always_ff @(posedge clk) begin
        if (rst) begin
            s_pending <= 5'b0;
            s_send    <= 1'b0;
        end
        else begin
            s_pending <= (s_pending & ~s_accept);
            if (s_trig)      s_send <= 1'b1;
            else if (s_done) s_send <= 1'b0;
        end
    end

    systolic_tx_source #(.DEBUG_MARKERS(1'b0), .CYCLE_COUNTER(1'b1), .N(4)) u_src4 (
        .clk(clk), .rst(rst),
        .send_go(s_send), .all_done(s_done),
        .debug_pending(s_pending), .debug_accept(s_accept),
        .C0(C0s), .C1(C1s), .cyc_latched(cycv),
        .m_valid(s_mv), .m_data(s_md), .m_ready(s_mr));

    uart_tx_streamer u_str4 (
        .clk(clk), .rst(rst),
        .s_valid(s_mv), .s_data(s_md), .s_ready(s_mr),
        .tx_start(s_start), .tx_data(s_byte), .tx_busy(s_busy));

    uart_tx_stub u_stub4 (
        .clk(clk), .rst(rst), .start(s_start), .data_in(s_byte),
        .busy(s_busy), .cap_byte(s_cap), .cap_valid(s_capv));

    logic [7:0] cap3 [0:255];
    integer     cap3_n;

    always_ff @(posedge clk) begin
        if (rst) cap3_n <= 0;
        else if (s_capv) begin
            cap3[cap3_n] <= s_cap;
            cap3_n       <= cap3_n + 1;
        end
    end

    logic [31:0] w3;

    /* ================= driver ================= */
    initial begin
        rst = 1'b1;
        wrA = 0; wrB = 0; wsel = '0; waddr = '0; wdata = '0;
        enable = 0; feed_t = '0; k_dim = KDIMW'(KMAX);
        cmp1_en = 0; cmp2_en = 0; s_trig = 0;
        errors3 = 0; checked3 = 0; fails = 0;
        cycv = 32'h11223344;
        for (i = 0; i < 4; i = i + 1)
            for (k = 0; k < 4; k = k + 1) begin
                C0s[i][k] = 32'hA0000000 + (i << 8) + k;
                C1s[i][k] = 32'hB0000000 + (i << 8) + k;
            end
        repeat (5) @(posedge clk);
        rst = 1'b0;
        repeat (2) @(posedge clk);

        /* ---- 寫滿所有 bank(A、B 各 8 bank x 32 深) ---- */
        for (b = 0; b < 8; b = b + 1)
            for (k = 0; k < KMAX; k = k + 1) begin
                @(posedge clk);
                wrA   <= 1'b1;
                wsel  <= 3'(b);
                waddr <= KW'(k);
                wdata <= pat(0, b, k);
            end
        @(posedge clk); wrA <= 1'b0;
        for (b = 0; b < 8; b = b + 1)
            for (k = 0; k < KMAX; k = k + 1) begin
                @(posedge clk);
                wrB   <= 1'b1;
                wsel  <= 3'(b);
                waddr <= KW'(k);
                wdata <= pat(1, b, k);
            end
        @(posedge clk); wrB <= 1'b0;
        repeat (4) @(posedge clk);

        /* ---- feed 交易:k_dim 掃 8/16/24/32 ---- */
        cmp1_en = 1; cmp2_en = 1;
        for (i = 8; i <= KMAX; i = i + 8) begin
            k_dim <= KDIMW'(i);
            @(posedge clk);
            enable <= 1'b1; feed_t <= '0;
            // 跑到 k_dim + 6(涵蓋兩種 N 的邊界:N=8 用滿,N=4 提早結束)
            for (t = 0; t <= i + 6; t = t + 1) begin
                @(posedge clk);
                feed_t <= feed_t + 1'b1;
            end
            enable <= 1'b0;
            repeat (6) @(posedge clk);   // 讓延後一拍的尾巴流完
        end
        cmp1_en = 0; cmp2_en = 0;

        /* ---- Part 3:N=4 TX ---- */
        @(posedge clk); s_trig <= 1'b1;
        @(posedge clk); s_trig <= 1'b0;
        repeat (3000) @(posedge clk);

        // 期望序列:C0 row-major、C1 row-major、cyc,全 little-endian
        begin
            integer idx;
            idx = 0;
            for (i = 0; i < 4; i = i + 1)
                for (k = 0; k < 4; k = k + 1) begin
                    w3 = C0s[i][k];
                    for (b = 0; b < 4; b = b + 1) begin
                        if (cap3[idx] !== w3[b*8 +: 8]) errors3 = errors3 + 1;
                        checked3 = checked3 + 1;
                        idx = idx + 1;
                    end
                end
            for (i = 0; i < 4; i = i + 1)
                for (k = 0; k < 4; k = k + 1) begin
                    w3 = C1s[i][k];
                    for (b = 0; b < 4; b = b + 1) begin
                        if (cap3[idx] !== w3[b*8 +: 8]) errors3 = errors3 + 1;
                        checked3 = checked3 + 1;
                        idx = idx + 1;
                    end
                end
            for (b = 0; b < 4; b = b + 1) begin
                if (cap3[idx] !== cycv[b*8 +: 8]) errors3 = errors3 + 1;
                checked3 = checked3 + 1;
                idx = idx + 1;
            end
            if (cap3_n !== idx) begin
                $display("  P3 byte count %0d != expected %0d", cap3_n, idx);
                errors3 = errors3 + 1;
            end
        end

        /* ---- scoreboard ---- */
        $display("");
        $display("P1 N=8 lockstep : errors=%0d checked=%0d cycles", errors1, checked1);
        $display("P1 negative     : errors=%0d (expect >0)", errors1n);
        $display("P2 N=4 contract : errors=%0d checked=%0d cycles", errors2, checked2);
        $display("P3 N=4 TX bytes : errors=%0d checked=%0d bytes (cap=%0d)",
                 errors3, checked3, cap3_n);

        if (errors1 != 0)  fails = fails + 1;
        if (errors1n == 0) fails = fails + 1;   // harness 必須抓得到錯
        if (errors2 != 0)  fails = fails + 1;
        if (errors3 != 0)  fails = fails + 1;
        if (checked1 == 0 || checked2 == 0 || checked3 == 0) fails = fails + 1;

        $display("");
        if (fails == 0) $display("PASS: N-parameterisation is identity at N=8, contract-correct at N=4");
        else            $display("FAIL: %0d check(s) failed", fails);
        $finish;
    end

endmodule
