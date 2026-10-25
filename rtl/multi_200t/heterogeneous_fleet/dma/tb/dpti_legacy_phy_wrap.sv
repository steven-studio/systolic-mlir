`default_nettype none

// ============================================================================
// dpti_legacy_phy_wrap.sv -- BENCH ONLY.
//
// Presents the EXISTING physical layer (dpti_byte_rx + dpti_byte_tx, both
// unmodified) behind the same port list as dpti_sync245_phy, so that
// tb_dpti_sync245_phy.sv can run the same FT2232H model and checks against it.
//
// The glue is exactly what systolic_dma_top does today (systolic_dma_top.sv,
// "Physical data bus ownership is selected by the output path"):
//   dpti_d = dpti_tx_oe ? dpti_tx_data : 8'bz;   dpti_wr_n = dpti_tx_wr_n;
//   byte_rx reads dpti_d, drives dpti_rd_n / dpti_oe_n.
// There is no arbitration between the two directions in that glue, and none is
// added here.  The reset is the top's synchronous dpti_rst, modelled by a
// two-flop synchronizer like the top's.
// ============================================================================

module dpti_legacy_phy_wrap (
    input  wire        clk,
    input  wire        arst,

    input  wire [7:0]  pad_d_i,
    output wire [7:0]  pad_d_o,
    output wire [7:0]  pad_d_t,
    input  wire        pad_rxf_n,
    input  wire        pad_txe_n,
    output wire        pad_rd_n,
    output wire        pad_oe_n,
    output wire        pad_wr_n,

    output wire [7:0]  rx_data,
    output wire        rx_valid,
    input  wire        rx_ready,

    input  wire [7:0]  tx_data,
    input  wire        tx_valid,
    output wire        tx_ready
);

    // systolic_dma_top: dpti_rst_meta / dpti_rst, synchronous, two stages.
    reg rst_meta = 1'b1;
    reg rst      = 1'b1;
    always @(posedge clk) begin
        if (arst) begin
            rst_meta <= 1'b1;
            rst      <= 1'b1;
        end
        else begin
            rst_meta <= 1'b0;
            rst      <= rst_meta;
        end
    end

    wire [7:0] tx_d;
    wire       tx_oe;

    dpti_byte_tx u_tx (
        .clk        (clk),
        .rst        (rst),
        .byte_data  (tx_data),
        .byte_valid (tx_valid),
        .byte_ready (tx_ready),
        .dpti_txe_n (pad_txe_n),
        .dpti_d_out (tx_d),
        .dpti_d_oe  (tx_oe),
        .dpti_wr_n  (pad_wr_n)
    );

    dpti_byte_rx u_rx (
        .dpti_clkout (clk),
        .rst         (rst),
        .dpti_d_in   (pad_d_i),
        .dpti_rxf_n  (pad_rxf_n),
        .dpti_rd_n   (pad_rd_n),
        .dpti_oe_n   (pad_oe_n),
        .byte_data   (rx_data),
        .byte_valid  (rx_valid),
        .byte_ready  (rx_ready)
    );

    assign pad_d_o = tx_d;
    assign pad_d_t = {8{~tx_oe}};

endmodule

`default_nettype wire
