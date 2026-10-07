`default_nettype none

// ============================================================================
// dpti_host_rx.sv
//
// FT2232H DPTI synchronous-FIFO RX endpoint.
//
// Clock domain:
//   dpti_clkout
//
// Input protocol:
//
//   dpti_rxf_n = 0  -> FT2232H has data available
//
// FPGA reads one byte by asserting:
//   dpti_oe_n = 0
//   dpti_rd_n = 0
//
// The received byte is captured on dpti_clkout.
//
// Command framing:
//
//   Byte 0 : opcode
//   Byte 1 : AXI4-Lite register address
//   Byte 2 : data[7:0]
//   Byte 3 : data[15:8]
//   Byte 4 : data[23:16]
//   Byte 5 : data[31:24]
//
// opcode:
//   0x01 = WRITE32
//
// After six bytes have been received:
//
//   cmd_valid = 1
//   cmd_addr  = byte[1]
//   cmd_data  = byte[2..5]
//
// cmd_valid is held until cmd_ready.
//
// IMPORTANT:
//   This module intentionally stops at the DPTI clock domain.
//   The resulting command is intended to cross into ui_clk through
//   an asynchronous FIFO in the next stage.
//
// No UART is used.
// ============================================================================

module dpti_host_rx #(
    parameter integer ADDR_W = 8
)(
    input  wire                  dpti_clkout,
    input  wire                  rst,

    // ------------------------------------------------------------------------
    // FT2232H DPTI interface.
    // ------------------------------------------------------------------------

    input  wire [7:0]            dpti_d_in,
    input  wire                  dpti_rxf_n,

    output wire                  dpti_rd_n,
    output wire                  dpti_oe_n,

    // ------------------------------------------------------------------------
    // Decoded command interface.
    // ------------------------------------------------------------------------

    output reg                   cmd_valid,
    input  wire                  cmd_ready,

    output reg  [ADDR_W-1:0]     cmd_addr,
    output reg  [31:0]           cmd_data
);

    localparam [2:0]
        ST_IDLE = 3'd0,
        ST_B0   = 3'd1,
        ST_B1   = 3'd2,
        ST_B2   = 3'd3,
        ST_B3   = 3'd4,
        ST_B4   = 3'd5,
        ST_B5   = 3'd6;

    reg [2:0] state;

    reg [7:0] opcode_r;
    reg [7:0] addr_r;

    reg [31:0] data_r;

    // ------------------------------------------------------------------------
    // DPTI synchronous-FIFO read request.
    //
    // One byte is transferred per dpti_clkout cycle:
    //
    //   cycle N:
    //       OE# = 0
    //       RD# = 0
    //
    //   cycle N+1 rising edge:
    //       capture D[7:0]
    //
    // RD# is therefore a registered one-cycle pulse.
    // ------------------------------------------------------------------------

    reg dpti_rd_active;

    assign dpti_oe_n =
        (!cmd_valid && !dpti_rxf_n) ? 1'b0 : 1'b1;

    assign dpti_rd_n =
        dpti_rd_active ? 1'b0 : 1'b1;

    // ------------------------------------------------------------------------
    // RX parser.
    //
    // The parser advances only when a requested DPTI byte is captured.
    // ------------------------------------------------------------------------

    always @(posedge dpti_clkout) begin
        if (rst) begin
            state <= ST_IDLE;

            opcode_r <= 8'd0;
            addr_r   <= 8'd0;
            data_r   <= 32'd0;

            cmd_valid <= 1'b0;
            cmd_addr  <= {ADDR_W{1'b0}};
            cmd_data  <= 32'd0;

            dpti_rd_active <= 1'b0;
        end
        else begin

            // ---------------------------------------------------------------
            // RD# is a one-cycle pulse.
            // ---------------------------------------------------------------

            if (dpti_rd_active) begin
                dpti_rd_active <= 1'b0;

                // -----------------------------------------------------------
                // Capture exactly one byte after the read request.
                // -----------------------------------------------------------

                case (state)

                    ST_IDLE: begin
                        opcode_r <= dpti_d_in;
                        state    <= ST_B1;
                    end

                    ST_B1: begin
                        addr_r <= dpti_d_in;
                        state  <= ST_B2;
                    end

                    ST_B2: begin
                        data_r[7:0] <= dpti_d_in;
                        state       <= ST_B3;
                    end

                    ST_B3: begin
                        data_r[15:8] <= dpti_d_in;
                        state        <= ST_B4;
                    end

                    ST_B4: begin
                        data_r[23:16] <= dpti_d_in;
                        state         <= ST_B5;
                    end

                    ST_B5: begin
                        data_r[31:24] <= dpti_d_in;

                        // ---------------------------------------------------
                        // Final byte completes one WRITE32 command.
                        // ---------------------------------------------------

                        if (opcode_r == 8'h01) begin
                            cmd_addr  <= addr_r;
                            cmd_data  <= {
                                dpti_d_in,
                                data_r[23:0]
                            };
                            cmd_valid <= 1'b1;
                        end

                        state <= ST_IDLE;
                    end

                    default: begin
                        state <= ST_IDLE;
                    end

                endcase
            end

            // ---------------------------------------------------------------
            // Completed command waits for the asynchronous FIFO.
            // ---------------------------------------------------------------

            if (cmd_valid && cmd_ready) begin
                cmd_valid <= 1'b0;
            end

            // ---------------------------------------------------------------
            // Start the next DPTI byte read only when:
            //
            //   1. no command is pending
            //   2. FIFO reports data available
            //   3. no read is already in progress
            // ---------------------------------------------------------------

            if (!cmd_valid &&
                !dpti_rxf_n &&
                !dpti_rd_active) begin

                dpti_rd_active <= 1'b1;
            end
        end
    end

endmodule

`default_nettype wire
