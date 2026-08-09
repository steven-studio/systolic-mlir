`timescale 1ns/1ps

module tb_array_fold_result_8x8;

    localparam int DATA_W = 32;

    logic clk;
    logic rst;

    logic [DATA_W-1:0] a_in [0:7];
    logic [DATA_W-1:0] b_in [0:7];

    logic a_valid_in [0:7];
    logic b_valid_in [0:7];

    logic fold_ctx_in_a [0:7];
    logic fold_ctx_in_b [0:7];

    logic c_valid_out;
    logic c_ctx_out;
    logic [DATA_W-1:0] c_out [0:7][0:7];

    integer errors;
    integer result_count;


    /*
     * DUT
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
     * 100 MHz clock
     */
    initial begin
        clk = 1'b0;
        forever #5 clk = ~clk;
    end


    /*
     * Convert integer -> FP32.
     *
     * Values used by this test are small positive integers,
     * so $shortrealtobits is sufficient.
     */
    function automatic logic [31:0] fp32(input shortreal x);
        fp32 = $shortrealtobits(x);
    endfunction


    /*
     * Drive one global systolic time step.
     *
     * global_k:
     *
     *   0..7  -> ctx0
     *   8..15 -> ctx1
     *
     * Row/column skew is generated here exactly like the
     * top-level feeder.
     */
    task automatic drive_cycle(input integer feed_t);

        integer r;
        integer c;
        integer gk;
        integer fold_id;
        integer k_idx;
        shortreal aval;
        shortreal bval;

        begin

            for (r = 0; r < 8; r = r + 1) begin
                a_in[r]          = 32'd0;
                a_valid_in[r]    = 1'b0;
                fold_ctx_in_a[r] = 1'b0;
            end

            for (c = 0; c < 8; c = c + 1) begin
                b_in[c]          = 32'd0;
                b_valid_in[c]    = 1'b0;
                fold_ctx_in_b[c] = 1'b0;
            end


            /*
             * A-side skew
             */
            for (r = 0; r < 8; r = r + 1) begin

                gk = feed_t - r;

                if ((gk >= 0) && (gk < 16)) begin

                    fold_id = gk >> 3;
                    k_idx   = gk & 7;

                    /*
                     * ctx0 A = I
                     * ctx1 A = 2I
                     */
                    if (r == k_idx) begin

                        if (fold_id == 0)
                            aval = 1.0;
                        else
                            aval = 2.0;

                    end
                    else begin
                        aval = 0.0;
                    end

                    a_in[r] = fp32(aval);

                    a_valid_in[r]    = 1'b1;
                    fold_ctx_in_a[r] = fold_id;

                end

            end


            /*
             * B-side skew
             *
             * B[k][c] = k*8 + c + 1
             */
            for (c = 0; c < 8; c = c + 1) begin

                gk = feed_t - c;

                if ((gk >= 0) && (gk < 16)) begin

                    fold_id = gk >> 3;
                    k_idx   = gk & 7;

                    bval = shortreal'(k_idx * 8 + c + 1);

                    b_in[c] = fp32(bval);

                    b_valid_in[c]    = 1'b1;
                    fold_ctx_in_b[c] = fold_id;

                end

            end

            if (feed_t >= 0 && feed_t <= 16) begin
                $display(
                    "FEED t=%0d | A0=%f av=%b actx=%b | B0=%f bv=%b bctx=%b",
                    feed_t,
                    $bitstoshortreal(a_in[0]),
                    a_valid_in[0],
                    fold_ctx_in_a[0],
                    $bitstoshortreal(b_in[0]),
                    b_valid_in[0],
                    fold_ctx_in_b[0]
                );
            end

            @(negedge clk);

        end

    endtask


    /*
     * Check one completed matrix.
     */
    task automatic check_result(input integer ctx);

        integer r;
        integer c;

        shortreal got;
        shortreal expected;
        shortreal diff;

        begin

            $display("");
            $display(
                "===== RESULT ctx=%0d time=%0t =====",
                ctx,
                $time
            );

            for (r = 0; r < 8; r = r + 1) begin

                for (c = 0; c < 8; c = c + 1) begin

                    got = $bitstoshortreal(c_out[r][c]);

                    if (ctx == 0)
                        expected =
                            shortreal'(r * 8 + c + 1);
                    else
                        expected =
                            shortreal'(2 * (r * 8 + c + 1));

                    diff = got - expected;

                    if (diff < 0.0)
                        diff = -diff;

                    if (diff > 0.001) begin

                        $display(
                            "FAIL ctx=%0d C[%0d][%0d] got=%f expected=%f bits=%h",
                            ctx,
                            r,
                            c,
                            got,
                            expected,
                            c_out[r][c]
                        );

                        errors = errors + 1;

                    end

                end

            end

            if (errors == 0)
                $display("ctx%0d matrix PASS", ctx);

        end

    endtask


    /*
     * Capture completed contexts.
     */
    always @(posedge clk) begin

        if (!rst && c_valid_out) begin

            #1;

            if (c_ctx_out == 1'b0) begin
                $display("ctx0 PE[0][0] banks:");

                for (int i = 0; i < 16; i = i + 1) begin
                    $display(
                        "bank[%0d] = %h (%f)",
                        i,
                        dut.acc_ctx0[0][0][i],
                        $bitstoshortreal(dut.acc_ctx0[0][0][i])
                    );
                end
            end

            $display(
                "c_valid_out: ctx=%0d time=%0t",
                c_ctx_out,
                $time
            );

            check_result(c_ctx_out);

            result_count = result_count + 1;

        end

    end

    integer dbg_cycle = 0;

    always @(posedge clk) begin
        if (!rst) begin
            dbg_cycle = dbg_cycle + 1;

            /*
            * PE00 input/capture state.
            * Print every cycle so we can see exactly when
            * pair_valid causes local_acc_sel to advance.
            */
            if (dut.ROW[0].COL[0].u_pe.pair_valid) begin
                $display(
                    "PE00_ALLOC cycle=%0d ctx=%0d sel=%0d a=%f b=%f local0=%0d local1=%0d",
                    dbg_cycle,
                    dut.ROW[0].COL[0].u_pe.fold_ctx_in,
                    dut.ROW[0].COL[0].u_pe.sel_reg,
                    $bitstoshortreal(dut.ROW[0].COL[0].u_pe.a_in),
                    $bitstoshortreal(dut.ROW[0].COL[0].u_pe.b_in),
                    dut.ROW[0].COL[0].u_pe.local_acc_sel[0],
                    dut.ROW[0].COL[0].u_pe.local_acc_sel[1]
                );
            end

            if (dut.ROW[0].COL[0].u_pe.product_valid) begin
                #1;
                $display(
                    "MUL cycle=%0d product=%f sel=%0d ctx=%0d",
                    dbg_cycle,
                    $bitstoshortreal(
                        dut.ROW[0].COL[0].u_pe.product
                    ),
                    dut.ROW[0].COL[0].u_pe.product_sel,
                    dut.ROW[0].COL[0].u_pe.product_ctx
                );
            end

            if (dut.ROW[0].COL[0].u_pe.add_valid) begin
                #1;
                $display(
                    "WB cycle=%0d add=%f wb_sel=%0d wb_ctx=%0d product_ctx=%0d addctx10=%0d addctx11=%0d",
                    dbg_cycle,
                    $bitstoshortreal(
                        dut.ROW[0].COL[0].u_pe.add_result
                    ),
                    dut.ROW[0].COL[0].u_pe.writeback_sel,
                    dut.ROW[0].COL[0].u_pe.writeback_ctx,
                    dut.ROW[0].COL[0].u_pe.product_ctx,
                    dut.ROW[0].COL[0].u_pe.add_ctx_pipe[10],
                    dut.ROW[0].COL[0].u_pe.add_ctx_pipe[11]
                );
            end
        end
    end

    /*
     * Main test
     */
    initial begin

        integer t;

        errors       = 0;
        result_count = 0;

        rst = 1'b1;

        for (int i = 0; i < 8; i = i + 1) begin
            a_in[i]          = 32'd0;
            b_in[i]          = 32'd0;
            a_valid_in[i]    = 1'b0;
            b_valid_in[i]    = 1'b0;
            fold_ctx_in_a[i] = 1'b0;
            fold_ctx_in_b[i] = 1'b0;
        end


        repeat (10)
            @(posedge clk);

        rst = 1'b0;

        @(posedge clk);


        $display("");
        $display("================================");
        $display("Starting two-fold pipeline test");
        $display("ctx0: A=I,  B=1..64");
        $display("ctx1: A=2I, B=1..64");
        $display("================================");
        $display("");


        for (t = 0; t <= 22; t = t + 1)
            drive_cycle(t);

        /*
        * Stop feeding immediately after the final feed cycle.
        */
        for (int i = 0; i < 8; i = i + 1) begin
            a_in[i]          = 32'd0;
            b_in[i]          = 32'd0;
            a_valid_in[i]    = 1'b0;
            b_valid_in[i]    = 1'b0;
            fold_ctx_in_a[i] = 1'b0;
            fold_ctx_in_b[i] = 1'b0;
        end

        /*
        * Now let PE FP pipelines drain.
        */
        repeat (100)
            @(posedge clk);


        $display("===== PE77 FINAL ACC =====");

        for (int dbg_b77 = 0; dbg_b77 < 16; dbg_b77 = dbg_b77 + 1) begin
            $display(
                "PE77_ACC bank=%0d ctx0=%f ctx1=%f",
                dbg_b77,
                $bitstoshortreal(
                    dut.ROW[7].COL[7].u_pe.dbg_acc_ctx0[dbg_b77]
                ),
                $bitstoshortreal(
                    dut.ROW[7].COL[7].u_pe.dbg_acc_ctx1[dbg_b77]
                )
            );
        end

        /*
         * MUL/ADD + reduction need time to drain.
         * Give it plenty of cycles for this functional test.
         */
        repeat (100000) begin

            @(posedge clk);

            if (result_count == 2) begin

                if (errors == 0) begin
                    $display("");
                    $display("============================");
                    $display("ALL FOLD RESULT TESTS PASS");
                    $display("============================");
                end
                else begin
                    $display("");
                    $display(
                        "TEST FAILED: %0d errors",
                        errors
                    );
                end

                $finish;

            end

        end


        $display("");
        $display(
            "TIMEOUT: received %0d/2 results",
            result_count
        );

        $finish;

    end


    // ============================================================
    // DEBUG: PE(0,0) multiplier input transaction
    // ============================================================
    always @(posedge clk) begin
        if (!rst) begin
            #1;
            if (dut.ROW[0].COL[0].u_pe.a_valid_reg &&
                dut.ROW[0].COL[0].u_pe.b_valid_reg) begin
                $display(
                    "PE00_MULIN cycle=%0d a=%f b=%f sel=%0d ctx=%0d",
                    dbg_cycle,
                    $bitstoshortreal(dut.ROW[0].COL[0].u_pe.a_reg),
                    $bitstoshortreal(dut.ROW[0].COL[0].u_pe.b_reg),
                    dut.ROW[0].COL[0].u_pe.sel_reg,
                    dut.ROW[0].COL[0].u_pe.fold_ctx_reg
                );
            end
        end
    end

    // ============================================================
    // DEBUG: PE(7,7) multiplier input transaction
    // ============================================================
    always @(posedge clk) begin
        if (!rst) begin
            #1;
            if (dut.ROW[7].COL[7].u_pe.a_valid_reg &&
                dut.ROW[7].COL[7].u_pe.b_valid_reg) begin
                $display(
                    "PE77_MULIN cycle=%0d a=%f b=%f sel=%0d ctx=%0d",
                    dbg_cycle,
                    $bitstoshortreal(dut.ROW[7].COL[7].u_pe.a_reg),
                    $bitstoshortreal(dut.ROW[7].COL[7].u_pe.b_reg),
                    dut.ROW[7].COL[7].u_pe.sel_reg,
                    dut.ROW[7].COL[7].u_pe.fold_ctx_reg
                );
            end
        end
    end

    // ============================================================
    // DEBUG: PE77 valid inputs and upstream sources
    // ============================================================
    always @(posedge clk) begin
        if (!rst && dbg_cycle >= 25 && dbg_cycle <= 45) begin
            #1;
            $display(
                "PE77_VALID cycle=%0d ain=%b bin=%b areg=%b breg=%b | PE76_aout=%b PE67_bout=%b",
                dbg_cycle,

                dut.ROW[7].COL[7].u_pe.a_valid_in,
                dut.ROW[7].COL[7].u_pe.b_valid_in,

                dut.ROW[7].COL[7].u_pe.a_valid_reg,
                dut.ROW[7].COL[7].u_pe.b_valid_reg,

                dut.ROW[7].COL[6].u_pe.a_valid_out,
                dut.ROW[6].COL[7].u_pe.b_valid_out
            );
        end
    end

    // ============================================================
    // DEBUG: entire valid chains feeding PE(7,7)
    // ============================================================
    always @(posedge clk) begin
        if (!rst && dbg_cycle >= 20 && dbg_cycle <= 45) begin
            #1;

            $display(
                "VCHAIN cycle=%0d | A7=%b%b%b%b%b%b%b%b%b | B7=%b%b%b%b%b%b%b%b%b",
                dbg_cycle,

                dut.a_valid_bus[7][0],
                dut.a_valid_bus[7][1],
                dut.a_valid_bus[7][2],
                dut.a_valid_bus[7][3],
                dut.a_valid_bus[7][4],
                dut.a_valid_bus[7][5],
                dut.a_valid_bus[7][6],
                dut.a_valid_bus[7][7],
                dut.a_valid_bus[7][8],

                dut.b_valid_bus[0][7],
                dut.b_valid_bus[1][7],
                dut.b_valid_bus[2][7],
                dut.b_valid_bus[3][7],
                dut.b_valid_bus[4][7],
                dut.b_valid_bus[5][7],
                dut.b_valid_bus[6][7],
                dut.b_valid_bus[7][7],
                dut.b_valid_bus[8][7]
            );
        end
    end

    always @(posedge clk) begin
        if (!rst && dbg_cycle >= 18 && dbg_cycle <= 35) begin
            #1;

            $display(
                "BOUNDARY cycle=%0d | A0=%b A1=%b A2=%b A3=%b A4=%b A5=%b A6=%b A7=%b | B0=%b B1=%b B2=%b B3=%b B4=%b B5=%b B6=%b B7=%b",
                dbg_cycle,

                a_valid_in[0],
                a_valid_in[1],
                a_valid_in[2],
                a_valid_in[3],
                a_valid_in[4],
                a_valid_in[5],
                a_valid_in[6],
                a_valid_in[7],

                b_valid_in[0],
                b_valid_in[1],
                b_valid_in[2],
                b_valid_in[3],
                b_valid_in[4],
                b_valid_in[5],
                b_valid_in[6],
                b_valid_in[7]
            );
        end
    end
endmodule
