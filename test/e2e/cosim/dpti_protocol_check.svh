// dpti_protocol_check.svh -- the FT2232H's side of the DPTI bus, as rules.
// BENCH ONLY: `include this inside a bench module that has the signals named
// below in scope (the bench's FT2232H model drives rxf_n / txe_n, the DUT
// drives rd_n / oe_n / wr_n, dpti_clk is CLKOUT).
//
// FTDI AN_130 (FT2232H in FT245 synchronous FIFO mode):
//   - "Only a read or a write cycle can be performed at any one time."
//   - OE# "should be driven low at least 1 clock period before driving RD#
//     low" (the FT2232H turns the data bus around on OE#).
//   - RD# is honoured only while RXF# is low; WR# only while TXE# is low.
//
// Every rule is checked at the rising edge of CLKOUT, where the FT2232H
// samples its strobes. A violation ends the simulation: on the board it is a
// bus fight (two drivers on D[7:0]) or a lost or garbage byte, neither of
// which any later check could attribute to its cause.
//
// Expected to pass on the synchronous runtime (the host never writes while
// a result is in flight) and to FAIL on systolic_dma_top before the DPTI
// bus arbiter (dpti_bus_arbiter.sv): its byte receiver and transmitter each
// own their strobes with nothing in between.

logic dpc_oe_n_prev = 1'b1;
int   dpc_violations = 0;
// Strobes that met the chip's flag high: RD# while RXF# is high, WR# while
// TXE# is high. In the FT245 synchronous mode these are not transfers (the
// chip moves a byte only when flag and strobe are both low at the edge) and
// a PHY that registers its strobes one edge behind the flags -- the
// registered PHY, dpti_sync245_phy.sv -- issues one such strobe after every
// burst; it counts them itself (tx_reject_count) and repeats the byte. They
// are counted here apart from the violations and never fatal.
int   dpc_idle_strobes = 0;

always @(posedge dpti_clk) begin
  if (dpc_enabled) begin
    if (!dpti_wr_n && !dpti_oe_n) begin
      dpc_violations++;
      $display("DPTI PROTOCOL VIOLATION t=%0t: WR# low while OE# low -- the FPGA drives D[7:0] while the FT2232H drives it (bus fight)", $time);
    end
    if (!dpti_rd_n && dpti_oe_n) begin
      dpc_violations++;
      $display("DPTI PROTOCOL VIOLATION t=%0t: RD# low with OE# high -- no byte is on the bus", $time);
    end
    if (!dpti_rd_n && dpc_oe_n_prev) begin
      dpc_violations++;
      $display("DPTI PROTOCOL VIOLATION t=%0t: RD# low in the first cycle of OE# low -- OE# must lead RD# by at least one CLKOUT", $time);
    end
    if (!dpti_rd_n && dpti_rxf_n) begin
      dpc_idle_strobes++;
      if (dpc_idle_strobes <= 4)
        $display("DPTI PROTOCOL NOTE t=%0t: RD# low while RXF# high -- no byte moves (strobe %0d)", $time, dpc_idle_strobes);
    end
    if (!dpti_wr_n && dpti_txe_n) begin
      dpc_idle_strobes++;
      if (dpc_idle_strobes <= 4)
        $display("DPTI PROTOCOL NOTE t=%0t: WR# low while TXE# high -- the byte is not taken, the PHY repeats it (strobe %0d)", $time, dpc_idle_strobes);
    end
    if (dpc_violations != 0 && dpc_fatal)
      $fatal(1, "DPTI protocol violation (see above)");
  end
  dpc_oe_n_prev <= dpti_oe_n;
end
