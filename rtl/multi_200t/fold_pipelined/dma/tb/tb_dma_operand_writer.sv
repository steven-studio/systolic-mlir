// -----------------------------------------------------------------------------
// tb_dma_operand_writer.sv -- equivalence proof against the UART decode.
//
// The golden model below is systolic_uart_top's rx_count decode, rewritten
// line for line:
//
//     rx_mat  = rx_count[RX_CNT_W-1:CHUNK_W]
//     rx_is_b = rx_mat[0]
//     rx_win  = rx_mat[.. :1]
//     a_lane  = rx_count[CHUNK_W-1:5]     a_koff = rx_count[4:2]
//     b_koff  = rx_count[CHUNK_W-1 -: 3]  b_lane = rx_count[LANE_W+1:2]
//     a_waddr = {rx_win, a_koff}          b_waddr = {rx_win, b_koff}
//     write happens on byte_pos == 3, i.e. rx_count % 4 == 3
//
// It walks the whole payload byte by byte, exactly as the UART path would, and
// records the resulting sequence of (matrix, bank, address, data).  The DUT is
// then fed the same payload as 128-bit beats and must produce the identical
// sequence, in the same order.
//
// That is the check the writer's header claims: the DMA path and the UART path
// put the same word in the same place.  Any disagreement -- a swapped A/B
// decode, a wrong lane slice, a beat's four words in the wrong lane order --
// shows up as the first differing entry, with both sides printed.
//
// Payload data is the word index itself, so a misplaced word names its own
// origin instead of being an anonymous mismatch.
//
// RUN IT AT MORE THAN ONE GEOMETRY.  N and K_MAX change every field width, and
// N = 8 is the degenerate case where lane and koff are the same width and the
// A and B slices coincide:
//
//   iverilog -g2012 -o w8.out  tb/tb_dma_operand_writer.sv dma_operand_writer.sv
//   iverilog -g2012 -Ptb_dma_operand_writer.N=4 -o w4.out  ...
//   iverilog -g2012 -Ptb_dma_operand_writer.K_MAX=64 -o wk.out ...
// -----------------------------------------------------------------------------

`timescale 1ns/1ps
`default_nettype none

module tb_dma_operand_writer #(
  parameter int N     = 8,
  parameter int K_MAX = 64          // small by default: the bench walks it all
);

  localparam int DW       = 128;
  localparam int BEAT_W   = 16;
  localparam int LANE_W   = $clog2(N);
  localparam int K_W      = $clog2(K_MAX);
  localparam int CHUNK_W  = 5 + LANE_W;
  localparam int RX_BYTES = K_MAX * 8 * N;
  localparam int RX_CNT_W = $clog2(RX_BYTES);
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
  wire [31:0]        wdata;
  wire [31:0]        words_written;
  wire               err_range;

  dma_operand_writer #(
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

  // ---- capture what the DUT does ----------------------------------------
  int idx = 0;
  always @(posedge clk) if (rst_n && (a_wr || b_wr)) begin
    if (idx >= g_isb.size()) begin
      fail($sformatf("write %0d past the end of the payload", idx));
    end else begin
      if (int'(b_wr) !== g_isb[idx])
        fail($sformatf("word %0d: matrix %s, golden says %s",
                       idx, b_wr ? "B" : "A", g_isb[idx] ? "B" : "A"));
      if (int'(wsel) !== g_bank[idx])
        fail($sformatf("word %0d: bank %0d, golden %0d", idx, wsel, g_bank[idx]));
      if (int'(waddr) !== g_addr[idx])
        fail($sformatf("word %0d: addr %0d, golden %0d", idx, waddr, g_addr[idx]));
      if (int'(wdata) !== g_data[idx])
        fail($sformatf("word %0d: data %0d, golden %0d", idx, wdata, g_data[idx]));
    end
    idx++;
  end

  // ---- drive the payload as beats ---------------------------------------
  int t0, t1;

  initial begin
    $display("\n=== tb_dma_operand_writer  N=%0d K_MAX=%0d ==================",
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
      // word 4i in the low lane, 4i+3 in the high lane -- byte order on the
      // wire, unchanged from DRAM through AXI to here
      dst_wr_data = { 32'(4*i + 3), 32'(4*i + 2), 32'(4*i + 1), 32'(4*i) };
      dst_wr_beat = BEAT_W'(i);
      dst_wr_en   = 1;
      @(negedge clk);
      while (dst_full) @(negedge clk);   // honour backpressure
    end
    dst_wr_en = 0;
    repeat (10) @(negedge clk);
    t1 = $time;

    if (idx != RX_WORDS)
      fail($sformatf("%0d words written, expected %0d", idx, RX_WORDS));
    if (words_written != RX_WORDS)
      fail("words_written counter disagrees with the observed writes");
    if (err_range)
      fail("err_range set on a payload-sized descriptor");

    $display("    words checked = %0d", idx);
    $display("    cycles per beat = %0.2f  (four is the single-port limit)",
             real'(t1 - t0) / 10.0 / real'(N_BEATS));

    // range guard
    dst_wr_beat = BEAT_W'(N_BEATS + 4);
    dst_wr_data = '0;
    dst_wr_en   = 1; @(negedge clk); dst_wr_en = 0;
    repeat (8) @(negedge clk);
    if (!err_range) fail("err_range not set for a beat past the payload");

    $display("=============================================================");
    if (errors == 0) $display("  DECODE IS IDENTICAL TO THE UART PATH\n");
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
