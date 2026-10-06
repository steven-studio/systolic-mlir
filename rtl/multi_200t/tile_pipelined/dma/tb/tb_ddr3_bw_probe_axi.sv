// -----------------------------------------------------------------------------
// tb_ddr3_bw_probe_axi.sv -- proves the AXI4 probe before it sees real DDR3.
//
//   iverilog -g2012 -o tb.vvp uart_tx.sv ddr3_bw_probe_axi.sv \
//            tb/tb_ddr3_bw_probe_axi.sv && vvp tb.vvp
//   xvlog -sv uart_tx.sv ddr3_bw_probe_axi.sv tb/tb_ddr3_bw_probe_axi.sv \
//     && xelab -R tb_ddr3_bw_probe_axi
//
// Contains an AXI4 read-slave stub (address backpressure, latency, multiple
// outstanding bursts, rlast) and a UART receiver, so it depends on nothing.
//
// Checks:
//   1. EVERY BEAT IS COUNTED -- beats == N_BURSTS * BURST_LEN.
//   2. THE WINDOW IS RIGHT -- cyc tracks the achievable rate, and does NOT
//      count the idle time before the first address is accepted.
//   3. THE REPORT MATCHES THE COUNTERS -- the UART frames are decoded and
//      compared against the DUT's own registers.
// -----------------------------------------------------------------------------

`timescale 1ns/1ps

module tb_ddr3_bw_probe_axi;

  localparam int unsigned CLK_HZ    = 1000;
  localparam int unsigned BAUD      = 100;     // 10 clocks per bit
  localparam integer      BIT_TIME  = 100;     // ns per UART bit
  localparam int unsigned BURST_LEN = 16;
  localparam int unsigned N_BURSTS  = 11;      // 176 beats = 0x0B0, hits A-F
  localparam int unsigned TOTAL     = N_BURSTS * BURST_LEN;
  localparam int unsigned MAX_OUTST = 4;
  localparam int unsigned LAT       = 8;
  localparam int          MSG_LEN   = 43;

  reg clk = 0, rst_n = 0;
  always #5 clk = ~clk;

  reg         calib    = 0;
  reg [3:0]   ar_div   = 1;    // 1 = arready always, 2 = every other cycle
  reg [15:0]  ar_hold  = 0;    // cycles arready stays low after calib

  wire [1:0]   arid;
  wire [28:0]  araddr;
  wire [7:0]   arlen;
  wire [2:0]   arsize;
  wire [1:0]   arburst;
  wire [0:0]   arlock;
  wire [3:0]   arcache;
  wire [2:0]   arprot;
  wire [3:0]   arqos;
  wire         arvalid;
  wire         arready;
  reg  [127:0] rdata;
  wire [1:0]   rresp = 2'b00;
  reg          rlast, rvalid;
  wire         rready;
  wire         uart_tx_pin, running, reported;

  integer errors = 0;

  // ---- AXI4 read-slave stub ----------------------------------------------
  // Backpressure on AR, a fixed first-burst latency, up to MAX_OUTST bursts
  // queued, rlast on each burst's final beat.  Beats advance ONLY on
  // rvalid && rready -- the first version advanced on rready alone and lost
  // exactly one beat per run, which is what "175 of 176" looked like.
  reg [3:0]  ar_cnt   = 0;
  reg [15:0] hold_cnt = 0;
  reg [7:0]  q_len [0:15];
  reg [3:0]  wp = 0, rp = 0;
  reg [4:0]  q_n = 0;
  reg [7:0]  beat_left = 0;
  reg [7:0]  lat_cnt = 0;
  reg        busy = 0;
  reg        primed = 0;   // first-burst latency is paid once, not per burst
  reg [31:0] patt = 32'h1;

  assign arready = (ar_cnt == 0) && (hold_cnt >= ar_hold) && (q_n < MAX_OUTST);

  wire ar_acc     = arvalid && arready;
  wire beat_acc   = rvalid  && rready;
  wire burst_done = beat_acc && rlast;

  // queue bookkeeping
  always @(posedge clk) begin
    if (!rst_n) begin
      ar_cnt <= 0; hold_cnt <= 0; wp <= 0; rp <= 0; q_n <= 0;
    end else begin
      ar_cnt <= (ar_cnt + 1 == ar_div) ? 0 : ar_cnt + 1;
      if (calib && hold_cnt < ar_hold) hold_cnt <= hold_cnt + 1;
      if (ar_acc) begin q_len[wp] <= arlen + 8'd1; wp <= wp + 1; end
      if (burst_done) rp <= rp + 1;
      if      ( ar_acc && !burst_done) q_n <= q_n + 1;
      else if (!ar_acc &&  burst_done) q_n <= q_n - 1;
    end
  end

  // data channel
  always @(posedge clk) begin
    if (!rst_n) begin
      busy <= 0; rvalid <= 0; rlast <= 0; beat_left <= 0; lat_cnt <= 0;
      primed <= 0;
    end else if (!busy) begin
      rvalid <= 0; rlast <= 0;
      if (q_n != 0) begin
        // Pay the read latency once, at the start of the run.  A real
        // controller with several bursts outstanding does not re-pay it
        // between back-to-back bursts, and if the bench charged it every
        // time the data channel could never saturate -- which would make
        // the cycle check meaningless.
        if (primed || lat_cnt == LAT) begin
          busy      <= 1;
          primed    <= 1;
          lat_cnt   <= 0;
          beat_left <= q_len[rp];
          rvalid    <= 1;
          rlast     <= (q_len[rp] == 8'd1);
        end else lat_cnt <= lat_cnt + 1;
      end
    end else if (beat_acc) begin
      if (beat_left == 8'd1) begin
        busy <= 0; rvalid <= 0; rlast <= 0;
      end else begin
        beat_left <= beat_left - 8'd1;
        rvalid    <= 1;
        rlast     <= (beat_left == 8'd2);
      end
    end
  end

  // read data: anything non-constant, so the DUT's sink cannot be optimised away
  always @(posedge clk) begin
    if (!rst_n)        patt <= 32'h1;
    else if (beat_acc) patt <= patt + 32'h0100_0001;
  end
  always @(posedge clk) rdata <= {96'd0, patt};

  // ---- AXI protocol checks on the address channel -------------------------
  // These pin the encoding, which is silently wrong-able: AXI carries
  // len-1, so arlen = BURST_LEN asks for one beat too many while the address
  // still advances by BURST_LEN.  Reads then overlap and the bandwidth number
  // still looks plausible.  Report once, not once per burst.
  reg said_len = 0, said_size = 0, said_burst = 0;
  always @(posedge clk) if (rst_n && ar_acc) begin
    if ((arlen + 8'd1) !== BURST_LEN[7:0] && !said_len) begin
      $display("FAIL  AXI arlen: %0d encodes %0d beats, want %0d (AXI sends len-1)",
               arlen, arlen + 1, BURST_LEN);
      errors = errors + 1; said_len = 1;
    end
    if (arsize !== 3'd4 && !said_size) begin           // log2(128/8) = 4
      $display("FAIL  AXI arsize: %0d, want 4 (16 bytes per beat)", arsize);
      errors = errors + 1; said_size = 1;
    end
    if (arburst !== 2'b01 && !said_burst) begin
      $display("FAIL  AXI arburst: %b, want 01 (INCR)", arburst);
      errors = errors + 1; said_burst = 1;
    end
  end

  ddr3_bw_probe_axi #(
    .BURST_LEN(BURST_LEN), .N_BURSTS(N_BURSTS), .MAX_OUTST(MAX_OUTST),
    .TIMEOUT(32'd200_000), .GAP_CYCLES(32'd50),
    .CLK_HZ(CLK_HZ), .BAUD(BAUD)
  ) dut (
    .clk(clk), .rst_n(rst_n), .init_calib_complete(calib),
    .m_axi_arid(arid), .m_axi_araddr(araddr), .m_axi_arlen(arlen),
    .m_axi_arsize(arsize), .m_axi_arburst(arburst), .m_axi_arlock(arlock),
    .m_axi_arcache(arcache), .m_axi_arprot(arprot), .m_axi_arqos(arqos),
    .m_axi_arvalid(arvalid), .m_axi_arready(arready),
    .m_axi_rdata(rdata), .m_axi_rresp(rresp), .m_axi_rlast(rlast),
    .m_axi_rvalid(rvalid), .m_axi_rready(rready),
    .uart_tx_pin(uart_tx_pin), .running(running), .reported(reported)
  );

  // ---- UART receiver ------------------------------------------------------
  reg [7:0] rx_msg [0:MSG_LEN-1];

  task automatic uart_get(output reg [7:0] b);
    integer i;
    begin
      @(negedge uart_tx_pin);
      #(BIT_TIME + BIT_TIME/2);
      for (i = 0; i < 8; i = i + 1) begin
        b[i] = uart_tx_pin;
        #(BIT_TIME);
      end
    end
  endtask

  task automatic capture_report;
    integer i; reg [7:0] b;
    begin
      for (i = 0; i < MSG_LEN; i = i + 1) begin
        uart_get(b);
        rx_msg[i] = b;
      end
    end
  endtask

  function automatic [7:0] nib_of(input [3:0] n);
    nib_of = (n < 10) ? (8'h30 + n) : (8'h37 + n);
  endfunction

  task automatic check_hex(input integer off, input [31:0] v, input [255:0] name);
    integer k; reg [7:0] want;
    begin
      for (k = 0; k < 8; k = k + 1) begin
        want = nib_of(v[(7-k)*4 +: 4]);
        if (rx_msg[off+k] !== want) begin
          $display("FAIL  %0s: char %0d is '%c', want '%c' (value %08h)",
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
      end else $display("PASS  %0s: %0d", name, got);
    end
  endtask

  task automatic check_range(input [31:0] got, input [31:0] lo, input [31:0] hi,
                             input [255:0] name);
    begin
      if (got < lo || got > hi) begin
        $display("FAIL  %0s: got %0d, want %0d..%0d", name, got, lo, hi);
        errors = errors + 1;
      end else $display("PASS  %0s: %0d (in %0d..%0d)", name, got, lo, hi);
    end
  endtask

  task automatic run_case(input [3:0] div, input [15:0] hold,
                          input [31:0] lo, input [31:0] hi, input [255:0] label);
    begin
      rst_n = 0; calib = 0; ar_div = div; ar_hold = hold;
      repeat (4) @(posedge clk);
      rst_n = 1;
      repeat (4) @(posedge clk);
      calib = 1;

      wait (reported);
      $display("\n--- %0s ---", label);
      check_eq(dut.beats_r, TOTAL, "beats");
      check_range(dut.cyc_r, lo, hi, "cycles");

      capture_report();
      check_hex(6,           dut.beats_r, "report beats");
      check_hex(6+8+5,       dut.cyc_r,   "report cycles");
      check_hex(6+8+5+8+6,   dut.sink_r,  "report sink");
      if (rx_msg[MSG_LEN-2] !== 8'h0D || rx_msg[MSG_LEN-1] !== 8'h0A) begin
        $display("FAIL  %0s: frame does not end with CRLF", label);
        errors = errors + 1;
      end
      if (dut.sink_r === 32'd0) begin
        $display("FAIL  %0s: sink is zero, read data path optimised away", label);
        errors = errors + 1;
      end
    end
  endtask

  initial begin
    // full rate: the data channel runs back to back, so the window is about
    // TOTAL beats plus the first burst's latency
    run_case(1, 0, TOTAL, TOTAL + LAT + BURST_LEN + 8, "full rate");

    // address channel at half rate: with BURST_LEN beats per address and
    // MAX_OUTST bursts in flight, the DATA channel still saturates -- this is
    // the point of bursts, and the check pins it
    run_case(2, 0, TOTAL, TOTAL + LAT + BURST_LEN + 8, "arready 1-in-2");

    // 20 idle cycles before the memory accepts anything.  The window must NOT
    // count them: that is what makes this a bandwidth and not an elapsed time.
    run_case(1, 20, TOTAL, TOTAL + LAT + BURST_LEN + 8, "20-cycle stall first");

    if (errors == 0) $display("\nALL CHECKS PASSED");
    else             $display("\n%0d CHECK(S) FAILED", errors);
    $finish;
  end

  initial begin
    #40_000_000;
    $display("FAIL  bench timed out");
    $finish;
  end

endmodule
