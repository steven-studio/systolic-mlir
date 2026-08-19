module fp_add (
    input  logic        clk,
    input  logic        rst,

    input  logic        valid_in,
    input  logic [31:0] a,
    input  logic [31:0] b,

    output logic        valid_out,
    output logic [31:0] result
);

    floating_point_add_0 u_add (
        .aclk                 (clk),

        .s_axis_a_tvalid      (valid_in),
        .s_axis_a_tdata       (a),

        .s_axis_b_tvalid      (valid_in),
        .s_axis_b_tdata       (b),

        .m_axis_result_tvalid (valid_out),
        .m_axis_result_tdata  (result)
    );

endmodule