// -----------------------------------------------------------------------------
// dma_operand_writer.sv -- turn DMA beats into operand-buffer writes.
//
// Sits between dma_engine's destination stream and the two systolic_operand_buffer
// instances, replacing what the UART receive path in systolic_uart_top does with
// its rx_count byte counter.
//
// ---- THE CONTRACT IS "BYTE-IDENTICAL TO THE UART PAYLOAD" --------------------
//
// The descriptor must point at a copy of exactly the payload the UART path
// receives -- the K_MAX*8*N bytes between the header and the end marker, in the
// same order:
//
//     A[k window 0] B[k window 0] A[k window 1] B[k window 1] ...
//
// It must NOT include FRAME_START, the k_dim header or FRAME_END: those are UART
// framing, and k_dim reaches the array through the control plane instead.
//
// Given that, this module is provably the same decode.  systolic_uart_top writes
// payload word w on the byte where rx_count = 4w+3, and every field it slices out
// of rx_count lives at bit 2 or above:
//
//     rx_mat  = rx_count[RX_CNT_W-1:CHUNK_W]        ->  w[WORD_W-1:LANE_W+3]
//     a_lane  = rx_count[CHUNK_W-1:5]               ->  w[LANE_W+2:3]
//     a_koff  = rx_count[4:2]                       ->  w[2:0]
//     b_koff  = rx_count[CHUNK_W-1:CHUNK_W-3]       ->  w[LANE_W+2:LANE_W]
//     b_lane  = rx_count[LANE_W+1:2]                ->  w[LANE_W-1:0]
//
// so every one of them is a plain bit slice of the payload WORD index, with no
// arithmetic anywhere.  And w = 4*beat + j is a concatenation, not a multiply,
// because j is two bits.  tb_dma_operand_writer.sv reimplements the rx_count
// decode as a golden model and checks the two agree on every word of a full
// payload -- that check is the proof, not this comment.
//
// ---- WHY FOUR CYCLES PER BEAT -----------------------------------------------
//
// systolic_operand_buffer has ONE 32-bit write port: a single wsel, waddr and
// wdata.  A DMA beat is 128 bits -- four fp32 words -- so it takes four cycles
// to land.  That caps operand fill at 1.00 word per ui_clk cycle, against a
// measured DDR3 supply of 3.63 and a fold-average array demand of 11.2.
//
//     THE BUFFER WRITE PORT, NOT DDR3, IS THE BINDING CONSTRAINT HERE.
//
// This module deliberately does not fix that.  It exists so bring-up step 3 can
// be reached without touching the array side at all: DMA in, one fold, compare
// bit-exact against the UART-fed result.  Correctness first, at 1.00 words per
// cycle; then widen the write path and measure the difference -- which is a
// cost-model decision worth reporting, not just an optimisation.
//
// When that widening happens, the two matrices want different fixes, and the
// asymmetry comes from the wire format rather than from the hardware:
//
//     A chunk word order = lane*8 + koff   ->  a beat's four words share a BANK
//                                              and span four consecutive k
//     B chunk word order = koff*N + lane   ->  a beat's four words share an
//                                              ADDRESS and span four banks
//
// So per-bank write ports fix B (four words per cycle) and do nothing for A;
// an asymmetric BRAM (128-bit write port, 32-bit read port) fixes both.  Note
// that a beat is never split across A and B: a chunk is 8*N words, always a
// multiple of four, so every beat lies wholly inside one chunk.
// -----------------------------------------------------------------------------

`default_nettype none

module dma_operand_writer #(
  parameter integer N          = 8,      // array edge, = bank count
  parameter integer K_MAX      = 256,    // operand buffer depth
  parameter integer AXI_DATA_W = 128,
  parameter integer BEAT_W     = 16
) (
  input  wire                     clk,
  input  wire                     rst_n,

  // ---- from dma_engine's destination stream ------------------------------
  input  wire                     dst_wr_en,
  input  wire [BEAT_W-1:0]        dst_wr_beat,     // beat index in the descriptor
  input  wire [AXI_DATA_W-1:0]    dst_wr_data,
  output wire                     dst_full,        // -> dma_engine.dst_full
  output wire                     dst_almost_full, // -> dma_engine.dst_almost_full

  // ---- to the two systolic_operand_buffer instances ----------------------
  // Only one of a_wr / b_wr is ever high: a beat lies wholly inside one chunk.
  output logic                    a_wr,
  output logic                    b_wr,
  output logic [$clog2(N)-1:0]    wsel,
  output logic [$clog2(K_MAX)-1:0] waddr,
  output logic [31:0]             wdata,

  // ---- observability -----------------------------------------------------
  output logic [31:0]             words_written,
  output logic                    err_range,       // beat index past the payload
  input  wire                     clear
);

  localparam integer LANE_W    = $clog2(N);
  localparam integer K_W       = $clog2(K_MAX);
  localparam integer CHUNK_W   = 5 + LANE_W;                // $clog2(32*N)
  localparam integer RX_BYTES  = K_MAX * 8 * N;
  localparam integer RX_WORDS  = RX_BYTES / 4;              // K_MAX*2*N
  localparam integer WORD_W    = $clog2(RX_WORDS);          // K_W + 1 + LANE_W
  localparam integer MAT_W     = WORD_W - (LANE_W + 3);     // K_W - 2
  localparam integer WIN_W     = MAT_W - 1;                 // K_W - 3
  localparam integer LAST_BEAT = RX_WORDS / 4 - 1;

  // ---- beat register and the 4-word walk ---------------------------------
  logic                  busy;
  logic [1:0]            cnt;
  logic [AXI_DATA_W-1:0] data_q;
  logic [BEAT_W-1:0]     beat_q;
  // A beat past the end of the payload would wrap the word index and overwrite
  // operands that are already correct.  Flagging that is not enough -- the
  // damage is done by the time anyone reads err_range -- so the writes are
  // suppressed as well, the same way dma_engine refuses a misaligned
  // descriptor instead of moving plausible-looking garbage.
  logic                  in_range;

  // Accept a new beat while the last word of the current one is being written,
  // so the steady-state rate is exactly one beat per four cycles rather than
  // one per five.
  wire accept = dst_wr_en && (!busy || (cnt == 2'd3));

  assign dst_full        = busy && (cnt != 2'd3);
  // There is no queue in front of this module, so "almost full" and "full" are
  // the same condition.  dma_engine uses the former only to stop issuing new
  // bursts; leaving it equal to dst_full is conservative and correct.
  assign dst_almost_full = dst_full;

  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      busy     <= 1'b0;
      cnt      <= 2'd0;
      data_q   <= '0;
      beat_q   <= '0;
      in_range <= 1'b0;
    end else begin
      if (accept) begin
        data_q   <= dst_wr_data;
        beat_q   <= dst_wr_beat;
        in_range <= (dst_wr_beat <= BEAT_W'(LAST_BEAT));
        busy     <= 1'b1;
        cnt      <= 2'd0;
      end else if (busy) begin
        if (cnt == 2'd3) busy <= 1'b0;
        else             cnt  <= cnt + 1'b1;
      end
    end
  end

  // ---- decode: payload word index -> {matrix, bank, address} -------------
  // w = 4*beat + cnt, which is a concatenation because cnt is two bits.
  wire [WORD_W-1:0] w = {beat_q[WORD_W-3:0], cnt};

  wire [MAT_W-1:0]  mat    = w[WORD_W-1 -: MAT_W];
  wire              is_b   = mat[0];
  wire [WIN_W-1:0]  win    = mat[MAT_W-1:1];

  wire [LANE_W-1:0] a_lane = w[LANE_W+2:3];
  wire [2:0]        a_koff = w[2:0];
  wire [2:0]        b_koff = w[LANE_W+2:LANE_W];
  wire [LANE_W-1:0] b_lane = w[LANE_W-1:0];

  // The word being written this cycle, LSB-first exactly as the UART assembles
  // it from four bytes: payload byte 4w+0 is the low byte of word w, and AXI
  // byte 0 of a beat is its lowest address, so no lane swap is needed.
  wire [31:0] word_sel = data_q[32*cnt +: 32];

  always_comb begin
    a_wr  = busy && in_range && !is_b;
    b_wr  = busy && in_range &&  is_b;
    wsel  = is_b ? b_lane : a_lane;
    waddr = is_b ? {win, b_koff} : {win, a_koff};
    wdata = word_sel;
  end

  // ---- observability -----------------------------------------------------
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      words_written <= '0;
      err_range     <= 1'b0;
    end else if (clear) begin
      words_written <= '0;
      err_range     <= 1'b0;
    end else begin
      if (a_wr || b_wr) words_written <= words_written + 1'b1;
      // A descriptor longer than the payload would wrap the word index and
      // overwrite operands that are already correct -- silently.  Flag it.
      if (accept && (dst_wr_beat > BEAT_W'(LAST_BEAT))) err_range <= 1'b1;
    end
  end

endmodule

`default_nettype wire
