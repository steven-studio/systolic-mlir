// -----------------------------------------------------------------------------
// tb_operand_throttle.sv -- self-checking bench for the operand rate limiter.
//
//   xsim:     xvlog -sv core/operand_throttle.sv tb/tb_operand_throttle.sv \
//             && xelab -R tb_operand_throttle
//   icarus:   iverilog -g2012 -o tb.vvp core/operand_throttle.sv \
//             tb/tb_operand_throttle.sv && vvp tb.vvp
//
// Cycles and grants are counted in one always_ff inside the bench, so both
// are sampled the same way on the same edge.  Polling an output from
// procedural code instead makes the result depend on the simulator's
// non-blocking-update visibility (xsim and Icarus disagreed by one cycle).
//
// Checks, in order of how much they protect:
//   1. BYPASS IS TRANSPARENT -- k steps in exactly k cycles, no stalls.  This
//      is the regression that guarantees every cycle count already published
//      is reproduced after the module is inserted.
//   2. KNEE IS AT 2N -- at beta = 2N words/cycle the throttle is cycle-
//      identical to bypass.  The array's operand demand is exactly 2N words
//      per reduction step, which is the traffic term the fleet model assumes.
//   3. RATE IS EXACT below the knee -- k steps take k * 2N / beta cycles, with
//      no priming cycle and no credit carried in from the previous fold.
// -----------------------------------------------------------------------------

`timescale 1ns/1ps

module tb_operand_throttle;

  localparam integer N              = 8;
  localparam integer WORDS_PER_STEP = 2 * N;   // 16 for N = 8

  reg         clk = 0, rst_n = 0;
  reg         beta_bypass, stat_clear, step_req;
  reg  [15:0] beta_q8_8;
  wire        step_gnt;
  wire [31:0] stall_cycles, step_count;

  always #5 clk = ~clk;

  operand_throttle #(.N(N)) dut (
    .clk(clk), .rst_n(rst_n),
    .beta_bypass(beta_bypass), .beta_q8_8(beta_q8_8),
    .step_req(step_req), .step_gnt(step_gnt),
    .stall_cycles(stall_cycles), .step_count(step_count),
    .stat_clear(stat_clear)
  );

  // ---- race-free measurement: both counters advance on the same edge ------
  reg         measuring = 0;
  reg  [31:0] cyc_ctr = 0, grants = 0;

  always @(posedge clk) begin
    if (!measuring) begin
      cyc_ctr <= 0;
      grants  <= 0;
    end else begin
      cyc_ctr <= cyc_ctr + 1;
      if (step_gnt) grants <= grants + 1;
    end
  end

  integer errors = 0;
  integer cyc;

  task run_fold(input integer steps);
    begin
      @(negedge clk); stat_clear = 1;
      @(negedge clk); stat_clear = 0; measuring = 1; step_req = 1;
      while (grants < steps) @(negedge clk);
      cyc = cyc_ctr;
      step_req = 0; measuring = 0;
      @(negedge clk);
    end
  endtask

  task check(input integer got, input integer want, input [255:0] name);
    begin
      if (got !== want) begin
        $display("FAIL  %0s: got %0d, want %0d", name, got, want);
        errors = errors + 1;
      end else
        $display("PASS  %0s: %0d cycles", name, got);
    end
  endtask

  task check_rate(input integer beta_words, input integer steps);
    begin
      beta_q8_8 = beta_words << 8;
      run_fold(steps);
      check(cyc, (steps * WORDS_PER_STEP) / beta_words, "rate");
      $display("      (beta = %0d words/cycle, %0d steps, %0d stalls)",
               beta_words, steps, stall_cycles);
    end
  endtask

  initial begin
    beta_bypass = 1; beta_q8_8 = 16'h0100;
    step_req = 0; stat_clear = 0;
    repeat (4) @(negedge clk); rst_n = 1; repeat (2) @(negedge clk);

    // 1. bypass is transparent
    run_fold(256);
    check(cyc, 256, "bypass");
    if (stall_cycles !== 0) begin
      $display("FAIL  bypass: %0d stalls, want 0", stall_cycles);
      errors = errors + 1;
    end

    // 2. knee: beta = 2N is cycle-identical to bypass
    beta_bypass = 0;
    beta_q8_8   = WORDS_PER_STEP << 8;
    run_fold(256);
    check(cyc, 256, "knee (beta = 2N)");
    if (stall_cycles !== 0) begin
      $display("FAIL  knee: %0d stalls, want 0", stall_cycles);
      errors = errors + 1;
    end

    // 3. exact rates below the knee, back to back (no credit carry-over)
    check_rate(8, 64);
    check_rate(4, 64);
    check_rate(2, 64);
    check_rate(1, 64);

    if (errors == 0) $display("\nALL CHECKS PASSED");
    else             $display("\n%0d CHECK(S) FAILED", errors);
    $finish;
  end

endmodule
