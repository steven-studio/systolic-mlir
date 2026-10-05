`default_nettype none

// ============================================================================
// dpti_mem_write_cdc_engine.sv
//
// MEM_WRITE decoded stream:
//
//   source clock domain
//       -> asynchronous CDC
//       -> ui_clk domain
//       -> AXI4 DDR write engine
//
// One command consists of:
//   one base-address header
//   TOTAL_BEATS payload beats
// ============================================================================

module dpti_mem_write_cdc_engine #(
    parameter integer AXI_DATA_W  = 128,
    parameter integer AXI_ADDR_W  = 29,
    parameter integer AXI_ID_W    = 2,
    parameter integer FIFO_DEPTH  = 8,
    parameter integer BURST_LEN   = 16,
    parameter integer TOTAL_BEATS = 64
) (
    // ------------------------------------------------------------------------
    // Source clock domain.
    // ------------------------------------------------------------------------

    input  wire                     src_clk,
    input  wire                     src_rst,

    input  wire                     src_start,
    input  wire [AXI_ADDR_W-1:0]    src_base_addr,

    input  wire [AXI_DATA_W-1:0]    src_data,
    input  wire                     src_valid,
    output wire                     src_ready,

    // ------------------------------------------------------------------------
    // ui_clk / AXI domain.
    // ------------------------------------------------------------------------

    input  wire                     ui_clk,
    input  wire                     ui_rst_n,

    output wire                     busy,
    output wire                     done,
    output wire                     err_protocol,
    output wire                     err_align,
    output wire                     err_resp,

    output wire [AXI_ID_W-1:0]      m_axi_awid,
    output wire [AXI_ADDR_W-1:0]    m_axi_awaddr,
    output wire [7:0]               m_axi_awlen,
    output wire [2:0]               m_axi_awsize,
    output wire [1:0]               m_axi_awburst,
    output wire [0:0]               m_axi_awlock,
    output wire [3:0]               m_axi_awcache,
    output wire [2:0]               m_axi_awprot,
    output wire [3:0]               m_axi_awqos,
    output wire                     m_axi_awvalid,
    input  wire                     m_axi_awready,

    output wire [AXI_DATA_W-1:0]    m_axi_wdata,
    output wire [AXI_DATA_W/8-1:0]  m_axi_wstrb,
    output wire                     m_axi_wlast,
    output wire                     m_axi_wvalid,
    input  wire                     m_axi_wready,

    input  wire [AXI_ID_W-1:0]      m_axi_bid,
    input  wire [1:0]               m_axi_bresp,
    input  wire                     m_axi_bvalid,
    output wire                     m_axi_bready
);

    wire                    writer_start;
    wire [AXI_ADDR_W-1:0]   writer_base_addr;

    wire [AXI_DATA_W-1:0]   writer_data;
    wire                    writer_valid;
    wire                    writer_ready;

    // ------------------------------------------------------------------------
    // Clock-domain crossing.
    // ------------------------------------------------------------------------

    dpti_mem_write_cdc #(
        .ADDR_W (AXI_ADDR_W),
        .DATA_W (AXI_DATA_W),
        .DEPTH  (FIFO_DEPTH)
    ) u_cdc (
        .src_clk       (src_clk),
        .src_rst       (src_rst),

        .src_start     (src_start),
        .src_base_addr (src_base_addr),

        .src_data      (src_data),
        .src_valid     (src_valid),
        .src_ready     (src_ready),

        .dst_clk       (ui_clk),
        .dst_rst       (~ui_rst_n),

        .dst_start     (writer_start),
        .dst_base_addr (writer_base_addr),

        .dst_data      (writer_data),
        .dst_valid     (writer_valid),
        .dst_ready     (writer_ready),

        .err_protocol  (err_protocol)
    );

    // ------------------------------------------------------------------------
    // AXI4 write engine.
    // ------------------------------------------------------------------------

    dpti_mem_write_engine #(
        .AXI_DATA_W  (AXI_DATA_W),
        .AXI_ADDR_W  (AXI_ADDR_W),
        .AXI_ID_W    (AXI_ID_W),
        .BURST_LEN   (BURST_LEN),
        .TOTAL_BEATS (TOTAL_BEATS)
    ) u_writer (
        .clk           (ui_clk),
        .rst_n         (ui_rst_n),

        .start         (writer_start),
        .base_addr     (writer_base_addr),

        .src_valid     (writer_valid),
        .src_data      (writer_data),
        .src_ready     (writer_ready),

        .busy          (busy),
        .done          (done),
        .err_align     (err_align),
        .err_resp      (err_resp),

        .m_axi_awid    (m_axi_awid),
        .m_axi_awaddr  (m_axi_awaddr),
        .m_axi_awlen   (m_axi_awlen),
        .m_axi_awsize  (m_axi_awsize),
        .m_axi_awburst (m_axi_awburst),
        .m_axi_awlock  (m_axi_awlock),
        .m_axi_awcache (m_axi_awcache),
        .m_axi_awprot  (m_axi_awprot),
        .m_axi_awqos   (m_axi_awqos),
        .m_axi_awvalid (m_axi_awvalid),
        .m_axi_awready (m_axi_awready),

        .m_axi_wdata   (m_axi_wdata),
        .m_axi_wstrb   (m_axi_wstrb),
        .m_axi_wlast   (m_axi_wlast),
        .m_axi_wvalid  (m_axi_wvalid),
        .m_axi_wready  (m_axi_wready),

        .m_axi_bid     (m_axi_bid),
        .m_axi_bresp   (m_axi_bresp),
        .m_axi_bvalid  (m_axi_bvalid),
        .m_axi_bready  (m_axi_bready)
    );

endmodule

`default_nettype wire
