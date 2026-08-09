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


    initial begin
        clk = 1'b0;
        forever #5 clk = ~clk;
    end


    function automatic logic [31:0] fp32(input shortreal x);
        fp32 = $shortrealtobits(x);
    endfunction


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
             * A-side skew.
             *
             * ctx0: A = I
             * ctx1: A = 2I
             */
            for (r = 0; r < 8; r = r + 1) begin

                gk = feed_t - r;

                if ((gk >= 0) && (gk < 16)) begin

                    fold_id = gk >> 3;
                    k_idx   = gk & 7;

                    if (r == k_idx) begin

                        if (fold_id == 0)
                            aval = 1.0;
                        else
                            aval = 2.0;

                    end
                    else begin

                        aval = 0.0;

                    end

                    a_in[r] =
                        fp32(aval);

                    a_valid_in[r] =
                        1'b1;

                    fold_ctx_in_a[r] =
                        fold_id;

                end

            end


            /*
             * B-side skew.
             *
             * B[k][c] = k*8 + c + 1
             */
            for (c = 0; c < 8; c = c + 1) begin

                gk = feed_t - c;

                if ((gk >= 0) && (gk < 16)) begin

                    fold_id = gk >> 3;
                    k_idx   = gk & 7;

                    bval =
                        shortreal'(k_idx * 8 + c + 1);

                    b_in[c] =
                        fp32(bval);

                    b_valid_in[c] =
                        1'b1;

                    fold_ctx_in_b[c] =
                        fold_id;

                end

            end

            @(negedge clk);

        end

    endtask


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

                    got =
                        $bitstoshortreal(c_out[r][c]);

                    if (ctx == 0)
                        expected =
                            shortreal'(r * 8 + c + 1);
                    else
                        expected =
                            shortreal'(
                                2 * (r * 8 + c + 1)
                            );

                    diff =
                        got - expected;

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

                        errors =
                            errors + 1;

                    end

                end

            end

            if (errors == 0)
                $display(
                    "ctx%0d matrix PASS",
                    ctx
                );

        end

    endtask


    /*
     * Only observe the public array interface.
     */
    always @(posedge clk) begin

        if (!rst && c_valid_out) begin

            #1;

            check_result(c_ctx_out);

            result_count =
                result_count + 1;

        end

    end

    always @(posedge clk) begin

        if (!rst) begin

            if (dut.pe_result_valid[0][0])
                $display(
                    "PE00 RESULT VALID time=%0t",
                    $time
                );

            if (dut.pe_result_valid[7][7])
                $display(
                    "PE77 RESULT VALID time=%0t",
                    $time
                );

        end

    end


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

        @(negedge clk);
        rst = 1'b0;


        $display("");
        $display("================================");
        $display("Starting two-fold pipeline test");
        $display("ctx0: A=I,  B=1..64");
        $display("ctx1: A=2I, B=1..64");
        $display("================================");
        $display("");


        /*
         * global k = 0..15
         * plus maximum row/column skew = 7
         *
         * therefore feed_t = 0..22.
         */
        for (t = 0; t <= 22; t = t + 1)
            drive_cycle(t);


        /*
         * Stop feeding immediately.
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
         * Wait for both output contexts.
         */
        repeat (1000) begin

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

        $display("===== PE RESULT STATUS =====");

        for (int rr = 0; rr < 8; rr = rr + 1) begin
            $write("row%0d: ", rr);

            for (int cc = 0; cc < 8; cc = cc + 1)
                $write("%0d ", dut.result_seen[rr][cc]);

            $display("");
        end

        $display(
            "PE01_DIAG state=%0d seen=%b finished=%b outstanding=%0d pair=%b pair_d=%b prodv=%b addv=%b tag=%b reduce_ctx=%b reduce_idx=%0d result_valid=%b",
            dut.ROW[0].COL[1].u_pe.state,
            dut.ROW[0].COL[1].u_pe.transaction_seen,
            dut.ROW[0].COL[1].u_pe.input_finished,
            dut.ROW[0].COL[1].u_pe.outstanding_adds,
            dut.ROW[0].COL[1].u_pe.pipe_pair_valid,
            dut.ROW[0].COL[1].u_pe.pipe_pair_valid_d,
            dut.ROW[0].COL[1].u_pe.product_valid,
            dut.ROW[0].COL[1].u_pe.add_valid,
            dut.ROW[0].COL[1].u_pe.add_result_is_accum,
            dut.ROW[0].COL[1].u_pe.reduce_ctx,
            dut.ROW[0].COL[1].u_pe.reduce_index,
            dut.ROW[0].COL[1].u_pe.result_valid
        );

        $display("");
        $display(
            "TIMEOUT: received %0d/2 results",
            result_count
        );

        $finish;

    end

    always @(posedge clk) begin
        if (!rst) begin

            if (dut.pe_result_valid[0][0])
                $display(
                    "PE00 RESULT VALID time=%0t",
                    $time
                );

            if (dut.pe_result_valid[7][7])
                $display(
                    "PE77 RESULT VALID time=%0t",
                    $time
                );

        end
    end

    always @(posedge clk) begin
        if (!rst && dut.pe_result_valid[7][7]) begin
            #1;

            $display("===== RESULT_SEEN WHEN PE77 DONE =====");

            for (int r = 0; r < 8; r = r + 1) begin
                $display(
                    "SEEN row=%0d : %b %b %b %b %b %b %b %b",
                    r,
                    dut.result_seen[r][0],
                    dut.result_seen[r][1],
                    dut.result_seen[r][2],
                    dut.result_seen[r][3],
                    dut.result_seen[r][4],
                    dut.result_seen[r][5],
                    dut.result_seen[r][6],
                    dut.result_seen[r][7]
                );
            end

            $display(
                "all_results_valid=%b",
                dut.all_results_valid
            );
        end
    end

    always @(posedge clk) begin
        if (!rst && dut.pe_result_valid[7][7]) begin
            #1;

            $display(
                "PE00_DIAG state=%0d seen=%b finished=%b outstanding=%0d pair=%b pair_d=%b prodv=%b addv=%b reduce_ctx=%b reduce_idx=%0d result_valid=%b",
                dut.ROW[0].COL[0].u_pe.state,
                dut.ROW[0].COL[0].u_pe.transaction_seen,
                dut.ROW[0].COL[0].u_pe.input_finished,
                dut.ROW[0].COL[0].u_pe.outstanding_adds,
                dut.ROW[0].COL[0].u_pe.pipe_pair_valid,
                dut.ROW[0].COL[0].u_pe.pipe_pair_valid_d,
                dut.ROW[0].COL[0].u_pe.product_valid,
                dut.ROW[0].COL[0].u_pe.add_valid,
                dut.ROW[0].COL[0].u_pe.reduce_ctx,
                dut.ROW[0].COL[0].u_pe.reduce_index,
                dut.ROW[0].COL[0].u_pe.result_valid
            );
        end
    end

endmodule