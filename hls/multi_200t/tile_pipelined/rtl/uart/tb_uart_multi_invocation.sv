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
            hdr = k_dim;
            send_uart_byte(hdr[7:0]);
            send_uart_byte(hdr[15:8]);
            send_uart_byte(hdr[23:16]);
            send_uart_byte(hdr[31:24]);

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

    initial begin
        #500_000_000;
        $display("TIMEOUT -- K_MAX=%0d K=%0d", K_MAX, K);
        $fatal(1);
    end

endmodule