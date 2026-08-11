`timescale 1ns/1ps

module tb_array_fold_24x24;

    /*
     * ============================================================
     * Clock / reset
     * ============================================================
     */

    logic clk;
    logic rst;

    initial begin
        clk = 1'b0;
        forever #5 clk = ~clk;   // 100 MHz
    end


    /*
     * ============================================================
     * Array inputs
     * ============================================================
     */

    logic [31:0] a_in [0:7];
    logic [31:0] b_in [0:7];

    logic a_valid_in [0:7];
    logic b_valid_in [0:7];

    logic fold_ctx_in_a [0:7];
    logic fold_ctx_in_b [0:7];


    /*
     * ============================================================
     * Array outputs
     * ============================================================
     */

    logic        c_valid_out;
    logic        c_ctx_out;
    logic [31:0] c_out [0:7][0:7];


    /*
     * ============================================================
     * DUT
     * ============================================================
     */

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


    /*
     * ============================================================
     * Full 24x24 input matrices
     * ============================================================
     *
     * First full-GEMM test:
     *
     * A = all 1.0
     * B = all 1.0
     *
     * A(24x24) * B(24x24) = C(24x24)
     *
     * Therefore every C element should be:
     *
     *   24.0
     *
     * K=24 gives three folds:
     *
     *   fold0: k=0..7   -> ctx0
     *   fold1: k=8..15  -> ctx1
     *   fold2: k=16..23 -> ctx0
     *
     * Therefore:
     *
     *   ctx0 = 16.0
     *   ctx1 =  8.0
     *   final = 24.0
     * ============================================================
     */

    logic [31:0] A [0:23][0:23];
    logic [31:0] B [0:23][0:23];

    localparam logic [31:0] FP32_ONE     = 32'h3f800000;
    localparam logic [31:0] FP32_EIGHT   = 32'h41000000;
    localparam logic [31:0] FP32_SIXTEEN = 32'h41800000;


    /*
     * ============================================================
     * Saved 24x24 context results
     * ============================================================
     */

    logic [31:0] C_ctx0 [0:23][0:23];
    logic [31:0] C_ctx1 [0:23][0:23];

    logic ctx0_seen;
    logic ctx1_seen;

    logic clear_seen;


    /*
     * ============================================================
     * Capture completion of both result contexts
     * ============================================================
     */

    always_ff @(posedge clk) begin

        if (rst || clear_seen) begin

            ctx0_seen <= 1'b0;
            ctx1_seen <= 1'b0;

        end
        else if (c_valid_out) begin

            if (c_ctx_out == 1'b0) begin

                ctx0_seen <= 1'b1;

                $display(
                    "CTX0 RESULT RECEIVED time=%0t",
                    $time
                );

            end
            else begin

                ctx1_seen <= 1'b1;

                $display(
                    "CTX1 RESULT RECEIVED time=%0t",
                    $time
                );

            end

        end

    end


    /*
     * ============================================================
     * Run one 8x8 output tile
     *
     * tm = output tile row
     * tn = output tile column
     *
     * tm,tn = 0..2
     *
     * Each output tile computes:
     *
     *   8x24 * 24x8
     *
     * using three continuous K=8 folds.
     * ============================================================
     */

    task automatic run_tile(
        input integer tm,
        input integer tn
    );

        begin

            $display("");
            $display(
                "========================================"
            );

            $display(
                "START TILE C%0d%0d",
                tm,
                tn
            );

            $display(
                "========================================"
            );


            /*
             * Clear sticky testbench completion flags
             * from the previous tile.
             */
            @(negedge clk);

            clear_seen =
                1'b1;


            @(negedge clk);

            clear_seen =
                1'b0;


            /*
             * ----------------------------------------------------
             * Feed K=24 continuously
             *
             * Last global k = 23
             * Maximum systolic skew = 7
             *
             * Last feed cycle:
             *
             *   23 + 7 = 30
             * ----------------------------------------------------
             */

            for (
                int feed_t = 0;
                feed_t <= 30;
                feed_t++
            ) begin

                @(negedge clk);

                /*
                 * =================================================
                 * Fold-pipeline debug
                 *
                 * K=24:
                 *
                 *   feed_t = 0  -> fold0 -> ctx0
                 *   feed_t = 8  -> fold1 -> ctx1
                 *   feed_t = 16 -> fold2 -> ctx0
                 *
                 * These messages verify that folds are injected
                 * continuously without waiting for reduction.
                 * =================================================
                 */
                if (
                    feed_t == 0  ||
                    feed_t == 8  ||
                    feed_t == 16
                ) begin

                    $display(
                        "FOLD_FEED tile=C%0d%0d fold=%0d ctx=%0d feed_t=%0d time=%0t",
                        tm,
                        tn,
                        feed_t >> 3,
                        (feed_t >> 3) & 1,
                        feed_t,
                        $time
                    );

                end


                /*
                 * Default boundary values.
                 */
                for (int i = 0; i < 8; i++) begin

                    a_in[i] =
                        32'd0;

                    b_in[i] =
                        32'd0;

                    a_valid_in[i] =
                        1'b0;

                    b_valid_in[i] =
                        1'b0;

                    fold_ctx_in_a[i] =
                        1'b0;

                    fold_ctx_in_b[i] =
                        1'b0;

                end


                /*
                 * =================================================
                 * A-side skew
                 * =================================================
                 */

                for (int r = 0; r < 8; r++) begin

                    integer gk_a;
                    integer fold_a;


                    gk_a =
                        feed_t - r;


                    if (
                        (gk_a >= 0) &&
                        (gk_a < 24)
                    ) begin

                        fold_a =
                            gk_a >> 3;


                        /*
                         * tm selects which global 8-row block
                         * feeds the array.
                         *
                         * tm=0 -> rows  0..7
                         * tm=1 -> rows  8..15
                         * tm=2 -> rows 16..23
                         */
                        a_in[r] =
                            A[
                                tm * 8 + r
                            ][
                                gk_a
                            ];


                        a_valid_in[r] =
                            1'b1;


                        /*
                         * fold0 -> ctx0
                         * fold1 -> ctx1
                         * fold2 -> ctx0
                         */
                        fold_ctx_in_a[r] =
                            fold_a[0];

                    end

                end


                /*
                 * =================================================
                 * B-side skew
                 * =================================================
                 */

                for (int c = 0; c < 8; c++) begin

                    integer gk_b;
                    integer fold_b;


                    gk_b =
                        feed_t - c;


                    if (
                        (gk_b >= 0) &&
                        (gk_b < 24)
                    ) begin

                        fold_b =
                            gk_b >> 3;


                        /*
                         * tn selects which global 8-column block
                         * feeds the array.
                         *
                         * tn=0 -> cols  0..7
                         * tn=1 -> cols  8..15
                         * tn=2 -> cols 16..23
                         */
                        b_in[c] =
                            B[
                                gk_b
                            ][
                                tn * 8 + c
                            ];


                        b_valid_in[c] =
                            1'b1;


                        fold_ctx_in_b[c] =
                            fold_b[0];

                    end

                end

            end


            /*
             * ----------------------------------------------------
             * Stop input injection.
             * ----------------------------------------------------
             */

            @(negedge clk);

            for (int i = 0; i < 8; i++) begin

                a_in[i] =
                    32'd0;

                b_in[i] =
                    32'd0;

                a_valid_in[i] =
                    1'b0;

                b_valid_in[i] =
                    1'b0;

                fold_ctx_in_a[i] =
                    1'b0;

                fold_ctx_in_b[i] =
                    1'b0;

            end


            /*
             * ----------------------------------------------------
             * Wait until the array publishes both contexts.
             * ----------------------------------------------------
             */

            wait (
                ctx0_seen &&
                ctx1_seen
            );


            /*
             * Wait one more clock so all registered outputs
             * are fully settled.
             */
            @(posedge clk);


            /*
             * ----------------------------------------------------
             * Store this 8x8 tile into the corresponding
             * 24x24 output locations.
             *
             * At this point PE scalar result registers contain
             * the reduced ctx0 / ctx1 values.
             * ----------------------------------------------------
             */

            for (int r = 0; r < 8; r++) begin

                for (int c = 0; c < 8; c++) begin

                    C_ctx0[
                        tm * 8 + r
                    ][
                        tn * 8 + c
                    ] =
                        dut.pe_result_ctx0[r][c];


                    C_ctx1[
                        tm * 8 + r
                    ][
                        tn * 8 + c
                    ] =
                        dut.pe_result_ctx1[r][c];

                end

            end


            $display(
                "TILE C%0d%0d DONE",
                tm,
                tn
            );


            /*
             * Let the array output FSM re-arm before
             * starting the next tile.
             */
            repeat (5)
                @(posedge clk);

        end

    endtask


    /*
     * ============================================================
     * Main stimulus
     * ============================================================
     */

    initial begin

        integer pass_count;


        /*
         * --------------------------------------------------------
         * Initial state
         * --------------------------------------------------------
         */

        rst =
            1'b1;

        clear_seen =
            1'b0;

        pass_count =
            0;


        for (int i = 0; i < 8; i++) begin

            a_in[i] =
                32'd0;

            b_in[i] =
                32'd0;

            a_valid_in[i] =
                1'b0;

            b_valid_in[i] =
                1'b0;

            fold_ctx_in_a[i] =
                1'b0;

            fold_ctx_in_b[i] =
                1'b0;

        end


        /*
         * --------------------------------------------------------
         * Fill complete 24x24 matrices with FP32 1.0.
         * --------------------------------------------------------
         */

        for (int r = 0; r < 24; r++) begin

            for (int c = 0; c < 24; c++) begin

                A[r][c] =
                    FP32_ONE;

                B[r][c] =
                    FP32_ONE;

                C_ctx0[r][c] =
                    32'd0;

                C_ctx1[r][c] =
                    32'd0;

            end

        end


        /*
         * --------------------------------------------------------
         * Reset DUT.
         * --------------------------------------------------------
         */

        repeat (10)
            @(posedge clk);


        rst =
            1'b0;


        repeat (5)
            @(posedge clk);


        /*
         * ========================================================
         * Compute all nine 8x8 output tiles
         *
         *        tn
         *       0    1    2
         *
         * tm 0 C00  C01  C02
         *    1 C10  C11  C12
         *    2 C20  C21  C22
         * ========================================================
         */

        for (int tm = 0; tm < 3; tm++) begin

            for (int tn = 0; tn < 3; tn++) begin

                run_tile(
                    tm,
                    tn
                );

            end

        end


        /*
         * ========================================================
         * Verify all 576 output elements
         * ========================================================
         */

        for (int r = 0; r < 24; r++) begin

            for (int c = 0; c < 24; c++) begin


                /*
                 * Even folds:
                 *
                 * fold0 + fold2
                 *
                 * 8 + 8 = 16
                 */
                if (
                    C_ctx0[r][c] !==
                    FP32_SIXTEEN
                ) begin

                    $display(
                        "FAIL ctx0 C[%0d][%0d] expected=%h got=%h",
                        r,
                        c,
                        FP32_SIXTEEN,
                        C_ctx0[r][c]
                    );

                    $fatal;

                end


                /*
                 * Odd fold:
                 *
                 * fold1 = 8
                 */
                if (
                    C_ctx1[r][c] !==
                    FP32_EIGHT
                ) begin

                    $display(
                        "FAIL ctx1 C[%0d][%0d] expected=%h got=%h",
                        r,
                        c,
                        FP32_EIGHT,
                        C_ctx1[r][c]
                    );

                    $fatal;

                end


                pass_count =
                    pass_count + 1;

            end

        end


        /*
         * ========================================================
         * PASS
         * ========================================================
         */

        $display("");
        $display(
            "=============================================="
        );

        $display(
            "FULL 24x24 GEMM TILED ACCUMULATION TEST PASS"
        );

        $display(
            "=============================================="
        );

        $display(
            "9 output tiles passed"
        );

        $display(
            "%0d output elements checked",
            pass_count
        );

        $display(
            "ctx0 = fold0 + fold2 = 16.0"
        );

        $display(
            "ctx1 = fold1         =  8.0"
        );

        $display(
            "mathematical final   = 24.0"
        );

        $display(
            "=============================================="
        );

        $display("");

        $finish;

    end


    /*
     * ============================================================
     * Timeout
     * ============================================================
     */

    initial begin

        #5_000_000;

        $display("");
        $display("TIMEOUT");
        $display("");

        $fatal;

    end


endmodule
