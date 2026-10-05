`default_nettype none

// ============================================================================
// dpti_axi4lite_master.sv
//
// Minimal AXI4-Lite write master for the DPTI host path.
//
// Command interface:
//   cmd_valid/cmd_ready
//   cmd_addr
//   cmd_data
//
// One command performs exactly one AXI4-Lite 32-bit register write.
//
// The master handles the fact that AXI4-Lite AW and W channels are
// independent and may complete in different cycles.
//
// Protocol:
//
//       cmd_valid
//           |
//           v
//       SEND_AW / SEND_W
//           |
//           v
//         WAIT_B
//           |
//           v
//          DONE
//
// No scheduling policy is present here.
// ============================================================================

module dpti_axi4lite_master #(
    parameter integer ADDR_W = 8
)(
    input  wire                  clk,
    input  wire                  rst,

    // ------------------------------------------------------------------------
    // Write command input.
    // ------------------------------------------------------------------------
    input  wire                  cmd_valid,
    output wire                  cmd_ready,

    input  wire [ADDR_W-1:0]     cmd_addr,
    input  wire [31:0]           cmd_data,

    // ------------------------------------------------------------------------
    // Command completion.
    // ------------------------------------------------------------------------
    output reg                   rsp_valid,
    input  wire                  rsp_ready,
    output reg  [1:0]            rsp_resp,

    // ------------------------------------------------------------------------
    // AXI4-Lite write address channel.
    // ------------------------------------------------------------------------
    output reg  [ADDR_W-1:0]     m_axi_awaddr,
    output reg                   m_axi_awvalid,
    input  wire                  m_axi_awready,

    // ------------------------------------------------------------------------
    // AXI4-Lite write data channel.
    // ------------------------------------------------------------------------
    output reg  [31:0]           m_axi_wdata,
    output reg  [3:0]            m_axi_wstrb,
    output reg                   m_axi_wvalid,
    input  wire                  m_axi_wready,

    // ------------------------------------------------------------------------
    // AXI4-Lite write response channel.
    // ------------------------------------------------------------------------
    input  wire [1:0]             m_axi_bresp,
    input  wire                   m_axi_bvalid,
    output wire                   m_axi_bready
);

    localparam [1:0] ST_IDLE  = 2'd0;
    localparam [1:0] ST_WRITE = 2'd1;
    localparam [1:0] ST_RESP  = 2'd2;

    reg [1:0] state;

    reg aw_done;
    reg w_done;

    assign cmd_ready =
        (state == ST_IDLE) &&
        !rsp_valid;

    // BREADY is asserted only while waiting for the response.
    assign m_axi_bready =
        (state == ST_RESP);

    always @(posedge clk) begin
        if (rst) begin
            state <= ST_IDLE;

            aw_done <= 1'b0;
            w_done  <= 1'b0;

            rsp_valid <= 1'b0;
            rsp_resp  <= 2'b00;

            m_axi_awaddr  <= {ADDR_W{1'b0}};
            m_axi_awvalid <= 1'b0;

            m_axi_wdata   <= 32'd0;
            m_axi_wstrb   <= 4'b1111;
            m_axi_wvalid  <= 1'b0;
        end
        else begin

            // ================================================================
            // Completion response is held until the consumer accepts it.
            // ================================================================

            if (rsp_valid && rsp_ready)
                rsp_valid <= 1'b0;

            // ================================================================
            // IDLE
            // ================================================================

            if (state == ST_IDLE) begin

                aw_done <= 1'b0;
                w_done  <= 1'b0;

                if (cmd_valid && cmd_ready) begin
                    m_axi_awaddr  <= cmd_addr;
                    m_axi_awvalid <= 1'b1;

                    m_axi_wdata   <= cmd_data;
                    m_axi_wstrb   <= 4'b1111;
                    m_axi_wvalid  <= 1'b1;

                    state <= ST_WRITE;
                end
            end

            // ================================================================
            // WRITE
            //
            // AW and W are independent AXI4-Lite channels.
            // Keep each VALID asserted until its READY handshake occurs.
            // ================================================================

            if (state == ST_WRITE) begin

                if (m_axi_awvalid && m_axi_awready) begin
                    m_axi_awvalid <= 1'b0;
                    aw_done       <= 1'b1;
                end

                if (m_axi_wvalid && m_axi_wready) begin
                    m_axi_wvalid <= 1'b0;
                    w_done       <= 1'b1;
                end

                // Both address and data have transferred.
                if ((aw_done || (m_axi_awvalid && m_axi_awready)) &&
                    (w_done  || (m_axi_wvalid  && m_axi_wready))) begin
                    state <= ST_RESP;
                end
            end

            // ================================================================
            // RESPONSE
            // ================================================================

            if (state == ST_RESP) begin

                if (m_axi_bvalid && m_axi_bready) begin
                    rsp_resp  <= m_axi_bresp;
                    rsp_valid <= 1'b1;

                    state <= ST_IDLE;
                end
            end
        end
    end

endmodule

`default_nettype wire
