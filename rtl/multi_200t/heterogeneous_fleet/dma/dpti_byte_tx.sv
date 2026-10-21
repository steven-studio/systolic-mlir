`default_nettype none
`timescale 1ns / 1ps

module dpti_byte_tx (
  input  wire       clk,
  input  wire       rst,

  // Byte-stream input.
  input  wire [7:0] byte_data,
  input  wire       byte_valid,
  output wire       byte_ready,

  // Physical-side flow control.
  input  wire       dpti_txe_n,

  // Physical bus drive.
  output wire [7:0] dpti_d_out,
  output wire       dpti_d_oe,
  output wire       dpti_wr_n
);

  /*
   * A byte is transferred only when:
   *
   *   byte_valid && !dpti_txe_n
   *
   * byte_ready therefore represents the physical sink's ability
   * to accept the byte in the current cycle.
   *
   * The upstream producer must hold byte_data stable while
   * byte_valid && !byte_ready.
   */

  assign byte_ready = !rst && !dpti_txe_n;

  assign dpti_d_out = byte_data;

  // Drive the data bus only for an actual byte transfer.
  assign dpti_d_oe =
      !rst &&
      byte_valid &&
      !dpti_txe_n;

  // Active-low write strobe.
  assign dpti_wr_n =
      ~(byte_valid &&
        byte_ready);

endmodule

`default_nettype wire
