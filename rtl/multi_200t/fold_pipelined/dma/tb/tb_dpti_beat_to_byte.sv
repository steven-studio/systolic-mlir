`default_nettype none
`timescale 1ns / 1ps

module tb_dpti_beat_to_byte;

  localparam integer DATA_W = 128;
  localparam integer BYTES  = DATA_W / 8;

  logic clk = 1'b0;
  always #5 clk = ~clk;

  logic              rst;
  logic [DATA_W-1:0] beat_data;
  logic              beat_valid;
  wire               beat_ready;

  wire [7:0]         byte_data;
  wire               byte_valid;
  logic              byte_ready;

  integer errors = 0;
  integer got_n  = 0;

  logic [7:0] got [0:31];

  dpti_beat_to_byte #(
    .DATA_W(DATA_W)
  ) dut (
    .clk        (clk),
    .rst        (rst),

    .beat_data  (beat_data),
    .beat_valid (beat_valid),
    .beat_ready (beat_ready),

    .byte_data  (byte_data),
    .byte_valid (byte_valid),
    .byte_ready (byte_ready)
  );

  always_ff @(posedge clk) begin
    if (!rst && byte_valid && byte_ready) begin
      got[got_n] <= byte_data;
      got_n      <= got_n + 1;
    end
  end

  task automatic send_beat(
    input logic [DATA_W-1:0] data
  );
    begin
      // Drive away from the DUT sampling edge so there is no
      // testbench/DUT scheduling race.
      @(negedge clk);
      beat_data  = data;
      beat_valid = 1'b1;

      // Hold valid and data until a real ready/valid handshake
      // occurs at a rising edge.
      do begin
        @(posedge clk);
      end while (!beat_ready);

      @(negedge clk);
      beat_valid = 1'b0;
    end
  endtask

  logic [DATA_W-1:0] beat0;
  logic [DATA_W-1:0] beat1;

  integer i;

  initial begin
    rst        = 1'b1;
    beat_data  = '0;
    beat_valid = 1'b0;
    byte_ready = 1'b0;

    beat0 = '0;
    beat1 = '0;

    // Distinct byte values make ordering errors obvious.
    for (i = 0; i < BYTES; i = i + 1) begin
      beat0[8*i +: 8] = 8'h10 + i;
      beat1[8*i +: 8] = 8'h80 + i;
    end

    repeat (4) @(posedge clk);
    rst = 1'b0;
    @(posedge clk);

    // ----------------------------------------------------------
    // Beat 0.
    // ----------------------------------------------------------
    send_beat(beat0);

    byte_ready = 1'b1;

    // Accept a few bytes.
    while (got_n < 5)
      @(posedge clk);

    // Change backpressure away from the DUT sampling edge.
    // This guarantees that the last accepted byte and the first stalled
    // byte have an unambiguous handshake boundary.
    @(negedge clk);
    byte_ready = 1'b0;

    // Once stalled, byte_valid and byte_data must remain stable across
    // every rising edge until ready is asserted again.
    begin
      logic [7:0] held;

      #1;
      held = byte_data;

      repeat (4) begin
        @(posedge clk);
        #1;

        if (!byte_valid) begin
          $display("FAIL: byte_valid dropped under backpressure");
          errors = errors + 1;
        end

        if (byte_data !== held) begin
          $display(
            "FAIL: byte_data changed under backpressure: %h -> %h",
            held, byte_data);
          errors = errors + 1;
        end
      end
    end

    @(negedge clk);
    byte_ready = 1'b1;

    while (got_n < 16)
      @(posedge clk);

    // ----------------------------------------------------------
    // Beat 1.
    // ----------------------------------------------------------
    send_beat(beat1);

    while (got_n < 32)
      @(posedge clk);

    byte_ready = 1'b0;

    repeat (2) @(posedge clk);

    // ----------------------------------------------------------
    // Check exact byte order.
    // ----------------------------------------------------------
    if (got_n != 32) begin
      $display("FAIL: got_n=%0d want=32", got_n);
      errors = errors + 1;
    end

    for (i = 0; i < 16; i = i + 1) begin
      if (got[i] !== (8'h10 + i)) begin
        $display(
          "FAIL beat0 byte %0d: got=%h want=%h",
          i, got[i], 8'h10 + i);
        errors = errors + 1;
      end

      if (got[16+i] !== (8'h80 + i)) begin
        $display(
          "FAIL beat1 byte %0d: got=%h want=%h",
          i, got[16+i], 8'h80 + i);
        errors = errors + 1;
      end
    end

    if (errors == 0) begin
      $display(
        "DPTI BEAT TO BYTE TEST: PASS (32/32 bytes)");
    end else begin
      $display(
        "DPTI BEAT TO BYTE TEST: FAIL (%0d errors)",
        errors);
      $fatal(1);
    end

    $finish;
  end

  initial begin
    #100000;
    $display("FAIL: timeout");
    $fatal(1);
  end

endmodule

`default_nettype wire
