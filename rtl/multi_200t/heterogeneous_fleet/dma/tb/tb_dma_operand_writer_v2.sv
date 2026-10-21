// -----------------------------------------------------------------------------
// tb_dma_operand_writer_v2.sv -- equivalence proof of the beat-wide writer
// against the UART decode.
//
// Same golden model as tb_dma_operand_writer: systolic_uart_top's rx_count
// decode walked byte by byte over the whole payload, recording the sequence of
// (matrix, bank, address, data) the serial path writes.  The v2 DUT is fed the
// payload as 128-bit beats and emits one write per beat; the bench EXPANDS each
// beat back into four words under the buffer's contract --
//
//     A beat:  word j -> bank wsel,     depth waddr + j    (waddr[1:0] == 0)
//     B beat:  word j -> bank wsel + j, depth waddr        (wsel[1:0]  == 0)
//
// -- and requires the expansion to equal the golden sequence, in order.  The
// zero-low-bits conditions are checked explicitly: they are what lets the
// buffer land the beat without an adder, so a writer that violated them would
// pass an image check on a buffer that happened to add and fail on this one.
//
// Also checked: exactly one cycle per beat with dst_full never asserted,
// words_written == RX_WORDS, err_range on a beat past the payload.
//
// RUN AT MORE THAN ONE GEOMETRY.  N = 8 is the degenerate case where the A and
// B lane/koff slices have the same width:
//
//   iverilog -g2012 -o w2_8.out tb/tb_dma_operand_writer_v2.sv dma_operand_writer_v2.sv
//   iverilog -g2012 -Ptb_dma_operand_writer_v2.N=4  -o w2_4.out  ...
//   iverilog -g2012 -Ptb_dma_operand_writer_v2.N=16 -o w2_16.out ...
// -----------------------------------------------------------------------------

`timescale 1ns/1ps
`default_nettype none

module tb_dma_operand_writer_v2 #(
  parameter int N     = 8,
  parameter int K_MAX = 64
);

  localparam int DW       = 128;
  localparam int BEAT_W   = 16;
  localparam int LANE_W   = $clog2(N);
  localparam int K_W      = $clog2(K_MAX);
  localparam int CHUNK_W  = 5 + LANE_W;
  localparam int RX_BYTES = K_MAX * 8 * N;
  localparam int RX_WORDS = RX_BYTES / 4;
  localparam int N_BEATS  = RX_WORDS / 4;

  logic clk = 0, rst_n = 0;
  always #5 clk = ~clk;

  logic              dst_wr_en   = 0;
  logic [BEAT_W-1:0] dst_wr_beat = 0;
  logic [DW-1:0]     dst_wr_data = 0;
  wire               dst_full, dst_almost_full;

  wire               a_wr, b_wr;
  wire [LANE_W-1:0]  wsel;
  wire [K_W-1:0]     waddr;
  wire [DW-1:0]      wdata;
  wire [31:0]        words_written;
  wire               err_range;

  dma_operand_writer_v2 #(
    .N(N), .K_MAX(K_MAX), .AXI_DATA_W(DW), .BEAT_W(BEAT_W)
  ) dut (
    .clk(clk), .rst_n(rst_n),
    .dst_wr_en(dst_wr_en), .dst_wr_beat(dst_wr_beat), .dst_wr_data(dst_wr_data),
    .dst_full(dst_full), .dst_almost_full(dst_almost_full),
    .a_wr(a_wr), .b_wr(b_wr), .wsel(wsel), .waddr(waddr), .wdata(wdata),
    .words_written(words_written), .err_range(err_range), .clear(1'b0)
  );

  int errors = 0;
  task fail(input string m);
    begin errors++; if (errors < 12) $display("    FAIL: %s", m); end
  endtask

  // ---- golden model: the UART rx_count decode, walked byte by byte -------
  int g_isb[$], g_bank[$], g_addr[$], g_data[$];

  task automatic build_golden();
    int rx_count, rx_mat, is_b, win, a_lane, a_koff, b_koff, b_lane, wrd;
    begin
      g_isb.delete(); g_bank.delete(); g_addr.delete(); g_data.delete();
      for (rx_count = 0; rx_count < RX_BYTES; rx_count++) begin
        if ((rx_count % 4) == 3) begin            // byte_pos == 3
          rx_mat = rx_count >> CHUNK_W;
          is_b   = rx_mat & 1;
          win    = rx_mat >> 1;
          a_lane = (rx_count >> 5) & (N - 1);
          a_koff = (rx_count >> 2) & 7;
          b_koff = (rx_count >> (CHUNK_W - 3)) & 7;
          b_lane = (rx_count >> 2) & (N - 1);
          wrd    = rx_count >> 2;                 // payload word index
          g_isb.push_back(is_b);
          g_bank.push_back(is_b ? b_lane : a_lane);
          g_addr.push_back(is_b ? ((win << 3) | b_koff) : ((win << 3) | a_koff));
          g_data.push_back(wrd);
        end
      end
    end
  endtask

  // ---- capture what the DUT does, expanding each beat to four words ------
  int idx = 0;
  int beats_seen = 0;
  int full_seen = 0;

  always @(posedge clk) if (rst_n) begin
    if (dst_full || dst_almost_full) full_seen++;
    if (a_wr && b_wr) fail("a_wr and b_wr high together");
    if (a_wr || b_wr) begin
      beats_seen++;
      if (idx + 3 >= g_isb.size()) begin
        fail($sformatf("beat at word %0d runs past the end of the payload", idx));
      end else begin
        // contract bits
        if (a_wr && (waddr[1:0] !== 2'b00))
          fail($sformatf("word %0d: A beat with waddr[1:0] = %0d", idx, waddr[1:0]));
        if (b_wr && (wsel[1:0] !== 2'b00))
          fail($sformatf("word %0d: B beat with wsel[1:0] = %0d", idx, wsel[1:0]));
        for (int j = 0; j < 4; j++) begin
          int e_bank, e_addr;
          if (int'(b_wr) !== g_isb[idx+j])
            fail($sformatf("word %0d: matrix %s, golden says %s",
                           idx+j, b_wr ? "B" : "A", g_isb[idx+j] ? "B" : "A"));
          e_bank = b_wr ? (int'(wsel) + j) : int'(wsel);
          e_addr = b_wr ? int'(waddr)      : (int'(waddr) + j);
          if (e_bank !== g_bank[idx+j])
            fail($sformatf("word %0d: bank %0d, golden %0d", idx+j, e_bank, g_bank[idx+j]));
          if (e_addr !== g_addr[idx+j])
            fail($sformatf("word %0d: addr %0d, golden %0d", idx+j, e_addr, g_addr[idx+j]));
          if (int'(wdata[32*j +: 32]) !== g_data[idx+j])
            fail($sformatf("word %0d: data %0d, golden %0d",
                           idx+j, wdata[32*j +: 32], g_data[idx+j]));
        end
      end
      idx += 4;
    end
  end

  // ---- drive the payload as beats, one per cycle -------------------------
  int t0, t1;

  initial begin
    $display("\n=== tb_dma_operand_writer_v2  N=%0d K_MAX=%0d ===============",
             N, K_MAX);
    $display("    payload %0d B = %0d words = %0d beats",
             RX_BYTES, RX_WORDS, N_BEATS);
    build_golden();
    if (g_isb.size() != RX_WORDS)
      fail($sformatf("golden model produced %0d words, expected %0d",
                     g_isb.size(), RX_WORDS));

    repeat (4) @(negedge clk); rst_n = 1; repeat (2) @(negedge clk);

    t0 = $time;
    for (int i = 0; i < N_BEATS; i++) begin
      dst_wr_data = { 32'(4*i + 3), 32'(4*i + 2), 32'(4*i + 1), 32'(4*i) };
      dst_wr_beat = BEAT_W'(i);
      dst_wr_en   = 1;
      @(negedge clk);
      while (dst_full) @(negedge clk);   // must never happen on v2
    end
    dst_wr_en = 0;
    repeat (10) @(negedge clk);
    t1 = $time;

    if (idx != RX_WORDS)
      fail($sformatf("%0d words written, expected %0d", idx, RX_WORDS));
    if (beats_seen != N_BEATS)
      fail($sformatf("%0d beats written, expected %0d", beats_seen, N_BEATS));
    if (words_written != RX_WORDS)
      fail($sformatf("words_written = %0d, expected %0d", words_written, RX_WORDS));
    if (err_range)
      fail("err_range set on a payload-sized descriptor");
    if (full_seen != 0)
      fail($sformatf("dst_full/almost_full asserted in %0d cycles", full_seen));

    $display("    words checked = %0d  (%0d beats)", idx, beats_seen);
    $display("    cycles per beat = %0.2f  (v1: 4.00)",
             real'(t1 - t0 - 100) / 10.0 / real'(N_BEATS));

    // range guard
    dst_wr_beat = BEAT_W'(N_BEATS + 4);
    dst_wr_data = '0;
    dst_wr_en   = 1; @(negedge clk); dst_wr_en = 0;
    repeat (8) @(negedge clk);
    if (!err_range) fail("err_range not set for a beat past the payload");
    if (idx != RX_WORDS) fail("an out-of-range beat produced a write");

    $display("=============================================================");
    if (errors == 0) $display("  BEAT DECODE IS IDENTICAL TO THE UART PATH\n");
    else             $display("  %0d FAILURE(S)\n", errors);
    $finish;
  end

  initial begin
    #50_000_000;
    $display("  GLOBAL TIMEOUT");
    $finish;
  end

endmodule

`default_nettype wire
