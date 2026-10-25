`default_nettype none

// ============================================================================
// dpti_mem_write_decoder.sv
//
// Decode one MEM_WRITE command from an already-decoded byte stream.
//
// Wire format:
//   byte 0      opcode = 0x02
//   byte 1..4   DDR byte address, little-endian
//   byte 5..8   payload length, little-endian
//   byte 9..    payload bytes
//
// The first implementation deliberately accepts exactly PAYLOAD_BYTES bytes.
// Payload bytes are packed little-endian into 128-bit output beats:
//   byte 0 -> bits [7:0]
//   ...
//   byte 15 -> bits [127:120]
//
// The output beat obeys normal valid/ready semantics and remains stable while
// out_valid=1 and out_ready=0.
// ============================================================================

module dpti_mem_write_decoder #(
    parameter integer ADDR_W        = 29,
    parameter integer DATA_W        = 128,
    parameter integer PAYLOAD_BYTES = 1024
) (
    input  wire                  clk,
    input  wire                  rst_n,

    input  wire [7:0]            byte_data,
    input  wire                  byte_valid,
    output wire                  byte_ready,

    output logic                 start,
    output logic [ADDR_W-1:0]    base_addr,

    output logic [DATA_W-1:0]    out_data,
    output logic                 out_valid,
    input  wire                  out_ready,

    output logic                 done,
    output logic                 err_opcode,
    output logic                 err_length
);

    localparam integer BYTES_PER_BEAT = DATA_W / 8;

    typedef enum logic [2:0] {
        S_OPCODE,
        S_ADDR,
        S_LENGTH,
        S_PAYLOAD,
        S_OUTPUT
    } state_t;

    state_t state;

    logic [31:0] addr_tmp;
    logic [31:0] length_tmp;

    integer header_index;
    integer payload_count;
    integer byte_index;

    // We can consume another input byte whenever we are not holding a complete
    // output beat waiting for downstream.
    assign byte_ready =
        (state != S_OUTPUT);

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state         <= S_OPCODE;

            addr_tmp      <= '0;
            length_tmp    <= '0;
            base_addr     <= '0;

            header_index  <= 0;
            payload_count <= 0;
            byte_index    <= 0;

            out_data      <= '0;
            out_valid     <= 1'b0;

            start         <= 1'b0;
            done          <= 1'b0;

            err_opcode    <= 1'b0;
            err_length    <= 1'b0;
        end
        else begin
            start <= 1'b0;
            done  <= 1'b0;

            case (state)

                S_OPCODE: begin
                    if (byte_valid && byte_ready) begin
                        if (byte_data != 8'h02) begin
                            err_opcode <= 1'b1;
                        end
                        else begin
                            addr_tmp     <= '0;
                            length_tmp   <= '0;
                            header_index <= 0;
                            state        <= S_ADDR;
                        end
                    end
                end

                S_ADDR: begin
                    if (byte_valid && byte_ready) begin
                        addr_tmp[8*header_index +: 8] <=
                            byte_data;

                        if (header_index == 3) begin
                            header_index <= 0;
                            state        <= S_LENGTH;
                        end
                        else begin
                            header_index <= header_index + 1;
                        end
                    end
                end

                S_LENGTH: begin
                    if (byte_valid && byte_ready) begin
                        length_tmp[8*header_index +: 8] <=
                            byte_data;

                        if (header_index == 3) begin
                            header_index <= 0;

                            // byte_data is the final, most-significant byte.
                            if ({
                                byte_data,
                                length_tmp[23:0]
                            } != PAYLOAD_BYTES) begin
                                err_length <= 1'b1;
                                state      <= S_OPCODE;
                            end
                            else begin
                                base_addr <=
                                    ADDR_W'(addr_tmp);

                                start         <= 1'b1;
                                payload_count <= 0;
                                byte_index    <= 0;
                                out_data      <= '0;

                                state <= S_PAYLOAD;
                            end
                        end
                        else begin
                            header_index <= header_index + 1;
                        end
                    end
                end

                S_PAYLOAD: begin
                    if (byte_valid && byte_ready) begin
                        out_data[8*byte_index +: 8] <=
                            byte_data;

                        payload_count <=
                            payload_count + 1;

                        if (byte_index ==
                            BYTES_PER_BEAT - 1) begin

                            byte_index <= 0;
                            out_valid  <= 1'b1;
                            state      <= S_OUTPUT;
                        end
                        else begin
                            byte_index <= byte_index + 1;
                        end
                    end
                end

                S_OUTPUT: begin
                    if (out_valid && out_ready) begin
                        out_valid <= 1'b0;

                        if (payload_count ==
                            PAYLOAD_BYTES) begin
                            done  <= 1'b1;
                            state <= S_OPCODE;
                        end
                        else begin
                            out_data <= '0;
                            state    <= S_PAYLOAD;
                        end
                    end
                end

                default:
                    state <= S_OPCODE;

            endcase
        end
    end

    initial begin
        if (DATA_W % 8 != 0)
            $fatal(
                1,
                "dpti_mem_write_decoder: DATA_W must be byte aligned"
            );

        if ((PAYLOAD_BYTES % BYTES_PER_BEAT) != 0)
            $fatal(
                1,
                "dpti_mem_write_decoder: payload must contain complete beats"
            );
    end

endmodule

`default_nettype wire
