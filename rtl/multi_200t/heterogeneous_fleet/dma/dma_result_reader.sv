`default_nettype none
`timescale 1ns / 1ps

/*
 * dma_result_reader -- turn the array's result tile into DMA beats.
 *
 * The mirror of dma_operand_writer, and it answers the same question in the
 * same way: the DMA path and the serial path must put the SAME image in
 * memory, so this module's contract is not "emit the results somehow" but
 * "emit exactly the byte stream systolic_tx_source emits, grouped into
 * beats".  tb_dma_result_reader proves it by running the real tx_source
 * beside this module and comparing byte for byte -- the check is the proof,
 * not this comment.
 *
 * ---- THE WIRE FORMAT --------------------------------------------------
 *
 * systolic_tx_source walks a byte counter and slices it:
 *
 *     cnt[1:0]                = byte lane within the word, little endian
 *     cnt[2 +: LANE_W]        = col
 *     cnt[2+LANE_W +: LANE_W] = row
 *
 * so word index w = cnt >> 2 is {row, col}: row major, N*N words, four bytes
 * each.  A 128-bit beat is four consecutive words, and AXI puts byte k of a
 * beat in bits [8k+7 : 8k], so beat b carries words 4b..4b+3 with word j in
 * bits [32j+31 : 32j].  Every index here is therefore a bit slice of a
 * concatenation -- w = {bidx, j} with j two bits wide -- and there is no
 * multiplier anywhere, for the same reason there is none in the operand
 * writer.
 *
 * ---- WHAT IS DELIBERATELY NOT SENT ------------------------------------
 *
 * tx_source can append four bytes of cycle counter when CYCLE_COUNTER=1.
 * Those are not in the DMA image.  One extra word makes N*N+1 words, which
 * for N=8 is 65 words = 16.25 beats, and a partial beat would need wstrb on
 * the write engine and a byte length in the descriptor.  The counter is a
 * measurement channel, not payload; it keeps going out over the serial link
 * and over JTAG, where it already is.
 *
 * ---- WHEN C MAY BE READ -----------------------------------------------
 *
 * C is read combinationally over TOTAL_BEATS cycles, so it must be stable
 * for the whole transfer.  That is the same assumption tx_source makes while
 * it walks its 4*N*N bytes, and it holds for the same reason: the main FSM
 * only raises start once a fold's reduction has landed and does not start
 * another until done.  Nothing here enforces it -- feeding early corrupts
 * the tile silently, exactly as it does on the serial path.
 */

module dma_result_reader #(
  /* Array edge length.  Must be a power of two (the index slicing depends on
   * the alignment) and at least 2, so that N*N is a whole number of beats. */
  parameter int     N          = 8,
  parameter integer AXI_DATA_W = 128
) (
  input  wire clk,                       // array clock
  input  wire rst,                       // active high, as in tx_source

  // ---- main FSM interface, mirroring tx_source's send_go / all_done ------
  input  wire  start,                    // level
  output logic done,                     // 1-cycle pulse

  // ---- content ----------------------------------------------------------
  input  wire [31:0] C [0:N-1][0:N-1],

  // ---- CDC FIFO write side (array clock domain) -------------------------
  output logic                  wr_en,
  output logic [AXI_DATA_W-1:0] wr_data,
  input  wire                   wfull
);

  localparam int LANE_W         = $clog2(N);
  localparam int WORDS_PER_BEAT = AXI_DATA_W / 32;      // 4 at 128 bit
  localparam int J_W            = $clog2(WORDS_PER_BEAT);
  localparam int WORD_W         = 2 * LANE_W;           // log2(N*N)
  localparam int BIDX_W         = WORD_W - J_W;
  localparam int TOTAL_BEATS    = (N * N) / WORDS_PER_BEAT;

  typedef enum logic [1:0] { R_IDLE, R_RUN, R_DRAIN } rd_t;

  rd_t               rd;
  logic [BIDX_W-1:0] bidx;
  logic              started;

  // Beat assembly.  w = {bidx, j} is a concatenation, and row / col are bit
  // slices of it -- no arithmetic on the index path.
  always_comb begin
    wr_data = '0;
    for (int j = 0; j < WORDS_PER_BEAT; j++) begin
      logic [WORD_W-1:0] w;
      w = {bidx, J_W'(j)};
      wr_data[32*j +: 32] = C[w[WORD_W-1 -: LANE_W]][w[LANE_W-1:0]];
    end
  end

  assign wr_en = (rd == R_RUN) && !wfull;

  always_ff @(posedge clk) begin
    if (rst) begin
      rd      <= R_IDLE;
      bidx    <= '0;
      started <= 1'b0;
      done    <= 1'b0;
    end else begin
      done <= 1'b0;

      case (rd)
        R_IDLE: begin
          // Same rearm semantics as tx_source: one transfer per raising of
          // start, and start must fall before another can begin.  Without
          // this a level-held start would stream the tile forever.
          if (start && !started) begin
            started <= 1'b1;
            bidx    <= '0;
            rd      <= R_RUN;
          end
          if (!start) started <= 1'b0;
        end

        R_RUN: begin
          if (wr_en) begin
            if (bidx == BIDX_W'(TOTAL_BEATS - 1)) begin
              rd <= R_DRAIN;
            end else begin
              bidx <= bidx + 1'b1;
            end
          end
        end

        R_DRAIN: begin
          // done pulses one cycle after the last beat is accepted, so a
          // consumer that watches done can assume the FIFO write has landed.
          done <= 1'b1;
          rd   <= R_IDLE;
        end

        default: rd <= R_IDLE;
      endcase
    end
  end

endmodule

`default_nettype wire
