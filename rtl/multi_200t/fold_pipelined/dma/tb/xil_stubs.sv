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
  input wire [31:0] probe_in0, input wire [31:0] probe_in1,
  input wire [31:0] probe_in2, input wire [31:0] probe_in3,
  input wire [7:0]  probe_in4
);
endmodule

`default_nettype wire
