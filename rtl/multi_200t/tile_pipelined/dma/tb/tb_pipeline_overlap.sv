`default_nettype none
`timescale 1ns / 1ps

/*
 * tb_pipeline_overlap -- consecutive invocations of ONE accelerator, offered
 * as fast as the hardware takes them, stamped stage by stage.  BENCH ONLY: it
 * reads the design through hierarchical references and drives only the top's
 * ports.  Companion of tb_cost_phases.sv (one job at a time, as the
 * synchronous runtime issues them).
 *
 *   run_pipeline_sim.sh <K_MAX_8X8> <K_MAX_4X4> <jobs.svh> <out_dir> [v1|v2] [plusargs...]
 *
 * The job list has tb_cost_phases' format (`pipe_jobs.svh`: NJ, JOB_DEV,
 * JOB_KDESC, JOB_KLOG, JOB_START, JOB_KOFF, JOB_KTOT); a GEMM is a run of
 * consecutive jobs on one device whose JOB_KOFF go 0, k_0, k_0 + k_1, ... up
 * to JOB_KTOT -- the compiler's K-folding of one output tile, whose result
 * tiles the host adds.  Every job has its own operand slab (SLAB_BASE + j *
 * SLAB_STRIDE, in the wire format, everything the job must not use filled
 * with large junk) and its own result tile (C_BASE + j * 256).
 *
 * Submission: job j+1 is offered the cycle after job j is accepted, and the
 * design alone decides when it is taken.  With +serial=1 the next job is
 * offered only once the previous one's result has been read back (the
 * synchronous runtime's order), which gives the serialized reference on the
 * same bench.
 *
 * What is checked: every element of every result tile against an exact
 * integer reference (JUNK in a result is off by thousands); every GEMM's sum
 * of tiles against the whole reduction; the operand words that landed per job
 * (ceil(k / 8) windows, nothing more); the multiply-accumulates every PE of
 * the device executed per job (= k), counted fold to fold; one c_valid_out per
 * fold per device (two tiles summed by a PE that missed the boundary would
 * publish once); the idle gap PE(0, 0) saw between consecutive tiles, which
 * must be >= the PE's boundary gap (MUL + ADD + 2 cycles, 21 with the IP's
 * 8 / 11 latencies that fp_ieee.sv models); no design or memory-model error.
 *
 * What is stamped per job, in cycles of the 100 MHz array clock:
 *   fire    descriptor accepted (job_fire)
 *   grant   this device's operand descriptor taken by the read engine
 *   fill    the job's last operand beat landed
 *   fold    the scheduler's release (fold_start)
 *   feed    the last cycle the feeder drove an operand into row 0 / column 0
 *   cv      the array's result published (c_valid_out)
 *   wbacc   write-back descriptor taken by the shared write engine
 *   done    last write response: the tile is in DDR (job_done)
 *   rb      result tile read back from DDR into the DPTI output path
 * and per device: whether consecutive jobs overlapped -- the next job's fill
 * granted before the previous one is done (fill overlap), its fold started
 * before the previous one published (feed / reduce overlap: the next tile
 * accumulating while the previous one reduces, the PE's ping-pong), and the
 * makespan first fire -> last done.
 *
 * +min_fill_overlaps=N / +min_feed_overlaps=N: at least N consecutive pairs
 * over all devices must show that overlap (the run script passes what the
 * build is expected to deliver; 0 = report only).  +expect_min_gap=G: the
 * smallest PE(0,0) gap over the run must be exactly G -- for a job list in
 * which the quiet-gap rule is what releases the fold (k_{j-1} + FEED_GAP
 * later than the fill and the bank rules), G = FEED_GAP of the contexts.
 */

module tb_pipeline_overlap #(
  parameter integer K_MAX_8X8 = 16,
  parameter integer K_MAX_4X4 = 16,
  parameter integer USE_V2    = 0,
  // The PE boundary gap at the modelled FP latencies (fp_ieee.sv: 8 / 11).
  parameter integer PE_GAP_MIN = 21,
  // The contexts' boundary gap (systolic_dma_top PE_QUIET_GAP); the bench's
  // negative control shrinks it below PE_GAP_MIN and expects a FAIL.
  parameter integer PE_QUIET_GAP = 23
);

`include "pipe_jobs.svh"

  localparam integer N           = 8;
  localparam integer MAX_JOBS    = 24;
  localparam integer SLAB_BASE   = 32'h0000_8000;
  localparam integer SLAB_STRIDE = 32'h0000_1000;   // 8 windows: k <= 56
  localparam integer C_BASE      = 32'h0000_1000;
  localparam integer MEM_BYTES   = 131072;
  localparam integer JUNK        = 4096;
  localparam integer MAX_CYC     = 4_000_000;
  localparam integer KMAX_BENCH  = (SLAB_STRIDE / 512 - 1) * 8;

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
    .NUM_8X8 (1), .NUM_4X4 (3), .PE_QUIET_GAP (PE_QUIET_GAP),
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

  function automatic integer slab_steps(input integer k);
    return ((k + 7) / 8) * 8;
  endfunction

  // ---- operands: every job its own slab; a GEMM's jobs share one A / B -----
  localparam integer KTOT_BENCH = 4096;
  integer      a_val [0:MAX_JOBS-1][0:N-1][0:63];   // A[row][step of this job]
  integer      b_val [0:MAX_JOBS-1][0:63][0:N-1];   // B[step of this job][col]
  logic [31:0] lcg = 32'd20261018;

  function automatic integer rnd_small();   // -8 .. 8
    lcg = lcg * 32'd1664525 + 32'd1013904223;
    return integer'((lcg >> 16) % 17) - 8;
  endfunction

  task automatic make_job(input integer j, input integer dev, input integer kdesc,
                          input integer klog);
    integer p, base, w, step, lane, v;
    begin
      p    = (dev == 0) ? 8 : 4;
      base = (SLAB_BASE + j * SLAB_STRIDE) / 4;
      for (step = 0; step < slab_steps(kdesc) + 8; step++) begin
        for (lane = 0; lane < 8; lane++) begin
          w = (step / 8) * 128 + lane * 8 + (step % 8);
          if (lane < p && step < klog) begin
            v = rnd_small();
            a_val[j][lane][step] = v;
          end else if (lane < p && step < kdesc)
            v = 0;
          else
            v = JUNK + step * 8 + lane;
          dut.u_mig_7series_0.mem[base + w] = int_to_fp32(v);
          w = (step / 8) * 128 + 64 + (step % 8) * 8 + lane;
          if (lane < p && step < klog) begin
            v = rnd_small();
            b_val[j][step][lane] = v;
          end else if (lane < p && step < kdesc)
            v = 0;
          else
            v = JUNK + 1000 + step * 8 + lane;
          dut.u_mig_7series_0.mem[base + w] = int_to_fp32(v);
        end
      end
    end
  endtask

  // The legacy compiler-facing port: valid until the registered ready.
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
      job_a_base      <= 64'(SLAB_BASE + j * SLAB_STRIDE);
      job_b_base      <= 64'(SLAB_BASE + j * SLAB_STRIDE);
      job_c_base      <= 64'(C_BASE + j * 256);
      job_valid       <= 1'b1;
      while (!job_ready) @(posedge clk);
      @(posedge clk);
      job_valid <= 1'b0;
    end
  endtask

  // ---- per-device event streams, attributed to jobs in submission order ----
  // Every stage of one device runs its jobs in order (the context's pipeline
  // is in-order; the serialized design trivially so), so the n-th event of a
  // kind on device d belongs to the n-th job submitted to device d.
  integer cyc = 0;
  integer dev_jobs  [0:3][0:MAX_JOBS-1];   // job index of the n-th job of device d
  integer dev_njobs [0:3];                  // jobs submitted to device d
  integer n_fire [0:3], n_grant [0:3], n_fold [0:3], n_cv [0:3], n_wbacc [0:3],
          n_done [0:3], n_rb [0:3], n_feed [0:3];
  integer t_fire [0:MAX_JOBS-1], t_grant [0:MAX_JOBS-1], t_fill [0:MAX_JOBS-1],
          t_fold [0:MAX_JOBS-1], t_feed [0:MAX_JOBS-1], t_cv [0:MAX_JOBS-1],
          t_wbacc [0:MAX_JOBS-1], t_done [0:MAX_JOBS-1], t_rb [0:MAX_JOBS-1];
  integer beats_job [0:MAX_JOBS-1];         // operand beats landed for the job
  integer macs_job  [0:MAX_JOBS-1];         // MACs of the device's PEs, fold to fold
  longint done_cbase [0:MAX_JOBS-1];        // ctx_job_c_base at job_done
  integer gap_job   [0:MAX_JOBS-1];         // PE(0,0) idle cycles before this tile
  integer mac_total [0:3];                  // running MAC count of the device
  integer mac_at_fold [0:3];                // mac_total at the last fold
  integer last_pair [0:3];                  // last cycle an operand pair entered PE(0,0)
  integer in_tile   [0:3];                  // a tile's operands are flowing

  initial begin
    for (int d = 0; d < 4; d++) begin
      dev_njobs[d] = 0; n_fire[d] = 0; n_grant[d] = 0; n_fold[d] = 0; n_cv[d] = 0;
      n_wbacc[d] = 0; n_done[d] = 0; n_rb[d] = 0; n_feed[d] = 0;
      mac_total[d] = 0; mac_at_fold[d] = 0; last_pair[d] = -1; in_tile[d] = 0;
      for (int j = 0; j < MAX_JOBS; j++) dev_jobs[d][j] = -1;
    end
    for (int j = 0; j < MAX_JOBS; j++) begin
      t_fire[j] = -1; t_grant[j] = -1; t_fill[j] = -1; t_fold[j] = -1; t_feed[j] = -1;
      t_cv[j] = -1; t_wbacc[j] = -1; t_done[j] = -1; t_rb[j] = -1;
      beats_job[j] = 0; macs_job[j] = -1; done_cbase[j] = -1; gap_job[j] = -1;
    end
  end

  // Array boundary signals, per device.
  wire fold_now [0:3];
  wire cv_now   [0:3];
  wire pair_now [0:3];     // an operand pair at PE(0,0)'s inputs
  wire feed_now [0:3];     // the feeder is driving row 0 or column 0
  assign fold_now[0] = dut.scheduler_fold_start_8x8[0];
  assign cv_now[0]   = dut.c_valid_out_8x8[0];
  assign pair_now[0] = dut.a_valid_to_8x8[0][0] && dut.b_valid_to_8x8[0][0];
  assign feed_now[0] = dut.a_valid_to_8x8[0][0] || dut.b_valid_to_8x8[0][0];
  for (genvar i = 0; i < 3; i++) begin : DEV4
    assign fold_now[1+i] = dut.scheduler_fold_start_4x4[i];
    assign cv_now[1+i]   = dut.c_valid_out_4x4[i];
    assign pair_now[1+i] = dut.a_valid_to_4x4[i][0] && dut.b_valid_to_4x4[i][0];
    assign feed_now[1+i] = dut.a_valid_to_4x4[i][0] || dut.b_valid_to_4x4[i][0];
  end

  // MACs per device this cycle: every PE with an operand pair entering its
  // multiplier (systolic_pe: pipe_pair_valid).
  wire pe_mac8 [0:7][0:7];
  wire pe_mac4 [0:2][0:3][0:3];
  for (genvar r = 0; r < 8; r++) begin : M8R
    for (genvar c = 0; c < 8; c++) begin : M8C
      assign pe_mac8[r][c] = dut.ACC_8X8[0].u_acc.ROW[r].COL[c].u_pe.pipe_pair_valid;
    end
  end
  for (genvar i = 0; i < 3; i++) begin : M4A
    for (genvar r = 0; r < 4; r++) begin : M4R
      for (genvar c = 0; c < 4; c++) begin : M4C
        assign pe_mac4[i][r][c] = dut.ACC_4X4[i].u_acc.ROW[r].COL[c].u_pe.pipe_pair_valid;
      end
    end
  end
  integer macs_now [0:3];
  always_comb begin
    for (int d = 0; d < 4; d++) macs_now[d] = 0;
    for (int r = 0; r < 8; r++)
      for (int c = 0; c < 8; c++)
        if (pe_mac8[r][c]) macs_now[0]++;
    for (int i = 0; i < 3; i++)
      for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
          if (pe_mac4[i][r][c]) macs_now[1+i]++;
  end

  logic feed_d [0:3];
  initial for (int d = 0; d < 4; d++) feed_d[d] = 1'b0;

  always_ff @(posedge clk) begin
    cyc <= cyc + 1;
    if (rstn) begin
      // acceptance
      if (dut.job_fire && dut.effective_job_device_id < 4) begin
        automatic integer d = dut.effective_job_device_id;
        automatic integer j = dut.effective_job_id;
        t_fire[j] <= cyc;
        n_fire[d] <= n_fire[d] + 1;
      end
      for (int d = 0; d < 4; d++) begin
        // operand descriptor granted; beats landing
        if (dut.rd_desc_valid[d] && dut.rd_desc_ready[d] && n_grant[d] < dev_njobs[d]) begin
          t_grant[dev_jobs[d][n_grant[d]]] <= cyc;
          n_grant[d] <= n_grant[d] + 1;
        end
        if (dut.ctx_dst_wr_en[d] && n_grant[d] > 0) begin
          beats_job[dev_jobs[d][n_grant[d] - 1]] <= beats_job[dev_jobs[d][n_grant[d] - 1]] + 1;
          t_fill[dev_jobs[d][n_grant[d] - 1]] <= cyc;
        end
        // fold: MACs of the previous tile close, the gap PE(0,0) saw opens
        mac_total[d] <= mac_total[d] + macs_now[d];
        if (fold_now[d] && n_fold[d] < dev_njobs[d]) begin
          t_fold[dev_jobs[d][n_fold[d]]] <= cyc;
          if (n_fold[d] > 0)
            macs_job[dev_jobs[d][n_fold[d] - 1]] <= mac_total[d] + macs_now[d] - mac_at_fold[d];
          mac_at_fold[d] <= mac_total[d] + macs_now[d];
          n_fold[d] <= n_fold[d] + 1;
        end
        // the feeder's last operand into row 0 / column 0
        feed_d[d] <= feed_now[d];
        if (feed_d[d] && !feed_now[d] && n_feed[d] < dev_njobs[d]) begin
          t_feed[dev_jobs[d][n_feed[d]]] <= cyc - 1;
          n_feed[d] <= n_feed[d] + 1;
        end
        // PE(0,0)'s idle gap between tiles
        if (pair_now[d]) begin
          if (!in_tile[d] && last_pair[d] >= 0 && n_fold[d] >= 1 && n_fold[d] <= dev_njobs[d])
            gap_job[dev_jobs[d][n_fold[d] - 1]] <= cyc - last_pair[d] - 1;
          in_tile[d]   <= 1;
          last_pair[d] <= cyc;
        end else if (in_tile[d] && !pair_now[d]) begin
          in_tile[d] <= 0;
        end
        // published
        if (cv_now[d] && n_cv[d] < MAX_JOBS) begin
          if (n_cv[d] < dev_njobs[d]) t_cv[dev_jobs[d][n_cv[d]]] <= cyc;
          n_cv[d] <= n_cv[d] + 1;
        end
        // write-back granted
        if (dut.wb_eng_accept && int'(dut.wb_grant_idx) == d && n_wbacc[d] < dev_njobs[d]) begin
          t_wbacc[dev_jobs[d][n_wbacc[d]]] <= cyc;
          n_wbacc[d] <= n_wbacc[d] + 1;
        end
        // done
        if (dut.ctx_job_done[d] && n_done[d] < dev_njobs[d]) begin
          t_done[dev_jobs[d][n_done[d]]]     <= cyc;
          done_cbase[dev_jobs[d][n_done[d]]] <= longint'(dut.ctx_job_c_base[d]);
          n_done[d] <= n_done[d] + 1;
        end
        // read back
        if (dut.rb_done && dut.rb_active && int'(dut.rb_owner) == d && n_rb[d] < dev_njobs[d]) begin
          t_rb[dev_jobs[d][n_rb[d]]] <= cyc;
          n_rb[d] <= n_rb[d] + 1;
        end
      end
    end
  end

  // ---- checks -------------------------------------------------------------
  integer errors = 0;
  integer serial = 0;
  integer min_fill_overlaps = 0, min_feed_overlaps = 0, expect_min_gap = -1;

  task automatic check_job(input integer j);
    integer dev, p, r, c, kk, bad, want_beats;
    longint want, got;
    bit     ok;
    begin
      dev = JOB_DEV[j];
      p   = (dev == 0) ? 8 : 4;
      bad = 0;
      for (r = 0; r < p; r++)
        for (c = 0; c < p; c++) begin
          got = fp32_to_int(dut.u_mig_7series_0.mem[(C_BASE + j * 256) / 4 + r * p + c], ok);
          want = 0;
          for (kk = 0; kk < JOB_KLOG[j]; kk++) want += longint'(a_val[j][r][kk]) * b_val[j][kk][c];
          if (!ok || got != want) begin
            if (bad < 3) $display("  FAIL: job %0d C[%0d][%0d] = %0d (ok %0b), reference %0d", j, r, c, got, ok, want);
            bad++;
          end
        end
      want_beats = slab_steps(JOB_KDESC[j]) * 4;   // 16 words per step, 4 words per beat
      if (beats_job[j] != want_beats) begin
        $display("  FAIL: job %0d: %0d operand beats landed, want %0d", j, beats_job[j], want_beats);
        bad++;
      end
      if (macs_job[j] != JOB_KDESC[j] * p * p) begin
        $display("  FAIL: job %0d: %0d multiply-accumulates, want %0d (k = %0d x %0d PEs)",
                 j, macs_job[j], JOB_KDESC[j] * p * p, JOB_KDESC[j], p * p);
        bad++;
      end
      if (done_cbase[j] != longint'(C_BASE + j * 256)) begin
        $display("  FAIL: job %0d completed with c_base 0x%0h, want 0x%0h", j, done_cbase[j], C_BASE + j * 256);
        bad++;
      end
      if (t_fire[j] < 0 || t_grant[j] < 0 || t_fill[j] < 0 || t_fold[j] < 0 || t_feed[j] < 0 ||
          t_cv[j] < 0 || t_wbacc[j] < 0 || t_done[j] < 0 || t_rb[j] < 0) begin
        $display("  FAIL: job %0d: a stage was never stamped", j);
        bad++;
      end
      if (gap_job[j] >= 0 && gap_job[j] < PE_GAP_MIN) begin
        $display("  FAIL: job %0d: PE(0,0) saw only %0d idle cycles before this tile (min %0d)",
                 j, gap_job[j], PE_GAP_MIN);
        bad++;
      end
      if (bad != 0) errors++;
      $display("PJOB job=%0d dev=%0d P=%0d kdesc=%0d klog=%0d koff=%0d ktot=%0d fire=%0d grant=%0d fill=%0d fold=%0d feed=%0d cv=%0d wbacc=%0d done=%0d rb=%0d words=%0d macs=%0d gap=%0d result=%s",
               j, dev, p, JOB_KDESC[j], JOB_KLOG[j], JOB_KOFF[j], JOB_KTOT[j],
               t_fire[j], t_grant[j], t_fill[j], t_fold[j], t_feed[j], t_cv[j], t_wbacc[j],
               t_done[j], t_rb[j], beats_job[j] * 4, macs_job[j], gap_job[j],
               (bad == 0) ? "exact" : "FAIL");
    end
  endtask

  task automatic check_gemm(input integer last);
    integer dev, p, r, c, kk, j, first, gbad;
    longint want, sum;
    bit ok;
    begin
      dev = JOB_DEV[last];
      p   = (dev == 0) ? 8 : 4;
      first = last;
      while (first > 0 && JOB_KOFF[first] != 0) first--;
      gbad = 0;
      for (r = 0; r < p; r++)
        for (c = 0; c < p; c++) begin
          sum = 0; want = 0;
          for (j = first; j <= last; j++) begin
            sum += fp32_to_int(dut.u_mig_7series_0.mem[(C_BASE + j * 256) / 4 + r * p + c], ok);
            for (kk = 0; kk < JOB_KLOG[j]; kk++) want += longint'(a_val[j][r][kk]) * b_val[j][kk][c];
          end
          if (sum != want) gbad++;
        end
      if (gbad != 0) errors++;
      $display("PGEMM first_job=%0d last_job=%0d dev=%0d P=%0d k=%0d invocations=%0d first_fire=%0d last_done=%0d last_rb=%0d result=%s",
               first, last, dev, p, JOB_KTOT[last], last - first + 1, t_fire[first], t_done[last],
               t_rb[last], (gbad == 0) ? "exact" : "FAIL");
    end
  endtask

  initial begin
    integer waited, total_fill_ov, total_feed_ov, j, d, prev;
    integer dev_first_fire [0:3], dev_last_done [0:3], dev_last_rb [0:3];
    integer dev_fill_ov [0:3], dev_feed_ov [0:3], dev_min_gap [0:3];
    if (!$value$plusargs("serial=%d", serial)) serial = 0;
    if (!$value$plusargs("min_fill_overlaps=%d", min_fill_overlaps)) min_fill_overlaps = 0;
    if (!$value$plusargs("min_feed_overlaps=%d", min_feed_overlaps)) min_feed_overlaps = 0;
    if (!$value$plusargs("expect_min_gap=%d", expect_min_gap)) expect_min_gap = -1;
    if (NJ > MAX_JOBS) $fatal(1, "tb_pipeline_overlap: at most %0d jobs", MAX_JOBS);
    for (j = 0; j < NJ; j++) begin
      if (JOB_DEV[j] < 0 || JOB_DEV[j] > 3 || JOB_START[j] < 0)
        $fatal(1, "tb_pipeline_overlap: job %0d names device %0d", j, JOB_DEV[j]);
      if (JOB_KDESC[j] < 1 || JOB_KDESC[j] > ((JOB_DEV[j] == 0) ? K_MAX_8X8 : K_MAX_4X4) ||
          JOB_KDESC[j] > KMAX_BENCH || JOB_KLOG[j] < 1 || JOB_KLOG[j] > JOB_KDESC[j])
        $fatal(1, "tb_pipeline_overlap: job %0d k %0d / %0d out of range", j, JOB_KDESC[j], JOB_KLOG[j]);
      if (JOB_KOFF[j] < 0 || JOB_KOFF[j] + JOB_KLOG[j] > JOB_KTOT[j] ||
          (JOB_KOFF[j] > 0 && (j == 0 || JOB_DEV[j - 1] != JOB_DEV[j] ||
                               JOB_KTOT[j - 1] != JOB_KTOT[j] ||
                               JOB_KOFF[j - 1] + JOB_KLOG[j - 1] != JOB_KOFF[j])) ||
          (JOB_KOFF[j] == 0 && j > 0 && JOB_KOFF[j - 1] + JOB_KLOG[j - 1] != JOB_KTOT[j - 1]))
        $fatal(1, "tb_pipeline_overlap: job %0d does not continue a K-folded GEMM", j);
      dev_jobs[JOB_DEV[j]][dev_njobs[JOB_DEV[j]]] = j;
      dev_njobs[JOB_DEV[j]]++;
    end
    if (NJ > 0 && JOB_KOFF[NJ - 1] + JOB_KLOG[NJ - 1] != JOB_KTOT[NJ - 1])
      $fatal(1, "tb_pipeline_overlap: the last GEMM is not complete");

    $display("== tb_pipeline_overlap: K_MAX_8X8 = %0d, K_MAX_4X4 = %0d, %s, %0d job(s), %s ==",
             K_MAX_8X8, K_MAX_4X4, (USE_V2 != 0) ? "v2" : "v1", NJ,
             serial ? "one at a time (serial)" : "offered back to back (pipelined)");
    for (j = 0; j < NJ; j++) make_job(j, JOB_DEV[j], JOB_KDESC[j], JOB_KLOG[j]);
    repeat (20) @(posedge clk);
    rstn = 1'b1;
    repeat (200) @(posedge clk);            // memory calibrated, fleet idle

    for (j = 0; j < NJ; j++) begin
      submit(j, JOB_DEV[j], JOB_KDESC[j], JOB_START[j]);
      if (serial) begin
        waited = 0;
        while (t_rb[j] < 0 && waited < MAX_CYC) begin @(posedge clk); waited++; end
        if (t_rb[j] < 0) begin
          $display("  FAIL: job %0d did not complete within %0d cycles", j, MAX_CYC);
          errors++;
        end
      end
    end
    waited = 0;
    while (waited < MAX_CYC) begin
      automatic bit all = 1'b1;
      for (j = 0; j < NJ; j++) if (t_rb[j] < 0) all = 1'b0;
      if (all) break;
      @(posedge clk); waited++;
    end
    if (waited >= MAX_CYC) begin
      $display("  FAIL: not every job completed within %0d cycles", MAX_CYC);
      errors++;
    end
    repeat (40) @(posedge clk);
    // The MACs of the last fold of every device close here.
    for (d = 0; d < 4; d++)
      if (n_fold[d] > 0 && dev_njobs[d] > 0)
        macs_job[dev_jobs[d][n_fold[d] - 1]] = mac_total[d] - mac_at_fold[d];

    for (j = 0; j < NJ; j++) begin
      check_job(j);
      if (JOB_KOFF[j] + JOB_KLOG[j] == JOB_KTOT[j]) check_gemm(j);
    end

    // ---- per device: pulse counts and overlap -------------------------------
    total_fill_ov = 0; total_feed_ov = 0;
    for (d = 0; d < 4; d++) begin
      dev_first_fire[d] = -1; dev_last_done[d] = -1; dev_last_rb[d] = -1;
      dev_fill_ov[d] = 0; dev_feed_ov[d] = 0; dev_min_gap[d] = -1;
      if (dev_njobs[d] == 0) continue;
      if (n_cv[d] != dev_njobs[d] || n_fold[d] != dev_njobs[d]) begin
        $display("  FAIL: device %0d: %0d jobs, %0d folds, %0d results published (want one of each per job)",
                 d, dev_njobs[d], n_fold[d], n_cv[d]);
        errors++;
      end
      for (int n = 0; n < dev_njobs[d]; n++) begin
        j = dev_jobs[d][n];
        if (n == 0) dev_first_fire[d] = t_fire[j];
        dev_last_done[d] = t_done[j];
        dev_last_rb[d]   = t_rb[j];
        if (n > 0) begin
          prev = dev_jobs[d][n - 1];
          if (t_grant[j] >= 0 && t_done[prev] >= 0 && t_grant[j] < t_done[prev]) dev_fill_ov[d]++;
          if (t_fold[j] >= 0 && t_cv[prev] >= 0 && t_fold[j] < t_cv[prev])      dev_feed_ov[d]++;
          if (gap_job[j] >= 0 && (dev_min_gap[d] < 0 || gap_job[j] < dev_min_gap[d]))
            dev_min_gap[d] = gap_job[j];
        end
      end
      total_fill_ov += dev_fill_ov[d];
      total_feed_ov += dev_feed_ov[d];
      $display("PDEV dev=%0d jobs=%0d folds=%0d published=%0d first_fire=%0d last_done=%0d last_rb=%0d makespan=%0d fill_overlaps=%0d feed_reduce_overlaps=%0d min_pe_gap=%0d",
               d, dev_njobs[d], n_fold[d], n_cv[d], dev_first_fire[d], dev_last_done[d], dev_last_rb[d],
               dev_last_done[d] - dev_first_fire[d], dev_fill_ov[d], dev_feed_ov[d], dev_min_gap[d]);
    end
    if (total_fill_ov < min_fill_overlaps) begin
      $display("  FAIL: %0d consecutive pairs overlapped their fill with the previous job's tail, want at least %0d",
               total_fill_ov, min_fill_overlaps);
      errors++;
    end
    if (expect_min_gap >= 0) begin
      automatic integer run_min = -1;
      for (d = 0; d < 4; d++)
        if (dev_min_gap[d] >= 0 && (run_min < 0 || dev_min_gap[d] < run_min)) run_min = dev_min_gap[d];
      if (run_min != expect_min_gap) begin
        $display("  FAIL: the smallest PE(0,0) gap of the run is %0d, the gap-bound rule should make it exactly %0d",
                 run_min, expect_min_gap);
        errors++;
      end else
        $display("  gap-bound: smallest PE(0,0) gap %0d = FEED_GAP", run_min);
    end
    if (total_feed_ov < min_feed_overlaps) begin
      $display("  FAIL: %0d consecutive pairs fed the next tile before the previous one published, want at least %0d",
               total_feed_ov, min_feed_overlaps);
      errors++;
    end
    if (dut.any_err !== 1'b0 || dut.err_w_owner !== 1'b0 || dut.u_mig_7series_0.errors != 0) begin
      $display("  FAIL: any_err %0b, err_w_owner %0b, memory-model errors %0d",
               dut.any_err, dut.err_w_owner, dut.u_mig_7series_0.errors);
      errors++;
    end
    $display("PSUM jobs=%0d fill_overlaps=%0d feed_reduce_overlaps=%0d", NJ, total_fill_ov, total_feed_ov);
    $display("== %s: %0d job(s), %0d error(s) ==", (errors == 0) ? "PASS" : "FAIL", NJ, errors);
    if (errors != 0) $fatal(1);
    $finish;
  end

endmodule

`default_nettype wire
