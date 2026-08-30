// -----------------------------------------------------------------------------
// dma_cdc_fifo.sv -- asynchronous FIFO between the MIG user-interface clock and
// the array clock.
//
// WHY IT IS ASYNCHRONOUS AND NOT A HANDSHAKE
//   ui_clk runs at 200 MHz (4:1 ratio on a 16-bit DDR3) and the array at
//   100 MHz, so the read side consumes at half the rate the write side can
//   deliver.  A two-way handshake would throw that headroom away; a FIFO keeps
//   it and turns it into slack that hides DDR3 refresh and row-miss bubbles.
//
// WHY GRAY POINTERS
//   The pointers are the only signals that cross domains.  A binary counter
//   changes many bits at once, and a sampling edge in the middle of that
//   change yields a pointer value that was never real -- which shows up as
//   data corruption once a month on hardware and never in simulation.  Gray
//   code changes one bit per increment, so a mid-flight sample is either the
//   old value or the new one.  Two-flop synchronisers on each side.
//
// ALMOST-FULL IS NOT OPTIONAL
//   The write side must stop issuing *before* the FIFO is full, because MIG
//   has commands in flight whose data cannot be refused when it returns.
//   AF_MARGIN must therefore be at least the maximum number of outstanding
//   read commands the engine allows.  Sizing it smaller drops beats silently.
//
// FULL AND EMPTY ARE REGISTERED
//   They must be.  The next write pointer depends on (wr_en & ~wfull), so a
//   combinational wfull closes a loop with any combinational producer -- which
//   is the normal case, and which a simulator resolves by spinning forever.
//   Registering them also keeps these flags off the critical path, where a
//   wide comparator across a 128-bit-deep FIFO does not belong.
// -----------------------------------------------------------------------------

`default_nettype none

module dma_cdc_fifo #(
  parameter integer DW        = 128,   // data width
  parameter integer AW        = 5,     // depth = 2**AW
  parameter integer AF_MARGIN = 8      // assert almost_full with this many slots left
) (
  // ---- write domain (ui_clk) --------------------------------------------
  input  wire            wclk,
  input  wire            wrst_n,
  input  wire            wr_en,
  input  wire [DW-1:0]   wr_data,
  output wire            wfull,
  output wire            walmost_full,

  // ---- read domain (array clk) ------------------------------------------
  input  wire            rclk,
  input  wire            rrst_n,
  input  wire            rd_en,
  output wire [DW-1:0]   rd_data,
  output wire            rempty
);

  localparam integer DEPTH = 1 << AW;

  // ---- storage: simple dual port, written in wclk, read in rclk ----------
  reg [DW-1:0] mem [0:DEPTH-1];

  // ---- pointers ----------------------------------------------------------
  reg  [AW:0] wbin, wgray, rbin, rgray;
  wire [AW:0] wbin_next  = wbin + (wr_en & ~wfull);
  wire [AW:0] wgray_next = (wbin_next >> 1) ^ wbin_next;
  wire [AW:0] rbin_next  = rbin + (rd_en & ~rempty);
  wire [AW:0] rgray_next = (rbin_next >> 1) ^ rbin_next;

  // ---- two-flop synchronisers -------------------------------------------
  // ASYNC_REG keeps each pair placed in adjacent slices so the first flop has
  // the whole clock period to settle out of metastability.
  //
  // WHAT SIMULATION CANNOT CHECK: deleting the second flop leaves this bench
  // -- and any event-driven bench -- entirely green, because a simulator has
  // no metastability to resolve.  The guards for that are synthesis-side:
  // these attributes, an ASYNC_REG-aware CDC report (report_cdc), and a
  // set_max_delay -datapath_only constraint on the pointer paths.  Verified by
  // mutation: removing a flop is invisible here.
  (* ASYNC_REG = "TRUE" *) reg [AW:0] wq1_rgray, wq2_rgray;  // rd ptr in wclk
  (* ASYNC_REG = "TRUE" *) reg [AW:0] rq1_wgray, rq2_wgray;  // wr ptr in rclk

  always @(posedge wclk or negedge wrst_n)
    if (!wrst_n) {wq2_rgray, wq1_rgray} <= '0;
    else         {wq2_rgray, wq1_rgray} <= {wq1_rgray, rgray};

  always @(posedge rclk or negedge rrst_n)
    if (!rrst_n) {rq2_wgray, rq1_wgray} <= '0;
    else         {rq2_wgray, rq1_wgray} <= {rq1_wgray, wgray};

  // ---- write side --------------------------------------------------------
  reg wfull_r, walmost_full_r;
  assign wfull        = wfull_r;
  assign walmost_full = walmost_full_r;

  always @(posedge wclk or negedge wrst_n)
    if (!wrst_n) begin wbin <= '0; wgray <= '0; end
    else         begin wbin <= wbin_next; wgray <= wgray_next; end

  always @(posedge wclk)
    if (wr_en && !wfull) mem[wbin[AW-1:0]] <= wr_data;

  // Full: next write pointer equals the synchronised read pointer with the
  // top two bits inverted -- the Gray-code way of saying "one lap ahead".
  wire wfull_val = (wgray_next == {~wq2_rgray[AW:AW-1], wq2_rgray[AW-2:0]});

  always @(posedge wclk or negedge wrst_n)
    if (!wrst_n) wfull_r <= 1'b0;
    else         wfull_r <= wfull_val;

  // Occupancy in the write domain needs the read pointer back in binary.
  function automatic [AW:0] gray2bin(input [AW:0] g);
    integer i;
    begin
      gray2bin[AW] = g[AW];
      for (i = AW-1; i >= 0; i = i - 1)
        gray2bin[i] = gray2bin[i+1] ^ g[i];
    end
  endfunction

  wire [AW:0] wq2_rbin  = gray2bin(wq2_rgray);
  wire [AW:0] occupancy = wbin_next - wq2_rbin;  // conservative: read ptr is stale

  always @(posedge wclk or negedge wrst_n)
    if (!wrst_n) walmost_full_r <= 1'b0;
    else         walmost_full_r <= (occupancy >= (DEPTH - AF_MARGIN));

  // ---- read side ---------------------------------------------------------
  reg rempty_r;
  assign rempty = rempty_r;

  always @(posedge rclk or negedge rrst_n)
    if (!rrst_n) begin rbin <= '0; rgray <= '0; end
    else         begin rbin <= rbin_next; rgray <= rgray_next; end

  wire rempty_val = (rgray_next == rq2_wgray);

  always @(posedge rclk or negedge rrst_n)
    if (!rrst_n) rempty_r <= 1'b1;
    else         rempty_r <= rempty_val;

  assign rd_data = mem[rbin[AW-1:0]];        // first-word-fall-through

endmodule

`default_nettype wire
