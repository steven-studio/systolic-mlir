`default_nettype none
`timescale 1ns / 1ps

/*
 * tb_dma_writeback_engine -- self-checking bench for the AXI4 write master.
 *
 * The slave stub is deliberately strict.  It is not enough to check that the
 * right bytes land: the bench must fail on a protocol violation that a real
 * MIG would answer with undefined behaviour rather than an error, because
 * those are exactly the bugs that survive to the board.  It therefore asserts
 * on
 *
 *   - a burst crossing a 4 KiB boundary,
 *   - a beat count that disagrees with awlen,
 *   - wlast on the wrong beat,
 *   - W data arriving with no AW outstanding.
 *
 * Run it at BURST_LEN=16 AND BURST_LEN=2.  At 16 the address channel is
 * scarce and aw_fire can never coincide with b_fire, so a credit wrongly
 * spent in that shared cycle is invisible -- the same hole tb_dma_engine has
 * on its read side, found by mutation testing there.
 *
 *   iverilog -g2012 -o tb.out -Ptb_dma_writeback_engine.BURST_LEN=16 \
 *            tb_dma_writeback_engine.sv ../dma_writeback_engine.sv && vvp tb.out
 *   iverilog -g2012 -o tb2.out -Ptb_dma_writeback_engine.BURST_LEN=2 ... && vvp tb2.out
 */

module tb_dma_writeback_engine;

  localparam integer AXI_DATA_W = 128;
  localparam integer AXI_ADDR_W = 29;
  localparam integer AXI_ID_W   = 2;
  localparam integer BEAT_W     = 16;
  parameter  integer BURST_LEN  = 16;      // overridden with -P
  localparam integer MAX_OUTSTANDING = 4;

  localparam integer BYTES_PER_BEAT = AXI_DATA_W / 8;
  localparam integer LSB            = $clog2(BYTES_PER_BEAT);

  logic clk = 1'b0;
  logic rst_n = 1'b0;
  always #5 clk = ~clk;

  // ---- DUT wiring --------------------------------------------------------
  logic                  desc_valid;
  wire                   desc_ready;
  logic [AXI_ADDR_W-1:0] desc_addr;
  logic [BEAT_W-1:0]     desc_beats;
  logic [7:0]            desc_tag;

  wire                   done_valid;
  wire [7:0]             done_tag;

  wire [AXI_ID_W-1:0]    awid;
  wire [AXI_ADDR_W-1:0]  awaddr;
  wire [7:0]             awlen;
  wire [2:0]             awsize;
  wire [1:0]             awburst;
  wire [0:0]             awlock;
  wire [3:0]             awcache;
  wire [2:0]             awprot;
  wire [3:0]             awqos;
  wire                   awvalid;
  logic                  awready;

  wire [AXI_DATA_W-1:0]   wdata;
  wire [AXI_DATA_W/8-1:0] wstrb;
  wire                    wlast;
  wire                    wvalid;
  logic                   wready;

  logic [AXI_ID_W-1:0]   bid = '0;
  logic [1:0]            bresp;
  logic                  bvalid;
  wire                   bready;

  logic                  src_valid;
  logic [AXI_DATA_W-1:0] src_data;
  wire                   src_ready;

  wire [31:0] busy_cycles, aw_stall_cycles, w_stall_cycles, src_starve_cycles;
  wire        err_align, err_resp;
  logic       stat_clear = 1'b0;

  dma_writeback_engine #(
    .AXI_DATA_W(AXI_DATA_W), .AXI_ADDR_W(AXI_ADDR_W), .AXI_ID_W(AXI_ID_W),
    .BEAT_W(BEAT_W), .BURST_LEN(BURST_LEN), .MAX_OUTSTANDING(MAX_OUTSTANDING)
  ) dut (
    .clk(clk), .rst_n(rst_n), .init_calib_complete(1'b1),
    .desc_valid(desc_valid), .desc_ready(desc_ready), .desc_addr(desc_addr),
    .desc_beats(desc_beats), .desc_tag(desc_tag),
    .done_valid(done_valid), .done_tag(done_tag),
    .m_axi_awid(awid), .m_axi_awaddr(awaddr), .m_axi_awlen(awlen),
    .m_axi_awsize(awsize), .m_axi_awburst(awburst), .m_axi_awlock(awlock),
    .m_axi_awcache(awcache), .m_axi_awprot(awprot), .m_axi_awqos(awqos),
    .m_axi_awvalid(awvalid), .m_axi_awready(awready),
    .m_axi_wdata(wdata), .m_axi_wstrb(wstrb), .m_axi_wlast(wlast),
    .m_axi_wvalid(wvalid), .m_axi_wready(wready),
    .m_axi_bid(bid), .m_axi_bresp(bresp), .m_axi_bvalid(bvalid), .m_axi_bready(bready),
    .src_valid(src_valid), .src_data(src_data), .src_ready(src_ready),
    .busy_cycles(busy_cycles), .aw_stall_cycles(aw_stall_cycles),
    .w_stall_cycles(w_stall_cycles), .src_starve_cycles(src_starve_cycles),
    .err_align(err_align), .err_resp(err_resp), .stat_clear(stat_clear)
  );

  // ---- strict AXI4 write slave ------------------------------------------
  localparam integer MEM_BEATS = 2048;
  logic [AXI_DATA_W-1:0] mem [0:MEM_BEATS-1];
  logic                  mem_written [0:MEM_BEATS-1];

  logic [AXI_ADDR_W-1:0] q_addr [0:15];
  logic [8:0]            q_len  [0:15];   // beats, 1..256
  integer                q_wr = 0, q_rd = 0;

  logic [AXI_ADDR_W-1:0] cur_addr;
  integer                cur_beats_left = 0;
  integer                cur_len_total  = 0;
  integer                b_pending      = 0;
  integer                errors         = 0;

  logic slave_err_mode = 1'b0;   // make the NEXT response SLVERR
  integer aw_stall_n = 0, w_stall_n = 0;

  task automatic fail(input string why);
    begin
      $display("  FAIL: %s  (t=%0t)", why, $time);
      errors = errors + 1;
    end
  endtask

  // awready / wready with pseudo-random stalls so the bench exercises both
  // channels' backpressure rather than the free-running happy path.
  integer lfsr = 32'h1234_5678;
  always @(posedge clk) begin
    lfsr <= {lfsr[30:0], lfsr[31] ^ lfsr[21] ^ lfsr[1] ^ lfsr[0]};
  end
  always_comb begin
    awready = (aw_stall_n == 0) ? 1'b1 : lfsr[3];
    wready  = (w_stall_n  == 0) ? 1'b1 : lfsr[7];
  end

  // AW capture, with the 4 KiB check the real slave will not do for you.
  always_ff @(posedge clk) begin
    if (rst_n && awvalid && awready) begin
      if (awburst !== 2'b01) fail("awburst is not INCR");
      if (awsize  !== 3'(LSB)) fail("awsize does not match the data width");
      if (((awaddr & 29'hFFF) + ((awlen + 1) << LSB)) > 4096)
        fail($sformatf("burst crosses a 4 KiB boundary: addr=%0h len=%0d",
                       awaddr, awlen + 1));
      q_addr[q_wr[3:0]] <= awaddr;
      q_len [q_wr[3:0]] <= 9'(awlen) + 9'd1;
      q_wr <= q_wr + 1;
    end
  end

  // W consumption against the queued AW.
  always_ff @(posedge clk) begin
    if (!rst_n) begin
      cur_beats_left <= 0;
      b_pending      <= 0;
    end else begin
      if (wvalid && wready) begin
        if (cur_beats_left == 0) begin
          if (q_rd == q_wr) begin
            fail("W beat with no AW outstanding");
          end else begin
            cur_addr       <= q_addr[q_rd[3:0]];
            cur_len_total  <= q_len [q_rd[3:0]];
            cur_beats_left <= q_len [q_rd[3:0]] - 1;
            q_rd           <= q_rd + 1;
            mem       [q_addr[q_rd[3:0]] >> LSB] <= wdata;
            mem_written[q_addr[q_rd[3:0]] >> LSB] <= 1'b1;
            if (wstrb !== {BYTES_PER_BEAT{1'b1}}) fail("wstrb is not all ones");
            if (wlast !== (q_len[q_rd[3:0]] == 1)) fail("wlast on the wrong beat");
          end
        end else begin
          cur_addr       <= cur_addr + AXI_ADDR_W'(BYTES_PER_BEAT);
          cur_beats_left <= cur_beats_left - 1;
          mem       [(cur_addr + AXI_ADDR_W'(BYTES_PER_BEAT)) >> LSB] <= wdata;
          mem_written[(cur_addr + AXI_ADDR_W'(BYTES_PER_BEAT)) >> LSB] <= 1'b1;
          if (wstrb !== {BYTES_PER_BEAT{1'b1}}) fail("wstrb is not all ones");
          if (wlast !== (cur_beats_left == 1)) fail("wlast on the wrong beat");
        end

        if (wlast) b_pending <= b_pending + 1;
      end

      if (bvalid && bready && !(wvalid && wready && wlast))
        b_pending <= b_pending - 1;
    end
  end

  // B response, one cycle after the burst completes at the earliest.
  always_ff @(posedge clk) begin
    if (!rst_n) begin
      bvalid <= 1'b0;
      bresp  <= 2'b00;
    end else begin
      if (bvalid && bready) bvalid <= 1'b0;
      if ((b_pending > 0) && !bvalid) begin
        bvalid <= 1'b1;
        bresp  <= slave_err_mode ? 2'b10 : 2'b00;
      end
    end
  end

  // ---- source ------------------------------------------------------------
  // Known pattern: beat n is {n, n, n, n} in 32-bit lanes, so a swapped or
  // dropped beat is visible in the memory image, not just in a count.
  integer src_n = 0;
  integer src_gap = 0;
  always_ff @(posedge clk) begin
    if (!rst_n) begin
      src_n <= 0;
    end else if (src_valid && src_ready) begin
      src_n <= src_n + 1;
    end
  end
  always_comb begin
    src_valid = (src_gap == 0) ? 1'b1 : lfsr[11];
    src_data  = {32'(src_n) + 32'h3000_0000, 32'(src_n) + 32'h2000_0000,
                 32'(src_n) + 32'h1000_0000, 32'(src_n)};
  end

  function automatic logic [AXI_DATA_W-1:0] expect_beat(input integer n);
    expect_beat = {32'(n) + 32'h3000_0000, 32'(n) + 32'h2000_0000,
                   32'(n) + 32'h1000_0000, 32'(n)};
  endfunction

  // ---- driving -----------------------------------------------------------
  task automatic reset_all;
    integer i;
    begin
      rst_n = 1'b0; desc_valid = 1'b0; slave_err_mode = 1'b0;
      q_wr = 0; q_rd = 0; src_n = 0;
      for (i = 0; i < MEM_BEATS; i = i + 1) mem_written[i] = 1'b0;
      repeat (4) @(posedge clk);
      rst_n = 1'b1;
      @(posedge clk);
    end
  endtask

  task automatic send_desc(input [AXI_ADDR_W-1:0] a,
                           input [BEAT_W-1:0]     n,
                           input [7:0]            t);
    logic accepted;
    begin
      // NBA 驅動 + 在時脈邊緣讀 ready:這樣 tb 看到的握手與 DUT 取樣的
      // 是同一組值。用 blocking assign 再在邊緣後讀 ready 會有 race,
      // 可能導致同一張 descriptor 被 latch 兩次。
      @(posedge clk);
      desc_addr  <= a;
      desc_beats <= n;
      desc_tag   <= t;
      desc_valid <= 1'b1;
      accepted = 1'b0;
      while (!accepted) begin
        @(posedge clk);
        if (desc_ready) begin
          desc_valid <= 1'b0;
          accepted    = 1'b1;
        end
      end
      @(posedge clk);
    end
  endtask

  task automatic wait_done(input integer limit);
    integer c;
    begin
      c = 0;
      while (!done_valid && (c < limit)) begin @(posedge clk); c = c + 1; end
      if (!done_valid) fail("no done_valid before the timeout");
    end
  endtask

  task automatic check_image(input [AXI_ADDR_W-1:0] base,
                             input integer          nbeats,
                             input integer          first_n);
    integer i;
    integer idx;
    begin
      for (i = 0; i < nbeats; i = i + 1) begin
        idx = (base >> LSB) + i;
        if (!mem_written[idx])
          fail($sformatf("beat %0d never written (idx %0d)", i, idx));
        else if (mem[idx] !== expect_beat(first_n + i))
          fail($sformatf("beat %0d: got %h want %h",
                         i, mem[idx], expect_beat(first_n + i)));
      end
    end
  endtask

  // ---- cases -------------------------------------------------------------
  integer base_n;

  initial begin
    $display("== tb_dma_writeback_engine, BURST_LEN=%0d ==", BURST_LEN);

    // 1. one result tile: 16 beats at an aligned address, no stalls
    reset_all;
    aw_stall_n = 0; w_stall_n = 0; src_gap = 0;
    base_n = src_n;
    send_desc(29'h0000_1000, 16, 8'hA1);
    wait_done(4000);
    if (done_tag !== 8'hA1) fail("done_tag does not match the descriptor");
    check_image(29'h0000_1000, 16, base_n);
    if (err_align || err_resp) fail("an error flag is set on the clean case");
    $display("  [1] 16-beat tile                      errors=%0d", errors);

    // 2. same payload with both channels stalling and the source gapping
    reset_all;
    aw_stall_n = 1; w_stall_n = 1; src_gap = 1;
    base_n = src_n;
    send_desc(29'h0000_2000, 16, 8'hA2);
    wait_done(20000);
    check_image(29'h0000_2000, 16, base_n);
    if (src_starve_cycles == 0) fail("src_starve_cycles never counted a gap");
    $display("  [2] with stalls and source gaps       errors=%0d", errors);

    // 3. longer than BURST_LEN: forces several bursts and the length reload
    reset_all;
    aw_stall_n = 1; w_stall_n = 1; src_gap = 0;
    base_n = src_n;
    send_desc(29'h0000_4000, 100, 8'hA3);
    wait_done(40000);
    check_image(29'h0000_4000, 100, base_n);
    $display("  [3] 100 beats, multi-burst            errors=%0d", errors);

    // 4. straddling a 4 KiB boundary: the slave asserts if the clamp is wrong
    reset_all;
    aw_stall_n = 0; w_stall_n = 0; src_gap = 0;
    base_n = src_n;
    send_desc(29'h0000_6FC0, 32, 8'hA4);   // 4 beats to the page end, then 28
    wait_done(40000);
    check_image(29'h0000_6FC0, 32, base_n);
    $display("  [4] across a 4 KiB boundary           errors=%0d", errors);

    // 5. misaligned descriptor: refused before anything is issued
    reset_all;
    base_n = src_n;
    send_desc(29'h0000_8004, 16, 8'hA5);
    repeat (50) @(posedge clk);
    if (!err_align) fail("a misaligned descriptor did not set err_align");
    if (q_wr != 0)  fail("a misaligned descriptor still issued an AW");
    if (done_valid) fail("a misaligned descriptor reported done");
    $display("  [5] misaligned descriptor refused     errors=%0d", errors);

    // 6. SLVERR must not pass as a successful write
    reset_all;
    slave_err_mode = 1'b1;
    send_desc(29'h0000_A000, 16, 8'hA6);
    wait_done(4000);
    if (!err_resp) fail("a SLVERR response did not set err_resp");
    $display("  [6] SLVERR flagged                    errors=%0d", errors);

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
