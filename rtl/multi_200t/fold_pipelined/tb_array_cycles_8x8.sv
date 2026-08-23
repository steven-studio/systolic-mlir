`timescale 1ns/1ps

module tb_array_cycles_8x8;

    localparam int DATA_W = 32;
    localparam int R      = 8;
    localparam int C      = 8;
    localparam int K      = 64;

    localparam int EXPECTED_WAVEFRONT = R + C - 2;   // 14
    localparam int EXPECTED_CYCLES    = K + R + C - 2; // 78

    logic clk;
    logic rst;

    logic [DATA_W-1:0] a_in [0:7];
    logic [DATA_W-1:0] b_in [0:7];

    logic a_valid_in [0:7];
    logic b_valid_in [0:7];

    logic [3:0] acc_sel;

    logic reduce_start;
    logic c_valid_out;

    logic [DATA_W-1:0] c_out [0:7][0:7];
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
     * 100 MHz clock
     * ============================================================
     */
    initial clk = 1'b0;
    always #5 clk = ~clk;


    /*
     * cycle = 0 means:
     *
     *   first k=0 operands enter the array boundary.
     *
     * We deliberately count from the first useful launch,
     * not from reset.
     */
    integer cycle;

    integer pe77_first_cycle;
    integer pe77_last_cycle;
    integer pe77_pair_count;


    /*
     * ============================================================
     * Stimulus
     *
     * Standard systolic skew:
     *
     * A[r][k] enters row r at:
     *
     *      t = k + r
     *
     * B[k][c] enters column c at:
     *
     *      t = k + c
     *
     * Therefore operands k meet at PE[r][c].
     * ============================================================
     */
    initial begin

        cycle            = -1;

        pe77_first_cycle = -1;
        pe77_last_cycle  = -1;
        pe77_pair_count  = 0;

        rst          = 1'b1;
        acc_sel      = 4'd0;
        reduce_start = 1'b0;

        for (int i = 0; i < 8; i++) begin
            a_in[i]       = '0;
            b_in[i]       = '0;

            a_valid_in[i] = 1'b0;
            b_valid_in[i] = 1'b0;
        end


        /*
         * Reset.
         */
        repeat (3) @(posedge clk);

        @(negedge clk);
        rst = 1'b0;


        /*
         * There are:
         *
         *   K + max(R-1, C-1)
         *
         * boundary-injection cycles.
         *
         * Last k=63 value for row/col 7 enters at:
         *
         *   63 + 7 = 70
         */
        for (int t = 0; t < K + 7; t++) begin

            @(negedge clk);

            /*
             * Clear boundary valids every cycle.
             */
            for (int i = 0; i < 8; i++) begin
                a_valid_in[i] = 1'b0;
                b_valid_in[i] = 1'b0;

                a_in[i] = '0;
                b_in[i] = '0;
            end


            /*
             * Feed A.
             *
             * For row r:
             *
             *   k = t - r
             */
            for (int r = 0; r < 8; r++) begin
                int k_idx;

                k_idx = t - r;

                if ((k_idx >= 0) && (k_idx < K)) begin

                    /*
                     * The actual value is irrelevant for this
                     * experiment; use 1.0f.
                     */
                    a_in[r]       = 32'h3f800000;
                    a_valid_in[r] = 1'b1;
                end
            end


            /*
             * Feed B.
             *
             * For column c:
             *
             *   k = t - c
             */
            for (int c = 0; c < 8; c++) begin
                int k_idx;

                k_idx = t - c;

                if ((k_idx >= 0) && (k_idx < K)) begin
                    b_in[c]       = 32'h3f800000;
                    b_valid_in[c] = 1'b1;
                end
            end

        end


        /*
         * Stop injection.
         */
        @(negedge clk);

        for (int i = 0; i < 8; i++) begin
            a_valid_in[i] = 1'b0;
            b_valid_in[i] = 1'b0;
        end


        /*
         * Allow the final wavefront to leave the boundaries
         * and reach PE[7][7].
         */
        repeat (16) @(posedge clk);


        $display("");
        $display("========== 8x8 ARRAY CYCLE TEST ==========");
        $display("R                      = %0d", R);
        $display("C                      = %0d", C);
        $display("K                      = %0d", K);
        $display("");

        $display(
            "Expected wavefront       = R + C - 2 = %0d",
            EXPECTED_WAVEFRONT
        );

        $display(
            "PE[7][7] first pair      = cycle %0d",
            pe77_first_cycle
        );

        $display(
            "PE[7][7] last pair       = cycle %0d",
            pe77_last_cycle
        );

        $display(
            "PE[7][7] pair count      = %0d",
            pe77_pair_count
        );

        $display(
            "Measured schedule cycles = %0d",
            pe77_last_cycle + 1
        );

        $display(
            "Expected schedule cycles = K + R + C - 2 = %0d",
            EXPECTED_CYCLES
        );

        $display("");


        /*
         * ========================================================
         * Assertions
         * ========================================================
         */

        if (pe77_first_cycle != EXPECTED_WAVEFRONT) begin

            $display(
                "FAIL: first PE[7][7] pair arrived at %0d, expected %0d",
                pe77_first_cycle,
                EXPECTED_WAVEFRONT
            );

            $fatal(1);

        end


        if (pe77_last_cycle != EXPECTED_CYCLES - 1) begin

            $display(
                "FAIL: last PE[7][7] pair arrived at %0d, expected %0d",
                pe77_last_cycle,
                EXPECTED_CYCLES - 1
            );

            $fatal(1);

        end


        if (pe77_pair_count != K) begin

            $display(
                "FAIL: PE[7][7] received %0d pairs, expected %0d",
                pe77_pair_count,
                K
            );

            $fatal(1);

        end


        $display(
            "PASS: 8x8 systolic wavefront satisfies K + R + C - 2 = %0d cycles.",
            EXPECTED_CYCLES
        );

        $display("==========================================");
        $display("");

        $finish;
    end


    /*
     * ============================================================
     * Cycle counter and PE[7][7] monitor
     * ============================================================
     *
     * cycle 0 is the first useful launch cycle.
     *
     * a_valid_bus[7][7] and b_valid_bus[7][7]
     * are the actual operand-valid signals entering PE[7][7].
     * ============================================================
     */
    always @(posedge clk) begin

        if (rst) begin
            cycle <= -1;
        end
        else begin

            cycle <= cycle + 1;


            if (
                dut.a_valid_bus[7][7]
                &&
                dut.b_valid_bus[7][7]
            ) begin

                if (pe77_pair_count == 0)
                    pe77_first_cycle = cycle;

                pe77_last_cycle = cycle;

                pe77_pair_count = pe77_pair_count + 1;

                $display(
                    "cycle=%0d PE[7][7] pair_valid=1 pair=%0d",
                    cycle,
                    pe77_pair_count
                );

            end

        end
    end

endmodule
