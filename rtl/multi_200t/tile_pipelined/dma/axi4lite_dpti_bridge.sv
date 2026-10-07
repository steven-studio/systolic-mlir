`default_nettype none

// ============================================================================
// AXI4-Lite -> DPTI descriptor bridge
//
// AXI4-Lite write channels are independent:
//
//   AW channel  -> address
//   W  channel  -> data
//
// Therefore AW and W may arrive in either order.
//
// Once both are captured:
//
//   AXI4-Lite
//       |
//       v
//   wr_valid / wr_addr / wr_data
//       |
//       v
//   DPTI descriptor bridge
//
// wr_valid is held until wr_ready is asserted.
// A normal AXI OKAY write response is generated after the DPTI write is
// accepted.
//
// Read path:
//   AXI read -> status register
//
// This module does not contain scheduling policy.
// ============================================================================

module axi4lite_dpti_bridge #(
    parameter integer ADDR_W = 8
)(
    input  wire                  aclk,
    input  wire                  aresetn,

    // ------------------------------------------------------------------------
    // AXI4-Lite write address channel
    // ------------------------------------------------------------------------
    input  wire [ADDR_W-1:0]     s_axi_awaddr,
    input  wire                  s_axi_awvalid,
    output wire                  s_axi_awready,

    // ------------------------------------------------------------------------
    // AXI4-Lite write data channel
    // ------------------------------------------------------------------------
    input  wire [31:0]           s_axi_wdata,
    input  wire [3:0]            s_axi_wstrb,
    input  wire                  s_axi_wvalid,
    output wire                  s_axi_wready,

    // ------------------------------------------------------------------------
    // AXI4-Lite write response channel
    // ------------------------------------------------------------------------
    output reg  [1:0]            s_axi_bresp,
    output reg                   s_axi_bvalid,
    input  wire                   s_axi_bready,

    // ------------------------------------------------------------------------
    // AXI4-Lite read address channel
    // ------------------------------------------------------------------------
    input  wire [ADDR_W-1:0]     s_axi_araddr,
    input  wire                  s_axi_arvalid,
    output wire                  s_axi_arready,

    // ------------------------------------------------------------------------
    // AXI4-Lite read data channel
    // ------------------------------------------------------------------------
    output reg  [31:0]           s_axi_rdata,
    output reg  [1:0]            s_axi_rresp,
    output reg                   s_axi_rvalid,
    input  wire                   s_axi_rready,

    // ------------------------------------------------------------------------
    // DPTI register-write interface
    // ------------------------------------------------------------------------
    output reg                   wr_valid,
    input  wire                  wr_ready,
    output reg  [ADDR_W-1:0]     wr_addr,
    output reg  [31:0]           wr_data,

    // ------------------------------------------------------------------------
    // DPTI status readback
    // ------------------------------------------------------------------------
    input  wire [31:0]            status
);

    // ------------------------------------------------------------------------
    // Independent AXI write-channel storage.
    // ------------------------------------------------------------------------

    reg                     aw_pending;
    reg [ADDR_W-1:0]        awaddr_reg;

    reg                     w_pending;
    reg [31:0]              wdata_reg;

    // ------------------------------------------------------------------------
    // AXI4-Lite ready signals.
    //
    // AW and W are intentionally independent.
    // ------------------------------------------------------------------------

    assign s_axi_awready =
        !aw_pending &&
        !wr_valid &&
        !s_axi_bvalid;

    assign s_axi_wready =
        !w_pending &&
        !wr_valid &&
        !s_axi_bvalid;

    // Read channel is independent of the write path.
    assign s_axi_arready =
        !s_axi_rvalid;

    // ------------------------------------------------------------------------
    // Sequential protocol logic.
    // ------------------------------------------------------------------------

    always @(posedge aclk) begin
        if (!aresetn) begin

            aw_pending <= 1'b0;
            awaddr_reg <= {ADDR_W{1'b0}};

            w_pending  <= 1'b0;
            wdata_reg  <= 32'd0;

            wr_valid   <= 1'b0;
            wr_addr    <= {ADDR_W{1'b0}};
            wr_data    <= 32'd0;

            s_axi_bvalid <= 1'b0;
            s_axi_bresp  <= 2'b00;

            s_axi_rvalid <= 1'b0;
            s_axi_rresp  <= 2'b00;
            s_axi_rdata  <= 32'd0;

        end
        else begin

            // ================================================================
            // AXI WRITE ADDRESS
            // ================================================================

            if (s_axi_awvalid && s_axi_awready) begin
                aw_pending <= 1'b1;
                awaddr_reg <= s_axi_awaddr;
            end

            // ================================================================
            // AXI WRITE DATA
            //
            // s_axi_wstrb is currently accepted but the DPTI protocol is
            // defined as a full 32-bit register write.
            // ================================================================

            if (s_axi_wvalid && s_axi_wready) begin
                w_pending <= 1'b1;
                wdata_reg <= s_axi_wdata;
            end

            // ================================================================
            // Once both halves of the AXI write exist, present one DPTI
            // register-write transaction.
            // ================================================================

            if (!wr_valid &&
                aw_pending &&
                w_pending &&
                !s_axi_bvalid) begin

                wr_valid <= 1'b1;
                wr_addr  <= awaddr_reg;
                wr_data  <= wdata_reg;
            end

            // ================================================================
            // DPTI accepts the transaction.
            //
            // This is the actual ownership-transfer point.
            // ================================================================

            if (wr_valid && wr_ready) begin

                wr_valid <= 1'b0;

                aw_pending <= 1'b0;
                w_pending  <= 1'b0;

                // AXI4-Lite OKAY response.
                s_axi_bresp  <= 2'b00;
                s_axi_bvalid <= 1'b1;
            end

            // ================================================================
            // AXI write response consumed.
            // ================================================================

            if (s_axi_bvalid && s_axi_bready) begin
                s_axi_bvalid <= 1'b0;
            end

            // ================================================================
            // AXI READ
            //
            // The DPTI bridge exposes status through the read channel.
            // The address is currently informational; status is returned for
            // the register space.
            // ================================================================

            if (s_axi_arvalid && s_axi_arready) begin
                s_axi_rdata  <= status;
                s_axi_rresp  <= 2'b00;
                s_axi_rvalid <= 1'b1;
            end

            if (s_axi_rvalid && s_axi_rready) begin
                s_axi_rvalid <= 1'b0;
            end

        end
    end

endmodule

`default_nettype wire
