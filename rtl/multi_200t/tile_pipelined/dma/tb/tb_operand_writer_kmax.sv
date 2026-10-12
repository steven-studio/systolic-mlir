// -----------------------------------------------------------------------------
// tb_operand_writer_kmax.sv -- dma_operand_writer at ANY K_MAX, against the
// host's packing.
//
// tb_dma_operand_writer.sv proves the writer equal to the UART decode, which
// exists only for slabs of whole windows at a power-of-two K_MAX.  This bench
// checks the other side of the contract at any K_MAX: the runtime
// (runtime/fpga_matmul_tiled.cpp) puts operand A[lane][step] at word
//
//     (step / 8) * 128 + lane * 8 + step % 8
//
// and B[step][lane] at word (step / 8) * 128 + 64 + (step % 8) * 8 + lane,
// for every step of a slab of ceil(K_MAX / 8) windows.  Fed that slab as
// 128-bit beats (word w = w itself, so a misplaced word names its origin),
// the writer must emit exactly one write per word, in order, with
//
//     a_wr / b_wr = the matrix,  wsel = the lane,  waddr = the step
//
// -- the FULL step, never truncated to the buffer's width: steps >= K_MAX are
// padding the caller drops at the buffer, and a truncated step could alias a
// real one.  words_written must count the slab, and a beat past it must raise
// err_range and write nothing.
//
//   iverilog -g2012 -Ptb_operand_writer_kmax.K_MAX=17 -o w.out \
//       tb/tb_operand_writer_kmax.sv dma_operand_writer.sv && vvp w.out
// -----------------------------------------------------------------------------

`timescale 1ns/1ps
`default_nettype none

module tb_operand_writer_kmax #(
  parameter int K_MAX = 17
);

  localparam int N        = 8;                         // wire lanes
  localparam int DW       = 128;
  localparam int BEAT_W   = 16;
  localparam int SLAB_K   = ((K_MAX + 7) / 8) * 8;
  localparam int WADDR_W  = $clog2(SLAB_K);
  localparam int RX_WORDS = SLAB_K * 2 * N;
  localparam int N_BEATS  = RX_WORDS / 4;

  logic clk = 0, rst_n = 0;
  always #5 clk = ~clk;

  logic              dst_wr_en   = 0;
  logic [BEAT_W-1:0] dst_wr_beat = 0;
  logic [DW-1:0]     dst_wr_data = 0;
  wire               dst_full, dst_almost_full;
  wire               a_wr, b_wr;
  wire [2:0]         wsel;
  wire [WADDR_W-1:0] waddr;
  wire [31:0]        wdata;
  wire [31:0]        words_written;
  wire               err_range;

  dma_operand_writer #(.N (N), .K_MAX (K_MAX), .AXI_DATA_W (DW), .BEAT_W (BEAT_W)) dut (
    .clk (clk), .rst_n (rst_n),
    .dst_wr_en (dst_wr_en), .dst_wr_beat (dst_wr_beat), .dst_wr_data (dst_wr_data),
    .dst_full (dst_full), .dst_almost_full (dst_almost_full),
    .a_wr (a_wr), .b_wr (b_wr), .wsel (wsel), .waddr (waddr), .wdata (wdata),
    .words_written (words_written), .err_range (err_range), .clear (1'b0)
  );

  int errors = 0;
  int seen   = 0;
  int padding_writes = 0;

  // The host's placement, inverted: word w -> (is_b, lane, step).
  task automatic expect_of(input int w, output bit is_b, output int lane, output int step);
    int win, off;
    begin
      win  = w / 128;
      off  = w % 128;
      is_b = (off >= 64);
      if (!is_b) begin
        lane = off / 8;
        step = win * 8 + off % 8;
      end else begin
        lane = (off - 64) % 8;
        step = win * 8 + (off - 64) / 8;
      end
    end
  endtask

  // Every write the writer emits, checked as it happens.
  always @(posedge clk) begin
    if (a_wr || b_wr) begin
      bit e_b; int e_lane, e_step;
      expect_of(seen, e_b, e_lane, e_step);
      if (a_wr && b_wr) begin
        $display("  FAIL word %0d: a_wr and b_wr together", seen); errors++;
      end
      if (b_wr !== e_b || int'(wsel) != e_lane || int'(waddr) != e_step || int'(wdata) != seen) begin
        if (errors < 8)
          $display("  FAIL word %0d: got %s lane %0d step %0d data %0d, host put %s lane %0d step %0d",
                   seen, b_wr ? "B" : "A", wsel, waddr, wdata, e_b ? "B" : "A", e_lane, e_step);
        errors++;
      end
      if (e_step >= K_MAX) padding_writes++;
      seen++;
    end
  end

  initial begin
    repeat (3) @(negedge clk);
    rst_n = 1;
    @(negedge clk);
    for (int b = 0; b < N_BEATS; b++) begin
      dst_wr_beat = BEAT_W'(b);
      for (int j = 0; j < 4; j++) dst_wr_data[32*j +: 32] = 32'(4 * b + j);
      dst_wr_en = 1;
      @(negedge clk);
      while (dst_full) @(negedge clk);
    end
    dst_wr_en = 0;
    repeat (8) @(negedge clk);

    if (seen != RX_WORDS) begin
      $display("  FAIL: %0d writes, the slab is %0d words", seen, RX_WORDS); errors++;
    end
    if (words_written != RX_WORDS) begin
      $display("  FAIL: words_written %0d, want %0d", words_written, RX_WORDS); errors++;
    end
    if (padding_writes != (SLAB_K - K_MAX) * 2 * N) begin
      $display("  FAIL: %0d padding-step writes, want %0d", padding_writes, (SLAB_K - K_MAX) * 2 * N);
      errors++;
    end
    if (err_range) begin
      $display("  FAIL: err_range set on a slab-sized payload"); errors++;
    end

    // One beat past the slab: flagged, and nothing written.
    dst_wr_beat = BEAT_W'(N_BEATS);
    dst_wr_data = '0;
    dst_wr_en = 1; @(negedge clk); dst_wr_en = 0;
    repeat (8) @(negedge clk);
    if (!err_range) begin
      $display("  FAIL: err_range not set for a beat past the slab"); errors++;
    end
    if (seen != RX_WORDS) begin
      $display("  FAIL: the out-of-range beat was written"); errors++;
    end

    $display("K_MAX=%0d: slab %0d steps (%0d windows), %0d words, %0d of them padding steps >= K_MAX, waddr %0d bits: %s",
             K_MAX, SLAB_K, SLAB_K / 8, RX_WORDS, padding_writes, WADDR_W,
             errors == 0 ? "PASS" : "FAIL");
    $finish;
  end

endmodule

`default_nettype wire
