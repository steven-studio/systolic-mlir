`timescale 1ns / 1ps
/*
 * tb_pe_counters -- 第 1 步的單元測試
 *
 * 檢查兩件事,兩件都不需要 PE 算對任何數學:
 *
 *   A. pass-through:a_out / b_out 是輸入延遲「恰好一拍」
 *   B. bank_counter:只在 product_valid 那一拍前進,序列是
 *      0,1,2,...,15,0,1,... 不跳號、不漏拍
 *
 * B 的檢查方式是「獨立重算」:tb 自己養一個 expect_bank,用同樣
 * 的規則前進,再跟 dut 的比。兩邊獨立算出同一個序列才算數 ——
 * 直接讀 dut 的值再跟自己比是沒有意義的。
 *
 * 跑法:
 *   verilator --binary -Wno-fatal --top-module tb_pe_counters \
 *       tb_pe_counters.sv fp_model.sv ../systolic_pe_tile.sv \
 *       -o tb && ./obj_dir/tb
 */
module tb_pe_counters;

    localparam int ACC_BANKS = 16;
    localparam int SEL_W     = $clog2(ACC_BANKS);

    logic clk = 1'b0;
    always #5 clk = ~clk;

    logic        rst;
    logic        a_valid_in, b_valid_in;
    logic [31:0] a_in, b_in;

    logic        a_valid_out, b_valid_out;
    logic [31:0] a_out, b_out;
    logic        acc_valid_out;
    logic [31:0] acc_out;

    systolic_pe_tile #(.ACC_BANKS(ACC_BANKS)) dut (
        .clk(clk), .rst(rst),
        .a_valid_in(a_valid_in), .b_valid_in(b_valid_in),
        .a_in(a_in), .b_in(b_in),
        .a_valid_out(a_valid_out), .b_valid_out(b_valid_out),
        .a_out(a_out), .b_out(b_out),
        .acc_valid_out(acc_valid_out), .acc_out(acc_out)
    );

    int errors  = 0;
    int checked = 0;

    task automatic chk(input logic cond, input string what);
        checked++;
        if (!cond) begin
            errors++;
            $display("  FAIL @%0t: %s", $time, what);
        end
    endtask

    /* ---------- A. pass-through:自己做一份一拍延遲 ---------- */
    logic [31:0] a_exp, b_exp;
    logic        av_exp, bv_exp;

    always_ff @(posedge clk) begin
        if (rst) begin
            a_exp <= '0;  b_exp <= '0;
            av_exp <= 1'b0;  bv_exp <= 1'b0;
        end
        else begin
            av_exp <= a_valid_in;
            bv_exp <= b_valid_in;
            if (a_valid_in) a_exp <= a_in;
            if (b_valid_in) b_exp <= b_in;
        end
    end

    /* ---------- B. 自己重算 bank 序列 ---------- */
    logic [SEL_W-1:0] expect_bank;
    int               products_seen;

    always_ff @(posedge clk) begin
        if (rst) begin
            expect_bank   <= '0;
            products_seen <= 0;
        end
        else begin
            /* 交棒那一拍計數器歸零 —— 下一個 tile 從 bank0 開始。
             *
             * 這裡讀 DUT 的 acc_handoff 是讀一個「事件」,不是讀被
             * 檢查的那個「值」。跟下面讀 product_valid 是同一件事:
             * 序列仍然由這裡獨立算,只是節拍對齊。 */
            if (dut.acc_handoff) begin
                expect_bank <= '0;
            end
            else if (dut.product_valid) begin
                expect_bank   <= expect_bank + 1'b1;
                products_seen <= products_seen + 1;
            end
        end
    end

    // 每一拍都比對。用 negedge 讀,避開與 always_ff 的競爭。
    always @(negedge clk) begin
        if (!rst) begin
            chk(a_out == a_exp && a_valid_out == av_exp, "a pass-through");
            chk(b_out == b_exp && b_valid_out == bv_exp, "b pass-through");
            chk(dut.accum_add_busy  <= 8'd64, "accum_add_busy 不該爆掉");
            chk(dut.reduce_add_busy <= 8'd64, "reduce_add_busy 不該爆掉");
            chk(!(dut.mul_busy == 0 && dut.product_valid),
                "乘法器空的時候不該吐出東西");
            chk(dut.product_bank == expect_bank,
                $sformatf("product_bank=%0d 期望 %0d",
                          dut.product_bank, expect_bank));
        end
    end

    int i;

    initial begin
        rst        = 1'b1;
        a_valid_in = 1'b0;
        b_valid_in = 1'b0;
        a_in       = '0;
        b_in       = '0;

        repeat (4) @(negedge clk);
        rst = 1'b0;

        // 連續丟 40 組。值用小整數,這一步不驗算術。
        for (i = 0; i < 40; i++) begin
            @(negedge clk);
            a_valid_in = 1'b1;
            b_valid_in = 1'b1;
            a_in       = 32'd1;
            b_in       = 32'(i + 1);
        end

        @(negedge clk);
        a_valid_in = 1'b0;
        b_valid_in = 1'b0;

        // 等管線排乾淨
        repeat (60) @(negedge clk);

        $display("");
        $display("乘積數    : %0d  (期望 40)", products_seen);
        /* 交棒之後計數器已經歸零,所以終值是 0 而不是 40 % 16。
         * 乒乓之前的舊行為是「歸約做完才歸零」,那時看到的是 8。 */
        $display("product_bank 終值 : %0d  (期望 0,交棒後已歸零)", dut.product_bank);
        chk(products_seen == 40, "fp_mul 一進一出:40 進 40 出");
        chk(dut.product_bank == SEL_W'(0), "product_bank 交棒後歸零");
        chk(dut.acc_state == 1'b0, "交棒後回到 ACC_IDLE");

        $display("");
        if (errors == 0)
            $display("PASS  (%0d 項檢查)", checked);
        else
            $display("FAIL  %0d / %0d 項不符", errors, checked);
        $finish;
    end

endmodule
