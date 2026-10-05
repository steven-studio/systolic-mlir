`timescale 1ns / 1ps
`default_nettype none

module tb_dpti_output_cdc_chain;

  logic ui_clk = 1'b0;
  logic dpti_clk = 1'b0;

  always #5 ui_clk   = ~ui_clk;
  always #7 dpti_clk = ~dpti_clk;

  logic ui_rst = 1'b1;
  logic dpti_rst = 1'b1;

  logic [127:0] src_data;
  logic         src_valid;
  wire          src_ready;

  wire [127:0]  cdc_data;
  wire          cdc_valid;
  wire          cdc_ready;

  wire [7:0]    byte_data;
  wire          byte_valid;
  wire          byte_ready;

  logic         dpti_txe_n;

  wire [7:0]    dpti_d_out;
  wire          dpti_d_oe;
  wire          dpti_wr_n;

  integer errors;
  integer got_n;

  logic [7:0] expected [0:63];

  dpti_output_cdc #(
      .DATA_W (128),
      .DEPTH  (8)
  ) u_cdc (
      .src_clk   (ui_clk),
      .src_rst   (ui_rst),

      .src_data  (src_data),
      .src_valid (src_valid),
      .src_ready (src_ready),

      .dst_clk   (dpti_clk),
      .dst_rst   (dpti_rst),

      .dst_data  (cdc_data),
      .dst_valid (cdc_valid),
      .dst_ready (cdc_ready)
  );

  dpti_beat_to_byte #(
      .DATA_W (128)
  ) u_ser (
      .clk        (dpti_clk),
      .rst        (dpti_rst),

      .beat_data  (cdc_data),
      .beat_valid (cdc_valid),
      .beat_ready (cdc_ready),

      .byte_data  (byte_data),
      .byte_valid (byte_valid),
      .byte_ready (byte_ready)
  );

  dpti_byte_tx u_tx (
      .clk         (dpti_clk),
      .rst         (dpti_rst),

      .byte_data   (byte_data),
      .byte_valid  (byte_valid),
      .byte_ready  (byte_ready),

      .dpti_txe_n  (dpti_txe_n),

      .dpti_d_out  (dpti_d_out),
      .dpti_d_oe   (dpti_d_oe),
      .dpti_wr_n   (dpti_wr_n)
  );

  task automatic send_beat(
      input logic [127:0] data
  );
    begin
      @(negedge ui_clk);
      src_data  = data;
      src_valid = 1'b1;

      do begin
        @(posedge ui_clk);
      end while (!src_ready);

      @(negedge ui_clk);
      src_valid = 1'b0;
    end
  endtask

  always @(posedge dpti_clk) begin
    if (!dpti_rst &&
        dpti_d_oe &&
        !dpti_wr_n) begin

      if (got_n >= 64) begin
        $display("FAIL: received extra byte %02h", dpti_d_out);
        errors = errors + 1;
      end
      else if (dpti_d_out !== expected[got_n]) begin
        $display(
          "FAIL byte %0d: expected=%02h got=%02h",
          got_n,
          expected[got_n],
          dpti_d_out
        );
        errors = errors + 1;
      end

      got_n = got_n + 1;
    end
  end

  integer i;
  integer b;

  initial begin
    errors = 0;
    got_n = 0;

    src_data = '0;
    src_valid = 1'b0;
    dpti_txe_n = 1'b0;

    for (i = 0; i < 64; i = i + 1)
      expected[i] = i[7:0];

    repeat (5) @(posedge ui_clk);
    ui_rst = 1'b0;

    repeat (5) @(posedge dpti_clk);
    dpti_rst = 1'b0;

    // Four 128-bit beats.
    for (b = 0; b < 4; b = b + 1) begin
      logic [127:0] beat;

      beat = '0;

      for (i = 0; i < 16; i = i + 1)
        beat[8*i +: 8] = (b * 16 + i);

      send_beat(beat);
    end

    // Exercise destination-side backpressure.
    while (got_n < 20)
      @(posedge dpti_clk);

    @(negedge dpti_clk);
    dpti_txe_n = 1'b1;

    repeat (6) @(posedge dpti_clk);

    @(negedge dpti_clk);
    dpti_txe_n = 1'b0;

    fork
      begin
        wait (got_n == 64);
      end

      begin
        repeat (1000) @(posedge dpti_clk);
        $display("FAIL: timeout waiting for 64 bytes");
        errors = errors + 1;
      end
    join_any
    disable fork;

    repeat (5) @(posedge dpti_clk);

    if (got_n != 64) begin
      $display(
        "FAIL: expected 64 physical bytes, got %0d",
        got_n
      );
      errors = errors + 1;
    end

    if (errors == 0) begin
      $display("==============================================");
      $display("DPTI OUTPUT CDC CHAIN TEST: PASS");
      $display("ui_clk -> async FIFO -> dpti_clkout");
      $display("4 x 128-bit beats -> 64/64 physical bytes");
      $display("CDC ordering verified");
      $display("output backpressure verified");
      $display("==============================================");
    end
    else begin
      $display("==============================================");
      $display(
        "DPTI OUTPUT CDC CHAIN TEST: FAIL (%0d errors)",
        errors
      );
      $display("==============================================");
      $fatal(1);
    end

    $finish;
  end

endmodule

`default_nettype wire
