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

    initial begin

        $display("=== UART FOLD TOP TEST ===");

        repeat (20) @(posedge clk);
        rst = 0;
        repeat (20) @(posedge clk);

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

        $display(
            "UART input complete: rx_count=%0d",
            dut.rx_count
        );

        repeat (50000) @(posedge clk);

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

endmodule
