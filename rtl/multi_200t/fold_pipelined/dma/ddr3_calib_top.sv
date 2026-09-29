// -----------------------------------------------------------------------------
// ddr3_calib_top.sv -- the bring-up gate, and nothing else.
//
// One question: does the DDR3 on this board calibrate?
//
// No probe, no UART, no AXI traffic.  MIG plus the two clocks it needs, with
// its status on the LEDs.  If this lights up, the memory is alive and the MIG
// configuration is right, and any later failure is in YOUR logic, not in the
// controller or the pinout.  That separation is the whole point of building
// this before the probe.
//
// CLOCKING  (nexys_video_mig_axi128.prj: SystemClock and ReferenceClock both
// "No Buffer", so the top supplies both)
//
//   R4 (100 MHz) --IBUF--+--BUFG-------------------> sys_clk_i  (100 MHz)
//                        |
//                        +--MMCME2_BASE--BUFG-----> clk_ref_i  (200 MHz)
//                           VCO = 100*10/1 = 1000 MHz  (Artix-7 -1: 600-1200)
//                           CLKOUT0 = 1000/5 = 200 MHz
//
// clk_ref_i MUST be 200 MHz: the IDELAYE2 taps in the DDR3 read path carry
// REFCLK_FREQUENCY = 200.000, and IDELAYCTRL calibrates them against this
// clock.  Feed it 100 MHz and every tap is worth twice its design delay --
// the tools only warn, the memory reads garbage.  (We did exactly that
// earlier by setting ReferenceClock = "Use System Clock"; it silenced a DRC
// error and quietly broke the interface.)
//
// LEDS -- read them left to right as "how far did it get"
//   led[0]  init_calib_complete   DDR3 calibrated.  THIS IS THE GATE.
//   led[1]  mmcm_locked_user      our 200 MHz MMCM locked
//   led[2]  mmcm_locked_mig       MIG's internal MMCM locked
//   led[3]  ~ui_clk_sync_rst      MIG's user interface is out of reset
//   led[4]  heartbeat on 100 MHz  board oscillator is running
//   led[5]  heartbeat on ui_clk   MIG is producing a user clock
//
//   All dark            -> no board clock, or not programmed
//   only led[4]         -> our clocking is dead; check the MMCM
//   led[1][4] only      -> MIG never came up; check sys_rst polarity
//   led[1..5] but not 0 -> clocks fine, CALIBRATION failed.  That is the
//                          9/3 stop condition: the memory itself, the part,
//                          or the timing parameters in the .prj.
// -----------------------------------------------------------------------------

`default_nettype none

module ddr3_calib_top (
  input  wire        sys_clk_pin,     // R4, 100 MHz
  input  wire        cpu_resetn,      // G4, active low
  output wire [5:0]  led,

  // ---- DDR3, verbatim from mig_7series_0.veo -----------------------------
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

  // ---- clocking ----------------------------------------------------------
  wire clk100_ibuf, clk200_raw, clkfb_raw, clkfb;
  wire clk_sys_100, clk_ref_200;
  wire mmcm_locked_user;

  IBUF u_ibuf (.I(sys_clk_pin), .O(clk100_ibuf));

  // RST tied low on purpose: the MMCM runs from power-up and the reset button
  // resets only MIG.  Keeps "clocking is alive" and "MIG is in reset" as two
  // separately observable states while bringing the board up.
  MMCME2_BASE #(
    .BANDWIDTH          ("OPTIMIZED"),
    .CLKIN1_PERIOD      (10.000),      // 100 MHz in
    .DIVCLK_DIVIDE      (1),
    .CLKFBOUT_MULT_F    (10.000),      // VCO = 1000 MHz
    .CLKFBOUT_PHASE     (0.000),
    .CLKOUT0_DIVIDE_F   (5.000),       // 200 MHz
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

  // ---- resets ------------------------------------------------------------
  // sys_rst is ACTIVE LOW (SysResetPolarity in the .prj).  Hold MIG in reset
  // until the button is released AND our reference clock is locked -- MIG
  // must never start calibrating against an unlocked clk_ref_i.
  wire sys_rst_n = cpu_resetn & mmcm_locked_user;

  wire ui_clk, ui_clk_sync_rst, mmcm_locked_mig, init_calib_complete;
  wire aresetn = ~ui_clk_sync_rst;

  // ---- MIG ---------------------------------------------------------------
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
    .aresetn             (aresetn),
    .app_sr_req          (1'b0),
    .app_ref_req         (1'b0),
    .app_zq_req          (1'b0),
    .app_sr_active       (),
    .app_ref_ack         (),
    .app_zq_ack          (),

    // AXI4 write channel -- idle.  This design issues no transactions; it
    // exists only to answer "does the memory calibrate".
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

    // AXI4 read channel -- idle
    .s_axi_arid          (2'b0),
    .s_axi_araddr        (29'b0),
    .s_axi_arlen         (8'b0),
    .s_axi_arsize        (3'b0),
    .s_axi_arburst       (2'b0),
    .s_axi_arlock        (1'b0),
    .s_axi_arcache       (4'b0),
    .s_axi_arprot        (3'b0),
    .s_axi_arqos         (4'b0),
    .s_axi_arvalid       (1'b0),
    .s_axi_arready       (),
    .s_axi_rid           (),
    .s_axi_rdata         (),
    .s_axi_rresp         (),
    .s_axi_rlast         (),
    .s_axi_rvalid        (),
    .s_axi_rready        (1'b0),

    .sys_clk_i           (clk_sys_100),
    .clk_ref_i           (clk_ref_200),
    .sys_rst             (sys_rst_n)
  );

  // ---- heartbeats: prove each clock is actually toggling ------------------
  (* keep = "true" *) reg [25:0] hb_sys = 26'd0;
  (* keep = "true" *) reg [25:0] hb_ui  = 26'd0;

  always_ff @(posedge clk_sys_100) hb_sys <= hb_sys + 1'b1;
  always_ff @(posedge ui_clk)      hb_ui  <= hb_ui  + 1'b1;

  assign led = { hb_ui[25],
                 hb_sys[25],
                 ~ui_clk_sync_rst,
                 mmcm_locked_mig,
                 mmcm_locked_user,
                 init_calib_complete };

endmodule

`default_nettype wire
