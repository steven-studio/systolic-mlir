`default_nettype none
`timescale 1ns / 1ps

/*
 * tb_dma_writeback_path -- the whole write-back chain, in one simulation.
 *
 *   C  ->  dma_result_reader  ->  dma_cdc_fifo  ->  dma_writeback_engine  ->  DRAM
 *          (array clock)          (crossing)        (ui_clk)                 (model)
 *
 * The unit benches prove each stage against its own contract.  This one
 * exists for the joins, which is where the read path's bugs actually were:
 * the operand path timed out on the board because a call site bypassed the
 * context struct, not because any module was wrong on its own.
 *
 * The check is the same one that matters everywhere on this path: read the
 * memory back and require it to be byte-identical to what the real
 * systolic_tx_source emits for the same C.  If the two paths ever disagree,
 * every measurement taken over one of them stops meaning anything.
 *
 * Two orderings are run, because the engine's header makes a claim about the
 * second one:
 *
 *   [1] reader first, then the descriptor -- the intended order, the payload
 *       is already in the FIFO when AW is issued and W never starves.
 *   [2] descriptor first, then the reader -- the engine is committed to a
 *       burst before any data exists, so W stalls mid-burst.  The header
 *       says that is legal and merely parks the memory controller; this
 *       proves the image is still correct, and that src_starve_cycles counts
 *       it rather than the starvation passing silently.
 *
 * The two clocks are deliberately unrelated (9 ns and 10 ns) so the crossing
 * is exercised rather than being an integer ratio that happens to work.
 *
 *   iverilog -g2012 -o tb.out tb_dma_writeback_path.sv \
 *       ../dma_result_reader.sv ../dma_cdc_fifo.sv ../dma_writeback_engine.sv \
 *       ../../core/tx_source.sv && vvp tb.out
 */

module tb_dma_writeback_path;

  localparam int     N          = 8;
  localparam integer AXI_DATA_W = 128;
  localparam integer AXI_ADDR_W = 29;
  localparam integer AXI_ID_W   = 2;
  localparam integer BEAT_W     = 16;
  localparam integer BURST_LEN  = 16;
  localparam int     TX_BYTES   = 4 * N * N;                     // 256
  localparam int     BEATS      = (N * N) / (AXI_DATA_W / 32);   // 16
  localparam integer LSB        = $clog2(AXI_DATA_W / 8);

  // ---- two unrelated clocks ---------------------------------------------
  logic aclk = 1'b0;    // array
  logic uclk = 1'b0;    // memory-controller user clock
  always #4.5 aclk = ~aclk;    // ~111 MHz
  always #5.0 uclk = ~uclk;    // 100 MHz

  logic arst   = 1'b1;         // active high, as tx_source and the reader want
  logic urst_n = 1'b0;

  logic [31:0] C [0:N-1][0:N-1];

  // ---- golden: the real serial source -----------------------------------
  logic      go_tx;
  wire       m_valid;
  wire [7:0] m_data;
  logic      m_ready = 1'b1;

  systolic_tx_source #(.DEBUG_MARKERS(1'b0), .CYCLE_COUNTER(1'b0), .N(N)) golden (
    .clk(aclk), .rst(arst),
    .send_go(go_tx), .all_done(),
    .debug_pending(5'b0), .debug_accept(),
    .C(C), .cyc_latched(32'h0),
    .m_valid(m_valid), .m_data(m_data), .m_ready(m_ready)
  );

  logic [7:0] gold_b [0:TX_BYTES-1];
  integer     gold_n = 0;
  always_ff @(posedge aclk) if (!arst && m_valid && m_ready) begin
    gold_b[gold_n] <= m_data;
    gold_n         <= gold_n + 1;
  end

  // ---- stage 1: reader ---------------------------------------------------
  logic                  rd_start;
  wire                   rd_done;
  wire                   f_wr_en;
  wire [AXI_DATA_W-1:0]  f_wr_data;
  wire                   f_wfull, f_walmost_full;

  dma_result_reader #(.N(N), .AXI_DATA_W(AXI_DATA_W)) reader (
    .clk(aclk), .rst(arst),
    .start(rd_start), .done(rd_done),
    .C(C),
    .wr_en(f_wr_en), .wr_data(f_wr_data), .wfull(f_wfull)
  );

  // ---- stage 2: the crossing --------------------------------------------
  // Depth 32 holds a whole 16-beat tile, which is what makes ordering [1]
  // starvation-free.  wclk is the array, rclk the controller: the mirror of
  // how the operand path instantiates it.
  wire                  f_rd_en;
  wire [AXI_DATA_W-1:0] f_rd_data;
  wire                  f_rempty;

  dma_cdc_fifo #(.DW(AXI_DATA_W), .AW(5), .AF_MARGIN(8)) fifo (
    .wclk(aclk), .wrst_n(~arst), .wr_en(f_wr_en), .wr_data(f_wr_data),
    .wfull(f_wfull), .walmost_full(f_walmost_full),
    .rclk(uclk), .rrst_n(urst_n), .rd_en(f_rd_en),
    .rd_data(f_rd_data), .rempty(f_rempty)
  );

  // The FIFO is first-word-fall-through, so its read side IS a valid/ready
  // stream: rd_data is already the head. This is the wiring the top level
  // should use.
  wire src_valid = ~f_rempty;
  wire src_ready;
  assign f_rd_en = src_valid & src_ready;

  // ---- stage 3: the engine ----------------------------------------------
  logic                  desc_valid;
  wire                   desc_ready;
  logic [AXI_ADDR_W-1:0] desc_addr;
  logic [BEAT_W-1:0]     desc_beats;
  logic [7:0]            desc_tag;
  wire                   done_valid;
  wire [7:0]             done_tag;

  wire [AXI_ID_W-1:0]   awid;
  wire [AXI_ADDR_W-1:0] awaddr;
  wire [7:0]            awlen;
  wire [2:0]            awsize;
  wire [1:0]            awburst;
  wire [0:0]            awlock;
  wire [3:0]            awcache;
  wire [2:0]            awprot;
  wire [3:0]            awqos;
  wire                  awvalid;
  logic                 awready;

  wire [AXI_DATA_W-1:0]   wdata;
  wire [AXI_DATA_W/8-1:0] wstrb;
  wire                    wlast, wvalid;
  logic                   wready;

  logic [1:0] bresp;
  logic       bvalid;
  wire        bready;

  wire [31:0] busy_cycles, aw_stall_cycles, w_stall_cycles, src_starve_cycles;
  wire        err_align, err_resp;

  dma_writeback_engine #(
    .AXI_DATA_W(AXI_DATA_W), .AXI_ADDR_W(AXI_ADDR_W), .AXI_ID_W(AXI_ID_W),
    .BEAT_W(BEAT_W), .BURST_LEN(BURST_LEN), .MAX_OUTSTANDING(4)
  ) engine (
    .clk(uclk), .rst_n(urst_n), .init_calib_complete(1'b1),
    .desc_valid(desc_valid), .desc_ready(desc_ready), .desc_addr(desc_addr),
    .desc_beats(desc_beats), .desc_tag(desc_tag),
    .done_valid(done_valid), .done_tag(done_tag),
    .m_axi_awid(awid), .m_axi_awaddr(awaddr), .m_axi_awlen(awlen),
    .m_axi_awsize(awsize), .m_axi_awburst(awburst), .m_axi_awlock(awlock),
    .m_axi_awcache(awcache), .m_axi_awprot(awprot), .m_axi_awqos(awqos),
    .m_axi_awvalid(awvalid), .m_axi_awready(awready),
    .m_axi_wdata(wdata), .m_axi_wstrb(wstrb), .m_axi_wlast(wlast),
    .m_axi_wvalid(wvalid), .m_axi_wready(wready),
    .m_axi_bid('0), .m_axi_bresp(bresp), .m_axi_bvalid(bvalid), .m_axi_bready(bready),
    .src_valid(src_valid), .src_data(f_rd_data), .src_ready(src_ready),
    .busy_cycles(busy_cycles), .aw_stall_cycles(aw_stall_cycles),
    .w_stall_cycles(w_stall_cycles), .src_starve_cycles(src_starve_cycles),
    .err_align(err_align), .err_resp(err_resp), .stat_clear(1'b0)
  );

  // ---- stage 4: behavioural DRAM ----------------------------------------
  localparam integer MEM_BEATS = 1024;
  logic [AXI_DATA_W-1:0] mem [0:MEM_BEATS-1];
  logic                  mem_written [0:MEM_BEATS-1];

  integer errors = 0;
  task automatic fail(input string why);
    begin $display("  FAIL: %s (t=%0t)", why, $time); errors = errors + 1; end
  endtask

  integer lfsr = 32'h5EED_1234;
  always_ff @(posedge uclk) lfsr <= {lfsr[30:0], lfsr[31]^lfsr[21]^lfsr[1]^lfsr[0]};
  always_comb begin
    awready = lfsr[2] | lfsr[6];
    wready  = lfsr[4] | lfsr[8];
  end

  logic [AXI_ADDR_W-1:0] burst_addr;
  integer                burst_left = 0;
  integer                b_pending  = 0;

  always_ff @(posedge uclk) begin
    if (!urst_n) begin
      burst_left <= 0;
      b_pending  <= 0;
    end else begin
      if (awvalid && awready) begin
        if (((awaddr & 29'hFFF) + ((awlen + 1) << LSB)) > 4096)
          fail("burst crosses a 4 KiB boundary");
        burst_addr <= awaddr;
        burst_left <= awlen + 1;
      end
      if (wvalid && wready) begin
        if (burst_left == 0) fail("W beat with no AW outstanding");
        if ((burst_addr >> LSB) >= MEM_BEATS)
          fail("write lands outside the model memory -- widen MEM_BEATS or move the base");
        if (wstrb !== {(AXI_DATA_W/8){1'b1}}) fail("wstrb is not all ones");
        if (wlast !== (burst_left == 1)) fail("wlast on the wrong beat");
        mem        [burst_addr >> LSB] <= wdata;
        mem_written[burst_addr >> LSB] <= 1'b1;
        burst_addr <= burst_addr + AXI_ADDR_W'(AXI_DATA_W/8);
        burst_left <= burst_left - 1;
        if (wlast) b_pending <= b_pending + 1;
      end
      if (bvalid && bready && !(wvalid && wready && wlast))
        b_pending <= b_pending - 1;
    end
  end

  always_ff @(posedge uclk) begin
    if (!urst_n) bvalid <= 1'b0;
    else begin
      if (bvalid && bready) bvalid <= 1'b0;
      if ((b_pending > 0) && !bvalid) begin bvalid <= 1'b1; bresp <= 2'b00; end
    end
  end

  // ---- checking ----------------------------------------------------------
  task automatic check_image(input [AXI_ADDR_W-1:0] base, input string what);
    integer i, idx, shown;
    logic [7:0] got;
    begin
      shown = 0;
      for (i = 0; i < TX_BYTES; i = i + 1) begin
        idx = (base >> LSB) + (i / (AXI_DATA_W/8));
        if (idx >= MEM_BEATS) begin
          if (shown < 4) $display("  FAIL[%s]: base is outside the model memory", what);
          shown = shown + 1; errors = errors + 1;
        end
        got = mem[idx][8*(i % (AXI_DATA_W/8)) +: 8];
        if (!mem_written[idx]) begin
          if (shown < 4) $display("  FAIL[%s]: beat %0d never written", what, i/16);
          shown = shown + 1; errors = errors + 1;
        end else if (got !== gold_b[i]) begin
          if (shown < 4)
            $display("  FAIL[%s]: byte %0d (word %0d, row %0d col %0d): serial %h, dram %h",
                     what, i, i/4, (i/4)/N, (i/4)%N, gold_b[i], got);
          shown = shown + 1; errors = errors + 1;
        end
      end
    end
  endtask

  task automatic send_desc(input [AXI_ADDR_W-1:0] a, input [7:0] t);
    logic accepted;
    begin
      @(posedge uclk);
      desc_addr <= a; desc_beats <= BEAT_W'(BEATS); desc_tag <= t; desc_valid <= 1'b1;
      accepted = 1'b0;
      while (!accepted) begin
        @(posedge uclk);
        if (desc_ready) begin desc_valid <= 1'b0; accepted = 1'b1; end
      end
    end
  endtask

  task automatic wait_done(input integer limit);
    integer c;
    begin
      c = 0;
      while (!done_valid && c < limit) begin @(posedge uclk); c = c + 1; end
      if (!done_valid) fail("engine never reported done");
    end
  endtask

  integer r, c_, starve_1, starve_2;

  initial begin
    $display("== tb_dma_writeback_path: C -> reader -> CDC -> engine -> DRAM ==");

    for (r = 0; r < N; r = r + 1)
      for (c_ = 0; c_ < N; c_ = c_ + 1)
        C[r][c_] = {8'hC0, 8'(r), 8'(c_), 8'(r*N + c_)};

    desc_valid = 1'b0; rd_start = 1'b0; go_tx = 1'b0;
    for (r = 0; r < MEM_BEATS; r = r + 1) mem_written[r] = 1'b0;

    repeat (6) @(posedge uclk);
    arst = 1'b0; urst_n = 1'b1;
    repeat (4) @(posedge uclk);

    // golden byte stream first, on the array clock
    go_tx = 1'b1;
    while (gold_n < TX_BYTES) @(posedge aclk);
    go_tx = 1'b0;
    repeat (4) @(posedge aclk);

    // ---- [1] reader first, then the descriptor -------------------------
    rd_start = 1'b1;
    while (!rd_done) @(posedge aclk);
    rd_start = 1'b0;
    starve_1 = src_starve_cycles;
    send_desc(29'h0000_1000, 8'hE1);
    wait_done(20000);
    if (done_tag !== 8'hE1) fail("done_tag does not match");
    check_image(29'h0000_1000, "reader first");
    if (src_starve_cycles != starve_1)
      fail("W starved even though the tile was already in the FIFO");
    $display("  [1] reader first, then descriptor      errors=%0d", errors);

    // ---- [2] descriptor first, then the reader -------------------------
    starve_2 = src_starve_cycles;
    fork
      send_desc(29'h0000_2000, 8'hE2);
      begin
        repeat (40) @(posedge aclk);      // engine is committed and waiting
        rd_start = 1'b1;
        while (!rd_done) @(posedge aclk);
        rd_start = 1'b0;
      end
    join
    wait_done(20000);
    check_image(29'h0000_2000, "descriptor first");
    if (src_starve_cycles <= starve_2)
      fail("W did not starve, so the ordering was not actually exercised");
    $display("  [2] descriptor first, W starves        errors=%0d  (starve %0d cyc)",
             errors, src_starve_cycles - starve_2);

    if (err_align) fail("err_align set");
    if (err_resp)  fail("err_resp set");

    $display("== %s: %0d error(s) ==", (errors == 0) ? "PASS" : "FAIL", errors);
    if (errors != 0) $fatal(1);
    $finish;
  end

  initial begin
    #5_000_000;
    $display("== FAIL: global timeout ==");
    $fatal(1);
  end

endmodule

`default_nettype wire
