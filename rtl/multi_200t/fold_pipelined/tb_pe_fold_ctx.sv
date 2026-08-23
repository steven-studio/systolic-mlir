`timescale 1ns/1ps

module tb_pe_fold_ctx;

    logic clk;
    logic rst;

    logic a_valid_in;
    logic b_valid_in;

    logic fold_ctx_in;

    logic [31:0] a_in;
    logic [31:0] b_in;

    logic a_valid_out;
    logic b_valid_out;

    logic fold_ctx_out;

    logic [31:0] a_out;
    logic [31:0] b_out;

    logic [31:0] dbg_acc_ctx0 [0:15];
    logic [31:0] dbg_acc_ctx1 [0:15];

    integer cycle;
    integer wb_count;

    systolic_pe_fold dut (
        .clk          (clk),
        .rst          (rst),

        .a_valid_in   (a_valid_in),
        .b_valid_in   (b_valid_in),

        .fold_ctx_in  (fold_ctx_in),

        .a_in         (a_in),
        .b_in         (b_in),

        .a_valid_out  (a_valid_out),
        .b_valid_out  (b_valid_out),

        .fold_ctx_out (fold_ctx_out),

        .a_out        (a_out),
        .b_out        (b_out),

        .dbg_acc_ctx0 (dbg_acc_ctx0),
        .dbg_acc_ctx1 (dbg_acc_ctx1)
    );

    initial clk = 1'b0;
    always #5 clk = ~clk;


    initial begin
        cycle = 0;
        wb_count = 0;

        rst = 1'b1;

        a_valid_in  = 1'b0;
        b_valid_in  = 1'b0;
        fold_ctx_in = 1'b0;

        a_in = 32'h3f800000; // 1.0
        b_in = 32'h3f800000; // 1.0

        repeat (3) @(posedge clk);

        @(negedge clk);
        rst = 1'b0;


        /*
         * Fold 0:
         * 64 consecutive beats, ctx = 0.
         */
        for (int i = 0; i < 64; i++) begin
            @(negedge clk);

            a_valid_in  = 1'b1;
            b_valid_in  = 1'b1;
            fold_ctx_in = 1'b0;

            a_in = 32'h3f800000;
            b_in = 32'h3f800000;
        end


        /*
         * Fold 1:
         * immediately follows Fold 0.
         *
         * IMPORTANT:
         * no idle cycle, no reset, no drain bubble.
         */
        for (int i = 0; i < 64; i++) begin
            @(negedge clk);

            a_valid_in  = 1'b1;
            b_valid_in  = 1'b1;
            fold_ctx_in = 1'b1;

            a_in = 32'h3f800000;
            b_in = 32'h3f800000;
        end


        /*
         * Stop injection.
         */
        @(negedge clk);

        a_valid_in = 1'b0;
        b_valid_in = 1'b0;

        /*
         * Let MUL/ADD pipeline drain.
         */
        repeat (40) @(posedge clk);


        $display("");
        $display("========== FOLD CONTEXT TEST ==========");

        $write("ctx0: ");
        for (int i = 0; i < 16; i++)
            $write("%h ", dbg_acc_ctx0[i]);
        $display("");

        $write("ctx1: ");
        for (int i = 0; i < 16; i++)
            $write("%h ", dbg_acc_ctx1[i]);
        $display("");

        $display("");

        /*
         * Each context receives 64 products.
         *
         * 64 / 16 banks = 4 accumulations per bank.
         *
         * 1.0 * 1.0 accumulated 4 times = 4.0
         *
         * FP32 4.0 = 0x40800000
         */
        for (int i = 0; i < 16; i++) begin

            if (dbg_acc_ctx0[i] !== 32'h40800000) begin
                $display(
                    "FAIL: ctx0 bank[%0d] = %h, expected 40800000",
                    i,
                    dbg_acc_ctx0[i]
                );
                $fatal(1);
            end

            if (dbg_acc_ctx1[i] !== 32'h40800000) begin
                $display(
                    "FAIL: ctx1 bank[%0d] = %h, expected 40800000",
                    i,
                    dbg_acc_ctx1[i]
                );
                $fatal(1);
            end
        end

        $display("PASS: ctx0 and ctx1 are independent.");
        $display("PASS: fold boundary required zero bubble.");
        $display("PASS: each bank accumulated 4.0 independently.");
        $display("=======================================");

        $finish;
    end


    /*
     * Optional writeback trace.
     *
     * These are internal signals, but useful here because this is
     * specifically a PE microarchitecture testbench.
     */
    always @(posedge clk) begin
        if (!rst) begin
            cycle <= cycle + 1;

            if (dut.add_valid) begin
                wb_count <= wb_count + 1;

                $display(
                    "cycle=%0d add_valid=1 ctx=%0d bank=%0d result=%h",
                    cycle,
                    dut.writeback_ctx,
                    dut.writeback_sel,
                    dut.add_result
                );
            end
        end
    end

endmodule
