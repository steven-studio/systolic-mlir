// -----------------------------------------------------------------------------
// dma_wr_checksum_v2.sv -- chk_wr over a beat-wide write stream.
//
// systolic_dma_top accumulates chk_wr one word per cycle from the v1 writer:
//
//     A word:  chk += wdata ^ {8'd0, bank[7:0], depth[15:0]}
//     B word:  chk += wdata ^ (32'h8000_0000 | {8'd0, bank[7:0], depth[15:0]})
//
// and compares the total against EXPECT_WR_CHK, the constant seed_ref.py and
// tb_dma_path print.  With the v2 writer four words land per cycle, so the
// accumulator has to take four terms per cycle -- word j of an A beat sits at
// depth waddr + j in bank wsel, word j of a B beat sits in bank wsel + j at
// depth waddr.  32-bit wrapping addition is commutative and associative, so
// the total is the SAME constant: the golden checksums of every 3b run, and
// the image checksum tb_dma_path prints per geometry, stay valid unchanged.
// tb_dma_path_v2 checks all three agree (v1 per-word, v2 per-beat, image).
//
// Drop-in for the chk_wr block of systolic_dma_top: clear on P_CALIB, feed
// the v2 writer's outputs, compare chk against EXPECT_WR_CHK as before.
// -----------------------------------------------------------------------------

`default_nettype none

module dma_wr_checksum_v2 #(
  parameter integer N          = 8,
  parameter integer K_MAX      = 256,
  parameter integer AXI_DATA_W = 128
) (
  input  wire                      clk,
  input  wire                      rst_n,
  input  wire                      clear,

  input  wire                      a_wr,
  input  wire                      b_wr,
  input  wire [$clog2(N)-1:0]      wsel,
  input  wire [$clog2(K_MAX)-1:0]  waddr,
  input  wire [AXI_DATA_W-1:0]     wdata,

  output logic [31:0]              chk
);

  localparam integer WORDS = AXI_DATA_W / 32;

  // position word of beat word j, per layout, in the top's {8'd0, bank, depth}
  // form.  The + j is on zeroed low bits in both cases, so it is an OR.
  function automatic logic [31:0] pos_a(input logic [$clog2(N)-1:0] bank,
                                        input logic [$clog2(K_MAX)-1:0] depth,
                                        input int j);
    logic [15:0] k16; logic [7:0] b8;
    begin
      k16   = 16'(depth) | 16'(j);
      b8    = 8'(bank);
      pos_a = {8'd0, b8, k16};
    end
  endfunction

  function automatic logic [31:0] pos_b(input logic [$clog2(N)-1:0] bank,
                                        input logic [$clog2(K_MAX)-1:0] depth,
                                        input int j);
    logic [15:0] k16; logic [7:0] b8;
    begin
      k16   = 16'(depth);
      b8    = 8'(bank) | 8'(j);
      pos_b = 32'h8000_0000 | {8'd0, b8, k16};
    end
  endfunction

  logic [31:0] term [0:WORDS-1];
  logic [31:0] sum_beat;

  always_comb begin
    sum_beat = '0;
    for (int j = 0; j < WORDS; j++) begin
      term[j]  = wdata[32*j +: 32] ^ (b_wr ? pos_b(wsel, waddr, j) : pos_a(wsel, waddr, j));
      sum_beat = sum_beat + term[j];
    end
  end

  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n)              chk <= '0;
    else if (clear)          chk <= '0;
    else if (a_wr || b_wr)   chk <= chk + sum_beat;
  end

endmodule

`default_nettype wire
