`timescale 1ns/1ps

module tb_uart_fold_top;

    // 為了讓 UART simulation 快很多：
    // 100 MHz / 10 Mbps = 10 clocks/bit
    localparam int CLK_HZ = 100_000_000;
    localparam int BAUD   = 10_000_000;
    localparam int CLKS_PER_BIT = CLK_HZ / BAUD;
    localparam time CLK_PERIOD = 10ns;
    localparam time BIT_PERIOD = CLK_PERIOD * CLKS_PER_BIT;

    logic clk = 0;
    logic rst = 1;
    logic uart_rx = 1;
    wire  uart_tx;

    systolic_uart_fold_top #(
        .CLK_HZ(CLK_HZ),
        .BAUD(BAUD)
    ) dut (
        .clk(clk),
        .rst(rst),
        .uart_rx(uart_rx),
        .uart_tx(uart_tx)
    );

    always #(CLK_PERIOD/2) clk = ~clk;

    task automatic send_uart_byte(input [7:0] data);
        integer i;
        begin
            // start
            uart_rx = 0;
            #(BIT_PERIOD);

            // data LSB first
            for (i = 0; i < 8; i = i + 1) begin
                uart_rx = data[i];
                #(BIT_PERIOD);
            end

            // stop
            uart_rx = 1;
            #(BIT_PERIOD);
        end
    endtask

    task automatic send_fp32(input shortreal x);
        logic [31:0] bits;
        begin
            bits = $shortrealtobits(x);

            send_uart_byte(bits[7:0]);
            send_uart_byte(bits[15:8]);
            send_uart_byte(bits[23:16]);
            send_uart_byte(bits[31:24]);
        end
    endtask

    integer r, c;

    task automatic send_transaction;
        begin

            /*
             * A0 = I
             */
            for (r = 0; r < 8; r = r + 1)
                for (c = 0; c < 8; c = c + 1)
                    send_fp32(
                        (r == c) ? 1.0 : 0.0
                    );

            /*
             * B0 = 1..64
             */
            for (r = 0; r < 8; r = r + 1)
                for (c = 0; c < 8; c = c + 1)
                    send_fp32(
                        shortreal'(r*8+c+1)
                    );

            /*
             * A1 = 2I
             */
            for (r = 0; r < 8; r = r + 1)
                for (c = 0; c < 8; c = c + 1)
                    send_fp32(
                        (r == c) ? 2.0 : 0.0
                    );

            /*
             * B1 = 1..64
             */
            for (r = 0; r < 8; r = r + 1)
                for (c = 0; c < 8; c = c + 1)
                    send_fp32(
                        shortreal'(r*8+c+1)
                    );

        end
    endtask


    integer txn_done_count = 0;

    always @(posedge clk) begin
        if (!rst && dut.tx_all_done)
            txn_done_count <= txn_done_count + 1;
    end


    initial begin

        $display("=== UART FOLD TOP TWO-TRANSACTION TEST ===");

        repeat (20) @(posedge clk);
        rst = 0;
        repeat (20) @(posedge clk);


        /*
         * ========================================================
         * TRANSACTION 1
         * ========================================================
         */
        $display("");
        $display("===== TXN 1 START =====");

        send_transaction();

        $display(
            "TXN1 UART input complete: rx_count=%0d",
            dut.rx_count
        );

        /*
         * Wait until the complete 512-byte result stream finishes.
         */
        wait (txn_done_count >= 1);

        $display(
            "===== TXN 1 DONE state=%0d c0_done=%b c1_done=%b =====",
            dut.state,
            dut.c0_done,
            dut.c1_done
        );

        /*
         * Give the design a little idle space.
         * DO NOT reset.
         */
        repeat (100) @(posedge clk);


        /*
         * ========================================================
         * TRANSACTION 2
         * ========================================================
         */
        $display("");
        $display("===== TXN 2 START =====");

        send_transaction();

        $display(
            "TXN2 UART input complete: rx_count=%0d",
            dut.rx_count
        );


        /*
         * Do NOT wait forever.
         *
         * This timeout lets us inspect the internal state if the
         * second transaction reproduces the hardware hang.
         */
        fork

            begin : WAIT_TXN2_DONE

                wait (txn_done_count >= 2);

                $display("");
                $display("===== TXN 2 PASS =====");

                $display(
                    "FINAL state=%0d c0_done=%b c1_done=%b tx_state=%0d tx_count=%0d",
                    dut.state,
                    dut.c0_done,
                    dut.c1_done,
                    dut.tx_state,
                    dut.tx_count
                );

                $finish;

            end


            begin : TXN2_TIMEOUT

                repeat (100000) @(posedge clk);

                $display("");
                $display("===== TXN 2 TIMEOUT =====");

                $display(
                    "TOP state=%0d feed_t=%0d c0_done=%b c1_done=%b c_valid=%b ctx=%b",
                    dut.state,
                    dut.feed_t,
                    dut.c0_done,
                    dut.c1_done,
                    dut.c_valid_out,
                    dut.c_ctx_out
                );

                $display(
                    "PE00 state=%0d transaction_seen=%b input_finished=%b outstanding_adds=%0d product_valid=%b add_valid=%b result_valid=%b",
                    dut.u_array.ROW[0].COL[0].u_pe.state,
                    dut.u_array.ROW[0].COL[0].u_pe.transaction_seen,
                    dut.u_array.ROW[0].COL[0].u_pe.input_finished,
                    dut.u_array.ROW[0].COL[0].u_pe.outstanding_adds,
                    dut.u_array.ROW[0].COL[0].u_pe.product_valid,
                    dut.u_array.ROW[0].COL[0].u_pe.add_valid,
                    dut.u_array.ROW[0].COL[0].u_pe.result_valid
                );

                $finish;

            end

        join_any

        disable fork;

    end



    /*
     * Critical state tracing
     */
    always @(posedge clk) begin

        if (dut.matrices_ready)
            $display(
                "MATRICES_READY time=%0t",
                $time
            );

        if (dut.c_valid_out)
            $display(
                "C_VALID time=%0t ctx=%b C00=%f",
                $time,
                dut.c_ctx_out,
                $bitstoshortreal(dut.c_out[0][0])
            );

        if (dut.tx_start)
            $display(
                "TX_START time=%0t count=%0d byte=%02x",
                $time,
                dut.tx_count,
                dut.tx_byte
            );

    end

    always @(posedge clk) begin
        if (!rst) begin

            if (dut.matrices_ready)
                $display(
                    "DBG MATRICES_READY time=%0t",
                    $time
                );

            if (dut.c_valid_out)
                $display(
                    "DBG C_VALID time=%0t ctx=%0d C00=%h",
                    $time,
                    dut.c_ctx_out,
                    dut.c_out[0][0]
                );

            // if (dut.state == dut.ST_SEND)
            //     $display(
            //         "DBG ST_SEND time=%0t tx_state=%0d started=%0d",
            //         $time,
            //         dut.tx_state,
            //         dut.tx_send_started
            //     );

            if (dut.tx_start)
                $display(
                    "DBG TX_START time=%0t count=%0d byte=%02h debug=%0d",
                    $time,
                    dut.tx_count,
                    dut.tx_byte,
                    dut.debug_tx_active
                );

            if (dut.tx_start && !dut.debug_tx_active)
                $display(
                    "RESULT_TX count=%0d byte=%02h time=%0t",
                    dut.tx_count,
                    dut.tx_byte,
                    $time
                );

            if (dut.tx_all_done)
                $display(
                    "RESULT_TX_ALL_DONE time=%0t",
                    $time
                );

            if (dut.state == 3'd3 && dut.tx_count == 9'd511)
                $display(
                    "RESULT_AT_511 tx_state=%0d busy=%0d time=%0t",
                    dut.tx_state,
                    dut.tx_busy,
                    $time
                );

        end
    end


    /*
     * ============================================================
     * Accelerator latency trace
     * ============================================================
     */
    always @(posedge clk) begin

        if (
            !rst &&
            dut.c_valid_out &&
            dut.c_ctx_out == 1'b1
        ) begin

            $display(
                "ACCEL_LATENCY cycles=%0d time_ns=%0d GOPS=%f",
                dut.accel_cycles + 1,
                (dut.accel_cycles + 1) * 10,
                204.8 / (dut.accel_cycles + 1)
            );

        end

    end

    integer cycle_ctr = 0;

    integer t0_start      = -1;
    integer t1_feed_done  = -1;
    integer t2_reduce     = -1;
    integer t3_result     = -1;

    always @(posedge clk) begin
        if (rst) begin
            cycle_ctr     <= 0;
            t0_start      <= -1;
            t1_feed_done  <= -1;
            t2_reduce     <= -1;
            t3_result     <= -1;
        end
        else begin
            cycle_ctr <= cycle_ctr + 1;

            /*
            * T0: complete input transaction received
            */
            if (dut.matrices_ready) begin
                t0_start <= cycle_ctr;

                $display(
                    "LAT_T0 matrices_ready cycle=%0d",
                    cycle_ctr
                );
            end

            /*
            * T1: top-level feed finished
            */
            if (
                dut.state == dut.ST_FEED &&
                dut.feed_t == 6'd22
            ) begin
                t1_feed_done <= cycle_ctr;

                $display(
                    "LAT_T1 feed_done cycle=%0d delta_from_T0=%0d",
                    cycle_ctr,
                    cycle_ctr - t0_start
                );
            end

            /*
            * T2:
            * first observed PE enters reduction.
            * Start with PE00 for debugging.
            */
            if (
                dut.u_array.ROW[0].COL[0].u_pe.state ==
                    dut.u_array.ROW[0].COL[0].u_pe.PE_REDUCE_ISSUE &&
                t2_reduce < 0
            ) begin
                t2_reduce <= cycle_ctr;

                $display(
                    "LAT_T2 PE00_reduce_start cycle=%0d drain_cycles=%0d",
                    cycle_ctr,
                    cycle_ctr - t1_feed_done
                );
            end

            /*
            * T3: final ctx1 matrix becomes valid
            */
            if (
                dut.c_valid_out &&
                dut.c_ctx_out == 1'b1
            ) begin
                t3_result <= cycle_ctr;

                $display(
                    "LAT_T3 result_done cycle=%0d",
                    cycle_ctr
                );

                $display(
                    "LAT_BREAKDOWN total=%0d feed=%0d drain=%0d reduce_plus_collect=%0d",
                    cycle_ctr - t0_start,
                    t1_feed_done - t0_start,
                    t2_reduce - t1_feed_done,
                    cycle_ctr - t2_reduce
                );

                /*
                * Rearm for transaction 2
                */
                t0_start      <= -1;
                t1_feed_done  <= -1;
                t2_reduce     <= -1;
                t3_result     <= -1;
            end
        end
    end


    /*
     * ============================================================
     * PE_REDUCTION_TABLE
     *
     * Record reduction start/end cycle for all 64 PEs.
     *
     * start:
     *   first cycle PE enters PE_REDUCE_ISSUE
     *
     * end:
     *   cycle PE asserts result_valid after ctx1 reduction
     * ============================================================
     */

    integer pe_red_start [0:7][0:7];
    integer pe_red_end   [0:7][0:7];

    integer pe_trace_cycle = 0;
    integer pr;
    integer pc;

    initial begin
        for (pr = 0; pr < 8; pr = pr + 1)
            for (pc = 0; pc < 8; pc = pc + 1) begin
                pe_red_start[pr][pc] = -1;
                pe_red_end[pr][pc]   = -1;
            end
    end

    always @(posedge clk) begin

        if (rst) begin

            pe_trace_cycle <= 0;

            for (pr = 0; pr < 8; pr = pr + 1)
                for (pc = 0; pc < 8; pc = pc + 1) begin
                    pe_red_start[pr][pc] <= -1;
                    pe_red_end[pr][pc]   <= -1;
                end

        end
        else begin

            pe_trace_cycle <= pe_trace_cycle + 1;

            /*
             * Explicitly trace all 64 generated PE instances.
             */

            if (
                pe_red_start[0][0] < 0 &&
                dut.u_array.ROW[0].COL[0].u_pe.state == 2'd1
            ) begin
                pe_red_start[0][0] <= pe_trace_cycle;
            end

            if (
                pe_red_end[0][0] < 0 &&
                dut.u_array.ROW[0].COL[0].u_pe.result_valid
            ) begin
                pe_red_end[0][0] <= pe_trace_cycle;
            end

            if (
                pe_red_start[0][1] < 0 &&
                dut.u_array.ROW[0].COL[1].u_pe.state == 2'd1
            ) begin
                pe_red_start[0][1] <= pe_trace_cycle;
            end

            if (
                pe_red_end[0][1] < 0 &&
                dut.u_array.ROW[0].COL[1].u_pe.result_valid
            ) begin
                pe_red_end[0][1] <= pe_trace_cycle;
            end

            if (
                pe_red_start[0][2] < 0 &&
                dut.u_array.ROW[0].COL[2].u_pe.state == 2'd1
            ) begin
                pe_red_start[0][2] <= pe_trace_cycle;
            end

            if (
                pe_red_end[0][2] < 0 &&
                dut.u_array.ROW[0].COL[2].u_pe.result_valid
            ) begin
                pe_red_end[0][2] <= pe_trace_cycle;
            end

            if (
                pe_red_start[0][3] < 0 &&
                dut.u_array.ROW[0].COL[3].u_pe.state == 2'd1
            ) begin
                pe_red_start[0][3] <= pe_trace_cycle;
            end

            if (
                pe_red_end[0][3] < 0 &&
                dut.u_array.ROW[0].COL[3].u_pe.result_valid
            ) begin
                pe_red_end[0][3] <= pe_trace_cycle;
            end

            if (
                pe_red_start[0][4] < 0 &&
                dut.u_array.ROW[0].COL[4].u_pe.state == 2'd1
            ) begin
                pe_red_start[0][4] <= pe_trace_cycle;
            end

            if (
                pe_red_end[0][4] < 0 &&
                dut.u_array.ROW[0].COL[4].u_pe.result_valid
            ) begin
                pe_red_end[0][4] <= pe_trace_cycle;
            end

            if (
                pe_red_start[0][5] < 0 &&
                dut.u_array.ROW[0].COL[5].u_pe.state == 2'd1
            ) begin
                pe_red_start[0][5] <= pe_trace_cycle;
            end

            if (
                pe_red_end[0][5] < 0 &&
                dut.u_array.ROW[0].COL[5].u_pe.result_valid
            ) begin
                pe_red_end[0][5] <= pe_trace_cycle;
            end

            if (
                pe_red_start[0][6] < 0 &&
                dut.u_array.ROW[0].COL[6].u_pe.state == 2'd1
            ) begin
                pe_red_start[0][6] <= pe_trace_cycle;
            end

            if (
                pe_red_end[0][6] < 0 &&
                dut.u_array.ROW[0].COL[6].u_pe.result_valid
            ) begin
                pe_red_end[0][6] <= pe_trace_cycle;
            end

            if (
                pe_red_start[0][7] < 0 &&
                dut.u_array.ROW[0].COL[7].u_pe.state == 2'd1
            ) begin
                pe_red_start[0][7] <= pe_trace_cycle;
            end

            if (
                pe_red_end[0][7] < 0 &&
                dut.u_array.ROW[0].COL[7].u_pe.result_valid
            ) begin
                pe_red_end[0][7] <= pe_trace_cycle;
            end

            if (
                pe_red_start[1][0] < 0 &&
                dut.u_array.ROW[1].COL[0].u_pe.state == 2'd1
            ) begin
                pe_red_start[1][0] <= pe_trace_cycle;
            end

            if (
                pe_red_end[1][0] < 0 &&
                dut.u_array.ROW[1].COL[0].u_pe.result_valid
            ) begin
                pe_red_end[1][0] <= pe_trace_cycle;
            end

            if (
                pe_red_start[1][1] < 0 &&
                dut.u_array.ROW[1].COL[1].u_pe.state == 2'd1
            ) begin
                pe_red_start[1][1] <= pe_trace_cycle;
            end

            if (
                pe_red_end[1][1] < 0 &&
                dut.u_array.ROW[1].COL[1].u_pe.result_valid
            ) begin
                pe_red_end[1][1] <= pe_trace_cycle;
            end

            if (
                pe_red_start[1][2] < 0 &&
                dut.u_array.ROW[1].COL[2].u_pe.state == 2'd1
            ) begin
                pe_red_start[1][2] <= pe_trace_cycle;
            end

            if (
                pe_red_end[1][2] < 0 &&
                dut.u_array.ROW[1].COL[2].u_pe.result_valid
            ) begin
                pe_red_end[1][2] <= pe_trace_cycle;
            end

            if (
                pe_red_start[1][3] < 0 &&
                dut.u_array.ROW[1].COL[3].u_pe.state == 2'd1
            ) begin
                pe_red_start[1][3] <= pe_trace_cycle;
            end

            if (
                pe_red_end[1][3] < 0 &&
                dut.u_array.ROW[1].COL[3].u_pe.result_valid
            ) begin
                pe_red_end[1][3] <= pe_trace_cycle;
            end

            if (
                pe_red_start[1][4] < 0 &&
                dut.u_array.ROW[1].COL[4].u_pe.state == 2'd1
            ) begin
                pe_red_start[1][4] <= pe_trace_cycle;
            end

            if (
                pe_red_end[1][4] < 0 &&
                dut.u_array.ROW[1].COL[4].u_pe.result_valid
            ) begin
                pe_red_end[1][4] <= pe_trace_cycle;
            end

            if (
                pe_red_start[1][5] < 0 &&
                dut.u_array.ROW[1].COL[5].u_pe.state == 2'd1
            ) begin
                pe_red_start[1][5] <= pe_trace_cycle;
            end

            if (
                pe_red_end[1][5] < 0 &&
                dut.u_array.ROW[1].COL[5].u_pe.result_valid
            ) begin
                pe_red_end[1][5] <= pe_trace_cycle;
            end

            if (
                pe_red_start[1][6] < 0 &&
                dut.u_array.ROW[1].COL[6].u_pe.state == 2'd1
            ) begin
                pe_red_start[1][6] <= pe_trace_cycle;
            end

            if (
                pe_red_end[1][6] < 0 &&
                dut.u_array.ROW[1].COL[6].u_pe.result_valid
            ) begin
                pe_red_end[1][6] <= pe_trace_cycle;
            end

            if (
                pe_red_start[1][7] < 0 &&
                dut.u_array.ROW[1].COL[7].u_pe.state == 2'd1
            ) begin
                pe_red_start[1][7] <= pe_trace_cycle;
            end

            if (
                pe_red_end[1][7] < 0 &&
                dut.u_array.ROW[1].COL[7].u_pe.result_valid
            ) begin
                pe_red_end[1][7] <= pe_trace_cycle;
            end

            if (
                pe_red_start[2][0] < 0 &&
                dut.u_array.ROW[2].COL[0].u_pe.state == 2'd1
            ) begin
                pe_red_start[2][0] <= pe_trace_cycle;
            end

            if (
                pe_red_end[2][0] < 0 &&
                dut.u_array.ROW[2].COL[0].u_pe.result_valid
            ) begin
                pe_red_end[2][0] <= pe_trace_cycle;
            end

            if (
                pe_red_start[2][1] < 0 &&
                dut.u_array.ROW[2].COL[1].u_pe.state == 2'd1
            ) begin
                pe_red_start[2][1] <= pe_trace_cycle;
            end

            if (
                pe_red_end[2][1] < 0 &&
                dut.u_array.ROW[2].COL[1].u_pe.result_valid
            ) begin
                pe_red_end[2][1] <= pe_trace_cycle;
            end

            if (
                pe_red_start[2][2] < 0 &&
                dut.u_array.ROW[2].COL[2].u_pe.state == 2'd1
            ) begin
                pe_red_start[2][2] <= pe_trace_cycle;
            end

            if (
                pe_red_end[2][2] < 0 &&
                dut.u_array.ROW[2].COL[2].u_pe.result_valid
            ) begin
                pe_red_end[2][2] <= pe_trace_cycle;
            end

            if (
                pe_red_start[2][3] < 0 &&
                dut.u_array.ROW[2].COL[3].u_pe.state == 2'd1
            ) begin
                pe_red_start[2][3] <= pe_trace_cycle;
            end

            if (
                pe_red_end[2][3] < 0 &&
                dut.u_array.ROW[2].COL[3].u_pe.result_valid
            ) begin
                pe_red_end[2][3] <= pe_trace_cycle;
            end

            if (
                pe_red_start[2][4] < 0 &&
                dut.u_array.ROW[2].COL[4].u_pe.state == 2'd1
            ) begin
                pe_red_start[2][4] <= pe_trace_cycle;
            end

            if (
                pe_red_end[2][4] < 0 &&
                dut.u_array.ROW[2].COL[4].u_pe.result_valid
            ) begin
                pe_red_end[2][4] <= pe_trace_cycle;
            end

            if (
                pe_red_start[2][5] < 0 &&
                dut.u_array.ROW[2].COL[5].u_pe.state == 2'd1
            ) begin
                pe_red_start[2][5] <= pe_trace_cycle;
            end

            if (
                pe_red_end[2][5] < 0 &&
                dut.u_array.ROW[2].COL[5].u_pe.result_valid
            ) begin
                pe_red_end[2][5] <= pe_trace_cycle;
            end

            if (
                pe_red_start[2][6] < 0 &&
                dut.u_array.ROW[2].COL[6].u_pe.state == 2'd1
            ) begin
                pe_red_start[2][6] <= pe_trace_cycle;
            end

            if (
                pe_red_end[2][6] < 0 &&
                dut.u_array.ROW[2].COL[6].u_pe.result_valid
            ) begin
                pe_red_end[2][6] <= pe_trace_cycle;
            end

            if (
                pe_red_start[2][7] < 0 &&
                dut.u_array.ROW[2].COL[7].u_pe.state == 2'd1
            ) begin
                pe_red_start[2][7] <= pe_trace_cycle;
            end

            if (
                pe_red_end[2][7] < 0 &&
                dut.u_array.ROW[2].COL[7].u_pe.result_valid
            ) begin
                pe_red_end[2][7] <= pe_trace_cycle;
            end

            if (
                pe_red_start[3][0] < 0 &&
                dut.u_array.ROW[3].COL[0].u_pe.state == 2'd1
            ) begin
                pe_red_start[3][0] <= pe_trace_cycle;
            end

            if (
                pe_red_end[3][0] < 0 &&
                dut.u_array.ROW[3].COL[0].u_pe.result_valid
            ) begin
                pe_red_end[3][0] <= pe_trace_cycle;
            end

            if (
                pe_red_start[3][1] < 0 &&
                dut.u_array.ROW[3].COL[1].u_pe.state == 2'd1
            ) begin
                pe_red_start[3][1] <= pe_trace_cycle;
            end

            if (
                pe_red_end[3][1] < 0 &&
                dut.u_array.ROW[3].COL[1].u_pe.result_valid
            ) begin
                pe_red_end[3][1] <= pe_trace_cycle;
            end

            if (
                pe_red_start[3][2] < 0 &&
                dut.u_array.ROW[3].COL[2].u_pe.state == 2'd1
            ) begin
                pe_red_start[3][2] <= pe_trace_cycle;
            end

            if (
                pe_red_end[3][2] < 0 &&
                dut.u_array.ROW[3].COL[2].u_pe.result_valid
            ) begin
                pe_red_end[3][2] <= pe_trace_cycle;
            end

            if (
                pe_red_start[3][3] < 0 &&
                dut.u_array.ROW[3].COL[3].u_pe.state == 2'd1
            ) begin
                pe_red_start[3][3] <= pe_trace_cycle;
            end

            if (
                pe_red_end[3][3] < 0 &&
                dut.u_array.ROW[3].COL[3].u_pe.result_valid
            ) begin
                pe_red_end[3][3] <= pe_trace_cycle;
            end

            if (
                pe_red_start[3][4] < 0 &&
                dut.u_array.ROW[3].COL[4].u_pe.state == 2'd1
            ) begin
                pe_red_start[3][4] <= pe_trace_cycle;
            end

            if (
                pe_red_end[3][4] < 0 &&
                dut.u_array.ROW[3].COL[4].u_pe.result_valid
            ) begin
                pe_red_end[3][4] <= pe_trace_cycle;
            end

            if (
                pe_red_start[3][5] < 0 &&
                dut.u_array.ROW[3].COL[5].u_pe.state == 2'd1
            ) begin
                pe_red_start[3][5] <= pe_trace_cycle;
            end

            if (
                pe_red_end[3][5] < 0 &&
                dut.u_array.ROW[3].COL[5].u_pe.result_valid
            ) begin
                pe_red_end[3][5] <= pe_trace_cycle;
            end

            if (
                pe_red_start[3][6] < 0 &&
                dut.u_array.ROW[3].COL[6].u_pe.state == 2'd1
            ) begin
                pe_red_start[3][6] <= pe_trace_cycle;
            end

            if (
                pe_red_end[3][6] < 0 &&
                dut.u_array.ROW[3].COL[6].u_pe.result_valid
            ) begin
                pe_red_end[3][6] <= pe_trace_cycle;
            end

            if (
                pe_red_start[3][7] < 0 &&
                dut.u_array.ROW[3].COL[7].u_pe.state == 2'd1
            ) begin
                pe_red_start[3][7] <= pe_trace_cycle;
            end

            if (
                pe_red_end[3][7] < 0 &&
                dut.u_array.ROW[3].COL[7].u_pe.result_valid
            ) begin
                pe_red_end[3][7] <= pe_trace_cycle;
            end

            if (
                pe_red_start[4][0] < 0 &&
                dut.u_array.ROW[4].COL[0].u_pe.state == 2'd1
            ) begin
                pe_red_start[4][0] <= pe_trace_cycle;
            end

            if (
                pe_red_end[4][0] < 0 &&
                dut.u_array.ROW[4].COL[0].u_pe.result_valid
            ) begin
                pe_red_end[4][0] <= pe_trace_cycle;
            end

            if (
                pe_red_start[4][1] < 0 &&
                dut.u_array.ROW[4].COL[1].u_pe.state == 2'd1
            ) begin
                pe_red_start[4][1] <= pe_trace_cycle;
            end

            if (
                pe_red_end[4][1] < 0 &&
                dut.u_array.ROW[4].COL[1].u_pe.result_valid
            ) begin
                pe_red_end[4][1] <= pe_trace_cycle;
            end

            if (
                pe_red_start[4][2] < 0 &&
                dut.u_array.ROW[4].COL[2].u_pe.state == 2'd1
            ) begin
                pe_red_start[4][2] <= pe_trace_cycle;
            end

            if (
                pe_red_end[4][2] < 0 &&
                dut.u_array.ROW[4].COL[2].u_pe.result_valid
            ) begin
                pe_red_end[4][2] <= pe_trace_cycle;
            end

            if (
                pe_red_start[4][3] < 0 &&
                dut.u_array.ROW[4].COL[3].u_pe.state == 2'd1
            ) begin
                pe_red_start[4][3] <= pe_trace_cycle;
            end

            if (
                pe_red_end[4][3] < 0 &&
                dut.u_array.ROW[4].COL[3].u_pe.result_valid
            ) begin
                pe_red_end[4][3] <= pe_trace_cycle;
            end

            if (
                pe_red_start[4][4] < 0 &&
                dut.u_array.ROW[4].COL[4].u_pe.state == 2'd1
            ) begin
                pe_red_start[4][4] <= pe_trace_cycle;
            end

            if (
                pe_red_end[4][4] < 0 &&
                dut.u_array.ROW[4].COL[4].u_pe.result_valid
            ) begin
                pe_red_end[4][4] <= pe_trace_cycle;
            end

            if (
                pe_red_start[4][5] < 0 &&
                dut.u_array.ROW[4].COL[5].u_pe.state == 2'd1
            ) begin
                pe_red_start[4][5] <= pe_trace_cycle;
            end

            if (
                pe_red_end[4][5] < 0 &&
                dut.u_array.ROW[4].COL[5].u_pe.result_valid
            ) begin
                pe_red_end[4][5] <= pe_trace_cycle;
            end

            if (
                pe_red_start[4][6] < 0 &&
                dut.u_array.ROW[4].COL[6].u_pe.state == 2'd1
            ) begin
                pe_red_start[4][6] <= pe_trace_cycle;
            end

            if (
                pe_red_end[4][6] < 0 &&
                dut.u_array.ROW[4].COL[6].u_pe.result_valid
            ) begin
                pe_red_end[4][6] <= pe_trace_cycle;
            end

            if (
                pe_red_start[4][7] < 0 &&
                dut.u_array.ROW[4].COL[7].u_pe.state == 2'd1
            ) begin
                pe_red_start[4][7] <= pe_trace_cycle;
            end

            if (
                pe_red_end[4][7] < 0 &&
                dut.u_array.ROW[4].COL[7].u_pe.result_valid
            ) begin
                pe_red_end[4][7] <= pe_trace_cycle;
            end

            if (
                pe_red_start[5][0] < 0 &&
                dut.u_array.ROW[5].COL[0].u_pe.state == 2'd1
            ) begin
                pe_red_start[5][0] <= pe_trace_cycle;
            end

            if (
                pe_red_end[5][0] < 0 &&
                dut.u_array.ROW[5].COL[0].u_pe.result_valid
            ) begin
                pe_red_end[5][0] <= pe_trace_cycle;
            end

            if (
                pe_red_start[5][1] < 0 &&
                dut.u_array.ROW[5].COL[1].u_pe.state == 2'd1
            ) begin
                pe_red_start[5][1] <= pe_trace_cycle;
            end

            if (
                pe_red_end[5][1] < 0 &&
                dut.u_array.ROW[5].COL[1].u_pe.result_valid
            ) begin
                pe_red_end[5][1] <= pe_trace_cycle;
            end

            if (
                pe_red_start[5][2] < 0 &&
                dut.u_array.ROW[5].COL[2].u_pe.state == 2'd1
            ) begin
                pe_red_start[5][2] <= pe_trace_cycle;
            end

            if (
                pe_red_end[5][2] < 0 &&
                dut.u_array.ROW[5].COL[2].u_pe.result_valid
            ) begin
                pe_red_end[5][2] <= pe_trace_cycle;
            end

            if (
                pe_red_start[5][3] < 0 &&
                dut.u_array.ROW[5].COL[3].u_pe.state == 2'd1
            ) begin
                pe_red_start[5][3] <= pe_trace_cycle;
            end

            if (
                pe_red_end[5][3] < 0 &&
                dut.u_array.ROW[5].COL[3].u_pe.result_valid
            ) begin
                pe_red_end[5][3] <= pe_trace_cycle;
            end

            if (
                pe_red_start[5][4] < 0 &&
                dut.u_array.ROW[5].COL[4].u_pe.state == 2'd1
            ) begin
                pe_red_start[5][4] <= pe_trace_cycle;
            end

            if (
                pe_red_end[5][4] < 0 &&
                dut.u_array.ROW[5].COL[4].u_pe.result_valid
            ) begin
                pe_red_end[5][4] <= pe_trace_cycle;
            end

            if (
                pe_red_start[5][5] < 0 &&
                dut.u_array.ROW[5].COL[5].u_pe.state == 2'd1
            ) begin
                pe_red_start[5][5] <= pe_trace_cycle;
            end

            if (
                pe_red_end[5][5] < 0 &&
                dut.u_array.ROW[5].COL[5].u_pe.result_valid
            ) begin
                pe_red_end[5][5] <= pe_trace_cycle;
            end

            if (
                pe_red_start[5][6] < 0 &&
                dut.u_array.ROW[5].COL[6].u_pe.state == 2'd1
            ) begin
                pe_red_start[5][6] <= pe_trace_cycle;
            end

            if (
                pe_red_end[5][6] < 0 &&
                dut.u_array.ROW[5].COL[6].u_pe.result_valid
            ) begin
                pe_red_end[5][6] <= pe_trace_cycle;
            end

            if (
                pe_red_start[5][7] < 0 &&
                dut.u_array.ROW[5].COL[7].u_pe.state == 2'd1
            ) begin
                pe_red_start[5][7] <= pe_trace_cycle;
            end

            if (
                pe_red_end[5][7] < 0 &&
                dut.u_array.ROW[5].COL[7].u_pe.result_valid
            ) begin
                pe_red_end[5][7] <= pe_trace_cycle;
            end

            if (
                pe_red_start[6][0] < 0 &&
                dut.u_array.ROW[6].COL[0].u_pe.state == 2'd1
            ) begin
                pe_red_start[6][0] <= pe_trace_cycle;
            end

            if (
                pe_red_end[6][0] < 0 &&
                dut.u_array.ROW[6].COL[0].u_pe.result_valid
            ) begin
                pe_red_end[6][0] <= pe_trace_cycle;
            end

            if (
                pe_red_start[6][1] < 0 &&
                dut.u_array.ROW[6].COL[1].u_pe.state == 2'd1
            ) begin
                pe_red_start[6][1] <= pe_trace_cycle;
            end

            if (
                pe_red_end[6][1] < 0 &&
                dut.u_array.ROW[6].COL[1].u_pe.result_valid
            ) begin
                pe_red_end[6][1] <= pe_trace_cycle;
            end

            if (
                pe_red_start[6][2] < 0 &&
                dut.u_array.ROW[6].COL[2].u_pe.state == 2'd1
            ) begin
                pe_red_start[6][2] <= pe_trace_cycle;
            end

            if (
                pe_red_end[6][2] < 0 &&
                dut.u_array.ROW[6].COL[2].u_pe.result_valid
            ) begin
                pe_red_end[6][2] <= pe_trace_cycle;
            end

            if (
                pe_red_start[6][3] < 0 &&
                dut.u_array.ROW[6].COL[3].u_pe.state == 2'd1
            ) begin
                pe_red_start[6][3] <= pe_trace_cycle;
            end

            if (
                pe_red_end[6][3] < 0 &&
                dut.u_array.ROW[6].COL[3].u_pe.result_valid
            ) begin
                pe_red_end[6][3] <= pe_trace_cycle;
            end

            if (
                pe_red_start[6][4] < 0 &&
                dut.u_array.ROW[6].COL[4].u_pe.state == 2'd1
            ) begin
                pe_red_start[6][4] <= pe_trace_cycle;
            end

            if (
                pe_red_end[6][4] < 0 &&
                dut.u_array.ROW[6].COL[4].u_pe.result_valid
            ) begin
                pe_red_end[6][4] <= pe_trace_cycle;
            end

            if (
                pe_red_start[6][5] < 0 &&
                dut.u_array.ROW[6].COL[5].u_pe.state == 2'd1
            ) begin
                pe_red_start[6][5] <= pe_trace_cycle;
            end

            if (
                pe_red_end[6][5] < 0 &&
                dut.u_array.ROW[6].COL[5].u_pe.result_valid
            ) begin
                pe_red_end[6][5] <= pe_trace_cycle;
            end

            if (
                pe_red_start[6][6] < 0 &&
                dut.u_array.ROW[6].COL[6].u_pe.state == 2'd1
            ) begin
                pe_red_start[6][6] <= pe_trace_cycle;
            end

            if (
                pe_red_end[6][6] < 0 &&
                dut.u_array.ROW[6].COL[6].u_pe.result_valid
            ) begin
                pe_red_end[6][6] <= pe_trace_cycle;
            end

            if (
                pe_red_start[6][7] < 0 &&
                dut.u_array.ROW[6].COL[7].u_pe.state == 2'd1
            ) begin
                pe_red_start[6][7] <= pe_trace_cycle;
            end

            if (
                pe_red_end[6][7] < 0 &&
                dut.u_array.ROW[6].COL[7].u_pe.result_valid
            ) begin
                pe_red_end[6][7] <= pe_trace_cycle;
            end

            if (
                pe_red_start[7][0] < 0 &&
                dut.u_array.ROW[7].COL[0].u_pe.state == 2'd1
            ) begin
                pe_red_start[7][0] <= pe_trace_cycle;
            end

            if (
                pe_red_end[7][0] < 0 &&
                dut.u_array.ROW[7].COL[0].u_pe.result_valid
            ) begin
                pe_red_end[7][0] <= pe_trace_cycle;
            end

            if (
                pe_red_start[7][1] < 0 &&
                dut.u_array.ROW[7].COL[1].u_pe.state == 2'd1
            ) begin
                pe_red_start[7][1] <= pe_trace_cycle;
            end

            if (
                pe_red_end[7][1] < 0 &&
                dut.u_array.ROW[7].COL[1].u_pe.result_valid
            ) begin
                pe_red_end[7][1] <= pe_trace_cycle;
            end

            if (
                pe_red_start[7][2] < 0 &&
                dut.u_array.ROW[7].COL[2].u_pe.state == 2'd1
            ) begin
                pe_red_start[7][2] <= pe_trace_cycle;
            end

            if (
                pe_red_end[7][2] < 0 &&
                dut.u_array.ROW[7].COL[2].u_pe.result_valid
            ) begin
                pe_red_end[7][2] <= pe_trace_cycle;
            end

            if (
                pe_red_start[7][3] < 0 &&
                dut.u_array.ROW[7].COL[3].u_pe.state == 2'd1
            ) begin
                pe_red_start[7][3] <= pe_trace_cycle;
            end

            if (
                pe_red_end[7][3] < 0 &&
                dut.u_array.ROW[7].COL[3].u_pe.result_valid
            ) begin
                pe_red_end[7][3] <= pe_trace_cycle;
            end

            if (
                pe_red_start[7][4] < 0 &&
                dut.u_array.ROW[7].COL[4].u_pe.state == 2'd1
            ) begin
                pe_red_start[7][4] <= pe_trace_cycle;
            end

            if (
                pe_red_end[7][4] < 0 &&
                dut.u_array.ROW[7].COL[4].u_pe.result_valid
            ) begin
                pe_red_end[7][4] <= pe_trace_cycle;
            end

            if (
                pe_red_start[7][5] < 0 &&
                dut.u_array.ROW[7].COL[5].u_pe.state == 2'd1
            ) begin
                pe_red_start[7][5] <= pe_trace_cycle;
            end

            if (
                pe_red_end[7][5] < 0 &&
                dut.u_array.ROW[7].COL[5].u_pe.result_valid
            ) begin
                pe_red_end[7][5] <= pe_trace_cycle;
            end

            if (
                pe_red_start[7][6] < 0 &&
                dut.u_array.ROW[7].COL[6].u_pe.state == 2'd1
            ) begin
                pe_red_start[7][6] <= pe_trace_cycle;
            end

            if (
                pe_red_end[7][6] < 0 &&
                dut.u_array.ROW[7].COL[6].u_pe.result_valid
            ) begin
                pe_red_end[7][6] <= pe_trace_cycle;
            end

            if (
                pe_red_start[7][7] < 0 &&
                dut.u_array.ROW[7].COL[7].u_pe.state == 2'd1
            ) begin
                pe_red_start[7][7] <= pe_trace_cycle;
            end

            if (
                pe_red_end[7][7] < 0 &&
                dut.u_array.ROW[7].COL[7].u_pe.result_valid
            ) begin
                pe_red_end[7][7] <= pe_trace_cycle;
            end

            /*
             * Final ctx1 matrix is published only after all PE results
             * have been observed. Print the table here.
             */
            if (
                dut.c_valid_out &&
                dut.c_ctx_out == 1'b1
            ) begin

                $display("===== PE REDUCTION CYCLE TABLE =====");

                for (pr = 0; pr < 8; pr = pr + 1) begin
                    for (pc = 0; pc < 8; pc = pc + 1) begin

                        $display(
                            "PE[%0d][%0d] start=%0d end=%0d duration=%0d",
                            pr,
                            pc,
                            pe_red_start[pr][pc],
                            pe_red_end[pr][pc],
                            pe_red_end[pr][pc] -
                            pe_red_start[pr][pc]
                        );

                    end
                end

                $display("===== END PE REDUCTION TABLE =====");

                /*
                 * Rearm for the next transaction.
                 */
                for (pr = 0; pr < 8; pr = pr + 1)
                    for (pc = 0; pc < 8; pc = pc + 1) begin
                        pe_red_start[pr][pc] <= -1;
                        pe_red_end[pr][pc]   <= -1;
                    end

            end

        end

    end


endmodule
