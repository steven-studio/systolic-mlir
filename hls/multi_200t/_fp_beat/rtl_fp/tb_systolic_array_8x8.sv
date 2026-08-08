`timescale 1ns/1ps

module tb_systolic_array_8x8;

    localparam int DATA_W = 32;

    logic clk;
    logic rst;

    logic [DATA_W-1:0] a_in [0:7];
    logic [DATA_W-1:0] b_in [0:7];

    logic a_valid_in [0:7];
    logic b_valid_in [0:7];

    logic [3:0] acc_sel;

    /*
     * Final reduction control/output
     */
    logic reduce_start;
    logic c_valid_out;
    logic [DATA_W-1:0] c_out [0:7][0:7];

    /*
     * Debug accumulator output
     */
    logic [DATA_W-1:0] dbg_acc_out [0:7][0:7][0:15];


    /*
     * ============================================================
     * DUT
     * ============================================================
     */
    systolic_array_8x8 dut (
        .clk         (clk),
        .rst         (rst),

        .a_in        (a_in),
        .b_in        (b_in),

        .a_valid_in  (a_valid_in),
        .b_valid_in  (b_valid_in),

        .acc_sel     (acc_sel),

        .reduce_start(reduce_start),
        .c_valid_out (c_valid_out),
        .c_out       (c_out),

        .dbg_acc_out (dbg_acc_out)
    );


    /*
     * ============================================================
     * Clock: 100 MHz
     * ============================================================
     */
    initial clk = 1'b0;
    always #5 clk = ~clk;


    integer cycle;


    /*
     * ============================================================
     * Test
     *
     * A = all ones
     * B = all ones
     *
     * Expected:
     *
     * C = A x B
     *
     * every C[r][c] = 8.0
     *
     * IEEE754:
     * 8.0 = 32'h41000000
     * ============================================================
     */
    initial begin

        cycle        = 0;
        rst          = 1'b1;
        acc_sel      = 4'd0;
        reduce_start = 1'b0;


        /*
         * Clear inputs.
         */
        for (int i = 0; i < 8; i++) begin
            a_in[i]       = '0;
            b_in[i]       = '0;
            a_valid_in[i] = 1'b0;
            b_valid_in[i] = 1'b0;
        end


        /*
         * Reset for two clocks.
         */
        repeat (2) @(posedge clk);

        @(negedge clk);
        rst = 1'b0;


        /*
         * ========================================================
         * Inject 8x8 all-ones GEMM
         *
         * For systolic skew:
         *
         * A row r begins at t=r
         * B col c begins at t=c
         *
         * Each row/column sends 8 values.
         * ========================================================
         */
        for (int t = 0; t < 22; t++) begin

            @(negedge clk);


            /*
             * Default: no input this cycle.
             */
            for (int i = 0; i < 8; i++) begin
                a_in[i]       = '0;
                b_in[i]       = '0;
                a_valid_in[i] = 1'b0;
                b_valid_in[i] = 1'b0;
            end


            /*
             * Inject A.
             */
            for (int r = 0; r < 8; r++) begin

                if ((t >= r) && (t < r + 8)) begin
                    a_in[r]       = 32'h3f800000; // 1.0
                    a_valid_in[r] = 1'b1;
                end

            end


            /*
             * Inject B.
             */
            for (int c = 0; c < 8; c++) begin

                if ((t >= c) && (t < c + 8)) begin
                    b_in[c]       = 32'h3f800000; // 1.0
                    b_valid_in[c] = 1'b1;
                end

            end

        end


        /*
         * ========================================================
         * Stop injection
         * ========================================================
         */
        @(negedge clk);

        for (int i = 0; i < 8; i++) begin
            a_in[i]       = '0;
            b_in[i]       = '0;
            a_valid_in[i] = 1'b0;
            b_valid_in[i] = 1'b0;
        end


        /*
         * ========================================================
         * Wait for MUL + ADD pipelines to drain.
         * ========================================================
         */
        repeat (50) @(posedge clk);


        /*
         * ========================================================
         * Start final reduction.
         *
         * Pulse reduce_start for exactly one cycle.
         * ========================================================
         */
        @(negedge clk);
        reduce_start = 1'b1;

        @(negedge clk);
        reduce_start = 1'b0;


        /*
         * ========================================================
         * Wait until C matrix is ready.
         * ========================================================
         */
        wait (c_valid_out == 1'b1);


        /*
         * c_out is valid in this cycle.
         */
        #1;


        /*
         * ========================================================
         * Print final C matrix
         * ========================================================
         */
        $display("");
        $display("========== C MATRIX ==========");

        for (int r = 0; r < 8; r++) begin

            for (int c = 0; c < 8; c++) begin
                $write("%h ", c_out[r][c]);
            end

            $display("");

        end

        $display("==============================");


        /*
         * ========================================================
         * Automatic correctness check
         * ========================================================
         */
        for (int r = 0; r < 8; r++) begin

            for (int c = 0; c < 8; c++) begin

                if (c_out[r][c] !== 32'h41000000) begin

                    $error(
                        "FAIL PE[%0d][%0d]: expected 41000000, got %h",
                        r,
                        c,
                        c_out[r][c]
                    );

                end

            end

        end


        $display("");
        $display("PASS: 8x8 all-ones GEMM produced all 8.0");
        $display("");


        /*
         * ========================================================
         * Optional accumulator dump
         * ========================================================
         */
        $display("========== ACCUMULATOR DUMP ==========");

        for (int r = 0; r < 8; r++) begin

            for (int c = 0; c < 8; c++) begin

                $write("PE[%0d][%0d]: ", r, c);

                for (int k = 0; k < 16; k++) begin
                    $write("%h ", dbg_acc_out[r][c][k]);
                end

                $display("");

            end

        end

        $display("======================================");


        $finish;

    end


    /*
     * ============================================================
     * Simple cycle monitor
     * ============================================================
     */
    always @(posedge clk) begin

        if (rst) begin
            cycle <= 0;
        end
        else begin
            cycle <= cycle + 1;
        end

    end


endmodule