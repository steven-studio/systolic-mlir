`timescale 1ns / 1ps
`default_nettype none

// -----------------------------------------------------------------------------
// operand_buffer_v2.sv -- the operand buffer with a beat-wide write port.
//
// READ SIDE: identical to systolic_operand_buffer.  N read addresses in, N
// words out one cycle later.  The feeder, the array, the control FSM and the
// cycle counter cannot tell which buffer they are talking to, which is what
// lets the 125-cycle equivalence check of the DMA top carry over unchanged.
//
// WRITE SIDE: one 128-bit beat per cycle -- four fp32 words, a bank select and
// a depth.  Which four (bank, depth) cells a beat fills is fixed by the wire
// format that the serial path and the DMA path share, and it differs per
// operand.  That asymmetry is the whole reason this module has two layouts:
//
//   B   chunk word order koff*N + lane.  A beat is four consecutive LANES at
//       one depth: four banks, one address.        LAYOUT_CYCLIC = 0 ("block").
//       Each bank keeps its one memory and gains its own write enable and its
//       own data lane of the beat.  No extra block RAM.
//
//   A   chunk word order lane*8 + koff.  A beat is four consecutive DEPTHS in
//       one lane: one bank, four addresses.        LAYOUT_CYCLIC = 1 ("cyclic").
//       A block RAM accepts one address per port per cycle, so no number of
//       per-bank ports helps.  The bank is split into four sub-banks by depth
//       modulo four, each K_MAX/4 deep -- the cyclic partition -- and the beat
//       writes the four sub-banks at one address.  The read side reads all
//       four at depth/4 and selects by depth mod 4 in a 4:1 mux placed AFTER
//       the block RAM output register, inside the cycle the synchronous read
//       already spends.
//
// THE MUX SELECT IS REGISTERED.  It is the depth's low two bits captured at
// the same edge that captures the read, not the live raddr.  With the live
// bits, a back-to-back read stream returns the previous cycle's sub-bank word
// under the current cycle's select; a read-one-address-then-wait check passes
// and only a streaming read catches it.  tb_dma_path_v2 does both.
//
// COST is granularity, not capacity: at K_MAX = 256 a bank is 1 KiB, already
// below the smallest primitive, so four sub-banks are four RAMB18 holding the
// bits of one.  The A instance goes from N to 4N primitives; the B instance
// does not change.  Whether the 4:1 mux stays inside the read cycle is a
// synthesis question, answered by the OOC run; if it does not, the fix is one
// more register stage and the fold constant H grows by one.
//
// PORT CONTRACT with dma_operand_writer_v2 (the writer zeroes the bits a
// layout does not use, and tb_dma_operand_writer_v2 checks that it does):
//   cyclic (A):  wsel  = bank,           waddr = depth of word 0, waddr[1:0] = 0
//   block  (B):  wsel  = lane of word 0, wsel[1:0] = 0,           waddr = depth
//   wdata[32*j +: 32] is word j of the beat, j = 0..3, as it arrives from AXI.
// Requires N_BANKS >= 4 (a B beat spans four lanes) and K_MAX a multiple of 4
// with K_MAX >= 8 (an A beat spans four depths inside one 8-deep k window).
// -----------------------------------------------------------------------------
module systolic_operand_buffer_v2 #(
    parameter int K_MAX         = 256,
    parameter int K_W           = $clog2(K_MAX),
    parameter int N_BANKS       = 8,
    parameter int SEL_W         = $clog2(N_BANKS),
    parameter bit LAYOUT_CYCLIC = 1'b0,        // 0 = block (B side), 1 = cyclic (A side)
    parameter int BEAT_W        = 128
)(
    input  wire                clk,

    input  wire                wr,
    input  wire [SEL_W-1:0]    wsel,
    input  wire [K_W-1:0]      waddr,
    input  wire [BEAT_W-1:0]   wdata,

    input  wire  [K_W-1:0]     raddr [0:N_BANKS-1],
    output wire  [31:0]        rdata [0:N_BANKS-1]
);

    localparam int WORDS     = BEAT_W / 32;      // words per beat, 4
    localparam int SUB_DEPTH = K_MAX / WORDS;    // depths per sub-bank
    localparam int SUB_W     = K_W - 2;          // sub-bank address width

    // Elaboration-time guards.  Both simulators and Vivado evaluate $error in
    // a generate condition; a geometry outside the contract stops here rather
    // than in a bench that happens not to cover it.
    generate
        if (N_BANKS < 4)
            $error("systolic_operand_buffer_v2: N_BANKS must be >= 4, a B beat spans four lanes");
        if ((K_MAX % WORDS) != 0 || K_MAX < 8)
            $error("systolic_operand_buffer_v2: K_MAX must be a multiple of 4 and >= 8");
        if (BEAT_W != 128)
            $error("systolic_operand_buffer_v2: BEAT_W must be 128 (four fp32 words)");
    endgenerate

    genvar gi, gj;

    generate
    if (!LAYOUT_CYCLIC) begin : BLOCK
        // -----------------------------------------------------------------
        // B side.  Bank gi takes word (gi mod 4) of every beat whose lane
        // group is gi/4.  Same memory shape as v1; only the enable and the
        // data lane are per bank now, which is what "per-bank write port"
        // means -- the RAMs always had their own ports, the v1 interface
        // serialised them.
        // -----------------------------------------------------------------
        for (gi = 0; gi < N_BANKS; gi = gi + 1) begin : BANK
            localparam int LANE = gi % WORDS;
            localparam int GRP  = gi / WORDS;

            (* ram_style = "block" *)
            logic [31:0] mem [0:K_MAX-1];
            logic [31:0] rdata_q;

            wire hit = wr && (int'(wsel >> 2) == GRP);

            always_ff @(posedge clk) begin
                if (hit)
                    mem[waddr] <= wdata[32*LANE +: 32];
                // synchronous read, one cycle, exactly as v1
                rdata_q <= mem[raddr[gi]];
            end

            assign rdata[gi] = rdata_q;
        end
    end else begin : CYCLIC
        // -----------------------------------------------------------------
        // A side.  Bank gi is four sub-banks; sub-bank j holds every depth
        // with depth mod 4 == j at address depth/4.  A beat writes all four
        // at waddr/4; a read reads all four at raddr/4 and picks one.
        // -----------------------------------------------------------------
        for (gi = 0; gi < N_BANKS; gi = gi + 1) begin : BANK
            wire              hit = wr && (wsel == SEL_W'(gi));
            wire [SUB_W-1:0]  wsub = waddr[K_W-1:2];
            wire [SUB_W-1:0]  rsub = raddr[gi][K_W-1:2];

            // the four registered read words, one per sub-bank, as one vector
            // so each sub-bank drives its own slice and nothing is multi-driven
            logic [WORDS*32-1:0] q;
            logic [1:0]          sel_q;

            for (gj = 0; gj < WORDS; gj = gj + 1) begin : SUB
                (* ram_style = "block" *)
                logic [31:0] mem [0:SUB_DEPTH-1];
                logic [31:0] rq;

                always_ff @(posedge clk) begin
                    if (hit)
                        mem[wsub] <= wdata[32*gj +: 32];
                    rq <= mem[rsub];
                end

                assign q[32*gj +: 32] = rq;
            end

            // The select is captured on the same edge as the read.  See the
            // header: the live raddr[1:0] would be one cycle early.
            always_ff @(posedge clk)
                sel_q <= raddr[gi][1:0];

            assign rdata[gi] = q[32*sel_q +: 32];
        end
    end
    endgenerate

endmodule

`default_nettype wire
