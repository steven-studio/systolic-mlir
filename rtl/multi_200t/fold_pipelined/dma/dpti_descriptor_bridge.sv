`default_nettype none

// ============================================================================
// dpti_descriptor_bridge.sv
//
// DPTI descriptor register-write bridge.
//
// This module deliberately does NOT assume how the CPU/host reaches the
// FPGA.  The physical host transport can drive the generic register-write
// interface below.
//
// Register protocol:
//
//   wr_valid + wr_ready
//   wr_addr
//   wr_data
//
// CONTROL:
//   0x00 bit[0] = submit
//
// Descriptor registers:
//   0x04 JOB_ID
//   0x08 DEVICE_ID
//   0x0c M
//   0x10 N
//   0x14 K
//   0x18 START_CYCLE
//   0x1c EST_CYCLES
//   0x20 A_BASE_LO
//   0x24 A_BASE_HI
//   0x28 B_BASE_LO
//   0x2c B_BASE_HI
//   0x30 C_BASE_LO
//   0x34 C_BASE_HI
//
// STATUS:
//   0x38 bit[0] = job_ready
//
// A submit produces a one-cycle job_valid pulse.  The descriptor remains
// stable until the downstream job interface accepts it.
//
// This module contains no scheduling policy.
// ============================================================================

module dpti_descriptor_bridge #(
    parameter integer ADDR_W = 8
)(
    input  wire                 clk,
    input  wire                 rst,

    // Generic host/register transport.
    input  wire                 wr_valid,
    output wire                 wr_ready,
    input  wire [ADDR_W-1:0]    wr_addr,
    input  wire [31:0]          wr_data,

    // DPTI job interface.
    output reg                  job_valid,
    input  wire                 job_ready,

    output reg  [31:0]          job_id,
    output reg  [31:0]          job_device_id,

    output reg  [31:0]          job_m,
    output reg  [31:0]          job_n,
    output reg  [31:0]          job_k,

    output reg  [31:0]          job_start_cycle,
    output reg  [31:0]          job_est_cycles,

    output reg  [63:0]          job_a_base,
    output reg  [63:0]          job_b_base,
    output reg  [63:0]          job_c_base,

    output wire [31:0]           status
);

    localparam [ADDR_W-1:0] REG_CONTROL     = 8'h00;
    localparam [ADDR_W-1:0] REG_JOB_ID      = 8'h04;
    localparam [ADDR_W-1:0] REG_DEVICE_ID   = 8'h08;
    localparam [ADDR_W-1:0] REG_M           = 8'h0c;
    localparam [ADDR_W-1:0] REG_N           = 8'h10;
    localparam [ADDR_W-1:0] REG_K           = 8'h14;
    localparam [ADDR_W-1:0] REG_START_CYCLE = 8'h18;
    localparam [ADDR_W-1:0] REG_EST_CYCLES  = 8'h1c;
    localparam [ADDR_W-1:0] REG_A_BASE_LO   = 8'h20;
    localparam [ADDR_W-1:0] REG_A_BASE_HI   = 8'h24;
    localparam [ADDR_W-1:0] REG_B_BASE_LO   = 8'h28;
    localparam [ADDR_W-1:0] REG_B_BASE_HI   = 8'h2c;
    localparam [ADDR_W-1:0] REG_C_BASE_LO   = 8'h30;
    localparam [ADDR_W-1:0] REG_C_BASE_HI   = 8'h34;

    assign wr_ready = !job_valid || job_ready;

    assign status = {31'd0, wr_ready};

    always @(posedge clk) begin
        if (rst) begin
            job_valid       <= 1'b0;

            job_id         <= 32'd0;
            job_device_id  <= 32'd0;

            job_m          <= 32'd0;
            job_n          <= 32'd0;
            job_k          <= 32'd0;

            job_start_cycle <= 32'd0;
            job_est_cycles  <= 32'd0;

            job_a_base     <= 64'd0;
            job_b_base     <= 64'd0;
            job_c_base     <= 64'd0;
        end
        else begin
            // A submitted job is consumed on the downstream handshake.
            if (job_valid && job_ready)
                job_valid <= 1'b0;

            if (wr_valid && wr_ready) begin
                case (wr_addr)

                    REG_CONTROL: begin
                        // bit 0 is the submit pulse.
                        if (wr_data[0])
                            job_valid <= 1'b1;
                    end

                    REG_JOB_ID:
                        job_id <= wr_data;

                    REG_DEVICE_ID:
                        job_device_id <= wr_data;

                    REG_M:
                        job_m <= wr_data;

                    REG_N:
                        job_n <= wr_data;

                    REG_K:
                        job_k <= wr_data;

                    REG_START_CYCLE:
                        job_start_cycle <= wr_data;

                    REG_EST_CYCLES:
                        job_est_cycles <= wr_data;

                    REG_A_BASE_LO:
                        job_a_base[31:0] <= wr_data;

                    REG_A_BASE_HI:
                        job_a_base[63:32] <= wr_data;

                    REG_B_BASE_LO:
                        job_b_base[31:0] <= wr_data;

                    REG_B_BASE_HI:
                        job_b_base[63:32] <= wr_data;

                    REG_C_BASE_LO:
                        job_c_base[31:0] <= wr_data;

                    REG_C_BASE_HI:
                        job_c_base[63:32] <= wr_data;

                    default:
                        begin
                        end

                endcase
            end
        end
    end

endmodule

`default_nettype wire
