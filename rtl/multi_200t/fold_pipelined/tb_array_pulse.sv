`timescale 1ns/1ps
/*
 * tb_array_pulse -- systolic_array_tile 的單元測試
 *
 * 這支測試存在的理由,是 PE 從「持續拉高的 result_valid」改成
 * 「一拍脈衝的 acc_valid_out」。那是語意改變,編譯器抓不到,
 * 所以要用測試把它釘住。
 *
 * 驗四件事:
 *   1. 算術正確        C = A x B,逐位元相同
 *   2. 恰好一拍        每次交易 c_valid_out 只脈衝一次(不是兩次)
 *   3. 結果會被抱住    脈衝過後晾著,c_out 不能變
 *                      (這是對 PE acc_out 的跨模組約定)
 *   4. 可以連做兩次    第二次交易的答案不能被第一次污染
 *
 * 第 4 項是抓到 PE 清零 bug 的同一個手法。
 *
 * 預設 N=2、K=4 —— 小到可以手算,也小到 verilator 秒跑完。
 * N/K 是參數,-GN=4 -GK=8 可以換更大的幾何再跑一次。
 *
 *
 * Verilator 的一個坑(踩過了,寫在這裡免得再踩)
 * ----------------------------------------------
 * Verilator 5.020 不會把「在 initial 區塊裡用程序指派驅動的
 * unpacked array」傳進子模組。埠上讀得到值(dut.a_valid_in[0] 是 1),
 * 但子模組內部的組合邏輯不會重新求值,一路都是 0。
 *
 * 最小重現:一個只有 assign o[i] = v_in[i] 的模組,v_in 由 initial
 * 驅動 -> o 恆為 0;同一個模組改由 always_ff 驅動 -> 正確。
 *
 * 所以這裡的做法是:initial 只推進「純量」的餵料節拍 feed_t /
 * feed_tx,真正的陣列埠一律由 always_ff 驅動。純量從 initial
 * 傳出去是正常的。
 *
 * 這是模擬器的限制,不是設計的問題 —— 舊版 array_tile 的埠型別
 * 完全一樣,拿它來跑會遇到同一件事。
 *
 *
 *   verilator --binary -Wall -Wno-fatal --top-module tb_array_pulse \
 *       tb/fp_model.sv systolic_pe_tile.sv systolic_array_tile.sv \
 *       tb/tb_array_pulse.sv
 */
module tb_array_pulse;

    /* N 與 K 可以從命令列覆寫:verilator -GN=4 -GK=8 */
    parameter int N = 2;
    parameter int K = 4;

    logic clk = 0;
    always #5 clk = ~clk;

    logic rst;

    logic [31:0] a_in [0:N-1];
    logic [31:0] b_in [0:N-1];
    logic        a_valid_in [0:N-1];
    logic        b_valid_in [0:N-1];

    logic        c_valid_out;
    logic [31:0] c_out [0:N-1][0:N-1];

    systolic_array_tile #(.N(N), .DATA_W(32)) dut (
        .clk(clk), .rst(rst),
        .a_in(a_in), .b_in(b_in),
        .a_valid_in(a_valid_in), .b_valid_in(b_valid_in),
        .c_valid_out(c_valid_out), .c_out(c_out)
    );

    /* ---------------- 刺激與期望值 ---------------- */

    logic [31:0] Amat [0:1][0:N-1][0:K-1];
    logic [31:0] Bmat [0:1][0:K-1][0:N-1];
    logic [31:0] Cexp [0:1][0:N-1][0:N-1];

    /*
     * 餵料節拍。純量,由 initial 推進。
     *   feed_t  = -1 代表這一拍不餵任何東西
     *   feed_tx = 現在在做第幾次交易
     */
    int feed_t  = -1;
    int feed_tx = 0;

    /*
     * 收縮式餵料 —— 一定要用 always_ff,見檔頭的 Verilator 註記。
     *
     *   a_in[r] 在第 t 拍送 A[r][t-r]
     *   b_in[c] 在第 t 拍送 B[t-c][c]
     *
     * 這樣 A[r][k] 與 B[k][c] 會剛好在 PE(r,c) 相遇:
     *   A[r][k] 走 c 步到達,B[k][c] 走 r 步到達,
     *   (k+r)+c == (k+c)+r。
     */
    always_ff @(posedge clk) begin
        for (int r = 0; r < N; r++) begin
            if (feed_t >= r && (feed_t - r) < K) begin
                a_valid_in[r] <= 1'b1;
                a_in[r]       <= Amat[feed_tx][r][feed_t - r];
            end
            else begin
                a_valid_in[r] <= 1'b0;
                a_in[r]       <= 32'd0;
            end
        end
        for (int c = 0; c < N; c++) begin
            if (feed_t >= c && (feed_t - c) < K) begin
                b_valid_in[c] <= 1'b1;
                b_in[c]       <= Bmat[feed_tx][feed_t - c][c];
            end
            else begin
                b_valid_in[c] <= 1'b0;
                b_in[c]       <= 32'd0;
            end
        end
    end

    /* 脈衝計數器 —— 第 2 項就靠它 */
    int pulse_count = 0;
    always @(posedge clk)
        if (!rst && c_valid_out) pulse_count <= pulse_count + 1;

    int errors = 0;
    int checks = 0;

    task automatic chk(input string what, input int got, input int want);
        checks++;
        if (got !== want) begin
            errors++;
            $display("  [FAIL] %-38s got %0d  want %0d", what, got, want);
        end
    endtask

    logic [31:0] snapshot [0:N-1][0:N-1];

    int tx, t, i, j, guard;

    initial begin
        /*
         * 刺激用可以任意 N/K 展開的規則產生:
         *   交易 0   A[r][k] = r*K + k + 1     B[k][c] = k*N + c + 1
         *   交易 1   A[r][k] = r + 2           B[k][c] = 1
         *
         * 期望值由這裡的參考三重迴圈算出來 —— 它跟 DUT 完全無關,
         * 是獨立的第二份實作,不是把 DUT 的邏輯抄一遍。
         */
        for (i = 0; i < N; i++)
            for (j = 0; j < K; j++) begin
                Amat[0][i][j] = 32'(i * K + j + 1);
                Amat[1][i][j] = 32'(i + 2);
            end
        for (i = 0; i < K; i++)
            for (j = 0; j < N; j++) begin
                Bmat[0][i][j] = 32'(i * N + j + 1);
                Bmat[1][i][j] = 32'd1;
            end

        for (tx = 0; tx < 2; tx++)
            for (i = 0; i < N; i++)
                for (j = 0; j < N; j++) begin
                    Cexp[tx][i][j] = 32'd0;
                    for (int kk = 0; kk < K; kk++)
                        Cexp[tx][i][j] = Cexp[tx][i][j]
                                       + Amat[tx][i][kk] * Bmat[tx][kk][j];
                end

        /*
         * 錨點:N=2、K=4 這一組我用手算過,寫在這裡驗證上面那個
         * 參考迴圈本身沒有寫錯。
         *
         *   A = [1 2 3 4]   B = [1 2]      C = [ 50  60]
         *       [5 6 7 8]       [3 4]          [114 140]
         *                       [5 6]
         *                       [7 8]
         */
        if (N == 2 && K == 4) begin
            chk("錨點 C[0][0]", int'(Cexp[0][0][0]), 50);
            chk("錨點 C[0][1]", int'(Cexp[0][0][1]), 60);
            chk("錨點 C[1][0]", int'(Cexp[0][1][0]), 114);
            chk("錨點 C[1][1]", int'(Cexp[0][1][1]), 140);
            chk("錨點 tx1 C[0][0]", int'(Cexp[1][0][0]), 8);
            chk("錨點 tx1 C[1][0]", int'(Cexp[1][1][0]), 12);
        end

        rst     = 1;
        feed_t  = -1;
        feed_tx = 0;
        repeat (4) @(negedge clk);
        rst = 0;

        for (tx = 0; tx < 2; tx++) begin

            $display("--- 交易 %0d ---", tx);

            feed_tx = tx;
            for (t = 0; t < K + N - 1; t++) begin
                feed_t = t;
                @(negedge clk);
            end
            feed_t = -1;
            @(negedge clk);

            /* 等發佈 */
            guard = 0;
            while (!c_valid_out && guard < 400) begin
                @(negedge clk);
                guard++;
            end
            if (guard >= 400) begin
                $display("  [FAIL] 交易 %0d 逾時,沒有等到 c_valid_out", tx);
                errors++;
                $finish;
            end

            /* 1. 算術 */
            for (i = 0; i < N; i++)
                for (j = 0; j < N; j++)
                    chk($sformatf("tx%0d c[%0d][%0d]", tx, i, j),
                        int'(c_out[i][j]), int'(Cexp[tx][i][j]));

            /* 3. 抱住:先拍一張快照 */
            for (i = 0; i < N; i++)
                for (j = 0; j < N; j++)
                    snapshot[i][j] = c_out[i][j];

            /* 2. 恰好一拍:等 60 拍,期間不能再有脈衝 */
            repeat (60) @(negedge clk);
            chk($sformatf("tx%0d 累計脈衝數", tx), pulse_count, tx + 1);

            /* 3. 抱住:60 拍之後值必須沒變 */
            for (i = 0; i < N; i++)
                for (j = 0; j < N; j++)
                    chk($sformatf("tx%0d c[%0d][%0d] 60 拍後仍相同", tx, i, j),
                        int'(c_out[i][j]), int'(snapshot[i][j]));
        end

        $display("");
        $display("=========================================");
        if (errors == 0)
            $display("  tb_array_pulse:  %0d 項全部通過", checks);
        else
            $display("  tb_array_pulse:  %0d / %0d 項失敗", errors, checks);
        $display("=========================================");
        $finish;
    end

endmodule
