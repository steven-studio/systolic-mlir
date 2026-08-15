`timescale 1ns/1ps

module tb_cycle_formula_4x4;

    /*
     * ============================================================
     * Test parameters
     * ============================================================
     */
    localparam int DATA_W = 32;

    localparam int R = 4;
    localparam int C = 4;
    localparam int K = 4;

    /*
     * Formula:
     *
     * T = N_FOLDS * ceil(K / DEPTH) + (R + C - 2)
     *
     * Current 4x4 test:
     *
     * N_FOLDS = 4
     * DEPTH   = 4
     *
     * ceil(4 / 4) = 1
     *
     * T = 4 * 1 + (4 + 4 - 2)
     *   = 4 + 6
     *   = 10 cycles
     */
    localparam int N_FOLDS = 4;
    localparam int DEPTH   = 4;

    localparam int K_CHUNKS =
        (K + DEPTH - 1) / DEPTH;

    localparam int EXPECTED_CYCLES =
        N_FOLDS * K_CHUNKS
        + (R + C - 2);


    /*
     * ============================================================
     * DUT signals
     * ============================================================
     */

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

    logic [DATA_W-1:0]
        dbg_acc_out [0:3][0:3][0:15];


    /*
     * ============================================================
     * DUT
     * ============================================================
     */

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


    /*
     * ============================================================
     * Clock: 100 MHz
     * ============================================================
     */

    initial clk = 1'b0;

    always #5 clk = ~clk;


    /*
     * ============================================================
     * Matrix storage
     * ============================================================
     */

    logic [31:0] A [0:3][0:3];
    logic [31:0] B [0:3][0:3];


    /*
     * ============================================================
     * Cycle measurement
     * ============================================================
     */

    integer cycle;

    integer first_launch_cycle;

    integer first_bottom_right_cycle;
    integer last_bottom_right_cycle;

    integer bottom_right_count;

    integer measured_cycles;


    /*
     * Bottom-right PE receives the final systolic wave.
     *
     * This is NOT add_valid and NOT c_valid_out.
     *
     * We intentionally observe pair_valid here because the formula
     * describes systolic scheduling / traversal rather than the
     * latency of the floating-point IP blocks.
     */
    wire bottom_right_pair_valid =
        dut.ROW[3].COL[3].u_pe.pair_valid;


    /*
     * ============================================================
     * Main test
     * ============================================================
     */

    initial begin

        /*
         * --------------------------------------------------------
         * Use simple 1.0 values.
         *
         * Numerical correctness was already tested separately.
         * Here we care only about cycle scheduling.
         * --------------------------------------------------------
         */

        for (int r = 0; r < 4; r++) begin
            for (int k = 0; k < 4; k++) begin
                A[r][k] = 32'h3f800000;
            end
        end

        for (int k = 0; k < 4; k++) begin
            for (int c = 0; c < 4; c++) begin
                B[k][c] = 32'h3f800000;
            end
        end


        /*
         * --------------------------------------------------------
         * Initial state
         * --------------------------------------------------------
         */

        cycle                    = 0;

        first_launch_cycle       = -1;

        first_bottom_right_cycle = -1;
        last_bottom_right_cycle  = -1;

        bottom_right_count       = 0;

        measured_cycles          = 0;

        rst                      = 1'b1;

        acc_sel                  = 4'd0;

        reduce_start             = 1'b0;


        for (int i = 0; i < 4; i++) begin

            a_in[i]       = '0;
            b_in[i]       = '0;

            a_valid_in[i] = 1'b0;
            b_valid_in[i] = 1'b0;

        end


        /*
         * --------------------------------------------------------
         * Reset
         * --------------------------------------------------------
         */

        repeat (2)
            @(posedge clk);

        @(negedge clk);

        rst = 1'b0;


        /*
         * --------------------------------------------------------
         * Systolic injection
         *
         * A[r][k] enters at:
         *
         *     t = r + k
         *
         * B[k][c] enters at:
         *
         *     t = c + k
         *
         * --------------------------------------------------------
         */

        for (int t = 0; t < 10; t++) begin

            @(negedge clk);


            /*
             * Default idle
             */

            for (int i = 0; i < 4; i++) begin

                a_in[i]       = '0;
                b_in[i]       = '0;

                a_valid_in[i] = 1'b0;
                b_valid_in[i] = 1'b0;

            end


            /*
             * Record the very first launch.
             *
             * t = 0 means A[0][0] and B[0][0]
             * enter the array.
             */

            if (t == 0) begin

                first_launch_cycle = cycle;

                $display("");
                $display(
                    "FIRST ARRAY LAUNCH : cycle %0d",
                    first_launch_cycle
                );

            end


            /*
             * Feed A
             */

            for (int r = 0; r < 4; r++) begin

                int k_idx;

                k_idx = t - r;

                if (
                    (k_idx >= 0)
                    &&
                    (k_idx < K)
                ) begin

                    a_in[r]       = A[r][k_idx];
                    a_valid_in[r] = 1'b1;

                end

            end


            /*
             * Feed B
             */

            for (int c = 0; c < 4; c++) begin

                int k_idx;

                k_idx = t - c;

                if (
                    (k_idx >= 0)
                    &&
                    (k_idx < K)
                ) begin

                    b_in[c]       = B[k_idx][c];
                    b_valid_in[c] = 1'b1;

                end

            end

        end


        /*
         * --------------------------------------------------------
         * Stop injection
         * --------------------------------------------------------
         */

        @(negedge clk);

        for (int i = 0; i < 4; i++) begin

            a_in[i]       = '0;
            b_in[i]       = '0;

            a_valid_in[i] = 1'b0;
            b_valid_in[i] = 1'b0;

        end


        /*
         * Give bottom-right PE time to receive everything.
         */

        repeat (10)
            @(posedge clk);


        /*
         * ========================================================
         * Result
         * ========================================================
         *
         * Formula counts cycles inclusively:
         *
         * cycle 0 ... cycle 9
         *
         * = 10 cycles
         *
         * Therefore:
         *
         * last event - first launch + 1
         * ========================================================
         */

        measured_cycles =
            last_bottom_right_cycle
            - first_launch_cycle
            + 1;


        $display("");
        $display(
            "============================================"
        );

        $display(
            "      SYSTOLIC CYCLE FORMULA TEST"
        );

        $display(
            "============================================"
        );

        $display(
            "R               = %0d",
            R
        );

        $display(
            "C               = %0d",
            C
        );

        $display(
            "K               = %0d",
            K
        );

        $display(
            "N_FOLDS         = %0d",
            N_FOLDS
        );

        $display(
            "DEPTH           = %0d",
            DEPTH
        );

        $display(
            "ceil(K/DEPTH)   = %0d",
            K_CHUNKS
        );

        $display("");

        $display(
            "Formula:"
        );

        $display(
            "T = N_FOLDS * ceil(K/DEPTH) + (R+C-2)"
        );

        $display("");

        $display(
            "Expected cycles = %0d",
            EXPECTED_CYCLES
        );

        $display(
            "Measured cycles = %0d",
            measured_cycles
        );

        $display("");

        $display(
            "First launch cycle       = %0d",
            first_launch_cycle
        );

        $display(
            "First bottom-right cycle = %0d",
            first_bottom_right_cycle
        );

        $display(
            "Last bottom-right cycle  = %0d",
            last_bottom_right_cycle
        );

        $display(
            "Bottom-right MAC count   = %0d",
            bottom_right_count
        );

        $display(
            "============================================"
        );


        /*
         * Bottom-right PE must execute K MACs.
         */

        if (bottom_right_count != K) begin

            $fatal(
                1,
                "FAIL: bottom-right PE expected %0d MACs, got %0d",
                K,
                bottom_right_count
            );

        end


        /*
         * Compare simulation against formula.
         */

        if (measured_cycles != EXPECTED_CYCLES) begin

            $fatal(
                1,
                "FAIL: formula=%0d cycles, simulation=%0d cycles",
                EXPECTED_CYCLES,
                measured_cycles
            );

        end


        $display("");
        $display(
            "PASS: cycle formula verified."
        );

        $display(
            "PASS: %0d simulated cycles == %0d formula cycles.",
            measured_cycles,
            EXPECTED_CYCLES
        );

        $display("");

        $finish;

    end


    /*
     * ============================================================
     * Global cycle counter
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


    /*
     * ============================================================
     * Observe bottom-right PE
     * ============================================================
     */

    always @(posedge clk) begin

        if (!rst) begin

            if (bottom_right_pair_valid) begin

                /*
                 * First valid pair arriving at PE[3][3].
                 */

                if (bottom_right_count == 0) begin

                    first_bottom_right_cycle = cycle;

                end


                /*
                 * Every valid pair updates the final cycle.
                 */

                last_bottom_right_cycle = cycle;

                bottom_right_count =
                    bottom_right_count + 1;


                $display(
                    "BOTTOM_RIGHT: cycle=%0d MAC=%0d",
                    cycle,
                    bottom_right_count
                );

            end

        end

    end


endmodule
