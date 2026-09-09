// -----------------------------------------------------------------------------
// dma_operand_writer_v2.sv -- one DMA beat per cycle into the operand buffers.
//
// The v1 writer (dma_operand_writer.sv) turns each 128-bit beat into four
// serial 32-bit writes because systolic_operand_buffer has one write port.
// That is the 1.00 word-per-cycle cap the paper measures.  This module is the
// same decode with the four-cycle walk removed: the beat goes to
// systolic_operand_buffer_v2 whole, and the buffer's layout (block on B,
// cyclic on A) lands it in one cycle.
//
// ---- THE CONTRACT IS STILL "BYTE-IDENTICAL TO THE UART PAYLOAD" -------------
//
// Nothing about the wire format changes.  The descriptor points at the same
// K_MAX*8*N-byte payload the UART path receives, in the same order, and this
// module writes word w of it to the same (matrix, bank, depth) the serial
// receiver would.  What changes is only how many of those writes happen per
// cycle.  The proof is tb_dma_operand_writer_v2: it walks the payload byte by
// byte through the rx_count decode and checks that every beat this module
// emits expands, word by word, to the golden sequence.
//
// ---- WHY THE DECODE NEEDS ONLY THE BEAT INDEX -------------------------------
//
// Word w = 4*beat + j, and every field the serial receiver slices out of the
// word index lives at bit 2 or above EXCEPT the two bits that j occupies:
//
//     A:  a_lane = w[LANE_W+2:3]   a_koff = w[2:0]      -> j = a_koff[1:0]
//     B:  b_koff = w[LANE_W+2:LANE_W]  b_lane = w[LANE_W-1:0] -> j = b_lane[1:0]
//
// So within a beat the four words share bank and win/koff[2] on the A side
// (four consecutive depths), and share depth on the B side (four consecutive
// lanes).  Decoding w0 = {beat, 2'b00} gives the cell of word 0; the buffer
// supplies the +j.  That is why wsel[1:0] is zero on a B beat and waddr[1:0]
// is zero on an A beat -- not a convention, a consequence of the format.
//
// A beat never straddles A and B: a chunk is 8*N words, a multiple of four.
//
// ---- THROUGHPUT ---------------------------------------------------------------
//
// One register stage, no backpressure: dst_full and dst_almost_full are tied
// low because the beat is consumed in the cycle after it is presented, every
// cycle.  The engine's credit logic therefore never stalls on the destination
// and r_stall_cycles should read ~0 on the v2 build; the fill rate becomes
// min(beta_mem, 4.00) = beta_mem.  words_written advances by four per beat so
// the DMA top's fill_complete test (words_written == RX_WORDS) is unchanged.
// -----------------------------------------------------------------------------

`default_nettype none

module dma_operand_writer_v2 #(
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
  output wire                     dst_full,        // -> dma_engine.dst_full (never)
  output wire                     dst_almost_full, // -> dma_engine.dst_almost_full (never)

  // ---- to the two systolic_operand_buffer_v2 instances -------------------
  // Only one of a_wr / b_wr is ever high: a beat lies wholly inside one chunk.
  output logic                    a_wr,
  output logic                    b_wr,
  output logic [$clog2(N)-1:0]    wsel,            // A: bank.  B: lane of word 0, [1:0] = 0
  output logic [$clog2(K_MAX)-1:0] waddr,          // A: depth of word 0, [1:0] = 0.  B: depth
  output logic [AXI_DATA_W-1:0]   wdata,           // the beat, word j at [32j +: 32]

  // ---- observability -----------------------------------------------------
  output logic [31:0]             words_written,
  output logic                    err_range,       // beat index past the payload
  input  wire                     clear
);

  localparam integer LANE_W    = $clog2(N);
  localparam integer K_W       = $clog2(K_MAX);
  localparam integer RX_BYTES  = K_MAX * 8 * N;
  localparam integer RX_WORDS  = RX_BYTES / 4;              // K_MAX*2*N
  localparam integer WORD_W    = $clog2(RX_WORDS);          // K_W + 1 + LANE_W
  localparam integer MAT_W     = WORD_W - (LANE_W + 3);     // K_W - 2
  localparam integer WIN_W     = MAT_W - 1;                 // K_W - 3
  localparam integer LAST_BEAT = RX_WORDS / 4 - 1;

  generate
    if (N < 4)
      $error("dma_operand_writer_v2: N must be >= 4, a B beat spans four lanes");
  endgenerate

  // ---- one register stage --------------------------------------------------
  logic                  valid_q;
  logic [AXI_DATA_W-1:0] data_q;
  logic [BEAT_W-1:0]     beat_q;
  logic                  in_range;   // suppress, not just flag, an out-of-range beat

  assign dst_full        = 1'b0;
  assign dst_almost_full = 1'b0;

  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      valid_q  <= 1'b0;
      data_q   <= '0;
      beat_q   <= '0;
      in_range <= 1'b0;
    end else begin
      valid_q <= dst_wr_en;
      if (dst_wr_en) begin
        data_q   <= dst_wr_data;
        beat_q   <= dst_wr_beat;
        in_range <= (dst_wr_beat <= BEAT_W'(LAST_BEAT));
      end
    end
  end

  // ---- decode: word 0 of the beat -> {matrix, bank, depth} ----------------
  wire [WORD_W-1:0] w0 = {beat_q[WORD_W-3:0], 2'b00};

  wire [MAT_W-1:0]  mat    = w0[WORD_W-1 -: MAT_W];
  wire              is_b   = mat[0];
  wire [WIN_W-1:0]  win    = mat[MAT_W-1:1];

  wire [LANE_W-1:0] a_lane = w0[LANE_W+2:3];
  wire [2:0]        a_koff = w0[2:0];               // [1:0] == 0 by construction
  wire [2:0]        b_koff = w0[LANE_W+2:LANE_W];
  wire [LANE_W-1:0] b_lane = w0[LANE_W-1:0];        // [1:0] == 0 by construction

  always_comb begin
    a_wr  = valid_q && in_range && !is_b;
    b_wr  = valid_q && in_range &&  is_b;
    wsel  = is_b ? b_lane : a_lane;
    waddr = is_b ? {win, b_koff} : {win, a_koff};
    wdata = data_q;
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
      if (a_wr || b_wr) words_written <= words_written + 32'd4;
      if (dst_wr_en && (dst_wr_beat > BEAT_W'(LAST_BEAT))) err_range <= 1'b1;
    end
  end

endmodule

`default_nettype wire
