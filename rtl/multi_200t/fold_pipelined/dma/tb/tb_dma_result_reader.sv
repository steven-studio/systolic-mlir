`default_nettype none
`timescale 1ns / 1ps

/*
 * tb_dma_result_reader -- equivalence against the real serial path.
 *
 * The golden model is not a reimplementation of tx_source's byte order; it
 * IS systolic_tx_source, instantiated beside the DUT on the same C.  A
 * reimplementation can drift from the module it copies, and this contract is
 * exactly the kind that drifts: it is a wire format nobody looks at until
 * the two paths disagree on the board.
 *
 * The bench runs the serial source to completion, collects 4*N*N bytes,
 * runs the reader to completion, flattens its beats to bytes, and requires
 * the two byte streams to be identical.
 *
 * N is swept: N=8 is the geometry where the row and col slices sit at
 * convenient boundaries, so it is the one that hides an index bug.  N=4 and
 * N=16 are run for the same reason tb_dma_path has to be repeated at other
 * geometries on the operand side.
 *
 *   iverilog -g2012 -o tb.out tb_dma_result_reader.sv \
 *            ../dma_result_reader.sv ../../core/tx_source.sv && vvp tb.out
 */

module rdr_case #(parameter int N = 8) ();

  localparam integer AXI_DATA_W = 128;
  localparam int     TX_BYTES   = 4 * N * N;
  localparam int     TOTAL_BEATS = (N * N) / (AXI_DATA_W / 32);

  logic clk = 1'b0;
  logic rst = 1'b1;
  always #5 clk = ~clk;

  logic [31:0] C [0:N-1][0:N-1];

  // ---- golden: the real serial source -----------------------------------
  logic       go_tx;
  wire        tx_all_done;
  wire        m_valid;
  wire [7:0]  m_data;
  logic       m_ready;
  logic       ready_en = 1'b0;

  systolic_tx_source #(.DEBUG_MARKERS(1'b0), .CYCLE_COUNTER(1'b0), .N(N)) golden (
    .clk(clk), .rst(rst),
    .send_go(go_tx), .all_done(tx_all_done),
    .debug_pending(5'b0), .debug_accept(),
    .C(C), .cyc_latched(32'h0),
    .m_valid(m_valid), .m_data(m_data), .m_ready(m_ready)
  );

  // ---- DUT ---------------------------------------------------------------
  logic       go_rd;
  wire        rd_done;
  wire        wr_en;
  wire [AXI_DATA_W-1:0] wr_data;
  logic       wfull;
  logic       wfull_en = 1'b0;

  dma_result_reader #(.N(N), .AXI_DATA_W(AXI_DATA_W)) dut (
    .clk(clk), .rst(rst),
    .start(go_rd), .done(rd_done),
    .C(C),
    .wr_en(wr_en), .wr_data(wr_data), .wfull(wfull)
  );

  // ---- collection --------------------------------------------------------
  logic [7:0] gold_b [0:4*N*N-1];
  logic [7:0] dma_b  [0:4*N*N-1];
  integer     gold_n = 0;
  integer     beat_n = 0;
  integer     errors = 0;
  logic       saw_full_write = 1'b0;

  always_ff @(posedge clk) begin
    if (!rst) begin
      if (m_valid && m_ready) begin
        gold_b[gold_n] <= m_data;
        gold_n         <= gold_n + 1;
      end
      // Model what the FIFO actually does: a write while full is not
      // stored.  Collecting on wr_en alone would let a reader that ignores
      // wfull pass, because the bench would keep the beat the hardware
      // would have dropped.
      if (wr_en && wfull) saw_full_write <= 1'b1;
      if (wr_en && !wfull) begin
        for (int k = 0; k < AXI_DATA_W/8; k++)
          dma_b[beat_n*(AXI_DATA_W/8) + k] <= wr_data[8*k +: 8];
        beat_n <= beat_n + 1;
      end
    end
  end

  // Pseudo-random FIFO backpressure and serial readiness, so neither path is
  // exercised only in its free-running form.  Both are driven from an
  // always_ff rather than from the stimulus block: a blocking assignment made
  // at the clock edge races the always_ff that samples it, and the checker
  // then sees wr_en computed from one value of wfull and wfull from another.
  integer lfsr = 32'h0BAD_F00D;
  always_ff @(posedge clk) begin
    lfsr    <= {lfsr[30:0], lfsr[31]^lfsr[21]^lfsr[1]^lfsr[0]};
    wfull   <= wfull_en   & lfsr[3] & lfsr[12];
    m_ready <= ready_en ? (lfsr[5] | lfsr[9]) : 1'b0;
  end

  integer i, r, c;

  initial begin
    // A pattern in which every word is distinct AND row/col are readable
    // from the value, so a transposed index is visible in the diff rather
    // than only in a checksum.
    for (r = 0; r < N; r = r + 1)
      for (c = 0; c < N; c = c + 1)
        C[r][c] = {8'hC0, 8'(r), 8'(c), 8'(r*N + c)};

    ready_en = 1'b0; wfull_en = 1'b0; go_tx = 1'b0; go_rd = 1'b0;
    repeat (4) @(posedge clk);
    rst = 1'b0;
    @(posedge clk);

    // --- golden run ---
    ready_en = 1'b1;
    go_tx    = 1'b1;
    while (gold_n < TX_BYTES) @(posedge clk);
    go_tx    = 1'b0;
    repeat (4) @(posedge clk);

    // --- DUT run ---
    wfull_en = 1'b1;
    go_rd    = 1'b1;
    while (beat_n < TOTAL_BEATS) @(posedge clk);
    wfull_en = 1'b0;
    while (!rd_done) @(posedge clk);
    go_rd = 1'b0;
    @(posedge clk);

    // --- compare ---
    if (saw_full_write)
      begin $display("  N=%0d FAIL: wr_en asserted while the FIFO was full", N);
            errors = errors + 1; end
    if (gold_n != TX_BYTES)
      begin $display("  N=%0d FAIL: golden produced %0d bytes, want %0d",
                     N, gold_n, TX_BYTES); errors = errors + 1; end
    if (beat_n != TOTAL_BEATS)
      begin $display("  N=%0d FAIL: reader produced %0d beats, want %0d",
                     N, beat_n, TOTAL_BEATS); errors = errors + 1; end

    for (i = 0; i < TX_BYTES; i = i + 1) begin
      if (gold_b[i] !== dma_b[i]) begin
        if (errors < 8)
          $display("  N=%0d FAIL: byte %0d (word %0d, row %0d col %0d, lane %0d): serial %h, dma %h",
                   N, i, i/4, (i/4)/N, (i/4)%N, i%4, gold_b[i], dma_b[i]);
        errors = errors + 1;
      end
    end

    if (errors == 0)
      $display("  N=%0d: %0d bytes identical to the serial path (%0d beats)",
               N, TX_BYTES, TOTAL_BEATS);
    else
      $display("  N=%0d: %0d differing byte(s)", N, errors);

    tb_dma_result_reader.total_errors = tb_dma_result_reader.total_errors + errors;
    tb_dma_result_reader.finished     = tb_dma_result_reader.finished + 1;
  end

endmodule


module tb_dma_result_reader;

  integer total_errors = 0;
  integer finished     = 0;

  rdr_case #(.N(4))  c4  ();
  rdr_case #(.N(8))  c8  ();
  rdr_case #(.N(16)) c16 ();

  initial begin
    $display("== tb_dma_result_reader: DMA image vs the serial byte stream ==");
    wait (finished == 3);
    #1;
    $display("== %s: %0d error(s) ==", (total_errors == 0) ? "PASS" : "FAIL", total_errors);
    if (total_errors != 0) $fatal(1);
    $finish;
  end

  initial begin
    #10_000_000;
    $display("== FAIL: global timeout ==");
    $fatal(1);
  end

endmodule

`default_nettype wire
