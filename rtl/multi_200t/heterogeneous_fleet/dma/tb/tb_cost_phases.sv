`default_nettype none
`timescale 1ns / 1ps

/*
 * tb_cost_phases -- cycle counts of single invocations of systolic_dma_top,
 * phase by phase, for calibrating and validating the compiler's cost model
 * (lib/Systolic/Transforms/CostModel.cpp).  BENCH ONLY: it reads the design
 * through hierarchical references and drives only the top's ports.
 *
 *   run_cost_phases.sh <K_MAX_8X8> <K_MAX_4X4> <jobs.svh> <out_dir> [v1|v2]
 *
 * The job list comes from an include file (`cost_jobs.svh`, written by
 * eval/cost_model_validation/validate.py):
 *
 *   localparam integer NJ = ...;
 *   localparam integer JOB_DEV   [0:NJ-1] = '{...};   device 0 (8x8), 1..3 (4x4)
 *   localparam integer JOB_KDESC [0:NJ-1] = '{...};   descriptor k (what runs)
 *   localparam integer JOB_KLOG  [0:NJ-1] = '{...};   real reduction steps
 *   localparam integer JOB_START [0:NJ-1] = '{...};   descriptor start_cycle
 *   localparam integer JOB_KOFF  [0:NJ-1] = '{...};   first step of this job in its GEMM
 *   localparam integer JOB_KTOT  [0:NJ-1] = '{...};   the GEMM's K
 *
 * A GEMM is a run of consecutive jobs on one device whose JOB_KOFF go 0,
 * klog_0, klog_0 + klog_1, ... up to JOB_KTOT: the K-folding of a K-step
 * reduction into invocations, as the compiler emits it and the runtime runs
 * it (runtime/fpga_matmul_tiled.cpp: one descriptor per invocation, the
 * host adds the invocations' result tiles, C += v).  A job on its own is a
 * GEMM of one invocation (JOB_KOFF = 0, JOB_KTOT = klog).
 *
 * Jobs run ONE AT A TIME, as the runtime issues them: the next descriptor is
 * presented as soon as the previous job's result has been written back and
 * read back (before that the top does not accept it: result_readback_busy).
 * Each job's operand slab is written into the DDR model in the host's wire
 * format (runtime/fpga_matmul_tiled.cpp): ceil(kdesc / 8) windows of 8 steps
 * x 8 lanes; steps < klog carry the GEMM's operands for steps JOB_KOFF ..
 * JOB_KOFF + klog - 1, steps klog .. kdesc-1 are zero (only for a padded
 * descriptor, kdesc > klog: what the runtime sent before variable-K, job.k =
 * K_MAX for a shorter chunk), everything the job must not use (steps >=
 * kdesc, lanes a 4x4 does not have) is large junk.  Every element of every
 * result tile is checked against an exact integer reference: each job's
 * against its own steps, and each GEMM's sum of result tiles against the
 * whole reduction.
 *
 * The reduction steps the hardware executes are counted, not inferred
 * (reduction_probe.svh): per PE the multiply-accumulates
 * (pipe_pair_valid), per array the operands the feeder streams into row 0
 * and column 0, the cycles in ST_FEED and the context's k_dim.  A job whose
 * PEs do not each execute exactly kdesc steps fails.
 *
 * Time stamps, in cycles of the 100 MHz array clock (ui_clk), per job:
 *
 *   fire   descriptor accepted (job_fire)                        -- start
 *   grant  this context's operand descriptor taken by the read engine
 *   fill   operand tile completely landed (context P_READ -> P_GO)
 *   fold   the scheduler's release (fold_start)
 *   cv     the array's result published (c_valid_out)
 *   hand   result handed to the write-back stage (P_FOLD -> P_WB)
 *   wbacc  write-back descriptor taken by the shared write engine
 *   done   last write response: the tile is in DDR (job_done)    -- end
 *   rb     result tile read back from DDR into the DPTI output path
 *
 * Phases the cost model predicts (FPGA-internal, contiguous, summing to
 * done - fire):  read = fold - fire (the fill plus the scheduler's two-cycle
 * release),  compute = cv - fold,  write = done - cv
 * (eval/cost_model_validation/validate.py).  rb - done is the start of the
 * host transfer (DPTI), reported separately; the top takes the context's
 * next job only after it.  JOB_START is the descriptor's start_cycle: 0, or
 * the compiler's values -- the scheduler holds a fold it is not yet due for,
 * which shows as fold - fill > 2.
 */

module tb_cost_phases #(
  parameter integer K_MAX_8X8 = 16,
  parameter integer K_MAX_4X4 = 16,
  parameter integer USE_V2    = 0
);

`include "cost_jobs.svh"

  localparam integer N         = 8;
  localparam integer SLAB_BASE = 32'h0000_4000;   // one slab at a time
  localparam integer C_BASE    = 32'h0000_1000;   // one result tile at a time
  localparam integer MEM_BYTES = 131072;          // mig_axi_model's 32768 words
  localparam integer JUNK      = 4096;
  localparam integer MAX_CYC   = 4_000_000;

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

`include "reduction_probe.svh"

  // ---- binary32 <-> small integers, exactly --------------------------------
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

  // ---- one GEMM's operands and reference -------------------------------------
  localparam integer KMAX_BENCH = ((MEM_BYTES - SLAB_BASE) / 512 - 1) * 8;
  localparam integer KTOT_BENCH = 4096;
  integer      a_val [0:N-1][0:KTOT_BENCH-1];   // A[row][step of the GEMM]
  integer      b_val [0:KTOT_BENCH-1][0:N-1];   // B[step of the GEMM][col]
  longint      c_sum [0:N-1][0:N-1];            // the host's C += v
  logic [31:0] lcg = 32'd20261012;

  function automatic integer rnd_small();   // -8 .. 8
    lcg = lcg * 32'd1664525 + 32'd1013904223;
    return integer'((lcg >> 16) % 17) - 8;
  endfunction

  function automatic integer slab_steps(input integer k);
    return ((k + 7) / 8) * 8;
  endfunction

  // A new GEMM: its K-step operands, and the host's C cleared.
  task automatic make_gemm(input integer ktot);
    begin
      for (int step = 0; step < ktot; step++)
        for (int lane = 0; lane < N; lane++) begin
          a_val[lane][step] = rnd_small();
          b_val[step][lane] = rnd_small();
        end
      for (int r = 0; r < N; r++)
        for (int c = 0; c < N; c++)
          c_sum[r][c] = 0;
    end
  endtask

  // One invocation's slab: the GEMM's steps koff .. koff + klog - 1.
  task automatic make_job(input integer dev, input integer kdesc, input integer klog,
                          input integer koff);
    integer p, base, w, step, lane, v;
    begin
      p    = (dev == 0) ? 8 : 4;
      base = SLAB_BASE / 4;
      for (step = 0; step < slab_steps(kdesc) + 8; step++) begin
        for (lane = 0; lane < 8; lane++) begin
          w = (step / 8) * 128 + lane * 8 + (step % 8);
          if (lane < p && step < klog)
            v = a_val[lane][koff + step];
          else if (lane < p && step < kdesc)
            v = 0;
          else
            v = JUNK + step * 8 + lane;
          dut.u_mig_7series_0.mem[base + w] = int_to_fp32(v);
          w = (step / 8) * 128 + 64 + (step % 8) * 8 + lane;
          if (lane < p && step < klog)
            v = b_val[koff + step][lane];
          else if (lane < p && step < kdesc)
            v = 0;
          else
            v = JUNK + 1000 + step * 8 + lane;
          dut.u_mig_7series_0.mem[base + w] = int_to_fp32(v);
        end
      end
    end
  endtask

  task automatic submit(input integer j, input integer dev, input integer kdesc,
                        input integer start);
    begin
      @(posedge clk);
      job_id          <= 32'(j);
      job_device_id   <= 32'(dev);
      job_m           <= (dev == 0) ? 32'd8 : 32'd4;
      job_n           <= (dev == 0) ? 32'd8 : 32'd4;
      job_k           <= 32'(kdesc);
      job_start_cycle <= 32'(start);
      job_est_cycles  <= 32'd1;
      job_a_base      <= 64'(SLAB_BASE);
      job_b_base      <= 64'(SLAB_BASE);
      job_c_base      <= 64'(C_BASE);
      job_valid       <= 1'b1;
      while (!job_ready) @(posedge clk);
      @(posedge clk);
      job_valid <= 1'b0;
    end
  endtask

  // ---- time stamps of the job in flight --------------------------------------
  integer cyc = 0;
  integer fire_dev = -1;
  integer t_fire = -1, t_grant = -1, t_fill = -1, t_fold = -1, t_cv = -1;
  integer t_hand = -1, t_wbacc = -1, t_done = -1, t_rb = -1;
  integer words_at_done = -1;

  wire fold_now = (fire_dev == 0) ? dut.scheduler_fold_start_8x8[0]
                : (fire_dev >= 1) ? dut.scheduler_fold_start_4x4[fire_dev - 1] : 1'b0;
  wire cv_now   = (fire_dev == 0) ? dut.c_valid_out_8x8[0]
                : (fire_dev >= 1) ? dut.c_valid_out_4x4[fire_dev - 1] : 1'b0;

  always_ff @(posedge clk) begin
    cyc <= cyc + 1;
    if (rstn) begin
      if (dut.job_fire) begin
        fire_dev <= dut.effective_job_device_id;
        t_fire  <= cyc;
        t_grant <= -1; t_fill <= -1; t_fold <= -1; t_cv <= -1;
        t_hand  <= -1; t_wbacc <= -1; t_done <= -1; t_rb <= -1;
        words_at_done <= -1;
      end else if (t_fire >= 0 && fire_dev >= 0) begin
        if (t_grant < 0 && dut.rd_desc_valid[fire_dev] && dut.rd_desc_ready[fire_dev])
          t_grant <= cyc;
        if (t_fill < 0 && dut.ctx_phase[fire_dev] == 4'd3)
          t_fill <= cyc;
        if (t_fold < 0 && fold_now)
          t_fold <= cyc;
        if (t_cv < 0 && t_fold >= 0 && cv_now)
          t_cv <= cyc;
        if (t_hand < 0 && t_cv >= 0 && dut.ctx_phase[fire_dev] == 4'd6)
          t_hand <= cyc;
        if (t_wbacc < 0 && t_cv >= 0 && dut.wb_eng_accept &&
            int'(dut.wb_grant_idx) == fire_dev)
          t_wbacc <= cyc;
        if (t_done < 0 && dut.ctx_job_done[fire_dev]) begin
          t_done        <= cyc;
          words_at_done <= dut.ctx_words_written[fire_dev];
        end
        if (t_rb < 0 && t_done >= 0 && dut.rb_done && int'(dut.rb_owner) == fire_dev)
          t_rb <= cyc;
      end
    end
  end

  // ---- the run ---------------------------------------------------------------
  integer errors = 0;

  // The result tile as it was read back, kept before the next job runs.
  longint c_tile [0:N-1][0:N-1];
  bit     c_ok   [0:N-1][0:N-1];

  task automatic snapshot_result(input integer dev);
    integer p;
    bit     ok;
    begin
      p = (dev == 0) ? 8 : 4;
      for (int r = 0; r < p; r++)
        for (int c = 0; c < p; c++) begin
          c_tile[r][c] = fp32_to_int(dut.u_mig_7series_0.mem[C_BASE / 4 + r * p + c], ok);
          c_ok[r][c]   = ok;
        end
    end
  endtask

  task automatic check_and_report(input integer j, input integer dev,
                                  input integer kdesc, input integer klog,
                                  input integer koff, input integer ktot,
                                  input integer gemm_fire, input integer prev_done,
                                  input integer mac_min, input integer mac_max,
                                  input integer mac_sum, input integer feed_a,
                                  input integer feed_b, input integer feed_cyc,
                                  input integer kdim);
    integer p, r, c, kk, bad, gbad;
    longint want;
    begin
      p   = (dev == 0) ? 8 : 4;
      bad = 0;
      for (r = 0; r < p; r++)
        for (c = 0; c < p; c++) begin
          want = 0;
          for (kk = koff; kk < koff + klog; kk++) want += longint'(a_val[r][kk]) * b_val[kk][c];
          if (!c_ok[r][c] || c_tile[r][c] != want) bad++;
          c_sum[r][c] += c_tile[r][c];
        end
      if (words_at_done != slab_steps(kdesc) * 16) bad++;
      // Exactly kdesc reduction steps, in every PE and into the array.
      if (mac_min != kdesc || mac_max != kdesc || feed_a != kdesc || feed_b != kdesc ||
          kdim != kdesc) bad++;
      if (bad != 0) errors++;
      $display("PHASES job=%0d dev=%0d P=%0d kdesc=%0d klog=%0d koff=%0d ktot=%0d fire=%0d grant=%0d fill=%0d fold=%0d cv=%0d hand=%0d wbacc=%0d done=%0d rb=%0d words=%0d steps_pe_min=%0d steps_pe_max=%0d macs=%0d feed_a=%0d feed_b=%0d feed_cycles=%0d k_dim=%0d gemm_fire=%0d prev_done=%0d result=%s",
               j, dev, p, kdesc, klog, koff, ktot, t_fire, t_grant, t_fill, t_fold, t_cv,
               t_hand, t_wbacc, t_done, t_rb, words_at_done, mac_min, mac_max, mac_sum,
               feed_a, feed_b, feed_cyc, kdim, gemm_fire, prev_done,
               (bad == 0) ? "exact" : "FAIL");
      if (koff + klog == ktot) begin
        gbad = 0;
        for (r = 0; r < p; r++)
          for (c = 0; c < p; c++) begin
            want = 0;
            for (kk = 0; kk < ktot; kk++) want += longint'(a_val[r][kk]) * b_val[kk][c];
            if (c_sum[r][c] != want) gbad++;
          end
        if (gbad != 0) errors++;
        $display("GEMM last_job=%0d dev=%0d P=%0d k=%0d first_fire=%0d last_done=%0d result=%s",
                 j, dev, p, ktot, gemm_fire, t_done, (gbad == 0) ? "exact" : "FAIL");
      end
    end
  endtask

  initial begin
    if (K_MAX_8X8 < 1 || K_MAX_4X4 < 1)
      $fatal(1, "tb_cost_phases: K_MAX must be positive");
    for (int j = 0; j < NJ; j++) begin
      if (JOB_DEV[j] < 0 || JOB_DEV[j] > 3 || JOB_START[j] < 0)
        $fatal(1, "tb_cost_phases: job %0d names device %0d", j, JOB_DEV[j]);
      if (JOB_KDESC[j] < 1 || JOB_KDESC[j] > ((JOB_DEV[j] == 0) ? K_MAX_8X8 : K_MAX_4X4) ||
          JOB_KDESC[j] > KMAX_BENCH || JOB_KLOG[j] < 1 || JOB_KLOG[j] > JOB_KDESC[j])
        $fatal(1, "tb_cost_phases: job %0d k %0d / %0d out of range", j, JOB_KDESC[j], JOB_KLOG[j]);
      // A GEMM starts at step 0 and every later job of it picks up where the
      // previous one stopped, on the same device.
      if (JOB_KOFF[j] < 0 || JOB_KTOT[j] > KTOT_BENCH || JOB_KOFF[j] + JOB_KLOG[j] > JOB_KTOT[j] ||
          (JOB_KOFF[j] > 0 && (j == 0 || JOB_DEV[j - 1] != JOB_DEV[j] ||
                               JOB_KTOT[j - 1] != JOB_KTOT[j] ||
                               JOB_KOFF[j - 1] + JOB_KLOG[j - 1] != JOB_KOFF[j])) ||
          (JOB_KOFF[j] == 0 && j > 0 && JOB_KOFF[j - 1] + JOB_KLOG[j - 1] != JOB_KTOT[j - 1]))
        $fatal(1, "tb_cost_phases: job %0d does not continue a K-folded GEMM (koff %0d, ktot %0d)",
               j, JOB_KOFF[j], JOB_KTOT[j]);
    end
    if (NJ > 0 && JOB_KOFF[NJ - 1] + JOB_KLOG[NJ - 1] != JOB_KTOT[NJ - 1])
      $fatal(1, "tb_cost_phases: the last GEMM is not complete");
    $display("== tb_cost_phases: K_MAX_8X8 = %0d, K_MAX_4X4 = %0d, %s, %0d job(s) ==",
             K_MAX_8X8, K_MAX_4X4, (USE_V2 != 0) ? "v2" : "v1", NJ);
    repeat (20) @(posedge clk);
    rstn = 1'b1;
    repeat (200) @(posedge clk);            // memory calibrated, fleet idle

    begin
      integer gemm_fire, prev_done;
      gemm_fire = -1;
      prev_done = -1;
      for (int j = 0; j < NJ; j++) begin
        integer waited, mn, mx, sum, fa, fb, fc, kd;
        if (JOB_KOFF[j] == 0) begin
          make_gemm(JOB_KTOT[j]);
          prev_done = -1;
        end
        make_job(JOB_DEV[j], JOB_KDESC[j], JOB_KLOG[j], JOB_KOFF[j]);
        submit(j, JOB_DEV[j], JOB_KDESC[j], JOB_START[j]);
        waited = 0;
        while ((t_rb < 0 || t_done < 0) && waited < MAX_CYC) begin
          @(posedge clk);
          waited++;
        end
        if (t_rb < 0) begin
          $display("  FAIL: job %0d did not complete within %0d cycles", j, MAX_CYC);
          errors++;
          break;
        end
        if (JOB_KOFF[j] == 0) gemm_fire = t_fire;
        // Taken the cycle the result is read back, before the next job runs:
        // the result tile, and the counters of this job.
        snapshot_result(JOB_DEV[j]);
        rp_summary(JOB_DEV[j], mn, mx, sum);
        fa = rp_feed_a[JOB_DEV[j]];
        fb = rp_feed_b[JOB_DEV[j]];
        fc = rp_feed_cyc[JOB_DEV[j]];
        kd = rp_kdim[JOB_DEV[j]];
        check_and_report(j, JOB_DEV[j], JOB_KDESC[j], JOB_KLOG[j], JOB_KOFF[j], JOB_KTOT[j],
                         gemm_fire, prev_done, mn, mx, sum, fa, fb, fc, kd);
        prev_done = t_done;
        // The next invocation of a GEMM goes out at once, as the runtime's
        // next descriptor would; a new GEMM after a short pause.
        if (j + 1 < NJ && JOB_KOFF[j + 1] == 0) repeat (16) @(posedge clk);
      end
    end

    if (dut.any_err !== 1'b0 || dut.u_mig_7series_0.errors != 0) begin
      $display("  FAIL: any_err %0b, memory-model errors %0d", dut.any_err, dut.u_mig_7series_0.errors);
      errors++;
    end
    $display("== %s: %0d job(s), %0d error(s) ==", (errors == 0) ? "PASS" : "FAIL", NJ, errors);
    if (errors != 0) $fatal(1);
    $finish;
  end

endmodule

`default_nettype wire
