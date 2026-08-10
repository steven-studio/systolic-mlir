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


endmodule
