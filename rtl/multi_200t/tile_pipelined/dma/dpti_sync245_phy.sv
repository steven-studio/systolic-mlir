`default_nettype none

// ============================================================================
// dpti_sync245_phy.sv
//
// FT2232H FT245 synchronous-FIFO PHY (RX and TX), standalone.
//
// NOT YET INTEGRATED.  It is a candidate replacement for dpti_byte_rx +
// dpti_byte_tx and is exercised only by tb/tb_dpti_sync245_phy.sv.
//
// Spec: FTDI DS_FT2232H, FT_000061 v2.10 (2025-11-21), Table 3.6, section 4.4,
// Figure 4.4, Table 4.1.  The protocol facts this module is built on:
//   * A byte is read on every rising CLKOUT edge at which RXF# and RD# are both
//     low (RXF# as the chip drove it before that edge).
//   * OE# must be low at least one clock period before RD# goes low.
//   * A byte is written on every rising CLKOUT edge at which TXE# and WR# are
//     both low; "the external system must monitor TXE# and its own WR# to check
//     that data has been accepted".
//   * Only a read or a write cycle can be performed at any one time.
//
// Clocking assumption (integration, not this file): clk is phase-aligned to the
// FT2232H CLKOUT at the FPGA pins (e.g. an MMCM with zero-delay compensation),
// so that every pad register below meets Table 4.1 when packed into the IOB.
//
// ---------------------------------------------------------------------------
// Cycle model.  Edge k = a rising edge of clk (= of CLKOUT).
//
//   Pad registers (IOB):
//     inputs  rxf_q, txe_q, d_q   capture the pins as they stood before edge k
//     outputs rd_n_r, oe_n_r, wr_n_r, d_o_r, d_t_r drive the pins during (k, k+1]
//     -> the chip samples at edge k+1 exactly what was registered at edge k.
//
//   Shadows: rd_low_prev / wr_low_prev hold, after edge k, what RD# / WR# were
//   on the pins before edge k -- i.e. what the chip sampled at edge k.
//
//   RX qualification (capture edge):
//     after edge k, rx_hit = rd_low_prev && !rxf_q  <=>  the chip transferred a
//     byte at edge k, and that byte is in d_q.  It is pushed at edge k+1.
//     RD# alone is never taken as a transfer: a strobe that met RXF# high moves
//     nothing and is discarded.
//
//   RX flow control: the decision at edge k (RD# for edge k+1) cannot see the
//   transfers at edges k-1 and k (pushed at k and k+1) nor the one it requests
//   (pushed at k+2).  RD# is therefore asserted only while
//   count <= RX_FIFO_DEPTH - 3, which makes overflow impossible; the sticky
//   err_rx_overflow exists only to prove that in simulation and on hardware.
//
//   OE#: registered.  RD# may go low only at an edge where OE# is already low
//   on the pin, so OE# leads RD# by >= 1 clock.  There is no path from any pad
//   input to any pad output.
//
//   TX acceptance (explicit confirmation, half rate):
//     edge e0  WR# low, D = byte (head of the TX FIFO, not yet popped)
//     edge e1  chip samples WR#/TXE#; we raise WR# (gap) -- outcome unknown yet
//     edge e2  wr_low_prev && !txe_q  => accepted: pop, may present the next
//              byte on this same edge; otherwise the SAME byte is presented
//              again.  A new byte is never presented while an earlier outcome
//              is unknown, so a rejected byte can never be overtaken.
//     Throughput is one byte per two clocks (30 MB/s at 60 MHz).
//
//   Arbitration: modes IDLE (nobody drives D), RX (OE# low, chip drives D),
//   TX (FPGA drives D).  Every change of bus owner passes through IDLE: the
//   edge that releases the bus (OE# high, or D to high-Z) is followed by
//   TURN_CYCLES + 1 edges before the next owner is enabled (3 clocks, 50 ns,
//   at the default 2).  Table 4.1 gives no OE#-high-to-D-released time;
//   TURN_CYCLES is a design margin, not a datasheet value.  A mode is left
//   (a) when it has nothing to do and the other side has work, or (b) after
//   RX_BURST_MAX / TX_BURST_MAX clocks if the other side is waiting.  The last
//   mode served loses the next tie.
//
// Reset: arst is asynchronous, active high (e.g. !mmcm_locked || system
// reset).  It forces RD#/OE#/WR# high and D to high-Z immediately, without a
// clock; release is synchronized to clk.  Bytes held inside the PHY when arst
// asserts are discarded.
// ============================================================================

module dpti_sync245_phy #(
    parameter integer RX_FIFO_DEPTH = 8,    // power of two, >= 8
    parameter integer TX_FIFO_DEPTH = 4,    // power of two, >= 4
    parameter integer TURN_CYCLES   = 2,    // >= 1
    parameter integer RX_BURST_MAX  = 64,
    parameter integer TX_BURST_MAX  = 64
) (
    input  wire        clk,
    input  wire        arst,

    // ---- FT2232H pads (to IOBUF / IBUF / OBUF at integration) ----
    input  wire [7:0]  pad_d_i,
    output wire [7:0]  pad_d_o,
    output wire [7:0]  pad_d_t,      // 1 = high-Z (Xilinx IOBUF T polarity)
    input  wire        pad_rxf_n,
    input  wire        pad_txe_n,
    output wire        pad_rd_n,
    output wire        pad_oe_n,
    output wire        pad_wr_n,

    // ---- RX byte stream: FT2232H -> fabric (valid/ready) ----
    output wire [7:0]  rx_data,
    output wire        rx_valid,
    input  wire        rx_ready,

    // ---- TX byte stream: fabric -> FT2232H (valid/ready) ----
    input  wire [7:0]  tx_data,
    input  wire        tx_valid,
    output wire        tx_ready,

    // ---- status ----
    output wire [1:0]  bus_mode,         // 0 IDLE, 1 RX, 2 TX
    output reg         err_rx_overflow,  // sticky; must never set
    output reg  [31:0] rx_xfer_count,    // bytes the chip handed over
    output reg  [31:0] tx_accept_count,  // bytes the chip confirmed
    output reg  [31:0] tx_reject_count   // WR# strobes that met TXE# high
);

    localparam [1:0] M_IDLE = 2'd0;
    localparam [1:0] M_RX   = 2'd1;
    localparam [1:0] M_TX   = 2'd2;

    localparam integer RX_LIMIT = RX_FIFO_DEPTH - 3;
    localparam integer BW       = (RX_BURST_MAX > TX_BURST_MAX) ? $clog2(RX_BURST_MAX + 1)
                                                                : $clog2(TX_BURST_MAX + 1);
    localparam integer TW       = $clog2(TURN_CYCLES + 1);
    localparam integer RCW      = $clog2(RX_FIFO_DEPTH) + 1;

    localparam [RCW-1:0] RX_LIMIT_C   = RX_LIMIT[RCW-1:0];
    localparam [TW-1:0]  TURN_C       = TURN_CYCLES[TW-1:0];
    localparam [BW-1:0]  RX_BURST_C   = RX_BURST_MAX[BW-1:0];
    localparam [BW-1:0]  TX_BURST_C   = TX_BURST_MAX[BW-1:0];

    // ------------------------------------------------------------------
    // Reset: asynchronous assert, synchronous release.
    // ------------------------------------------------------------------
    (* ASYNC_REG = "TRUE" *) reg [1:0] rst_sync;

    always @(posedge clk or posedge arst) begin
        if (arst)
            rst_sync <= 2'b11;
        else
            rst_sync <= {rst_sync[0], 1'b0};
    end

    wire rst = rst_sync[1];

    // ------------------------------------------------------------------
    // Pad registers.  Each pad connects only to its own register.
    // ------------------------------------------------------------------
    (* IOB = "TRUE" *) reg [7:0] d_q;
    (* IOB = "TRUE" *) reg       rxf_q;
    (* IOB = "TRUE" *) reg       txe_q;

    (* IOB = "TRUE" *) reg       rd_n_r;
    (* IOB = "TRUE" *) reg       oe_n_r;
    (* IOB = "TRUE" *) reg       wr_n_r;
    (* IOB = "TRUE" *) reg [7:0] d_o_r;
    (* IOB = "TRUE" *) reg [7:0] d_t_r;   // one register per bit, one per IOB

    always @(posedge clk) begin
        d_q <= pad_d_i;
    end

    always @(posedge clk or posedge rst) begin
        if (rst) begin
            rxf_q <= 1'b1;
            txe_q <= 1'b1;
        end
        else begin
            rxf_q <= pad_rxf_n;
            txe_q <= pad_txe_n;
        end
    end

    assign pad_rd_n = rd_n_r;
    assign pad_oe_n = oe_n_r;
    assign pad_wr_n = wr_n_r;
    assign pad_d_o  = d_o_r;
    assign pad_d_t  = d_t_r;

    // What the chip sampled at the last edge.
    reg rd_low_prev;
    reg wr_low_prev;

    // ------------------------------------------------------------------
    // RX FIFO.
    // ------------------------------------------------------------------
    wire                             rx_hit = rd_low_prev && !rxf_q;
    wire [7:0]                       rxf_head;
    /* verilator lint_off UNUSEDSIGNAL */
    wire [7:0]                       rxf_head1;   // RX never needs the look-ahead
    wire                             tx_empty;    // TX uses tx_count instead
    /* verilator lint_on UNUSEDSIGNAL */
    wire [$clog2(RX_FIFO_DEPTH):0]   rx_count;
    wire                             rx_full;
    wire                             rx_empty;

    dpti_sync245_fifo #(.DEPTH(RX_FIFO_DEPTH)) u_rx_fifo (
        .clk   (clk),
        .rst   (rst),
        .push  (rx_hit),
        .din   (d_q),
        .pop   (rx_valid && rx_ready),
        .head  (rxf_head),
        .head1 (rxf_head1),
        .count (rx_count),
        .full  (rx_full),
        .empty (rx_empty)
    );

    assign rx_valid = !rx_empty;
    assign rx_data  = rxf_head;

    wire rx_space = (rx_count <= RX_LIMIT_C);

    // ------------------------------------------------------------------
    // TX FIFO.  The head is popped only when the chip confirms it.
    // ------------------------------------------------------------------
    wire                             tx_acc = wr_low_prev && !txe_q;
    wire                             tx_rej = wr_low_prev &&  txe_q;
    wire [7:0]                       txf_head;
    wire [7:0]                       txf_head1;
    wire [$clog2(TX_FIFO_DEPTH):0]   tx_count;
    wire                             tx_full;

    dpti_sync245_fifo #(.DEPTH(TX_FIFO_DEPTH)) u_tx_fifo (
        .clk   (clk),
        .rst   (rst),
        .push  (tx_valid && tx_ready),
        .din   (tx_data),
        .pop   (tx_acc),
        .head  (txf_head),
        .head1 (txf_head1),
        .count (tx_count),
        .full  (tx_full),
        .empty (tx_empty)
    );

    assign tx_ready = !rst && !tx_full;

    // Bytes still waiting after this edge's confirmation, and the next one.
    wire       tx_left_ge1 = tx_acc ? (tx_count >= 2) : (tx_count >= 1);
    wire [7:0] tx_next     = tx_acc ? txf_head1 : txf_head;

    // ------------------------------------------------------------------
    // Requests seen by the arbiter (from registered pad state only).
    // ------------------------------------------------------------------
    wire rx_want = !rxf_q && rx_space;
    wire tx_want = tx_left_ge1 && !txe_q;

    // ------------------------------------------------------------------
    // Bus mode, strobes, data.
    // ------------------------------------------------------------------
    reg [1:0]    mode;
    reg [TW-1:0] turn_cnt;
    reg [BW-1:0] burst_cnt;
    reg          last_tx;     // 1: TX was served last, RX wins the next tie

    assign bus_mode = mode;

    wire turn_done = (turn_cnt >= TURN_C);

    // TX: an edge at which no WR# strobe is on the pins (so the previous one,
    // if any, has its outcome in tx_acc / tx_rej now).
    wire tx_slot = wr_n_r;

    always @(posedge clk or posedge rst) begin
        if (rst) begin
            rd_n_r          <= 1'b1;
            oe_n_r          <= 1'b1;
            wr_n_r          <= 1'b1;
            d_o_r           <= 8'h00;
            d_t_r           <= 8'hFF;
            rd_low_prev     <= 1'b0;
            wr_low_prev     <= 1'b0;
            mode            <= M_IDLE;
            turn_cnt        <= {TW{1'b0}};
            burst_cnt       <= {BW{1'b0}};
            last_tx         <= 1'b0;
            err_rx_overflow <= 1'b0;
            rx_xfer_count   <= 32'd0;
            tx_accept_count <= 32'd0;
            tx_reject_count <= 32'd0;
        end
        else begin
            rd_low_prev <= !rd_n_r;
            wr_low_prev <= !wr_n_r;

            if (rx_hit)
                rx_xfer_count <= rx_xfer_count + 32'd1;
            if (rx_hit && rx_full)
                err_rx_overflow <= 1'b1;
            if (tx_acc)
                tx_accept_count <= tx_accept_count + 32'd1;
            if (tx_rej)
                tx_reject_count <= tx_reject_count + 32'd1;

            case (mode)

            // ----------------------------------------------------------
            M_IDLE: begin
                rd_n_r <= 1'b1;
                oe_n_r <= 1'b1;
                wr_n_r <= 1'b1;
                d_t_r  <= 8'hFF;

                if (!turn_done)
                    turn_cnt <= turn_cnt + 1'b1;

                if (turn_done) begin
                    if (rx_want && (!tx_want || last_tx)) begin
                        mode      <= M_RX;
                        oe_n_r    <= 1'b0;   // RD# follows no earlier than next edge
                        burst_cnt <= {BW{1'b0}};
                    end
                    else if (tx_want) begin
                        mode      <= M_TX;
                        d_t_r     <= 8'h00;
                        d_o_r     <= tx_next;
                        wr_n_r    <= 1'b0;   // data and WR# launched together
                        burst_cnt <= {BW{1'b0}};
                    end
                end
            end

            // ----------------------------------------------------------
            M_RX: begin
                if (burst_cnt != {BW{1'b1}})
                    burst_cnt <= burst_cnt + 1'b1;

                if (tx_want && (rxf_q || !rx_space || burst_cnt >= RX_BURST_C)) begin
                    // Leave: both strobes high together.  A transfer at this
                    // edge (RD# low before it) is already in d_q / rd_low_prev.
                    mode     <= M_IDLE;
                    rd_n_r   <= 1'b1;
                    oe_n_r   <= 1'b1;
                    turn_cnt <= {TW{1'b0}};
                    last_tx  <= 1'b0;
                end
                else begin
                    oe_n_r <= 1'b0;
                    // OE# is already low on the pins (oe_n_r == 0 here), so
                    // RD# low from this edge satisfies OE#-before-RD#.
                    rd_n_r <= !(!oe_n_r && !rxf_q && rx_space);
                end
            end

            // ----------------------------------------------------------
            M_TX: begin
                if (burst_cnt != {BW{1'b1}})
                    burst_cnt <= burst_cnt + 1'b1;

                if (!tx_slot) begin
                    // A strobe is on the pins: always follow it with a gap.
                    wr_n_r <= 1'b1;
                end
                else if (!tx_left_ge1 ||
                         (rx_want && (txe_q || burst_cnt >= TX_BURST_C))) begin
                    // Nothing in flight (outcome handled this edge): release.
                    mode     <= M_IDLE;
                    d_t_r    <= 8'hFF;
                    turn_cnt <= {TW{1'b0}};
                    last_tx  <= 1'b1;
                end
                else if (!txe_q) begin
                    d_o_r  <= tx_next;
                    wr_n_r <= 1'b0;
                end
            end

            default: begin
                mode <= M_IDLE;
            end

            endcase
        end
    end

endmodule

`default_nettype wire
