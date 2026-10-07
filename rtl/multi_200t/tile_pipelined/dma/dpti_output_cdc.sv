`default_nettype none
`timescale 1ns / 1ps

module dpti_output_cdc #(
    parameter integer DATA_W = 128,
    parameter integer DEPTH  = 8
) (
    // Source domain: ui_clk.
    input  wire                 src_clk,
    input  wire                 src_rst,

    input  wire [DATA_W-1:0]    src_data,
    input  wire                 src_valid,
    output wire                 src_ready,

    // Destination domain: physical-interface clock.
    input  wire                 dst_clk,
    input  wire                 dst_rst,

    output wire [DATA_W-1:0]    dst_data,
    output wire                 dst_valid,
    input  wire                 dst_ready
);

    dpti_async_fifo #(
        .DATA_W (DATA_W),
        .DEPTH  (DEPTH)
    ) u_fifo (
        .wr_clk   (src_clk),
        .wr_rst   (src_rst),

        .wr_valid (src_valid),
        .wr_ready (src_ready),
        .wr_data  (src_data),

        .rd_clk   (dst_clk),
        .rd_rst   (dst_rst),

        .rd_valid (dst_valid),
        .rd_ready (dst_ready),
        .rd_data  (dst_data)
    );

endmodule

`default_nettype wire
