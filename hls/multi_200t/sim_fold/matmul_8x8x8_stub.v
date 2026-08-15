// matmul_8x8x8 -- SIMULATION STUB. Not for synthesis.
//
// Same port list as the HLS core, integer arithmetic instead of fp32:
//   Cinout[i][j]_o = Cinout[i][j]_i + A_i_q0 + B_j_q0
// with address0 tied to 0, so the operand terms are word 0 of the
// currently selected fold. Latency mimics the real core (K + 14 + 11)
// so the controller's ap_start/ap_done handshake sees realistic
// timing rather than a combinational done.
`timescale 1ns / 1ps
module matmul_8x8x8 (
    input  wire        ap_clk,
    input  wire        ap_rst,
    input  wire        ap_start,
    output reg         ap_done,
    output wire        ap_idle,
    output wire        ap_ready,
    input  wire [31:0] K,
    output wire [5:0]  A_0_address0,
    output wire        A_0_ce0,
    input  wire [31:0] A_0_q0,
    output wire [5:0]  A_1_address0,
    output wire        A_1_ce0,
    input  wire [31:0] A_1_q0,
    output wire [5:0]  A_2_address0,
    output wire        A_2_ce0,
    input  wire [31:0] A_2_q0,
    output wire [5:0]  A_3_address0,
    output wire        A_3_ce0,
    input  wire [31:0] A_3_q0,
    output wire [5:0]  A_4_address0,
    output wire        A_4_ce0,
    input  wire [31:0] A_4_q0,
    output wire [5:0]  A_5_address0,
    output wire        A_5_ce0,
    input  wire [31:0] A_5_q0,
    output wire [5:0]  A_6_address0,
    output wire        A_6_ce0,
    input  wire [31:0] A_6_q0,
    output wire [5:0]  A_7_address0,
    output wire        A_7_ce0,
    input  wire [31:0] A_7_q0,
    output wire [5:0]  B_0_address0,
    output wire        B_0_ce0,
    input  wire [31:0] B_0_q0,
    output wire [5:0]  B_1_address0,
    output wire        B_1_ce0,
    input  wire [31:0] B_1_q0,
    output wire [5:0]  B_2_address0,
    output wire        B_2_ce0,
    input  wire [31:0] B_2_q0,
    output wire [5:0]  B_3_address0,
    output wire        B_3_ce0,
    input  wire [31:0] B_3_q0,
    output wire [5:0]  B_4_address0,
    output wire        B_4_ce0,
    input  wire [31:0] B_4_q0,
    output wire [5:0]  B_5_address0,
    output wire        B_5_ce0,
    input  wire [31:0] B_5_q0,
    output wire [5:0]  B_6_address0,
    output wire        B_6_ce0,
    input  wire [31:0] B_6_q0,
    output wire [5:0]  B_7_address0,
    output wire        B_7_ce0,
    input  wire [31:0] B_7_q0,
    input  wire [31:0] Cinout_0_0_i,
    output wire [31:0] Cinout_0_0_o,
    output wire        Cinout_0_0_o_ap_vld,
    input  wire [31:0] Cinout_0_1_i,
    output wire [31:0] Cinout_0_1_o,
    output wire        Cinout_0_1_o_ap_vld,
    input  wire [31:0] Cinout_0_2_i,
    output wire [31:0] Cinout_0_2_o,
    output wire        Cinout_0_2_o_ap_vld,
    input  wire [31:0] Cinout_0_3_i,
    output wire [31:0] Cinout_0_3_o,
    output wire        Cinout_0_3_o_ap_vld,
    input  wire [31:0] Cinout_0_4_i,
    output wire [31:0] Cinout_0_4_o,
    output wire        Cinout_0_4_o_ap_vld,
    input  wire [31:0] Cinout_0_5_i,
    output wire [31:0] Cinout_0_5_o,
    output wire        Cinout_0_5_o_ap_vld,
    input  wire [31:0] Cinout_0_6_i,
    output wire [31:0] Cinout_0_6_o,
    output wire        Cinout_0_6_o_ap_vld,
    input  wire [31:0] Cinout_0_7_i,
    output wire [31:0] Cinout_0_7_o,
    output wire        Cinout_0_7_o_ap_vld,
    input  wire [31:0] Cinout_1_0_i,
    output wire [31:0] Cinout_1_0_o,
    output wire        Cinout_1_0_o_ap_vld,
    input  wire [31:0] Cinout_1_1_i,
    output wire [31:0] Cinout_1_1_o,
    output wire        Cinout_1_1_o_ap_vld,
    input  wire [31:0] Cinout_1_2_i,
    output wire [31:0] Cinout_1_2_o,
    output wire        Cinout_1_2_o_ap_vld,
    input  wire [31:0] Cinout_1_3_i,
    output wire [31:0] Cinout_1_3_o,
    output wire        Cinout_1_3_o_ap_vld,
    input  wire [31:0] Cinout_1_4_i,
    output wire [31:0] Cinout_1_4_o,
    output wire        Cinout_1_4_o_ap_vld,
    input  wire [31:0] Cinout_1_5_i,
    output wire [31:0] Cinout_1_5_o,
    output wire        Cinout_1_5_o_ap_vld,
    input  wire [31:0] Cinout_1_6_i,
    output wire [31:0] Cinout_1_6_o,
    output wire        Cinout_1_6_o_ap_vld,
    input  wire [31:0] Cinout_1_7_i,
    output wire [31:0] Cinout_1_7_o,
    output wire        Cinout_1_7_o_ap_vld,
    input  wire [31:0] Cinout_2_0_i,
    output wire [31:0] Cinout_2_0_o,
    output wire        Cinout_2_0_o_ap_vld,
    input  wire [31:0] Cinout_2_1_i,
    output wire [31:0] Cinout_2_1_o,
    output wire        Cinout_2_1_o_ap_vld,
    input  wire [31:0] Cinout_2_2_i,
    output wire [31:0] Cinout_2_2_o,
    output wire        Cinout_2_2_o_ap_vld,
    input  wire [31:0] Cinout_2_3_i,
    output wire [31:0] Cinout_2_3_o,
    output wire        Cinout_2_3_o_ap_vld,
    input  wire [31:0] Cinout_2_4_i,
    output wire [31:0] Cinout_2_4_o,
    output wire        Cinout_2_4_o_ap_vld,
    input  wire [31:0] Cinout_2_5_i,
    output wire [31:0] Cinout_2_5_o,
    output wire        Cinout_2_5_o_ap_vld,
    input  wire [31:0] Cinout_2_6_i,
    output wire [31:0] Cinout_2_6_o,
    output wire        Cinout_2_6_o_ap_vld,
    input  wire [31:0] Cinout_2_7_i,
    output wire [31:0] Cinout_2_7_o,
    output wire        Cinout_2_7_o_ap_vld,
    input  wire [31:0] Cinout_3_0_i,
    output wire [31:0] Cinout_3_0_o,
    output wire        Cinout_3_0_o_ap_vld,
    input  wire [31:0] Cinout_3_1_i,
    output wire [31:0] Cinout_3_1_o,
    output wire        Cinout_3_1_o_ap_vld,
    input  wire [31:0] Cinout_3_2_i,
    output wire [31:0] Cinout_3_2_o,
    output wire        Cinout_3_2_o_ap_vld,
    input  wire [31:0] Cinout_3_3_i,
    output wire [31:0] Cinout_3_3_o,
    output wire        Cinout_3_3_o_ap_vld,
    input  wire [31:0] Cinout_3_4_i,
    output wire [31:0] Cinout_3_4_o,
    output wire        Cinout_3_4_o_ap_vld,
    input  wire [31:0] Cinout_3_5_i,
    output wire [31:0] Cinout_3_5_o,
    output wire        Cinout_3_5_o_ap_vld,
    input  wire [31:0] Cinout_3_6_i,
    output wire [31:0] Cinout_3_6_o,
    output wire        Cinout_3_6_o_ap_vld,
    input  wire [31:0] Cinout_3_7_i,
    output wire [31:0] Cinout_3_7_o,
    output wire        Cinout_3_7_o_ap_vld,
    input  wire [31:0] Cinout_4_0_i,
    output wire [31:0] Cinout_4_0_o,
    output wire        Cinout_4_0_o_ap_vld,
    input  wire [31:0] Cinout_4_1_i,
    output wire [31:0] Cinout_4_1_o,
    output wire        Cinout_4_1_o_ap_vld,
    input  wire [31:0] Cinout_4_2_i,
    output wire [31:0] Cinout_4_2_o,
    output wire        Cinout_4_2_o_ap_vld,
    input  wire [31:0] Cinout_4_3_i,
    output wire [31:0] Cinout_4_3_o,
    output wire        Cinout_4_3_o_ap_vld,
    input  wire [31:0] Cinout_4_4_i,
    output wire [31:0] Cinout_4_4_o,
    output wire        Cinout_4_4_o_ap_vld,
    input  wire [31:0] Cinout_4_5_i,
    output wire [31:0] Cinout_4_5_o,
    output wire        Cinout_4_5_o_ap_vld,
    input  wire [31:0] Cinout_4_6_i,
    output wire [31:0] Cinout_4_6_o,
    output wire        Cinout_4_6_o_ap_vld,
    input  wire [31:0] Cinout_4_7_i,
    output wire [31:0] Cinout_4_7_o,
    output wire        Cinout_4_7_o_ap_vld,
    input  wire [31:0] Cinout_5_0_i,
    output wire [31:0] Cinout_5_0_o,
    output wire        Cinout_5_0_o_ap_vld,
    input  wire [31:0] Cinout_5_1_i,
    output wire [31:0] Cinout_5_1_o,
    output wire        Cinout_5_1_o_ap_vld,
    input  wire [31:0] Cinout_5_2_i,
    output wire [31:0] Cinout_5_2_o,
    output wire        Cinout_5_2_o_ap_vld,
    input  wire [31:0] Cinout_5_3_i,
    output wire [31:0] Cinout_5_3_o,
    output wire        Cinout_5_3_o_ap_vld,
    input  wire [31:0] Cinout_5_4_i,
    output wire [31:0] Cinout_5_4_o,
    output wire        Cinout_5_4_o_ap_vld,
    input  wire [31:0] Cinout_5_5_i,
    output wire [31:0] Cinout_5_5_o,
    output wire        Cinout_5_5_o_ap_vld,
    input  wire [31:0] Cinout_5_6_i,
    output wire [31:0] Cinout_5_6_o,
    output wire        Cinout_5_6_o_ap_vld,
    input  wire [31:0] Cinout_5_7_i,
    output wire [31:0] Cinout_5_7_o,
    output wire        Cinout_5_7_o_ap_vld,
    input  wire [31:0] Cinout_6_0_i,
    output wire [31:0] Cinout_6_0_o,
    output wire        Cinout_6_0_o_ap_vld,
    input  wire [31:0] Cinout_6_1_i,
    output wire [31:0] Cinout_6_1_o,
    output wire        Cinout_6_1_o_ap_vld,
    input  wire [31:0] Cinout_6_2_i,
    output wire [31:0] Cinout_6_2_o,
    output wire        Cinout_6_2_o_ap_vld,
    input  wire [31:0] Cinout_6_3_i,
    output wire [31:0] Cinout_6_3_o,
    output wire        Cinout_6_3_o_ap_vld,
    input  wire [31:0] Cinout_6_4_i,
    output wire [31:0] Cinout_6_4_o,
    output wire        Cinout_6_4_o_ap_vld,
    input  wire [31:0] Cinout_6_5_i,
    output wire [31:0] Cinout_6_5_o,
    output wire        Cinout_6_5_o_ap_vld,
    input  wire [31:0] Cinout_6_6_i,
    output wire [31:0] Cinout_6_6_o,
    output wire        Cinout_6_6_o_ap_vld,
    input  wire [31:0] Cinout_6_7_i,
    output wire [31:0] Cinout_6_7_o,
    output wire        Cinout_6_7_o_ap_vld,
    input  wire [31:0] Cinout_7_0_i,
    output wire [31:0] Cinout_7_0_o,
    output wire        Cinout_7_0_o_ap_vld,
    input  wire [31:0] Cinout_7_1_i,
    output wire [31:0] Cinout_7_1_o,
    output wire        Cinout_7_1_o_ap_vld,
    input  wire [31:0] Cinout_7_2_i,
    output wire [31:0] Cinout_7_2_o,
    output wire        Cinout_7_2_o_ap_vld,
    input  wire [31:0] Cinout_7_3_i,
    output wire [31:0] Cinout_7_3_o,
    output wire        Cinout_7_3_o_ap_vld,
    input  wire [31:0] Cinout_7_4_i,
    output wire [31:0] Cinout_7_4_o,
    output wire        Cinout_7_4_o_ap_vld,
    input  wire [31:0] Cinout_7_5_i,
    output wire [31:0] Cinout_7_5_o,
    output wire        Cinout_7_5_o_ap_vld,
    input  wire [31:0] Cinout_7_6_i,
    output wire [31:0] Cinout_7_6_o,
    output wire        Cinout_7_6_o_ap_vld,
    input  wire [31:0] Cinout_7_7_i,
    output wire [31:0] Cinout_7_7_o,
    output wire        Cinout_7_7_o_ap_vld
);

    assign A_0_address0 = 6'd0;
    assign A_0_ce0      = 1'b1;
    assign A_1_address0 = 6'd0;
    assign A_1_ce0      = 1'b1;
    assign A_2_address0 = 6'd0;
    assign A_2_ce0      = 1'b1;
    assign A_3_address0 = 6'd0;
    assign A_3_ce0      = 1'b1;
    assign A_4_address0 = 6'd0;
    assign A_4_ce0      = 1'b1;
    assign A_5_address0 = 6'd0;
    assign A_5_ce0      = 1'b1;
    assign A_6_address0 = 6'd0;
    assign A_6_ce0      = 1'b1;
    assign A_7_address0 = 6'd0;
    assign A_7_ce0      = 1'b1;
    assign B_0_address0 = 6'd0;
    assign B_0_ce0      = 1'b1;
    assign B_1_address0 = 6'd0;
    assign B_1_ce0      = 1'b1;
    assign B_2_address0 = 6'd0;
    assign B_2_ce0      = 1'b1;
    assign B_3_address0 = 6'd0;
    assign B_3_ce0      = 1'b1;
    assign B_4_address0 = 6'd0;
    assign B_4_ce0      = 1'b1;
    assign B_5_address0 = 6'd0;
    assign B_5_ce0      = 1'b1;
    assign B_6_address0 = 6'd0;
    assign B_6_ce0      = 1'b1;
    assign B_7_address0 = 6'd0;
    assign B_7_ce0      = 1'b1;

    reg  [31:0] cnt;
    reg         busy;
    assign ap_idle  = ~busy;
    assign ap_ready = ap_done;

    always @(posedge ap_clk) begin
        if (ap_rst) begin
            busy <= 0; cnt <= 0; ap_done <= 0;
        end else begin
            ap_done <= 0;
            if (!busy && ap_start) begin
                busy <= 1;
                cnt  <= K + 32'd25;      // K + (R+C-2) + c0
            end else if (busy) begin
                if (cnt == 0) begin
                    busy <= 0; ap_done <= 1;
                end else begin
                    cnt <= cnt - 1;
                end
            end
        end
    end

    assign Cinout_0_0_o = Cinout_0_0_i + A_0_q0 + B_0_q0;
    assign Cinout_0_0_o_ap_vld = ap_done;
    assign Cinout_0_1_o = Cinout_0_1_i + A_0_q0 + B_1_q0;
    assign Cinout_0_1_o_ap_vld = ap_done;
    assign Cinout_0_2_o = Cinout_0_2_i + A_0_q0 + B_2_q0;
    assign Cinout_0_2_o_ap_vld = ap_done;
    assign Cinout_0_3_o = Cinout_0_3_i + A_0_q0 + B_3_q0;
    assign Cinout_0_3_o_ap_vld = ap_done;
    assign Cinout_0_4_o = Cinout_0_4_i + A_0_q0 + B_4_q0;
    assign Cinout_0_4_o_ap_vld = ap_done;
    assign Cinout_0_5_o = Cinout_0_5_i + A_0_q0 + B_5_q0;
    assign Cinout_0_5_o_ap_vld = ap_done;
    assign Cinout_0_6_o = Cinout_0_6_i + A_0_q0 + B_6_q0;
    assign Cinout_0_6_o_ap_vld = ap_done;
    assign Cinout_0_7_o = Cinout_0_7_i + A_0_q0 + B_7_q0;
    assign Cinout_0_7_o_ap_vld = ap_done;
    assign Cinout_1_0_o = Cinout_1_0_i + A_1_q0 + B_0_q0;
    assign Cinout_1_0_o_ap_vld = ap_done;
    assign Cinout_1_1_o = Cinout_1_1_i + A_1_q0 + B_1_q0;
    assign Cinout_1_1_o_ap_vld = ap_done;
    assign Cinout_1_2_o = Cinout_1_2_i + A_1_q0 + B_2_q0;
    assign Cinout_1_2_o_ap_vld = ap_done;
    assign Cinout_1_3_o = Cinout_1_3_i + A_1_q0 + B_3_q0;
    assign Cinout_1_3_o_ap_vld = ap_done;
    assign Cinout_1_4_o = Cinout_1_4_i + A_1_q0 + B_4_q0;
    assign Cinout_1_4_o_ap_vld = ap_done;
    assign Cinout_1_5_o = Cinout_1_5_i + A_1_q0 + B_5_q0;
    assign Cinout_1_5_o_ap_vld = ap_done;
    assign Cinout_1_6_o = Cinout_1_6_i + A_1_q0 + B_6_q0;
    assign Cinout_1_6_o_ap_vld = ap_done;
    assign Cinout_1_7_o = Cinout_1_7_i + A_1_q0 + B_7_q0;
    assign Cinout_1_7_o_ap_vld = ap_done;
    assign Cinout_2_0_o = Cinout_2_0_i + A_2_q0 + B_0_q0;
    assign Cinout_2_0_o_ap_vld = ap_done;
    assign Cinout_2_1_o = Cinout_2_1_i + A_2_q0 + B_1_q0;
    assign Cinout_2_1_o_ap_vld = ap_done;
    assign Cinout_2_2_o = Cinout_2_2_i + A_2_q0 + B_2_q0;
    assign Cinout_2_2_o_ap_vld = ap_done;
    assign Cinout_2_3_o = Cinout_2_3_i + A_2_q0 + B_3_q0;
    assign Cinout_2_3_o_ap_vld = ap_done;
    assign Cinout_2_4_o = Cinout_2_4_i + A_2_q0 + B_4_q0;
    assign Cinout_2_4_o_ap_vld = ap_done;
    assign Cinout_2_5_o = Cinout_2_5_i + A_2_q0 + B_5_q0;
    assign Cinout_2_5_o_ap_vld = ap_done;
    assign Cinout_2_6_o = Cinout_2_6_i + A_2_q0 + B_6_q0;
    assign Cinout_2_6_o_ap_vld = ap_done;
    assign Cinout_2_7_o = Cinout_2_7_i + A_2_q0 + B_7_q0;
    assign Cinout_2_7_o_ap_vld = ap_done;
    assign Cinout_3_0_o = Cinout_3_0_i + A_3_q0 + B_0_q0;
    assign Cinout_3_0_o_ap_vld = ap_done;
    assign Cinout_3_1_o = Cinout_3_1_i + A_3_q0 + B_1_q0;
    assign Cinout_3_1_o_ap_vld = ap_done;
    assign Cinout_3_2_o = Cinout_3_2_i + A_3_q0 + B_2_q0;
    assign Cinout_3_2_o_ap_vld = ap_done;
    assign Cinout_3_3_o = Cinout_3_3_i + A_3_q0 + B_3_q0;
    assign Cinout_3_3_o_ap_vld = ap_done;
    assign Cinout_3_4_o = Cinout_3_4_i + A_3_q0 + B_4_q0;
    assign Cinout_3_4_o_ap_vld = ap_done;
    assign Cinout_3_5_o = Cinout_3_5_i + A_3_q0 + B_5_q0;
    assign Cinout_3_5_o_ap_vld = ap_done;
    assign Cinout_3_6_o = Cinout_3_6_i + A_3_q0 + B_6_q0;
    assign Cinout_3_6_o_ap_vld = ap_done;
    assign Cinout_3_7_o = Cinout_3_7_i + A_3_q0 + B_7_q0;
    assign Cinout_3_7_o_ap_vld = ap_done;
    assign Cinout_4_0_o = Cinout_4_0_i + A_4_q0 + B_0_q0;
    assign Cinout_4_0_o_ap_vld = ap_done;
    assign Cinout_4_1_o = Cinout_4_1_i + A_4_q0 + B_1_q0;
    assign Cinout_4_1_o_ap_vld = ap_done;
    assign Cinout_4_2_o = Cinout_4_2_i + A_4_q0 + B_2_q0;
    assign Cinout_4_2_o_ap_vld = ap_done;
    assign Cinout_4_3_o = Cinout_4_3_i + A_4_q0 + B_3_q0;
    assign Cinout_4_3_o_ap_vld = ap_done;
    assign Cinout_4_4_o = Cinout_4_4_i + A_4_q0 + B_4_q0;
    assign Cinout_4_4_o_ap_vld = ap_done;
    assign Cinout_4_5_o = Cinout_4_5_i + A_4_q0 + B_5_q0;
    assign Cinout_4_5_o_ap_vld = ap_done;
    assign Cinout_4_6_o = Cinout_4_6_i + A_4_q0 + B_6_q0;
    assign Cinout_4_6_o_ap_vld = ap_done;
    assign Cinout_4_7_o = Cinout_4_7_i + A_4_q0 + B_7_q0;
    assign Cinout_4_7_o_ap_vld = ap_done;
    assign Cinout_5_0_o = Cinout_5_0_i + A_5_q0 + B_0_q0;
    assign Cinout_5_0_o_ap_vld = ap_done;
    assign Cinout_5_1_o = Cinout_5_1_i + A_5_q0 + B_1_q0;
    assign Cinout_5_1_o_ap_vld = ap_done;
    assign Cinout_5_2_o = Cinout_5_2_i + A_5_q0 + B_2_q0;
    assign Cinout_5_2_o_ap_vld = ap_done;
    assign Cinout_5_3_o = Cinout_5_3_i + A_5_q0 + B_3_q0;
    assign Cinout_5_3_o_ap_vld = ap_done;
    assign Cinout_5_4_o = Cinout_5_4_i + A_5_q0 + B_4_q0;
    assign Cinout_5_4_o_ap_vld = ap_done;
    assign Cinout_5_5_o = Cinout_5_5_i + A_5_q0 + B_5_q0;
    assign Cinout_5_5_o_ap_vld = ap_done;
    assign Cinout_5_6_o = Cinout_5_6_i + A_5_q0 + B_6_q0;
    assign Cinout_5_6_o_ap_vld = ap_done;
    assign Cinout_5_7_o = Cinout_5_7_i + A_5_q0 + B_7_q0;
    assign Cinout_5_7_o_ap_vld = ap_done;
    assign Cinout_6_0_o = Cinout_6_0_i + A_6_q0 + B_0_q0;
    assign Cinout_6_0_o_ap_vld = ap_done;
    assign Cinout_6_1_o = Cinout_6_1_i + A_6_q0 + B_1_q0;
    assign Cinout_6_1_o_ap_vld = ap_done;
    assign Cinout_6_2_o = Cinout_6_2_i + A_6_q0 + B_2_q0;
    assign Cinout_6_2_o_ap_vld = ap_done;
    assign Cinout_6_3_o = Cinout_6_3_i + A_6_q0 + B_3_q0;
    assign Cinout_6_3_o_ap_vld = ap_done;
    assign Cinout_6_4_o = Cinout_6_4_i + A_6_q0 + B_4_q0;
    assign Cinout_6_4_o_ap_vld = ap_done;
    assign Cinout_6_5_o = Cinout_6_5_i + A_6_q0 + B_5_q0;
    assign Cinout_6_5_o_ap_vld = ap_done;
    assign Cinout_6_6_o = Cinout_6_6_i + A_6_q0 + B_6_q0;
    assign Cinout_6_6_o_ap_vld = ap_done;
    assign Cinout_6_7_o = Cinout_6_7_i + A_6_q0 + B_7_q0;
    assign Cinout_6_7_o_ap_vld = ap_done;
    assign Cinout_7_0_o = Cinout_7_0_i + A_7_q0 + B_0_q0;
    assign Cinout_7_0_o_ap_vld = ap_done;
    assign Cinout_7_1_o = Cinout_7_1_i + A_7_q0 + B_1_q0;
    assign Cinout_7_1_o_ap_vld = ap_done;
    assign Cinout_7_2_o = Cinout_7_2_i + A_7_q0 + B_2_q0;
    assign Cinout_7_2_o_ap_vld = ap_done;
    assign Cinout_7_3_o = Cinout_7_3_i + A_7_q0 + B_3_q0;
    assign Cinout_7_3_o_ap_vld = ap_done;
    assign Cinout_7_4_o = Cinout_7_4_i + A_7_q0 + B_4_q0;
    assign Cinout_7_4_o_ap_vld = ap_done;
    assign Cinout_7_5_o = Cinout_7_5_i + A_7_q0 + B_5_q0;
    assign Cinout_7_5_o_ap_vld = ap_done;
    assign Cinout_7_6_o = Cinout_7_6_i + A_7_q0 + B_6_q0;
    assign Cinout_7_6_o_ap_vld = ap_done;
    assign Cinout_7_7_o = Cinout_7_7_i + A_7_q0 + B_7_q0;
    assign Cinout_7_7_o_ap_vld = ap_done;

endmodule
