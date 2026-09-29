// -----------------------------------------------------------------------------
// tb_ddr3_bw_probe.sv -- proves the probe counts and reports correctly BEFORE
// it ever sees a real MIG.
//
//   icarus: iverilog -g2012 -o tb.vvp uart_tx.sv ddr3_bw_probe.sv \
//           tb/tb_ddr3_bw_probe.sv && vvp tb.vvp
//   xsim:   xvlog -sv uart_tx.sv ddr3_bw_probe.sv tb/tb_ddr3_bw_probe.sv \
//           && xelab -R tb_ddr3_bw_probe
//
// The bench contains its own MIG UI stub (backpressure + fixed-latency return
// queue) and its own UART receiver, so it depends on nothing else.
//
// Checks:
//   1. EVERY BEAT IS COUNTED -- beats == N_READS with no backpressure.  If this
//      fails the bandwidth number is meaningless.
//   2. THE WINDOW IS RIGHT -- cyc tracks the issue rate: ~N_READS cycles at
//      full rate, ~2*N_READS at half rate.  Catches a window that opens early,
//      never opens, or counts while idle.
//   3. THE REPORT MATCHES THE COUNTERS -- the bench decodes the UART frames and
//      compares the ASCII against the DUT's registers.  This is what stops you
//      reading a wrong number off a terminal and believing it.
// -----------------------------------------------------------------------------

`timescale 1ns/1ps

module tb_ddr3_bw_probe;

  // small baud divisor so the report does not dominate simulation time
  localparam int unsigned CLK_HZ  = 1000;
  localparam int unsigned BAUD    = 100;        // DIV = 10 clocks per bit
  localparam int unsigned BITCLK  = 10;         // clocks per UART bit
  localparam int unsigned N_READS = 171;   // 0x0000_00AB -- exercises the
                                           // A-F branch of the hex formatter
  localparam int unsigned LAT     = 6;
  localparam int unsigned MSG_LEN = 43;

  reg clk = 0, rst_n = 0;
  always #5 clk = ~clk;                          // 10 ns period
  localparam integer BIT_TIME = 10 * BITCLK;     // 100 ns per UART bit

  reg calib = 0;
  reg [3:0]  rdy_div  = 1;                       // 1 = always ready, 2 = half rate
  reg [15:0] rdy_hold = 0;                       // cycles app_rdy stays low after calib

  wire [28:0] app_addr;
  wire [2:0]  app_cmd;
  wire        app_en, uart_tx_pin, running, reported;
  wire        app_rdy;
  reg [127:0] app_rd_data;
  wire        app_rd_data_valid;

  integer errors = 0;

  // ---- MIG UI stub -------------------------------------------------------
  reg [3:0]      rdy_cnt  = 0;
  reg [15:0]     hold_cnt = 0;
  reg [LAT-1:0]  pipe    = 0;
  reg [31:0]     patt    = 32'h1;

  always @(posedge clk) begin
    if (!rst_n) begin
      rdy_cnt <= 0; pipe <= 0; patt <= 32'h1; hold_cnt <= 0;
    end else begin
      rdy_cnt <= (rdy_cnt + 1 == rdy_div) ? 0 : rdy_cnt + 1;
      if (calib && hold_cnt < rdy_hold) hold_cnt <= hold_cnt + 1'b1;
      pipe    <= {pipe[LAT-2:0], (app_en & app_rdy)};
      if (app_en & app_rdy) patt <= patt + 32'h0100_0001;
    end
  end

  // continuous assign, NOT always @(*): with rdy_div = 1 the counter never
  // changes, so an always @(*) would never fire and app_rdy would stay X.
  assign app_rdy = (rdy_cnt == 0) && (hold_cnt >= rdy_hold);
  assign app_rd_data_valid = pipe[LAT-1];
  always @(posedge clk) app_rd_data <= {96'd0, patt};

  ddr3_bw_probe #(
    .N_READS(N_READS), .TIMEOUT(32'd100_000), .GAP_CYCLES(32'd50),
    .CLK_HZ(CLK_HZ), .BAUD(BAUD)
  ) dut (
    .clk(clk), .rst_n(rst_n), .init_calib_complete(calib),
    .app_addr(app_addr), .app_cmd(app_cmd), .app_en(app_en), .app_rdy(app_rdy),
    .app_rd_data(app_rd_data), .app_rd_data_valid(app_rd_data_valid),
    .uart_tx_pin(uart_tx_pin), .running(running), .reported(reported)
  );

  // ---- UART receiver -----------------------------------------------------
  reg [7:0] rx_msg [0:MSG_LEN-1];
  integer   rx_n = 0;

  task automatic uart_get(output reg [7:0] b);
    integer i;
    begin
      @(negedge uart_tx_pin);            // start bit
      #(BIT_TIME + BIT_TIME/2);          // centre of bit 0
      for (i = 0; i < 8; i = i + 1) begin
        b[i] = uart_tx_pin;
        #(BIT_TIME);
      end
    end
  endtask

  task automatic capture_report;
    integer i;
    reg [7:0] b;
    begin
      rx_n = 0;
      for (i = 0; i < MSG_LEN; i = i + 1) begin
        uart_get(b);
        rx_msg[i] = b;
        rx_n = rx_n + 1;
      end
    end
  endtask

  function automatic [7:0] nib_of(input [3:0] n);
    nib_of = (n < 10) ? (8'h30 + n) : (8'h37 + n);
  endfunction

  // compare eight ASCII hex chars at rx_msg[off] against v
  task automatic check_hex(input integer off, input [31:0] v, input [255:0] name);
    integer k;
    reg [7:0] want;
    begin
      for (k = 0; k < 8; k = k + 1) begin
        want = nib_of(v[(7-k)*4 +: 4]);
        if (rx_msg[off+k] !== want) begin
          $display("FAIL  %0s: char %0d is '%c', want '%c'  (value %08h)",
                   name, k, rx_msg[off+k], want, v);
          errors = errors + 1;
        end
      end
    end
  endtask

  task automatic check_eq(input [31:0] got, input [31:0] want, input [255:0] name);
    begin
      if (got !== want) begin
        $display("FAIL  %0s: got %0d, want %0d", name, got, want);
        errors = errors + 1;
      end else
        $display("PASS  %0s: %0d", name, got);
    end
  endtask

  task automatic check_range(input [31:0] got, input [31:0] lo, input [31:0] hi,
                             input [255:0] name);
    begin
      if (got < lo || got > hi) begin
        $display("FAIL  %0s: got %0d, want %0d..%0d", name, got, lo, hi);
        errors = errors + 1;
      end else
        $display("PASS  %0s: %0d (in %0d..%0d)", name, got, lo, hi);
    end
  endtask

  // ---- run one measurement and read the report back ----------------------
  task automatic run_case(input [3:0] div, input [15:0] hold,
                          input [31:0] cyc_lo, input [31:0] cyc_hi,
                          input [255:0] label);
    begin
      rst_n = 0; calib = 0; rdy_div = div; rdy_hold = hold;
      repeat (4) @(posedge clk);
      rst_n = 1;
      repeat (4) @(posedge clk);
      calib = 1;

      wait (reported);                             // S_SEND reached
      $display("\n--- %0s ---", label);
      check_eq(dut.beats_r, N_READS, "beats");
      check_range(dut.cyc_r, cyc_lo, cyc_hi, "cycles");

      capture_report();
      // "BEATS=" is 6 chars, then 8; " CYC=" is 5, then 8; " SINK=" is 6, then 8
      check_hex(6,       dut.beats_r, "report beats");
      check_hex(6+8+5,   dut.cyc_r,   "report cycles");
      check_hex(6+8+5+8+6, dut.sink_r, "report sink");
      if (rx_msg[MSG_LEN-2] !== 8'h0D || rx_msg[MSG_LEN-1] !== 8'h0A) begin
        $display("FAIL  %0s: frame does not end with CRLF", label);
        errors = errors + 1;
      end
      if (dut.sink_r === 32'd0) begin
        $display("FAIL  %0s: sink is zero, the read data path was optimised away",
                 label);
        errors = errors + 1;
      end
    end
  endtask

  initial begin
    // full rate: one command per cycle, so the window is N_READS cycles plus
    // the return latency of the last beat
    run_case(1, 0, N_READS, N_READS + LAT + 4, "full rate, no start delay");

    // half rate: app_rdy every other cycle, so issue takes twice as long
    run_case(2, 0, 2*N_READS - 4, 2*N_READS + LAT + 4, "half rate (app_rdy 1-in-2)");

    // 20 idle cycles before the memory accepts anything.  The window must NOT
    // count them: this is the case that pins 'the window opens on the first
    // accepted command', which is what makes the bandwidth a bandwidth and not
    // an elapsed time.
    run_case(1, 20, N_READS, N_READS + LAT + 4, "full rate after a 20-cycle stall");

    if (errors == 0) $display("\nALL CHECKS PASSED");
    else             $display("\n%0d CHECK(S) FAILED", errors);
    $finish;
  end

  initial begin
    #20_000_000;
    $display("FAIL  bench timed out");
    $finish;
  end

endmodule
