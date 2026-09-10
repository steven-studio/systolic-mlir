`default_nettype none
`timescale 1ns / 1ps

/*
 * xil_stubs.sv -- simulation-only replacements for the Xilinx primitives and
 * IP that systolic_dma_top instantiates.  BENCH ONLY.  This file is not in
 * dma_top_build.tcl's source list and must never be: on the board these names
 * resolve to the real primitives.
 *
 * The arithmetic is NOT stubbed here.  tb/fp_model.sv already replaces fp_mul
 * and fp_add at the wrapper level with integer pipelines of the same latency,
 * and it is what every other bench in this tree uses; compiling it in place of
 * core/fp_mul.sv and core/fp_add.sv keeps this bench on the same model the
 * array's own benches are written against, rather than inventing a second one.
 */

module IBUF (input wire I, output wire O); assign O = I; endmodule
module BUFG (input wire I, output wire O); assign O = I; endmodule

module MMCME2_BASE #(
  parameter BANDWIDTH = "OPTIMIZED", parameter real CLKIN1_PERIOD = 10.0,
  parameter integer DIVCLK_DIVIDE = 1, parameter real CLKFBOUT_MULT_F = 10.0,
  parameter real CLKFBOUT_PHASE = 0.0, parameter real CLKOUT0_DIVIDE_F = 5.0,
  parameter real CLKOUT0_DUTY_CYCLE = 0.5, parameter real CLKOUT0_PHASE = 0.0,
  parameter real REF_JITTER1 = 0.01, parameter STARTUP_WAIT = "FALSE"
) (
  input wire CLKIN1, input wire CLKFBIN,
  output wire CLKFBOUT, output wire CLKFBOUTB,
  output wire CLKOUT0, output wire CLKOUT0B,
  output wire CLKOUT1, output wire CLKOUT1B,
  output wire CLKOUT2, output wire CLKOUT2B,
  output wire CLKOUT3, output wire CLKOUT3B,
  output wire CLKOUT4, output wire CLKOUT5, output wire CLKOUT6,
  output wire LOCKED, input wire PWRDWN, input wire RST
);
  assign CLKFBOUT = CLKIN1;
  assign CLKOUT0  = CLKIN1;   // the 200 MHz IDELAYCTRL reference is unused here
  assign LOCKED   = 1'b1;
endmodule

module vio_0 (
  input wire clk,
  input wire [31:0] probe_in0,  input wire [31:0] probe_in1,
  input wire [31:0] probe_in2,  input wire [31:0] probe_in3,
  input wire [7:0]  probe_in4,
  // the nine operand-path counters (fill, engine x3, wb, wb engine x4)
  input wire [31:0] probe_in5,  input wire [31:0] probe_in6,
  input wire [31:0] probe_in7,  input wire [31:0] probe_in8,
  input wire [31:0] probe_in9,  input wire [31:0] probe_in10,
  input wire [31:0] probe_in11, input wire [31:0] probe_in12,
  input wire [31:0] probe_in13,
  // the invocation loop's three: wall clock, summed compute, status word
  input wire [31:0] probe_in14, input wire [31:0] probe_in15,
  input wire [31:0] probe_in16,
  // OUTPUT probe.  On the board this is a JTAG-written register; here it is
  // +n_inv=<count> on the simulation command line, which is the same contract
  // -- the invocation count reaches the design from outside it, and the design
  // latches it once, on the way out of P_CALIB.
  output wire [3:0] probe_out0,
  // The re-run request.  On the board dma_top_build.tcl writes it over JTAG;
  // here the bench pokes rerun_arg directly, which is the same contract.
  output wire       probe_out1
);
  logic [3:0] n_inv_arg = 4'd1;
  logic       rerun_arg = 1'b0;
  int unsigned n_inv_plus;
  initial begin
    if ($value$plusargs("n_inv=%d", n_inv_plus)) n_inv_arg = 4'(n_inv_plus);
  end
  assign probe_out0 = n_inv_arg;
  assign probe_out1 = rerun_arg;
endmodule

`default_nettype wire
