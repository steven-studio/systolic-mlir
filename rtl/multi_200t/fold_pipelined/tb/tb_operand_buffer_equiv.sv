`timescale 1ns/1ps

/*
 * ============================================================
 * tb_operand_buffer_equiv -- 重構前後的等價性證明
 * ============================================================
 *
 * tb_feeder_buffer 驗的是「契約」:抽出來的模組合不合乎我們想要
 * 的語意。它答不了重構真正該答的問題 —— 新的東西跟舊的東西是不
 * 是同一個電路。契約寫錯的時候,契約測試會跟著錯一起通過。
 *
 * 這裡把舊頂層裡的 A_MEM / B_MEM generate 區塊逐字複製成 golden,
 * 與 systolic_operand_buffer 的兩個實例並排,餵完全相同的刺激,
 * 每一拍比對八個 bank 的輸出。任何一拍任何一個 bank 不同就 FAIL。
 *
 * 刺激分兩段:
 *   1. 完整的 RX 掃描 —— 照真實 framing 的順序把每個 window 的
 *      A、B 各 64 個字寫滿,同時隨機讀。位址解碼若有偏移,這段抓得到。
 *   2. 隨機寫 + 隨機讀 —— 涵蓋 write/read 同位址、bank 交錯等
 *      順序性行為。
 *
 * 用 !== 比較,所以未初始化的 X 在兩邊都是 X 時算相同,不會在
 * 寫滿之前誤報。
 *
 * 語法刻意保守:只用 tb_feeder_buffer 已經在 xsim 編過的構造
 * (int 變數的 bit-select、sized literal),不用 (expr)'(...) 這種
 * 括號寬度轉型,也不用 string 引數 —— xvlog 對這些的支援比標準弱,
 * 而這個 bench 的價值就在於它到哪裡都跑得起來。
 * ============================================================
 */
module tb_operand_buffer_equiv #(
    parameter int K_MAX = 64
);

    localparam int K_W      = $clog2(K_MAX);
    localparam int WIN_W    = K_W - 3;          // window 欄位寬度
    localparam int N_WIN    = K_MAX / 8;

    logic clk = 1'b0;
    always #5 clk = ~clk;

    logic            buf_wr    = 1'b0;
    logic            rx_is_b   = 1'b0;
    logic [2:0]      rx_row    = '0;
    logic [2:0]      rx_col    = '0;
    logic [WIN_W-1:0] rx_win   = '0;
    logic [31:0]     buf_wdata = '0;

    wire [K_W-1:0] a_waddr = {rx_win, rx_col};
    wire [K_W-1:0] b_waddr = {rx_win, rx_row};

    logic [K_W-1:0] a_raddr [0:7];
    logic [K_W-1:0] b_raddr [0:7];


    /*
     * ------------------------------------------------------------
     * GOLDEN -- 自重構前的 systolic_uart_tile_top 逐字複製
     * ------------------------------------------------------------
     */
    wire [31:0] a_rdata_g [0:7];
    wire [31:0] b_rdata_g [0:7];

    genvar gi;

    generate

        for (gi = 0; gi < 8; gi = gi + 1) begin : A_MEM

            logic [31:0] mem [0:K_MAX-1];
            logic [31:0] rdata_q;

            always_ff @(posedge clk) begin
                if (buf_wr && !rx_is_b && rx_row == 3'(unsigned'(gi)))
                    mem[a_waddr] <= buf_wdata;

                rdata_q <= mem[a_raddr[gi]];
            end

            assign a_rdata_g[gi] = rdata_q;

        end

        for (gi = 0; gi < 8; gi = gi + 1) begin : B_MEM

            logic [31:0] mem [0:K_MAX-1];
            logic [31:0] rdata_q;

            always_ff @(posedge clk) begin
                if (buf_wr && rx_is_b && rx_col == 3'(unsigned'(gi)))
                    mem[b_waddr] <= buf_wdata;

                rdata_q <= mem[b_raddr[gi]];
            end

            assign b_rdata_g[gi] = rdata_q;

        end

    endgenerate


    /*
     * ------------------------------------------------------------
     * DUT -- 重構後的兩個實例,轉置寫在埠上
     * ------------------------------------------------------------
     */
    wire [31:0] a_rdata_d [0:7];
    wire [31:0] b_rdata_d [0:7];

    systolic_operand_buffer #(.K_MAX(K_MAX), .K_W(K_W)) u_a_buf (
        .clk   (clk),
        .wr    (buf_wr && !rx_is_b),
        .wsel  (rx_row),
        .waddr (a_waddr),
        .wdata (buf_wdata),
        .raddr (a_raddr),
        .rdata (a_rdata_d)
    );

    systolic_operand_buffer #(.K_MAX(K_MAX), .K_W(K_W)) u_b_buf (
        .clk   (clk),
        .wr    (buf_wr && rx_is_b),
        .wsel  (rx_col),
        .waddr (b_waddr),
        .wdata (buf_wdata),
        .raddr (b_raddr),
        .rdata (b_rdata_d)
    );


    int errors  = 0;
    int checked = 0;

    // 失敗訊息用的當下座標,取代 string 引數。
    int dbg_phase = 0;      // 0 = RX 掃描, 1 = 隨機
    int dbg_t     = 0;

    task automatic compare();
        for (int i = 0; i < 8; i++) begin
            checked = checked + 2;
            if (a_rdata_d[i] !== a_rdata_g[i]) begin
                $display("phase=%0d t=%0d A bank %0d: golden %08h  dut %08h",
                         dbg_phase, dbg_t, i, a_rdata_g[i], a_rdata_d[i]);
                errors++;
            end
            if (b_rdata_d[i] !== b_rdata_g[i]) begin
                $display("phase=%0d t=%0d B bank %0d: golden %08h  dut %08h",
                         dbg_phase, dbg_t, i, b_rdata_g[i], b_rdata_d[i]);
                errors++;
            end
        end
    endtask

    task automatic rand_raddr();
        int ra, rb;
        for (int i = 0; i < 8; i++) begin
            ra = $urandom_range(0, K_MAX-1);
            rb = $urandom_range(0, K_MAX-1);
            a_raddr[i] = ra[K_W-1:0];
            b_raddr[i] = rb[K_W-1:0];
        end
    endtask


    initial begin

        for (int i = 0; i < 8; i++) begin
            a_raddr[i] = '0;
            b_raddr[i] = '0;
        end

        @(negedge clk);

        /* ---- 第一段:照真實 RX 順序寫滿,同時隨機讀 ---- */
        dbg_phase = 0;
        for (int w = 0; w < N_WIN; w++) begin
            for (int m = 0; m < 2; m++) begin
                for (int r = 0; r < 8; r++) begin
                    for (int c = 0; c < 8; c++) begin
                        buf_wr    = 1'b1;
                        rx_is_b   = m[0];
                        rx_win    = w[WIN_W-1:0];
                        rx_row    = r[2:0];
                        rx_col    = c[2:0];
                        buf_wdata = $urandom;
                        dbg_t     = ((w*2 + m)*8 + r)*8 + c;
                        rand_raddr();
                        @(posedge clk);
                        #1 compare();
                        @(negedge clk);
                    end
                end
            end
        end

        buf_wr = 1'b0;

        /* ---- 第二段:隨機寫 + 隨機讀 ---- */
        dbg_phase = 1;
        begin
            int rw, rb, rr, rc, rwin;
            for (int t = 0; t < 4000; t++) begin
                rw   = $urandom_range(0, 1);
                rb   = $urandom_range(0, 1);
                rwin = $urandom_range(0, N_WIN-1);
                rr   = $urandom_range(0, 7);
                rc   = $urandom_range(0, 7);

                buf_wr    = rw[0];
                rx_is_b   = rb[0];
                rx_win    = rwin[WIN_W-1:0];
                rx_row    = rr[2:0];
                rx_col    = rc[2:0];
                buf_wdata = $urandom;
                dbg_t     = t;

                rand_raddr();
                @(posedge clk);
                #1 compare();
                @(negedge clk);
            end
        end

        if (checked == 0) begin
            $display("FAIL: checked == 0,沒有比對到任何一拍");
            errors++;
        end

        $display("K_MAX = %0d  K_W = %0d  比對 %0d 次", K_MAX, K_W, checked);
        if (errors == 0)
            $display("PASS: 重構前後逐拍等價");
        else
            $display("FAIL: %0d 處不等價", errors);

        $finish;
    end

endmodule
