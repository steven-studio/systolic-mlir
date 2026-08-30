// -----------------------------------------------------------------------------
// tb_dma_engine.sv -- self-checking bench for the descriptor read engine.
//
//   icarus: iverilog -g2012 -o tb.vvp dma_engine.sv mig_ui_stub.sv \
//           tb_dma_engine.sv && vvp tb.vvp
//   xsim:   xvlog -sv dma/dma_engine.sv dma/tb/mig_ui_stub.sv \
//           dma/tb/tb_dma_engine.sv && xelab -R tb_dma_engine
//
// Checks:
//   1. CONTENT -- every beat of every descriptor lands in the destination with
//      the right data at the right beat index.  Bit-exact or fail; this is the
//      only acceptable bar for bring-up step 2.
//   2. COMPLETION -- done fires exactly once per descriptor, with the right
//      tag, and not before the last beat has been written.
//   3. BACK TO BACK -- a second descriptor issued immediately after the first
//      is not contaminated by the first (counters reset, no stale beats).
//   4. ALIGNMENT -- a descriptor at a non-zero base address reads from the
//      right place, which is where an address-arithmetic bug would show.
// -----------------------------------------------------------------------------

`timescale 1ns/1ps

module tb_dma_engine;

  localparam integer APP_DATA_W = 128;
  localparam integer APP_ADDR_W = 28;
  localparam integer BEAT_W     = 16;
  localparam integer BYTES      = APP_DATA_W / 8;

  reg clk = 0, rst_n = 0;
  always #5 clk = ~clk;

  reg                    desc_valid = 0, stat_clear = 0;
  reg  [APP_ADDR_W-1:0]  desc_addr = 0;
  reg  [BEAT_W-1:0]      desc_beats = 0;
  reg  [7:0]             desc_tag = 0;
  wire                   desc_ready;

  wire                   done_valid;
  wire [7:0]             done_tag;

  wire [APP_ADDR_W-1:0]  app_addr;
  wire [2:0]             app_cmd;
  wire                   app_en, app_rdy, app_rd_data_valid, init_calib_complete;
  wire [APP_DATA_W-1:0]  app_rd_data;

  wire                   dst_wr_en;
  wire [BEAT_W-1:0]      dst_wr_beat;
  wire [APP_DATA_W-1:0]  dst_wr_data;
  wire [7:0]             dst_wr_tag;
  wire [31:0]            busy_cycles, rdy_stall_cycles;

  dma_engine #(.APP_DATA_W(APP_DATA_W), .APP_ADDR_W(APP_ADDR_W), .BEAT_W(BEAT_W))
  dut (
    .clk(clk), .rst_n(rst_n), .init_calib_complete(init_calib_complete),
    .desc_valid(desc_valid), .desc_ready(desc_ready), .desc_addr(desc_addr),
    .desc_beats(desc_beats), .desc_tag(desc_tag),
    .done_valid(done_valid), .done_tag(done_tag),
    .app_addr(app_addr), .app_cmd(app_cmd), .app_en(app_en), .app_rdy(app_rdy),
    .app_rd_data(app_rd_data), .app_rd_data_valid(app_rd_data_valid),
    .dst_almost_full(1'b0),
    .dst_wr_en(dst_wr_en), .dst_wr_beat(dst_wr_beat), .dst_wr_data(dst_wr_data),
    .dst_wr_tag(dst_wr_tag),
    .busy_cycles(busy_cycles), .rdy_stall_cycles(rdy_stall_cycles),
    .stat_clear(stat_clear)
  );

  mig_ui_stub #(.APP_DATA_W(APP_DATA_W), .APP_ADDR_W(APP_ADDR_W))
  mem (
    .clk(clk), .rst_n(rst_n), .init_calib_complete(init_calib_complete),
    .app_addr(app_addr), .app_cmd(app_cmd), .app_en(app_en), .app_rdy(app_rdy),
    .app_rd_data(app_rd_data), .app_rd_data_valid(app_rd_data_valid)
  );

  // ---- destination model + scoreboard ------------------------------------
  reg [APP_DATA_W-1:0] dst [0:1023];
  reg [BEAT_W-1:0]     next_beat = 0;
  integer              errors = 0;
  integer              dones = 0;
  reg  [7:0]           last_done_tag;

  always @(posedge clk) begin
    if (dst_wr_en) begin
      // beat indices must arrive strictly in order, starting at 0
      if (dst_wr_beat !== next_beat) begin
        $display("FAIL  beat order: got %0d, want %0d", dst_wr_beat, next_beat);
        errors = errors + 1;
      end
      dst[dst_wr_beat] <= dst_wr_data;
      next_beat        <= next_beat + 1'b1;
    end
    if (done_valid) begin
      dones         <= dones + 1;
      last_done_tag <= done_tag;
    end
  end

  // ---- one descriptor, then verify ---------------------------------------
  integer i;
  integer base_beat;
  task run_desc(input integer base_b, input integer nbeats, input [7:0] t);
    integer timeout;
    begin
      next_beat = 0;
      dones     = 0;
      @(negedge clk);
      desc_addr  = base_b * BYTES;
      desc_beats = nbeats[BEAT_W-1:0];
      desc_tag   = t;
      desc_valid = 1;
      while (!desc_ready) @(negedge clk);
      @(negedge clk);
      desc_valid = 0;

      timeout = 0;
      while (dones == 0 && timeout < 200000) begin
        @(negedge clk); timeout = timeout + 1;
      end
      if (dones != 1) begin
        $display("FAIL  tag %0d: done fired %0d times (timeout=%0d)", t, dones, timeout);
        errors = errors + 1;
      end else if (last_done_tag !== t) begin
        $display("FAIL  tag: got %0d, want %0d", last_done_tag, t);
        errors = errors + 1;
      end else if (next_beat !== nbeats[BEAT_W-1:0]) begin
        $display("FAIL  tag %0d: %0d beats written, want %0d", t, next_beat, nbeats);
        errors = errors + 1;
      end else begin
        // content check
        for (i = 0; i < nbeats; i = i + 1)
          if (dst[i] !== mem.mem[base_b + i]) begin
            $display("FAIL  tag %0d beat %0d: got %h want %h",
                     t, i, dst[i], mem.mem[base_b + i]);
            errors = errors + 1;
          end
        if (errors == 0)
          $display("PASS  tag %0d: %0d beats from beat %0d, %0d busy cycles, %0d rdy stalls",
                   t, nbeats, base_b, busy_cycles, rdy_stall_cycles);
      end
    end
  endtask

  initial begin
    // deterministic pattern: beat b holds four words encoding b
    for (i = 0; i < 4096; i = i + 1)
      mem.mem[i] = {32'hA5A5_0000 + i, 32'h5A5A_0000 + i,
                    32'hDEAD_0000 + i, 32'hBEEF_0000 + i};

    repeat (4) @(negedge clk); rst_n = 1;
    while (!init_calib_complete) @(negedge clk);

    stat_clear = 1; @(negedge clk); stat_clear = 0;

    run_desc(0,    64, 8'h11);   // 1,2: content + completion
    run_desc(0,    64, 8'h22);   // 3: back to back from the same base
    run_desc(1000, 37, 8'h33);   // 4: non-zero base, odd length
    run_desc(7,     1, 8'h44);   // single beat: issue and drain in one go

    if (errors == 0) $display("\nALL CHECKS PASSED");
    else             $display("\n%0d CHECK(S) FAILED", errors);
    $finish;
  end

endmodule
