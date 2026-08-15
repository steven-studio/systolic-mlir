`timescale 1ns/1ps

module tb_systolic_array_4x4;

    localparam int DATA_W = 32;

    logic clk;
    logic rst;

    logic [DATA_W-1:0] a_in [0:3];
    logic [DATA_W-1:0] b_in [0:3];

    logic a_valid_in [0:3];
    logic b_valid_in [0:3];

    logic [3:0] acc_sel;

    logic reduce_start;
    logic c_valid_out;

    logic [DATA_W-1:0] c_out [0:3][0:3];

    logic [DATA_W-1:0] dbg_acc_out [0:3][0:3][0:15];


    systolic_array_4x4 dut (
        .clk          (clk),
        .rst          (rst),

        .a_in         (a_in),
        .b_in         (b_in),

        .a_valid_in   (a_valid_in),
        .b_valid_in   (b_valid_in),

        .acc_sel      (acc_sel),

        .reduce_start (reduce_start),

        .c_valid_out  (c_valid_out),
        .c_out        (c_out),

        .dbg_acc_out  (dbg_acc_out)
    );


    // 100 MHz
    initial clk = 1'b0;
    always #5 clk = ~clk;


    /*
     * A =
     *
     *  1   2   3   4
     *  5   6   7   8
     *  9  10  11  12
     * 13  14  15  16
     *
     *
     * B =
     *
     * 1 0 2 1
     * 0 1 0 2
     * 1 0 1 0
     * 0 1 0 1
     *
     *
     * Expected C =
     *
     *  4   6   5   9
     * 12  14  17  25
     * 20  22  29  41
     * 28  30  41  57
     */


    /*
     * Store A and B as IEEE-754 single precision.
     */
    logic [31:0] A [0:3][0:3];
    logic [31:0] B [0:3][0:3];


    initial begin

        /*
         * ========================================================
         * Matrix A
         * ========================================================
         */

        A[0][0] = 32'h3e000000; //  0.125
        A[0][1] = 32'h40800000; //  4.0
        A[0][2] = 32'hbf000000; // -0.5
        A[0][3] = 32'h41000000; //  8.0

        A[1][0] = 32'hc0000000; // -2.0
        A[1][1] = 32'h3e800000; //  0.25
        A[1][2] = 32'h40400000; //  3.0
        A[1][3] = 32'hbf800000; // -1.0

        A[2][0] = 32'h3f000000; //  0.5
        A[2][1] = 32'hc0800000; // -4.0
        A[2][2] = 32'h3fc00000; //  1.5
        A[2][3] = 32'h40000000; //  2.0

        A[3][0] = 32'h40c00000; //  6.0
        A[3][1] = 32'hbe000000; // -0.125
        A[3][2] = 32'hc0000000; // -2.0
        A[3][3] = 32'h3e800000; //  0.25


        /*
         * ========================================================
         * Matrix B
         * ========================================================
         */

        B[0][0] = 32'h3f000000; //  0.5
        B[0][1] = 32'hc0000000; // -2.0
        B[0][2] = 32'h40800000; //  4.0
        B[0][3] = 32'h3e000000; //  0.125

        B[1][0] = 32'h41000000; //  8.0
        B[1][1] = 32'h3e800000; //  0.25
        B[1][2] = 32'hbf800000; // -1.0
        B[1][3] = 32'h40000000; //  2.0

        B[2][0] = 32'hbf000000; // -0.5
        B[2][1] = 32'h40400000; //  3.0
        B[2][2] = 32'h3f000000; //  0.5
        B[2][3] = 32'hc0800000; // -4.0

        B[3][0] = 32'h3e800000; //  0.25
        B[3][1] = 32'h3f000000; //  0.5
        B[3][2] = 32'h40000000; //  2.0
        B[3][3] = 32'hbf000000; // -0.5


        /*
         * ========================================================
         * Initial state
         * ========================================================
         */

        rst          = 1'b1;
        acc_sel      = 4'd0;
        reduce_start = 1'b0;

        for (int i = 0; i < 4; i++) begin
            a_in[i]       = '0;
            b_in[i]       = '0;
            a_valid_in[i] = 1'b0;
            b_valid_in[i] = 1'b0;
        end


        /*
         * Reset
         */
        repeat (2) @(posedge clk);

        @(negedge clk);
        rst = 1'b0;


        /*
         * ========================================================
         * Systolic injection
         *
         * At time t:
         *
         * A[r][k] enters row r when t = r + k
         * B[k][c] enters col c when t = c + k
         * ========================================================
         */

        for (int t = 0; t < 10; t++) begin

            @(negedge clk);


            /*
             * Default idle.
             */
            for (int i = 0; i < 4; i++) begin
                a_in[i]       = '0;
                b_in[i]       = '0;
                a_valid_in[i] = 1'b0;
                b_valid_in[i] = 1'b0;
            end


            /*
             * Feed A rows.
             */
            for (int r = 0; r < 4; r++) begin

                int k;

                k = t - r;

                if ((k >= 0) && (k < 4)) begin
                    a_in[r]       = A[r][k];
                    a_valid_in[r] = 1'b1;
                end

            end


            /*
             * Feed B columns.
             */
            for (int c = 0; c < 4; c++) begin

                int k;

                k = t - c;

                if ((k >= 0) && (k < 4)) begin
                    b_in[c]       = B[k][c];
                    b_valid_in[c] = 1'b1;
                end

            end

        end


        /*
         * Stop input.
         */
        @(negedge clk);

        for (int i = 0; i < 4; i++) begin
            a_in[i]       = '0;
            b_in[i]       = '0;
            a_valid_in[i] = 1'b0;
            b_valid_in[i] = 1'b0;
        end


        /*
         * Let multiplier + accumulator pipelines drain.
         */
        repeat (50) @(posedge clk);


        /*
         * Start bank reduction.
         */
        @(negedge clk);
        reduce_start = 1'b1;

        @(negedge clk);
        reduce_start = 1'b0;


        /*
         * Wait for the final 4x4 C matrix.
         */
        wait (c_valid_out === 1'b1);

        #1;


        $display("");
        $display("========== 4x4 C MATRIX ==========");

        for (int r = 0; r < 4; r++) begin
            for (int c = 0; c < 4; c++) begin
                $write("%h ", c_out[r][c]);
            end

            $display("");
        end

        $display("==================================");


        /*
         * ========================================================
         * Check expected result
         * ========================================================
         */

        check_result(0, 0, 32'h41180000); //  9.5
        check_result(0, 1, 32'hbe800000); // -0.25
        check_result(0, 2, 32'h40e00000); //  7.0
        check_result(0, 3, 32'h40c00000); //  6.0

        check_result(1, 0, 32'h3f800000); //  1.0
        check_result(1, 1, 32'h3f000000); //  0.5
        check_result(1, 2, 32'h40600000); //  3.5
        check_result(1, 3, 32'hc0000000); // -2.0

        check_result(2, 0, 32'hc0c80000); // -6.25
        check_result(2, 1, 32'h41180000); //  9.5
        check_result(2, 2, 32'hc0800000); // -4.0
        check_result(2, 3, 32'h40c80000); //  6.25

        check_result(3, 0, 32'h40a80000); //  5.25
        check_result(3, 1, 32'hbf600000); // -0.875
        check_result(3, 2, 32'h40980000); //  4.75
        check_result(3, 3, 32'h40000000); //  2.0


        $display("");
        $display("PASS: 4x4 matrix multiplication is correct.");
        $display("");

        $finish;

    end


    /*
     * ============================================================
     * Result checker
     * ============================================================
     */

    task automatic check_result(
        input int r,
        input int c,
        input logic [31:0] expected
    );

        begin

            if (c_out[r][c] !== expected) begin

                $error(
                    "FAIL C[%0d][%0d]: expected=%h actual=%h",
                    r,
                    c,
                    expected,
                    c_out[r][c]
                );

                $fatal;

            end

        end

    endtask


endmodule
