/*
 * fp_ieee.sv -- fp_mul / fp_add for the board co-simulation.  BENCH ONLY.
 *
 * tb/fp_model.sv, which every other bench uses, multiplies and adds the
 * 32-bit words as integers: right for checking the PE's control path, wrong
 * for a host that sends IEEE-754 floats.  The co-simulation runs the real
 * runtime against the real RTL, so the stand-in for the Xilinx IP has to be
 * floating point:
 *
 *   - binary32, round to nearest even (C float arithmetic, see
 *     dpti_cosim.cpp -- Verilator promotes shortreal to double, so the
 *     arithmetic is done on the C side);
 *   - denormal inputs and outputs flushed to signed zero, as the
 *     Floating-Point Operator IP does;
 *   - the latencies the IP was generated with (ip/fp32/*.xci C_Latency:
 *     mul 8, add 11), one result per valid_in, in order -- the same
 *     contract fp_model keeps.
 *
 * The integer test operands the E2E regression uses are exact in binary32,
 * so on them this model and the IP agree bit for bit; it is not a claim
 * about every input.
 */

`ifndef FP_MUL_LAT
`define FP_MUL_LAT 8
`endif
`ifndef FP_ADD_LAT
`define FP_ADD_LAT 11
`endif

import "DPI-C" pure function int unsigned cosim_f32_mul(input int unsigned a,
                                                        input int unsigned b);
import "DPI-C" pure function int unsigned cosim_f32_add(input int unsigned a,
                                                        input int unsigned b);

module fp_mul #(parameter int LAT = `FP_MUL_LAT) (
    input  logic        clk,
    input  logic        rst,
    input  logic        valid_in,
    input  logic [31:0] a,
    input  logic [31:0] b,
    output logic        valid_out,
    output logic [31:0] result
);
    logic        v_pipe [0:LAT-1];
    logic [31:0] d_pipe [0:LAT-1];

    always_ff @(posedge clk) begin
        if (rst) begin
            for (int i = 0; i < LAT; i++) begin
                v_pipe[i] <= 1'b0;
                d_pipe[i] <= 32'd0;
            end
        end
        else begin
            v_pipe[0] <= valid_in;
            d_pipe[0] <= cosim_f32_mul(a, b);
            for (int i = 1; i < LAT; i++) begin
                v_pipe[i] <= v_pipe[i-1];
                d_pipe[i] <= d_pipe[i-1];
            end
        end
    end

    assign valid_out = v_pipe[LAT-1];
    assign result    = d_pipe[LAT-1];
endmodule


module fp_add #(parameter int LAT = `FP_ADD_LAT) (
    input  logic        clk,
    input  logic        rst,
    input  logic        valid_in,
    input  logic [31:0] a,
    input  logic [31:0] b,
    output logic        valid_out,
    output logic [31:0] result
);
    logic        v_pipe [0:LAT-1];
    logic [31:0] d_pipe [0:LAT-1];

    always_ff @(posedge clk) begin
        if (rst) begin
            for (int i = 0; i < LAT; i++) begin
                v_pipe[i] <= 1'b0;
                d_pipe[i] <= 32'd0;
            end
        end
        else begin
            v_pipe[0] <= valid_in;
            d_pipe[0] <= cosim_f32_add(a, b);
            for (int i = 1; i < LAT; i++) begin
                v_pipe[i] <= v_pipe[i-1];
                d_pipe[i] <= d_pipe[i-1];
            end
        end
    end

    assign valid_out = v_pipe[LAT-1];
    assign result    = d_pipe[LAT-1];
endmodule
