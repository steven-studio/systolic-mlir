`default_nettype none
`timescale 1ns / 1ps

/*
 * tb_cost_concurrent -- the four accelerators of systolic_dma_top running at
 * the same time, phase by phase, for checking how far the cost model's
 * "every array fills on its own" holds when the fills share the one AXI read
 * port (dma_engine_multi) and the one write-back engine.  BENCH ONLY: it
 * reads the design through hierarchical references and drives only the
 * top's ports.  Companion of tb_cost_phases.sv (one job at a time).
 *
 *   run_cost_phases.sh <K_MAX_8X8> <K_MAX_4X4> <waves.svh> <out_dir> [v1|v2] concurrent
 *
 * The waves come from an include file (`cost_waves.svh`):
 *
 *   localparam integer NW = ...;
 *   localparam integer WAVE_KDESC [0:NW-1][0:3] = '{...};  descriptor k per device, 0 = idle
 *   localparam integer WAVE_KLOG  [0:NW-1][0:3] = '{...};  real reduction steps
 *   localparam integer WAVE_GAP   [0:NW-1]      = '{...};  cycles between submissions
 *
 * A wave submits one job to every device it names, in device order 0..3,
 * WAVE_GAP cycles apart (0: as fast as the descriptor port takes them), each
 * with its own operand slab and result tile, and waits until every result is
 * written back and read back before the next wave.  Every element of every
 * result is checked against an exact integer reference.
 *
 * Per device, the same time stamps as tb_cost_phases (cycles of the 100 MHz
 * array clock); per wave the makespan, first descriptor accepted -> last
 * result tile in DDR.  start_cycle = 0 on every descriptor.  The reduction
 * steps each array executes are counted as in tb_cost_phases
 * (reduction_probe.svh); a device whose PEs do not each execute exactly
 * kdesc steps fails.
 */

module tb_cost_concurrent #(
  parameter integer K_MAX_8X8 = 16,
  parameter integer K_MAX_4X4 = 16,
  parameter integer USE_V2    = 0
);

`include "cost_waves.svh"

  localparam integer N           = 8;
  localparam integer SLAB_BASE   = 32'h0000_4000;
  localparam integer SLAB_STRIDE = 32'h0000_6000;   // 48 windows per device
  localparam integer C_BASE      = 32'h0000_1000;
  localparam integer C_STRIDE    = 32'h0000_0100;   // one 8x8 tile
  localparam integer JUNK        = 4096;
  localparam integer MAX_CYC     = 4_000_000;
  localparam integer KMAX_BENCH  = (SLAB_STRIDE / 512 - 1) * 8;   // 376 steps

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

  // ---- operands and references, per device -----------------------------------
  integer      a_val [0:3][0:N-1][0:KMAX_BENCH-1];
  integer      b_val [0:3][0:KMAX_BENCH-1][0:N-1];
  logic [31:0] lcg = 32'd20261013;

  function automatic integer rnd_small();   // -8 .. 8
    lcg = lcg * 32'd1664525 + 32'd1013904223;
    return integer'((lcg >> 16) % 17) - 8;
  endfunction

  function automatic integer slab_steps(input integer k);
    return ((k + 7) / 8) * 8;
  endfunction

  function automatic integer slab_base(input integer dev);
    return SLAB_BASE + dev * SLAB_STRIDE;
  endfunction

  task automatic make_job(input integer dev, input integer kdesc, input integer klog);
    integer p, base, w, step, lane, v;
    begin
      p    = (dev == 0) ? 8 : 4;
      base = slab_base(dev) / 4;
      for (step = 0; step < slab_steps(kdesc) + 8; step++) begin
        for (lane = 0; lane < 8; lane++) begin
          w = (step / 8) * 128 + lane * 8 + (step % 8);
          if (lane < p && step < klog) begin
            v = rnd_small();
            a_val[dev][lane][step] = v;
          end else if (lane < p && step < kdesc)
            v = 0;
          else
            v = JUNK + step * 8 + lane;
          dut.u_mig_7series_0.mem[base + w] = int_to_fp32(v);
          w = (step / 8) * 128 + 64 + (step % 8) * 8 + lane;
          if (lane < p && step < klog) begin
            v = rnd_small();
            b_val[dev][step][lane] = v;
          end else if (lane < p && step < kdesc)
            v = 0;
          else
            v = JUNK + 1000 + step * 8 + lane;
          dut.u_mig_7series_0.mem[base + w] = int_to_fp32(v);
        end
      end
    end
  endtask

  task automatic submit(input integer j, input integer dev, input integer kdesc);
    begin
      @(posedge clk);
      job_id          <= 32'(j);
      job_device_id   <= 32'(dev);
      job_m           <= (dev == 0) ? 32'd8 : 32'd4;
      job_n           <= (dev == 0) ? 32'd8 : 32'd4;
      job_k           <= 32'(kdesc);
      job_start_cycle <= 32'd0;
      job_est_cycles  <= 32'd1;
      job_a_base      <= 64'(slab_base(dev));
      job_b_base      <= 64'(slab_base(dev));
      job_c_base      <= 64'(C_BASE + dev * C_STRIDE);
      job_valid       <= 1'b1;
      while (!job_ready) @(posedge clk);
      @(posedge clk);
      job_valid <= 1'b0;
    end
  endtask

`include "reduction_probe.svh"

  // ---- time stamps, per device ------------------------------------------------
  integer cyc = 0;
  integer t_fire [0:3], t_grant [0:3], t_fill [0:3], t_fold [0:3];
  integer t_cv [0:3], t_done [0:3], t_rb [0:3], words_at_done [0:3];
  initial for (int d = 0; d < 4; d++) begin
    t_fire[d] = -1; t_grant[d] = -1; t_fill[d] = -1; t_fold[d] = -1;
    t_cv[d] = -1; t_done[d] = -1; t_rb[d] = -1; words_at_done[d] = -1;
  end

  logic [3:0] fold_now, cv_now;
  always_comb begin
    fold_now[0] = dut.scheduler_fold_start_8x8[0];
    cv_now[0]   = dut.c_valid_out_8x8[0];
    for (int i = 0; i < 3; i++) begin
      fold_now[i + 1] = dut.scheduler_fold_start_4x4[i];
      cv_now[i + 1]   = dut.c_valid_out_4x4[i];
    end
  end

  logic clear_stamps = 1'b0;
  always_ff @(posedge clk) begin
    cyc <= cyc + 1;
    if (clear_stamps) begin
      for (int d = 0; d < 4; d++) begin
        t_fire[d] <= -1; t_grant[d] <= -1; t_fill[d] <= -1; t_fold[d] <= -1;
        t_cv[d] <= -1; t_done[d] <= -1; t_rb[d] <= -1; words_at_done[d] <= -1;
      end
    end else if (rstn) begin
      if (dut.job_fire && dut.effective_job_device_id < 4)
        t_fire[dut.effective_job_device_id] <= cyc;
      for (int d = 0; d < 4; d++) begin
        if (t_fire[d] >= 0) begin
          if (t_grant[d] < 0 && dut.rd_desc_valid[d] && dut.rd_desc_ready[d]) t_grant[d] <= cyc;
          if (t_fill[d] < 0 && dut.ctx_phase[d] == 4'd3)                     t_fill[d]  <= cyc;
          if (t_fold[d] < 0 && fold_now[d])                                  t_fold[d]  <= cyc;
          if (t_cv[d] < 0 && t_fold[d] >= 0 && cv_now[d])                    t_cv[d]    <= cyc;
          if (t_done[d] < 0 && dut.ctx_job_done[d]) begin
            t_done[d]        <= cyc;
            words_at_done[d] <= dut.ctx_words_written[d];
          end
          if (t_rb[d] < 0 && t_done[d] >= 0 && dut.rb_done && int'(dut.rb_owner) == d)
            t_rb[d] <= cyc;
        end
      end
    end
  end

  // ---- the run ---------------------------------------------------------------
  integer errors = 0;

  task automatic check_device(input integer w, input integer dev,
                              input integer kdesc, input integer klog);
    integer p, r, c, kk, bad, mn, mx, sum;
    longint want, got;
    bit     ok;
    begin
      p   = (dev == 0) ? 8 : 4;
      bad = 0;
      for (r = 0; r < p; r++)
        for (c = 0; c < p; c++) begin
          got = fp32_to_int(dut.u_mig_7series_0.mem[(C_BASE + dev * C_STRIDE) / 4 + r * p + c], ok);
          want = 0;
          for (kk = 0; kk < klog; kk++) want += longint'(a_val[dev][r][kk]) * b_val[dev][kk][c];
          if (!ok || got != want) bad++;
        end
      if (words_at_done[dev] != slab_steps(kdesc) * 16) bad++;
      // Exactly kdesc reduction steps, in every PE and into the array.
      rp_summary(dev, mn, mx, sum);
      if (mn != kdesc || mx != kdesc || rp_feed_a[dev] != kdesc || rp_feed_b[dev] != kdesc ||
          rp_kdim[dev] != kdesc) bad++;
      if (bad != 0) errors++;
      $display("CPHASES wave=%0d dev=%0d P=%0d kdesc=%0d klog=%0d fire=%0d grant=%0d fill=%0d fold=%0d cv=%0d done=%0d rb=%0d words=%0d steps_pe_min=%0d steps_pe_max=%0d macs=%0d feed_a=%0d feed_b=%0d feed_cycles=%0d k_dim=%0d result=%s",
               w, dev, p, kdesc, klog, t_fire[dev], t_grant[dev], t_fill[dev],
               t_fold[dev], t_cv[dev], t_done[dev], t_rb[dev], words_at_done[dev],
               mn, mx, sum, rp_feed_a[dev], rp_feed_b[dev], rp_feed_cyc[dev], rp_kdim[dev],
               (bad == 0) ? "exact" : "FAIL");
    end
  endtask

  initial begin
    if (K_MAX_8X8 < 1 || K_MAX_4X4 < 1)
      $fatal(1, "tb_cost_concurrent: K_MAX must be positive");
    for (int w = 0; w < NW; w++)
      for (int d = 0; d < 4; d++)
        if (WAVE_KDESC[w][d] != 0 &&
            (WAVE_KDESC[w][d] < 1 || WAVE_KDESC[w][d] > ((d == 0) ? K_MAX_8X8 : K_MAX_4X4) ||
             WAVE_KDESC[w][d] > KMAX_BENCH || WAVE_KLOG[w][d] < 1 ||
             WAVE_KLOG[w][d] > WAVE_KDESC[w][d]))
          $fatal(1, "tb_cost_concurrent: wave %0d device %0d k %0d / %0d out of range",
                 w, d, WAVE_KDESC[w][d], WAVE_KLOG[w][d]);
    $display("== tb_cost_concurrent: K_MAX_8X8 = %0d, K_MAX_4X4 = %0d, %s, %0d wave(s) ==",
             K_MAX_8X8, K_MAX_4X4, (USE_V2 != 0) ? "v2" : "v1", NW);
    repeat (20) @(posedge clk);
    rstn = 1'b1;
    repeat (200) @(posedge clk);            // memory calibrated, fleet idle

    for (int w = 0; w < NW; w++) begin
      integer waited, first, last, busy;
      bit     pending;
      @(posedge clk) clear_stamps <= 1'b1;
      @(posedge clk) clear_stamps <= 1'b0;
      for (int d = 0; d < 4; d++)
        if (WAVE_KDESC[w][d] != 0) make_job(d, WAVE_KDESC[w][d], WAVE_KLOG[w][d]);
      for (int d = 0; d < 4; d++)
        if (WAVE_KDESC[w][d] != 0) begin
          submit(w * 4 + d, d, WAVE_KDESC[w][d]);
          repeat (WAVE_GAP[w]) @(posedge clk);
        end
      waited = 0;
      do begin
        @(posedge clk);
        waited++;
        pending = 1'b0;
        for (int d = 0; d < 4; d++)
          if (WAVE_KDESC[w][d] != 0 && (t_rb[d] < 0 || t_done[d] < 0)) pending = 1'b1;
      end while (pending && waited < MAX_CYC);
      if (pending) begin
        $display("  FAIL: wave %0d did not complete within %0d cycles", w, MAX_CYC);
        errors++;
        break;
      end
      repeat (4) @(posedge clk);
      first = -1; last = -1; busy = 0;
      for (int d = 0; d < 4; d++)
        if (WAVE_KDESC[w][d] != 0) begin
          check_device(w, d, WAVE_KDESC[w][d], WAVE_KLOG[w][d]);
          if (first < 0 || t_fire[d] < first) first = t_fire[d];
          if (t_done[d] > last) last = t_done[d];
          busy++;
        end
      $display("WAVE wave=%0d devices=%0d gap=%0d first_fire=%0d last_done=%0d makespan=%0d",
               w, busy, WAVE_GAP[w], first, last, last - first);
      repeat (16) @(posedge clk);
    end

    if (dut.any_err !== 1'b0 || dut.u_mig_7series_0.errors != 0) begin
      $display("  FAIL: any_err %0b, memory-model errors %0d", dut.any_err, dut.u_mig_7series_0.errors);
      errors++;
    end
    $display("== %s: %0d wave(s), %0d error(s) ==", (errors == 0) ? "PASS" : "FAIL", NW, errors);
    if (errors != 0) $fatal(1);
    $finish;
  end

endmodule

`default_nettype wire
