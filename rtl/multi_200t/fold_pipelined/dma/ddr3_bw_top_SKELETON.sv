// -----------------------------------------------------------------------------
// ddr3_bw_top_SKELETON.sv
//
// READ THIS BEFORE USING IT.  The probe and the UART are finished and verified.
// This file is NOT: the MIG instance below is a shape, not a port list.
//
// MIG's generated module name and its exact ports depend on YOUR .xci -- single-
// ended vs differential system clock, whether a separate 200 MHz reference clock
// is used, DQ width, address width, and the ratio.  Do not trust the names here.
//
// GET THE REAL ONE:
//   Vivado -> Sources -> right-click the MIG IP -> "Copy IP Instantiation
//   Template" (or open <ip_name>/<ip_name>.veo).  Paste that in place of the
//   block below and connect the app_* signals to the probe.
//
// Or, much easier for the first bring-up: generate the IP Example Design
// (right-click the IP -> Open IP Example Design).  Build and run THAT first --
// it is Xilinx's own traffic generator and it answers "is this memory alive"
// with zero RTL from you.  Only once its pass indicator lights should you come
// back here and swap in the probe.
// -----------------------------------------------------------------------------

`default_nettype none

module ddr3_bw_top (
  // ---- board ----
  input  wire        sys_clk_i,        // Nexys Video: 100 MHz system clock
  input  wire        cpu_resetn,       // active-low reset button
  output wire        uart_tx_pin,      // to the USB-UART bridge
  output wire [3:0]  led,

  // ---- DDR3 physical: copy this list VERBATIM from the .veo -------------
  output wire [14:0] ddr3_addr,
  output wire [2:0]  ddr3_ba,
  output wire        ddr3_cas_n,
  output wire        ddr3_ck_n,
  output wire        ddr3_ck_p,
  output wire        ddr3_cke,
  output wire        ddr3_ras_n,
  output wire        ddr3_reset_n,
  output wire        ddr3_we_n,
  inout  wire [15:0] ddr3_dq,
  inout  wire [1:0]  ddr3_dqs_n,
  inout  wire [1:0]  ddr3_dqs_p,
  output wire        ddr3_cs_n,
  output wire [1:0]  ddr3_dm,
  output wire        ddr3_odt
);

  // ---- signals between MIG and the probe ---------------------------------
  localparam int APP_ADDR_W = 29;   // <-- take from the .veo / MIG summary
  localparam int APP_DATA_W = 128;  // <-- 2 * nCK_PER_CLK * DQ_WIDTH

  wire                    ui_clk;
  wire                    ui_clk_sync_rst;
  wire                    init_calib_complete;
  wire [APP_ADDR_W-1:0]   app_addr;
  wire [2:0]              app_cmd;
  wire                    app_en;
  wire                    app_rdy;
  wire [APP_DATA_W-1:0]   app_rd_data;
  wire                    app_rd_data_valid;
  wire                    running, reported;

  wire rst_n = ~ui_clk_sync_rst;

  // =========================================================================
  // REPLACE THIS ENTIRE BLOCK with the instantiation template from your .veo.
  // The names below are the common 7-series DDR3 set, but yours may differ.
  // =========================================================================
  mig_7series_0 u_mig (
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
    .ddr3_cs_n           (ddr3_cs_n),
    .ddr3_dm             (ddr3_dm),
    .ddr3_odt            (ddr3_odt),

    .app_addr            (app_addr),
    .app_cmd             (app_cmd),
    .app_en              (app_en),
    .app_rdy             (app_rdy),
    .app_rd_data         (app_rd_data),
    .app_rd_data_end     (),
    .app_rd_data_valid   (app_rd_data_valid),

    // write path unused by the probe -- tie off exactly as the .veo expects
    .app_wdf_data        ({APP_DATA_W{1'b0}}),
    .app_wdf_end         (1'b0),
    .app_wdf_mask        ({(APP_DATA_W/8){1'b0}}),
    .app_wdf_wren        (1'b0),
    .app_wdf_rdy         (),

    .app_sr_req          (1'b0),
    .app_ref_req         (1'b0),
    .app_zq_req          (1'b0),
    .app_sr_active       (),
    .app_ref_ack         (),
    .app_zq_ack          (),

    .ui_clk              (ui_clk),
    .ui_clk_sync_rst     (ui_clk_sync_rst),
    .init_calib_complete (init_calib_complete),
    .device_temp         (),

    .sys_clk_i           (sys_clk_i),
    .sys_rst             (cpu_resetn)      // check the polarity in YOUR .xci
  );
  // =========================================================================

  ddr3_bw_probe #(
    .APP_ADDR_W  (APP_ADDR_W),
    .APP_DATA_W  (APP_DATA_W),
    .N_READS     (32'd65536),
    .ADDR_STRIDE (8),          // <-- confirm against the example design
    .CLK_HZ      (200_000_000), // <-- MUST match the real ui_clk, or the baud
    .BAUD        (115_200)      //     rate is wrong and the terminal shows junk
  ) u_probe (
    .clk                 (ui_clk),
    .rst_n               (rst_n),
    .init_calib_complete (init_calib_complete),
    .app_addr            (app_addr),
    .app_cmd             (app_cmd),
    .app_en              (app_en),
    .app_rdy             (app_rdy),
    .app_rd_data         (app_rd_data),
    .app_rd_data_valid   (app_rd_data_valid),
    .uart_tx_pin         (uart_tx_pin),
    .running             (running),
    .reported            (reported)
  );

  // led[0] calibration done, led[1] issuing, led[2] finished, led[3] heartbeat
  logic [25:0] beat;
  always_ff @(posedge ui_clk) beat <= beat + 1'b1;
  assign led = {beat[25], reported, running, init_calib_complete};

endmodule

`default_nettype wire
