`timescale 1ns / 1ps
`default_nettype none

module tb_dpti_output_chain;

  logic clk = 1'b0;
  logic rst = 1'b1;

  always #5 clk = ~clk;

  logic [127:0] beat_data;
  logic         beat_valid;
  wire          beat_ready;

  wire [7:0] byte_data;
  wire       byte_valid;
  wire       byte_ready;

  logic      dpti_txe_n;
  wire [7:0] dpti_d_out;
  wire       dpti_d_oe;
  wire       dpti_wr_n;

  integer errors;
  integer got_n;

  logic [7:0] expected [0:63];

  dpti_beat_to_byte #(
    .DATA_W(128)
  ) u_ser (
    .clk        (clk),
    .rst        (rst),

    .beat_data  (beat_data),
    .beat_valid (beat_valid),
    .beat_ready (beat_ready),

    .byte_data  (byte_data),
    .byte_valid (byte_valid),
    .byte_ready (byte_ready)
  );

  dpti_byte_tx u_tx (
    .clk         (clk),
    .rst         (rst),

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
      @(negedge clk);
      beat_data  = data;
      beat_valid = 1'b1;

      do begin
        @(posedge clk);
      end while (!beat_ready);

      @(negedge clk);
      beat_valid = 1'b0;
    end
  endtask

  // Count bytes only when the physical write transfer is active.
  always @(posedge clk) begin
    if (!rst && !dpti_wr_n) begin
      if (!dpti_d_oe) begin
        $display(
          "FAIL: write active without bus drive at byte %0d",
          got_n
        );
        errors = errors + 1;
      end

      if (got_n >= 64) begin
        $display(
          "FAIL: unexpected extra byte %02h",
          dpti_d_out
        );
        errors = errors + 1;
      end
      else if (dpti_d_out !== expected[got_n]) begin
        $display(
          "FAIL: byte %0d expected=%02h got=%02h",
          got_n,
          expected[got_n],
          dpti_d_out
        );
        errors = errors + 1;
      end

      got_n = got_n + 1;
    end
  end

  initial begin : MAIN
    integer i;
    integer timeout;

    errors     = 0;
    got_n      = 0;
    beat_data  = '0;
    beat_valid = 1'b0;
    dpti_txe_n = 1'b0;

    // Expected stream:
    // beat 0 -> 00..0f
    // beat 1 -> 10..1f
    // beat 2 -> 20..2f
    // beat 3 -> 30..3f
    for (i = 0; i < 64; i = i + 1)
      expected[i] = i[7:0];

    repeat (5) @(posedge clk);

    @(negedge clk);
    rst = 1'b0;

    fork
      begin : PRODUCER
        logic [127:0] b;
        integer beat_i;
        integer lane;

        for (beat_i = 0; beat_i < 4; beat_i = beat_i + 1) begin
          b = '0;

          for (lane = 0; lane < 16; lane = lane + 1)
            b[8*lane +: 8] =
                (beat_i * 16 + lane);

          send_beat(b);
        end
      end

      begin : BACKPRESSURE
        // Allow part of the first beat through.
        while (got_n < 7)
          @(posedge clk);

        // Apply backpressure away from the sampling edge.
        @(negedge clk);
        dpti_txe_n = 1'b1;

        repeat (6)
          @(posedge clk);

        @(negedge clk);
        dpti_txe_n = 1'b0;

        // Stall again in a later beat.
        while (got_n < 37)
          @(posedge clk);

        @(negedge clk);
        dpti_txe_n = 1'b1;

        repeat (4)
          @(posedge clk);

        @(negedge clk);
        dpti_txe_n = 1'b0;
      end
    join_none

    timeout = 0;

    while ((got_n < 64) && (timeout < 2000)) begin
      @(posedge clk);
      timeout = timeout + 1;
    end

    if (got_n != 64) begin
      $display(
        "FAIL: timeout, received %0d/64 bytes",
        got_n
      );
      errors = errors + 1;
    end

    repeat (5) @(posedge clk);

    if (got_n != 64) begin
      $display(
        "FAIL: byte count changed after completion: %0d",
        got_n
      );
      errors = errors + 1;
    end

    if (errors == 0) begin
      $display("==============================================");
      $display("DPTI OUTPUT CHAIN TEST: PASS");
      $display("4 x 128-bit beats -> 64/64 physical bytes");
      $display("little-endian byte order verified");
      $display("output backpressure verified");
      $display("==============================================");
    end
    else begin
      $display(
        "DPTI OUTPUT CHAIN TEST: FAIL (%0d errors)",
        errors
      );
      $fatal(1);
    end

    $finish;
  end

endmodule

`default_nettype wire
