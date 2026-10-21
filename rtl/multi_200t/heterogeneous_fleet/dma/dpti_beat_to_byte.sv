`default_nettype none
`timescale 1ns / 1ps

/*
 * dpti_beat_to_byte
 *
 * Serialize one DATA_W-bit input beat into DATA_W/8 bytes.
 *
 * Byte order matches the existing DMA image convention:
 *
 *   byte 0  = beat_data[7:0]
 *   byte 1  = beat_data[15:8]
 *   ...
 *   byte 15 = beat_data[127:120]   when DATA_W=128
 *
 * Both sides use ready/valid handshakes.
 *
 * Once a beat is accepted, it is held internally until all bytes have
 * been accepted.  Output data therefore remains stable under downstream
 * backpressure.
 */

module dpti_beat_to_byte #(
  parameter integer DATA_W = 128
) (
  input  wire              clk,
  input  wire              rst,

  // Beat input.
  input  wire [DATA_W-1:0] beat_data,
  input  wire              beat_valid,
  output wire              beat_ready,

  // Byte output.
  output wire [7:0]        byte_data,
  output wire              byte_valid,
  input  wire              byte_ready
);

  localparam integer BYTES = DATA_W / 8;
  localparam integer IDX_W = $clog2(BYTES);

  logic [DATA_W-1:0] beat_r;
  logic [IDX_W-1:0]  byte_idx;
  logic              busy;

  assign beat_ready = !rst && !busy;

  assign byte_valid = busy;

  assign byte_data =
      beat_r[8*byte_idx +: 8];

  always_ff @(posedge clk) begin
    if (rst) begin
      beat_r   <= '0;
      byte_idx <= '0;
      busy     <= 1'b0;
    end else begin
      if (!busy) begin
        if (beat_valid) begin
          beat_r   <= beat_data;
          byte_idx <= '0;
          busy     <= 1'b1;
        end
      end else if (byte_ready) begin
        if (byte_idx == IDX_W'(BYTES - 1)) begin
          byte_idx <= '0;
          busy     <= 1'b0;
        end else begin
          byte_idx <= byte_idx + 1'b1;
        end
      end
    end
  end

endmodule

`default_nettype wire
