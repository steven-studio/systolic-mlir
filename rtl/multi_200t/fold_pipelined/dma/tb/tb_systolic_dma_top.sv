`default_nettype none
`timescale 1ns / 1ps

/*
 * tb_systolic_dma_top -- the integrated design, end to end, in simulation.
 *
 * Every stage below this has its own bench and its own mutants.  What none of
 * them can see is the seam: two write masters on one AXI channel, a phase
 * machine that hands the channel over between them, and a result reader started
 * from that machine and reading the C register the array wrote.  This bench
 * runs the real systolic_dma_top -- unmodified, with only the Xilinx IP
 * replaced -- from power-on to P_DONE, and asks four questions:
 *
 *   1. Does the operand path still work?  chk_wr is accumulated from the
 *      operand write stream and does not depend on the arithmetic, so the
 *      board's constant 0x3f880780 must come back even though fp_mul and
 *      fp_add here are integer pipelines.  If it does not, the write-back work
 *      broke the read path.
 *   2. Does the result tile reach memory intact?  Every word of the image at
 *      the result region is compared against the C register itself, in the order
 *      systolic_tx_source defines.
 *   3. Did the two write masters ever overlap?  err_w_owner is the design's own
 *      answer; the memory model's interleaving check is an independent one,
 *      taken from the far side of the bus.
 *   4. Does it finish at all?  A write-back whose last write response never
 *      returns hangs here rather than on the board.
 *
 * chk_c is deliberately NOT checked.  fp_model.sv is not floating point, so C
 * here is not the board's C.  What C contains is settled by the board and by
 * tools/seed_ref.py; what this bench settles is that whatever C contains
 * arrives in memory unchanged.
 *
 *   verilator --binary -Wno-fatal --timing --public-flat-rw \
 *       --top-module tb_systolic_dma_top -o tbrun \
 *       tb/tb_systolic_dma_top.sv tb/xil_stubs.sv tb/mig_axi_model.sv \
 *       ../../tb/fp_model.sv ../systolic_dma_top.sv ... && ./obj_dir/tbrun
 *
 * Icarus cannot run this one: the feeder and the array talk through unpacked
 * array ports, and its always_comb sensitivity inference leaves them at X.
 * That is why the array's own benches in tb/ are Verilator commands.
 */

module tb_systolic_dma_top #(
  // 0 = v1 operand path, 1 = beat-wide v2.  verilator -GUSE_V2=1 selects v2;
  // run_top_sim.sh v2 does that.  Same bench, same checks, both paths.
  parameter bit     USE_V2 = 1'b0,
  // 16 is the 3b geometry, 256 the paper's.  run_top_sim.sh [v1|v2] [K_MAX].
  parameter integer K_MAX  = 16
);

  localparam integer N        = 8;
  localparam integer RX_BYTES = K_MAX * 8 * N;
  localparam integer RX_WORDS = RX_BYTES / 4;
  // How many invocations this run makes: +n_inv=<count> on the command line,
  // which xil_stubs' vio_0 drives onto the design's output probe.  The operand
  // image is that many slabs long, so the write-back region moves with it --
  // the same arithmetic systolic_dma_top does from the latched n_inv, computed
  // here independently so the two have to agree.
  int unsigned n_inv = 1;
  int unsigned wb_base;
  int unsigned f;
  localparam integer MAX_CYC  = 400_000;
  // From tools/seed_ref.py --mode 1 --kmax <K_MAX>; the same table the build
  // script carries.  chk_wr does not depend on the arithmetic, so this bench
  // checks the tabulated constant even though fp_model is not floating point.
  localparam logic [31:0] EXPECT_WR_CHK = (K_MAX == 16)  ? 32'h3F88_0780 :
                                          (K_MAX == 32)  ? 32'h805C_1F00 :
                                          (K_MAX == 256) ? 32'h2DC7_F800 : 32'h0;

  logic clk  = 1'b0;
  logic rstn = 1'b0;
  always #5 clk = ~clk;                     // 100 MHz, the ui_clk frequency

  wire [7:0]  led;
  wire [14:0] ddr3_addr;  wire [2:0] ddr3_ba;
  wire        ddr3_cas_n, ddr3_ras_n, ddr3_reset_n, ddr3_we_n;
  wire [0:0]  ddr3_ck_n, ddr3_ck_p, ddr3_cke, ddr3_odt;
  wire [1:0]  ddr3_dm;
  wire [15:0] ddr3_dq;    wire [1:0] ddr3_dqs_n, ddr3_dqs_p;

  // EXPECT_WR_CHK is passed in, as dma_top_build.tcl passes it on the board:
  // the design's own growing expectation (want_wr, what led[4] is derived from)
  // is only meaningful against the constant for THIS geometry, and taking the
  // module default would leave the bench checking the K_MAX = 16 constant on a
  // K_MAX = 32 run.
  systolic_dma_top #(
    .N (N), .K_MAX (K_MAX), .K_DIM (K_MAX),
    .BASE_ADDR (0), .WB_GAP_BYTES (4096), .USE_V2 (USE_V2),
    .EXPECT_WR_CHK (EXPECT_WR_CHK)
  ) dut (
    .sys_clk_pin (clk), .cpu_resetn (rstn), .led (led),
    .ddr3_addr (ddr3_addr), .ddr3_ba (ddr3_ba), .ddr3_cas_n (ddr3_cas_n),
    .ddr3_ck_n (ddr3_ck_n), .ddr3_ck_p (ddr3_ck_p), .ddr3_cke (ddr3_cke),
    .ddr3_ras_n (ddr3_ras_n), .ddr3_reset_n (ddr3_reset_n),
    .ddr3_we_n (ddr3_we_n), .ddr3_dq (ddr3_dq),
    .ddr3_dqs_n (ddr3_dqs_n), .ddr3_dqs_p (ddr3_dqs_p),
    .ddr3_dm (ddr3_dm), .ddr3_odt (ddr3_odt)
  );

  // The memory model's ownership monitor needs the real boundary between the
  // operand image and the result tile, which moves with K_MAX; run_top_sim.sh
  // passes it as +wb_region=<bytes>.  Check that it arrived, or the monitor is
  // silently checking the wrong line.
  initial begin
    if (!$value$plusargs("n_inv=%d", n_inv)) n_inv = 1;
    wb_base = n_inv * RX_BYTES + 4096;
    #1;
    if (dut.u_mig_7series_0.wb_region != wb_base) begin
      $display("  FAIL: memory model wb_region=%0d but wb_base=%0d -- pass +wb_region=%0d",
               dut.u_mig_7series_0.wb_region, wb_base, wb_base);
      $fatal(1);
    end
  end

  // A stall in this design is a phase that never advances, so the phase is the
  // trace.  One line per transition turns a timeout from "it hung" into "it
  // hung in P_READ with 191 of 256 words written".
  logic [3:0] phase_d;
  always_ff @(posedge clk) begin
    phase_d <= dut.phase;
    if (dut.phase !== phase_d)
      $display("  phase %0d -> %0d   words=%0d", phase_d, dut.phase,
               dut.words_written);
  end

  integer errors = 0;
  integer waited = 0;
  // the first run's counters, kept to compare the re-run against
  logic [31:0] f_fill, f_cyc, f_wb, f_span, f_chk, f_words, f_eng;
  integer i, r, c;
  logic [31:0] got, want;

  initial begin
    $display("== tb_systolic_dma_top: seed -> DDR -> fold -> C -> DDR  (operand path %0s) ==",
             USE_V2 ? "v2, beat-wide" : "v1, four cycles per beat");
    $display("   %0d invocation(s) of k = %0d; result region at 0x%0h",
             n_inv, K_MAX, wb_base);
    repeat (20) @(posedge clk);
    rstn = 1'b1;

    // wb_done_sticky is set by the FIRST write-back, so it is not the end of a
    // multi-invocation run.  P_DONE is.
    while (dut.phase !== 4'd7 && waited < MAX_CYC) begin
      @(posedge clk);
      waited = waited + 1;
    end

    if (dut.phase !== 4'd7) begin
      $display("  FAIL: the run did not reach P_DONE after %0d cycles", MAX_CYC);
      $display("        phase=%0d seed=%0b read=%0b fold=%0b words=%0d",
               dut.phase, dut.seed_done_sticky, dut.read_done_sticky,
               dut.fold_done_sticky, dut.words_written);
      errors = errors + 1;
    end

    repeat (20) @(posedge clk);

    // ---- 1. the operand path is untouched ---------------------------------
    if (dut.words_written !== 32'(n_inv * RX_WORDS)) begin
      $display("  FAIL: words_written = %0d, want %0d (%0d slabs)",
               dut.words_written, n_inv * RX_WORDS, n_inv);
      errors = errors + 1;
    end
    if (dut.folds_done !== 8'(n_inv)) begin
      $display("  FAIL: folds_done = %0d, want %0d", dut.folds_done, n_inv);
      errors = errors + 1;
    end
    // The design latches n_inv on the way out of P_CALIB, so this is checked
    // after the run, not at time 0 when the register is still at its reset
    // value: the address map the design used has to be the one this bench
    // checked the image against.
    if (dut.n_inv !== 4'(n_inv)) begin
      $display("  FAIL: the design latched n_inv = %0d, the bench asked for %0d",
               dut.n_inv, n_inv);
      errors = errors + 1;
    end
    if (dut.wb_region_base !== wb_base) begin
      $display("  FAIL: the design put the result region at %0d, this bench at %0d",
               dut.wb_region_base, wb_base);
      errors = errors + 1;
    end
    if (EXPECT_WR_CHK == 32'h0)
      $display("  (no tabulated chk_wr for K_MAX=%0d -- add it from seed_ref.py)", K_MAX);
    else if (dut.chk_wr !== (EXPECT_WR_CHK * 32'(n_inv))) begin
      $display("  FAIL: chk_wr = %h, want %h (the board's constant x %0d)",
               dut.chk_wr, EXPECT_WR_CHK * 32'(n_inv), n_inv);
      errors = errors + 1;
    end else
      $display("  chk_wr = %h matches the board's constant x %0d",
               dut.chk_wr, n_inv);
    // The design's own growing expectation must have grown the same way -- it
    // is what drives led[4], and a bench that only checked chk_wr would not
    // notice the gate going dark on a correct run.
    if (dut.want_wr !== (EXPECT_WR_CHK * 32'(n_inv))) begin
      $display("  FAIL: want_wr = %h after %0d invocation(s), expected %h",
               dut.want_wr, n_inv, EXPECT_WR_CHK * 32'(n_inv));
      errors = errors + 1;
    end

    // ---- 2. the result image ----------------------------------------------
    // Word w of the image is C[w >> log2 N][w & (N-1)]: row major, exactly the
    // order systolic_tx_source walks and tb_dma_result_reader pins down.
    // One tile per invocation, each 256 B on from the last.  Every slab carries
    // the same operands, so every tile must equal the C register the last
    // invocation left behind: a tile that differs is a tile written from the
    // wrong fold, or to the wrong address.
    for (f = 0; f < n_inv; f = f + 1) begin
      for (i = 0; i < N*N; i = i + 1) begin
        r    = i / N;
        c    = i % N;
        want = dut.C[r][c];
        got  = dut.u_mig_7series_0.mem[(wb_base + f*N*N*4)/4 + i];
        if (got !== want) begin
          if (errors < 8)
            $display("  FAIL: tile %0d word %0d (row %0d col %0d): memory %h, C %h",
                     f, i, r, c, got, want);
          errors = errors + 1;
        end
      end
    end

    // ---- 3. the two write masters never overlapped ------------------------
    if (dut.err_w_owner !== 1'b0) begin
      $display("  FAIL: err_w_owner latched -- a master drove the write channel it did not own");
      errors = errors + 1;
    end
    if (dut.u_mig_7series_0.errors != 0) begin
      $display("  FAIL: the memory model reported %0d protocol error(s)",
               dut.u_mig_7series_0.errors);
      errors = errors + dut.u_mig_7series_0.errors;
    end
    if (dut.any_err !== 1'b0) begin
      $display("  FAIL: any_err latched (seed %0b%0b eng %0b%0b wr %0b wb %0b%0b own %0b)",
               dut.seed_err_align, dut.seed_err_resp,
               dut.eng_err_align, dut.eng_err_resp, dut.wr_err_range,
               dut.wb_err_align, dut.wb_err_resp, dut.err_w_owner);
      errors = errors + 1;
    end

    if (errors == 0)
      $display("  all %0d result words in memory match C, from one owner at a time",
               n_inv * N*N);

    // ---- 4. reported, not asserted ----------------------------------------
    // Printed, not checked.  The PE is deliberately agnostic to how many
    // cycles its multiplier and adder take -- fp_model's own header makes that
    // the point of running it at LAT = 3/5 and 9/12 -- so the cycle count in
    // simulation tracks the model's latency, not the IP's, and a difference of
    // a few cycles from the board's 125 is the model being a model.  Asserting
    // it here would be claiming these stubs are the array.  The number that
    // means something is the one the board reports, and dma_top_build.tcl
    // checks that one against EXPECT_CYC.
    $display("  cyc_latched = %0d  (the board reports k + 2(N-1) + 95 = %0d at k_dim = %0d)",
             dut.cyc_latched, K_MAX + 2*(N-1) + 95, K_MAX);
    if (n_inv > 1)
      $display("  cyc_total   = %0d over %0d invocations", dut.cyc_total, n_inv);

    // ---- 5. the operand-path counters, reported ---------------------------
    // The memory model returns a beat per cycle with no DRAM latency, so these
    // are the port-bound floor, not the board's numbers: v1 fill is ~4x the
    // beat count (one word per cycle into the single port) with r_stall about
    // three quarters of it; v2 fill is ~the beat count with r_stall ~0.  The
    // board adds the controller's latency and its 3.63 words/cycle ceiling.
    $display("  fill_cycles = %0d   (%0d beats)   engine busy %0d  rdy_stall %0d  r_stall %0d",
             dut.fill_cycles, RX_WORDS / 4, dut.eng_busy_cycles,
             dut.eng_rdy_stall_cycles, dut.eng_r_stall_cycles);
    $display("  wb_cycles   = %0d   wb engine busy %0d  aw_stall %0d  w_stall %0d  src_starve %0d",
             dut.wb_cycles, dut.wb_busy_cycles, dut.wb_aw_stall_cycles,
             dut.wb_w_stall_cycles, dut.wb_src_starve_cycles);
    // T as the paper adds it up, and the wall clock it sits inside.  The gap is
    // P_GO, the hand-offs, and n_inv scans of C -- instrumentation, not work.
    $display("  T (fill+compute+wb) = %0d   t_span (first AR to last B) = %0d   gap %0d",
             dut.fill_cycles + dut.cyc_total + dut.wb_cycles, dut.t_span,
             dut.t_span - (dut.fill_cycles + dut.cyc_total + dut.wb_cycles));
    if (USE_V2 && dut.eng_r_stall_cycles > dut.fill_cycles / 8) begin
      $display("  FAIL: v2 should not stall the engine's data channel (r_stall %0d of fill %0d)",
               dut.eng_r_stall_cycles, dut.fill_cycles);
      errors = errors + 1;
    end
    if (!USE_V2 && dut.eng_r_stall_cycles < dut.fill_cycles / 2) begin
      $display("  FAIL: v1 should stall the data channel ~3/4 of the fill (r_stall %0d of fill %0d)",
               dut.eng_r_stall_cycles, dut.fill_cycles);
      errors = errors + 1;
    end

    // ---- 6. the same run again, through the re-run probe ------------------
    // +rerun asks the design to start over from P_CALIB without a reset, which
    // is how the board sweeps n_inv.  Every counter is defined over "this run",
    // so the second run's numbers must equal the first's exactly.  A counter
    // that is cleared only by ui_rst_n doubles here -- and on the board it
    // would have been read as a slower run rather than as a broken counter.
    if ($test$plusargs("rerun")) begin
      f_fill  = dut.fill_cycles;   f_cyc   = dut.cyc_total;
      f_wb    = dut.wb_cycles;     f_span  = dut.t_span;
      f_chk   = dut.chk_wr;        f_words = dut.words_written;
      f_eng   = dut.eng_busy_cycles;
      // The memory model's interleaving monitor is a per-run invariant: once a
      // write has landed in the result region, a write back down in the operand
      // region means the two masters overlapped.  A re-run seeds the operand
      // region again, legitimately, so the monitor is re-armed here rather than
      // taught about runs -- the check it makes is exactly the check the second
      // run needs, once its starting assumption is restored.
      dut.u_mig_7series_0.wb_seen = 0;
      dut.u_vio.rerun_arg = 1'b1;
      repeat (4) @(posedge clk);
      dut.u_vio.rerun_arg = 1'b0;
      waited = 0;
      while (dut.phase !== 4'd7 && waited < MAX_CYC) begin
        @(posedge clk);
        waited = waited + 1;
      end
      repeat (20) @(posedge clk);
      if (dut.phase !== 4'd7) begin
        $display("  FAIL: the re-run never reached P_DONE (phase %0d)", dut.phase);
        errors = errors + 1;
      end
      if (dut.fill_cycles !== f_fill || dut.cyc_total !== f_cyc ||
          dut.wb_cycles !== f_wb || dut.t_span !== f_span ||
          dut.chk_wr !== f_chk || dut.words_written !== f_words ||
          dut.eng_busy_cycles !== f_eng) begin
        $display("  FAIL: the re-run does not report the same run as the first");
        $display("        fill %0d/%0d  cyc %0d/%0d  wb %0d/%0d  span %0d/%0d",
                 dut.fill_cycles, f_fill, dut.cyc_total, f_cyc,
                 dut.wb_cycles, f_wb, dut.t_span, f_span);
        $display("        chk_wr %h/%h  words %0d/%0d  eng busy %0d/%0d",
                 dut.chk_wr, f_chk, dut.words_written, f_words,
                 dut.eng_busy_cycles, f_eng);
        errors = errors + 1;
      end else
        $display("  re-run reproduces the first run exactly (fill %0d, T %0d)",
                 dut.fill_cycles, dut.fill_cycles + dut.cyc_total + dut.wb_cycles);
      if (dut.any_err !== 1'b0) begin
        $display("  FAIL: any_err latched during the re-run");
        errors = errors + 1;
      end
    end

    $display("== %s: %0d error(s) ==", (errors == 0) ? "PASS" : "FAIL", errors);
    if (errors != 0) $fatal(1);
    $finish;
  end

endmodule

`default_nettype wire
