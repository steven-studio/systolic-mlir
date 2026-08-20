`timescale 1ns/1ps

// tb_uart_multi_invocation.sv -- drive systolic_uart_tile_top through a
// full ceil(K/K_MAX) invocation sequence over UART, check every partial
// result, and MEASURE the per-invocation cycle cost.
//
//   xelab tb_uart_multi_invocation -generic_top "K=65" -s sim && xsim sim -R
//   xelab tb_uart_multi_invocation -generic_top "K_MAX=64 K=100" ...
//
// WHY THIS EXISTS RATHER THAN THE HOST SCRIPT
//
// The design counts elapsed cycles in last_accel_cycles but never
// transmits them, so test_uart_multi_invocation.py can only predict the
// per-invocation overhead. In simulation the counter is directly
// readable, exactly as tb_uart_fold_top.sv already reads
// dut.accel_cycles. This bench is therefore the only thing that can
// currently confirm
//
//     cycles_i = k_dim_i + (rows + cols - 2) + c0
//
// on the real RTL rather than on paper.
//
// SCOPE
//
// The response is read from dut.C0/dut.C1 rather than by decoding the
// UART TX line. The TX serializer is unchanged by the k_dim work and is
// already covered by tb_uart_fold_top.sv; re-decoding it here would add
// bytes to check without adding a failure mode this bench is looking
// for. The bench does wait for TX to drain, because the main FSM only
// returns to ST_IDLE afterwards and the next invocation depends on it.

module tb_uart_multi_invocation;

    // Hardware capacity of the DUT, and the workload to decompose.
    parameter int K_MAX = 64;
    parameter int K     = 65;

    localparam int CLK_HZ       = 100_000_000;
    localparam int BAUD         = 10_000_000;      // fast, for sim only
    localparam int CLKS_PER_BIT = CLK_HZ / BAUD;
    localparam time CLK_PERIOD  = 10ns;
    localparam time BIT_PERIOD  = CLK_PERIOD * CLKS_PER_BIT;

    localparam int HDR_BYTES = 4;

    /*
     * RX framing. The DUT powers up in RX_HUNT and only leaves it when
     * the sliding four-byte window matches FRAME_START, so a request
     * without markers is never accepted and matrices_ready never fires.
     * This bench used to send bare header+payload, which meant it could
     * only ever reach the watchdog -- a hang that looks exactly like the
     * board symptom but is caused by the bench, not the design.
     *
     * Little-endian on the wire, matching the header's k_dim and
     * test_uart_kmax.py.
     */
    localparam logic [31:0] FRAME_START = 32'hA55A_C33C;
    localparam logic [31:0] FRAME_END   = 32'h5AA5_3CC3;

    localparam int ROWS = 8, COLS = 8;
    localparam int FILL_DRAIN = ROWS + COLS - 2;   // 14

    localparam int N_INV = (K + K_MAX - 1) / K_MAX;

    logic clk = 1'b0;
    logic rst;
    logic uart_rx_line = 1'b1;
    logic uart_tx_line;

    always #5 clk = ~clk;

    systolic_uart_tile_top #(
        .CLK_HZ        (CLK_HZ),
        .BAUD          (BAUD),
        .K_MAX         (K_MAX),
        .DEBUG_MARKERS (1'b0)
    ) dut (
        .clk     (clk),
        .rst     (rst),
        .uart_rx (uart_rx_line),
        .uart_tx (uart_tx_line)
    );

    // -------------------------------------------------------------------
    // Stimulus: small integers, so every product and partial sum is
    // exactly representable in float32 and the check can be bit-exact
    // regardless of summation order.
    // -------------------------------------------------------------------
    function automatic shortreal a_val(input int r, input int k);
        return shortreal'(((r + k) % 7) - 3);
    endfunction

    function automatic shortreal b_val(input int k, input int c);
        return shortreal'(((3 * k + c) % 5) - 2);
    endfunction

    // Poison for local k >= k_dim: large and non-cancelling, so a design
    // that ignores k_dim cannot accidentally produce the right answer.
    localparam shortreal POISON = 1024.0;

    // -------------------------------------------------------------------
    // UART TX (host -> DUT)
    // -------------------------------------------------------------------
    task automatic send_uart_byte(input logic [7:0] data);
        int i;
        begin
            uart_rx_line = 1'b0;                 // start
            #(BIT_PERIOD);
            for (i = 0; i < 8; i++) begin
                uart_rx_line = data[i];
                #(BIT_PERIOD);
            end
            uart_rx_line = 1'b1;                 // stop
            #(BIT_PERIOD);
        end
    endtask

    task automatic send_word_le(input logic [31:0] w);
        begin
            send_uart_byte(w[7:0]);
            send_uart_byte(w[15:8]);
            send_uart_byte(w[23:16]);
            send_uart_byte(w[31:24]);
        end
    endtask

    task automatic send_fp32(input shortreal x);
        logic [31:0] w;
        begin
            w = $shortrealtobits(x);
            send_uart_byte(w[7:0]);
            send_uart_byte(w[15:8]);
            send_uart_byte(w[23:16]);
            send_uart_byte(w[31:24]);
        end
    endtask

    // One request: 4-byte little-endian k_dim header, then the A/B
    // payload interleaved one 8-deep k window at a time. base_k is where
    // this invocation starts in the global reduction.
    task automatic send_request(input int k_dim, input int base_k);
        int w, r, c, lk;
        logic [31:0] hdr;
        begin
            send_word_le(FRAME_START);

            hdr = k_dim;
            send_word_le(hdr);

            for (w = 0; w < K_MAX / 8; w++) begin
                // A window: [row][local k]
                for (r = 0; r < 8; r++)
                    for (c = 0; c < 8; c++) begin
                        lk = w * 8 + c;
                        send_fp32(lk < k_dim ? a_val(r, base_k + lk) : POISON);
                    end
                // B window: [local k][col]
                for (r = 0; r < 8; r++)
                    for (c = 0; c < 8; c++) begin
                        lk = w * 8 + r;
                        send_fp32(lk < k_dim ? b_val(base_k + lk, c) : POISON);
                    end
            end

            send_word_le(FRAME_END);
        end
    endtask

    // -------------------------------------------------------------------
    // Expected per-invocation contexts. Mirrors the RTL: the context for
    // LOCAL reduction step lk is (lk >> 3) & 1, counted from 0 at the
    // start of this invocation, not of the workload.
    // -------------------------------------------------------------------
    shortreal exp0 [0:7][0:7];
    shortreal exp1 [0:7][0:7];
    shortreal acc  [0:7][0:7];
    shortreal ref_c[0:7][0:7];   // NOT "ref": SystemVerilog reserved word

    task automatic compute_expected(input int k_dim, input int base_k);
        int r, c, lk;
        begin
            for (r = 0; r < 8; r++)
                for (c = 0; c < 8; c++) begin
                    exp0[r][c] = 0.0;
                    exp1[r][c] = 0.0;
                end
            for (lk = 0; lk < k_dim; lk++)
                for (r = 0; r < 8; r++)
                    for (c = 0; c < 8; c++) begin
                        if (((lk >> 3) & 1) == 0)
                            exp0[r][c] += a_val(r, base_k + lk)
                                        * b_val(base_k + lk, c);
                        else
                            exp1[r][c] += a_val(r, base_k + lk)
                                        * b_val(base_k + lk, c);
                    end
        end
    endtask

    // -------------------------------------------------------------------
    int errors;
    int total_cycles;

    initial begin
        int i, r, c, base, kd;
        int meas, pred;
        shortreal got0, got1;

        errors = 0;
        total_cycles = 0;

        for (r = 0; r < 8; r++)
            for (c = 0; c < 8; c++) begin
                acc[r][c] = 0.0;
                ref_c[r][c] = 0.0;
            end

        // Reference over the whole workload.
        for (i = 0; i < K; i++)
            for (r = 0; r < 8; r++)
                for (c = 0; c < 8; c++)
                    ref_c[r][c] += a_val(r, i) * b_val(i, c);

        $display("");
        $display("==============================================");
        $display(" K_MAX = %0d   K = %0d   -> %0d invocation(s)",
                 K_MAX, K, N_INV);
        $display("==============================================");

        rst = 1'b1;
        repeat (20) @(posedge clk);
        rst = 1'b0;
        repeat (10) @(posedge clk);

        for (i = 0; i < N_INV; i++) begin
            base = i * K_MAX;
            kd   = (K - base) < K_MAX ? (K - base) : K_MAX;

            compute_expected(kd, base);

            $display("");
            $display("--- invocation %0d: k range [%0d,%0d), k_dim=%0d ---",
                     i, base, base + kd, kd);

            send_request(kd, base);

            // matrices_ready -> both contexts published
            wait (dut.c0_done && dut.c1_done);
            @(posedge clk);

            meas = dut.last_accel_cycles;
            pred = kd + FILL_DRAIN + 386;
            total_cycles += meas;

            $display("    measured cycles = %0d", meas);
            $display("    k_dim + %0d + c0 : c0 = %0d",
                     FILL_DRAIN, meas - kd - FILL_DRAIN);

            for (r = 0; r < 8; r++)
                for (c = 0; c < 8; c++) begin
                    got0 = $bitstoshortreal(dut.C0[r][c]);
                    got1 = $bitstoshortreal(dut.C1[r][c]);
                    if (got0 != exp0[r][c]) begin
                        $display("    FAIL ctx0[%0d][%0d] got=%f exp=%f",
                                 r, c, got0, exp0[r][c]);
                        errors++;
                    end
                    if (got1 != exp1[r][c]) begin
                        $display("    FAIL ctx1[%0d][%0d] got=%f exp=%f",
                                 r, c, got1, exp1[r][c]);
                        errors++;
                    end
                    acc[r][c] += got0 + got1;
                end

            if (errors == 0)
                $display("    partial result OK");

            // The main FSM only returns to ST_IDLE after TX drains.
            wait (dut.state == 3'd0);
            repeat (20) @(posedge clk);
        end

        // ---------------------------------------------------------------
        $display("");
        $display("==============================================");
        for (r = 0; r < 8; r++)
            for (c = 0; c < 8; c++)
                if (acc[r][c] != ref_c[r][c]) begin
                    $display(" FAIL C[%0d][%0d] got=%f exp=%f",
                             r, c, acc[r][c], ref_c[r][c]);
                    errors++;
                end

        $display(" accumulated over %0d invocation(s): %s",
                 N_INV, errors == 0 ? "BIT-EXACT" : "MISMATCH");
        $display(" total measured cycles = %0d", total_cycles);
        $display(" model K + n*(r+c-2+c0) = %0d",
                 K + N_INV * (FILL_DRAIN + 386));
        $display("==============================================");

        // Machine-readable, for a sweep driver to grep.
        $display("MULTIINV,%0d,%0d,%0d,%0d,%0d",
                 K_MAX, K, N_INV, total_cycles, errors);

        if (errors != 0) $fatal(1, "%0d mismatches", errors);
        $display("");
        $display("PASS");
        $finish;
    end

    /*
     * ===================================================================
     * Watchdog with a stage diagnosis
     * ===================================================================
     *
     * A bare TIMEOUT says the design stopped but not where, which is the
     * same blindness as the board's silent RX 0/512. Everything the
     * breadcrumb bytes would have told us is directly visible here, so
     * the watchdog reports the furthest stage reached and the state each
     * FSM is parked in.
     *
     * The PE completion count is the important one. ST_WAIT_RESULT waits
     * for all 64 PEs, so "0 of 64" means the valid stream never reached
     * the array at all, while "63 of 64" means one PE is stuck -- two
     * completely different bugs behind one identical symptom.
     */
    /* 清除走 rst,不走 initial。initial 加 always_ff 同時寫同一個變數,
     * xelab 會報 VRFC 10-3818 / 10-2921「invalid combination of procedural
     * drivers」—— 診斷用的旗標本身變成不可靠,正好毀掉它的用途。 */
    logic saw_ready, saw_feed, saw_wait, saw_send, saw_c0, saw_c1;

    always_ff @(posedge clk) begin
        if (rst) begin
            saw_ready <= 1'b0;
            saw_feed  <= 1'b0;
            saw_wait  <= 1'b0;
            saw_send  <= 1'b0;
            saw_c0    <= 1'b0;
            saw_c1    <= 1'b0;
        end
        else begin
            if (dut.matrices_ready)      saw_ready <= 1'b1;
            if (dut.state == 3'd1)       saw_feed  <= 1'b1;
            if (dut.state == 3'd2)       saw_wait  <= 1'b1;
            if (dut.state == 3'd3)       saw_send  <= 1'b1;
            if (dut.c0_done)             saw_c0    <= 1'b1;
            if (dut.c1_done)             saw_c1    <= 1'b1;
        end
    end

    function automatic int pes_done();
        int n = 0;
        for (int r = 0; r < 8; r++)
            for (int c = 0; c < 8; c++)
                if (dut.u_array.result_seen[r][c]) n++;
        return n;
    endfunction

    initial begin
        #500_000_000;

        $display("");
        $display("==============================================");
        $display(" TIMEOUT -- K_MAX=%0d K=%0d", K_MAX, K);
        $display("==============================================");
        $display(" 走到哪一級:");
        $display("   matrices_ready   : %s", saw_ready ? "yes" : "NO  <-- RX framing 沒收到完整 frame");
        $display("   ST_FEED          : %s", saw_feed  ? "yes" : "NO");
        $display("   ST_WAIT_RESULT   : %s", saw_wait  ? "yes" : "NO");
        $display("   c0_done          : %s", saw_c0    ? "yes" : "NO");
        $display("   c1_done          : %s", saw_c1    ? "yes" : "NO");
        $display("   ST_SEND          : %s", saw_send  ? "yes" : "NO");
        $display("");
        $display(" 停在哪:");
        $display("   rx_state=%0d rx_count=%0d hdr_done=%0b k_dim=%0d",
                 dut.rx_state, dut.rx_count, dut.hdr_done, dut.k_dim);
        $display("   state=%0d feed_t=%0d tx_state=%0d",
                 dut.state, dut.feed_t, dut.tx_state);
        $display("   PE 完成數 = %0d / 64   all_results_valid=%0b out_state=%0d",
                 pes_done(), dut.u_array.all_results_valid,
                 dut.u_array.out_state);
        $display("==============================================");

        $fatal(1, "watchdog");
    end

endmodule