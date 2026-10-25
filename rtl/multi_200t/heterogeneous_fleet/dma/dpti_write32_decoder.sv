`default_nettype none

// ============================================================================
// dpti_write32_decoder.sv
//
// Decode one fixed-size WRITE32 command from a byte stream.
//
// Wire format:
//   byte 0 : opcode = 0x01
//   byte 1 : register address
//   byte 2 : data[7:0]
//   byte 3 : data[15:8]
//   byte 4 : data[23:16]
//   byte 5 : data[31:24]
//
// cmd_valid is held until cmd_ready.
// ============================================================================

module dpti_write32_decoder #(
    parameter integer ADDR_W = 8
) (
    input  wire                  clk,
    input  wire                  rst,

    input  wire [7:0]            byte_data,
    input  wire                  byte_valid,
    output wire                  byte_ready,

    output reg                   cmd_valid,
    input  wire                  cmd_ready,

    output reg [ADDR_W-1:0]      cmd_addr,
    output reg [31:0]            cmd_data,

    output reg                   err_opcode
);

    localparam [2:0]
        ST_OPCODE = 3'd0,
        ST_ADDR   = 3'd1,
        ST_D0     = 3'd2,
        ST_D1     = 3'd3,
        ST_D2     = 3'd4,
        ST_D3     = 3'd5;

    reg [2:0] state;

    reg [7:0]  addr_r;
    reg [31:0] data_r;

    // Stop consuming bytes while a completed command is waiting downstream.
    assign byte_ready = !cmd_valid;

    always @(posedge clk) begin
        if (rst) begin
            state      <= ST_OPCODE;

            addr_r     <= 8'h00;
            data_r     <= 32'h00000000;

            cmd_valid  <= 1'b0;
            cmd_addr   <= {ADDR_W{1'b0}};
            cmd_data   <= 32'h00000000;

            err_opcode <= 1'b0;
        end
        else begin

            if (cmd_valid && cmd_ready)
                cmd_valid <= 1'b0;

            if (byte_valid && byte_ready) begin
                case (state)

                    ST_OPCODE: begin
                        if (byte_data == 8'h01) begin
                            state <= ST_ADDR;
                        end
                        else begin
                            err_opcode <= 1'b1;
                            state      <= ST_OPCODE;
                        end
                    end

                    ST_ADDR: begin
                        addr_r <= byte_data;
                        state  <= ST_D0;
                    end

                    ST_D0: begin
                        data_r[7:0] <= byte_data;
                        state       <= ST_D1;
                    end

                    ST_D1: begin
                        data_r[15:8] <= byte_data;
                        state        <= ST_D2;
                    end

                    ST_D2: begin
                        data_r[23:16] <= byte_data;
                        state         <= ST_D3;
                    end

                    ST_D3: begin
                        data_r[31:24] <= byte_data;

                        cmd_addr <= addr_r;
                        cmd_data <= {
                            byte_data,
                            data_r[23:0]
                        };

                        cmd_valid <= 1'b1;
                        state     <= ST_OPCODE;
                    end

                    default:
                        state <= ST_OPCODE;

                endcase
            end
        end
    end

endmodule

`default_nettype wire
