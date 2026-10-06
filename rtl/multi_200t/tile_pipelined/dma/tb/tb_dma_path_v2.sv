// -----------------------------------------------------------------------------
// tb_dma_path_v2.sv -- the beat-wide read path, end to end, against v1 and
// against the UART decode.
//
//     behavioural DRAM -> AXI4 slave stub -> dma_engine -> dma_operand_writer_v2
//                      -> the REAL systolic_operand_buffer_v2 instances
//                         (cyclic layout on A, block layout on B)
//
// plus, fed from a tap on the same beat stream:
//
//                                  -> dma_operand_writer (v1, four cycles/beat)
//                                  -> the REAL systolic_operand_buffer (v1)
//
// After a descriptor completes, every depth of every bank of BOTH buffer pairs
// is read back through the unchanged read side and compared three ways:
//
//   1. v2 image == golden image from the rx_count decode  (the UART contract)
//   2. v2 image == v1 image loaded from the same payload  (the paper's check:
//      the cyclic partition moves where words live, and the contract is about
//      what the feeder sees, so the image is compared, not the stream)
//   3. the three checksums agree: v1 per-word chk_wr as systolic_dma_top
//      computes it, v2 per-beat chk from dma_wr_checksum_v2, and the image
//      checksum this bench prints as the board constant for the geometry.
//
// And then a STREAMING read: every bank issues a new, skewed address every
// cycle for K_MAX + N cycles, the way the feeder does, and each cycle's data
// is checked against the golden image.  The read-and-wait check above cannot
// see a mux select taken from the live raddr instead of the registered one;
// this can.
//
// TWO BASE ADDRESSES, as in tb_dma_path, then the seed-writer path.
//
// BUILD (from dma/):
//   iverilog -g2012 -o path2.out tb/tb_dma_path_v2.sv \
//       dma_engine.sv dma_seed_writer.sv \
//       dma_operand_writer.sv dma_operand_writer_v2.sv dma_wr_checksum_v2.sv \
//       ../core/operand_buffer.sv ../core/operand_buffer_v2.sv
//   vvp path2.out
//
// Geometry: -Ptb_dma_path_v2.N=4 / 16, -Ptb_dma_path_v2.K_MAX=32 / 256,
// -Ptb_dma_path_v2.SEED_MODE=1.  N = 8 is the degenerate geometry; run 4 and
// 16 as well, and K_MAX = 32 and 256 for the two k_max bitstreams.
// -----------------------------------------------------------------------------

`timescale 1ns/1ps
`default_nettype none

module tb_dma_path_v2 #(
  parameter int N         = 8,
  parameter int K_MAX     = 64,
  parameter int SEED_MODE = 0,
  parameter int MODULUS   = 127
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

  // ---- the seed value of payload word i (same two spellings as tb_dma_path)
  function automatic logic [31:0] seed_val(input int i);
    int v, e;
    logic [31:0] sh;
    begin
      if (SEED_MODE == 0) begin
        seed_val = 32'(i);
      end else begin
        v = (i % MODULUS) + 1;
        if      (v >= 64) e = 6;
        else if (v >= 32) e = 5;
        else if (v >= 16) e = 4;
        else if (v >=  8) e = 3;
        else if (v >=  4) e = 2;
        else if (v >=  2) e = 1;
        else              e = 0;
        sh       = 32'(v) << (23 - e);
        seed_val = {1'b0, 8'(127 + e), sh[22:0]};
      end
    end
  endfunction

  int errors = 0;
  task fail(input string m);
    begin errors++; if (errors < 15) $display("    FAIL: %s", m); end
  endtask

  // ---- behavioural DRAM --------------------------------------------------
  localparam int MEM_WORDS = 65536;
  logic [31:0] dram [0:MEM_WORDS-1];

  task automatic wipe();
    begin for (int i = 0; i < MEM_WORDS; i++) dram[i] = 32'hDEAD_0000 + i; end
  endtask

  task automatic load_payload(input int base_byte);
    int i;
    begin
      wipe();
      for (i = 0; i < RX_WORDS; i++)  dram[(base_byte/4) + i] = seed_val(i);
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
            gold_b[b_lane][(win << 3) | b_koff] = seed_val(wrd);
            seen_b[b_lane][(win << 3) | b_koff] = 1;
          end else begin
            gold_a[a_lane][(win << 3) | a_koff] = seed_val(wrd);
            seen_a[a_lane][(win << 3) | a_koff] = 1;
          end
        end
      end
    end
  endtask

  // ---- DUT wiring: engine -> writer v2 -> buffers v2 ----------------------
  logic          desc_valid = 0;
  wire           desc_ready;
  logic [AW-1:0] desc_addr  = 0;
  logic [15:0]   desc_beats = 0;
  wire           done_valid;

  wire [AW-1:0]  araddr;  wire [7:0] arlen;  wire arvalid;  logic arready = 1;
  logic [DW-1:0] rdata = 0;  logic rlast = 0, rvalid = 0;  wire rready;

  wire           dst_wr_en;  wire [15:0] dst_wr_beat;  wire [DW-1:0] dst_wr_data;
  wire           dst_full, dst_almost_full;
  wire           a_wr2, b_wr2;
  wire [LANE_W-1:0] wsel2;  wire [K_W-1:0] waddr2;  wire [DW-1:0] wdata2;
  wire [31:0]    words_written2;
  wire           err_align, err_resp, err_range2;
  wire [31:0]    chk_v2;
  logic          chk_clear = 0;

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

  dma_operand_writer_v2 #(
    .N(N), .K_MAX(K_MAX), .AXI_DATA_W(DW), .BEAT_W(16)
  ) u_wr2 (
    .clk(clk), .rst_n(rst_n),
    .dst_wr_en(dst_wr_en), .dst_wr_beat(dst_wr_beat), .dst_wr_data(dst_wr_data),
    .dst_full(dst_full), .dst_almost_full(dst_almost_full),
    .a_wr(a_wr2), .b_wr(b_wr2), .wsel(wsel2), .waddr(waddr2), .wdata(wdata2),
    .words_written(words_written2), .err_range(err_range2), .clear(chk_clear)
  );

  dma_wr_checksum_v2 #(.N(N), .K_MAX(K_MAX), .AXI_DATA_W(DW)) u_chk2 (
    .clk(clk), .rst_n(rst_n), .clear(chk_clear),
    .a_wr(a_wr2), .b_wr(b_wr2), .wsel(wsel2), .waddr(waddr2), .wdata(wdata2),
    .chk(chk_v2)
  );

  logic [K_W-1:0] a_raddr [0:N-1];
  logic [K_W-1:0] b_raddr [0:N-1];
  wire  [31:0]    a_rdata2 [0:N-1];
  wire  [31:0]    b_rdata2 [0:N-1];

  systolic_operand_buffer_v2 #(.K_MAX(K_MAX), .K_W(K_W), .N_BANKS(N),
                               .LAYOUT_CYCLIC(1'b1)) u_a_buf2 (
    .clk(clk), .wr(a_wr2), .wsel(wsel2), .waddr(waddr2), .wdata(wdata2),
    .raddr(a_raddr), .rdata(a_rdata2));

  systolic_operand_buffer_v2 #(.K_MAX(K_MAX), .K_W(K_W), .N_BANKS(N),
                               .LAYOUT_CYCLIC(1'b0)) u_b_buf2 (
    .clk(clk), .wr(b_wr2), .wsel(wsel2), .waddr(waddr2), .wdata(wdata2),
    .raddr(b_raddr), .rdata(b_rdata2));

  // ---- the v1 chain, replayed from a tap on the same beat stream ----------
  // The v2 writer never stalls the engine; the v1 writer needs four cycles per
  // beat.  So the beats are queued as they pass and replayed into v1 at v1's
  // pace.  Same payload, same beat order, same beat indices.
  logic [15:0] tap_beat[$];
  logic [DW-1:0] tap_data[$];
  always @(posedge clk) if (rst_n && dst_wr_en) begin
    tap_beat.push_back(dst_wr_beat);
    tap_data.push_back(dst_wr_data);
  end

  logic          v1_en = 0;
  logic [15:0]   v1_beat = 0;
  logic [DW-1:0] v1_data = 0;
  wire           v1_full, v1_afull;
  wire           a_wr1, b_wr1;
  wire [LANE_W-1:0] wsel1;  wire [K_W-1:0] waddr1;  wire [31:0] wdata1;
  wire [31:0]    words_written1;
  wire           err_range1;

  dma_operand_writer #(
    .N(N), .K_MAX(K_MAX), .AXI_DATA_W(DW), .BEAT_W(16)
  ) u_wr1 (
    .clk(clk), .rst_n(rst_n),
    .dst_wr_en(v1_en), .dst_wr_beat(v1_beat), .dst_wr_data(v1_data),
    .dst_full(v1_full), .dst_almost_full(v1_afull),
    .a_wr(a_wr1), .b_wr(b_wr1), .wsel(wsel1), .waddr(waddr1), .wdata(wdata1),
    .words_written(words_written1), .err_range(err_range1), .clear(chk_clear)
  );

  wire  [31:0]    a_rdata1 [0:N-1];
  wire  [31:0]    b_rdata1 [0:N-1];

  systolic_operand_buffer #(.K_MAX(K_MAX), .K_W(K_W), .N_BANKS(N)) u_a_buf1 (
    .clk(clk), .wr(a_wr1), .wsel(wsel1), .waddr(waddr1), .wdata(wdata1),
    .raddr(a_raddr), .rdata(a_rdata1));

  systolic_operand_buffer #(.K_MAX(K_MAX), .K_W(K_W), .N_BANKS(N)) u_b_buf1 (
    .clk(clk), .wr(b_wr1), .wsel(wsel1), .waddr(waddr1), .wdata(wdata1),
    .raddr(b_raddr), .rdata(b_rdata1));

  // v1 chk_wr, computed exactly as systolic_dma_top does it
  logic [31:0] chk_v1;
  wire [31:0]  wpos1 = {8'd0, 8'(wsel1), 16'(waddr1)};
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n)        chk_v1 <= '0;
    else if (chk_clear) chk_v1 <= '0;
    else if (a_wr1)    chk_v1 <= chk_v1 + (wdata1 ^ wpos1);
    else if (b_wr1)    chk_v1 <= chk_v1 + (wdata1 ^ (32'h8000_0000 | wpos1));
  end

  // replay process: drain the tap into v1, honouring v1's backpressure
  int v1_beats_done = 0;
  always @(negedge clk) begin
    if (!rst_n) begin
      v1_en <= 0;
    end else if (v1_en && v1_full) begin
      // hold the beat until the writer takes it
    end else if (tap_beat.size() > 0) begin
      v1_beat <= tap_beat.pop_front();
      v1_data <= tap_data.pop_front();
      v1_en   <= 1;
    end else begin
      v1_en <= 0;
    end
  end
  // a beat is taken when presented and not stalled
  always @(posedge clk) if (rst_n && v1_en && !v1_full) v1_beats_done++;

  // ---- fill timing on the v2 chain ---------------------------------------
  int t_desc = 0, t_last_v2 = 0, t_last_v1 = 0;
  always @(posedge clk) begin
    if (rst_n && desc_valid && desc_ready) t_desc = $time;
    if (rst_n && (a_wr2 || b_wr2))          t_last_v2 = $time;
    if (rst_n && (a_wr1 || b_wr1))          t_last_v1 = $time;
  end

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
    .BURST_LEN(BL), .TOTAL_BEATS(N_BEATS),
    .SEED_MODE(SEED_MODE), .MODULUS(MODULUS)
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

  // ---- read both images back and compare ---------------------------------
  int checked;
  logic [31:0] chk_img;

  task automatic verify(input string what);
    begin
      checked = 0;
      chk_img = '0;
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
            chk_img = chk_img + (a_rdata2[bk] ^ 32'({bk[7:0], k[15:0]}));
            if (a_rdata2[bk] !== gold_a[bk][k])
              fail($sformatf("%s A bank %0d k %0d: v2 got %0d expected %0d",
                             what, bk, k, a_rdata2[bk], gold_a[bk][k]));
            if (a_rdata2[bk] !== a_rdata1[bk])
              fail($sformatf("%s A bank %0d k %0d: v2 %0d != v1 %0d",
                             what, bk, k, a_rdata2[bk], a_rdata1[bk]));
          end
          if (seen_b[bk][k]) begin
            checked++;
            chk_img = chk_img + (b_rdata2[bk] ^ 32'(32'h8000_0000 | {bk[7:0], k[15:0]}));
            if (b_rdata2[bk] !== gold_b[bk][k])
              fail($sformatf("%s B bank %0d k %0d: v2 got %0d expected %0d",
                             what, bk, k, b_rdata2[bk], gold_b[bk][k]));
            if (b_rdata2[bk] !== b_rdata1[bk])
              fail($sformatf("%s B bank %0d k %0d: v2 %0d != v1 %0d",
                             what, bk, k, b_rdata2[bk], b_rdata1[bk]));
          end
        end
      end
      $display("    %-42s %0d entries, v2 == golden == v1   %s",
               what, checked, (errors == 0) ? "ok" : "<-- FAILED");
    end
  endtask

  // Streaming read: bank bk reads depth (t + bk) mod K_MAX on cycle t, a new
  // address every cycle, the feeder's access pattern.  The data is sampled
  // where the consumer samples it -- late in the NEXT cycle, after the next
  // address has already been applied -- because that is the only place a mux
  // select taken from the live raddr instead of the registered copy shows:
  // sampled right after the edge, while raddr is still the address just
  // read, a live select returns the right word and the bug is invisible.
  task automatic stream_verify(input string what);
    int addr_a  [0:N-1];
    int prev_a  [0:N-1];
    int nchk;
    begin
      nchk = 0;
      for (int t = 0; t <= K_MAX + N + 4; t++) begin
        // apply this cycle's address (right after the previous edge)
        for (int bk = 0; bk < N; bk++) begin
          addr_a[bk]  = (t + bk) % K_MAX;
          a_raddr[bk] = K_W'(addr_a[bk]);
          b_raddr[bk] = K_W'(addr_a[bk]);
        end
        // mid-cycle: the previous address's read is on rdata and the new
        // address is already on raddr -- what the consumer's register sees
        @(negedge clk);
        if (t > 0) begin
          for (int bk = 0; bk < N; bk++) begin
            if (seen_a[bk][prev_a[bk]]) begin
              nchk++;
              if (a_rdata2[bk] !== gold_a[bk][prev_a[bk]])
                fail($sformatf("%s stream A bank %0d k %0d: v2 got %0d expected %0d",
                               what, bk, prev_a[bk], a_rdata2[bk], gold_a[bk][prev_a[bk]]));
              if (a_rdata2[bk] !== a_rdata1[bk])
                fail($sformatf("%s stream A bank %0d k %0d: v2 %0d != v1 %0d",
                               what, bk, prev_a[bk], a_rdata2[bk], a_rdata1[bk]));
            end
            if (seen_b[bk][prev_a[bk]]) begin
              nchk++;
              if (b_rdata2[bk] !== gold_b[bk][prev_a[bk]])
                fail($sformatf("%s stream B bank %0d k %0d: v2 got %0d expected %0d",
                               what, bk, prev_a[bk], b_rdata2[bk], gold_b[bk][prev_a[bk]]));
              if (b_rdata2[bk] !== b_rdata1[bk])
                fail($sformatf("%s stream B bank %0d k %0d: v2 %0d != v1 %0d",
                               what, bk, prev_a[bk], b_rdata2[bk], b_rdata1[bk]));
            end
          end
        end
        @(posedge clk); #1;      // this edge captures addr_a
        for (int bk = 0; bk < N; bk++) prev_a[bk] = addr_a[bk];
      end
      $display("    %-42s %0d entries, one address per cycle   %s",
               {what, " (streaming)"}, nchk, (errors == 0) ? "ok" : "<-- FAILED");
    end
  endtask

  task automatic check_checksums(input string what);
    begin
      if (chk_v2 !== chk_v1)
        fail($sformatf("%s: per-beat chk 0x%08h != v1 per-word chk 0x%08h", what, chk_v2, chk_v1));
      if (chk_v2 !== chk_img)
        fail($sformatf("%s: per-beat chk 0x%08h != image chk 0x%08h", what, chk_v2, chk_img));
      if (words_written2 !== 32'(RX_WORDS))
        fail($sformatf("%s: v2 words_written %0d, expected %0d", what, words_written2, RX_WORDS));
      if (words_written1 !== 32'(RX_WORDS))
        fail($sformatf("%s: v1 words_written %0d, expected %0d", what, words_written1, RX_WORDS));
      if (err_range2) fail($sformatf("%s: v2 err_range set on a payload-sized descriptor", what));
      if (err_range1) fail($sformatf("%s: v1 err_range set on a payload-sized descriptor", what));
    end
  endtask

  task automatic wait_both(input string what);
    int guard;
    begin
      guard = 0;
      while (!done_valid && guard < 400000) begin @(posedge clk); guard++; end
      if (guard >= 400000) fail($sformatf("%s: descriptor never completed", what));
      // let the v1 replay drain
      guard = 0;
      while ((tap_beat.size() > 0 || v1_en || u_wr1.busy) && guard < 400000) begin
        @(posedge clk); guard++;
      end
      if (guard >= 400000) fail($sformatf("%s: v1 replay never drained", what));
      repeat (8) @(posedge clk);
    end
  endtask

  // one clear pulse resets both writers' counters and all three checksums
  task automatic clear_counters();
    begin
      @(negedge clk); chk_clear = 1; @(negedge clk); chk_clear = 0;
    end
  endtask

  task automatic transfer(input int base, input string what);
    begin
      load_payload(base);
      clear_counters();
      desc_addr  = AW'(base);
      desc_beats = 16'(N_BEATS);
      @(negedge clk); desc_valid = 1;
      @(negedge clk); desc_valid = 0;
      wait_both(what);
      verify(what);
      stream_verify(what);
      check_checksums(what);
      $display("    %-42s fill v2 = %0d cycles (%0.2f w/c)   v1 = %0d cycles (%0.2f w/c)",
               "", (t_last_v2 - t_desc) / 10, real'(RX_WORDS) / real'((t_last_v2 - t_desc) / 10),
               (t_last_v1 - t_desc) / 10, real'(RX_WORDS) / real'((t_last_v1 - t_desc) / 10));
    end
  endtask

  initial begin
    $display("\n=== tb_dma_path_v2  N=%0d K_MAX=%0d SEED_MODE=%0d =================",
             N, K_MAX, SEED_MODE);
    $display("    payload %0d B = %0d beats, one descriptor", RX_BYTES, N_BEATS);
    build_golden();
    for (int bk = 0; bk < N; bk++) begin a_raddr[bk] = '0; b_raddr[bk] = '0; end
    repeat (4) @(negedge clk); rst_n = 1; repeat (4) @(negedge clk);

    transfer('h00000, "base 0x00000 (256 B aligned)");
    transfer('h00fc0, "base 0x00fc0 (splits at a 4 KiB edge)");

    // Seed DRAM through dma_seed_writer, then the same transfer.
    wipe();
    seed_base = AW'('h00000);
    @(negedge clk); seed_start = 1; @(negedge clk); seed_start = 0;
    while (!seed_done) @(posedge clk);
    if (seed_err_align) fail("seed writer refused an aligned base");
    for (int i = 0; i < RX_WORDS; i++)
      if (dram[i] !== seed_val(i)) begin
        fail($sformatf("seeded DRAM word %0d = 0x%08h, expected 0x%08h",
                       i, dram[i], seed_val(i)));
        i = RX_WORDS;
      end
    clear_counters();
    desc_addr  = AW'('h00000);
    desc_beats = 16'(N_BEATS);
    @(negedge clk); desc_valid = 1; @(negedge clk); desc_valid = 0;
    wait_both("seeded");
    verify("seeded by dma_seed_writer, then read");
    stream_verify("seeded by dma_seed_writer, then read");
    check_checksums("seeded");
    $display("    board checksum for this geometry = 0x%08h  (v1 per-word 0x%08h, v2 per-beat 0x%08h)",
             chk_img, chk_v1, chk_v2);

    if (err_align)  fail("err_align set on an aligned descriptor");
    if (err_resp)   fail("err_resp set with an OKAY-only slave");

    $display("=============================================================");
    if (errors == 0)
      $display("  V2 IMAGE == V1 IMAGE == UART PATH, FROM BOTH ADDRESSES; CHECKSUMS AGREE\n");
    else
      $display("  %0d FAILURE(S)\n", errors);
    $finish;
  end

  initial begin
    #400_000_000;
    $display("  GLOBAL TIMEOUT");
    $finish;
  end

endmodule

`default_nettype wire
