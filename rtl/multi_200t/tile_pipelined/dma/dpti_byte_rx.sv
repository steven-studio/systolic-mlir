`default_nettype none

// ============================================================================
// dpti_byte_rx.sv
//
// Physical synchronous-FIFO byte receiver.
//
// This module owns the physical read handshake only.  It performs no command
// framing and no opcode decoding.
//
// Clock domain: dpti_clkout
//
// Output uses ordinary valid/ready semantics:
//   byte_valid remains asserted and byte_data remains stable until byte_ready.
// ============================================================================

module dpti_byte_rx (
    input  wire       dpti_clkout,
    input  wire       rst,

    input  wire [7:0] dpti_d_in,
    input  wire       dpti_rxf_n,

    output wire       dpti_rd_n,
    output wire       dpti_oe_n,

    output reg  [7:0] byte_data,
    output reg        byte_valid,
    input  wire       byte_ready
);

    reg dpti_rd_active;

    // Do not request another physical byte while the previous byte is waiting
    // for the downstream parser.
    assign dpti_oe_n =
        (!byte_valid && !dpti_rxf_n)
            ? 1'b0
            : 1'b1;

    assign dpti_rd_n =
        dpti_rd_active
            ? 1'b0
            : 1'b1;

    always @(posedge dpti_clkout) begin
        if (rst) begin
            dpti_rd_active <= 1'b0;
            byte_data      <= 8'h00;
            byte_valid     <= 1'b0;
        end
        else begin

            // A previously requested byte is captured now.
            if (dpti_rd_active) begin
                dpti_rd_active <= 1'b0;
                byte_data      <= dpti_d_in;
                byte_valid     <= 1'b1;
            end

            // Downstream consumed the held byte.
            if (byte_valid && byte_ready) begin
                byte_valid <= 1'b0;
            end

            // Request the next byte only when no byte is currently pending and
            // no physical read request is already in flight.
            if (!byte_valid &&
                !dpti_rxf_n &&
                !dpti_rd_active) begin

                dpti_rd_active <= 1'b1;
            end
        end
    end

endmodule

`default_nettype wire
