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
  parameter integer K_MAX  = 16,
  // Runtime reduction length supplied by the scheduler job.
  // K_MAX is hardware capacity; JOB_K is this job's actual K.
  parameter integer JOB_K = K_MAX,
  // 0 = legacy bring-up FSM owns fold_start.
  // 1 = external scheduler testbench owns accelerator start.
  parameter bit     USE_EXTERNAL_SCHEDULER = 1'b0
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

  // Scheduler integration signals.
  //
  // Legacy mode:
  //   USE_EXTERNAL_SCHEDULER = 0
  //   scheduler_fold_start is inactive.
  //
  // External-scheduler mode:
  //   generate exactly one scheduler_fold_start pulse while
  //   the accelerator is in P_FOLD.
  wire scheduler_c_done;

  // ----------------------------------------------------------
  // Scheduler job interface.
  //
  // This TB sends one logical job descriptor.  The current RTL
  // only latches the descriptor; DMA address generation is not
  // changed yet.
  // ----------------------------------------------------------
  logic        job_valid;
  wire         job_ready;

  logic [31:0] job_id;
  logic [31:0] job_m;
  logic [31:0] job_n;
  logic [31:0] job_k;
  logic [63:0] job_a_base;
  logic [63:0] job_b_base;
  logic [63:0] job_c_base;

  logic        job_sent;

  logic [31:0] accepted_job_id;
  logic [31:0] accepted_job_m;
  logic [31:0] accepted_job_n;
  logic [31:0] accepted_job_k;
  logic [63:0] accepted_job_a_base;
  logic [63:0] accepted_job_b_base;
  logic [63:0] accepted_job_c_base;

  always_comb begin
    // Do not present a scheduler transaction while the DUT UI clock
    // domain is still in reset.  Otherwise job_valid && job_ready can
    // be observed by the TB before the DUT's job registers are able
    // to latch the descriptor.
    job_valid  = USE_EXTERNAL_SCHEDULER &&
                 !job_sent &&
                 dut.ui_rst_n;

    job_id     = 32'd100;
    job_m      = 32'd8;
    job_n      = 32'd8;
    job_k      = 32'(JOB_K);

    job_a_base = 64'h0000_0000_0000_1000;
    job_b_base = 64'h0000_0000_0000_2000;
    job_c_base = 64'h0000_0000_0000_3000;
  end

  // Scheduler job producer and DUT consumer must use the same clock.
  // The DUT latches the descriptor on ui_clk; using the raw sys clk here
  // could make the TB observe job_valid && job_ready without the DUT
  // actually sampling job_fire on the corresponding ui_clk edge.
  always_ff @(posedge dut.ui_clk or negedge rstn) begin
    if (!rstn) begin
      job_sent         <= 1'b0;
      accepted_job_id  <= 32'd0;
      accepted_job_m   <= 32'd0;
      accepted_job_n   <= 32'd0;
      accepted_job_k   <= 32'd0;
      accepted_job_a_base <= 64'd0;
      accepted_job_b_base <= 64'd0;
      accepted_job_c_base <= 64'd0;
    end
    else if (job_valid && job_ready) begin
      job_sent            <= 1'b1;

      accepted_job_id     <= job_id;
      accepted_job_m      <= job_m;
      accepted_job_n      <= job_n;
      accepted_job_k      <= job_k;
      accepted_job_a_base <= job_a_base;
      accepted_job_b_base <= job_b_base;
      accepted_job_c_base <= job_c_base;
    end
  end

  logic scheduler_start_sent;

  always_ff @(posedge dut.ui_clk or negedge rstn) begin
    if (!rstn) begin
      scheduler_start_sent <= 1'b0;
    end
    else if (dut.phase != 4'd4) begin
      // P_FOLD = 4.
      // Leaving P_FOLD rearms the next invocation.
      scheduler_start_sent <= 1'b0;
    end
    else if (USE_EXTERNAL_SCHEDULER) begin
      scheduler_start_sent <= 1'b1;
    end
  end

  wire scheduler_fold_start =
      USE_EXTERNAL_SCHEDULER &&
      (dut.phase == 4'd4) &&
      !scheduler_start_sent;

  // EXPECT_WR_CHK is passed in, as dma_top_build.tcl passes it on the board:
  // the design's own growing expectation (want_wr, what led[4] is derived from)
  // is only meaningful against the constant for THIS geometry, and taking the
  // module default would leave the bench checking the K_MAX = 16 constant on a
  // K_MAX = 32 run.
  systolic_dma_top #(
    .N (N), .K_MAX (K_MAX), .K_DIM (K_MAX),
    .BASE_ADDR (0),
    .WB_GAP_BYTES (4096),
    .USE_V2 (USE_V2),
    .EXPECT_WR_CHK (EXPECT_WR_CHK),
    .USE_EXTERNAL_SCHEDULER (USE_EXTERNAL_SCHEDULER)
  ) dut (
    .sys_clk_pin (clk), .cpu_resetn (rstn), .led (led),
    .ddr3_addr (ddr3_addr), .ddr3_ba (ddr3_ba), .ddr3_cas_n (ddr3_cas_n),
    .ddr3_ck_n (ddr3_ck_n), .ddr3_ck_p (ddr3_ck_p), .ddr3_cke (ddr3_cke),
    .ddr3_ras_n (ddr3_ras_n), .ddr3_reset_n (ddr3_reset_n),
    .ddr3_we_n (ddr3_we_n), .ddr3_dq (ddr3_dq),
    .ddr3_dqs_n (ddr3_dqs_n), .ddr3_dqs_p (ddr3_dqs_p),
    .ddr3_dm (ddr3_dm), .ddr3_odt (ddr3_odt),
    .scheduler_fold_start (scheduler_fold_start),
    .scheduler_c_done (scheduler_c_done),

    .job_valid  (job_valid),
    .job_ready  (job_ready),
    .job_id     (job_id),
    .job_m      (job_m),
    .job_n      (job_n),
    .job_k (JOB_K),
    .job_a_base (job_a_base),
    .job_b_base (job_b_base),
    .job_c_base (job_c_base)
  );

  // The memory model's ownership monitor needs the real boundary between the
  // operand image and the result tile, which moves with K_MAX; run_top_sim.sh
  // passes it as +wb_region=<bytes>.  Check that it arrived, or the monitor is
  // silently checking the wrong line.
  initial begin
    if (!$value$plusargs("n_inv=%d", n_inv)) n_inv = 1;
    // ----------------------------------------------------------
    // job-aware verification base
    //
    // Legacy mode reconstructs the historical run-level result
    // region exactly as before.
    //
    // External scheduler mode verifies against the C base carried
    // by the accepted job descriptor.
    // ----------------------------------------------------------
    // ----------------------------------------------------------
    // Job-aware verification base.
    //
    // This initial block executes before the scheduler handshake,
    // so accepted_job_c_base has not been latched yet.
    //
    // The memory model's +wb_region argument must therefore be
    // checked against the descriptor presented by the TB itself.
    // The accepted descriptor is verified separately after
    // job_fire.
    // ----------------------------------------------------------
    if (USE_EXTERNAL_SCHEDULER) begin
      wb_base = job_c_base;
    end
    else begin
      wb_base = n_inv * RX_BYTES + 4096;
    end
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
    if (dut.phase !== phase_d && $test$plusargs("verbose"))
      $display("  phase %0d -> %0d   words=%0d", phase_d, dut.phase,
               dut.words_written);
  end

  // ---- TEMP DEBUG: scheduler handshake -------------------------------
  // ---- TEMP DEBUG: scheduler clock/reset -----------------------------
  integer rst_dbg_count = 0;

  always @(posedge dut.ui_clk) begin
    if (rst_dbg_count < 20) begin
      $display(
        "RSTDBG t=%0t clk=%0b rstn=%0b ui_rst_n=%0b init_calib=%0b phase=%0d",
        $time,
        clk,
        rstn,
        dut.ui_rst_n,
        dut.init_calib_complete,
        dut.phase
      );
      rst_dbg_count = rst_dbg_count + 1;
    end
  end

  integer job_dbg_count = 0;

  always @(posedge dut.ui_clk) begin
    if (job_dbg_count < 20 &&
        (job_valid || job_ready || dut.job_fire || dut.job_active)) begin
      $display(
        "JOBDBG t=%0t phase=%0d valid=%0b ready=%0b fire=%0b busy=%0b active=%0b id=%0d a=0x%0h k=%0d",
        $time,
        dut.phase,
        job_valid,
        job_ready,
        dut.job_fire,
        dut.job_busy,
        dut.job_active,
        dut.job_id_reg,
        dut.job_a_base_reg,
        dut.job_k_reg
      );
      job_dbg_count = job_dbg_count + 1;
    end
  end

  // ----------------------------------------------------------
  // Scheduler interface verification.
  //
  // In external mode the TB itself owns the start pulse.
  // Count the actual pulse seen by the DUT, rather than merely
  // trusting the parameter value.
  //
  // scheduler_c_done is the completion observed at the same
  // interface boundary.
  // ----------------------------------------------------------
  integer scheduler_start_pulses = 0;
  logic scheduler_done_seen = 1'b0;

  always_ff @(posedge clk) begin
    if (scheduler_fold_start)
      scheduler_start_pulses <= scheduler_start_pulses + 1;

    if (scheduler_c_done)
      scheduler_done_seen <= 1'b1;
  end

  // Runtime job K must fit within the compiled hardware capacity.
  initial begin
    if (JOB_K <= 0 || JOB_K > K_MAX) begin
      $display("  FAIL: JOB_K=%0d is outside hardware capacity K_MAX=%0d",
               JOB_K, K_MAX);
      $fatal(1);
    end
  end

  integer errors = 0;
  integer waited = 0;
  // the first run's counters, kept to compare the re-run against
  logic [31:0] f_fill, f_cyc, f_wb, f_span, f_chk, f_words, f_eng;
  integer i, r, c;
  logic [31:0] got, want;

  // ---- TEMP DEBUG: capture seed writer AXI stream --------------------
  integer seed_dbg_count = 0;

  always @(posedge clk) begin
    if (dut.sd_awvalid && dut.sd_awready) begin
      $display("SEED AW addr=0x%0h len=%0d",
               dut.sd_awaddr,
               dut.sd_awlen);
    end

    if (dut.sd_wvalid && dut.sd_wready && seed_dbg_count < 16) begin
      $display("SEED W beat[%0d] data=%032h last=%0b",
               seed_dbg_count,
               dut.sd_wdata,
               dut.sd_wlast);
      seed_dbg_count = seed_dbg_count + 1;
    end
  end

  // ---- TEMP DEBUG: capture first DMA destination beats ----------------
  integer dma_dbg_count = 0;

  always @(posedge clk) begin
    if (dut.dst_wr_en && dma_dbg_count < 16) begin
      $display("DMA beat[%0d] beat=%0d data=%032h",
               dma_dbg_count,
               dut.dst_wr_beat,
               dut.dst_wr_data);
      dma_dbg_count = dma_dbg_count + 1;
    end
  end

  initial begin
    $display("== tb_systolic_dma_top: seed -> DDR -> fold -> C -> DDR  (operand path %0s) ==",
             USE_V2 ? "v2, beat-wide" : "v1, four cycles per beat");    $display(
      "   %0d invocation(s): K_MAX=%0d, JOB_K=%0d; result region at 0x%0h",
      n_inv, K_MAX, JOB_K, wb_base);
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

    // ---- TEMP DEBUG: inspect DMA -> operand-writer stream -------------
    // The checksum mismatch means the writer received a different word
    // stream from seed_ref.py.  Inspect the first 16 AXI beats directly
    // at the dma_engine destination interface.
    $display("---- DEBUG DMA destination stream ----");
    $display("job_rx_bytes=%0d job_n_beats=%0d job_rx_words=%0d",
             dut.job_rx_bytes,
             dut.job_n_beats,
             dut.job_rx_words);
    $display("words_written=%0d chk_wr=%h",
             dut.words_written,
             dut.chk_wr);
    $display("--------------------------------------");

    // ---- TEMP DEBUG: seed writer status -------------------------------
    $display("---- DEBUG seed writer ----");
    $display("seed_start=%0b seed_busy=%0b seed_done=%0b seed_err_align=%0b seed_err_resp=%0b",
             dut.seed_start,
             dut.seed_busy,
             dut.seed_done,
             dut.seed_err_align,
             dut.seed_err_resp);
    $display("slab_addr=0x%0h job_a_base_reg=0x%0h job_k_reg=%0d",
             dut.slab_addr,
             dut.job_a_base_reg,
             dut.job_k_reg);
    $display("---------------------------");

    // ---- TEMP DEBUG: inspect the first 16 words written into DDR ----------
    // For K_MAX=32, JOB_K=16, the external scheduler should read only the
    // first 64 AXI beats = 256 payload words.  The first words must match
    // seed_ref.py's mode-1 stream:
    //   1.0, 2.0, 3.0, ... as FP32 bit patterns.
    $display("---- DEBUG DDR operand region ----");
    for (i = 0; i < 16; i = i + 1) begin
      $display("DDR[%0d] @ 0x%0h = %h",
               i,
               32'h1000 + i*4,
               dut.u_mig_7series_0.mem[32'h1000/4 + i]);
    end
    $display("----------------------------------");

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
             dut.cyc_latched, JOB_K + 2*(N-1) + 95, JOB_K);
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
    // ---- 4. scheduler interface -----------------------------------------
    //
    // Legacy mode must never generate an external start pulse.
    // External mode must generate exactly one pulse and eventually
    // observe the accelerator completion.
    if (USE_EXTERNAL_SCHEDULER) begin
      if (scheduler_start_pulses !== n_inv) begin
        $display("  FAIL: external scheduler generated %0d start pulse(s), want %0d",
                 scheduler_start_pulses, n_inv);
        errors = errors + 1;
      end
      else begin
        $display("  scheduler_fold_start = exactly %0d external start pulse(s)",
                 n_inv);
      end

      if (!scheduler_done_seen) begin
        $display("  FAIL: external scheduler never observed scheduler_c_done");
        errors = errors + 1;
      end
      else begin
        $display("  scheduler_c_done = observed");
      end
    end
    else begin
      if (scheduler_start_pulses !== 0) begin
        $display("  FAIL: legacy mode generated %0d external scheduler pulse(s)",
                 scheduler_start_pulses);
        errors = errors + 1;
      end
    end

    // ---- 6. scheduler job boundary ---------------------------------------
    //
    // A valid external job must be accepted exactly once and the
    // descriptor observed at that handshake must remain stable.
    if (USE_EXTERNAL_SCHEDULER) begin
      if (!job_sent) begin
        $display("  FAIL: external scheduler job was never accepted");
        errors = errors + 1;
      end
      else begin
        $display("  scheduler job = accepted");
      end

      if (dut.job_id_reg !== 32'd100) begin
        $display("  FAIL: job_id_reg = %0d, want 100", dut.job_id_reg);
        errors = errors + 1;
      end

      if (dut.job_m_reg !== 32'd8) begin
        $display("  FAIL: job_m_reg = %0d, want 8", dut.job_m_reg);
        errors = errors + 1;
      end

      if (dut.job_n_reg !== 32'd8) begin
        $display("  FAIL: job_n_reg = %0d, want 8", dut.job_n_reg);
        errors = errors + 1;
      end

      if (dut.job_k_reg !== 32'(JOB_K)) begin
        $display("  FAIL: job_k_reg = %0d, want %0d",
                 dut.job_k_reg, JOB_K);
        errors = errors + 1;
      end

      if (dut.job_a_base_reg !== 64'h0000_0000_0000_1000) begin
        $display("  FAIL: job_a_base_reg = %h, want 1000",
                 dut.job_a_base_reg);
        errors = errors + 1;
      end

      if (dut.job_b_base_reg !== 64'h0000_0000_0000_2000) begin
        $display("  FAIL: job_b_base_reg = %h, want 2000",
                 dut.job_b_base_reg);
        errors = errors + 1;
      end

      if (dut.job_c_base_reg !== 64'h0000_0000_0000_3000) begin
        $display("  FAIL: job_c_base_reg = %h, want 3000",
                 dut.job_c_base_reg);
        errors = errors + 1;
      end
    end

    if (errors != 0) $fatal(1);
    $finish;
  end

endmodule

`default_nettype wire
