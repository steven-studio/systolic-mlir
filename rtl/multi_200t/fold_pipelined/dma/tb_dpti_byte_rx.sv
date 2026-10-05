`timescale 1ns/1ps
`default_nettype none

module tb_dpti_byte_rx;

    reg        dpti_clkout;
    reg        rst;

    reg [7:0]  dpti_d_in;
    reg        dpti_rxf_n;

    wire       dpti_rd_n;
    wire       dpti_oe_n;

    wire [7:0] byte_data;
    wire       byte_valid;
    reg        byte_ready;

    integer rd_pulses;

    dpti_byte_rx dut (
        .dpti_clkout (dpti_clkout),
        .rst         (rst),

        .dpti_d_in   (dpti_d_in),
        .dpti_rxf_n  (dpti_rxf_n),

        .dpti_rd_n   (dpti_rd_n),
        .dpti_oe_n   (dpti_oe_n),

        .byte_data   (byte_data),
        .byte_valid  (byte_valid),
        .byte_ready  (byte_ready)
    );

    initial begin
        dpti_clkout = 1'b0;
        forever #5 dpti_clkout = ~dpti_clkout;
    end

    // Present one physical input byte and wait until the DUT requests it.
    task present_byte;
        input [7:0] value;
        begin
            @(negedge dpti_clkout);

            dpti_d_in  = value;
            dpti_rxf_n = 1'b0;

            @(negedge dpti_rd_n);

            // The byte is captured at the next rising edge.
            @(posedge dpti_clkout);

            @(negedge dpti_clkout);
            dpti_rxf_n = 1'b1;
            dpti_d_in  = 8'h00;
        end
    endtask

    always @(negedge dpti_rd_n) begin
        rd_pulses = rd_pulses + 1;
    end

    initial begin
        dpti_d_in  = 8'h00;
        dpti_rxf_n = 1'b1;
        byte_ready = 1'b0;
        rd_pulses  = 0;

        rst = 1'b1;

        repeat (4)
            @(posedge dpti_clkout);

        rst = 1'b0;

        // --------------------------------------------------------------------
        // Byte #1.
        // --------------------------------------------------------------------

        present_byte(8'hA5);

        @(posedge dpti_clkout);
        #1;

        if (!byte_valid) begin
            $display("FAIL: byte_valid not asserted for first byte");
            $finish(1);
        end

        if (byte_data !== 8'hA5) begin
            $display(
                "FAIL: first byte=%02x expected=A5",
                byte_data
            );
            $finish(1);
        end

        // Hold downstream stalled for several cycles.
        repeat (4) begin
            @(posedge dpti_clkout);
            #1;

            if (!byte_valid) begin
                $display(
                    "FAIL: byte_valid dropped while byte_ready=0"
                );
                $finish(1);
            end

            if (byte_data !== 8'hA5) begin
                $display(
                    "FAIL: byte_data changed while stalled"
                );
                $finish(1);
            end
        end

        if (rd_pulses != 1) begin
            $display(
                "FAIL: generated %0d reads while first byte stalled",
                rd_pulses
            );
            $finish(1);
        end

        $display("PASS: first byte held under backpressure.");

        // Consume byte #1.
        @(negedge dpti_clkout);
        byte_ready = 1'b1;

        @(posedge dpti_clkout);

        @(negedge dpti_clkout);
        byte_ready = 1'b0;

        @(posedge dpti_clkout);
        #1;

        if (byte_valid) begin
            $display(
                "FAIL: byte_valid remained asserted after handshake"
            );
            $finish(1);
        end

        // --------------------------------------------------------------------
        // Byte #2 proves receiver can restart.
        // --------------------------------------------------------------------

        present_byte(8'h3C);

        @(posedge dpti_clkout);
        #1;

        if (!byte_valid) begin
            $display("FAIL: byte_valid not asserted for second byte");
            $finish(1);
        end

        if (byte_data !== 8'h3C) begin
            $display(
                "FAIL: second byte=%02x expected=3C",
                byte_data
            );
            $finish(1);
        end

        if (rd_pulses != 2) begin
            $display(
                "FAIL: rd_pulses=%0d expected=2",
                rd_pulses
            );
            $finish(1);
        end

        // Consume second byte.
        @(negedge dpti_clkout);
        byte_ready = 1'b1;

        @(posedge dpti_clkout);

        @(negedge dpti_clkout);
        byte_ready = 1'b0;

        $display("----------------------------------------");
        $display("DPTI BYTE RX TEST: PASS");
        $display("bytes=2 rd_pulses=%0d", rd_pulses);
        $display("----------------------------------------");

        $finish(0);
    end

endmodule

`default_nettype wire
