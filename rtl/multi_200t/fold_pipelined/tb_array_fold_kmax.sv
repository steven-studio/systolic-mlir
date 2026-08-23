`timescale 1ns/1ps

// tb_array_fold_kmax.sv -- parameterised correctness + cycle-count bench for
// systolic_array_8x8_fold at an arbitrary reduction depth K.
//
//   xelab tb_array_fold_kmax -generic_top "K=64" -s sim && xsim sim -R
//
// Generalises tb_array_fold_24x24.sv, which hardcodes K=24. The array itself
// has no K parameter -- K is purely how long the feeder asserts valid -- so
// this bench needs no RTL change to sweep K, and the cycle numbers it prints
// are available today, before any synthesis run.
//
// WHAT IT MEASURES
//
//   feed_cycles     first valid beat -> last valid beat        (= K + 7)
//   drain_cycles    last valid beat  -> ctx1 published
//   total_cycles    first valid beat -> ctx1 published
//
// drain_cycles is the interesting column. The per-PE final reduction walks
// ACC_BANKS(16) banks for each of 2 contexts through one shared FP adder,
// waiting on add_valid each time, so it costs roughly 32 * add_latency and
// is INDEPENDENT of K. That fixed cost is what a larger k_max amortises;
// it is the whole reason the sweep has a knee.
//
// CORRECTNESS
//
// A = B = 1.0 everywhere, so PE(r,c) accumulates 1.0 per k. With NF = K/8
// folds mapped onto 2 contexts (even folds -> ctx0, odd -> ctx1):
//
//   ctx0 = ceil(NF/2) * 8
//   ctx1 = floor(NF/2) * 8
//   ctx0 + ctx1 = K
//
// At K=24 that is 16.0 / 8.0, matching tb_array_fold_24x24.

module tb_array_fold_kmax;

    // Reduction depth. Must be a positive multiple of 8.
    parameter int K = 64;

    localparam int NF = K / 8;

    localparam int EXP_CTX0 = ((NF + 1) / 2) * 8;   // even folds
    localparam int EXP_CTX1 = (NF / 2) * 8;         // odd folds

    logic clk = 1'b0;
    logic rst;

    always #5 clk = ~clk;                            // 100 MHz

    logic [31:0] a_in [0:7];
    logic [31:0] b_in [0:7];

    logic a_valid_in [0:7];
    logic b_valid_in [0:7];

    logic fold_ctx_in_a [0:7];
    logic fold_ctx_in_b [0:7];

    logic        c_valid_out;
    logic        c_ctx_out;
    logic [31:0] c_out [0:7][0:7];

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

    localparam logic [31:0] FP32_ONE = 32'h3f800000;

    logic [31:0] exp_ctx0;
    logic [31:0] exp_ctx1;

    logic [31:0] C_ctx0 [0:7][0:7];
    logic [31:0] C_ctx1 [0:7][0:7];

    logic ctx0_seen;
    logic ctx1_seen;

    // -------------------------------------------------------------------
    // Free-running cycle counter and event timestamps
    // -------------------------------------------------------------------
    integer cycle;
    integer first_valid_cycle;
    integer last_valid_cycle;
    integer ctx1_cycle;

    always_ff @(posedge clk) begin
        if (rst) cycle <= 0;
        else     cycle <= cycle + 1;
    end

    // -------------------------------------------------------------------
    // Capture both published contexts
    // -------------------------------------------------------------------
    // ctx1_cycle is reset here rather than in the initial block: driving
    // the same variable from both an initial and an always_ff is a
    // multiple-driver error (VRFC 10-3818 / 10-2921), and while xsim
    // happened to give the intended result, the behaviour is not defined
    // by the standard.
    always_ff @(posedge clk) begin
        if (rst) begin
            ctx0_seen  <= 1'b0;
            ctx1_seen  <= 1'b0;
            ctx1_cycle <= -1;
        end
        else if (c_valid_out) begin
            if (c_ctx_out == 1'b0) begin
                for (int r = 0; r < 8; r++)
                    for (int c = 0; c < 8; c++)
                        C_ctx0[r][c] <= c_out[r][c];
                ctx0_seen <= 1'b1;
            end
            else begin
                for (int r = 0; r < 8; r++)
                    for (int c = 0; c < 8; c++)
                        C_ctx1[r][c] <= c_out[r][c];
                ctx1_seen  <= 1'b1;
                ctx1_cycle <= cycle;
            end
        end
    end

    task automatic clear_inputs();
        for (int i = 0; i < 8; i++) begin
            a_in[i]          = 32'd0;
            b_in[i]          = 32'd0;
            a_valid_in[i]    = 1'b0;
            b_valid_in[i]    = 1'b0;
            fold_ctx_in_a[i] = 1'b0;
            fold_ctx_in_b[i] = 1'b0;
        end
    endtask

    // -------------------------------------------------------------------
    // Feed K beats with the same skew the RTL top uses:
    //   global_k = t - lane,  fold = global_k >> 3,  ctx = fold[0]
    // -------------------------------------------------------------------
    task automatic feed_k();
        integer gk;
        integer fold;
        logic   any_valid;

        for (int t = 0; t < K + 7; t++) begin
            @(negedge clk);
            clear_inputs();
            any_valid = 1'b0;

            for (int r = 0; r < 8; r++) begin
                gk = t - r;
                if (gk >= 0 && gk < K) begin
                    fold             = gk >> 3;
                    a_in[r]          = FP32_ONE;
                    a_valid_in[r]    = 1'b1;
                    fold_ctx_in_a[r] = fold[0];
                    any_valid        = 1'b1;
                end
            end

            for (int c = 0; c < 8; c++) begin
                gk = t - c;
                if (gk >= 0 && gk < K) begin
                    fold             = gk >> 3;
                    b_in[c]          = FP32_ONE;
                    b_valid_in[c]    = 1'b1;
                    fold_ctx_in_b[c] = fold[0];
                    any_valid        = 1'b1;
                end
            end

            if (any_valid) begin
                if (first_valid_cycle < 0) first_valid_cycle = cycle;
                last_valid_cycle = cycle;
            end
        end

        @(negedge clk);
        clear_inputs();
    endtask

    integer errors;

    initial begin
        errors            = 0;
        first_valid_cycle = -1;
        last_valid_cycle  = -1;
        // ctx1_cycle is reset inside the always_ff above -- see note there.

        if (K <= 0 || (K % 8) != 0) begin
            $display("FAIL: K must be a positive multiple of 8 (got %0d)", K);
            $fatal(1);
        end

        exp_ctx0 = $shortrealtobits(shortreal'(EXP_CTX0));
        exp_ctx1 = $shortrealtobits(shortreal'(EXP_CTX1));

        rst = 1'b1;
        clear_inputs();
        repeat (10) @(posedge clk);
        rst = 1'b0;
        repeat (5)  @(posedge clk);

        feed_k();

        wait (ctx0_seen && ctx1_seen);
        @(posedge clk);

        // ---------------------------------------------------------------
        // Correctness
        // ---------------------------------------------------------------
        for (int r = 0; r < 8; r++) begin
            for (int c = 0; c < 8; c++) begin
                if (C_ctx0[r][c] !== exp_ctx0) begin
                    $display("FAIL ctx0 C[%0d][%0d] expected=%h got=%h",
                             r, c, exp_ctx0, C_ctx0[r][c]);
                    errors++;
                end
                if (C_ctx1[r][c] !== exp_ctx1) begin
                    $display("FAIL ctx1 C[%0d][%0d] expected=%h got=%h",
                             r, c, exp_ctx1, C_ctx1[r][c]);
                    errors++;
                end
            end
        end

        // ---------------------------------------------------------------
        // Report. The KMAXCSV line is what collect_kmax.py parses.
        // ---------------------------------------------------------------
        $display("");
        $display("==============================================");
        $display(" K                = %0d  (%0d folds)", K, NF);
        $display(" expected ctx0    = %0d.0", EXP_CTX0);
        $display(" expected ctx1    = %0d.0", EXP_CTX1);
        $display(" ctx0 + ctx1      = %0d", EXP_CTX0 + EXP_CTX1);
        $display("----------------------------------------------");
        $display(" first valid beat = %0d", first_valid_cycle);
        $display(" last  valid beat = %0d", last_valid_cycle);
        $display(" ctx1 published   = %0d", ctx1_cycle);
        $display(" feed  cycles     = %0d",
                 last_valid_cycle - first_valid_cycle + 1);
        $display(" drain cycles     = %0d", ctx1_cycle - last_valid_cycle);
        $display(" TOTAL cycles     = %0d",
                 ctx1_cycle - first_valid_cycle + 1);
        $display("==============================================");

        // k,feed_cycles,drain_cycles,total_cycles,errors
        $display("KMAXCSV,%0d,%0d,%0d,%0d,%0d",
                 K,
                 last_valid_cycle - first_valid_cycle + 1,
                 ctx1_cycle - last_valid_cycle,
                 ctx1_cycle - first_valid_cycle + 1,
                 errors);

        if (errors == 0) $display("PASS: K=%0d fold accumulation correct", K);
        else             $display("FAIL: %0d mismatches at K=%0d", errors, K);

        if (errors != 0) $fatal(1);
        $finish;
    end

    initial begin
        #20_000_000;
        $display("TIMEOUT at K=%0d -- array never published both contexts", K);
        $fatal(1);
    end

endmodule