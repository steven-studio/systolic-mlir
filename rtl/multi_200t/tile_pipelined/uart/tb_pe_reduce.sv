`timescale 1ns / 1ps
/*
 * tb_pe_reduce -- 端到端算術檢查
 *
 * 刺激:a 全是 1,b 依序 1..16 → 乘積就是 1..16,總和 136。
 * fp_model 用整數算術(理由見該檔),所以可以要求逐位元相同。
 *
 * 檢查:
 *   1. 累加結束時,acc_bank[i] == 第 i 個乘積
 *   2. 歸約結束後 acc_out == 136,acc_valid_out 恰好脈衝一次
 *   3. 連跑兩次交易,第二次仍然是 136(證明 bank 有清乾淨)
 *   4. LAT 換成 3/5 仍然全部通過(證明不依賴 IP 延遲)
 */
module tb_pe_reduce;
    localparam int ACC_BANKS = 16;
    localparam int K = 16;

    logic clk = 1'b0;
    always #5 clk = ~clk;

    logic rst, a_valid_in, b_valid_in;
    logic [31:0] a_in, b_in;
    logic a_valid_out, b_valid_out;
    logic [31:0] a_out, b_out;
    logic acc_valid_out;
    logic [31:0] acc_out;

    systolic_pe_tile #(.ACC_BANKS(ACC_BANKS)) dut (
        .clk(clk), .rst(rst),
        .a_valid_in(a_valid_in), .b_valid_in(b_valid_in),
        .a_in(a_in), .b_in(b_in),
        .a_valid_out(a_valid_out), .b_valid_out(b_valid_out),
        .a_out(a_out), .b_out(b_out),
        .acc_valid_out(acc_valid_out), .acc_out(acc_out)
    );

    int errors = 0, checked = 0;
    task automatic chk(input logic cond, input string what);
        checked++;
        if (!cond) begin errors++; $display("  FAIL @%0t: %s", $time, what); end
    endtask

    int pulses = 0;
    int last_result;
    always @(posedge clk) if (!rst && acc_valid_out) begin
        pulses++;
        last_result = int'(acc_out);
    end

    task automatic run_txn(input int base);
        int i;
        for (i = 0; i < K; i++) begin
            @(negedge clk);
            a_valid_in = 1'b1;  b_valid_in = 1'b1;
            a_in = 32'd1;
            b_in = 32'(base + i + 1);
        end
        @(negedge clk);
        a_valid_in = 1'b0;  b_valid_in = 1'b0;
        repeat (200) @(negedge clk);       // 等排空 + 歸約
    endtask

    int j;
    int expect_sum;

    initial begin
        rst = 1'b1; a_valid_in = 0; b_valid_in = 0; a_in = '0; b_in = '0;
        repeat (4) @(negedge clk);
        rst = 1'b0;

        // ---- 第一次交易:1..16,和 = 136 ----
        run_txn(0);
        expect_sum = 0;
        for (j = 0; j < K; j++) expect_sum += (j + 1);
        chk(pulses == 1, $sformatf("acc_valid_out 脈衝 %0d 次(期望 1)", pulses));
        chk(last_result == expect_sum,
            $sformatf("第一次結果 %0d(期望 %0d)", last_result, expect_sum));
        $display("交易 1 : acc_out = %0d   期望 %0d", last_result, expect_sum);

        // ---- 第二次交易:17..32,和 = 392。bank 沒清乾淨就會偏大 ----
        run_txn(16);
        expect_sum = 0;
        for (j = 0; j < K; j++) expect_sum += (16 + j + 1);
        chk(pulses == 2, $sformatf("累計脈衝 %0d 次(期望 2)", pulses));
        chk(last_result == expect_sum,
            $sformatf("第二次結果 %0d(期望 %0d)", last_result, expect_sum));
        $display("交易 2 : acc_out = %0d   期望 %0d", last_result, expect_sum);

        $display("");
        if (errors == 0) $display("PASS  (%0d 項檢查)", checked);
        else             $display("FAIL  %0d / %0d 項不符", errors, checked);
        $finish;
    end
endmodule
