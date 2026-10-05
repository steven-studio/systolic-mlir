`default_nettype none

module dpti_mem_write_path #(
    parameter integer AXI_DATA_W  = 128,
    parameter integer AXI_ADDR_W  = 29,
    parameter integer AXI_ID_W    = 2,
    parameter integer BURST_LEN   = 16,
    parameter integer PAYLOAD_BYTES = 1024
) (
    input  wire                     clk,
    input  wire                     rst_n,

    input  wire [7:0]               byte_data,
    input  wire                     byte_valid,
    output wire                     byte_ready,

    output wire                     busy,
    output wire                     done,
    output wire                     err_opcode,
    output wire                     err_length,
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

    localparam integer TOTAL_BEATS =
        PAYLOAD_BYTES / (AXI_DATA_W / 8);

    wire                    write_start;
    wire [AXI_ADDR_W-1:0]   write_base_addr;

    wire [AXI_DATA_W-1:0]   payload_data;
    wire                    payload_valid;
    wire                    payload_ready;

    wire decoder_done;
    wire writer_done;

    dpti_mem_write_decoder #(
        .ADDR_W        (AXI_ADDR_W),
        .DATA_W        (AXI_DATA_W),
        .PAYLOAD_BYTES (PAYLOAD_BYTES)
    ) u_decoder (
        .clk        (clk),
        .rst_n      (rst_n),

        .byte_data  (byte_data),
        .byte_valid (byte_valid),
        .byte_ready (byte_ready),

        .start      (write_start),
        .base_addr  (write_base_addr),

        .out_data   (payload_data),
        .out_valid  (payload_valid),
        .out_ready  (payload_ready),

        .done       (decoder_done),
        .err_opcode (err_opcode),
        .err_length (err_length)
    );

    dpti_mem_write_engine #(
        .AXI_DATA_W  (AXI_DATA_W),
        .AXI_ADDR_W  (AXI_ADDR_W),
        .AXI_ID_W    (AXI_ID_W),
        .BURST_LEN   (BURST_LEN),
        .TOTAL_BEATS (TOTAL_BEATS)
    ) u_writer (
        .clk            (clk),
        .rst_n          (rst_n),

        .start          (write_start),
        .base_addr      (write_base_addr),

        .src_valid      (payload_valid),
        .src_data       (payload_data),
        .src_ready      (payload_ready),

        .busy           (busy),
        .done           (writer_done),
        .err_align      (err_align),
        .err_resp       (err_resp),

        .m_axi_awid     (m_axi_awid),
        .m_axi_awaddr   (m_axi_awaddr),
        .m_axi_awlen    (m_axi_awlen),
        .m_axi_awsize   (m_axi_awsize),
        .m_axi_awburst  (m_axi_awburst),
        .m_axi_awlock   (m_axi_awlock),
        .m_axi_awcache  (m_axi_awcache),
        .m_axi_awprot   (m_axi_awprot),
        .m_axi_awqos    (m_axi_awqos),
        .m_axi_awvalid  (m_axi_awvalid),
        .m_axi_awready  (m_axi_awready),

        .m_axi_wdata    (m_axi_wdata),
        .m_axi_wstrb    (m_axi_wstrb),
        .m_axi_wlast    (m_axi_wlast),
        .m_axi_wvalid   (m_axi_wvalid),
        .m_axi_wready   (m_axi_wready),

        .m_axi_bid      (m_axi_bid),
        .m_axi_bresp    (m_axi_bresp),
        .m_axi_bvalid   (m_axi_bvalid),
        .m_axi_bready   (m_axi_bready)
    );

    // Completion visible to the caller means DDR accepted the complete
    // transaction, not merely that the decoder consumed the final input byte.
    assign done = writer_done;

    initial begin
        if ((PAYLOAD_BYTES % (AXI_DATA_W / 8)) != 0)
            $fatal(
                1,
                "dpti_mem_write_path: PAYLOAD_BYTES must contain complete AXI beats"
            );
    end

endmodule

`default_nettype wire
