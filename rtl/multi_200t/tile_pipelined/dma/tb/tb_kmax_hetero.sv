`default_nettype none
`timescale 1ns / 1ps

/*
 * tb_kmax_hetero -- systolic_dma_top at independent K_MAX_8X8 / K_MAX_4X4,
 * end to end, with IEEE binary32 arithmetic.  BENCH ONLY.
 *
 *   run_kmax_sim.sh <K_MAX_8X8> <K_MAX_4X4> [v1|v2]
 *
 * The regression benches (tb_systolic_dma_top) check the operand path with
 * checksums tabulated for k = 16 and 32 and run the arithmetic through
 * tb/fp_model.sv, an integer model; neither can say whether a 17-deep 8x8
 * next to 7-deep 4x4s computes the right numbers.  This bench builds the real
 * top -- external scheduler, only the Xilinx IP replaced -- with fp_ieee.sv
 * (the co-simulation's binary32 model, latencies 8 / 11) and checks every
 * element of every result tile against an exact integer reference.  The
 * operand path is v1 (USE_V2 = 0: the archived bitstream's and the
 * co-simulation's, any K_MAX) unless USE_V2 = 1 selects the beat-wide v2 path,
 * whose K_MAX must be a multiple of 8 and at least 16.
 *
 *
 *   wave 1  the four arrays computing at once, at their own K_MAX: one job
 *           per accelerator, all four submitted before the memory model has
 *           finished calibrating, so the four contexts start their operand
 *           fills in the same cycle and the read engine interleaves them beat
 *           by beat (the method of tb_systolic_dma_top's
 *           hetero_full_concurrent).  Every job moves the same number of
 *           windows, W = min(ceil(K_MAX_8X8 / 8), ceil(K_MAX_4X4 / 8)), so the
 *           fills also end together: the 8x8 at k = min(K_MAX_8X8, 8W), 4x4 #0
 *           at min(K_MAX_4X4, 8W), #1 at the shallowest k with W windows, #2
 *           one below #0.  The four operand descriptors must be granted in
 *           the same cycle and all four COMPUTE intervals must overlap.
 *   wave 2  the depth extremes, submitted back to back once memory is up: the
 *           8x8 at k = K_MAX_8X8, 4x4 #0 at K_MAX_4X4, #1 at 1, #2 at
 *           K_MAX_4X4 - 1.  All four must be accepted before any of them
 *           completes -- each context holds a job of its own depth at the
 *           same time.  Their COMPUTE intervals are printed but not required
 *           to overlap: a fill that starts while its slot is alone on the
 *           read channel goes out as 16-beat bursts and fills that arrive a
 *           few cycles later queue behind them (dma_engine_multi, not
 *           changed for K_MAX: four k = 16 jobs submitted this way fill
 *           cycle for cycle the same on the golden revision, the 8x8 about
 *           190 cycles ahead of the 4x4s), and fills of different window
 *           counts end at different times.  So whether these four compute at
 *           once depends on submission timing and on k, not on K_MAX; wave 1
 *           is the concurrency check.
 *   wave 3  the 8x8 again at a smaller k (its buffers still hold the deeper
 *           job's operands); a reduction of 2 * K_MAX_4X4 + 1 on 4x4 #0 as
 *           three chained invocations (k = K_MAX_4X4, K_MAX_4X4, 1) whose
 *           result tiles this bench adds -- the partial sums the runtime
 *           accumulates; 4x4 #1 and #2 at k = K_MAX_4X4 and 1 alongside
 *   wave 4  descriptors whose k the named accelerator cannot hold (K_MAX + 1
 *           on a 4x4, K_MAX + 1 on the 8x8, 0): each must be seen by the
 *           acceptance logic with job_k_ok = 0 and never accepted, and
 *           err_job_k must latch (it is sticky, so the per-descriptor
 *           evidence is job_k_ok); a valid job afterwards still runs
 *
 * Every operand slab is written into the DDR model in the host's wire format
 * (ceil(k / 8) windows of 8 steps, 8 lanes), and everything the job must NOT
 * use is filled with large junk instead of zero: steps k .. end of the last
 * window, the next window, and lanes 4..7 for a 4x4.  A result that picked up
 * any of it is off by thousands, not by rounding.  words_written must be
 * ceil(k / 8) * 8 * 16 (whole windows, nothing more).
 */

module tb_kmax_hetero #(
  parameter integer K_MAX_8X8 = 16,
  parameter integer K_MAX_4X4 = 16,
  parameter integer USE_V2    = 0
);

  localparam integer N = 8;
  localparam integer MAX_JOBS = 16;
  // Operand slabs of job j at SLAB_BASE + j * SLAB_STRIDE, result tiles at
  // C_BASE + j * 256: all inside mig_axi_model's 128 KiB, above wb_region.
  localparam integer SLAB_BASE   = 32'h0000_8000;
  localparam integer SLAB_STRIDE = 4096;
  localparam integer C_BASE      = 32'h0000_1000;
  localparam integer MAX_CYC     = 400_000;
  localparam integer JUNK        = 4096;   // operands nothing may read

  logic clk = 1'b0, dpti_clkout = 1'b0, rstn = 1'b0;
  always #5     clk = ~clk;
  always #8.333 dpti_clkout = ~dpti_clkout;

  logic        job_valid = 1'b0;
  wire         job_ready;
  logic [31:0] job_id = 0, job_device_id = 0, job_m = 0, job_n = 0, job_k = 0;
  logic [31:0] job_start_cycle = 0, job_est_cycles = 0;
  logic [63:0] job_a_base = 0, job_b_base = 0, job_c_base = 0;

  wire [7:0]  led;
  wire [14:0] ddr3_addr;  wire [2:0] ddr3_ba;
  wire        ddr3_cas_n, ddr3_ras_n, ddr3_reset_n, ddr3_we_n;
  wire [0:0]  ddr3_ck_n, ddr3_ck_p, ddr3_cke, ddr3_odt;
  wire [1:0]  ddr3_dm;
  wire [15:0] ddr3_dq;    wire [1:0] ddr3_dqs_n, ddr3_dqs_p;
  wire [7:0]  dpti_d;
  wire        dpti_rd_n, dpti_wr_n, dpti_oe_n;

  systolic_dma_top #(
    .N (N), .K_MAX_8X8 (K_MAX_8X8), .K_MAX_4X4 (K_MAX_4X4), .K_DIM (K_MAX_8X8),
    .NUM_8X8 (1), .NUM_4X4 (3),
    .USE_V2 (USE_V2 != 0), .USE_EXTERNAL_SCHEDULER (1'b1), .USE_LEGACY_JOB_PORTS (1'b1)
  ) dut (
    .sys_clk_pin (clk), .cpu_resetn (rstn), .led (led),
    .ddr3_addr (ddr3_addr), .ddr3_ba (ddr3_ba), .ddr3_cas_n (ddr3_cas_n),
    .ddr3_ck_n (ddr3_ck_n), .ddr3_ck_p (ddr3_ck_p), .ddr3_cke (ddr3_cke),
    .ddr3_ras_n (ddr3_ras_n), .ddr3_reset_n (ddr3_reset_n),
    .ddr3_we_n (ddr3_we_n), .ddr3_dq (ddr3_dq),
    .ddr3_dqs_n (ddr3_dqs_n), .ddr3_dqs_p (ddr3_dqs_p),
    .ddr3_dm (ddr3_dm), .ddr3_odt (ddr3_odt),
    .scheduler_fold_start (1'b0), .scheduler_c_done (),
    .job_valid (job_valid), .job_ready (job_ready), .job_id (job_id),
    .job_device_id (job_device_id), .job_m (job_m), .job_n (job_n),
    .job_k (job_k), .job_start_cycle (job_start_cycle),
    .job_est_cycles (job_est_cycles), .job_a_base (job_a_base),
    .job_b_base (job_b_base), .job_c_base (job_c_base),
    .dpti_wr_valid (1'b0), .dpti_wr_ready (), .dpti_wr_addr (8'd0),
    .dpti_wr_data (32'd0),
    .dpti_d (dpti_d), .dpti_rxf_n (1'b1), .dpti_txe_n (1'b0),
    .dpti_clkout (dpti_clkout),
    .dpti_rd_n (dpti_rd_n), .dpti_wr_n (dpti_wr_n), .dpti_oe_n (dpti_oe_n),
    .dpti_siwun (1'b1)
  );

  // ---- binary32 <-> small integers, exactly ----------------------------
  function automatic logic [31:0] int_to_fp32(input integer v);
    integer m, e;
    logic   s;
    begin
      if (v == 0) return 32'h0;
      s = (v < 0);
      m = s ? -v : v;
      e = 0;
      for (int b = 1; b < 31; b++) if ((m >> b) != 0) e = b;
      return {s, 8'(127 + e), 23'((m << (23 - e)) & 32'h007F_FFFF)};
    end
  endfunction

  // Integer value of an integral binary32 (|v| < 2^31); 'x' if it is not one.
  function automatic longint fp32_to_int(input logic [31:0] f, output bit ok);
    integer e;
    longint m;
    begin
      ok = 1'b1;
      if (f[30:0] == 0) return 0;
      e = int'(f[30:23]) - 127;
      if (e < 0 || e > 30) begin ok = 1'b0; return 0; end
      m = longint'({1'b1, f[22:0]});
      if (e < 23) begin
        if ((m & ((longint'(1) << (23 - e)) - 1)) != 0) ok = 1'b0;
        m = m >> (23 - e);
      end else m = m << (e - 23);
      return f[31] ? -m : m;
    end
  endfunction

  // ---- jobs ---------------------------------------------------------------
  integer  j_dev [0:MAX_JOBS-1];
  integer  j_k   [0:MAX_JOBS-1];
  integer  j_a   [0:MAX_JOBS-1][0:N-1][0:63];   // A[row][step], k <= 64
  integer  j_b   [0:MAX_JOBS-1][0:63][0:N-1];   // B[step][col]
  integer  n_jobs = 0;
  integer  errors = 0;
  logic [31:0] lcg = 32'd20261012;

  function automatic integer rnd_small();   // -8 .. 8
    lcg = lcg * 32'd1664525 + 32'd1013904223;
    return integer'((lcg >> 16) % 17) - 8;
  endfunction

  function automatic integer slab_steps(input integer k);
    return ((k + 7) / 8) * 8;
  endfunction

  // Write job j's operand slab into DDR in the wire format.  Steps beyond k,
  // the next window, and lanes beyond the array are junk.
  task automatic make_job(input integer j, input integer dev, input integer k);
    integer p, base, w, step, lane, v;
    begin
      p = (dev == 0) ? 8 : 4;
      j_dev[j] = dev;
      j_k[j]   = k;
      base     = (SLAB_BASE + j * SLAB_STRIDE) / 4;
      for (step = 0; step < slab_steps(k) + 8; step++) begin
        for (lane = 0; lane < 8; lane++) begin
          // A[lane][step]
          w = (step / 8) * 128 + lane * 8 + (step % 8);
          if (step < k && lane < p) begin
            v = rnd_small();
            j_a[j][lane][step] = v;
          end else
            v = JUNK + step * 8 + lane;
          dut.u_mig_7series_0.mem[base + w] = int_to_fp32(v);
          // B[step][lane]
          w = (step / 8) * 128 + 64 + (step % 8) * 8 + lane;
          if (step < k && lane < p) begin
            v = rnd_small();
            j_b[j][step][lane] = v;
          end else
            v = JUNK + 1000 + step * 8 + lane;
          dut.u_mig_7series_0.mem[base + w] = int_to_fp32(v);
        end
      end
    end
  endtask

  // The legacy compiler-facing port: valid until the registered ready.
  task automatic submit(input integer j);
    begin
      @(posedge clk);
      job_id          <= 32'(j);
      job_device_id   <= 32'(j_dev[j]);
      job_m           <= (j_dev[j] == 0) ? 32'd8 : 32'd4;
      job_n           <= (j_dev[j] == 0) ? 32'd8 : 32'd4;
      job_k           <= 32'(j_k[j]);
      job_start_cycle <= 32'd0;
      job_est_cycles  <= 32'd0;
      job_a_base      <= 64'(SLAB_BASE + j * SLAB_STRIDE);
      job_b_base      <= 64'(SLAB_BASE + j * SLAB_STRIDE);
      job_c_base      <= 64'(C_BASE + j * 256);
      job_valid       <= 1'b1;
      while (!job_ready) @(posedge clk);
      @(posedge clk);
      job_valid <= 1'b0;
    end
  endtask

  // ---- per-context completion, words; per-wave timeline -------------------
  // For waves 1 and 2 (wave = the stimulus' current wave), the first of each
  // per context: descriptor acceptance (job_fire), operand descriptor granted
  // by the read engine, COMPUTE interval (first operand valid into the array
  // .. its c_valid_out, the boundary signals hetero_full_concurrent uses), and
  // job completion.
  integer done_cnt [0:3];
  integer words_at_done [0:3];
  integer cyc = 0;
  integer wave = 0;
  integer acc_cyc [1:2][0:3], grant_cyc [1:2][0:3], cstart [1:2][0:3], cdone [1:2][0:3];
  integer jdone_cyc [1:2][0:3];
  initial begin
    for (int q = 0; q < 4; q++) begin
      done_cnt[q] = 0; words_at_done[q] = -1;
    end
    for (int w = 1; w <= 2; w++)
      for (int q = 0; q < 4; q++) begin
        acc_cyc[w][q] = -1; grant_cyc[w][q] = -1; cstart[w][q] = -1; cdone[w][q] = -1;
        jdone_cyc[w][q] = -1;
      end
  end

  always_ff @(posedge clk) begin
    cyc <= cyc + 1;
    if (rstn) begin
      for (int q = 0; q < 4; q++)
        if (dut.ctx_job_done[q]) begin
          done_cnt[q]      <= done_cnt[q] + 1;
          words_at_done[q] <= dut.ctx_words_written[q];
        end
      if (wave == 1 || wave == 2) begin
        if (dut.job_fire && dut.effective_job_device_id < 4 &&
            acc_cyc[wave][dut.effective_job_device_id] < 0)
          acc_cyc[wave][dut.effective_job_device_id] <= cyc;
        for (int q = 0; q < 4; q++) begin
          if (grant_cyc[wave][q] < 0 && dut.rd_desc_valid[q] && dut.rd_desc_ready[q])
            grant_cyc[wave][q] <= cyc;
          if (jdone_cyc[wave][q] < 0 && dut.ctx_job_done[q])
            jdone_cyc[wave][q] <= cyc;
        end
        if (cstart[wave][0] < 0 && (dut.a_valid_to_8x8[0][0] || dut.b_valid_to_8x8[0][0]))
          cstart[wave][0] <= cyc;
        if (cdone[wave][0] < 0 && cstart[wave][0] >= 0 && dut.c_valid_out_8x8[0])
          cdone[wave][0] <= cyc;
        for (int q = 0; q < 3; q++) begin
          if (cstart[wave][1+q] < 0 && (dut.a_valid_to_4x4[q][0] || dut.b_valid_to_4x4[q][0]))
            cstart[wave][1+q] <= cyc;
          if (cdone[wave][1+q] < 0 && cstart[wave][1+q] >= 0 && dut.c_valid_out_4x4[q])
            cdone[wave][1+q] <= cyc;
        end
      end
    end
  end

  task automatic print_compute(input integer w);
    $display("  COMPUTE 8x8 [%0d, %0d]  4x4#0 [%0d, %0d]  4x4#1 [%0d, %0d]  4x4#2 [%0d, %0d]",
             cstart[w][0], cdone[w][0], cstart[w][1], cdone[w][1],
             cstart[w][2], cdone[w][2], cstart[w][3], cdone[w][3]);
  endtask

  task automatic wait_done(input integer q, input integer target);
    integer waited;
    begin
      waited = 0;
      while (done_cnt[q] < target && waited < MAX_CYC) begin
        @(posedge clk);
        waited++;
      end
      if (done_cnt[q] < target) begin
        $display("  FAIL: device %0d: %0d of %0d jobs done after %0d cycles (phase %0d)",
                 q, done_cnt[q], target, MAX_CYC, dut.ctx_phase[q]);
        errors++;
      end
      repeat (4) @(posedge clk);
    end
  endtask

  // Compare job j's result tile with the reference; acc_* chain partial sums.
  longint acc [0:N-1][0:N-1];

  task automatic check_job(input integer j, input bit accumulate, input bit last,
                           input integer ref_first, input integer ref_last);
    integer p, r, c, kk, jj, bad, q;
    longint want, got;
    bit     ok;
    begin
      p = (j_dev[j] == 0) ? 8 : 4;
      q = j_dev[j];
      bad = 0;
      if (words_at_done[q] != slab_steps(j_k[j]) * 16) begin
        $display("  FAIL: job %0d: device %0d wrote %0d operand words, want %0d (ceil(%0d/8) windows)",
                 j, q, words_at_done[q], slab_steps(j_k[j]) * 16, j_k[j]);
        errors++;
      end
      for (r = 0; r < p; r++)
        for (c = 0; c < p; c++) begin
          got = fp32_to_int(dut.u_mig_7series_0.mem[(C_BASE + j * 256) / 4 + r * p + c], ok);
          if (!ok) begin
            if (bad < 4) $display("  FAIL: job %0d C[%0d][%0d] = %h is not an integer",
                                  j, r, c, dut.u_mig_7series_0.mem[(C_BASE + j * 256) / 4 + r * p + c]);
            bad++;
            continue;
          end
          if (!accumulate) begin
            want = 0;
            for (kk = 0; kk < j_k[j]; kk++) want += longint'(j_a[j][r][kk]) * j_b[j][kk][c];
            if (got != want) begin
              if (bad < 4) $display("  FAIL: job %0d (device %0d, k=%0d) C[%0d][%0d] = %0d, reference %0d",
                                    j, q, j_k[j], r, c, got, want);
              bad++;
            end
          end else begin
            acc[r][c] = (j == ref_first) ? got : acc[r][c] + got;
            if (last) begin
              want = 0;
              for (jj = ref_first; jj <= ref_last; jj++)
                for (kk = 0; kk < j_k[jj]; kk++)
                  want += longint'(j_a[jj][r][kk]) * j_b[jj][kk][c];
              if (acc[r][c] != want) begin
                if (bad < 4) $display("  FAIL: chained jobs %0d..%0d C[%0d][%0d] = %0d, reference %0d",
                                      ref_first, ref_last, r, c, acc[r][c], want);
                bad++;
              end
            end
          end
        end
      errors += bad;
      if (!accumulate)
        $display("  job %0d  device %0d (%0dx%0d)  k = %2d  %0d words  %s", j, q, p, p,
                 j_k[j], words_at_done[q], bad ? "FAIL" : "exact");
      else
        $display("  job %0d  device %0d (%0dx%0d)  k = %2d  %0d words  partial sum %0d of jobs %0d..%0d%s",
                 j, q, p, p, j_k[j], words_at_done[q], j - ref_first + 1, ref_first, ref_last,
                 last ? (bad ? "  FAIL" : "  -> exact") : "");
    end
  endtask

  // A descriptor that must never run: the refusal path takes it off the job
  // interface (job_ready, so the producer does not hang -- it is reported
  // to the host as a completion record with status 1, docs/ASYNC.md 2.4)
  // but no context ever fires it, and err_job_k is latched.
  task automatic expect_refused(input integer dev, input integer k);
    integer waited;
    bit     fired, consumed, seen;
    begin
      @(posedge clk);
      job_id <= 32'hBAD0 + dev; job_device_id <= 32'(dev); job_k <= 32'(k);
      job_m <= 32'd4; job_n <= 32'd4;
      job_a_base <= 64'(SLAB_BASE); job_b_base <= 64'(SLAB_BASE);
      job_c_base <= 64'(C_BASE + 15 * 256);
      job_valid <= 1'b1;
      fired    = 1'b0;
      consumed = 1'b0;
      seen     = 1'b0;
      for (waited = 0; waited < 3000; waited++) begin
        @(posedge clk);
        if (dut.job_fire) fired = 1'b1;
        // The handshake, as submit() does it: the descriptor is taken (by
        // the refusal path) at the edge after job_ready is seen, and the
        // producer withdraws it then.
        if (job_valid && job_ready) begin
          consumed = 1'b1;
          @(posedge clk);
          job_valid <= 1'b0;
        end
        // This descriptor, judged by the acceptance logic: k outside 1..K_MAX.
        if (dut.effective_job_valid && dut.target_valid && !dut.job_k_ok &&
            dut.effective_job_device_id == 32'(dev) && dut.effective_job_k == 32'(k))
          seen = 1'b1;
      end
      job_valid <= 1'b0;
      @(posedge clk);
      if (fired || !consumed || !seen || dut.err_job_k !== 1'b1 || dut.any_err !== 1'b1) begin
        $display("  FAIL: device %0d k = %0d: fired %0b, taken by the refusal path %0b, judged out of range %0b, err_job_k %0b, any_err %0b (want 0, 1, 1, 1, 1)",
                 dev, k, fired, consumed, seen, dut.err_job_k, dut.any_err);
        errors++;
      end else
        $display("  device %0d k = %0d: refused (job_k_ok = 0, taken by the refusal path, never fired), err_job_k latched", dev, k);
    end
  endtask

  integer k8_small, k4_less, ov_lo, ov_hi, base_done [0:3];
  integer win_w, k_lo, k8_w, k40_w, k42_w, acc_last, done_first;

  initial begin
    if (K_MAX_8X8 < 1 || K_MAX_4X4 < 1 || K_MAX_8X8 > 56 || K_MAX_4X4 > 56)
      $fatal(1, "tb_kmax_hetero: K_MAX_8X8 / K_MAX_4X4 must be 1..56 (its address map)");
    if (USE_V2 != 0 && (K_MAX_8X8 % 8 != 0 || K_MAX_4X4 % 8 != 0 || K_MAX_8X8 < 16 || K_MAX_4X4 < 16))
      $fatal(1, "tb_kmax_hetero: the v2 operand path needs K_MAX multiples of 8, at least 16");

    $display("== tb_kmax_hetero: K_MAX_8X8 = %0d, K_MAX_4X4 = %0d, %s operand path, binary32 arithmetic ==",
             K_MAX_8X8, K_MAX_4X4, (USE_V2 != 0) ? "v2" : "v1");
    // Operands go into the memory model after its own time-0 initialisation.
    repeat (2) @(posedge clk);

    // ---- wave 1: the four arrays at once, each at its own K_MAX ------------
    // Same number of windows for every job, so the four interleaved fills
    // also end together.
    win_w = (K_MAX_8X8 + 7) / 8;
    if ((K_MAX_4X4 + 7) / 8 < win_w) win_w = (K_MAX_4X4 + 7) / 8;
    k_lo  = 8 * (win_w - 1) + 1;                               // shallowest k, W windows
    k8_w  = (K_MAX_8X8 < 8 * win_w) ? K_MAX_8X8 : 8 * win_w;
    k40_w = (K_MAX_4X4 < 8 * win_w) ? K_MAX_4X4 : 8 * win_w;
    k42_w = (k40_w - 1 > k_lo) ? k40_w - 1 : k_lo;
    make_job(0, 0, k8_w);
    make_job(1, 1, k40_w);
    make_job(2, 2, k_lo);
    make_job(3, 3, k42_w);
    repeat (18) @(posedge clk);
    rstn = 1'b1;
    // Submitted at once: the contexts wait in P_CALIB for the memory, then
    // all four leave it -- and request their operands -- in the same cycle.
    wave = 1;
    $display("-- wave 1: all four arrays at once, %0d window(s) each: k = %0d / %0d / %0d / %0d",
             win_w, k8_w, k40_w, k_lo, k42_w);
    for (int j = 0; j < 4; j++) submit(j);
    for (int q = 0; q < 4; q++) wait_done(q, 1);
    for (int j = 0; j < 4; j++) check_job(j, 1'b0, 1'b0, 0, 0);
    print_compute(1);
    if (grant_cyc[1][0] < 0 || grant_cyc[1][1] != grant_cyc[1][0] ||
        grant_cyc[1][2] != grant_cyc[1][0] || grant_cyc[1][3] != grant_cyc[1][0]) begin
      $display("  FAIL: the four operand descriptors were not granted in the same cycle (%0d %0d %0d %0d)",
               grant_cyc[1][0], grant_cyc[1][1], grant_cyc[1][2], grant_cyc[1][3]);
      errors++;
    end
    ov_lo = cstart[1][0]; ov_hi = cdone[1][0];
    for (int q = 1; q < 4; q++) begin
      if (cstart[1][q] > ov_lo) ov_lo = cstart[1][q];
      if (cdone[1][q] < ov_hi)  ov_hi = cdone[1][q];
    end
    if (cstart[1][0] >= 0 && cstart[1][1] >= 0 && cstart[1][2] >= 0 && cstart[1][3] >= 0 &&
        ov_lo < ov_hi)
      $display("  operand descriptors granted together at %0d; four-way COMPUTE overlap [%0d, %0d] = %0d cycles",
               grant_cyc[1][0], ov_lo, ov_hi, ov_hi - ov_lo);
    else begin
      $display("  FAIL: the four arrays did not compute concurrently (max start %0d, min done %0d)",
               ov_lo, ov_hi);
      errors++;
    end
    if (dut.any_err !== 1'b0) begin
      $display("  FAIL: any_err latched in wave 1");
      errors++;
    end

    // ---- wave 2: the depth extremes, back to back once memory is up --------
    k4_less = (K_MAX_4X4 > 1) ? K_MAX_4X4 - 1 : 1;
    make_job(4, 0, K_MAX_8X8);
    make_job(5, 1, K_MAX_4X4);
    make_job(6, 2, 1);
    make_job(7, 3, k4_less);
    for (int q = 0; q < 4; q++) base_done[q] = done_cnt[q];
    wave = 2;
    $display("-- wave 2: k = %0d (8x8) / %0d / 1 / %0d (4x4s), submitted back to back",
             K_MAX_8X8, K_MAX_4X4, k4_less);
    for (int j = 4; j < 8; j++) submit(j);
    for (int q = 0; q < 4; q++) wait_done(q, base_done[q] + 1);
    for (int j = 4; j < 8; j++) check_job(j, 1'b0, 1'b0, 0, 0);
    print_compute(2);
    acc_last = acc_cyc[2][0]; done_first = jdone_cyc[2][0];
    for (int q = 1; q < 4; q++) begin
      if (acc_cyc[2][q] > acc_last) acc_last = acc_cyc[2][q];
      if (jdone_cyc[2][q] < done_first) done_first = jdone_cyc[2][q];
    end
    if (acc_cyc[2][0] >= 0 && acc_cyc[2][1] >= 0 && acc_cyc[2][2] >= 0 && acc_cyc[2][3] >= 0 &&
        jdone_cyc[2][0] >= 0 && jdone_cyc[2][1] >= 0 && jdone_cyc[2][2] >= 0 && jdone_cyc[2][3] >= 0 &&
        acc_last < done_first)
      $display("  all four jobs held at once: accepted by %0d, first completion at %0d",
               acc_last, done_first);
    else begin
      $display("  FAIL: the four jobs were not held at once (last acceptance %0d, first completion %0d)",
               acc_last, done_first);
      errors++;
    end
    if (dut.any_err !== 1'b0) begin
      $display("  FAIL: any_err latched in wave 2");
      errors++;
    end
    wave = 0;

    // ---- wave 3: smaller k after a deeper one; a chained reduction ---------
    k8_small = (K_MAX_8X8 > 1) ? K_MAX_8X8 / 2 + 1 : 1;
    if (k8_small > K_MAX_8X8) k8_small = K_MAX_8X8;
    make_job(8, 0, k8_small);
    make_job(9, 1, K_MAX_4X4);       // chained: 9, 10, 11 = one reduction of
    make_job(10, 1, K_MAX_4X4);      //   2 * K_MAX_4X4 + 1 steps
    make_job(11, 1, 1);
    make_job(12, 2, K_MAX_4X4);
    make_job(13, 3, 1);
    for (int q = 0; q < 4; q++) base_done[q] = done_cnt[q];
    $display("-- wave 3: device 0 at k = %0d after k = %0d; a %0d-step reduction on device 1 in three invocations",
             k8_small, K_MAX_8X8, 2 * K_MAX_4X4 + 1);
    submit(8);
    submit(12);
    submit(13);
    submit(9);
    wait_done(1, base_done[1] + 1);
    check_job(9, 1'b1, 1'b0, 9, 11);
    submit(10);
    wait_done(1, base_done[1] + 2);
    check_job(10, 1'b1, 1'b0, 9, 11);
    submit(11);
    wait_done(1, base_done[1] + 3);
    check_job(11, 1'b1, 1'b1, 9, 11);
    wait_done(0, base_done[0] + 1);
    wait_done(2, base_done[2] + 1);
    wait_done(3, base_done[3] + 1);
    check_job(8, 1'b0, 1'b0, 0, 0);
    check_job(12, 1'b0, 1'b0, 0, 0);
    check_job(13, 1'b0, 1'b0, 0, 0);
    if (dut.any_err !== 1'b0) begin
      $display("  FAIL: any_err latched in wave 3");
      errors++;
    end

    // ---- wave 4: k the accelerator cannot hold ----------------------------
    $display("-- wave 4: descriptors outside 1..K_MAX of the named accelerator");
    expect_refused(1, K_MAX_4X4 + 1);
    expect_refused(0, K_MAX_8X8 + 1);
    expect_refused(2, 0);
    make_job(14, 1, K_MAX_4X4);
    base_done[1] = done_cnt[1];
    submit(14);
    wait_done(1, base_done[1] + 1);
    check_job(14, 1'b0, 1'b0, 0, 0);

    if (dut.err_w_owner !== 1'b0 || dut.u_mig_7series_0.errors != 0) begin
      $display("  FAIL: err_w_owner %0b, memory-model errors %0d",
               dut.err_w_owner, dut.u_mig_7series_0.errors);
      errors++;
    end

    $display("== %s: K_MAX_8X8 = %0d, K_MAX_4X4 = %0d, %s, %0d error(s) ==",
             (errors == 0) ? "PASS" : "FAIL", K_MAX_8X8, K_MAX_4X4,
             (USE_V2 != 0) ? "v2" : "v1", errors);
    if (errors != 0) $fatal(1);
    $finish;
  end

endmodule

`default_nettype wire
