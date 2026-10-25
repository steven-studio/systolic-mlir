// -----------------------------------------------------------------------------
// dma_bringup_top.sv -- bring-up step 3a on the board.
//
//   DDR3 -> MIG -> dma_engine -> dma_operand_writer -> the operand buffers
//
// and nothing else.  The array is deliberately absent: if this lights up, the
// operand path works, and step 3b can then blame the array alone.
//
// SEQUENCE, once calibration completes:
//
//   1. dma_seed_writer fills SEED_BEATS beats at BASE_ADDR with a known image
//      (word at byte address A holds A/4 - BASE/4).  Nothing else in this
//      design can put a known pattern into DDR3, and without one the only
//      thing this test could report is "something moved".
//   2. dma_engine reads the same region back as one descriptor.
//   3. dma_operand_writer decodes it into u_a_buf / u_b_buf, using exactly the
//      byte layout systolic_uart_top's rx_count decode uses.
//   4. The buffers are scanned through their own read ports and a
//      position-sensitive checksum is accumulated over every entry.
//
// WHAT THE CHECKSUM DOES AND DOES NOT PROVE -- read this before trusting LD3.
//
//   It is NOT a correctness proof.  Placement correctness -- that each word
//   lands in the bank and address the UART path would have put it in -- is
//   proved in simulation by tb_dma_path.sv, entry by entry against a golden
//   model of the rx_count decode, at several geometries and from two base
//   addresses whose bursts split differently.  That is a logic property and it
//   does not become more true by running on silicon.
//
//   What the board adds is TRANSPORT.  The checksum catches dropped beats,
//   duplicated beats, a bank whose write enable never fires, a BRAM that does
//   not take the data, and an address that walks off.  Those are the failures
//   a simulator cannot have.  EXPECT_CHK is printed by tb_dma_path.sv for the
//   same N and K_MAX -- it comes from the golden model, not from a previous
//   run of this design, so a wrong-but-consistent design cannot agree with it.
//
//   Whole payload, one descriptor: RX_BYTES = K_MAX*8*N covers every (bank, k)
//   of both A and B exactly once, so every entry is scanned and none is stale.
//
// LEDS, right to left
//   led[0] init_calib_complete   memory alive
//   led[1] seed written
//   led[2] descriptor complete
//   led[3] CHECKSUM MATCHES      <- the gate
//   led[4] any error latched     (alignment, response, range)
//   led[5] heartbeat, ui_clk
//
// Read the numbers over JTAG with bringup_build.tcl -tclargs read.
// -----------------------------------------------------------------------------

`default_nettype none

module dma_bringup_top #(
  parameter integer N          = 8,
  parameter integer K_MAX      = 256,
  // Printed by tb_dma_path.sv for this N and K_MAX.  N=8 K_MAX=256 -> 387FDC00.
  parameter logic [31:0] EXPECT_CHK = 32'h387F_DC00,
  parameter integer BASE_ADDR  = 0
) (
  input  wire        sys_clk_pin,     // R4, 100 MHz
  input  wire        cpu_resetn,      // G4, active low
  output wire [5:0]  led,

  output wire [14:0] ddr3_addr,
  output wire [2:0]  ddr3_ba,
  output wire        ddr3_cas_n,
  output wire [0:0]  ddr3_ck_n,
  output wire [0:0]  ddr3_ck_p,
  output wire [0:0]  ddr3_cke,
  output wire        ddr3_ras_n,
  output wire        ddr3_reset_n,
  output wire        ddr3_we_n,
  inout  wire [15:0] ddr3_dq,
  inout  wire [1:0]  ddr3_dqs_n,
  inout  wire [1:0]  ddr3_dqs_p,
  output wire [1:0]  ddr3_dm,
  output wire [0:0]  ddr3_odt
);

  localparam integer AXI_DATA_W = 128;
  localparam integer AXI_ADDR_W = 29;
  localparam integer LANE_W     = $clog2(N);
  localparam integer K_W        = $clog2(K_MAX);
  localparam integer RX_BYTES   = K_MAX * 8 * N;
  localparam integer N_BEATS    = RX_BYTES / (AXI_DATA_W/8);

  // ---- clocking: identical to ddr3_calib_top / ddr3_bw_top ----------------
  wire clk100_ibuf, clk200_raw, clkfb_raw, clkfb;
  wire clk_sys_100, clk_ref_200;
  wire mmcm_locked_user;

  IBUF u_ibuf (.I(sys_clk_pin), .O(clk100_ibuf));

  MMCME2_BASE #(
    .BANDWIDTH ("OPTIMIZED"), .CLKIN1_PERIOD (10.000),
    .DIVCLK_DIVIDE (1), .CLKFBOUT_MULT_F (10.000), .CLKFBOUT_PHASE (0.000),
    .CLKOUT0_DIVIDE_F (5.000), .CLKOUT0_DUTY_CYCLE (0.500), .CLKOUT0_PHASE (0.000),
    .REF_JITTER1 (0.010), .STARTUP_WAIT ("FALSE")
  ) u_mmcm (
    .CLKIN1 (clk100_ibuf), .CLKFBIN (clkfb),
    .CLKFBOUT (clkfb_raw), .CLKFBOUTB (),
    .CLKOUT0 (clk200_raw), .CLKOUT0B (),
    .CLKOUT1 (), .CLKOUT1B (), .CLKOUT2 (), .CLKOUT2B (),
    .CLKOUT3 (), .CLKOUT3B (), .CLKOUT4 (), .CLKOUT5 (), .CLKOUT6 (),
    .LOCKED (mmcm_locked_user), .PWRDWN (1'b0), .RST (1'b0)
  );

  BUFG u_bufg_fb  (.I(clkfb_raw),  .O(clkfb));
  BUFG u_bufg_200 (.I(clk200_raw), .O(clk_ref_200));
  BUFG u_bufg_100 (.I(clk100_ibuf),.O(clk_sys_100));

  wire sys_rst_n = cpu_resetn & mmcm_locked_user;

  wire ui_clk, ui_clk_sync_rst, mmcm_locked_mig, init_calib_complete;
  wire ui_rst_n = ~ui_clk_sync_rst;

  // ---- AXI write channel: the seeder --------------------------------------
  wire [1:0]   awid;   wire [28:0] awaddr;  wire [7:0] awlen;
  wire [2:0]   awsize; wire [1:0]  awburst; wire [0:0] awlock;
  wire [3:0]   awcache; wire [2:0] awprot;  wire [3:0] awqos;
  wire         awvalid, awready;
  wire [127:0] wdata_axi; wire [15:0] wstrb; wire wlast, wvalid, wready;
  wire [1:0]   bid, bresp; wire bvalid, bready;

  // ---- AXI read channel: the engine ---------------------------------------
  wire [1:0]   arid;   wire [28:0] araddr;  wire [7:0] arlen;
  wire [2:0]   arsize; wire [1:0]  arburst; wire [0:0] arlock;
  wire [3:0]   arcache; wire [2:0] arprot;  wire [3:0] arqos;
  wire         arvalid, arready;
  wire [127:0] rdata_axi; wire [1:0] rresp; wire rlast, rvalid, rready;

  // ---- sequencer ----------------------------------------------------------
  typedef enum logic [2:0] {P_CALIB, P_SEED, P_READ, P_SCAN, P_DONE} phase_t;
  phase_t phase;

  logic          seed_start;
  wire           seed_busy, seed_done, seed_err_align, seed_err_resp;

  logic          desc_valid;
  wire           desc_ready, read_done;
  wire           eng_err_align, eng_err_resp, wr_err_range;
  wire [31:0]    words_written;

  // Scan index walks (k, bank) pairs, not k alone.  The first version summed
  // all 2N terms of one k in a single cycle -- for N=8 that is sixteen 32-bit
  // adds, and the tool disagreed with the claim that a depth-4 tree fits in
  // 10 ns on a -1 part: WNS came out at -1.011 ns with every failing path in
  // this adder.  Addition is commutative, so scanning one bank per cycle gives
  // the same checksum from two adds instead of sixteen, and EXPECT_CHK is
  // unchanged.  The scan takes N times as many cycles -- 2048 instead of 256 --
  // which is nothing next to the transfer it follows.
  //
  // Worth naming plainly: the failing paths were in the MEASUREMENT, not in
  // the path being measured.  An instrument that cannot meet timing casts
  // doubt on every number it reports, even when the number turns out right.
  logic [K_W+LANE_W:0] scan_i;
  logic          scan_val_d;
  logic [31:0]   chk;
  logic          desc_started;
  logic          seed_done_sticky, read_done_sticky;

  localparam integer SCAN_N = K_MAX * N;
  wire scan_last = (scan_i == (K_W+LANE_W+1)'(SCAN_N));
  wire [K_W-1:0]    scan_k    = scan_i[K_W+LANE_W-1:LANE_W];
  wire [LANE_W-1:0] scan_bank = scan_i[LANE_W-1:0];

  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n) begin
      phase      <= P_CALIB;
      seed_start <= 1'b0;
      desc_valid <= 1'b0;
      scan_i     <= '0;
    end else begin
      seed_start <= 1'b0;
      desc_valid <= 1'b0;
      case (phase)
        P_CALIB: if (init_calib_complete) begin
                   seed_start <= 1'b1;
                   phase      <= P_SEED;
                 end
        P_SEED:  if (seed_done) phase <= P_READ;
        P_READ:  begin
                   if (!desc_started) desc_valid <= 1'b1;
                   if (read_done) begin scan_i <= '0; phase <= P_SCAN; end
                 end
        P_SCAN:  if (scan_last) phase <= P_DONE;
                 else           scan_i <= scan_i + 1'b1;
        default: ;
      endcase
    end
  end

  // desc_valid is a pulse; remember it was taken so P_READ does not re-issue.
  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n)                    desc_started <= 1'b0;
    else if (phase == P_SEED)         desc_started <= 1'b0;
    else if (desc_valid && desc_ready) desc_started <= 1'b1;
  end

  // ---- seeder -------------------------------------------------------------
  dma_seed_writer #(
    .AXI_DATA_W (AXI_DATA_W), .AXI_ADDR_W (AXI_ADDR_W), .AXI_ID_W (2),
    .BURST_LEN (16), .TOTAL_BEATS (N_BEATS)
  ) u_seed (
    .clk (ui_clk), .rst_n (ui_rst_n),
    .start (seed_start), .base_addr (AXI_ADDR_W'(BASE_ADDR)),
    .busy (seed_busy), .done (seed_done),
    .err_align (seed_err_align), .err_resp (seed_err_resp),
    .m_axi_awid (awid), .m_axi_awaddr (awaddr), .m_axi_awlen (awlen),
    .m_axi_awsize (awsize), .m_axi_awburst (awburst), .m_axi_awlock (awlock),
    .m_axi_awcache (awcache), .m_axi_awprot (awprot), .m_axi_awqos (awqos),
    .m_axi_awvalid (awvalid), .m_axi_awready (awready),
    .m_axi_wdata (wdata_axi), .m_axi_wstrb (wstrb), .m_axi_wlast (wlast),
    .m_axi_wvalid (wvalid), .m_axi_wready (wready),
    .m_axi_bid (bid), .m_axi_bresp (bresp), .m_axi_bvalid (bvalid),
    .m_axi_bready (bready)
  );

  // ---- read engine --------------------------------------------------------
  wire          dst_wr_en;
  wire [15:0]   dst_wr_beat;
  wire [127:0]  dst_wr_data;
  wire          dst_full, dst_almost_full;

  dma_engine #(
    .AXI_DATA_W (AXI_DATA_W), .AXI_ADDR_W (AXI_ADDR_W), .AXI_ID_W (2),
    .BEAT_W (16), .BURST_LEN (16), .MAX_OUTSTANDING (8)
  ) u_eng (
    .clk (ui_clk), .rst_n (ui_rst_n), .init_calib_complete (init_calib_complete),
    .desc_valid (desc_valid), .desc_ready (desc_ready),
    .desc_addr (AXI_ADDR_W'(BASE_ADDR)), .desc_beats (16'(N_BEATS)),
    .desc_tag (8'h3A),
    .done_valid (read_done), .done_tag (),
    .m_axi_arid (arid), .m_axi_araddr (araddr), .m_axi_arlen (arlen),
    .m_axi_arsize (arsize), .m_axi_arburst (arburst), .m_axi_arlock (arlock),
    .m_axi_arcache (arcache), .m_axi_arprot (arprot), .m_axi_arqos (arqos),
    .m_axi_arvalid (arvalid), .m_axi_arready (arready),
    .m_axi_rid (2'b0), .m_axi_rdata (rdata_axi), .m_axi_rresp (rresp),
    .m_axi_rlast (rlast), .m_axi_rvalid (rvalid), .m_axi_rready (rready),
    .dst_almost_full (dst_almost_full), .dst_full (dst_full),
    .dst_wr_en (dst_wr_en), .dst_wr_beat (dst_wr_beat),
    .dst_wr_data (dst_wr_data), .dst_wr_tag (),
    .busy_cycles (), .rdy_stall_cycles (), .r_stall_cycles (),
    .err_align (eng_err_align), .err_resp (eng_err_resp), .stat_clear (1'b0)
  );

  // ---- operand writer and the two buffers ---------------------------------
  wire                a_wr, b_wr;
  wire [LANE_W-1:0]   wsel;
  wire [K_W-1:0]      waddr;
  wire [31:0]         wdata_buf;

  dma_operand_writer #(
    .N (N), .K_MAX (K_MAX), .AXI_DATA_W (AXI_DATA_W), .BEAT_W (16)
  ) u_wr (
    .clk (ui_clk), .rst_n (ui_rst_n),
    .dst_wr_en (dst_wr_en), .dst_wr_beat (dst_wr_beat), .dst_wr_data (dst_wr_data),
    .dst_full (dst_full), .dst_almost_full (dst_almost_full),
    .a_wr (a_wr), .b_wr (b_wr), .wsel (wsel), .waddr (waddr), .wdata (wdata_buf),
    .words_written (words_written), .err_range (wr_err_range), .clear (1'b0)
  );

  logic [K_W-1:0] a_raddr [0:N-1];
  logic [K_W-1:0] b_raddr [0:N-1];
  wire  [31:0]    a_rdata [0:N-1];
  wire  [31:0]    b_rdata [0:N-1];

  wire [K_W-1:0] scan_addr = scan_k;
  always_comb
    for (int i = 0; i < N; i++) begin
      a_raddr[i] = scan_addr;
      b_raddr[i] = scan_addr;
    end

  systolic_operand_buffer #(.K_MAX(K_MAX), .K_W(K_W), .N_BANKS(N)) u_a_buf (
    .clk (ui_clk), .wr (a_wr), .wsel (wsel), .waddr (waddr), .wdata (wdata_buf),
    .raddr (a_raddr), .rdata (a_rdata));

  systolic_operand_buffer #(.K_MAX(K_MAX), .K_W(K_W), .N_BANKS(N)) u_b_buf (
    .clk (ui_clk), .wr (b_wr), .wsel (wsel), .waddr (waddr), .wdata (wdata_buf),
    .raddr (b_raddr), .rdata (b_rdata));

  // ---- checksum -----------------------------------------------------------
  // Same formula tb_dma_path.sv uses, so EXPECT_CHK comes from the golden
  // model rather than from a previous run of this design.  The buffer read is
  // synchronous, so the address issued on cycle t is accumulated on t+1.
  logic [K_W-1:0]    scan_k_d;
  logic [LANE_W-1:0] scan_bank_d;
  logic [31:0]       chk_delta;

  // One cycle's contribution: all N banks of A and of B at the delayed address.
  // Written as a combinational sum rather than a loop with a local accumulator
  // inside always_ff -- Icarus rejects the latter, and a register-to-register
  // adder tree of depth log2(2N) has a full cycle at 100 MHz anyway.
  wire [15:0] kd16  = 16'(scan_k_d);
  wire [7:0]  bank8 = 8'(scan_bank_d);
  wire [31:0] pos   = {8'd0, bank8, kd16};
  always_comb
    chk_delta = (a_rdata[scan_bank_d] ^ pos)
              + (b_rdata[scan_bank_d] ^ (32'h8000_0000 | pos));

  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n) begin
      scan_val_d  <= 1'b0;
      scan_k_d    <= '0;
      scan_bank_d <= '0;
      chk         <= '0;
    end else begin
      scan_val_d  <= (phase == P_SCAN) && !scan_last;
      scan_k_d    <= scan_k;
      scan_bank_d <= scan_bank;
      if (phase == P_CALIB)  chk <= '0;
      else if (scan_val_d)   chk <= chk + chk_delta;
    end
  end

  wire chk_match = (phase == P_DONE) && (chk == EXPECT_CHK);
  wire any_err   = seed_err_align | seed_err_resp
                 | eng_err_align  | eng_err_resp | wr_err_range;

  // ---- MIG ----------------------------------------------------------------
  mig_7series_0 u_mig_7series_0 (
    .ddr3_addr (ddr3_addr), .ddr3_ba (ddr3_ba), .ddr3_cas_n (ddr3_cas_n),
    .ddr3_ck_n (ddr3_ck_n), .ddr3_ck_p (ddr3_ck_p), .ddr3_cke (ddr3_cke),
    .ddr3_ras_n (ddr3_ras_n), .ddr3_reset_n (ddr3_reset_n), .ddr3_we_n (ddr3_we_n),
    .ddr3_dq (ddr3_dq), .ddr3_dqs_n (ddr3_dqs_n), .ddr3_dqs_p (ddr3_dqs_p),
    .init_calib_complete (init_calib_complete),
    .ddr3_dm (ddr3_dm), .ddr3_odt (ddr3_odt),

    .ui_clk (ui_clk), .ui_clk_sync_rst (ui_clk_sync_rst),
    .ui_addn_clk_0 (), .ui_addn_clk_1 (), .ui_addn_clk_2 (),
    .ui_addn_clk_3 (), .ui_addn_clk_4 (),
    .mmcm_locked (mmcm_locked_mig), .aresetn (ui_rst_n),
    .app_sr_req (1'b0), .app_ref_req (1'b0), .app_zq_req (1'b0),
    .app_sr_active (), .app_ref_ack (), .app_zq_ack (),

    .s_axi_awid (awid), .s_axi_awaddr (awaddr), .s_axi_awlen (awlen),
    .s_axi_awsize (awsize), .s_axi_awburst (awburst), .s_axi_awlock (awlock),
    .s_axi_awcache (awcache), .s_axi_awprot (awprot), .s_axi_awqos (awqos),
    .s_axi_awvalid (awvalid), .s_axi_awready (awready),
    .s_axi_wdata (wdata_axi), .s_axi_wstrb (wstrb), .s_axi_wlast (wlast),
    .s_axi_wvalid (wvalid), .s_axi_wready (wready),
    .s_axi_bid (bid), .s_axi_bresp (bresp), .s_axi_bvalid (bvalid),
    .s_axi_bready (bready),

    .s_axi_arid (arid), .s_axi_araddr (araddr), .s_axi_arlen (arlen),
    .s_axi_arsize (arsize), .s_axi_arburst (arburst), .s_axi_arlock (arlock),
    .s_axi_arcache (arcache), .s_axi_arprot (arprot), .s_axi_arqos (arqos),
    .s_axi_arvalid (arvalid), .s_axi_arready (arready),
    .s_axi_rid (), .s_axi_rdata (rdata_axi), .s_axi_rresp (rresp),
    .s_axi_rlast (rlast), .s_axi_rvalid (rvalid), .s_axi_rready (rready),

    .sys_clk_i (clk_sys_100), .clk_ref_i (clk_ref_200), .sys_rst (sys_rst_n)
  );

  // ---- JTAG readout -------------------------------------------------------
  vio_0 u_vio (
    .clk       (ui_clk),
    .probe_in0 (chk),
    .probe_in1 (words_written),
    .probe_in2 (EXPECT_CHK),
    .probe_in3 ({ 2'b0, any_err, chk_match,
                  (phase == P_DONE), read_done_sticky, seed_done_sticky,
                  init_calib_complete })
  );

  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n) begin
      seed_done_sticky <= 1'b0;
      read_done_sticky <= 1'b0;
    end else begin
      if (seed_done) seed_done_sticky <= 1'b1;
      if (read_done) read_done_sticky <= 1'b1;
    end
  end

  // ---- LEDs ---------------------------------------------------------------
  (* keep = "true" *) reg [25:0] hb_ui = 26'd0;
  always_ff @(posedge ui_clk) hb_ui <= hb_ui + 1'b1;

  assign led = { hb_ui[25], any_err, chk_match,
                 read_done_sticky, seed_done_sticky, init_calib_complete };

endmodule

`default_nettype wire
