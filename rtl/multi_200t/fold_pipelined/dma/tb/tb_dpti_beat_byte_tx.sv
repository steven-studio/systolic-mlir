`default_nettype none
`timescale 1ns / 1ps

module tb_dpti_beat_byte_tx;

  localparam integer DATA_W = 128;
  localparam integer BYTES  = DATA_W / 8;

  logic clk = 1'b0;
  always #5 clk = ~clk;

  logic rst;

  // ----------------------------------------------------------
  // Beat source
  // ----------------------------------------------------------
  logic [DATA_W-1:0] beat_data;
  logic              beat_valid;
  wire               beat_ready;

  // ----------------------------------------------------------
  // Serializer -> byte transmitter
  // ----------------------------------------------------------
  wire [7:0] byte_data;
  wire       byte_valid;
  wire       byte_ready;

  // ----------------------------------------------------------
  // Physical-side model
  // ----------------------------------------------------------
  logic      dpti_txe_n;
  wire [7:0] dpti_d_out;
  wire       dpti_d_oe;
  wire       dpti_wr_n;

  integer errors = 0;
  integer got_n  = 0;

  logic [7:0] got [0:31];

  dpti_beat_to_byte #(
    .DATA_W(DATA_W)
  ) u_serializer (
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

  // A physical byte transfer occurs when the output-enable and
  // active-low write strobe are both asserted.
  always_ff @(posedge clk) begin
    if (!rst && dpti_d_oe && !dpti_wr_n) begin
      if (got_n < 32)
        got[got_n] <= dpti_d_out;

      got_n <= got_n + 1;
    end
  end

  task automatic send_beat(
    input logic [DATA_W-1:0] data
  );
    begin
      // Drive away from the sampling edge.
      @(negedge clk);
      beat_data  = data;
      beat_valid = 1'b1;

      // Hold until a real beat handshake occurs.
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
    dpti_txe_n = 1'b1;

    beat0 = '0;
    beat1 = '0;

    // Expected physical stream:
    //
    //   10 11 ... 1f
    //   80 81 ... 8f
    //
    for (i = 0; i < BYTES; i = i + 1) begin
      beat0[8*i +: 8] = 8'h10 + i;
      beat1[8*i +: 8] = 8'h80 + i;
    end

    repeat (4) @(posedge clk);

    @(negedge clk);
    rst        = 1'b0;
    dpti_txe_n = 1'b0;

    // ----------------------------------------------------------
    // First beat
    // ----------------------------------------------------------
    send_beat(beat0);

    // Let several bytes physically leave.
    while (got_n < 5)
      @(posedge clk);

    // ----------------------------------------------------------
    // Physical backpressure in the middle of the beat.
    // ----------------------------------------------------------
    @(negedge clk);
    dpti_txe_n = 1'b1;

    begin
      logic [7:0] held;

      #1;
      held = dpti_d_out;

      repeat (5) begin
        @(posedge clk);
        #1;

        if (byte_ready !== 1'b0) begin
          $display(
            "FAIL: byte_ready asserted during backpressure");
          errors = errors + 1;
        end

        if (dpti_d_oe !== 1'b0) begin
          $display(
            "FAIL: output enable asserted during backpressure");
          errors = errors + 1;
        end

        if (dpti_wr_n !== 1'b1) begin
          $display(
            "FAIL: write strobe asserted during backpressure");
          errors = errors + 1;
        end

        if (dpti_d_out !== held) begin
          $display(
            "FAIL: output byte changed under backpressure: %h -> %h",
            held, dpti_d_out);
          errors = errors + 1;
        end
      end
    end

    @(negedge clk);
    dpti_txe_n = 1'b0;

    while (got_n < 16)
      @(posedge clk);

    // ----------------------------------------------------------
    // Second beat
    // ----------------------------------------------------------
    send_beat(beat1);

    // Add another shorter stall.
    while (got_n < 23)
      @(posedge clk);

    @(negedge clk);
    dpti_txe_n = 1'b1;

    repeat (3) @(posedge clk);

    @(negedge clk);
    dpti_txe_n = 1'b0;

    while (got_n < 32)
      @(posedge clk);

    // Move away from the final transfer edge before checking.
    @(negedge clk);

    // ----------------------------------------------------------
    // Exact physical byte-stream check
    // ----------------------------------------------------------
    if (got_n != 32) begin
      $display(
        "FAIL: physical byte count=%0d want=32",
        got_n);
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
        "DPTI BEAT -> BYTE -> TX TEST: PASS (32/32 physical bytes)");
    end else begin
      $display(
        "DPTI BEAT -> BYTE -> TX TEST: FAIL (%0d errors)",
        errors);
      $fatal(1);
    end

    $finish;
  end

  initial begin
    #200000;
    $display(
      "FAIL: timeout got_n=%0d beat_ready=%b byte_valid=%b byte_ready=%b",
      got_n, beat_ready, byte_valid, byte_ready);
    $fatal(1);
  end

endmodule

`default_nettype wire
