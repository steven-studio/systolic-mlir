`default_nettype none

module dpti_write32_rx #(
    parameter integer ADDR_W = 8
) (
    input  wire                  dpti_clkout,
    input  wire                  rst,

    input  wire [7:0]            dpti_d_in,
    input  wire                  dpti_rxf_n,

    output wire                  dpti_rd_n,
    output wire                  dpti_oe_n,

    output wire                  cmd_valid,
    input  wire                  cmd_ready,
    output wire [ADDR_W-1:0]     cmd_addr,
    output wire [31:0]           cmd_data,

    output wire                  err_opcode
);

    wire [7:0] byte_data;
    wire       byte_valid;
    wire       byte_ready;

    dpti_byte_rx u_byte_rx (
        .dpti_clkout (dpti_clkout),
        .rst         (rst),

        .dpti_d_in   (dpti_d_in),
        .dpti_rxf_n  (dpti_rxf_n),

        .dpti_rd_n   (dpti_rd_n),
        .dpti_oe_n   (dpti_oe_n),

        .byte_data   (byte_data),
        .byte_valid  (byte_valid),
        .byte_ready  (byte_ready)
    );

    dpti_write32_decoder #(
        .ADDR_W (ADDR_W)
    ) u_decoder (
        .clk        (dpti_clkout),
        .rst        (rst),

        .byte_data  (byte_data),
        .byte_valid (byte_valid),
        .byte_ready (byte_ready),

        .cmd_valid  (cmd_valid),
        .cmd_ready  (cmd_ready),

        .cmd_addr   (cmd_addr),
        .cmd_data   (cmd_data),

        .err_opcode (err_opcode)
    );

endmodule

`default_nettype wire
