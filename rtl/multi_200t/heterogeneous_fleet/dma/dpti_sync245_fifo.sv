`default_nettype none

// ============================================================================
// dpti_sync245_fifo.sv
//
// Small single-clock byte FIFO for dpti_sync245_phy.
//
//   * DEPTH must be a power of two, at least 4.
//   * A push while full is ignored (the PHY never relies on it; the RX side
//     reserves space before it strobes RD#, the TX side exports !full as ready).
//   * head  = oldest entry, valid when count >= 1.
//   * head1 = second-oldest entry, valid when count >= 2.  The TX path uses it
//     to present the next byte on the same edge the accepted one is popped.
//   * head / head1 are stable until a pop.
// ============================================================================

module dpti_sync245_fifo #(
    parameter integer DEPTH = 8
) (
    input  wire                       clk,
    input  wire                       rst,

    input  wire                       push,
    input  wire [7:0]                 din,
    input  wire                       pop,

    output wire [7:0]                 head,
    output wire [7:0]                 head1,
    output reg  [$clog2(DEPTH):0]     count,
    output wire                       full,
    output wire                       empty
);

    localparam integer AW = $clog2(DEPTH);
    localparam [AW:0]  FULL_COUNT = DEPTH[AW:0];

    reg [7:0]    mem [0:DEPTH-1];
    reg [AW-1:0] rp;
    reg [AW-1:0] wp;

    wire do_push = push && !full;
    wire do_pop  = pop  && !empty;

    assign full  = (count == FULL_COUNT);
    assign empty = (count == 0);
    assign head  = mem[rp];
    assign head1 = mem[rp + 1'b1];

    always @(posedge clk) begin
        if (do_push)
            mem[wp] <= din;
    end

    always @(posedge clk or posedge rst) begin
        if (rst) begin
            rp    <= {AW{1'b0}};
            wp    <= {AW{1'b0}};
            count <= {(AW+1){1'b0}};
        end
        else begin
            if (do_push)
                wp <= wp + 1'b1;
            if (do_pop)
                rp <= rp + 1'b1;
            case ({do_push, do_pop})
                2'b10:   count <= count + 1'b1;
                2'b01:   count <= count - 1'b1;
                default: count <= count;
            endcase
        end
    end

endmodule

`default_nettype wire
