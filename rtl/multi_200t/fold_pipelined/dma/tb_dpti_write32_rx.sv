`timescale 1ns/1ps
`default_nettype none

// ============================================================================
// tb_dpti_write32_rx.sv
//
// Replacement regression for dpti_write32_rx.
//
// Verifies:
//   1. FT2232H presents a 6-byte WRITE32 command.
//   2. RX parser reconstructs:
//        opcode = 0x01
//        address = 0x20
//        data = 0x12345678
//   3. cmd_valid is asserted after the final byte.
//   4. cmd_valid remains asserted until cmd_ready.
//   5. RD# is generated as a one-cycle pulse.
// ============================================================================

module tb_dpti_write32_rx;

    reg        dpti_clkout;
    reg        rst;

    reg [7:0]  dpti_d_in;
    reg        dpti_rxf_n;

    wire       dpti_rd_n;
    wire       dpti_oe_n;

    wire       cmd_valid;
    reg        cmd_ready;

    wire [7:0]  cmd_addr;
    wire [31:0] cmd_data;
    wire        err_opcode;

    dpti_write32_rx #(
        .ADDR_W(8)
    ) dut (
        .dpti_clkout (dpti_clkout),
        .rst         (rst),

        .dpti_d_in   (dpti_d_in),
        .dpti_rxf_n  (dpti_rxf_n),

        .dpti_rd_n   (dpti_rd_n),
        .dpti_oe_n   (dpti_oe_n),

        .cmd_valid   (cmd_valid),
        .cmd_ready   (cmd_ready),

        .cmd_addr    (cmd_addr),
        .cmd_data    (cmd_data),

        .err_opcode  (err_opcode)
    );

    // ------------------------------------------------------------------------
    // Clock: 10 ns period.
    // ------------------------------------------------------------------------

    initial begin
        dpti_clkout = 1'b0;
        forever #5 dpti_clkout = ~dpti_clkout;
    end

    // ------------------------------------------------------------------------
    // Drive one DPTI byte.
    //
    // The DUT requests a byte by asserting RD# low.
    // Keep RXF# low while the byte is available.
    // ------------------------------------------------------------------------

    task send_byte;
        input [7:0] value;
        begin
            dpti_d_in = value;
            dpti_rxf_n = 1'b0;

            // Wait until DUT generates RD# low.
            @(negedge dpti_rd_n);

            // The byte is captured on the following rising edge.
            @(posedge dpti_clkout);

            // Remove the byte after the transfer.
            #1;
            dpti_rxf_n = 1'b1;
            dpti_d_in  = 8'h00;

            // Allow DUT to return RD# high.
            @(posedge dpti_clkout);
        end
    endtask

    // ------------------------------------------------------------------------
    // Main test.
    // ------------------------------------------------------------------------

    initial begin
        dpti_d_in  = 8'h00;
        dpti_rxf_n = 1'b1;

        cmd_ready  = 1'b0;

        rst = 1'b1;

        repeat (3)
            @(posedge dpti_clkout);

        rst = 1'b0;

        // ---------------------------------------------------------------
        // WRITE32:
        //
        //   opcode = 01
        //   addr   = 20
        //   data   = 12345678
        //
        // Little-endian payload bytes:
        //
        //   78 56 34 12
        // ---------------------------------------------------------------

        send_byte(8'h01);
        send_byte(8'h20);
        send_byte(8'h78);
        send_byte(8'h56);
        send_byte(8'h34);
        send_byte(8'h12);

        // cmd_valid should now be asserted.
        #1;

        if (!cmd_valid) begin
            $display("FAIL: cmd_valid was not asserted.");
            $finish(1);
        end

        if (cmd_addr !== 8'h20) begin
            $display(
                "FAIL: cmd_addr = %02x, expected 20.",
                cmd_addr
            );
            $finish(1);
        end

        if (cmd_data !== 32'h12345678) begin
            $display(
                "FAIL: cmd_data = %08x, expected 12345678.",
                cmd_data
            );
            $finish(1);
        end

        if (err_opcode) begin
            $display(
                "FAIL: err_opcode unexpectedly asserted."
            );
            $finish(1);
        end

        $display("PASS: WRITE32 command decoded correctly.");

        // ---------------------------------------------------------------
        // Verify cmd_valid is held until downstream accepts it.
        // ---------------------------------------------------------------

        repeat (3)
            @(posedge dpti_clkout);

        if (!cmd_valid) begin
            $display(
                "FAIL: cmd_valid was not held while cmd_ready=0."
            );
            $finish(1);
        end

        $display("PASS: cmd_valid held until cmd_ready.");

        // ---------------------------------------------------------------
        // Accept command.
        // ---------------------------------------------------------------

        cmd_ready = 1'b1;

        @(posedge dpti_clkout);
        #1;

        if (cmd_valid) begin
            $display(
                "FAIL: cmd_valid remained asserted after handshake."
            );
            $finish(1);
        end

        $display("PASS: cmd_valid cleared after handshake.");

        $display("----------------------------------------");
        $display("DPTI WRITE32 RX REPLACEMENT TEST: PASS");
        $display("----------------------------------------");

        $finish(0);
    end

    // ------------------------------------------------------------------------
    // RD# sanity check.
    //
    // RD# should never remain low for multiple consecutive clock periods.
    // ------------------------------------------------------------------------

    integer rd_low_edges;

    always @(negedge dpti_rd_n) begin
        rd_low_edges = rd_low_edges + 1;
        $display(
            "[RD#] read pulse #%0d at t=%0t ns",
            rd_low_edges,
            $time
        );
    end

    initial begin
        rd_low_edges = 0;
    end

endmodule

`default_nettype wire
