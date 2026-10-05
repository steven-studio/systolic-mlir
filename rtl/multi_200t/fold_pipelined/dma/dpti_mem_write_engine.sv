`default_nettype none

// ============================================================================
// dpti_mem_write_engine.sv
//
// Streaming payload -> DDR AXI4 write engine.
//
// The source side supplies complete AXI_DATA_W-wide beats.  This module owns
// only the DDR write transaction; byte reception and beat packing belong to
// the upstream transport logic.
//
// One transaction writes TOTAL_BEATS consecutive beats beginning at base_addr.
// Only one AXI burst is outstanding at a time.
// ============================================================================

module dpti_mem_write_engine #(
    parameter integer AXI_DATA_W  = 128,
    parameter integer AXI_ADDR_W  = 29,
    parameter integer AXI_ID_W    = 2,
    parameter integer BURST_LEN   = 16,
    parameter integer TOTAL_BEATS = 64
) (
    input  wire                     clk,
    input  wire                     rst_n,

    input  wire                     start,
    input  wire [AXI_ADDR_W-1:0]    base_addr,

    // ------------------------------------------------------------------------
    // Source beat stream.
    // ------------------------------------------------------------------------
    input  wire                     src_valid,
    input  wire [AXI_DATA_W-1:0]    src_data,
    output wire                     src_ready,

    output logic                    busy,
    output logic                    done,
    output logic                    err_align,
    output logic                    err_resp,

    // ------------------------------------------------------------------------
    // AXI4 write address channel.
    // ------------------------------------------------------------------------
    output wire [AXI_ID_W-1:0]      m_axi_awid,
    output logic [AXI_ADDR_W-1:0]   m_axi_awaddr,
    output wire [7:0]               m_axi_awlen,
    output wire [2:0]               m_axi_awsize,
    output wire [1:0]               m_axi_awburst,
    output wire [0:0]               m_axi_awlock,
    output wire [3:0]               m_axi_awcache,
    output wire [2:0]               m_axi_awprot,
    output wire [3:0]               m_axi_awqos,
    output logic                    m_axi_awvalid,
    input  wire                     m_axi_awready,

    // ------------------------------------------------------------------------
    // AXI4 write data channel.
    // ------------------------------------------------------------------------
    output wire [AXI_DATA_W-1:0]    m_axi_wdata,
    output wire [AXI_DATA_W/8-1:0]  m_axi_wstrb,
    output wire                     m_axi_wlast,
    output wire                     m_axi_wvalid,
    input  wire                     m_axi_wready,

    // ------------------------------------------------------------------------
    // AXI4 write response channel.
    // ------------------------------------------------------------------------
    input  wire [AXI_ID_W-1:0]      m_axi_bid,
    input  wire [1:0]               m_axi_bresp,
    input  wire                     m_axi_bvalid,
    output wire                     m_axi_bready
);

    localparam integer BYTES_PER_BEAT = AXI_DATA_W / 8;
    localparam integer LSB =
        $clog2(BYTES_PER_BEAT);
    localparam integer BURST_BYTES =
        BURST_LEN * BYTES_PER_BEAT;
    localparam integer N_BURSTS =
        TOTAL_BEATS / BURST_LEN;

    typedef enum logic [1:0] {
        S_IDLE,
        S_AW,
        S_W,
        S_B
    } state_t;

    state_t state;

    logic [31:0] burst_left;
    logic [7:0]  beat_left;

    assign m_axi_awid    = '0;
    assign m_axi_awlen   = 8'(BURST_LEN - 1);
    assign m_axi_awsize  = 3'(LSB);
    assign m_axi_awburst = 2'b01;
    assign m_axi_awlock  = 1'b0;
    assign m_axi_awcache = 4'b0011;
    assign m_axi_awprot  = 3'b000;
    assign m_axi_awqos   = 4'b0000;

    assign m_axi_wdata   = src_data;
    assign m_axi_wstrb   = '1;

    // The source beat is consumed exactly when AXI consumes it.
    assign m_axi_wvalid =
        (state == S_W) && src_valid;

    assign src_ready =
        (state == S_W) && m_axi_wready;

    assign m_axi_wlast =
        (state == S_W) &&
        (beat_left == 0);

    assign m_axi_bready = 1'b1;

    assign busy = (state != S_IDLE);

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state         <= S_IDLE;

            m_axi_awaddr  <= '0;
            m_axi_awvalid <= 1'b0;

            burst_left    <= '0;
            beat_left     <= '0;

            done          <= 1'b0;
            err_align     <= 1'b0;
            err_resp      <= 1'b0;
        end
        else begin
            done <= 1'b0;

            if (m_axi_bvalid &&
                (m_axi_bresp != 2'b00))
                err_resp <= 1'b1;

            case (state)

                S_IDLE: begin
                    if (start) begin
                        if (base_addr[
                            $clog2(BURST_BYTES)-1:0
                        ] != '0) begin
                            err_align <= 1'b1;
                        end
                        else begin
                            m_axi_awaddr  <= base_addr;
                            burst_left    <= N_BURSTS;
                            m_axi_awvalid <= 1'b1;
                            state         <= S_AW;
                        end
                    end
                end

                S_AW: begin
                    if (m_axi_awready) begin
                        m_axi_awvalid <= 1'b0;
                        beat_left     <=
                            8'(BURST_LEN - 1);
                        state         <= S_W;
                    end
                end

                S_W: begin
                    if (src_valid &&
                        m_axi_wready) begin

                        if (beat_left == 0) begin
                            state <= S_B;
                        end
                        else begin
                            beat_left <=
                                beat_left - 1'b1;
                        end
                    end
                end

                S_B: begin
                    if (m_axi_bvalid) begin
                        if (burst_left == 1) begin
                            done  <= 1'b1;
                            state <= S_IDLE;
                        end
                        else begin
                            burst_left <=
                                burst_left - 1'b1;

                            m_axi_awaddr <=
                                m_axi_awaddr +
                                AXI_ADDR_W'(BURST_BYTES);

                            m_axi_awvalid <= 1'b1;
                            state         <= S_AW;
                        end
                    end
                end

                default:
                    state <= S_IDLE;

            endcase
        end
    end

    initial begin
        if (AXI_DATA_W % 8 != 0)
            $fatal(
                1,
                "dpti_mem_write_engine: AXI_DATA_W must be byte aligned"
            );

        if (TOTAL_BEATS <= 0)
            $fatal(
                1,
                "dpti_mem_write_engine: TOTAL_BEATS must be positive"
            );

        if (BURST_LEN <= 0)
            $fatal(
                1,
                "dpti_mem_write_engine: BURST_LEN must be positive"
            );

        if ((TOTAL_BEATS % BURST_LEN) != 0)
            $fatal(
                1,
                "dpti_mem_write_engine: TOTAL_BEATS must be divisible by BURST_LEN"
            );
    end

endmodule

`default_nettype wire
