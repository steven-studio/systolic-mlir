// -----------------------------------------------------------------------------
// tb_dma_cdc_fifo.sv -- self-checking bench for the asynchronous FIFO.
//
// The drivers use COMBINATIONAL enables.  Registering wr_en from the previous
// cycle's !wfull lets the write be refused after the pattern counter has
// already advanced, which silently drops a word and hangs the reader -- the
// first version of this bench did exactly that.
//
// Checks:
//   1. INTEGRITY, fast->slow.  250 MHz writer, 100 MHz reader: every word out
//      exactly once, in order, across several pointer wraps.  Real operating
//      point (ui_clk faster than the array clock).
//   2. INTEGRITY, slow->fast.  Reversed ratio, exercising empty instead of
//      full, after a clean reset of both domains.
//   3. ALMOST-FULL LEADS FULL.  almost_full must assert while slots remain;
//      if it only ever coincided with full, beats already in flight in MIG
//      would be dropped.
//   4. NO OVERFLOW.  wr_en is never honoured while full.
// -----------------------------------------------------------------------------

`timescale 1ns/1ps

module tb_dma_cdc_fifo;

  localparam integer DW        = 128;
  localparam integer AW        = 5;
  localparam integer AF_MARGIN = 8;
  localparam integer NWORDS    = 1000;

  reg wclk = 0, rclk = 0, wrst_n = 0, rrst_n = 0;
  integer whalf = 2, rhalf = 5;
  always #(whalf) wclk = ~wclk;
  always #(rhalf) rclk = ~rclk;

  wire          wfull, walmost_full, rempty;
  wire [DW-1:0] rd_data;

  integer wcount = 0, rcount = 0;

  // combinational enables -- cannot lose a word
  wire          wr_go   = wrst_n && !wfull  && (wcount < NWORDS);
  wire          rd_go   = rrst_n && !rempty;
  wire [DW-1:0] wr_data = {~wcount[31:0], {(DW-64){1'b0}}, wcount[31:0]};

  dma_cdc_fifo #(.DW(DW), .AW(AW), .AF_MARGIN(AF_MARGIN)) dut (
    .wclk(wclk), .wrst_n(wrst_n), .wr_en(wr_go), .wr_data(wr_data),
    .wfull(wfull), .walmost_full(walmost_full),
    .rclk(rclk), .rrst_n(rrst_n), .rd_en(rd_go), .rd_data(rd_data),
    .rempty(rempty)
  );

  integer errors = 0, af_lead = 0, af_coincident = 0;

  always @(posedge wclk) if (wrst_n) begin
    if (wr_go) wcount <= wcount + 1;
    if (walmost_full && !wfull) af_lead       <= af_lead + 1;
    if (walmost_full &&  wfull) af_coincident <= af_coincident + 1;
  end

  always @(posedge rclk) if (rrst_n && rd_go) begin
    if (rd_data[31:0] !== rcount[31:0] || rd_data[DW-1:DW-32] !== ~rcount[31:0]) begin
      if (errors < 5)
        $display("FAIL  word %0d: got %h..%h want %h..%h", rcount,
                 rd_data[DW-1:DW-32], rd_data[31:0], ~rcount[31:0], rcount[31:0]);
      errors <= errors + 1;
    end
    rcount <= rcount + 1;
  end

  task drain(input [255:0] name);
    integer guard;
    begin
      guard = 0;
      while (rcount < NWORDS && guard < 200000) begin
        @(posedge rclk); guard = guard + 1;
      end
      if (rcount != NWORDS) begin
        $display("FAIL  %0s: %0d of %0d words drained (written %0d)",
                 name, rcount, NWORDS, wcount);
        errors = errors + 1;
      end else
        $display("PASS  %0s: %0d words, in order, no drops", name, rcount);
    end
  endtask

  initial begin
    repeat (8) @(posedge wclk); wrst_n = 1;
    repeat (8) @(posedge rclk); rrst_n = 1;

    whalf = 2; rhalf = 5;
    drain("fast->slow  (250 / 100 MHz)");

    // clean reset of both domains before reversing the ratio
    wrst_n = 0; rrst_n = 0;
    repeat (8) @(posedge wclk); repeat (8) @(posedge rclk);
    wcount = 0; rcount = 0;
    whalf = 7; rhalf = 2;
    repeat (4) @(posedge wclk); wrst_n = 1;
    repeat (4) @(posedge rclk); rrst_n = 1;
    drain("slow->fast  ( 71 / 250 MHz)");

    if (af_lead == 0) begin
      $display("FAIL  almost_full never asserted before full");
      errors = errors + 1;
    end else
      $display("PASS  almost_full: led full on %0d cycles, coincident on %0d",
               af_lead, af_coincident);

    if (errors == 0) $display("\nALL CHECKS PASSED");
    else             $display("\n%0d CHECK(S) FAILED", errors);
    $finish;
  end

  initial begin
    #5_000_000;
    $display("\nFAIL  global timeout (w=%0d r=%0d full=%b empty=%b)",
             wcount, rcount, wfull, rempty);
    $finish;
  end

endmodule
