`timescale 1ns/1ps

// tb_array_fold_fprandom.sv -- 用「真的會受加法順序影響」的資料驗證歸約。
//
//   vivado -mode batch -source sim_kmax.tcl \
//          -tclargs tb_array_fold_fprandom "K=64"
//
// 為什麼需要這支
//
//   tb_array_fold_kmax 餵 A=B=1.0，tb_array_fold_result_8x8 餵小整數。
//   兩者在 float32 下加起來都是精確的，所以任何加法順序都給出逐位元
//   相同的結果 —— 它們驗得了「有沒有加到正確的那一組數」，驗不了
//   「換了順序之後捨入有沒有變」。
//
//   歸約從串列改成樹狀之後，順序一定不同。這支用隨機的、尾數填滿的
//   float32 把差異逼出來。
//
// 資料來源
//
//   自帶 xorshift PRNG，不用 $urandom —— 這樣新舊兩版 RTL 跑出來的
//   輸入保證逐位元相同，diff 才有意義。
//
//   指數限制在 118..134（約 1e-3 ~ 1e3），避開 Inf / NaN / 非正規數，
//   同時讓不同項的量級差夠大，使捨入誤差確實會累積。
//
// 輸出
//
//   FPDUMP,ctx,r,c,<hw hex>,<float64 參考 hex>,<ulp 差>
//
//   把新舊兩版各跑一次、grep ^FPDUMP 存檔再 diff：
//     完全相同 -> 順序沒有實際影響，bit-exact 宣稱不受衝擊
//     差幾 ulp -> 正常的順序效應，需在論文註明並重跑 bit-exact sweep
//     差很大   -> 索引或層數有 bug
//
//   ULPSUM 那行是對 float64 真值的總誤差。樹狀歸約的誤差成長是
//   O(log n) 而非 O(n)，所以新版的 ULPSUM 應該「不會比舊版差」，
//   多半還會更好。那是可以寫進論文的加分項。

module tb_array_fold_fprandom;

    parameter int K    = 64;          // 必須是 8 的正倍數
    parameter int SEED = 32'h1234_5678;

    localparam int NF = K / 8;

    logic clk = 1'b0;
    logic rst;

    always #5 clk = ~clk;             // 100 MHz

    logic [31:0] a_in [0:7];
    logic [31:0] b_in [0:7];

    logic a_valid_in [0:7];
    logic b_valid_in [0:7];

    logic fold_ctx_in_a [0:7];
    logic fold_ctx_in_b [0:7];

    logic        c_valid_out;
    logic        c_ctx_out;
    logic [31:0] c_out [0:7][0:7];

    systolic_array_8x8_fold dut (
        .clk           (clk),
        .rst           (rst),
        .a_in          (a_in),
        .b_in          (b_in),
        .a_valid_in    (a_valid_in),
        .b_valid_in    (b_valid_in),
        .fold_ctx_in_a (fold_ctx_in_a),
        .fold_ctx_in_b (fold_ctx_in_b),
        .c_valid_out   (c_valid_out),
        .c_ctx_out     (c_ctx_out),
        .c_out         (c_out)
    );


    // ------------------------------------------------------------------
    // 決定性 PRNG（xorshift32）
    //
    // 不用 $urandom：不同 Vivado 版本的實作可能不同，而這支測試的全部
    // 意義就在於兩次執行的輸入必須逐位元相同。
    // ------------------------------------------------------------------
    logic [31:0] prng_state;

    function automatic logic [31:0] prng_next();
        logic [31:0] x;
        begin
            x = prng_state;
            x = x ^ (x << 13);
            x = x ^ (x >> 17);
            x = x ^ (x << 5);
            prng_state = x;
            prng_next  = x;
        end
    endfunction

    // 隨機但「安全」的 float32：填滿尾數，指數限制在合理範圍
    function automatic logic [31:0] rand_f32();
        logic [31:0] r;
        logic        s;
        logic [7:0]  e;
        logic [22:0] m;
        begin
            r = prng_next();
            s = r[31];
            e = 8'd118 + (r[30:24] % 8'd17);   // 118..134
            m = r[22:0];
            rand_f32 = {s, e, m};
        end
    endfunction


    // ------------------------------------------------------------------
    // 測試資料。索引方式與 feed 的 skew 一致：
    //   A[r][gk]、B[gk][c]，gk = 0..K-1
    // ------------------------------------------------------------------
    logic [31:0] Aval [0:7][0:1023];
    logic [31:0] Bval [0:1023][0:7];

    // float64 參考值，逐 gk 累加（與 reference.py 的順序一致）
    real ref_ctx0 [0:7][0:7];
    real ref_ctx1 [0:7][0:7];

    logic [31:0] C_ctx0 [0:7][0:7];
    logic [31:0] C_ctx1 [0:7][0:7];

    logic ctx0_seen, ctx1_seen;

    integer r, c, gk, fold;

    // ULP 距離可達 2^31 量級，用 longint 避免溢位與號誌問題
    longint ulp_diff;
    longint ulp_sum;
    longint ulp_max;


    task automatic clear_inputs();
        integer i;
        begin
            for (i = 0; i < 8; i = i + 1) begin
                a_in[i]          = 32'h0;
                b_in[i]          = 32'h0;
                a_valid_in[i]    = 1'b0;
                b_valid_in[i]    = 1'b0;
                fold_ctx_in_a[i] = 1'b0;
                fold_ctx_in_b[i] = 1'b0;
            end
        end
    endtask


    // 與 tb_array_fold_kmax 相同的 skew：global_k = t - lane
    task automatic feed_k();
        integer t, lane, g, fld;
        begin
            for (t = 0; t < K + 7; t = t + 1) begin
                @(negedge clk);
                clear_inputs();

                for (lane = 0; lane < 8; lane = lane + 1) begin
                    g = t - lane;
                    if (g >= 0 && g < K) begin
                        fld = g >> 3;
                        a_in[lane]          = Aval[lane][g];
                        a_valid_in[lane]    = 1'b1;
                        fold_ctx_in_a[lane] = fld[0];
                    end
                end

                for (lane = 0; lane < 8; lane = lane + 1) begin
                    g = t - lane;
                    if (g >= 0 && g < K) begin
                        fld = g >> 3;
                        b_in[lane]          = Bval[g][lane];
                        b_valid_in[lane]    = 1'b1;
                        fold_ctx_in_b[lane] = fld[0];
                    end
                end
            end

            @(negedge clk);
            clear_inputs();
        end
    endtask


    // 結果擷取
    always_ff @(posedge clk) begin
        if (rst) begin
            ctx0_seen <= 1'b0;
            ctx1_seen <= 1'b0;
        end
        else if (c_valid_out) begin
            for (int rr = 0; rr < 8; rr++) begin
                for (int cc = 0; cc < 8; cc++) begin
                    if (c_ctx_out == 1'b0) C_ctx0[rr][cc] <= c_out[rr][cc];
                    else                   C_ctx1[rr][cc] <= c_out[rr][cc];
                end
            end
            if (c_ctx_out == 1'b0) ctx0_seen <= 1'b1;
            else                   ctx1_seen <= 1'b1;
        end
    end


    // ULP 距離（同號 float32 可直接比整數表示）
    function automatic longint ulp_dist(input logic [31:0] x,
                                        input logic [31:0] y);
        longint xi, yi;
        begin
            xi = x[31] ? (64'sd2147483648 - longint'({1'b0, x[30:0]}))
                       :                    longint'({1'b0, x[30:0]});
            yi = y[31] ? (64'sd2147483648 - longint'({1'b0, y[30:0]}))
                       :                    longint'({1'b0, y[30:0]});
            ulp_dist = (xi > yi) ? (xi - yi) : (yi - xi);
        end
    endfunction


    initial begin

        if (K <= 0 || (K % 8) != 0) begin
            $display("FAIL: K must be a positive multiple of 8 (got %0d)", K);
            $fatal(1);
        end

        prng_state = SEED;

        // 產生資料並同步累出 float64 參考值
        for (r = 0; r < 8; r = r + 1)
            for (c = 0; c < 8; c = c + 1) begin
                ref_ctx0[r][c] = 0.0;
                ref_ctx1[r][c] = 0.0;
            end

        for (gk = 0; gk < K; gk = gk + 1) begin
            for (r = 0; r < 8; r = r + 1) Aval[r][gk] = rand_f32();
            for (c = 0; c < 8; c = c + 1) Bval[gk][c] = rand_f32();
        end

        for (gk = 0; gk < K; gk = gk + 1) begin
            fold = gk >> 3;
            for (r = 0; r < 8; r = r + 1) begin
                for (c = 0; c < 8; c = c + 1) begin
                    if (fold[0] == 1'b0)
                        ref_ctx0[r][c] = ref_ctx0[r][c] +
                            real'($bitstoshortreal(Aval[r][gk])) *
                            real'($bitstoshortreal(Bval[gk][c]));
                    else
                        ref_ctx1[r][c] = ref_ctx1[r][c] +
                            real'($bitstoshortreal(Aval[r][gk])) *
                            real'($bitstoshortreal(Bval[gk][c]));
                end
            end
        end

        rst = 1'b1;
        clear_inputs();
        repeat (10) @(posedge clk);
        rst = 1'b0;
        repeat (5)  @(posedge clk);

        feed_k();

        wait (ctx0_seen && ctx1_seen);
        @(posedge clk);

        ulp_sum = 0;
        ulp_max = 0;

        $display("");
        $display("=== tb_array_fold_fprandom  K=%0d  SEED=%h ===", K, SEED);

        for (r = 0; r < 8; r = r + 1) begin
            for (c = 0; c < 8; c = c + 1) begin

                ulp_diff = ulp_dist(C_ctx0[r][c],
                                    $shortrealtobits(shortreal'(ref_ctx0[r][c])));
                ulp_sum = ulp_sum + ulp_diff;
                if (ulp_diff > ulp_max) ulp_max = ulp_diff;
                $display("FPDUMP,0,%0d,%0d,%h,%h,%0d",
                         r, c, C_ctx0[r][c],
                         $shortrealtobits(shortreal'(ref_ctx0[r][c])),
                         ulp_diff);

                ulp_diff = ulp_dist(C_ctx1[r][c],
                                    $shortrealtobits(shortreal'(ref_ctx1[r][c])));
                ulp_sum = ulp_sum + ulp_diff;
                if (ulp_diff > ulp_max) ulp_max = ulp_diff;
                $display("FPDUMP,1,%0d,%0d,%h,%h,%0d",
                         r, c, C_ctx1[r][c],
                         $shortrealtobits(shortreal'(ref_ctx1[r][c])),
                         ulp_diff);

            end
        end

        $display("");
        $display("ULPSUM,%0d,%0d,%0d", K, ulp_sum, ulp_max);
        $display("  對 float64 真值的總 ULP 誤差 = %0d，最大 = %0d",
                 ulp_sum, ulp_max);
        $display("  （這不是 pass/fail。新舊兩版比較才有意義：");
        $display("    ULPSUM 沒有變大 = 樹狀歸約的精度沒有退步）");
        $display("");

        $finish;
    end

endmodule
