`timescale 1ns/1ps
/*
 * tb_array_h_decomp -- H 是由哪幾段拍數組成的?
 *
 * 第 4 章 eq:H-decomp 把每次 invocation 的固定成本 H 拆成排空 g、
 * 歸約樹 r、交棒 c 三段。板子上的計數器只量得到總和
 * (T = k + 2(N-1) + H),三段各自幾拍先前只能從 RTL 推。這支 bench
 * 把「推」換成「量」:在 PE(N-1,N-1) 與陣列的輸出控制器上打時間戳,
 * 把 H 的每一拍歸屬到一個明確的狀態轉移。
 *
 * 計數區間刻意與 systolic_uart_top 的 cyc_count 一致:
 *   起點 = feed_t == 0 那一拍(feeder 送出 BRAM 位址,運算元下一拍
 *          才進陣列)
 *   終點 = c_valid_out 那一拍
 *   兩端皆含 → T = 終點 - 起點 + 1
 *
 * 時間戳(全部是「那一拍」的 cycle 編號,在 negedge 取樣):
 *   t_last   最後一組運算元抵達 PE(N-1,N-1) 的輸入埠
 *   t_hand   acc_handoff 成立(累加側排空、翻 set、歸約開始)
 *   t_bar[i] 第 i 層樹的 barrier(todo==0 && busy==0 && !read_valid)
 *   t_done   red_state == RED_DONE
 *   t_pulse  PE 的 acc_valid_out
 *   t_arr    陣列 all_arrived
 *   t_pub    陣列 OUT_CAN_PUBLISH(= clear_arrived)
 *   t_cv     c_valid_out(= 終點)
 *
 * 歸屬:
 *   g = t_hand - t_last        排空:輸入暫存 1 + fp_mul + BRAM 讀 1
 *                              + fp_add + busy 歸零 1
 *   r = t_bar[3] - t_hand      四層樹:每層 = 發射數 + BRAM 讀 1
 *                              + fp_add + busy 歸零 1
 *   c = t_cv - t_bar[3] + 1    RED_DONE 1 + acc_valid_out 1 + all_arrived 1
 *                              + CAN_PUBLISH 1 + c_valid_out 1,再加起點
 *                              那一拍的 operand-buffer 同步讀 1
 *   g + r + c 必須等於 H = T - k - 2(N-1),否則 FAIL。
 *
 * 浮點 IP 的延遲從 ip/fp32 底下的 .xci 讀:C_Latency = 8 (mul)、11 (add)。
 * fp_model.sv 的預設 9/12 是 PE 註解裡的名目值,不是 IP 的設定;在這
 * 支 bench 裡 LAT 決定答案,所以要從命令列明確給:
 *
 *   mkdir -p sim_out/h_decomp
 *   verilator --binary -Wno-fatal --top-module tb_array_h_decomp \
 *       -DFP_MUL_LAT=8 -DFP_ADD_LAT=11 -GN=8 -GK=8 \
 *       tb/fp_model.sv core/systolic_pe_bram.sv core/systolic_array.sv \
 *       tb/tb_array_h_decomp.sv -o tb_h --Mdir sim_out/h_decomp \
 *   && sim_out/h_decomp/tb_h
 *
 *   Icarus 也行(-P 改參數):
 *   iverilog -g2012 -DFP_MUL_LAT=8 -DFP_ADD_LAT=11 \
 *       -Ptb_array_h_decomp.N=8 -Ptb_array_h_decomp.K=8 \
 *       -s tb_array_h_decomp -o sim_out/tb_h \
 *       tb/fp_model.sv core/systolic_pe_bram.sv core/systolic_array.sv \
 *       tb/tb_array_h_decomp.sv && vvp -n sim_out/tb_h
 *
 * 8/11 下 N=8 給 T(8)=117、T(64)=173、T(256)=365,N=4 給 T(8)=109,
 * 都是 22 + 67 + 6 = 95,與板測逐拍相同。用 9/12 跑會得到
 * 24 + 71 + 6 = 101,對不上板測的 95 —— 這是 IP 延遲是 8/11 而不是
 * 9/12 的旁證(正證是 .xci 本身)。
 *
 * 陣列埠一律由 always_ff 驅動、initial 只推進純量 feed_t,理由見
 * tb_array_pulse 檔頭的 Verilator 註記。
 */
module tb_array_h_decomp;

    parameter int N   = 8;
    parameter int K   = 8;
    parameter int NTX = 2;      // 連做幾次交易(第二次驗證交棒後狀態乾淨)

    localparam int L = N - 1;   // 最後一個 PE 的座標 (L, L)

    logic clk = 0;
    always #5 clk = ~clk;

    logic rst;

    logic [31:0] a_in [0:N-1];
    logic [31:0] b_in [0:N-1];
    logic        a_valid_in [0:N-1];
    logic        b_valid_in [0:N-1];

    logic        c_valid_out;
    logic [31:0] c_out [0:N-1][0:N-1];

    systolic_array #(.N(N), .DATA_W(32)) dut (
        .clk(clk), .rst(rst),
        .a_in(a_in), .b_in(b_in),
        .a_valid_in(a_valid_in), .b_valid_in(b_valid_in),
        .c_valid_out(c_valid_out), .c_out(c_out)
    );

    /* ---------------- 刺激與期望值 ---------------- */

    logic [31:0] Amat [0:N-1][0:K-1];
    logic [31:0] Bmat [0:K-1][0:N-1];
    logic [31:0] Cexp [0:N-1][0:N-1];

    int feed_t = -1;

    /* 收縮式餵料,與 systolic_tile_feeder 同一個時序:feed_t 在第 t 拍
     * 算位址,運算元第 t+1 拍進陣列;列 r 延後 r 拍、行 c 延後 c 拍。 */
    always_ff @(posedge clk) begin
        for (int r = 0; r < N; r++) begin
            if (feed_t >= r && (feed_t - r) < K) begin
                a_valid_in[r] <= 1'b1;
                a_in[r]       <= Amat[r][feed_t - r];
            end
            else begin
                a_valid_in[r] <= 1'b0;
                a_in[r]       <= 32'd0;
            end
        end
        for (int c = 0; c < N; c++) begin
            if (feed_t >= c && (feed_t - c) < K) begin
                b_valid_in[c] <= 1'b1;
                b_in[c]       <= Bmat[feed_t - c][c];
            end
            else begin
                b_valid_in[c] <= 1'b0;
                b_in[c]       <= 32'd0;
            end
        end
    end

    /* ---------------- 時間戳 ---------------- */

    int cyc = 0;
    always_ff @(posedge clk) cyc <= cyc + 1;

    int t_start, t_last, t_hand, t_done, t_pulse, t_arr, t_pub, t_cv;
    int t_bar [0:3];
    int n_bar;

    task automatic clear_stamps();
        t_start = -1; t_last = -1; t_hand = -1; t_done = -1;
        t_pulse = -1; t_arr = -1;  t_pub = -1;  t_cv = -1;
        for (int i = 0; i < 4; i++) t_bar[i] = -1;
        n_bar = 0;
    endtask

    /* 歸約樹的層間 barrier,條件逐字抄自 systolic_pe_bram 的 RED_RUN:
     * 沒事做 + 管線空了。red_state 的編碼是 IDLE=0 / RUN=1 / DONE=2。 */
    wire pe_bar = (dut.ROW[L].COL[L].u_pe.red_state == 2'd1)
               && (dut.ROW[L].COL[L].u_pe.reduce_todo == '0)
               && (dut.ROW[L].COL[L].u_pe.reduce_add_busy == '0)
               && !dut.ROW[L].COL[L].u_pe.reduce_read_valid;

    /* 每個戳記的條件都先讀自己一次(t_x < 0)。Verilator 5.020 會把一個
     * 「在這個 always 裡只寫不讀」的戳記變數整個優化掉 —— initial 那邊
     * 讀到的永遠是 -1,而且沒有任何警告。踩過了,寫在這裡。 */
    logic pair_d = 1'b0;

    always @(negedge clk) begin
        if (!rst) begin
            /* 運算元串流在輸入暫存級(a_reg/b_reg)結束的那一拍:pair_d 高、
             * pipe_pair_valid 已低。最後一組在暫存級是 cyc-1、在輸入埠是
             * cyc-2;t_last 記輸入埠,與頂層 k + 2(N-1) 的算法對齊。 */
            pair_d <= dut.ROW[L].COL[L].u_pe.pipe_pair_valid;
            if (pair_d && !dut.ROW[L].COL[L].u_pe.pipe_pair_valid && t_last < 0)
                t_last <= cyc - 2;

            if (dut.ROW[L].COL[L].u_pe.acc_handoff && t_hand < 0)       t_hand  <= cyc;
            if (pe_bar && n_bar < 4) begin
                t_bar[n_bar] <= cyc;
                n_bar        <= n_bar + 1;
            end
            if (dut.ROW[L].COL[L].u_pe.red_state == 2'd2 && t_done < 0) t_done  <= cyc;
            if (dut.ROW[L].COL[L].u_pe.acc_valid_out && t_pulse < 0)    t_pulse <= cyc;
            if (dut.all_arrived   && t_arr < 0)                          t_arr   <= cyc;
            if (dut.clear_arrived && t_pub < 0)                          t_pub   <= cyc;
            if (c_valid_out       && t_cv  < 0)                          t_cv    <= cyc;
        end
    end

    /* ---------------- 主流程 ---------------- */

    int errors = 0;
    int checks = 0;

    task automatic chk(input string what, input int got, input int want);
        checks++;
        if (got !== want) begin
            errors++;
            $display("  [FAIL] %-40s got %0d  want %0d", what, got, want);
        end
    endtask

    int tx, t, i, j, guard;
    int T, H, g, r, c;

    initial begin
        /* 小整數,fp_model 用整數算術,結果逐位元可比。 */
        for (i = 0; i < N; i++)
            for (j = 0; j < K; j++)
                Amat[i][j] = 32'(i + j + 1);
        for (i = 0; i < K; i++)
            for (j = 0; j < N; j++)
                Bmat[i][j] = 32'(i * N + j + 1);
        for (i = 0; i < N; i++)
            for (j = 0; j < N; j++) begin
                Cexp[i][j] = 32'd0;
                for (int kk = 0; kk < K; kk++)
                    Cexp[i][j] = Cexp[i][j] + Amat[i][kk] * Bmat[kk][j];
            end

        $display("tb_array_h_decomp  N=%0d  K=%0d  fp_mul LAT=%0d  fp_add LAT=%0d",
                 N, K,
                 dut.ROW[0].COL[0].u_pe.u_fp_mul.LAT,
                 dut.ROW[0].COL[0].u_pe.u_fp_add_accum.LAT);

        rst    = 1;
        feed_t = -1;
        repeat (4) @(negedge clk);
        rst = 0;
        repeat (2) @(negedge clk);

        for (tx = 0; tx < NTX; tx++) begin
            clear_stamps();

            /* 起點:feed_t == 0 的那一拍。此刻 cyc 就是頂層
             * cyc_count 起算的那一拍(運算元要到下一拍才進陣列)。 */
            for (t = 0; t < K + N - 1; t++) begin
                feed_t = t;
                if (t == 0) t_start = cyc;
                @(negedge clk);
            end
            feed_t = -1;

            guard = 0;
            while (t_cv < 0 && guard < 4000) begin
                @(negedge clk);
                guard++;
            end
            if (t_cv < 0) begin
                $display("  [FAIL] 交易 %0d 逾時,沒有等到 c_valid_out", tx);
                errors++;
                $finish;
            end

            /* 從 dut.pe_acc 讀而不是 c_out:c_out 只是它的 assign,而 Icarus
             * 不會把二維 unpacked array 的輸出埠傳出來(讀到 x)。這樣同一支
             * bench 在 Verilator 與 Icarus 下都能跑。 */
            for (i = 0; i < N; i++)
                for (j = 0; j < N; j++)
                    chk($sformatf("tx%0d c[%0d][%0d]", tx, i, j),
                        int'(dut.pe_acc[i][j]), int'(Cexp[i][j]));

            T = t_cv - t_start + 1;
            H = T - K - 2 * (N - 1);
            g = t_hand   - t_last;
            r = t_bar[3] - t_hand;
            c = t_cv     - t_bar[3] + 1;

            $display("");
            $display("--- 交易 %0d ---", tx);
            $display("  起點 feed_t=0        cyc %0d", t_start);
            $display("  最後運算元到 PE(%0d,%0d) cyc %0d   (= 起點 + k + 2(N-1) = %0d)",
                     L, L, t_last, t_start + K + 2 * (N - 1));
            $display("  acc_handoff          cyc %0d   g = %0d", t_hand, g);
            $display("  樹 barrier stride 8  cyc %0d   層 = %0d", t_bar[0], t_bar[0] - t_hand);
            $display("  樹 barrier stride 4  cyc %0d   層 = %0d", t_bar[1], t_bar[1] - t_bar[0]);
            $display("  樹 barrier stride 2  cyc %0d   層 = %0d", t_bar[2], t_bar[2] - t_bar[1]);
            $display("  樹 barrier stride 1  cyc %0d   層 = %0d   r = %0d",
                     t_bar[3], t_bar[3] - t_bar[2], r);
            $display("  RED_DONE             cyc %0d", t_done);
            $display("  acc_valid_out        cyc %0d", t_pulse);
            $display("  all_arrived          cyc %0d", t_arr);
            $display("  OUT_CAN_PUBLISH      cyc %0d", t_pub);
            $display("  c_valid_out (終點)   cyc %0d   c = %0d (含起點的 buffer 同步讀 1)",
                     t_cv, c);
            $display("  T = %0d = k %0d + 2(N-1) %0d + H %0d", T, K, 2 * (N - 1), H);
            $display("  H = g + r + c = %0d + %0d + %0d = %0d", g, r, c, g + r + c);

            chk($sformatf("tx%0d 最後運算元抵達拍 = 起點+k+2(N-1)", tx),
                t_last, t_start + K + 2 * (N - 1));
            chk($sformatf("tx%0d 四層 barrier 都有量到", tx), n_bar, 4);
            chk($sformatf("tx%0d g + r + c == H", tx), g + r + c, H);
            chk($sformatf("tx%0d RED_DONE 緊接最後一層 barrier", tx), t_done, t_bar[3] + 1);
            chk($sformatf("tx%0d acc_valid_out 緊接 RED_DONE", tx), t_pulse, t_done + 1);
            chk($sformatf("tx%0d all_arrived 緊接脈衝", tx), t_arr, t_pulse + 1);
            chk($sformatf("tx%0d CAN_PUBLISH 緊接 all_arrived", tx), t_pub, t_arr + 1);
            chk($sformatf("tx%0d c_valid_out 緊接 CAN_PUBLISH", tx), t_cv, t_pub + 1);

            /* 交易之間留夠長的空檔,模擬 UART 頂層一次一個 fold 的情境 */
            repeat (120) @(negedge clk);
        end

        $display("");
        $display("=========================================");
        if (errors == 0)
            $display("PASS  tb_array_h_decomp: %0d 項全部通過", checks);
        else
            $display("FAIL  tb_array_h_decomp: %0d / %0d 項失敗", errors, checks);
        $display("=========================================");
        $finish;
    end

endmodule
