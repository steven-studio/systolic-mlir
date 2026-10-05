`default_nettype none
`timescale 1ns / 1ps

module tb_dpti_byte_tx;

  logic clk = 1'b0;
  always #5 clk = ~clk;

  logic       rst;
  logic [7:0] byte_data;
  logic       byte_valid;
  wire        byte_ready;

  logic       dpti_txe_n;

  wire [7:0]  dpti_d_out;
  wire        dpti_d_oe;
  wire        dpti_wr_n;

  integer errors = 0;

  dpti_byte_tx dut (
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

  task automatic check(
    input logic       want_ready,
    input logic       want_oe,
    input logic       want_wr_n,
    input logic [7:0] want_data,
    input string      what
  );
    begin
      #1;

      if (byte_ready !== want_ready) begin
        $display(
          "FAIL %s: byte_ready=%b want=%b",
          what, byte_ready, want_ready);
        errors = errors + 1;
      end

      if (dpti_d_oe !== want_oe) begin
        $display(
          "FAIL %s: dpti_d_oe=%b want=%b",
          what, dpti_d_oe, want_oe);
        errors = errors + 1;
      end

      if (dpti_wr_n !== want_wr_n) begin
        $display(
          "FAIL %s: dpti_wr_n=%b want=%b",
          what, dpti_wr_n, want_wr_n);
        errors = errors + 1;
      end

      if (dpti_d_out !== want_data) begin
        $display(
          "FAIL %s: dpti_d_out=%h want=%h",
          what, dpti_d_out, want_data);
        errors = errors + 1;
      end
    end
  endtask

  initial begin
    rst        = 1'b1;
    byte_data  = 8'h00;
    byte_valid = 1'b0;
    dpti_txe_n = 1'b1;

    repeat (3) @(posedge clk);

    // Reset: no transfer is possible.
    byte_data = 8'h11;
    check(1'b0, 1'b0, 1'b1, 8'h11, "reset");

    rst = 1'b0;
    @(posedge clk);

    // Idle.
    byte_data  = 8'h22;
    byte_valid = 1'b0;
    dpti_txe_n = 1'b0;
    check(1'b1, 1'b0, 1'b1, 8'h22, "idle");

    // Backpressure.
    byte_data  = 8'hA5;
    byte_valid = 1'b1;
    dpti_txe_n = 1'b1;
    check(1'b0, 1'b0, 1'b1, 8'hA5, "backpressure");

    repeat (4) begin
      @(posedge clk);
      check(1'b0, 1'b0, 1'b1, 8'hA5,
            "held-under-backpressure");
    end

    // Release backpressure: A5 becomes transferable.
    dpti_txe_n = 1'b0;
    check(1'b1, 1'b1, 1'b0, 8'hA5,
          "transfer-A5");

    @(posedge clk);

    // Next byte.
    byte_data = 8'h3C;
    check(1'b1, 1'b1, 1'b0, 8'h3C,
          "transfer-3C");

    @(posedge clk);

    byte_valid = 1'b0;
    check(1'b1, 1'b0, 1'b1, 8'h3C,
          "return-idle");

    if (errors == 0) begin
      $display("DPTI BYTE TX TEST: PASS");
    end else begin
      $display(
        "DPTI BYTE TX TEST: FAIL (%0d errors)",
        errors);
      $fatal(1);
    end

    $finish;
  end

endmodule

`default_nettype wire
