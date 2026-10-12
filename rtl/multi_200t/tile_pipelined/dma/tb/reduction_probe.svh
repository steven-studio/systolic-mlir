// reduction_probe.svh -- the reduction steps the hardware actually executes,
// per accelerator and per job.  BENCH ONLY: included at module scope by
// tb_cost_phases.sv and tb_cost_concurrent.sv (after `dut` and `clk`), it
// reads the design through hierarchical references and drives nothing, so
// the design's function and timing are those of the RTL alone.
//
// Per accelerator d (0: ACC_8X8[0], 1..3: ACC_4X4[0..2]), every counter is
// cleared in the cycle d accepts a job (ctx_job_fire[d]) and counts array-
// clock cycles from the next one on:
//
//   rp_mac[d][r][c]  cycles PE (r, c) has an operand pair entering its
//                    multiplier (systolic_pe: pipe_pair_valid = a_valid_reg
//                    && b_valid_reg): the multiply-accumulates it performs,
//                    i.e. the reduction steps it executes
//   rp_feed_a[d]     cycles the feeder presents a valid A operand to row 0
//                    (systolic_array a_valid_in[0])
//   rp_feed_b[d]     cycles the feeder presents a valid B operand to
//                    column 0 (b_valid_in[0])
//   rp_feed_cyc[d]   cycles the context's control FSM is in ST_FEED
//   rp_kdim[d]       the context's k_dim (systolic_accel_context: job_k_reg
//                    under the external scheduler) when the fold starts
//
// rp_summary(d, ...) folds the PE counters of device d into min / max / sum
// over its P x P PEs.

  wire [31:0] rp_mac8 [0:7][0:7];
  wire [31:0] rp_mac4 [0:2][0:3][0:3];

  for (genvar r = 0; r < 8; r++) begin : RP8_ROW
    for (genvar c = 0; c < 8; c++) begin : RP8_COL
      integer n = 0;
      always_ff @(posedge clk)
        if (dut.ctx_job_fire[0])                                         n <= 0;
        else if (dut.ACC_8X8[0].u_acc.ROW[r].COL[c].u_pe.pipe_pair_valid) n <= n + 1;
      assign rp_mac8[r][c] = 32'(n);
    end
  end

  for (genvar i = 0; i < 3; i++) begin : RP4_ACC
    for (genvar r = 0; r < 4; r++) begin : RP4_ROW
      for (genvar c = 0; c < 4; c++) begin : RP4_COL
        integer n = 0;
        always_ff @(posedge clk)
          if (dut.ctx_job_fire[1 + i])                                       n <= 0;
          else if (dut.ACC_4X4[i].u_acc.ROW[r].COL[c].u_pe.pipe_pair_valid) n <= n + 1;
        assign rp_mac4[i][r][c] = 32'(n);
      end
    end
  end

  wire [3:0] rp_a_valid0, rp_b_valid0, rp_in_feed, rp_fold;
  wire [31:0] rp_k_dim [0:3];
  assign rp_a_valid0[0] = dut.ACC_8X8[0].u_acc.a_valid_in[0];
  assign rp_b_valid0[0] = dut.ACC_8X8[0].u_acc.b_valid_in[0];
  assign rp_in_feed[0]  = (3'(dut.ACC_8X8[0].u_ctx.state) == 3'd1);   // ST_FEED
  assign rp_k_dim[0]    = 32'(dut.ACC_8X8[0].u_ctx.k_dim);
  assign rp_fold[0]     = dut.scheduler_fold_start_8x8[0];
  for (genvar i = 0; i < 3; i++) begin : RP4_CTL
    assign rp_a_valid0[1 + i] = dut.ACC_4X4[i].u_acc.a_valid_in[0];
    assign rp_b_valid0[1 + i] = dut.ACC_4X4[i].u_acc.b_valid_in[0];
    assign rp_in_feed[1 + i]  = (3'(dut.ACC_4X4[i].u_ctx.state) == 3'd1);
    assign rp_k_dim[1 + i]    = 32'(dut.ACC_4X4[i].u_ctx.k_dim);
    assign rp_fold[1 + i]     = dut.scheduler_fold_start_4x4[i];
  end

  integer rp_feed_a [0:3], rp_feed_b [0:3], rp_feed_cyc [0:3], rp_kdim [0:3];
  initial for (int d = 0; d < 4; d++) begin
    rp_feed_a[d] = 0; rp_feed_b[d] = 0; rp_feed_cyc[d] = 0; rp_kdim[d] = -1;
  end

  always_ff @(posedge clk)
    for (int d = 0; d < 4; d++)
      if (dut.ctx_job_fire[d]) begin
        rp_feed_a[d] <= 0; rp_feed_b[d] <= 0; rp_feed_cyc[d] <= 0; rp_kdim[d] <= -1;
      end else begin
        if (rp_a_valid0[d]) rp_feed_a[d]   <= rp_feed_a[d] + 1;
        if (rp_b_valid0[d]) rp_feed_b[d]   <= rp_feed_b[d] + 1;
        if (rp_in_feed[d])  rp_feed_cyc[d] <= rp_feed_cyc[d] + 1;
        if (rp_fold[d])     rp_kdim[d]     <= integer'(rp_k_dim[d]);
      end

  task automatic rp_summary(input integer d, output integer mn, output integer mx,
                            output integer sum);
    integer p, v;
    begin
      p = (d == 0) ? 8 : 4;
      mn = -1; mx = -1; sum = 0;
      for (int r = 0; r < p; r++)
        for (int c = 0; c < p; c++) begin
          v = (d == 0) ? integer'(rp_mac8[r][c]) : integer'(rp_mac4[d - 1][r][c]);
          if (mn < 0 || v < mn) mn = v;
          if (v > mx) mx = v;
          sum += v;
        end
    end
  endtask
