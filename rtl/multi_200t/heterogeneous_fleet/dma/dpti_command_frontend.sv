`default_nettype none

// ============================================================================
// dpti_command_frontend.sv
//
// Shared byte-stream command frontend.
//
// Supported commands:
//   0x01 : WRITE32
//   0x02 : MEM_WRITE
//
// The opcode byte is forwarded to the selected decoder, so the already-tested
// decoders remain unchanged.
//
// Clock domain: dpti_clkout
// ============================================================================

module dpti_command_frontend #(
    parameter integer ADDR_W        = 8,
    parameter integer MEM_ADDR_W    = 29,
    parameter integer MEM_DATA_W    = 128,
    parameter integer PAYLOAD_BYTES = 1024
) (
    input  wire                     clk,
    input  wire                     rst,

    input  wire [7:0]               byte_data,
    input  wire                     byte_valid,
    output wire                     byte_ready,

    // WRITE32 output.
    output wire                     cmd_valid,
    input  wire                     cmd_ready,
    output wire [ADDR_W-1:0]        cmd_addr,
    output wire [31:0]              cmd_data,

    // MEM_WRITE decoded stream.
    output wire                     mem_start,
    output wire [MEM_ADDR_W-1:0]    mem_base_addr,

    output wire [MEM_DATA_W-1:0]    mem_data,
    output wire                     mem_valid,
    input  wire                     mem_ready,

    output wire                     mem_done,

    output reg                      err_opcode,
    output wire                     err_write32_opcode,
    output wire                     err_mem_opcode,
    output wire                     err_mem_length
);

    localparam [1:0]
        OWNER_IDLE    = 2'd0,
        OWNER_WRITE32 = 2'd1,
        OWNER_MEM     = 2'd2;

    reg [1:0] owner;

    wire write32_byte_ready;
    wire mem_byte_ready;

    wire write32_byte_valid;
    wire mem_byte_valid;

    // ------------------------------------------------------------------------
    // Route the current byte.
    //
    // In OWNER_IDLE the opcode itself is forwarded directly to the selected
    // decoder.  Therefore neither decoder needs a special "body-only" mode.
    // ------------------------------------------------------------------------

    assign write32_byte_valid =
        byte_valid &&
        (
            (owner == OWNER_WRITE32) ||
            ((owner == OWNER_IDLE) &&
             (byte_data == 8'h01))
        );

    assign mem_byte_valid =
        byte_valid &&
        (
            (owner == OWNER_MEM) ||
            ((owner == OWNER_IDLE) &&
             (byte_data == 8'h02))
        );

    assign byte_ready =
        (owner == OWNER_WRITE32)
            ? write32_byte_ready
        : (owner == OWNER_MEM)
            ? mem_byte_ready
        : (byte_data == 8'h01)
            ? write32_byte_ready
        : (byte_data == 8'h02)
            ? mem_byte_ready
        : 1'b1;

    // ------------------------------------------------------------------------
    // Command ownership.
    // ------------------------------------------------------------------------

    always @(posedge clk) begin
        if (rst) begin
            owner      <= OWNER_IDLE;
            err_opcode <= 1'b0;
        end
        else begin

            // Unknown opcode is consumed in IDLE and reported.
            if ((owner == OWNER_IDLE) &&
                byte_valid &&
                byte_ready &&
                (byte_data != 8'h01) &&
                (byte_data != 8'h02)) begin

                err_opcode <= 1'b1;
            end

            case (owner)

                OWNER_IDLE: begin
                    if (byte_valid && byte_ready) begin
                        if (byte_data == 8'h01)
                            owner <= OWNER_WRITE32;
                        else if (byte_data == 8'h02)
                            owner <= OWNER_MEM;
                    end
                end

                OWNER_WRITE32: begin
                    // Do not release ownership until the completed command has
                    // actually been accepted downstream.
                    if (cmd_valid && cmd_ready)
                        owner <= OWNER_IDLE;
                end

                OWNER_MEM: begin
                    // mem_done means the complete payload has left the decoder.
                    if (mem_done)
                        owner <= OWNER_IDLE;
                end

                default:
                    owner <= OWNER_IDLE;

            endcase
        end
    end

    // ------------------------------------------------------------------------
    // WRITE32 decoder.
    // ------------------------------------------------------------------------

    dpti_write32_decoder #(
        .ADDR_W (ADDR_W)
    ) u_write32 (
        .clk        (clk),
        .rst        (rst),

        .byte_data  (byte_data),
        .byte_valid (write32_byte_valid),
        .byte_ready (write32_byte_ready),

        .cmd_valid  (cmd_valid),
        .cmd_ready  (cmd_ready),

        .cmd_addr   (cmd_addr),
        .cmd_data   (cmd_data),

        .err_opcode (err_write32_opcode)
    );

    // ------------------------------------------------------------------------
    // MEM_WRITE decoder.
    // ------------------------------------------------------------------------

    dpti_mem_write_decoder #(
        .ADDR_W        (MEM_ADDR_W),
        .DATA_W        (MEM_DATA_W),
        .PAYLOAD_BYTES (PAYLOAD_BYTES)
    ) u_mem_write (
        .clk        (clk),
        .rst_n      (~rst),

        .byte_data  (byte_data),
        .byte_valid (mem_byte_valid),
        .byte_ready (mem_byte_ready),

        .start      (mem_start),
        .base_addr  (mem_base_addr),

        .out_data   (mem_data),
        .out_valid  (mem_valid),
        .out_ready  (mem_ready),

        .done       (mem_done),

        .err_opcode (err_mem_opcode),
        .err_length (err_mem_length)
    );

endmodule

`default_nettype wire
