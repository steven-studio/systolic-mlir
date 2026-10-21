`default_nettype none
`timescale 1ns / 1ps

/*
 * tb_fleet_jobs -- the validation bench of the fleet cost model (Cost Model
 * V2, docs/COSTMODEL_V2.md).  BENCH ONLY: it drives the top's ports and reads
 * the design through hierarchical references; the RTL is the bitstream's,
 * unchanged.
 *
 * The jobs come from a text file named at run time (+jobs=FILE), so one
 * Verilator build per (K_MAX_8X8, K_MAX_4X4, USE_V2) runs any number of
 * cases:
 *
 *   one line per job, in submission order:   dev k present dep gap
 *     dev      0 = the 8x8, 1..3 = the 4x4s
 *     k        the job's reduction depth (job.k), 1..K_MAX of that device
 *     present  the job is offered no earlier than T0 + present cycles
 *     dep      -1, or the index of an earlier job: the job is offered no
 *              earlier than that job's completion (job_done) + gap cycles
 *   lines starting with '#' are comments
 *
 * Submission is in file order through the single job port (job_valid /
 * job_ready): job i is offered once job i-1 has been accepted and its own
 * present / dep conditions hold, and stays offered until accepted -- exactly
 * the model's in-order port (CostModelV2.h, PortPolicy::InOrder).
 *
 * Operands: every device has five operand slots laid out as the asynchronous
 * runtime lays them out (runtime/fpga_async_runtime.cpp operand_addr /
 * result_addr: 0x10000 + (dev * 5 + slot) * slab_bytes, results at 0x3000 +
 * (dev * 4 + slot) * 256), filled once with small integers; job n of a device
 * reads slot n % 5 and writes result slot n % 4.  Every result is checked
 * against the exact integer reference the moment its job completes.
 *
 * Output (cycles of the 100 MHz array clock since T0):
 *   VJ  fire  <cyc> <dev>                 job accepted (job_fire)
 *   VD  <cyc> <slot> <beats>              read descriptor accepted by the engine
 *   VA  <cyc> <slot> <len> <qcount>       burst committed to the AR register
 *   VL  <cyc> <slot>                      the descriptor's last beat returned
 *   VF  land <cyc> <dev>                  fill complete (last word landed)
 *   VF  fold|cv|wbacc|done <cyc> <dev>
 *   VR  take|rbdone <cyc> <dev>
 *   VS  ...                               end-of-run statistics
 *   VERR ...                              a wrong result
 */

module tb_fleet_jobs #(
  parameter integer K_MAX_8X8 = 16,
  parameter integer K_MAX_4X4 = 16,
  parameter integer USE_V2    = 0
);

  localparam integer N           = 8;
  localparam integer KMAX_ANY    = (K_MAX_8X8 > K_MAX_4X4) ? K_MAX_8X8 : K_MAX_4X4;
  localparam integer SLAB_BYTES  = ((KMAX_ANY + 7) / 8) * 512;
  localparam integer SLAB_STEPS  = ((KMAX_ANY + 7) / 8) * 8;
  localparam integer OP_BASE     = 32'h0001_0000;
  localparam integer OP_SLOTS    = 5;
  localparam integer RES_BASE    = 32'h0000_3000;
  localparam integer RES_SLOTS   = 4;
  localparam integer MAX_JOBS    = 4096;
  localparam integer MAX_CYC     = 20_000_000;

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

  // ---- binary32 <-> small integers, exactly (tb_cost_concurrent's) ---------
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

  // ---- the operand slots ---------------------------------------------------
  integer      a_val [0:3][0:OP_SLOTS-1][0:N-1][0:SLAB_STEPS-1];
  integer      b_val [0:3][0:OP_SLOTS-1][0:SLAB_STEPS-1][0:N-1];
  logic [31:0] lcg = 32'd20261021;

  function automatic integer rnd_small();   // -8 .. 8
    lcg = lcg * 32'd1664525 + 32'd1013904223;
    return integer'((lcg >> 16) % 17) - 8;
  endfunction

  function automatic integer op_addr(input integer dev, input integer slot);
    return OP_BASE + (dev * OP_SLOTS + slot) * SLAB_BYTES;
  endfunction

  function automatic integer res_addr(input integer dev, input integer slot);
    return RES_BASE + (dev * RES_SLOTS + slot) * 256;
  endfunction

  task automatic fill_slots();
    integer base, w, v;
    for (int dev = 0; dev < 4; dev++)
      for (int s = 0; s < OP_SLOTS; s++) begin
        base = op_addr(dev, s) / 4;
        for (int step = 0; step < SLAB_STEPS; step++)
          for (int lane = 0; lane < 8; lane++) begin
            v = rnd_small();
            a_val[dev][s][lane][step] = v;
            w = (step / 8) * 128 + lane * 8 + (step % 8);
            dut.u_mig_7series_0.mem[base + w] = int_to_fp32(v);
            v = rnd_small();
            b_val[dev][s][step][lane] = v;
            w = (step / 8) * 128 + 64 + (step % 8) * 8 + lane;
            dut.u_mig_7series_0.mem[base + w] = int_to_fp32(v);
          end
      end
  endtask

  // ---- the job list --------------------------------------------------------
  integer nj = 0;
  integer j_dev [0:MAX_JOBS-1], j_k [0:MAX_JOBS-1], j_present [0:MAX_JOBS-1];
  integer j_dep [0:MAX_JOBS-1], j_gap [0:MAX_JOBS-1];
  integer j_nth [0:MAX_JOBS-1];          // the job's index among its device's jobs
  integer j_done [0:MAX_JOBS-1];         // completion cycle, -1 until then
  integer dev_jobs [0:3];

  task automatic read_jobs();
    string fname, line;
    integer fd, n, dev, k, present, dep, gap;
    if (!$value$plusargs("jobs=%s", fname)) $fatal(1, "tb_fleet_jobs: +jobs=FILE is required");
    fd = $fopen(fname, "r");
    if (fd == 0) $fatal(1, "tb_fleet_jobs: cannot open %s", fname);
    for (int d = 0; d < 4; d++) dev_jobs[d] = 0;
    while (!$feof(fd)) begin
      line = "";
      void'($fgets(line, fd));
      if (line.len() == 0 || line.getc(0) == "#") continue;
      n = $sscanf(line, "%d %d %d %d %d", dev, k, present, dep, gap);
      if (n != 5) continue;
      if (dev < 0 || dev > 3 || k < 1 || k > ((dev == 0) ? K_MAX_8X8 : K_MAX_4X4) ||
          present < 0 || dep >= nj || nj >= MAX_JOBS)
        $fatal(1, "tb_fleet_jobs: bad job line %0d: %s", nj, line);
      j_dev[nj] = dev; j_k[nj] = k; j_present[nj] = present; j_dep[nj] = dep; j_gap[nj] = gap;
      j_nth[nj] = dev_jobs[dev]; dev_jobs[dev]++; j_done[nj] = -1;
      nj++;
    end
    $fclose(fd);
  endtask

  // ---- time and stamps -----------------------------------------------------
  integer cyc = 0;           // since T0
  logic   running = 1'b0;
  always_ff @(posedge clk) if (running) cyc <= cyc + 1;

  // Completion order per device = job order per device (the context runs its
  // jobs in order), so the n-th done of device d is job (d, n).
  integer done_seen [0:3];
  integer dev_job_of [0:3][0:MAX_JOBS-1];
  integer errors = 0;

  logic [3:0] fold_now, cv_now, land_now;
  always_comb begin
    fold_now[0] = dut.scheduler_fold_start_8x8[0];
    cv_now[0]   = dut.c_valid_out_8x8[0];
    land_now[0] = dut.ACC_8X8[0].u_ctx.fill_complete;
    fold_now[1] = dut.scheduler_fold_start_4x4[0];
    cv_now[1]   = dut.c_valid_out_4x4[0];
    land_now[1] = dut.ACC_4X4[0].u_ctx.fill_complete;
    fold_now[2] = dut.scheduler_fold_start_4x4[1];
    cv_now[2]   = dut.c_valid_out_4x4[1];
    land_now[2] = dut.ACC_4X4[1].u_ctx.fill_complete;
    fold_now[3] = dut.scheduler_fold_start_4x4[2];
    cv_now[3]   = dut.c_valid_out_4x4[2];
    land_now[3] = dut.ACC_4X4[2].u_ctx.fill_complete;
  end

  task automatic check_result(input integer j);
    integer dev, p, slot, rslot, bad;
    longint want, got;
    bit ok;
    dev = j_dev[j];
    p = (dev == 0) ? 8 : 4;
    slot = j_nth[j] % OP_SLOTS;
    rslot = j_nth[j] % RES_SLOTS;
    bad = 0;
    for (int r = 0; r < p; r++)
      for (int c = 0; c < p; c++) begin
        got = fp32_to_int(dut.u_mig_7series_0.mem[res_addr(dev, rslot) / 4 + r * p + c], ok);
        want = 0;
        for (int kk = 0; kk < j_k[j]; kk++)
          want += longint'(a_val[dev][slot][r][kk]) * b_val[dev][slot][kk][c];
        if (!ok || got != want) bad++;
      end
    if (bad != 0) begin
      $display("VERR job=%0d dev=%0d k=%0d wrong elements=%0d", j, dev, j_k[j], bad);
      errors++;
    end
  endtask

  integer max_q = 0, shared_cycles = 0, r_stall = 0, rb_full_cycles = 0;
  logic fill_complete_q [0:3];
  initial for (int d = 0; d < 4; d++) fill_complete_q[d] = 1'b0;

  always @(posedge clk) if (running) begin
    if (dut.job_fire)
      $display("VJ fire %0d %0d", cyc, dut.effective_job_device_id);
    for (int s = 0; s < 5; s++)
      if (dut.rd_desc_valid[s] && dut.rd_desc_ready[s])
        $display("VD %0d %0d %0d", cyc, s, dut.rd_desc_beats[s]);
    if (dut.u_eng.ar_issue)
      $display("VA %0d %0d %0d %0d", cyc, dut.u_eng.grant, dut.u_eng.sl_len[dut.u_eng.grant], dut.u_eng.q_count);
    if (dut.u_eng.done_valid)
      $display("VL %0d %0d", cyc - 1, (dut.u_eng.done_tag == 8'h3C) ? 4 : int'(dut.u_eng.done_tag) - 8'h30);
    if (int'(dut.u_eng.q_count) > max_q) max_q = int'(dut.u_eng.q_count);
    if (dut.u_eng.shared) shared_cycles++;
    if (dut.u_eng.m_axi_rvalid && !dut.u_eng.m_axi_rready) r_stall++;
    if (!dut.rb_cdc_src_ready) rb_full_cycles++;
    for (int d = 0; d < 4; d++) begin
      if (land_now[d] && !fill_complete_q[d]) $display("VF land %0d %0d", cyc, d);
      fill_complete_q[d] <= land_now[d];
      if (fold_now[d]) $display("VF fold %0d %0d", cyc, d);
      if (cv_now[d])   $display("VF cv %0d %0d", cyc, d);
      if (dut.ctx_job_done[d]) begin
        $display("VF done %0d %0d", cyc, d);
        j_done[dev_job_of[d][done_seen[d]]] = cyc;
        check_result(dev_job_of[d][done_seen[d]]);
        done_seen[d]++;
      end
    end
    if (dut.wb_eng_accept) $display("VF wbacc %0d %0d", cyc, dut.wb_grant_idx);
    if (dut.rb_take)       $display("VR take %0d %0d", cyc, dut.rb_grant);
    if ((dut.rb_hdr_valid || dut.rb_dst_wr_en) && dut.rb_cdc_src_ready)
      $display("VO push %0d %0t", cyc, $realtime);
    if (dut.rb_done)       $display("VR rbdone %0d %0d", cyc, dut.rb_owner);
  end

  // The readback FIFO's reader (the DPTI clock domain): every beat popped
  // into the byte serializer, stamped with the array cycle count and time.
  always @(posedge dut.clk_dpti)
    if (running && dut.rb_cdc_dst_valid && dut.rb_cdc_dst_ready)
      $display("VO pop %0d %0t", cyc, $realtime);

  // ---- the run ---------------------------------------------------------------
  initial begin
    integer next, waited, rb_total, rb_done_n;
    read_jobs();
    for (int d = 0; d < 4; d++) done_seen[d] = 0;
    begin
      integer cnt [0:3];
      for (int d = 0; d < 4; d++) cnt[d] = 0;
      for (int j = 0; j < nj; j++) begin
        dev_job_of[j_dev[j]][cnt[j_dev[j]]] = j;
        cnt[j_dev[j]]++;
      end
    end
    $display("== tb_fleet_jobs: K_MAX_8X8 = %0d, K_MAX_4X4 = %0d, %s, %0d job(s) ==",
             K_MAX_8X8, K_MAX_4X4, (USE_V2 != 0) ? "v2" : "v1", nj);
    repeat (20) @(posedge clk);
    rstn = 1'b1;
    repeat (200) @(posedge clk);            // memory calibrated, fleet idle
    fill_slots();
    @(posedge clk);
    running <= 1'b1;                         // T0: cyc counts from here
    $display("VT0");

    // The port is driven on the falling edge, so job_ready / job_fire are
    // sampled stable and a job fires on the rising edge it is offered on.
    // A job consumed at a rising edge lets the next one be offered at the
    // following falling edge: back-to-back jobs fire on consecutive cycles.
    next = 0;
    waited = 0;
    while (next < nj && waited < MAX_CYC) begin
      @(negedge clk);
      waited++;
      if (job_valid && dut.job_fire) begin
        @(posedge clk);
        #1 job_valid = 1'b0;
        next++;
      end else if (!job_valid &&
                   cyc >= j_present[next] &&
                   (j_dep[next] < 0 ||
                    (j_done[j_dep[next]] >= 0 && cyc >= j_done[j_dep[next]] + j_gap[next]))) begin
        job_id          = 32'(next);
        job_device_id   = 32'(j_dev[next]);
        job_m           = (j_dev[next] == 0) ? 32'd8 : 32'd4;
        job_n           = (j_dev[next] == 0) ? 32'd8 : 32'd4;
        job_k           = 32'(j_k[next]);
        job_start_cycle = 32'd0;
        job_est_cycles  = 32'd1;
        job_a_base      = 64'(op_addr(j_dev[next], j_nth[next] % OP_SLOTS));
        job_b_base      = 64'd0;
        job_c_base      = 64'(res_addr(j_dev[next], j_nth[next] % RES_SLOTS));
        job_valid       = 1'b1;
      end
    end
    job_valid <= 1'b0;

    // drain: every job done and every result read back
    rb_total = nj;
    waited = 0;
    do begin
      @(posedge clk);
      waited++;
      rb_done_n = 0;
      for (int j = 0; j < nj; j++) if (j_done[j] >= 0) rb_done_n++;
    end while ((rb_done_n < nj || dut.rb_active || (|dut.rb_pending)) && waited < MAX_CYC);
    repeat (2000) @(posedge clk);            // the output path empties

    if (rb_done_n < nj) begin
      $display("  FAIL: %0d of %0d jobs completed", rb_done_n, nj);
      errors++;
    end
    if (dut.any_err !== 1'b0 || dut.u_mig_7series_0.errors != 0) begin
      $display("  FAIL: any_err %0b, memory-model errors %0d", dut.any_err, dut.u_mig_7series_0.errors);
      errors++;
    end
    $display("VS max_q=%0d shared_cycles=%0d r_stall=%0d rb_fifo_full_cycles=%0d busy=%0d",
             max_q, shared_cycles, r_stall, rb_full_cycles, dut.u_eng.busy_cycles);
    $display("== %s: %0d job(s), %0d error(s) ==", (errors == 0) ? "PASS" : "FAIL", nj, errors);
    if (errors != 0) $fatal(1);
    $finish;
  end

endmodule

`default_nettype wire
