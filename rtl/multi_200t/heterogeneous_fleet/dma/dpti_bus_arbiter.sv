`default_nettype none

// ============================================================================
// dpti_bus_arbiter.sv
//
// The FPGA's side of the FT2232H synchronous-FIFO bus, both directions, one
// owner at a time.
//
// Before this module systolic_dma_top wired dpti_byte_rx and dpti_byte_tx
// straight to the pins: the receiver drove OE# / RD# whenever RXF# was low,
// the transmitter drove D[7:0] / WR# whenever it held a byte and TXE# was
// low, and nothing kept the two apart. That was correct for a host that
// never writes while a result is in flight (the synchronous runtime) and a
// bus fight for any other host: FTDI AN_130, "Only a read or a write cycle
// can be performed at any one time."
//
// Rules this module keeps (AN_130, FT245 synchronous FIFO mode):
//   - D[7:0] is driven by the FT2232H while OE# is low, by the FPGA only
//     while OE# is high and WR# is low;
//   - OE# goes low at least one CLKOUT before RD# (bus turnaround);
//   - RD# only while RXF# is low, WR# only while TXE# is low;
//   - between a read phase and a write phase the bus passes through an idle
//     cycle with OE# high, so the FT2232H has released D before the FPGA
//     drives it.
//
// Policy: when both directions want the bus they alternate, each phase at
// most CHUNK_BYTES long. The result direction (tx) is served first when both
// are waiting: it is small (a 16-byte completion record plus a 64- or
// 256-byte tile) and every credit the host needs to send more comes back
// through it, while the host's writes sit comfortably in the FT2232H's own
// 1 KiB receive buffer for one chunk.
//
// The byte-level handshakes are exactly those of dpti_byte_rx (byte_valid
// held with the byte until byte_ready; the next physical read is requested
// only once the held byte is taken) and dpti_byte_tx (byte_ready = the sink
// can take it this cycle), so dpti_command_frontend and dpti_beat_to_byte are
// unchanged. The read rhythm is also dpti_byte_rx's -- RD# a registered
// one-cycle pulse, the byte captured on the following edge -- so a stream the
// board already handled at one byte per three CLKOUTs arrives at the same
// rate; faster bursts (RD# held low) are a later change, not this one.
//
// Clock domain: dpti_clkout. Reset: dpti_rst, synchronous, active high.
// ============================================================================

module dpti_bus_arbiter #(
    parameter integer CHUNK_BYTES = 64
) (
    input  wire       clk,          // dpti_clkout, 60 MHz
    input  wire       rst,

    // ---- FT2232H pins ------------------------------------------------------
    input  wire [7:0] dpti_d_in,    // the bus, as the FT2232H drives it
    input  wire       dpti_rxf_n,
    input  wire       dpti_txe_n,
    output wire       dpti_rd_n,
    output wire       dpti_oe_n,
    output wire       dpti_wr_n,
    output wire [7:0] dpti_d_out,   // what the FPGA drives...
    output wire       dpti_d_oe,    // ...and when

    // ---- host -> FPGA bytes (to dpti_command_frontend) ---------------------
    output reg  [7:0] rx_byte_data,
    output reg        rx_byte_valid,
    input  wire       rx_byte_ready,

    // ---- FPGA -> host bytes (from dpti_beat_to_byte) -----------------------
    input  wire [7:0] tx_byte_data,
    input  wire       tx_byte_valid,
    output wire       tx_byte_ready,

    // ---- observation --------------------------------------------------------
    output wire [2:0] state_o,
    output reg  [31:0] turnarounds   // phases entered, either direction
);

    localparam [2:0]
        S_IDLE    = 3'd0,
        S_RX_TURN = 3'd1,   // OE# low, no RD# yet: the FT2232H turns the bus around
        S_RX      = 3'd2,
        S_TX      = 3'd3;

    reg [2:0]  state;
    reg        rd_active;        // the registered one-cycle RD# pulse
    reg [15:0] chunk_left;

    assign state_o = state;

    // A byte can be taken when none is held (dpti_byte_rx's rule).
    wire rx_room = !rx_byte_valid;
    wire rx_want = !dpti_rxf_n && rx_room;
    wire tx_want = tx_byte_valid && !dpti_txe_n;

    // ------------------------------------------------------------------------
    // Pins.
    // ------------------------------------------------------------------------
    // OE# is low for the whole read phase, from the turnaround cycle on.
    assign dpti_oe_n = !((state == S_RX_TURN) || (state == S_RX));
    assign dpti_rd_n = !rd_active;

    // The transmitter's handshake, only inside the write phase: a byte is
    // transferred in the cycle both sides are ready.
    wire tx_fire = (state == S_TX) && tx_byte_valid && !dpti_txe_n;
    assign tx_byte_ready = tx_fire;
    assign dpti_d_out    = tx_byte_data;
    assign dpti_d_oe     = tx_fire;
    assign dpti_wr_n     = !tx_fire;

    // ------------------------------------------------------------------------
    // The receiver's byte register (dpti_byte_rx, verbatim in behaviour).
    // ------------------------------------------------------------------------
    always @(posedge clk) begin
        if (rst) begin
            rx_byte_data  <= 8'h00;
            rx_byte_valid <= 1'b0;
        end
        else begin
            if (rd_active) begin
                rx_byte_data  <= dpti_d_in;
                rx_byte_valid <= 1'b1;
            end
            if (rx_byte_valid && rx_byte_ready)
                rx_byte_valid <= 1'b0;
        end
    end

    // ------------------------------------------------------------------------
    // Ownership.
    // ------------------------------------------------------------------------
    always @(posedge clk) begin
        if (rst) begin
            state       <= S_IDLE;
            rd_active   <= 1'b0;
            chunk_left  <= 16'd0;
            turnarounds <= 32'd0;
        end
        else begin
            rd_active <= 1'b0;

            case (state)

                // Pick a direction. tx first when both want the bus.
                S_IDLE: begin
                    if (tx_want) begin
                        state       <= S_TX;
                        chunk_left  <= CHUNK_BYTES[15:0];
                        turnarounds <= turnarounds + 32'd1;
                    end
                    else if (rx_want) begin
                        state       <= S_RX_TURN;
                        chunk_left  <= CHUNK_BYTES[15:0];
                        turnarounds <= turnarounds + 32'd1;
                    end
                end

                // One cycle with OE# low and RD# high: AN_130's "at least 1
                // clock period before driving RD# low".
                S_RX_TURN: begin
                    state <= S_RX;
                end

                // Request a byte when there is one and there is room for it;
                // the byte register above captures it one cycle later. Leave
                // at a byte boundary: no more bytes, no room (the parser is
                // stalled) or the chunk is up and the other side is waiting.
                S_RX: begin
                    if (!rd_active && !dpti_rxf_n && rx_room &&
                        (chunk_left != 16'd0 || !tx_byte_valid)) begin
                        rd_active  <= 1'b1;
                        if (chunk_left != 16'd0)
                            chunk_left <= chunk_left - 16'd1;
                    end
                    else if (!rd_active) begin
                        // Nothing to request this cycle. Stay if a byte is
                        // merely late (RXF# high for a moment) and nobody
                        // else wants the bus; otherwise release it.
                        if (tx_byte_valid || (dpti_rxf_n && !rx_byte_valid) ||
                            (!rx_room && dpti_rxf_n))
                            state <= S_IDLE;
                    end
                end

                // Drive bytes while the sink has room and bytes keep coming.
                // Leave when there is nothing to send, when the FT2232H's
                // buffer is full, or when the chunk is up and a byte is
                // waiting to be read.
                S_TX: begin
                    if (tx_fire) begin
                        if (chunk_left != 16'd0)
                            chunk_left <= chunk_left - 16'd1;
                    end
                    if (!tx_byte_valid || dpti_txe_n ||
                        (chunk_left == 16'd0 && !dpti_rxf_n))
                        state <= S_IDLE;
                end

                default: state <= S_IDLE;

            endcase
        end
    end

endmodule

`default_nettype wire
