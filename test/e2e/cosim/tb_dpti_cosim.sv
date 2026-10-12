`default_nettype none
`timescale 1ns / 1ps

/*
 * tb_dpti_cosim -- the board, in simulation, behind the host's DPTI link.
 * BENCH ONLY.
 *
 * systolic_dma_top is built exactly as dma_top_build.tcl builds the
 * 1x8x8 + 3x4x4 bitstream (external scheduler, no legacy job ports, v1
 * operand path); only the Xilinx IP is replaced: the MIG by
 * tb/mig_axi_model.sv, the MMCM/VIO/buffers by tb/xil_stubs.sv, and the
 * floating-point IP by fp_ieee.sv.
 *
 * Reduction depth: K_MAX_8X8 and K_MAX_4X4, independent (build.sh passes
 * them; 16 / 16 by default, the archived bitstream's). A top that predates
 * the split -- one K_MAX for every context, e.g. the tag
 * tile-pipelined-1x8x8-3x4x4-20261009 -- is built without
 * COSIM_SPLIT_KMAX and takes K_MAX = 16, the only value it was built with.
 *
 * The FT2232H is the part this bench adds. In synchronous FIFO mode it
 * clocks the interface at 60 MHz (CLKOUT); on every rising edge
 *   - with RXF# low it offers the next host byte on D while OE# is low,
 *     and a low RD# consumes that byte;
 *   - with TXE# low a low WR# hands it the byte the FPGA drives on D.
 * The bytes come from and go to a host over a UNIX socket (dpti_cosim.cpp);
 * the host is the unmodified runtime with libftdi replaced by
 * libftdi_cosim.so, so every byte here is a byte the runtime would put on
 * the USB cable.
 *
 * Simulation time stops while no host is connected, as an idle board would
 * wait; the design keeps its state across host sessions, as a programmed
 * board does. +max_sessions=<n> ends the run after n sessions (0: never).
 *
 * With the split-K_MAX RTL (COSIM_SPLIT_KMAX), every job also prints, when
 * its result tile is in DDR, the reduction steps the hardware executed for
 * it -- counted per PE, read-only, by rtl/.../dma/tb/reduction_probe.svh --
 * next to the descriptor's k:
 *   COSIM_STEPS dev=<d> job_k=<k> steps_pe_min=.. steps_pe_max=.. macs=..
 *               feed_a=.. feed_b=.. feed_cycles=.. k_dim=.. fire=.. done=..
 */

module tb_dpti_cosim #(
  parameter bit     USE_V2    = 1'b0,
  parameter integer K_MAX_8X8 = 16,
  parameter integer K_MAX_4X4 = 16
);

  import "DPI-C" function int  cosim_init(input string path, input int max_sessions);
  import "DPI-C" function int  cosim_service();
  import "DPI-C" function int  cosim_rx_count();
  import "DPI-C" function int  cosim_rx_peek();
  import "DPI-C" function void cosim_rx_pop();
  import "DPI-C" function void cosim_tx_push(input int b);

  logic sys_clk  = 1'b0;
  logic dpti_clk = 1'b0;
  always #5     sys_clk  = ~sys_clk;     // R4: 100 MHz board oscillator
  always #8.333 dpti_clk = ~dpti_clk;    // FT2232H CLKOUT, 60 MHz

  logic cpu_resetn = 1'b0;

  wire [7:0]  led;
  wire [14:0] ddr3_addr;  wire [2:0] ddr3_ba;
  wire        ddr3_cas_n, ddr3_ras_n, ddr3_reset_n, ddr3_we_n;
  wire [0:0]  ddr3_ck_n, ddr3_ck_p, ddr3_cke, ddr3_odt;
  wire [1:0]  ddr3_dm;
  wire [15:0] ddr3_dq;    wire [1:0] ddr3_dqs_n, ddr3_dqs_p;

  // ---- the FT2232H side of the DPTI bus ---------------------------------
  wire  [7:0] dpti_d;
  wire        dpti_rd_n, dpti_wr_n, dpti_oe_n;
  logic [7:0] ft_data = 8'h00;
  logic       ft_rxf_n = 1'b1;     // no host byte yet
  logic       ft_txe_n = 1'b0;     // the host side always has room

  // The FT2232H drives D only while the FPGA holds OE# low.
  assign dpti_d = !dpti_oe_n ? ft_data : 8'bz;

  systolic_dma_top #(
`ifdef COSIM_SPLIT_KMAX
    .N (8), .K_MAX_8X8 (K_MAX_8X8), .K_MAX_4X4 (K_MAX_4X4), .K_DIM (K_MAX_8X8),
`else
    .N (8), .K_MAX (16), .K_DIM (16),
`endif
    .NUM_8X8 (1), .NUM_4X4 (3),
    .USE_EXTERNAL_SCHEDULER (1'b1),
    .USE_LEGACY_JOB_PORTS (1'b0),
    .USE_V2 (USE_V2),
    .EXPECT_WR_CHK (32'h3F88_0780),
    .EXPECT_C_CHK (32'hC74B_2660)
  ) dut (
    .sys_clk_pin (sys_clk), .cpu_resetn (cpu_resetn), .led (led),
    .ddr3_addr (ddr3_addr), .ddr3_ba (ddr3_ba), .ddr3_cas_n (ddr3_cas_n),
    .ddr3_ck_n (ddr3_ck_n), .ddr3_ck_p (ddr3_ck_p), .ddr3_cke (ddr3_cke),
    .ddr3_ras_n (ddr3_ras_n), .ddr3_reset_n (ddr3_reset_n),
    .ddr3_we_n (ddr3_we_n), .ddr3_dq (ddr3_dq),
    .ddr3_dqs_n (ddr3_dqs_n), .ddr3_dqs_p (ddr3_dqs_p),
    .ddr3_dm (ddr3_dm), .ddr3_odt (ddr3_odt),

    // Unused on the board build (USE_LEGACY_JOB_PORTS = 0): tied off here
    // as the board's top leaves them.
    .scheduler_fold_start (1'b0), .scheduler_c_done (),
    .job_valid (1'b0), .job_ready (), .job_id (32'd0), .job_device_id (32'd0),
    .job_m (32'd0), .job_n (32'd0), .job_k (32'd0),
    .job_start_cycle (32'd0), .job_est_cycles (32'd0),
    .job_a_base (64'd0), .job_b_base (64'd0), .job_c_base (64'd0),
    .dpti_wr_valid (1'b0), .dpti_wr_ready (), .dpti_wr_addr (8'd0),
    .dpti_wr_data (32'd0),

    .dpti_d (dpti_d), .dpti_rxf_n (ft_rxf_n), .dpti_txe_n (ft_txe_n),
    .dpti_clkout (dpti_clk),
    .dpti_rd_n (dpti_rd_n), .dpti_wr_n (dpti_wr_n), .dpti_oe_n (dpti_oe_n),
    .dpti_siwun (1'b1)
  );

`ifdef COSIM_SPLIT_KMAX
  // ---- reduction steps per job (bench-only, nothing here drives the design) --
  wire clk = sys_clk;   // the array clock: ui_clk is sys_clk in mig_axi_model
`include "reduction_probe.svh"

  integer cj_cyc = 0;
  integer cj_k [0:3], cj_fire [0:3];
  initial for (int d = 0; d < 4; d++) begin cj_k[d] = -1; cj_fire[d] = -1; end
  always_ff @(posedge sys_clk) begin
    cj_cyc <= cj_cyc + 1;
    if (dut.job_fire && dut.effective_job_device_id < 4) begin
      cj_k[dut.effective_job_device_id]    <= integer'(dut.effective_job_k);
      cj_fire[dut.effective_job_device_id] <= cj_cyc;
    end
  end
  for (genvar d = 0; d < 4; d++) begin : COSIM_STEPS
    always @(posedge sys_clk)
      if (dut.ctx_job_done[d]) begin
        integer mn, mx, sum;
        rp_summary(d, mn, mx, sum);
        $display("COSIM_STEPS dev=%0d job_k=%0d steps_pe_min=%0d steps_pe_max=%0d macs=%0d feed_a=%0d feed_b=%0d feed_cycles=%0d k_dim=%0d fire=%0d done=%0d",
                 d, cj_k[d], mn, mx, sum, rp_feed_a[d], rp_feed_b[d], rp_feed_cyc[d], rp_kdim[d],
                 cj_fire[d], cj_cyc);
      end
  end
`endif

  string socket_path;
  int    max_sessions;
  bit    serving = 1'b0;

  initial begin
    if (!$value$plusargs("socket=%s", socket_path))
      socket_path = "/tmp/systolic-dpti-cosim.sock";
    if (!$value$plusargs("max_sessions=%d", max_sessions))
      max_sessions = 0;

    if (cosim_init(socket_path, max_sessions) != 0)
      $fatal(1, "cosim: cannot listen on %s", socket_path);

    // Power-on: reset, MIG calibration, every reset synchronizer released --
    // the state a freshly programmed board is in when the host opens it.
    repeat (20) @(posedge sys_clk);
    cpu_resetn = 1'b1;
    repeat (2000) @(posedge sys_clk);
    if (led[0] !== 1'b1)
      $fatal(1, "cosim: init_calib_complete (led[0]) did not rise");
    $display("cosim: board up (led=%b, K_MAX_8X8=%0d K_MAX_4X4=%0d), serving the DPTI link on %s",
             led, K_MAX_8X8, K_MAX_4X4, socket_path);
    serving = 1'b1;
  end

  // One FT2232H clock: account for the transfers this edge completes (the
  // strobes as they stood before it), then present the next state.
  always @(posedge dpti_clk) begin
    if (serving) begin
      if (!dpti_rd_n && !ft_rxf_n)
        cosim_rx_pop();
      if (!dpti_wr_n && !ft_txe_n)
        cosim_tx_push(int'(dpti_d));

      if (cosim_service() != 0) begin
        $display("cosim: %0d session(s) served, finishing", max_sessions);
        $finish;
      end

      ft_rxf_n <= (cosim_rx_count() == 0);
      ft_data  <= 8'(cosim_rx_peek());
    end
  end

endmodule

`default_nettype wire
