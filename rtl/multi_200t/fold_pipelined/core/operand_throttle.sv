// -----------------------------------------------------------------------------
// operand_throttle.sv
//
// A programmable operand-rate limiter for the fp32 output-stationary systolic
// array.  Sits between the operand-buffer read sequencer and the PE array and
// gates the "advance one reduction step" handshake, so that the array is fed at
// a configurable rate of beta operand words per cycle.
//
// WHY THIS EXISTS
//   The fleet cost model treats bandwidth beta as a free parameter and sweeps
//   it.  This module makes beta an *independent variable on hardware*: hold the
//   fold fixed, sweep beta, read T(beta) from CYCLE_COUNTER.  Two results fall
//   out of one sweep:
//     (1) the knee of T(beta) is the array's measured operand demand, which
//         should sit at 2N words per reduction step -- a direct hardware check
//         of the traffic term (2K/s + 1 words per output element);
//     (2) H should be INVARIANT in beta, because H is the tail after the last
//         operand is consumed.  If H moves with beta, H is a transfer artifact
//         and the paper's central claim is wrong.  This is the cheapest
//         falsification test of H that exists on this board.
//
// INTERFACE ASSUMPTIONS  (adapt names to the actual core -- marked ADAPT)
//   The array's k-sequencer is assumed to advance one reduction step per cycle
//   via a simple request/grant or enable signal.  Insert this module by
//   renaming that enable:  step_req is the sequencer's original enable, and
//   step_gnt becomes the enable actually consumed by the datapath.  Everything
//   downstream (BRAM addressing, bank rotation, the tree, the tail) is
//   untouched, so beta_bypass=1 must reproduce today's cycle counts exactly.
//
// RATE MODEL
//   Each reduction step consumes WORDS_PER_STEP = 2*N operand words (N words of
//   A across the rows, N words of B across the columns).  beta is expressed in
//   Q8.8 words per cycle: beta_q8_8 = 16'h0100 is 1.00 word/cycle,
//   16'h1000 is 16.00 words/cycle.  A token bucket accumulates beta_q8_8 every
//   cycle and spends WORDS_PER_STEP<<8 per granted step.  Credit is capped at
//   one step so that idle time between folds cannot be banked and spent as a
//   burst -- a hard rate limit, which is what the model assumes.
// -----------------------------------------------------------------------------

`default_nettype none

module operand_throttle #(
  parameter int unsigned N     = 8,          // array side
  parameter int unsigned ACC_W = 32          // token accumulator width
) (
  input  wire                 clk,
  input  wire                 rst_n,

  // ---- configuration: write before a fold is issued, hold stable ----------
  input  wire                 beta_bypass,   // 1 = unthrottled (baseline path)
  input  wire [15:0]          beta_q8_8,     // operand words per cycle, Q8.8

  // ---- gated handshake ---------------------------------------------------
  input  wire                 step_req,      // sequencer wants to advance
  output wire                 step_gnt,      // datapath enable (use this one)

  // ---- observability (read back with the cycle count) --------------------
  output logic [31:0]         stall_cycles,  // cycles where req && !gnt
  output logic [31:0]         step_count,    // granted steps this fold
  input  wire                 stat_clear     // pulse at fold start
);

  // Words consumed per granted reduction step, in Q8.8.
  localparam int unsigned WORDS_PER_STEP = 2 * N;
  localparam logic [ACC_W-1:0] STEP_COST =
      ACC_W'(WORDS_PER_STEP) << 8;

  logic [ACC_W-1:0] acc;

  // Credit including this cycle's tokens.  Testing the *incoming* balance
  // rather than the registered one removes a one-cycle priming latency that
  // would otherwise be charged once per fold whenever the throttle is enabled
  // -- a constant that would show up as "H grew by 1 when throttled" and
  // contaminate the H-invariance test.  With this form, beta = 2N is
  // cycle-identical to bypass.
  wire [ACC_W-1:0] acc_incoming = acc + ACC_W'(beta_q8_8);
  wire have_credit = (acc_incoming >= STEP_COST);
  assign step_gnt  = step_req & (beta_bypass | have_credit);

  // Token bucket.  Credit accrues every cycle and is capped at one step's
  // worth, so the limiter is a rate limit and not a burst allowance.
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      acc <= '0;
    end else if (beta_bypass || stat_clear) begin
      // Parked in bypass; cleared at fold start so that credit accrued while
      // the array was idle between folds cannot be spent as a burst on the
      // next fold.  Without this, T(beta) depends on what ran before it and
      // the sweep is not reproducible.
      acc <= '0;
    end else begin
      logic [ACC_W-1:0] next;
      next = acc_incoming;
      if (step_gnt) begin
        // Spend one step.  next >= STEP_COST is guaranteed when step_gnt is
        // asserted in the non-bypass path, so this cannot underflow.
        next = next - STEP_COST;
      end
      // Cap at one step of standing credit.
      acc <= (next > STEP_COST) ? STEP_COST : next;
    end
  end

  // ---- statistics --------------------------------------------------------
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      stall_cycles <= '0;
      step_count   <= '0;
    end else if (stat_clear) begin
      stall_cycles <= '0;
      step_count   <= '0;
    end else begin
      if (step_req && !step_gnt) stall_cycles <= stall_cycles + 1;
      if (step_gnt)              step_count   <= step_count   + 1;
    end
  end

`ifdef FORMAL_OR_SIM_ASSERTS
  // Bypass must be transparent: this is the regression that protects every
  // number already in the paper.
  assert property (@(posedge clk) disable iff (!rst_n)
                   beta_bypass |-> (step_gnt == step_req));
  // Never grant without credit in throttled mode.
  assert property (@(posedge clk) disable iff (!rst_n)
                   (!beta_bypass && step_gnt) |-> have_credit);
`endif

endmodule

`default_nettype wire
