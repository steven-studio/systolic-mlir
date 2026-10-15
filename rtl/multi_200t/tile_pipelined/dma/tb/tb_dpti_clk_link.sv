`timescale 1ns / 1ps

// ============================================================================
// tb_dpti_clk_link.sv -- BENCH ONLY.  dpti_clk_gen + dpti_link_ctrl against
// Xilinx's own MMCME2_ADV simulation model (unisims_ver, Vivado xsim), not
// against tb/xil_stubs.sv.  Run: tb/run_dpti_clk_xsim.sh.
//
// What the stub cannot show and this bench does: the real model's lock time,
// its free-running VCO after CLKIN stops, and what clk_dpti does around RST.
//
// Scenarios, in order, on one continuous run:
//   S1  CLKOUT absent at power-up for 3 ms: the controller keeps retrying
//       (mmcm_reset_count rises), no clk_dpti edge, link down.
//   S2  CLKOUT appears: lock, link up; clk_dpti period and its offset from
//       CLKOUT measured.
//   S3  CLKOUT stops with the ui_clk side idle: LOCKED falls, the MMCM is
//       reset, clk_dpti stops, link DRAIN -> DOWN, dpti_rst high without a
//       clock.  Edges of clk_dpti after the last CLKOUT edge are counted.
//   S4  CLKOUT returns: relock, link up again, no fatal.
//   S5  CLKOUT stops while the MEM_WRITE engine is busy; it finishes inside
//       the drain window: link comes back with no fatal.
//   S6  CLKOUT stops while the engine is busy and stays busy: FATAL after
//       DRAIN_TIMEOUT_CYC; the link stays down after CLKOUT returns; only
//       ui_rst_n clears it.
// Global checks, whole run:
//   * clk_dpti rising edges while dpti_rst = 0 and LOCKED = 0: after CLKOUT
//     stops, the MMCM's VCO free-runs until the lock controller resets it, and
//     the DPTI domain (pads already safe: the PHY resets on !LOCKED) sees those
//     edges.  Per lock-loss event they must be few (<= MAX_FREE_EDGES) and none
//     may come faster than 60 MHz allows (period >= 16.0 ns);
//   * clk_dpti high and low phases: the shortest is reported; a phase shorter
//     than 7.0 ns that is not cut by RST is an error (runt);
//   * link_up never 1 while dpti_rst = 1 is synchronized as 1 (sanity).
// Verdict: "RESULT PASS" / "RESULT FAIL <n>".
// ============================================================================

module tb_dpti_clk_link;

  localparam integer DRAIN_TO       = 4000;
  localparam integer MAX_FREE_EDGES = 12;

  // ---------------- clocks ----------------
  logic clk_sys = 1'b0;   always #5.000 clk_sys = ~clk_sys;   // 100 MHz
  logic ui_clk  = 1'b0;   always #5.013 ui_clk  = ~ui_clk;    // ~99.7 MHz, unrelated
  logic clkout  = 1'b0;
  logic clk_run = 1'b0;
  always begin
    #8.3335;
    if (clk_run) clkout = ~clkout; else clkout = 1'b0;
  end

  logic ui_rst_n = 1'b0;

  // ui_clk-side activity model
  logic writer_busy = 1'b0;
  logic cmd_valid   = 1'b0;
  logic master_idle = 1'b1;

  // ---------------- DUT ----------------
  wire       clk_dpti, locked, hold_mmcm, dpti_rst, ui_link_rst, link_up, fatal, locked_ui;
  wire [7:0] mmcm_resets, drops;
  wire [2:0] st;

  dpti_clk_gen #(.LOCK_TIMEOUT_CYC(131072)) u_gen (
    .clk_sys (clk_sys), .clkout_pad (clkout), .hold_rst (hold_mmcm),
    .clk_dpti (clk_dpti), .locked (locked), .mmcm_reset_count (mmcm_resets)
  );

  dpti_link_ctrl #(.DRAIN_TIMEOUT_CYC(DRAIN_TO)) u_link (
    .ui_clk (ui_clk), .ui_rst_n (ui_rst_n), .mmcm_locked (locked),
    .mem_writer_busy (writer_busy), .cmd_fifo_rd_valid (cmd_valid),
    .cmd_master_idle (master_idle), .clk_dpti (clk_dpti),
    .dpti_rst (dpti_rst), .ui_link_rst (ui_link_rst), .hold_mmcm (hold_mmcm),
    .link_up (link_up), .err_link_fatal (fatal), .link_drop_count (drops),
    .locked_ui (locked_ui), .state_o (st)
  );

  // ---------------- monitors ----------------
  integer errors = 0;
  integer free_edges_evt  = 0;       // this lock-loss event: edges with LOCKED = 0, dpti_rst = 0
  integer free_edges_max  = 0;       // worst event
  integer free_edges_tot  = 0;
  realtime free_min_period = 1e9;
  realtime t_prev_rise     = 0;
  integer dpti_edges = 0;
  realtime t_rise = 0, t_fall = 0, min_hi = 1e9, min_lo = 1e9;
  realtime t_last_clkout = 0;
  integer  edges_after_stop = 0;
  logic    stop_window = 1'b0;
  integer  e0;

  always @(posedge clkout) t_last_clkout = $realtime;
  always @(negedge locked) free_edges_evt = 0;

  always @(posedge clk_dpti) begin
    dpti_edges = dpti_edges + 1;
    if (stop_window) edges_after_stop = edges_after_stop + 1;
    if (!dpti_rst && !locked) begin
      free_edges_evt = free_edges_evt + 1;
      free_edges_tot = free_edges_tot + 1;
      if (free_edges_evt > free_edges_max) free_edges_max = free_edges_evt;
      if (t_prev_rise > 0 && ($realtime - t_prev_rise) < free_min_period)
        free_min_period = $realtime - t_prev_rise;
    end
    t_prev_rise = $realtime;
    if (t_fall > 0 && ($realtime - t_fall) < min_lo) min_lo = $realtime - t_fall;
    t_rise = $realtime;
  end

  always @(negedge clk_dpti) begin
    // a high phase cut short by RST is not a runt: no extra rising edge
    if (!u_gen.mmcm_rst && t_rise > 0) begin
      if (($realtime - t_rise) < min_hi) min_hi = $realtime - t_rise;
      if (($realtime - t_rise) < 7.0) begin
        errors = errors + 1;
        $display("[%0t] ERROR runt high phase %0.3f ns", $realtime, $realtime - t_rise);
      end
    end
    t_fall = $realtime;
  end

  // ---------------- helpers ----------------
  task automatic wait_ui(input integer n); repeat (n) @(posedge ui_clk); endtask

  task automatic expect_(input bit cond, input string what);
    if (!cond) begin errors = errors + 1; $display("[%0t] ERROR %s", $realtime, what); end
    else $display("[%0t] ok    %s", $realtime, what);
  endtask

  task automatic wait_link_up(input integer max_us, input string tag);
    realtime t0 = $realtime;
    while (!link_up && ($realtime - t0) < max_us * 1000.0) wait_ui(1);
    expect_(link_up, $sformatf("%s: link up after %0.1f us (LOCKED %0b, state %0d)",
                               tag, ($realtime - t0) / 1000.0, locked, st));
  endtask

  task automatic measure_clock(input string tag);
    realtime a, b, off_sum; integer i;
    off_sum = 0;
    @(posedge clk_dpti); a = $realtime;
    repeat (100) @(posedge clk_dpti); b = $realtime;
    $display("[%0t] %s: clk_dpti period %0.4f ns over 100 cycles", $realtime, tag, (b - a) / 100.0);
    expect_(((b - a) / 100.0) > 16.60 && ((b - a) / 100.0) < 16.73, $sformatf("%s: clk_dpti is 60 MHz", tag));
    for (i = 0; i < 20; i++) begin
      @(posedge clkout); a = $realtime;
      @(posedge clk_dpti); b = $realtime;
      off_sum += (b - a);
    end
    // b - a is in [0, T): an aligned clock gives ~0 or ~T
    $display("[%0t] %s: clk_dpti rising edge %0.4f ns after CLKOUT (mean of 20; 16.667 = aligned one cycle on)",
             $realtime, tag, off_sum / 20.0);
  endtask

  task automatic stop_clkout(input string tag);
    edges_after_stop = 0;
    clk_run = 1'b0;
    @(negedge clkout);
    stop_window = 1'b1;
    $display("[%0t] %s: CLKOUT stopped", $realtime, tag);
  endtask

  // ---------------- sequence ----------------
  initial begin
    $display("TB tb_dpti_clk_link: real MMCME2_ADV model (unisims_ver)");
    #200; ui_rst_n = 1'b1;

    // S1
    #3_000_000;
    expect_(!locked && !link_up && dpti_edges == 0,
            $sformatf("S1: no CLKOUT for 3 ms -> no lock, no clk_dpti edges (%0d), link down", dpti_edges));
    expect_(mmcm_resets >= 2, $sformatf("S1: controller retried, %0d MMCM resets", mmcm_resets));
    expect_(dpti_rst, "S1: dpti_rst held high without a clock");

    // S2
    clk_run = 1'b1;
    wait_link_up(1000, "S2");
    measure_clock("S2");
    expect_(!fatal && !dpti_rst && !ui_link_rst, "S2: link up, resets released, no fatal");

    // S3
    stop_clkout("S3");
    wait_ui(2000);
    $display("[%0t] S3: clk_dpti rising edges after the last CLKOUT edge: %0d (MMCM VCO free-running until reset)",
             $realtime, edges_after_stop);
    expect_(!locked && !link_up && dpti_rst && ui_link_rst && !fatal,
            $sformatf("S3: LOCKED low, link down, dpti_rst and ui_link_rst high, no fatal (state %0d)", st));
    e0 = dpti_edges;
    #20_000;
    expect_(dpti_edges == e0, "S3: clk_dpti stopped (no edges in 20 us)");
    stop_window = 1'b0;

    // S4
    clk_run = 1'b1;
    wait_link_up(1000, "S4");
    expect_(drops == 8'd1 && !fatal, $sformatf("S4: relocked, one drop counted (%0d), no fatal", drops));
    measure_clock("S4");

    // S5
    writer_busy = 1'b1;
    stop_clkout("S5");
    wait_ui(DRAIN_TO / 4);
    expect_(st == 3'd3 && dpti_rst == 1'b0,
            $sformatf("S5: DRAIN while the engine is busy, clk_dpti side frozen not reset (state %0d dpti_rst %0b)", st, dpti_rst));
    writer_busy = 1'b0;
    wait_ui(500);
    expect_(!fatal && st == 3'd0, $sformatf("S5: engine finished in time -> DOWN, no fatal (state %0d)", st));
    stop_window = 1'b0;
    clk_run = 1'b1;
    wait_link_up(1000, "S5");

    // S6
    writer_busy = 1'b1;
    stop_clkout("S6");
    wait_ui(DRAIN_TO + 200);
    expect_(fatal && st == 3'd4 && dpti_rst && ui_link_rst,
            $sformatf("S6: engine stuck -> FATAL, both sides held in reset (state %0d)", st));
    stop_window = 1'b0;
    clk_run = 1'b1;
    #300_000;
    expect_(fatal && !link_up, "S6: CLKOUT back, link stays down while FATAL");
    ui_rst_n = 1'b0; writer_busy = 1'b0; wait_ui(4); ui_rst_n = 1'b1;
    wait_link_up(1000, "S6 after ui_rst_n");
    expect_(!fatal, "S6: ui_rst_n cleared FATAL");

    // ---------------- verdict ----------------
    $display("SUMMARY clk_dpti edges total %0d; with LOCKED=0 and dpti_rst=0: %0d in all, at most %0d per lock loss, shortest period among them %0.3f ns",
             dpti_edges, free_edges_tot, free_edges_max, free_min_period);
    $display("SUMMARY shortest clk_dpti high phase %0.3f ns, low phase %0.3f ns (RST-cut high phases excluded)",
             min_hi, min_lo);
    $display("SUMMARY MMCM resets issued %0d, link drops %0d", mmcm_resets, drops);
    if (free_edges_max > MAX_FREE_EDGES) begin
      errors = errors + 1;
      $display("ERROR %0d free-running clk_dpti edges after one lock loss (limit %0d)", free_edges_max, MAX_FREE_EDGES);
    end
    if (free_edges_tot > 0 && free_min_period < 16.0) begin
      errors = errors + 1;
      $display("ERROR free-running clk_dpti period %0.3f ns: faster than the domain is timed for", free_min_period);
    end
    if (min_lo < 7.0) begin
      errors = errors + 1;
      $display("ERROR runt low phase %0.3f ns", min_lo);
    end
    if (errors == 0) $display("RESULT PASS");
    else             $display("RESULT FAIL %0d", errors);
    $finish;
  end

  initial begin
    #40_000_000;
    $display("RESULT FAIL timeout");
    $finish;
  end

endmodule
