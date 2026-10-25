`default_nettype none
`timescale 1ns / 1ps

/*
 * tb_dpti_bus_arbiter -- the DPTI bus arbiter against an FT2232H that
 * delivers and takes bytes at random, with a parser that stalls at random.
 * BENCH ONLY. Icarus:
 *
 *   iverilog -g2012 -I ../../../../../test/e2e/cosim -o /tmp/tb_arb \
 *       tb_dpti_bus_arbiter.sv ../dpti_bus_arbiter.sv && vvp /tmp/tb_arb
 *
 * Checks, over 20 000 bytes each way under four traffic mixes:
 *   - every byte the host wrote arrives at the parser, in order, once;
 *   - every byte the design offered reaches the host, in order, once;
 *   - the FT2232H's strobe rules (dpti_protocol_check.svh) are never broken
 *     (RXF# in the model never retracts without a read, as on the chip;
 *     TXE# may rise at any time, as on the chip);
 *   - no direction starves: with both sides always busy the run takes no
 *     more than 110% of the bus minimum (3 cycles per byte read, 1 per byte
 *     written).
 */
module tb_dpti_bus_arbiter;

  logic clk = 1'b0;
  always #8.333 clk = ~clk;        // 60 MHz CLKOUT
  logic rst = 1'b1;

  // ---- FT2232H side ---------------------------------------------------------
  byte unsigned ft_rx_q[$];        // what the host wrote, not yet read by the FPGA
  byte unsigned ft_tx_q[$];        // what the FPGA wrote, not yet read by the host
  logic [7:0] ft_data = 8'h00;
  logic       dpti_rxf_n = 1'b1;
  logic       dpti_txe_n = 1'b1;
  logic       rxf_gate = 1'b1;     // random: the host's next bytes arrive this cycle
  logic       txe_gate = 1'b1;     // random: the host reads this cycle (else TXE# may fill)
  integer     host_pending = 0;    // bytes the host has written but USB has not delivered

  wire [7:0] d_in = ft_data;
  wire        dpti_rd_n, dpti_oe_n, dpti_wr_n, d_oe;
  wire [7:0]  d_out;

  // ---- parser side ------------------------------------------------------------
  wire [7:0] rx_data;  wire rx_valid;  logic rx_ready = 1'b0;
  logic [7:0] tx_data = 8'h00; logic tx_valid = 1'b0; wire tx_ready;

  dpti_bus_arbiter #(.CHUNK_BYTES (64)) dut (
    .clk (clk), .rst (rst),
    .dpti_d_in (d_in), .dpti_rxf_n (dpti_rxf_n), .dpti_txe_n (dpti_txe_n),
    .dpti_rd_n (dpti_rd_n), .dpti_oe_n (dpti_oe_n), .dpti_wr_n (dpti_wr_n),
    .dpti_d_out (d_out), .dpti_d_oe (d_oe),
    .rx_byte_data (rx_data), .rx_byte_valid (rx_valid), .rx_byte_ready (rx_ready),
    .tx_byte_data (tx_data), .tx_byte_valid (tx_valid), .tx_byte_ready (tx_ready),
    .state_o (), .turnarounds ()
  );

  // The checker's inputs.
  wire dpti_clk = clk;
  bit  dpc_enabled = 1'b1;
  bit  dpc_fatal   = 1'b1;
  `include "dpti_protocol_check.svh"

  // ---- the FT2232H -----------------------------------------------------------
  integer rd_count = 0, wr_count = 0;
  always @(posedge clk) begin
    if (!dpti_rd_n && !dpti_rxf_n) begin
      void'(ft_rx_q.pop_front());
      rd_count++;
    end
    if (!dpti_wr_n && !dpti_txe_n) begin
      if (!d_oe) begin
        $display("FAIL: WR# low without the FPGA driving D");
        $fatal(1);
      end
      ft_tx_q.push_back(d_out);
      wr_count++;
    end
    // USB delivers the host's bytes in small bursts when the gate allows;
    // RXF# follows the FT2232H's buffer and never retracts without a read.
    if (rxf_gate && host_pending > 0) begin
      int n;
      n = (host_pending < 4) ? host_pending : 4;
      for (int i = 0; i < n; i++) begin
        ft_rx_q.push_back(8'((rx_seen_offset) * 7 + 3));
        rx_seen_offset++;
      end
      host_pending -= n;
    end
    dpti_rxf_n <= (ft_rx_q.size() == 0);
    dpti_txe_n <= (ft_tx_q.size() >= 1024) || !txe_gate;
    ft_data    <= (ft_rx_q.size() > 0) ? ft_rx_q[0] : 8'hxx;
  end

  // ---- the parser (consumes rx bytes, checks the sequence) ------------------
  integer rx_seen = 0, errors = 0;
  always @(posedge clk) begin
    if (rx_valid && rx_ready) begin
      if (rx_data !== 8'(rx_seen * 7 + 3)) begin
        if (errors < 10) $display("FAIL: rx byte %0d = %02h, want %02h", rx_seen, rx_data, 8'(rx_seen * 7 + 3));
        errors++;
      end
      rx_seen++;
    end
  end

  // ---- the design's transmitter (a sequence of bytes) ------------------------
  integer tx_sent = 0, tx_total = 0;
  always @(posedge clk) begin
    if (tx_valid && tx_ready) begin
      tx_valid <= (tx_sent + 1 < tx_total);
      tx_data  <= 8'((tx_sent + 1) * 5 + 1);
      tx_sent++;
    end
  end

  integer rx_seen_offset = 0;
  task automatic host_write(input int n);
    host_pending += n;
  endtask

  // The host reads what the FPGA sent and checks the sequence.
  integer host_read = 0;
  task automatic host_drain();
    while (ft_tx_q.size() > 0) begin
      byte unsigned b = ft_tx_q.pop_front();
      if (b !== 8'(host_read * 5 + 1)) begin
        if (errors < 10) $display("FAIL: tx byte %0d = %02h, want %02h", host_read, b, 8'(host_read * 5 + 1));
        errors++;
      end
      host_read++;
    end
  endtask

  // A traffic mix: how often the parser is ready, how often the host offers
  // or reads. Runs until both directions moved `n` bytes.
  integer rx_busy_cycles = 0, tx_busy_cycles = 0;
  task automatic mix(input string name, input int n, input int p_ready, input int p_rxf, input int p_txe);
    int t;
    rx_seen = 0; rx_seen_offset = 0; host_read = 0; tx_sent = 0; tx_total = n;
    rx_busy_cycles = 0; tx_busy_cycles = 0;
    ft_rx_q.delete(); ft_tx_q.delete();
    host_write(n);
    @(posedge clk);
    tx_valid <= 1'b1; tx_data <= 8'(1);
    t = 0;
    while ((rx_seen < n || host_read < n) && t < 40 * n) begin
      @(posedge clk);
      rx_ready <= ($urandom_range(0, 99) < p_ready);
      rxf_gate <= ($urandom_range(0, 99) < p_rxf);
      txe_gate <= ($urandom_range(0, 99) < p_txe);
      if (!dpti_oe_n) rx_busy_cycles++;
      if (d_oe)       tx_busy_cycles++;
      if (txe_gate) host_drain();
      t++;
    end
    rx_ready <= 1'b1; rxf_gate <= 1'b1; txe_gate <= 1'b1;
    repeat (50) @(posedge clk);
    host_drain();
    if (rx_seen != n || host_read != n) begin
      $display("FAIL: %s: rx %0d / %0d, tx read %0d / %0d after %0d cycles", name, rx_seen, n, host_read, n, t);
      errors++;
    end
    $display("  %s: %0d bytes each way in %0d cycles; bus read %0d cycles, written %0d cycles; violations %0d",
             name, n, t, rx_busy_cycles, tx_busy_cycles, dpc_violations);
    // With both sides always busy the bus is the only limit: 3 cycles per
    // byte read (request, capture, hand over), 1 per byte written, plus a
    // turnaround every chunk. More than 10% over that sum means a direction
    // waited for nothing.
    if (p_ready == 100 && p_rxf == 100 && p_txe == 100 && t * 10 > (3 * n + n) * 11) begin
      $display("FAIL: %s: %0d cycles for %0d bytes each way, more than 110%% of the bus minimum %0d",
               name, t, n, 4 * n);
      errors++;
    end
  endtask

  initial begin
    repeat (5) @(posedge clk);
    rst = 1'b0;
    repeat (5) @(posedge clk);
    mix("both always busy",       20000, 100, 100, 100);
    mix("parser stalls, host slow", 20000, 40, 60, 100);
    mix("host reads slowly",       20000, 100, 100, 30);
    mix("everything random",       20000, 50, 50, 50);
    if (errors == 0 && dpc_violations == 0) $display("PASS: tb_dpti_bus_arbiter");
    else $display("FAIL: tb_dpti_bus_arbiter: %0d error(s), %0d violation(s)", errors, dpc_violations);
    $finish;
  end

endmodule

`default_nettype wire
