`timescale 1ns/1ps

module tb_array_fold_cycles_8x8;

    localparam int DATA_W = 32;

    localparam int R      = 8;
    localparam int C      = 8;
    localparam int K      = 64;
    localparam int NFOLDS = 2;

    localparam int EXPECTED_WAVEFRONT =
        R + C - 2;                         // 14

    localparam int EXPECTED_SHARED =
        NFOLDS * K + EXPECTED_WAVEFRONT;   // 142

    localparam int EXPECTED_PER_FOLD =
        NFOLDS * (K + EXPECTED_WAVEFRONT); // 156


    localparam logic [31:0] FP_ONE   = 32'h3f800000;
    localparam logic [31:0] FP_TWO   = 32'h40000000;
    localparam logic [31:0] FP_FOUR  = 32'h40800000;
    localparam logic [31:0] FP_EIGHT = 32'h41000000;


    logic clk;
    logic rst;

    logic [DATA_W-1:0] a_in [0:7];
    logic [DATA_W-1:0] b_in [0:7];

    logic a_valid_in [0:7];
    logic b_valid_in [0:7];

    logic fold_ctx_in_a [0:7];
    logic fold_ctx_in_b [0:7];

    logic [DATA_W-1:0]
        dbg_acc_ctx0 [0:7][0:7][0:15];

    logic [DATA_W-1:0]
        dbg_acc_ctx1 [0:7][0:7][0:15];


    /*
     * ============================================================
     * DUT
     * ============================================================
     */
    systolic_array_8x8_fold dut (
        .clk              (clk),
        .rst              (rst),

        .a_in             (a_in),
        .b_in             (b_in),

        .a_valid_in       (a_valid_in),
        .b_valid_in       (b_valid_in),

        .fold_ctx_in_a    (fold_ctx_in_a),
        .fold_ctx_in_b    (fold_ctx_in_b),

        .dbg_acc_ctx0     (dbg_acc_ctx0),
        .dbg_acc_ctx1     (dbg_acc_ctx1)
    );


    /*
     * 100 MHz
     */
    initial clk = 1'b0;
    always #5 clk = ~clk;


    integer cycle;

    integer first_launch_cycle;
    integer fold1_launch_cycle;

    integer pe77_first_cycle;
    integer pe77_last_cycle;
    integer pe77_pair_count;

    integer numerical_errors;


    initial begin

        cycle              = -1;

        first_launch_cycle = -1;
        fold1_launch_cycle = -1;

        pe77_first_cycle   = -1;
        pe77_last_cycle    = -1;
        pe77_pair_count    = 0;

        numerical_errors   = 0;

        rst = 1'b1;

        for (int i = 0; i < 8; i++) begin

            a_in[i] = 32'd0;
            b_in[i] = 32'd0;

            a_valid_in[i] = 1'b0;
            b_valid_in[i] = 1'b0;

            fold_ctx_in_a[i] = 1'b0;
            fold_ctx_in_b[i] = 1'b0;

        end


        /*
         * Reset
         */
        repeat (3) @(posedge clk);

        @(negedge clk);
        rst = 1'b0;


        /*
         * ========================================================
         * Two folds, back-to-back, zero bubble.
         *
         * Fold 0:
         *     global_k = 0..63
         *     A = 1.0
         *     B = 1.0
         *
         * Fold 1:
         *     global_k = 64..127
         *     A = 2.0
         *     B = 1.0
         *
         * Systolic skew:
         *
         *     A row r:
         *       global_k = t-r
         *
         *     B col c:
         *       global_k = t-c
         * ========================================================
         */

        for (
            int t = 0;
            t < NFOLDS*K + (R-1);
            t++
        ) begin

            @(negedge clk);


            /*
             * Clear boundary every cycle.
             */
            for (int i = 0; i < 8; i++) begin

                a_in[i] = 32'd0;
                b_in[i] = 32'd0;

                a_valid_in[i] = 1'b0;
                b_valid_in[i] = 1'b0;

                fold_ctx_in_a[i] = 1'b0;
                fold_ctx_in_b[i] = 1'b0;

            end


            /*
             * ====================================================
             * A-side
             * ====================================================
             */
            for (int r = 0; r < 8; r++) begin

                int global_k;
                int fold_id;

                global_k = t - r;

                if (
                    (global_k >= 0)
                    &&
                    (global_k < NFOLDS*K)
                ) begin

                    fold_id = global_k / K;

                    if (fold_id == 0)
                        a_in[r] = FP_ONE;
                    else
                        a_in[r] = FP_TWO;

                    a_valid_in[r] = 1'b1;

                    fold_ctx_in_a[r] =
                        fold_id[0];

                end

            end


            /*
             * ====================================================
             * B-side
             * ====================================================
             */
            for (int c = 0; c < 8; c++) begin

                int global_k;
                int fold_id;

                global_k = t - c;

                if (
                    (global_k >= 0)
                    &&
                    (global_k < NFOLDS*K)
                ) begin

                    fold_id = global_k / K;

                    b_in[c] = FP_ONE;

                    b_valid_in[c] = 1'b1;

                    fold_ctx_in_b[c] =
                        fold_id[0];

                end

            end

        end


        /*
         * Stop injection.
         */
        @(negedge clk);

        for (int i = 0; i < 8; i++) begin

            a_in[i] = 32'd0;
            b_in[i] = 32'd0;

            a_valid_in[i] = 1'b0;
            b_valid_in[i] = 1'b0;

        end


        /*
         * Drain FP pipelines before checking accumulator contents.
         *
         * IMPORTANT:
         * these extra cycles are NOT part of the measured
         * systolic wavefront span.
         */
        repeat (40) @(posedge clk);


        /*
         * ========================================================
         * Timing report
         * ========================================================
         */

        $display("");
        $display("========== 8x8 FOLD PIPELINE TEST ==========");

        $display("R                         = %0d", R);
        $display("C                         = %0d", C);
        $display("K                         = %0d", K);
        $display("Nfolds                    = %0d", NFOLDS);

        $display("");

        $display(
            "Expected R+C-2            = %0d",
            EXPECTED_WAVEFRONT
        );

        $display(
            "Expected shared-wavefront = %0d",
            EXPECTED_SHARED
        );

        $display(
            "Expected per-fold model   = %0d",
            EXPECTED_PER_FOLD
        );

        $display("");

        $display(
            "first launch cycle         = %0d",
            first_launch_cycle
        );

        $display(
            "fold1 first launch cycle   = %0d",
            fold1_launch_cycle
        );

        $display(
            "PE[7][7] first pair cycle  = %0d",
            pe77_first_cycle
        );

        $display(
            "PE[7][7] last pair cycle   = %0d",
            pe77_last_cycle
        );

        $display(
            "PE[7][7] pair count        = %0d",
            pe77_pair_count
        );

        $display(
            "Measured total cycles      = %0d",
            pe77_last_cycle
            - first_launch_cycle
            + 1
        );

        $display("");


        /*
         * ========================================================
         * Timing assertions
         * ========================================================
         */

        if (first_launch_cycle != 0) begin

            $display(
                "FAIL: first launch = %0d, expected 0",
                first_launch_cycle
            );

            $fatal(1);

        end


        /*
         * Fold 1 starts immediately at cycle 64.
         */
        if (fold1_launch_cycle != K) begin

            $display(
                "FAIL: fold1 launch = %0d, expected %0d",
                fold1_launch_cycle,
                K
            );

            $fatal(1);

        end


        /*
         * PE[7][7] first sees the wavefront after:
         *
         *     7 + 7 = 14 cycles
         */
        if (
            pe77_first_cycle
            != EXPECTED_WAVEFRONT
        ) begin

            $display(
                "FAIL: PE[7][7] first pair = %0d, expected %0d",
                pe77_first_cycle,
                EXPECTED_WAVEFRONT
            );

            $fatal(1);

        end


        /*
         * Main timing hypothesis:
         *
         *     Nfolds*K + R+C-2
         */
        if (
            (
                pe77_last_cycle
                - first_launch_cycle
                + 1
            )
            != EXPECTED_SHARED
        ) begin

            $display(
                "FAIL: measured total = %0d, expected %0d",
                pe77_last_cycle
                - first_launch_cycle
                + 1,
                EXPECTED_SHARED
            );

            $fatal(1);

        end


        /*
         * 2 folds × 64 beats = 128 pairs.
         */
        if (
            pe77_pair_count
            != NFOLDS*K
        ) begin

            $display(
                "FAIL: PE[7][7] received %0d pairs, expected %0d",
                pe77_pair_count,
                NFOLDS*K
            );

            $fatal(1);

        end


        /*
         * ========================================================
         * Numerical verification
         *
         * ctx0:
         *
         *   64 × (1*1)
         *   spread across 16 banks
         *   => 4 accumulations/bank
         *   => 4.0
         *
         * ctx1:
         *
         *   64 × (2*1)
         *   spread across 16 banks
         *   => 4 × 2
         *   => 8.0
         * ========================================================
         */

        $display(
            "========== NUMERICAL CHECK =========="
        );


        for (int r = 0; r < 8; r++) begin

            for (int c = 0; c < 8; c++) begin

                for (
                    int bank = 0;
                    bank < 16;
                    bank++
                ) begin


                    if (
                        dbg_acc_ctx0[r][c][bank]
                        !== FP_FOUR
                    ) begin

                        $display(
                            "FAIL: PE[%0d][%0d] ctx0 bank[%0d] = %h, expected %h",
                            r,
                            c,
                            bank,
                            dbg_acc_ctx0[r][c][bank],
                            FP_FOUR
                        );

                        numerical_errors =
                            numerical_errors + 1;

                    end


                    if (
                        dbg_acc_ctx1[r][c][bank]
                        !== FP_EIGHT
                    ) begin

                        $display(
                            "FAIL: PE[%0d][%0d] ctx1 bank[%0d] = %h, expected %h",
                            r,
                            c,
                            bank,
                            dbg_acc_ctx1[r][c][bank],
                            FP_EIGHT
                        );

                        numerical_errors =
                            numerical_errors + 1;

                    end

                end

            end

        end


        if (numerical_errors != 0) begin

            $display(
                "FAIL: numerical_errors = %0d",
                numerical_errors
            );

            $fatal(1);

        end


        /*
         * Print representative bottom-right PE.
         */
        $write("PE[7][7] ctx0: ");

        for (int bank = 0; bank < 16; bank++)
            $write(
                "%h ",
                dbg_acc_ctx0[7][7][bank]
            );

        $display("");


        $write("PE[7][7] ctx1: ");

        for (int bank = 0; bank < 16; bank++)
            $write(
                "%h ",
                dbg_acc_ctx1[7][7][bank]
            );

        $display("");
        $display("");


        /*
         * ========================================================
         * PASS
         * ========================================================
         */

        $display(
            "PASS: fold boundary is zero-bubble."
        );

        $display(
            "PASS: PE[7][7] saw %0d consecutive reduction pairs.",
            NFOLDS*K
        );

        $display(
            "PASS: measured total = %0d cycles.",
            EXPECTED_SHARED
        );

        $display(
            "PASS: hardware propagation matches Nfolds*K + (R+C-2)."
        );

        $display(
            "PASS: shared-wavefront model beats per-fold model by %0d cycles.",
            EXPECTED_PER_FOLD
            - EXPECTED_SHARED
        );

        $display(
            "PASS: ctx0 numerical accumulation = 4.0 in every PE/bank."
        );

        $display(
            "PASS: ctx1 numerical accumulation = 8.0 in every PE/bank."
        );

        $display(
            "PASS: fold contexts remain numerically independent."
        );

        $display(
            "============================================"
        );

        $finish;

    end


    /*
     * ============================================================
     * Cycle monitor
     * ============================================================
     */
    always @(posedge clk) begin

        if (rst) begin

            cycle <= -1;

        end
        else begin

            cycle <= cycle + 1;


            /*
             * First PE boundary.
             */
            if (
                dut.a_valid_bus[0][0]
                &&
                dut.b_valid_bus[0][0]
            ) begin

                if (first_launch_cycle < 0)
                    first_launch_cycle = cycle;


                /*
                 * First ctx1 input marks second fold launch.
                 */
                if (
                    fold1_launch_cycle < 0
                    &&
                    dut.fold_ctx_a_bus[0][0]
                    == 1'b1
                ) begin

                    fold1_launch_cycle = cycle;

                end

            end


            /*
             * Bottom-right PE.
             */
            if (
                dut.a_valid_bus[7][7]
                &&
                dut.b_valid_bus[7][7]
            ) begin

                if (pe77_pair_count == 0)
                    pe77_first_cycle = cycle;

                pe77_last_cycle =
                    cycle;

                pe77_pair_count =
                    pe77_pair_count + 1;

            end

        end

    end

endmodule
