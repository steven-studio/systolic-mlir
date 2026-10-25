// -----------------------------------------------------------------------------
// ddr3_bw_top.sv -- ddr3_calib_top with the probe wired in.
//
// Same clocking, same pinout, same LEDs.  The only difference is that the AXI4
// read channel now goes to ddr3_bw_probe_axi instead of being tied off, and the
// probe reports over its own UART.
//
// Bring it up in this order and each failure has one place to be:
//   1. ddr3_calib_top   -> does the memory calibrate?              (DONE, LD0 lit)
//   2. ddr3_bw_top      -> does it deliver data, and how fast?      (this file)
//
// If LD0 lights here as it did in step 1, the memory is fine and anything wrong
// is in the probe or the AXI wiring -- not in MIG, the clocks or the pinout.
//
// WHAT COMES OUT, at 115200 8N1 on the USB-UART, repeating every ~1 s:
//
//     BEATS=00010000 CYC=0001C4B7 SINK=8B3C10F7
//
//   words_per_ui_clk = 4 * BEATS / CYC          (128-bit beat = 4 fp32 words)
//   beta_ceiling     = words_per_ui_clk * f_ui / f_array
//                    = 4 * BEATS / CYC          (ui_clk = f_array = 100 MHz)
//
//   Compare against the fold-average operand demand
//   2sK/(K+2(s-1)+H) = 4096/365 = 11.2 words per array cycle at s=8, K=256.
//
//   Derived ceiling from the config is 4.00.  Whatever this measures will be
//   LOWER: refresh, row activation, and the controller's own scheduling.  The
//   gap between 4.00 and the measured number is itself a result -- it is the
//   efficiency of this memory controller on a purely sequential read stream,
//   and it is the best case the operand path will ever see.
//
//   Report it as beta_ceiling, never as beta.  See the header of
//   ddr3_bw_probe_axi.sv for why, and for why it is not the same quantity the
//   operand throttle sweeps.
// -----------------------------------------------------------------------------

`default_nettype none

module ddr3_bw_top (
  input  wire        sys_clk_pin,     // R4, 100 MHz
  input  wire        cpu_resetn,      // G4, active low
  output wire        uart_tx_pin,     // to the USB-UART bridge
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

  // ---- clocking: identical to ddr3_calib_top ------------------------------
  wire clk100_ibuf, clk200_raw, clkfb_raw, clkfb;
  wire clk_sys_100, clk_ref_200;
  wire mmcm_locked_user;

  IBUF u_ibuf (.I(sys_clk_pin), .O(clk100_ibuf));

  MMCME2_BASE #(
    .BANDWIDTH          ("OPTIMIZED"),
    .CLKIN1_PERIOD      (10.000),
    .DIVCLK_DIVIDE      (1),
    .CLKFBOUT_MULT_F    (10.000),      // VCO = 1000 MHz
    .CLKFBOUT_PHASE     (0.000),
    .CLKOUT0_DIVIDE_F   (5.000),       // 200 MHz for IDELAYCTRL
    .CLKOUT0_DUTY_CYCLE (0.500),
    .CLKOUT0_PHASE      (0.000),
    .REF_JITTER1        (0.010),
    .STARTUP_WAIT       ("FALSE")
  ) u_mmcm (
    .CLKIN1   (clk100_ibuf),
    .CLKFBIN  (clkfb),
    .CLKFBOUT (clkfb_raw),  .CLKFBOUTB (),
    .CLKOUT0  (clk200_raw), .CLKOUT0B  (),
    .CLKOUT1  (),           .CLKOUT1B  (),
    .CLKOUT2  (),           .CLKOUT2B  (),
    .CLKOUT3  (),           .CLKOUT3B  (),
    .CLKOUT4  (), .CLKOUT5 (), .CLKOUT6 (),
    .LOCKED   (mmcm_locked_user),
    .PWRDWN   (1'b0),
    .RST      (1'b0)
  );

  BUFG u_bufg_fb  (.I(clkfb_raw),   .O(clkfb));
  BUFG u_bufg_200 (.I(clk200_raw),  .O(clk_ref_200));
  BUFG u_bufg_100 (.I(clk100_ibuf), .O(clk_sys_100));

  wire sys_rst_n = cpu_resetn & mmcm_locked_user;

  wire ui_clk, ui_clk_sync_rst, mmcm_locked_mig, init_calib_complete;
  wire ui_rst_n = ~ui_clk_sync_rst;

  // ---- AXI4 read channel: probe <-> MIG -----------------------------------
  wire [1:0]   arid;
  wire [28:0]  araddr;
  wire [7:0]   arlen;
  wire [2:0]   arsize;
  wire [1:0]   arburst;
  wire [0:0]   arlock;
  wire [3:0]   arcache;
  wire [2:0]   arprot;
  wire [3:0]   arqos;
  wire         arvalid, arready;
  wire [127:0] rdata;
  wire [1:0]   rresp;
  wire         rlast, rvalid, rready;
  wire         probe_running, probe_reported;
  wire [31:0]  dbg_beats, dbg_cyc, dbg_sink;

  ddr3_bw_probe_axi #(
    .AXI_ADDR_W (29),
    .AXI_DATA_W (128),
    .AXI_ID_W   (2),
    .BURST_LEN  (16),           // 16 beats x 16 bytes = 256 B per burst
    .N_BURSTS   (32'd4096),     // 65536 beats = 1 MiB of sequential reads
    .MAX_OUTST  (8),
    .BASE_ADDR  (0),
    .CLK_HZ     (100_000_000),  // ui_clk -- MUST match, or the baud is wrong
    .BAUD       (115_200)
  ) u_probe (
    .clk                 (ui_clk),
    .rst_n               (ui_rst_n),
    .init_calib_complete (init_calib_complete),
    .m_axi_arid          (arid),
    .m_axi_araddr        (araddr),
    .m_axi_arlen         (arlen),
    .m_axi_arsize        (arsize),
    .m_axi_arburst       (arburst),
    .m_axi_arlock        (arlock),
    .m_axi_arcache       (arcache),
    .m_axi_arprot        (arprot),
    .m_axi_arqos         (arqos),
    .m_axi_arvalid       (arvalid),
    .m_axi_arready       (arready),
    .m_axi_rdata         (rdata),
    .m_axi_rresp         (rresp),
    .m_axi_rlast         (rlast),
    .m_axi_rvalid        (rvalid),
    .m_axi_rready        (rready),
    .uart_tx_pin         (uart_tx_pin),
    .running             (probe_running),
    .reported            (probe_reported),
    .dbg_beats           (dbg_beats),
    .dbg_cyc             (dbg_cyc),
    .dbg_sink            (dbg_sink)
  );

  // ---- read the result over JTAG, independently of the UART ---------------
  // A VIO samples registers on demand; no trigger, no capture buffer.  Read it
  // with:  vivado -mode batch -source bw_build.tcl -tclargs read
  vio_0 u_vio (
    .clk        (ui_clk),
    .probe_in0  (dbg_beats),
    .probe_in1  (dbg_cyc),
    .probe_in2  (dbg_sink),
    .probe_in3  ({1'b0, probe_reported, probe_running, init_calib_complete})
  );

  // ---- MIG ----------------------------------------------------------------
  mig_7series_0 u_mig_7series_0 (
    .ddr3_addr           (ddr3_addr),
    .ddr3_ba             (ddr3_ba),
    .ddr3_cas_n          (ddr3_cas_n),
    .ddr3_ck_n           (ddr3_ck_n),
    .ddr3_ck_p           (ddr3_ck_p),
    .ddr3_cke            (ddr3_cke),
    .ddr3_ras_n          (ddr3_ras_n),
    .ddr3_reset_n        (ddr3_reset_n),
    .ddr3_we_n           (ddr3_we_n),
    .ddr3_dq             (ddr3_dq),
    .ddr3_dqs_n          (ddr3_dqs_n),
    .ddr3_dqs_p          (ddr3_dqs_p),
    .init_calib_complete (init_calib_complete),
    .ddr3_dm             (ddr3_dm),
    .ddr3_odt            (ddr3_odt),

    .ui_clk              (ui_clk),
    .ui_clk_sync_rst     (ui_clk_sync_rst),
    .ui_addn_clk_0       (),
    .ui_addn_clk_1       (),
    .ui_addn_clk_2       (),
    .ui_addn_clk_3       (),
    .ui_addn_clk_4       (),
    .mmcm_locked         (mmcm_locked_mig),
    .aresetn             (ui_rst_n),
    .app_sr_req          (1'b0),
    .app_ref_req         (1'b0),
    .app_zq_req          (1'b0),
    .app_sr_active       (),
    .app_ref_ack         (),
    .app_zq_ack          (),

    // write channel unused: this design only reads
    .s_axi_awid          (2'b0),
    .s_axi_awaddr        (29'b0),
    .s_axi_awlen         (8'b0),
    .s_axi_awsize        (3'b0),
    .s_axi_awburst       (2'b0),
    .s_axi_awlock        (1'b0),
    .s_axi_awcache       (4'b0),
    .s_axi_awprot        (3'b0),
    .s_axi_awqos         (4'b0),
    .s_axi_awvalid       (1'b0),
    .s_axi_awready       (),
    .s_axi_wdata         (128'b0),
    .s_axi_wstrb         (16'b0),
    .s_axi_wlast         (1'b0),
    .s_axi_wvalid        (1'b0),
    .s_axi_wready        (),
    .s_axi_bid           (),
    .s_axi_bresp         (),
    .s_axi_bvalid        (),
    .s_axi_bready        (1'b0),

    // read channel: the probe
    .s_axi_arid          (arid),
    .s_axi_araddr        (araddr),
    .s_axi_arlen         (arlen),
    .s_axi_arsize        (arsize),
    .s_axi_arburst       (arburst),
    .s_axi_arlock        (arlock),
    .s_axi_arcache       (arcache),
    .s_axi_arprot        (arprot),
    .s_axi_arqos         (arqos),
    .s_axi_arvalid       (arvalid),
    .s_axi_arready       (arready),
    .s_axi_rid           (),
    .s_axi_rdata         (rdata),
    .s_axi_rresp         (rresp),
    .s_axi_rlast         (rlast),
    .s_axi_rvalid        (rvalid),
    .s_axi_rready        (rready),

    .sys_clk_i           (clk_sys_100),
    .clk_ref_i           (clk_ref_200),
    .sys_rst             (sys_rst_n)
  );

  // ---- LEDs ---------------------------------------------------------------
  //   led[0] init_calib_complete   memory alive       (must light, as before)
  //   led[1] probe issuing         reads in flight    (brief, ~0.7 ms)
  //   led[2] probe finished        report looping     (stays lit)
  //   led[3] our 200 MHz MMCM locked
  //   led[4] heartbeat, 100 MHz
  //   led[5] heartbeat, ui_clk
  (* keep = "true" *) reg [25:0] hb_sys = 26'd0;
  (* keep = "true" *) reg [25:0] hb_ui  = 26'd0;
  always_ff @(posedge clk_sys_100) hb_sys <= hb_sys + 1'b1;
  always_ff @(posedge ui_clk)      hb_ui  <= hb_ui  + 1'b1;

  assign led = { hb_ui[25], hb_sys[25], mmcm_locked_user,
                 probe_reported, probe_running, init_calib_complete };

endmodule

`default_nettype wire
