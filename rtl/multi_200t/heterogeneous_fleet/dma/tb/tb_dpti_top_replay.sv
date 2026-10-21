`timescale 1ns / 1ps

// ============================================================================
// tb_dpti_top_replay.sv -- BENCH ONLY.  Level-2 integration bench for the
// FT2232H physical layer inside systolic_dma_top.
//
// The whole top (only the Xilinx IP replaced, as in the co-simulation:
// tb/xil_stubs.sv, tb/mig_axi_model.sv, test/e2e/cosim/fp_ieee.sv) is driven
// through its DPTI pins by a timing-aware FT2232H model, replaying a host byte
// stream captured from a real runtime session (libftdi_cosim.so,
// SYSTOLIC_DPTI_COSIM_CAPTURE).  Every byte the FPGA writes to the chip is
// dumped (+TXOUT) and, with +REF, compared with a reference stream.
//
// Works with either physical layer:
//   +define+PHY_SYNC245   the After tree, DPTI_PHY_SYNC245 = 1
//   +define+PHY_LEGACY_PARAM  the After tree, DPTI_PHY_SYNC245 = 0
//   (neither)             the Before tree (no such parameter, legacy layer)
// The FPGA's own bus drive is read hierarchically (legacy: dpti_tx_oe /
// dpti_tx_data; sync245: g_sync245_phy.pad_d_t / pad_d_o) for the
// contention checks only.
//
// Chip model, FT_000061 v2.10 Table 4.1 [DS] unless marked [ASM]:
//   CLKOUT 16.667 ns; RXF#, TXE#, read DATA change 1..7.15 ns after CLKOUT
//   (+TCO_MODE 0 random / 1 max / 2 min); OE# -> DATA valid 1..7.15 ns; RD#,
//   OE#, WR#, write DATA setup 8 ns, holds 0; transfers at an edge iff
//   RXF#&RD# (read) / TXE#&WR# (write) low before it; RD# needs OE# low at
//   this and the previous edge; never WR# with OE# or RD# low.
//   [ASM] OE# high -> D released after +TOFF (7.15); TXE# single-cycle
//   glitches (+GLITCH per mille); host bytes arrive in chunks (+CHUNK, +GAP).
// Board / FPGA [ASM]: +OFFS (CLKOUT pin -> FPGA register clock), +TD (trace),
//   +D_RD/+D_OE/+D_WR/+D_DAT (FPGA register or input -> pad), +TSU_F/+TH_F
//   (FPGA capture window, +CHK_WIN=1).
//
// Scenarios: +RESET_AT=n pulses cpu_resetn after the chip has handed over n
// bytes, then replays the whole stream again on the reset design (+STOPCLK=1:
// with CLKOUT stopped while reset is asserted).  The bytes written after the
// reset must equal the reference from its first byte.
// Link loss (RTL with dpti_link_ctrl; the run script defines HAS_LINK_CTRL):
//   +LINKDROP_GATE=g  CLKOUT stops for +LINKDROP_US microseconds once job g's
//                     result is complete -- a host closing and reopening the
//                     device between jobs -- and the replay continues.  It
//                     must still match the reference, with no fatal error.
//   +LINKDROP_AT=n    CLKOUT stops for +LINKDROP_US once the chip has handed
//                     over n bytes, wherever that is -- inside a MEM_WRITE it
//                     must latch dpti_link_fatal (a negative control: RESULT
//                     FAIL with led6_any_error=1 is the expected outcome).
//   Both check that RD#, OE#, WR# are high and D released while CLKOUT is
//   stopped, and print "LINK drops= fatal= mmcm_resets=".
//
// Verdict line: "RESULT PASS|FAIL ..." -- PASS needs zero protocol, setup,
// contention, window, spurious-strobe and reference errors, the whole stream
// consumed, the TX stream complete, and led[6] (any error latched) low.
// ============================================================================

module tb_dpti_top_replay #(
  parameter integer K_MAX_8X8 = 16,
  parameter integer K_MAX_4X4 = 16,
  parameter integer MAXB      = 4194304
);

  // ---------------- configuration ----------------
  real    T_FT   = 16.667;
  real    OFFS   = 0.10;
  real    TD     = 0.0;
  real    D_RD   = 0.5, D_OE = 0.5, D_WR = 0.5, D_DAT = 0.5;
  real    TSU_F  = 0.5, TH_F = 0.5;
  real    TOFF   = 7.15;
  integer TCO_MODE = 0;
  integer CHK_WIN  = 1;
  integer TXCAP    = 4096;
  integer DRAIN_PCT = 100;
  integer GLITCH   = 0;
  integer CHUNK    = 0;      // 0: the whole stream available at once
  integer GAP      = 0;
  integer RESET_AT = 0;
  integer LINKDROP_GATE = -1;
  integer LINKDROP_AT   = 0;
  integer LINKDROP_US   = 50;
  integer STOPCLK  = 0;
  integer NBYTES   = 0;
  integer NREF     = -1;
  integer SEED     = 1;
  integer IDLE_END = 30000;  // CLKOUT cycles with no traffic that end the run
  integer NGATES   = 0;      // job boundaries (offsets just past each SUBMIT)
  integer QUIET    = 3000;   // CLKOUT cycles without a written byte = result complete
  integer GATE_TO  = 4000000; // CLKOUT cycles to wait for a job's result
  real    D_IN     = 0.0;    // FPGA input pad -> capture register
  string  STREAM, TXOUT, REF, NAME, GATES;

  // ---------------- clocks / reset ----------------
  logic sys_clk = 1'b0;
  always #5 sys_clk = ~sys_clk;
  logic clk_ft = 1'b0;      // CLKOUT at the chip
  logic clk_run = 1'b1;
  logic clk_dut = 1'b0;     // dpti_clkout as the FPGA registers see it
  logic cpu_resetn = 1'b0;

  initial begin
    forever begin
      #(T_FT / 2.0);
      if (clk_run) clk_ft = ~clk_ft;
      else         clk_ft = 1'b0;
    end
  end
  always @(clk_ft) clk_dut <= #(OFFS) clk_ft;

  // ---------------- DUT ----------------
  wire [7:0]  led;
  wire [14:0] ddr3_addr;  wire [2:0] ddr3_ba;
  wire        ddr3_cas_n, ddr3_ras_n, ddr3_reset_n, ddr3_we_n;
  wire [0:0]  ddr3_ck_n, ddr3_ck_p, ddr3_cke, ddr3_odt;
  wire [1:0]  ddr3_dm;
  wire [15:0] ddr3_dq;    wire [1:0] ddr3_dqs_n, ddr3_dqs_p;

  wire  [7:0] dpti_d;
  wire        dpti_rd_n, dpti_wr_n, dpti_oe_n;
  logic       rxf_dut = 1'b1, txe_dut = 1'b1;
  logic [7:0] ft_val_dut = 8'h00;
  logic       ft_en_dut  = 1'b0;
  assign dpti_d = ft_en_dut ? ft_val_dut : 8'bz;

  systolic_dma_top #(
    .N (8), .K_MAX_8X8 (K_MAX_8X8), .K_MAX_4X4 (K_MAX_4X4), .K_DIM (K_MAX_8X8),
    .NUM_8X8 (1), .NUM_4X4 (3),
    .USE_EXTERNAL_SCHEDULER (1'b1),
    .USE_LEGACY_JOB_PORTS (1'b0),
    .USE_V2 (1'b0),
`ifdef PHY_SYNC245
    .DPTI_PHY_SYNC245 (1'b1),
`elsif PHY_LEGACY_PARAM
    .DPTI_PHY_SYNC245 (1'b0),
`endif
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
    .scheduler_fold_start (1'b0), .scheduler_c_done (),
    .job_valid (1'b0), .job_ready (), .job_id (32'd0), .job_device_id (32'd0),
    .job_m (32'd0), .job_n (32'd0), .job_k (32'd0),
    .job_start_cycle (32'd0), .job_est_cycles (32'd0),
    .job_a_base (64'd0), .job_b_base (64'd0), .job_c_base (64'd0),
    .dpti_wr_valid (1'b0), .dpti_wr_ready (), .dpti_wr_addr (8'd0),
    .dpti_wr_data (32'd0),
    .dpti_d (dpti_d), .dpti_rxf_n (rxf_dut), .dpti_txe_n (txe_dut),
    .dpti_clkout (clk_dut),
    .dpti_rd_n (dpti_rd_n), .dpti_wr_n (dpti_wr_n), .dpti_oe_n (dpti_oe_n),
    .dpti_siwun (1'b1)
  );

  // The FPGA's own drive of D (register / logic side, before the pad).
  wire       fpga_en_int;
  wire [7:0] fpga_val_int;
`ifdef PHY_SYNC245
  assign fpga_en_int  = !dut.g_sync245_phy.pad_d_t[0];
  assign fpga_val_int = dut.g_sync245_phy.pad_d_o;
`else
  assign fpga_en_int  = dut.dpti_tx_oe;
  assign fpga_val_int = dut.dpti_tx_data;
`endif

  // ---------------- pads -> chip (transport delays) ----------------
  logic       rd_ft = 1'b1, oe_ft = 1'b1, wr_ft = 1'b1;
  logic       fpga_en_ft = 1'b0, fpga_en_pin = 1'b0;
  logic [7:0] fpga_val_ft = 8'h00;
  // Arrival times at the chip pins are recorded where each delayed
  // assignment is scheduled (q_*) and consumed at the chip's edges.  The
  // first version stamped them in separate level-sensitive blocks
  // (always @(oe_ft) t_oe = $realtime;), which never updated in this
  // simulator, so no setup check could fire.
  realtime q_rd [$], q_oe [$], q_wr [$], q_d [$];
  always @(dpti_rd_n)    begin rd_ft       <= #(D_RD + TD)  dpti_rd_n;    q_rd.push_back($realtime + D_RD + TD); end
  always @(dpti_oe_n)    begin oe_ft       <= #(D_OE + TD)  dpti_oe_n;    q_oe.push_back($realtime + D_OE + TD); end
  always @(dpti_wr_n)    begin wr_ft       <= #(D_WR + TD)  dpti_wr_n;    q_wr.push_back($realtime + D_WR + TD); end
  always @(fpga_en_int)  begin fpga_en_ft  <= #(D_DAT + TD) fpga_en_int;  q_d.push_back($realtime + D_DAT + TD); end
  always @(fpga_val_int) begin fpga_val_ft <= #(D_DAT + TD) fpga_val_int; q_d.push_back($realtime + D_DAT + TD); end
  always @(fpga_en_int)  fpga_en_pin <= #(D_DAT)      fpga_en_int;

  // ---------------- chip state ----------------
  logic [7:0] stream [0:MAXB-1];
  logic [7:0] refb   [0:MAXB-1];
  integer     gate   [0:4095];
  integer avail_hi = 0;        // bytes made available by the host
  integer rd_ptr   = 0;        // next byte the chip hands over
  integer n_tx     = 0;        // bytes the chip accepted (this pass)
  integer n_tx_total = 0;
  integer txbuf    = 0;
  logic   rxf_state = 1'b1, txe_state = 1'b1;
  logic   prev_oe_low = 1'b0;
  logic   ft_en_ft = 1'b0;     // chip drive enable at its own pins
  logic   serving = 1'b0;
  integer pass_no = 1;
  integer last_activity = 0;
  integer ft_cyc = 0;
  integer txf;

  integer err_proto = 0, err_setup = 0, err_contend = 0, err_wdata = 0;
  integer err_ref = 0, err_win = 0, err_spurious = 0, n_rej = 0, n_rd_empty = 0;
  integer reported = 0;
  realtime t_rd = 0, t_oe = 0, t_wr = 0, t_d = 0;
  realtime t_rxf = 0, t_txe = 0, t_dd = 0, t_dedge = -100.0, t_dcrit = -100.0, t_ftedge = 0;
  logic in_reset = 1'b1;
  integer rst_edges = 0;      // CLKOUT edges since the current reset began

  task automatic rep(input string kind, input string msg);
    if (reported < 25) $display("TB %s @%0.3f ns: %s", kind, $realtime, msg);
    reported = reported + 1;
  endtask

  function automatic real urand(input real lo, input real hi);
    integer r;
    r = $random(SEED); if (r < 0) r = -r;
    return lo + (hi - lo) * ((r % 10001) / 10000.0);
  endfunction
  function automatic integer upct();
    integer r;
    r = $random(SEED); if (r < 0) r = -r;
    return r % 1000;
  endfunction
  function automatic real tco();
    if (TCO_MODE == 1) return 7.15;
    if (TCO_MODE == 2) return 1.0;
    return urand(1.0, 7.15);
  endfunction

  // last arrival at the chip pins not later than now (queues are time-ordered:
  // one constant delay per queue)
  function automatic realtime last_le(inout realtime q [$], input realtime last);
    realtime t;
    t = last;
    while (q.size() > 0 && q[0] <= $realtime) t = q.pop_front();
    return t;
  endfunction

  // chip drive of D follows OE# at its pins
  always @(oe_ft) begin
    real t6;
    if (oe_ft == 1'b0) begin
      t6 = urand(1.0, 7.15);
      ft_en_ft  <= #(t6) 1'b1;
      ft_en_dut <= #(t6 + TD + D_IN) 1'b1;
      q_dd.push_back($realtime + t6 + TD + D_IN);
    end
    else begin
      ft_en_ft  <= #(TOFF) 1'b0;
      ft_en_dut <= #(TOFF + TD + D_IN) 1'b0;
      q_dd.push_back($realtime + TOFF + TD + D_IN);
    end
  end

  // contention, both ends: sampled every 0.5 ns, one count per overlap
  logic ov_ft = 1'b0, ov_pin = 1'b0;
  integer n_contend_samples = 0;
  initial forever begin
    #0.5;
    if (serving && ft_en_ft && fpga_en_ft) begin
      n_contend_samples = n_contend_samples + 1;
      if (!ov_ft) begin err_contend = err_contend + 1; rep("CONTENTION", "both drive D at the chip pins"); end
      ov_ft = 1'b1;
    end else ov_ft = 1'b0;
    if (serving && ft_en_dut && fpga_en_pin) begin
      n_contend_samples = n_contend_samples + 1;
      if (!ov_pin) begin err_contend = err_contend + 1; rep("CONTENTION", "both drive D at the FPGA pins"); end
      ov_pin = 1'b1;
    end else ov_pin = 1'b0;
  end

  // FPGA capture window (inputs at the DUT ports vs clk_dut).  Change times
  // come from q_rxf / q_txe / q_dd, filled where the changes are scheduled;
  // each change is checked against the edge before it (hold) and the latest
  // change against this edge (setup).
  realtime q_rxf [$], q_txe [$], q_dd [$];
  logic [7:0] last_val_sched = 8'h00;
  realtime cap_t [$];
  logic [7:0] cap_v [$];
  always @(posedge clk_dut) begin
    realtime tc;
    while (q_rxf.size() > 0 && q_rxf[0] <= $realtime) begin
      tc = q_rxf.pop_front(); t_rxf = tc;
      if (serving && CHK_WIN != 0 && tc - t_dedge < TH_F) begin err_win = err_win + 1; rep("FPGA-HOLD", "RXF# changed in the hold window"); end
    end
    while (q_txe.size() > 0 && q_txe[0] <= $realtime) begin
      tc = q_txe.pop_front(); t_txe = tc;
      if (serving && CHK_WIN != 0 && tc - t_dedge < TH_F) begin err_win = err_win + 1; rep("FPGA-HOLD", "TXE# changed in the hold window"); end
    end
    while (q_dd.size() > 0 && q_dd[0] <= $realtime) begin
      tc = q_dd.pop_front(); t_dd = tc;
      if (serving && CHK_WIN != 0 && t_dcrit == t_dedge && tc - t_dedge < TH_F) begin err_win = err_win + 1; rep("FPGA-HOLD", "D changed in the hold window of a capture carrying a byte"); end
    end
    t_dedge = $realtime;
    if (serving && CHK_WIN != 0) begin
      if ($realtime - t_rxf < TSU_F) begin err_win = err_win + 1; rep("FPGA-SETUP", "RXF# not stable before the edge"); end
      if ($realtime - t_txe < TSU_F) begin err_win = err_win + 1; rep("FPGA-SETUP", "TXE# not stable before the edge"); end
    end
    while (cap_t.size() > 0 && cap_t[0] < $realtime - T_FT / 2.0) begin void'(cap_t.pop_front()); void'(cap_v.pop_front()); end
    if (cap_t.size() > 0 && cap_t[0] <= $realtime + T_FT / 2.0) begin
      logic [7:0] v;
      void'(cap_t.pop_front()); v = cap_v.pop_front();
      t_dcrit = $realtime;
      if (serving && CHK_WIN != 0) begin
        if ($realtime - t_dd < TSU_F) begin err_win = err_win + 1; rep("FPGA-SETUP", "D not stable before the edge capturing a byte"); end
        if (!ft_en_dut || ft_val_dut != v) begin err_win = err_win + 1; rep("FPGA-SETUP", "D at the FPGA differs from the byte transferred"); end
      end
    end
  end

  // ---------------- RX at the PHY / frontend boundary ----------------
  // Every byte the command frontend takes must be the next byte of the host
  // stream: no loss, duplicate, reorder or corruption inside the FPGA.
  integer n_fe = 0, err_fe = 0;
  always @(posedge clk_dut) begin
    if (dut.dpti_byte_valid && dut.dpti_byte_ready && !in_reset) begin
      if (n_fe >= NBYTES || dut.dpti_byte_data != stream[n_fe]) begin
        err_fe = err_fe + 1;
        if (err_fe <= 5) $display("TB RX-FRONTEND @%0.3f ns: byte %0d taken %02x, host sent %02x",
                                  $realtime, n_fe, dut.dpti_byte_data, (n_fe < NBYTES) ? stream[n_fe] : 8'h00);
      end
      n_fe = n_fe + 1;
    end
  end

  // ---------------- internal event trace (+TRACE=1, observation only) ----------------
  integer TRACE = 0;
  always @(posedge sys_clk) if (TRACE != 0) begin
    if (dut.job_fire)
      $display("TRACE %0.1f ns job_fire dev=%0d k=%0d m=%0d", $realtime, dut.effective_job_device_id,
               dut.effective_job_k, dut.effective_job_m);
    for (int d = 0; d < 4; d++) if (dut.ctx_job_done[d]) $display("TRACE %0.1f ns ctx_job_done dev=%0d", $realtime, d);
  end
  always @(oe_ft or rxf_dut) if (TRACE > 1 && serving)
    $display("TRACE2 %0.3f ns oe_ft=%b rxf_dut=%b dpti_oe_n=%b (last CLKOUT edge %0.3f)", $realtime, oe_ft, rxf_dut, dpti_oe_n, t_ftedge);
  always @(posedge clk_dut) if (TRACE != 0) begin
    if (dut.dpti_mem_start) $display("TRACE %0.1f ns dpti_mem_start", $realtime);
    if (dut.dpti_mem_done)  $display("TRACE %0.1f ns dpti_mem_done", $realtime);
    if (dut.dpti_host_cmd_valid && dut.dpti_host_cmd_ready)
      $display("TRACE %0.1f ns write32 addr=%02x data=%08x", $realtime, dut.dpti_host_cmd_addr, dut.dpti_host_cmd_data);
  end

  // ---------------- one CLKOUT edge ----------------
  always @(posedge clk_ft) begin
    logic s_rd, s_oe, s_wr, nx;
    logic [7:0] s_d;
    real tc;
    s_rd = rd_ft; s_oe = oe_ft; s_wr = wr_ft; s_d = fpga_val_ft;
    t_ftedge = $realtime;
    t_rd = last_le(q_rd, t_rd);
    t_oe = last_le(q_oe, t_oe);
    t_wr = last_le(q_wr, t_wr);
    t_d  = last_le(q_d,  t_d);
    ft_cyc = ft_cyc + 1;

    if (serving) begin
      if ($realtime - t_oe < 8.0) begin err_setup = err_setup + 1; rep("SETUP", "OE# within t7 = 8 ns of CLKOUT"); end
      if ($realtime - t_rd < 8.0) begin err_setup = err_setup + 1; rep("SETUP", "RD# within t9 = 8 ns of CLKOUT"); end
      if ($realtime - t_wr < 8.0) begin err_setup = err_setup + 1; rep("SETUP", "WR# within t14 = 8 ns of CLKOUT"); end
      if (!s_rd && !(!s_oe && prev_oe_low)) begin err_proto = err_proto + 1; rep("PROTOCOL", "RD# low without OE# low at this and the previous edge"); end
      if (!s_wr && (!s_oe || !s_rd)) begin err_proto = err_proto + 1; rep("PROTOCOL", "WR# low with OE# or RD# low"); end
      if (!s_wr) begin
        if ($realtime - t_d < 8.0) begin err_setup = err_setup + 1; rep("SETUP", "write DATA within t12 = 8 ns of CLKOUT"); end
        if (!fpga_en_ft) begin err_wdata = err_wdata + 1; rep("WDATA", "WR# low while the FPGA does not drive D"); end
      end
    end
    // A strobe launched just before reset may still reach the chip on the
    // next edge or two; after that, none while the design is held in reset.
    if (in_reset) rst_edges = rst_edges + 1;
    if (in_reset && rst_edges > 3 && (!s_rd || !s_wr)) begin
      err_spurious = err_spurious + 1; rep("SPURIOUS", "RD#/WR# low at the chip while the design is in reset");
    end

    // read transfer
    if (!s_rd && !rxf_state) begin
      if (CHK_WIN != 0) begin cap_t.push_back($realtime + OFFS); cap_v.push_back(stream[rd_ptr]); end
      rd_ptr = rd_ptr + 1;
      last_activity = ft_cyc;
    end
    else if (!s_rd) n_rd_empty = n_rd_empty + 1;

    // write transfer
    if (!s_wr && !txe_state) begin
      $fwrite(txf, "%02x\n", s_d);
      if (NREF >= 0) begin
        if (n_tx >= NREF) begin err_ref = err_ref + 1; rep("REF", "more bytes written than the reference holds"); end
        else if (s_d != refb[n_tx]) begin
          err_ref = err_ref + 1;
          if (err_ref <= 5) $display("TB REF @%0.3f ns: byte %0d expected %02x got %02x", $realtime, n_tx, refb[n_tx], s_d);
        end
      end
      n_tx = n_tx + 1; n_tx_total = n_tx_total + 1;
      txbuf = txbuf + 1;
      last_activity = ft_cyc;
    end
    else if (!s_wr) n_rej = n_rej + 1;

    if (txbuf > 0 && (upct() / 10) < DRAIN_PCT) txbuf = txbuf - 1;
    prev_oe_low = !s_oe;

    // next RXF# / TXE# / read DATA
    nx = (rd_ptr >= avail_hi);
    if (nx != rxf_state) begin
      rxf_state = nx; tc = tco();
      rxf_dut <= #(tc + TD + D_IN) nx;
      q_rxf.push_back($realtime + tc + TD + D_IN);
    end
    nx = (txbuf >= TXCAP) || (upct() < GLITCH);
    if (nx != txe_state) begin
      txe_state = nx; tc = tco();
      txe_dut <= #(tc + TD + D_IN) nx;
      q_txe.push_back($realtime + tc + TD + D_IN);
    end
    tc = tco();
    begin
      logic [7:0] nv;
      nv = (rd_ptr < avail_hi) ? stream[rd_ptr] : 8'h00;
      ft_val_dut <= #(tc + TD + D_IN) nv;
      if (nv != last_val_sched) begin q_dd.push_back($realtime + tc + TD + D_IN); last_val_sched = nv; end
    end
  end

  // ---------------- link loss ----------------
  integer err_linkdrop = 0;
  task automatic link_drop(input string why);
    $display("TB LINKDROP (%s): CLKOUT stopped for %0d us at chip byte %0d, %0d bytes written",
             why, LINKDROP_US, rd_ptr, n_tx);
    serving = 1'b0;
    clk_run = 1'b0;
    #2000;
    if (dpti_rd_n !== 1'b1 || dpti_oe_n !== 1'b1 || dpti_wr_n !== 1'b1 || fpga_en_int !== 1'b0) begin
      err_linkdrop = err_linkdrop + 1;
      $display("TB LINKDROP: rd_n=%b oe_n=%b wr_n=%b fpga_drive=%b with CLKOUT stopped",
               dpti_rd_n, dpti_oe_n, dpti_wr_n, fpga_en_int);
    end
    #((LINKDROP_US * 1000.0) - 2000.0);
    clk_run = 1'b1;
    repeat (2) @(posedge clk_ft);
    serving = 1'b1;
  endtask

  initial begin
    #1;
    wait (LINKDROP_AT > 0 && serving && rd_ptr >= LINKDROP_AT && pass_no == 1);
    link_drop("at byte");
  end

  // ---------------- host ----------------
  // The host as the runtime behaves: it sends one job's frames (operand
  // MEM_WRITE, descriptor WRITE32s, SUBMIT) and then waits for that job's
  // result before sending the next -- every job stages its slab at the same
  // DDR address, so running ahead would overwrite operands in use.
  integer err_gate = 0;
  logic   feed_done = 1'b0;
  logic   feed_aborted = 1'b0;   // the FPGA stopped reading: no later job can run
  task automatic host_feed();
    integer n, g, lim, tx0, quiet, waited;
    avail_hi = 0;
    for (g = 0; g <= NGATES; g = g + 1) begin
      lim = (g < NGATES) ? gate[g] : NBYTES;
      while (avail_hi < lim) begin
        if (CHUNK == 0) avail_hi = lim;
        else begin
          n = 1 + (upct() % CHUNK);
          avail_hi = (avail_hi + n > lim) ? lim : avail_hi + n;
          repeat (upct() % (GAP + 1)) @(posedge clk_ft);
        end
      end
      if (g < NGATES) begin
        // The FPGA reads available bytes within a few clocks; GATE_TO bounds
        // the wait so a design that has stopped reading (a fatal link loss,
        // +LINKDROP_AT) ends the run with a verdict instead of hanging.
        waited = 0;
        while (rd_ptr < lim && waited < GATE_TO) begin @(posedge clk_ft); waited = waited + 1; end
        if (rd_ptr < lim) begin
          err_gate = err_gate + 1;
          feed_aborted = 1'b1;
          $display("TB GATE: job %0d's bytes not consumed within %0d CLKOUT cycles (chip byte %0d of %0d); remaining jobs not fed",
                   g, GATE_TO, rd_ptr, lim);
          break;
        end
        tx0 = n_tx; quiet = 0; waited = 0;
        while (!(n_tx > tx0 && quiet >= QUIET) && waited < GATE_TO) begin
          @(posedge clk_ft);
          waited = waited + 1;
          if (n_tx > tx0 && ft_cyc - last_activity > 0) quiet = ft_cyc - last_activity; else quiet = 0;
        end
        $display("TB job %0d: %0d bytes written after its SUBMIT (waited %0d CLKOUT cycles)", g, n_tx - tx0, waited);
        if (waited >= GATE_TO) begin
          err_gate = err_gate + 1;
          $display("TB GATE: no complete result for job %0d (bytes written since submit: %0d)", g, n_tx - tx0);
        end
        if (g == LINKDROP_GATE) link_drop($sformatf("after job %0d", g));
      end
    end
    feed_done = 1'b1;
  endtask

  task automatic wait_board_up();
    repeat (20) @(posedge sys_clk);
    cpu_resetn = 1'b1;
    repeat (2000) @(posedge sys_clk);
    if (led[0] !== 1'b1) begin $display("TB FATAL: init_calib_complete did not rise"); $finish; end
    repeat (8) @(posedge clk_ft);
    in_reset = 1'b0;
    serving = 1'b1;
  endtask

  task automatic wait_done();
    while (!(feed_done && (rd_ptr >= NBYTES || feed_aborted) && ft_cyc - last_activity > IDLE_END)) @(posedge clk_ft);
  endtask

  initial begin
    integer i, fd, cnt;
    if (!$value$plusargs("STREAM=%s", STREAM)) begin $display("need +STREAM"); $finish; end
    if (!$value$plusargs("NBYTES=%d", NBYTES)) begin $display("need +NBYTES"); $finish; end
    if (!$value$plusargs("TXOUT=%s", TXOUT)) TXOUT = "tx.hex";
    if (!$value$plusargs("NAME=%s", NAME)) NAME = "replay";
    void'($value$plusargs("OFFS=%f", OFFS));
    void'($value$plusargs("TD=%f", TD));
    if ($value$plusargs("T_CO=%f", D_RD)) begin D_OE = D_RD; D_WR = D_RD; D_DAT = D_RD; end
    void'($value$plusargs("D_RD=%f", D_RD));
    void'($value$plusargs("D_OE=%f", D_OE));
    void'($value$plusargs("D_WR=%f", D_WR));
    void'($value$plusargs("D_DAT=%f", D_DAT));
    void'($value$plusargs("TSU_F=%f", TSU_F));
    void'($value$plusargs("TH_F=%f", TH_F));
    void'($value$plusargs("TOFF=%f", TOFF));
    void'($value$plusargs("TCO_MODE=%d", TCO_MODE));
    void'($value$plusargs("CHK_WIN=%d", CHK_WIN));
    void'($value$plusargs("TXCAP=%d", TXCAP));
    void'($value$plusargs("DRAIN_PCT=%d", DRAIN_PCT));
    void'($value$plusargs("GLITCH=%d", GLITCH));
    void'($value$plusargs("CHUNK=%d", CHUNK));
    void'($value$plusargs("GAP=%d", GAP));
    void'($value$plusargs("RESET_AT=%d", RESET_AT));
    void'($value$plusargs("STOPCLK=%d", STOPCLK));
    void'($value$plusargs("LINKDROP_GATE=%d", LINKDROP_GATE));
    void'($value$plusargs("LINKDROP_AT=%d", LINKDROP_AT));
    void'($value$plusargs("LINKDROP_US=%d", LINKDROP_US));
    void'($value$plusargs("SEED=%d", SEED));
    void'($value$plusargs("IDLE_END=%d", IDLE_END));
    void'($value$plusargs("QUIET=%d", QUIET));
    void'($value$plusargs("GATE_TO=%d", GATE_TO));
    void'($value$plusargs("TRACE=%d", TRACE));
    void'($value$plusargs("D_IN=%f", D_IN));
    if ($value$plusargs("GATES=%s", GATES)) begin
      void'($value$plusargs("NGATES=%d", NGATES));
      if (NGATES > 0) $readmemh(GATES, gate, 0, NGATES - 1);
    end
    $readmemh(STREAM, stream, 0, NBYTES - 1);
    if ($value$plusargs("REF=%s", REF)) begin
      void'($value$plusargs("NREF=%d", NREF));
      if (NREF > 0) $readmemh(REF, refb, 0, NREF - 1);
    end
    txf = $fopen(TXOUT, "w");
`ifdef PHY_SYNC245
    $display("CFG name=%s phy=sync245 NBYTES=%0d NREF=%0d NGATES=%0d D_IN=%0.3f OFFS=%0.3f TD=%0.3f D_RD=%0.3f D_OE=%0.3f D_WR=%0.3f D_DAT=%0.3f TSU_F=%0.2f TH_F=%0.2f TOFF=%0.2f TCO_MODE=%0d CHK_WIN=%0d TXCAP=%0d DRAIN=%0d GLITCH=%0d CHUNK=%0d GAP=%0d RESET_AT=%0d STOPCLK=%0d SEED=%0d",
`else
    $display("CFG name=%s phy=legacy NBYTES=%0d NREF=%0d NGATES=%0d D_IN=%0.3f OFFS=%0.3f TD=%0.3f D_RD=%0.3f D_OE=%0.3f D_WR=%0.3f D_DAT=%0.3f TSU_F=%0.2f TH_F=%0.2f TOFF=%0.2f TCO_MODE=%0d CHK_WIN=%0d TXCAP=%0d DRAIN=%0d GLITCH=%0d CHUNK=%0d GAP=%0d RESET_AT=%0d STOPCLK=%0d SEED=%0d",
`endif
             NAME, NBYTES, NREF, NGATES, D_IN, OFFS, TD, D_RD, D_OE, D_WR, D_DAT, TSU_F, TH_F, TOFF, TCO_MODE, CHK_WIN,
             TXCAP, DRAIN_PCT, GLITCH, CHUNK, GAP, RESET_AT, STOPCLK, SEED);

    wait_board_up();

    if (RESET_AT > 0) begin
      // pass 1: part of the stream, then reset in the middle of traffic
      fork host_feed(); join_none
      while (rd_ptr < RESET_AT) @(posedge clk_ft);
      repeat (40) @(posedge clk_ft);
      $display("TB reset at chip byte %0d, %0d bytes written so far (pass 1, not compared)", rd_ptr, n_tx);
      serving = 1'b0; in_reset = 1'b1; rst_edges = 0; pass_no = 2;
      if (STOPCLK != 0) begin
        clk_run = 1'b0;
        #200;
        cpu_resetn = 1'b0;
        #2000;
        // CLKOUT stopped: the pads must already be inactive
        if (dpti_rd_n !== 1'b1 || dpti_oe_n !== 1'b1 || dpti_wr_n !== 1'b1 || fpga_en_int !== 1'b0) begin
          err_spurious = err_spurious + 1;
          $display("TB RESET-STOPPED-CLK: rd_n=%b oe_n=%b wr_n=%b fpga_drive=%b with CLKOUT stopped and cpu_resetn low",
                   dpti_rd_n, dpti_oe_n, dpti_wr_n, fpga_en_int);
        end
        clk_run = 1'b1;
      end
      else begin
        cpu_resetn = 1'b0;
      end
      repeat (50) @(posedge clk_ft);
      // the host purges the chip and starts again
      disable fork;
      rd_ptr = 0; avail_hi = 0; txbuf = 0; n_tx = 0; feed_done = 1'b0; n_fe = 0;
      $fclose(txf); txf = $fopen(TXOUT, "w");
      wait_board_up();
    end
    fork host_feed(); join_none
    wait_done();

    begin
      integer total;
      logic ok_ref, ok_len;
      ok_len = (rd_ptr == NBYTES);
      ok_ref = (NREF < 0) || (n_tx == NREF && err_ref == 0);
      total = err_proto + err_setup + err_contend + err_wdata + err_ref + err_win + err_spurious + err_gate + err_fe +
              err_linkdrop +
              ((n_fe == NBYTES) ? 0 : 1) +
              (ok_len ? 0 : 1) + (ok_ref ? 0 : 1) + (led[6] ? 1 : 0);
      $display("RXCHECK frontend_took=%0d of %0d, mismatches=%0d", n_fe, NBYTES, err_fe);
      $display("CHECKS contention_samples=%0d", n_contend_samples);
`ifdef HAS_LINK_CTRL
      $display("LINK drops=%0d fatal=%0d mmcm_resets=%0d state=%0d pads_during_drop_errors=%0d",
               dut.dpti_link_drop_count, dut.dpti_link_fatal, dut.dpti_mmcm_reset_count,
               dut.dpti_link_state, err_linkdrop);
`endif
      $display("COUNTS host_bytes=%0d chip_handed_over=%0d fpga_written=%0d ref_bytes=%0d wr_rejected_by_txe=%0d rd_on_empty=%0d led=%b",
               NBYTES, rd_ptr, n_tx, NREF, n_rej, n_rd_empty, led);
      $display("ERRORS protocol=%0d setup=%0d contention=%0d wdata=%0d window=%0d spurious_or_reset=%0d job_result_missing=%0d ref_mismatch=%0d stream_incomplete=%0d tx_len_or_ref=%0d led6_any_error=%0d",
               err_proto, err_setup, err_contend, err_wdata, err_win, err_spurious, err_gate, err_ref,
               ok_len ? 0 : 1, ok_ref ? 0 : 1, led[6] ? 1 : 0);
      $display("RESULT %s name=%s total_errors=%0d", (total == 0) ? "PASS" : "FAIL", NAME, total);
    end
    $fclose(txf);
    $finish;
  end

  initial begin
    #(64'd400_000_000_000);
    $display("RESULT FAIL name=%s total_errors=-1 (time limit)", NAME);
    $finish;
  end

endmodule
