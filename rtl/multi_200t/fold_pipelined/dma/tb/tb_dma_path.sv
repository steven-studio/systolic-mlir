// -----------------------------------------------------------------------------
// tb_dma_path.sv -- the whole read path, end to end, before it goes near a board.
//
//     behavioural DRAM -> AXI4 slave stub -> dma_engine -> dma_operand_writer
//                      -> the REAL systolic_operand_buffer instances
//
// and then the buffers are READ BACK through their own read ports and compared,
// entry by entry, against what the UART receive path would have put there.
//
// This is bring-up step 3a done in simulation.  The two unit benches check the
// engine and the writer in isolation; this one checks the thing that actually
// matters -- that after a descriptor completes, the operand memories hold
// exactly the image the UART path produces from the same payload.  Nothing
// short of reading the memories proves that: a decode can be self-consistent
// on the write stream and still land in the wrong bank.
//
// TWO BASE ADDRESSES.  The same payload is transferred from an aligned address
// and from one four beats below a 4 KiB boundary, so the bursts split
// differently.  The resulting operand image must be identical: where the data
// sat in DRAM is not allowed to change what the array sees.  That is the check
// that would catch a burst-splitting bug which the unit benches, working from
// beat indices rather than memory contents, cannot see.
//
// BUILD (from dma/):
//   iverilog -g2012 -o path.out tb/tb_dma_path.sv \
//            dma_engine.sv dma_operand_writer.sv ../core/operand_buffer.sv
//   vvp path.out
//
// Geometry is overridable, and N = 8 is the degenerate case where the A and B
// lane/koff slices coincide -- run N=4 too:
//   iverilog -g2012 -Ptb_dma_path.N=4 -o path4.out ...
// -----------------------------------------------------------------------------

`timescale 1ns/1ps
`default_nettype none

module tb_dma_path #(
  parameter int N     = 8,
  parameter int K_MAX = 64
);

  localparam int DW       = 128;
  localparam int AW       = 29;
  localparam int BL       = 16;
  localparam int MAXO     = 8;
  localparam int BPB      = DW/8;
  localparam int LANE_W   = $clog2(N);
  localparam int K_W      = $clog2(K_MAX);
  localparam int CHUNK_W  = 5 + LANE_W;
  localparam int RX_BYTES = K_MAX * 8 * N;
  localparam int RX_WORDS = RX_BYTES / 4;
  localparam int N_BEATS  = RX_WORDS / 4;

  logic clk = 0, rst_n = 0;
  always #5 clk = ~clk;

  int errors = 0;
  task fail(input string m);
    begin errors++; if (errors < 15) $display("    FAIL: %s", m); end
  endtask

  // ---- behavioural DRAM --------------------------------------------------
  // Word w of the payload holds the value w, so a word that lands in the wrong
  // place names its own origin.  load_payload writes it directly; the seed
  // phase below writes the SAME image through dma_seed_writer over AXI, so the
  // two agree only if the write master is correct.
  localparam int MEM_WORDS = 65536;
  logic [31:0] dram [0:MEM_WORDS-1];

  task automatic wipe();
    begin for (int i = 0; i < MEM_WORDS; i++) dram[i] = 32'hDEAD_0000 + i; end
  endtask

  task automatic load_payload(input int base_byte);
    int i;
    begin
      wipe();
      for (i = 0; i < RX_WORDS; i++)  dram[(base_byte/4) + i] = i;
    end
  endtask

  // ---- golden operand image, from the UART rx_count decode ---------------
  logic [31:0] gold_a [0:N-1][0:K_MAX-1];
  logic [31:0] gold_b [0:N-1][0:K_MAX-1];
  logic        seen_a [0:N-1][0:K_MAX-1];
  logic        seen_b [0:N-1][0:K_MAX-1];

  task automatic build_golden();
    int rx_count, rx_mat, is_b, win, a_lane, a_koff, b_koff, b_lane, wrd;
    begin
      for (int bk = 0; bk < N; bk++)
        for (int kk = 0; kk < K_MAX; kk++) begin
          gold_a[bk][kk] = 32'hX; gold_b[bk][kk] = 32'hX;
          seen_a[bk][kk] = 0;     seen_b[bk][kk] = 0;
        end
      for (rx_count = 0; rx_count < RX_BYTES; rx_count++) begin
        if ((rx_count % 4) == 3) begin
          rx_mat = rx_count >> CHUNK_W;
          is_b   = rx_mat & 1;
          win    = rx_mat >> 1;
          a_lane = (rx_count >> 5) & (N - 1);
          a_koff = (rx_count >> 2) & 7;
          b_koff = (rx_count >> (CHUNK_W - 3)) & 7;
          b_lane = (rx_count >> 2) & (N - 1);
          wrd    = rx_count >> 2;
          if (is_b) begin
            gold_b[b_lane][(win << 3) | b_koff] = 32'(wrd);
            seen_b[b_lane][(win << 3) | b_koff] = 1;
          end else begin
            gold_a[a_lane][(win << 3) | a_koff] = 32'(wrd);
            seen_a[a_lane][(win << 3) | a_koff] = 1;
          end
        end
      end
    end
  endtask

  // ---- DUT wiring --------------------------------------------------------
  logic          desc_valid = 0;
  wire           desc_ready;
  logic [AW-1:0] desc_addr  = 0;
  logic [15:0]   desc_beats = 0;
  wire           done_valid;

  wire [AW-1:0]  araddr;  wire [7:0] arlen;  wire arvalid;  logic arready = 1;
  logic [DW-1:0] rdata = 0;  logic rlast = 0, rvalid = 0;  wire rready;

  wire           dst_wr_en;  wire [15:0] dst_wr_beat;  wire [DW-1:0] dst_wr_data;
  wire           dst_full, dst_almost_full;
  wire           a_wr, b_wr;
  wire [LANE_W-1:0] wsel;  wire [K_W-1:0] waddr;  wire [31:0] wdata;
  wire           err_align, err_resp, err_range;

  dma_engine #(
    .AXI_DATA_W(DW), .AXI_ADDR_W(AW), .AXI_ID_W(2),
    .BEAT_W(16), .BURST_LEN(BL), .MAX_OUTSTANDING(MAXO)
  ) u_eng (
    .clk(clk), .rst_n(rst_n), .init_calib_complete(1'b1),
    .desc_valid(desc_valid), .desc_ready(desc_ready), .desc_addr(desc_addr),
    .desc_beats(desc_beats), .desc_tag(8'h5A),
    .done_valid(done_valid), .done_tag(),
    .m_axi_arid(), .m_axi_araddr(araddr), .m_axi_arlen(arlen), .m_axi_arsize(),
    .m_axi_arburst(), .m_axi_arlock(), .m_axi_arcache(), .m_axi_arprot(),
    .m_axi_arqos(), .m_axi_arvalid(arvalid), .m_axi_arready(arready),
    .m_axi_rid(2'd0), .m_axi_rdata(rdata), .m_axi_rresp(2'b00),
    .m_axi_rlast(rlast), .m_axi_rvalid(rvalid), .m_axi_rready(rready),
    .dst_almost_full(dst_almost_full), .dst_full(dst_full),
    .dst_wr_en(dst_wr_en), .dst_wr_beat(dst_wr_beat), .dst_wr_data(dst_wr_data),
    .dst_wr_tag(), .busy_cycles(), .rdy_stall_cycles(), .r_stall_cycles(),
    .err_align(err_align), .err_resp(err_resp), .stat_clear(1'b0)
  );

  dma_operand_writer #(
    .N(N), .K_MAX(K_MAX), .AXI_DATA_W(DW), .BEAT_W(16)
  ) u_wr (
    .clk(clk), .rst_n(rst_n),
    .dst_wr_en(dst_wr_en), .dst_wr_beat(dst_wr_beat), .dst_wr_data(dst_wr_data),
    .dst_full(dst_full), .dst_almost_full(dst_almost_full),
    .a_wr(a_wr), .b_wr(b_wr), .wsel(wsel), .waddr(waddr), .wdata(wdata),
    .words_written(), .err_range(err_range), .clear(1'b0)
  );

  logic [K_W-1:0] a_raddr [0:N-1];
  logic [K_W-1:0] b_raddr [0:N-1];
  wire  [31:0]    a_rdata [0:N-1];
  wire  [31:0]    b_rdata [0:N-1];

  systolic_operand_buffer #(.K_MAX(K_MAX), .K_W(K_W), .N_BANKS(N)) u_a_buf (
    .clk(clk), .wr(a_wr), .wsel(wsel), .waddr(waddr), .wdata(wdata),
    .raddr(a_raddr), .rdata(a_rdata));

  systolic_operand_buffer #(.K_MAX(K_MAX), .K_W(K_W), .N_BANKS(N)) u_b_buf (
    .clk(clk), .wr(b_wr), .wsel(wsel), .waddr(waddr), .wdata(wdata),
    .raddr(b_raddr), .rdata(b_rdata));

  // ---- AXI slave stub over the behavioural DRAM --------------------------
  int q_addr[$], q_len[$];
  int cur_addr, cur_left;

  function automatic logic [DW-1:0] dram_beat(input int byte_addr);
    int wi;
    begin
      wi = byte_addr / 4;
      dram_beat = { dram[wi+3], dram[wi+2], dram[wi+1], dram[wi] };
    end
  endfunction

  always @(posedge clk) begin
    if (!rst_n) begin
      rvalid <= 0; rlast <= 0; cur_addr <= 0; cur_left <= 0;
      q_addr.delete(); q_len.delete();
    end else begin
      if (arvalid && arready) begin
        q_addr.push_back(int'(araddr));
        q_len.push_back(int'(arlen) + 1);
      end
      if (!rvalid || rready) begin
        if (rvalid && rready && (cur_left > 1)) begin
          cur_addr <= cur_addr + BPB;
          cur_left <= cur_left - 1;
          rdata    <= dram_beat(cur_addr + BPB);
          rlast    <= (cur_left == 2);
          rvalid   <= 1;
        end else if (q_addr.size() > 0) begin
          cur_addr <= q_addr[0];
          cur_left <= q_len[0];
          rdata    <= dram_beat(q_addr[0]);
          rlast    <= (q_len[0] == 1);
          rvalid   <= 1;
          q_addr.delete(0);
          q_len.delete(0);
        end else begin
          rvalid <= 0; rlast <= 0;
        end
      end
    end
  end

  // ---- seed writer + AXI write-slave stub --------------------------------
  logic          seed_start = 0;
  logic [AW-1:0] seed_base  = 0;
  wire           seed_busy, seed_done, seed_err_align, seed_err_resp;

  wire [AW-1:0]  awaddr;  wire [7:0] awlen;  wire awvalid;  logic awready = 1;
  wire [DW-1:0]  wdata_w; wire wlast, wvalid;  logic wready = 1;
  logic          bvalid = 0;

  dma_seed_writer #(
    .AXI_DATA_W(DW), .AXI_ADDR_W(AW), .AXI_ID_W(2),
    .BURST_LEN(BL), .TOTAL_BEATS(N_BEATS)
  ) u_seed (
    .clk(clk), .rst_n(rst_n),
    .start(seed_start), .base_addr(seed_base),
    .busy(seed_busy), .done(seed_done),
    .err_align(seed_err_align), .err_resp(seed_err_resp),
    .m_axi_awid(), .m_axi_awaddr(awaddr), .m_axi_awlen(awlen), .m_axi_awsize(),
    .m_axi_awburst(), .m_axi_awlock(), .m_axi_awcache(), .m_axi_awprot(),
    .m_axi_awqos(), .m_axi_awvalid(awvalid), .m_axi_awready(awready),
    .m_axi_wdata(wdata_w), .m_axi_wstrb(), .m_axi_wlast(wlast),
    .m_axi_wvalid(wvalid), .m_axi_wready(wready),
    .m_axi_bid(2'd0), .m_axi_bresp(2'b00), .m_axi_bvalid(bvalid),
    .m_axi_bready()
  );

  int w_addr, w_left;
  logic w_active = 0;
  always @(posedge clk) begin
    if (!rst_n) begin w_active <= 0; bvalid <= 0; end
    else begin
      bvalid <= 0;
      if (awvalid && awready && !w_active) begin
        w_addr <= int'(awaddr); w_left <= int'(awlen) + 1; w_active <= 1;
      end
      if (wvalid && wready && w_active) begin
        for (int j = 0; j < DW/32; j++)
          dram[w_addr/4 + j] = wdata_w[32*j +: 32];
        w_addr <= w_addr + BPB;
        w_left <= w_left - 1;
        if (wlast) begin w_active <= 0; bvalid <= 1; end
      end
    end
  end

  // ---- read the buffers back and compare ---------------------------------
  int checked;

  // Position-sensitive checksum over the whole operand image.  The board
  // cannot hold a golden image, so it computes this and compares against the
  // constant simulation prints here.  Placement correctness is proved in
  // simulation; on hardware this catches dropped beats, stuck banks and BRAMs
  // that did not take the write.
  logic [31:0] chk;

  task automatic verify(input string what);
    begin
      checked = 0;
      chk     = '0;
      for (int k = 0; k < K_MAX; k++) begin
        for (int bk = 0; bk < N; bk++) begin
          a_raddr[bk] = K_W'(k);
          b_raddr[bk] = K_W'(k);
        end
        @(posedge clk);          // address issued
        @(posedge clk);          // synchronous read: data valid now
        for (int bk = 0; bk < N; bk++) begin
          if (seen_a[bk][k]) begin
            checked++;
            chk = chk + (a_rdata[bk] ^ 32'({bk[7:0], k[15:0]}));
            if (a_rdata[bk] !== gold_a[bk][k])
              fail($sformatf("%s A bank %0d k %0d: got %0d expected %0d",
                             what, bk, k, a_rdata[bk], gold_a[bk][k]));
          end
          if (seen_b[bk][k]) begin
            checked++;
            chk = chk + (b_rdata[bk] ^ 32'(32'h8000_0000 | {bk[7:0], k[15:0]}));
            if (b_rdata[bk] !== gold_b[bk][k])
              fail($sformatf("%s B bank %0d k %0d: got %0d expected %0d",
                             what, bk, k, b_rdata[bk], gold_b[bk][k]));
          end
        end
      end
      $display("    %-42s %0d entries compared   %s",
               what, checked, (errors == 0) ? "ok" : "<-- FAILED");
    end
  endtask

  task automatic transfer(input int base, input string what);
    int guard;
    begin
      load_payload(base);
      desc_addr  = AW'(base);
      desc_beats = 16'(N_BEATS);
      @(negedge clk); desc_valid = 1;
      @(negedge clk); desc_valid = 0;
      guard = 0;
      while (!done_valid && guard < 200000) begin @(posedge clk); guard++; end
      if (guard >= 200000) fail($sformatf("%s: descriptor never completed", what));
      repeat (8) @(posedge clk);
      verify(what);
    end
  endtask

  initial begin
    $display("\n=== tb_dma_path  N=%0d K_MAX=%0d ===========================", N, K_MAX);
    $display("    payload %0d B = %0d beats, one descriptor", RX_BYTES, N_BEATS);
    build_golden();
    for (int bk = 0; bk < N; bk++) begin a_raddr[bk] = '0; b_raddr[bk] = '0; end
    repeat (4) @(negedge clk); rst_n = 1; repeat (4) @(negedge clk);

    transfer('h00000, "base 0x00000 (256 B aligned)");
    transfer('h00fc0, "base 0x00fc0 (splits at a 4 KiB edge)");

    // Seed DRAM through dma_seed_writer instead of loading it directly, then
    // run the same transfer.  This is bring-up step 3a exactly as the board
    // will do it: nothing but the design itself put the image there.
    wipe();
    seed_base = AW'('h00000);
    @(negedge clk); seed_start = 1; @(negedge clk); seed_start = 0;
    while (!seed_done) @(posedge clk);
    if (seed_err_align) fail("seed writer refused an aligned base");
    for (int i = 0; i < RX_WORDS; i++)
      if (dram[i] !== 32'(i)) begin
        fail($sformatf("seeded DRAM word %0d = %0d, expected %0d", i, dram[i], i));
        i = RX_WORDS;
      end
    desc_addr  = AW'('h00000);
    desc_beats = 16'(N_BEATS);
    @(negedge clk); desc_valid = 1; @(negedge clk); desc_valid = 0;
    while (!done_valid) @(posedge clk);
    repeat (8) @(posedge clk);
    verify("seeded by dma_seed_writer, then read");
    $display("    board checksum for this geometry = 0x%08h", chk);

    if (err_align) fail("err_align set on an aligned descriptor");
    if (err_resp)  fail("err_resp set with an OKAY-only slave");
    if (err_range) fail("err_range set on a payload-sized descriptor");

    $display("=============================================================");
    if (errors == 0)
      $display("  OPERAND IMAGE MATCHES THE UART PATH, FROM BOTH ADDRESSES\n");
    else
      $display("  %0d FAILURE(S)\n", errors);
    $finish;
  end

  initial begin
    #200_000_000;
    $display("  GLOBAL TIMEOUT");
    $finish;
  end

endmodule

`default_nettype wire
