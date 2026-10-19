`default_nettype none
`timescale 1ns / 1ps

/*
 * tb_dpti_direct -- the board behind its DPTI link, driven by SystemVerilog
 * tasks that stand in for the host. BENCH ONLY.
 *
 * tb_dpti_cosim (test/e2e/cosim) runs the real runtime over a socket; it can
 * only send what the runtime sends, when the runtime sends it. This bench
 * puts bytes on the link at a chosen cycle, which is what the DPTI / DMA
 * hazards of the asynchronous design need: a MEM_WRITE arriving while
 * another context's write-back burst is open, a result tile leaving while a
 * slab is still coming in, a descriptor the hardware refuses, two jobs sent
 * to one device. systolic_dma_top is built exactly as dma_top_build.tcl
 * builds the 1x8x8 + 3x4x4 bitstream (external scheduler, no legacy job
 * ports, v1 operand path); only the Xilinx IP is replaced (tb/xil_stubs.sv,
 * tb/mig_axi_model.sv, test/e2e/cosim/fp_ieee.sv).
 *
 * The FT2232H model is the one of tb_dpti_cosim (60 MHz CLKOUT; RXF# low
 * while the host queue holds a byte, RD# consumes it; WR# while TXE# low
 * hands the FPGA's byte to the host queue), plus the FT2232H's own rules on
 * the strobes (test/e2e/cosim/dpti_protocol_check.svh), which count
 * violations instead of stopping so a test can report them.
 *
 * Wire format, byte for byte what runtime/fpga_matmul_tiled.cpp sends:
 *   WRITE32    0x01 addr data[7:0] data[15:8] data[23:16] data[31:24]
 *   MEM_WRITE  0x02 addr[31:0] LE, length[31:0] LE, HOST_SLAB_BYTES payload
 *   descriptor 14 WRITE32 (job_id, device_id, m, n, k, start, est, a/b/c
 *              base lo/hi), CONTROL = 1 last
 *   slab       windows of 8 steps: words w*128 + r*8 + koff (A, row r),
 *              w*128 + 64 + koff*8 + c (B, column c), fp32 little-endian
 *   result     P x P fp32 row-major, streamed back; with +completion_header=1
 *              (RTL with the completion record) a 16-byte header first:
 *              magic 0x5953_4A43, job_id, device_id, status, payload_bytes,
 *              seq
 *
 * Operands are small integers (exact in binary32), so a result is checked
 * as an integer against an integer reference; the floating-point unit is the
 * IEEE model, not fp_model.sv's integer pipeline.
 *
 *   +test=basic       one job per device, one at a time (the synchronous host)
 *   +test=multi       four jobs, one per device, all submitted before any
 *                     result is read; the completions are attributed by the
 *                     header (needs +completion_header=1) -- without it the
 *                     bench can only read them in submission order, which is
 *                     the ambiguity the header exists to remove
 *   +test=refused     a descriptor with k = K_MAX + 1 (device 1), then a valid
 *                     job on device 0: the link must not wedge, the valid job
 *                     must complete; with the header the refusal is reported
 *                     as a completion with status 1 and no payload
 *   +test=collision   (run with +w_stall=8) a MEM_WRITE for device 1 placed in
 *                     the middle of device 0's write-back burst: the memory
 *                     must see one master at a time and both images intact
 *   +test=direction   a MEM_WRITE still streaming in while device 0's result
 *                     streams out: no strobe-rule violation, both intact
 *   +test=hol         two descriptors to device 0 back to back while nothing
 *                     reads the first result: documents the head-of-line
 *                     stall of the descriptor path (expected on every RTL;
 *                     the host's credit rule is what avoids it)
 *
 * Every test ends with one line `RESULT <test> PASS|FAIL|STALL ...`.
 */

module tb_dpti_direct #(
  parameter bit     USE_V2    = 1'b0,
  parameter integer K_MAX_8X8 = 16,
  parameter integer K_MAX_4X4 = 16,
  // The top's DPTI PHY: 1 the registered FT2232H PHY (the board's default),
  // 0 the legacy pads path with dpti_bus_arbiter (run_dpti_direct.sh PHY=).
  parameter bit     DPTI_PHY_SYNC245 = 1'b1
);

  // ---- clocks, reset ---------------------------------------------------
  logic sys_clk  = 1'b0;
  logic dpti_clk = 1'b0;
  always #5     sys_clk  = ~sys_clk;     // R4: 100 MHz board oscillator
  always #8.333 dpti_clk = ~dpti_clk;    // FT2232H CLKOUT, 60 MHz

  logic cpu_resetn = 1'b0;

  wire [7:0]  led;
  wire [14:0] ddr3_addr;  wire [2:0] ddr3_ba;
  wire        ddr3_cas_n, ddr3_ras_n, ddr3_reset_n, ddr3_we_n;
  wire [0:0]  ddr3_ck_n, ddr3_ck_p, ddr3_cke, ddr3_odt;
  wire [1:0]  ddr3_dm;
  wire [15:0] ddr3_dq;    wire [1:0] ddr3_dqs_n, ddr3_dqs_p;

  // ---- the FT2232H side of the DPTI bus --------------------------------
  wire  [7:0] dpti_d;
  wire        dpti_rd_n, dpti_wr_n, dpti_oe_n;
  logic [7:0] ft_data  = 8'h00;
  logic       ft_rxf_n = 1'b1;     // no host byte yet
  logic       ft_txe_n = 1'b0;     // the host side has room unless a test says not
  wire        dpti_rxf_n = ft_rxf_n;
  wire        dpti_txe_n = ft_txe_n;

  // The FT2232H drives D only while the FPGA holds OE# low.
  assign dpti_d = !dpti_oe_n ? ft_data : 8'bz;

  systolic_dma_top #(
    .N (8), .K_MAX_8X8 (K_MAX_8X8), .K_MAX_4X4 (K_MAX_4X4), .K_DIM (K_MAX_8X8),
    .NUM_8X8 (1), .NUM_4X4 (3),
    .USE_EXTERNAL_SCHEDULER (1'b1),
    .USE_LEGACY_JOB_PORTS (1'b0),
    .USE_V2 (USE_V2),
    .DPTI_PHY_SYNC245 (DPTI_PHY_SYNC245),
    .EXPECT_WR_CHK (32'h3F88_0780),
    .EXPECT_C_CHK (32'hC74B_2660)
  ) dut (
    .sys_clk_pin (sys_clk), .cpu_resetn (cpu_resetn), .led (led),
    .ddr3_addr (ddr3_addr), .ddr3_ba (ddr3_ba), .ddr3_cas_n (ddr3_cas_n),
    .ddr3_ck_n (ddr3_ck_n), .ddr3_ck_p (ddr3_ck_p), .ddr3_cke (ddr3_cke),
    .ddr3_ras_n (ddr3_ras_n), .ddr3_reset_n (ddr3_reset_n),
    .ddr3_we_n (ddr3_we_n), .ddr3_dq (ddr3_dq),
    .ddr3_dqs_n (ddr3_dqs_n), .ddr3_dqs_p (ddr3_dqs_p),
    .ddr3_dm (ddr3_dm), .ddr3_odt (ddr3_odt),
    .scheduler_fold_start (1'b0), .scheduler_c_done (),
    .job_valid (1'b0), .job_ready (), .job_id (32'd0), .job_device_id (32'd0),
    .job_m (32'd0), .job_n (32'd0), .job_k (32'd0),
    .job_start_cycle (32'd0), .job_est_cycles (32'd0),
    .job_a_base (64'd0), .job_b_base (64'd0), .job_c_base (64'd0),
    .dpti_wr_valid (1'b0), .dpti_wr_ready (), .dpti_wr_addr (8'd0),
    .dpti_wr_data (32'd0),
    .dpti_d (dpti_d), .dpti_rxf_n (ft_rxf_n), .dpti_txe_n (ft_txe_n),
    .dpti_clkout (dpti_clk),
    .dpti_rd_n (dpti_rd_n), .dpti_wr_n (dpti_wr_n), .dpti_oe_n (dpti_oe_n),
    .dpti_siwun (1'b1)
  );

  // ---- FT2232H FIFO model ----------------------------------------------
  // host_tx_q: host -> FPGA; what the host has written and the FPGA has not
  // read yet (on the board, the FT2232H's receive buffer plus whatever the
  // host's blocked write still holds). host_rx_q: FPGA -> host; what the
  // FPGA has sent and the host has not read yet. In synchronous FIFO mode
  // the FT2232H holds 2 x 512 bytes each way (AN_130), so TXE# goes high
  // once the host leaves TX_BUF_BYTES unread -- the FPGA then cannot send,
  // which is the half of the deadlock the host's rule "always drain the IN
  // direction" exists for.
  localparam int TX_BUF_BYTES = 1024;
  byte unsigned host_tx_q[$];
  byte unsigned host_rx_q[$];
  longint       rd_strobes = 0;
  longint       wr_strobes = 0;
  int           max_unconsumed = 0;   // the deepest host_tx_q ever got

  always @(posedge dpti_clk) begin
    if (!dpti_rd_n && !ft_rxf_n) begin
      void'(host_tx_q.pop_front());
      rd_strobes++;
    end
    if (!dpti_wr_n && !ft_txe_n) begin
      host_rx_q.push_back(dpti_d);
      wr_strobes++;
    end
    ft_rxf_n <= (host_tx_q.size() == 0);
    ft_data  <= (host_tx_q.size() > 0) ? host_tx_q[0] : 8'h00;
    ft_txe_n <= (host_rx_q.size() >= TX_BUF_BYTES);
    if (host_tx_q.size() > max_unconsumed) max_unconsumed = host_tx_q.size();
  end

  // ---- the FT2232H's rules on the strobes ------------------------------
  bit dpc_enabled = 1'b1;
  bit dpc_fatal   = 1'b0;      // count; the test decides what a count means
  `include "dpti_protocol_check.svh"

  // ---- geometry --------------------------------------------------------
  localparam integer SLAB_K_8X8 = ((K_MAX_8X8 + 7) / 8) * 8;
  localparam integer SLAB_K_4X4 = ((K_MAX_4X4 + 7) / 8) * 8;
  localparam integer HOST_SLAB_K = (SLAB_K_8X8 >= SLAB_K_4X4) ? SLAB_K_8X8 : SLAB_K_4X4;
  localparam integer HOST_SLAB_BYTES = HOST_SLAB_K * 8 * 8;
  localparam integer HEADER_BYTES = 16;
  localparam logic [31:0] HEADER_MAGIC = 32'h5953_4A43;

  // DDR map of this bench: a slab slot per device (256 B aligned), a result
  // slot per device (16 B aligned). The memory model holds 128 KiB.
  function automatic int slab_addr(input int dev);
    slab_addr = 32'h0001_0000 + dev * ((HOST_SLAB_BYTES + 255) / 256) * 256;
  endfunction
  function automatic int result_addr(input int dev);
    result_addr = 32'h0000_3000 + dev * 1024;
  endfunction
  function automatic int geom(input int dev);
    geom = (dev == 0) ? 8 : 4;
  endfunction
  function automatic int kmax_of(input int dev);
    kmax_of = (dev == 0) ? K_MAX_8X8 : K_MAX_4X4;
  endfunction

  // ---- fp32 <-> small integer ------------------------------------------
  function automatic logic [31:0] f32_of_int(input int v);
    int mag, msb;
    logic [31:0] r;
    begin
      if (v == 0) return 32'h0;
      mag = (v < 0) ? -v : v;
      msb = 0;
      for (int i = 30; i >= 0; i--) if (mag[i]) begin msb = i; break; end
      r = '0;
      r[31]    = (v < 0);
      r[30:23] = 8'(127 + msb);
      r[22:0]  = 23'((mag << (23 - msb)) & 32'h007F_FFFF);
      return r;
    end
  endfunction

  // -1 << 30 marks "not an integer value" (a sentinel no test produces).
  function automatic int int_of_f32(input logic [31:0] b);
    int e, m, v;
    begin
      if (b[30:0] == 0) return 0;
      e = int'(b[30:23]) - 127;
      if (e < 0 || e > 23) return (1 << 30);
      m = int'({9'b1, b[22:0]}) ;
      if ((m & ((1 << (23 - e)) - 1)) != 0) return (1 << 30);
      v = m >> (23 - e);
      return b[31] ? -v : v;
    end
  endfunction

  // ---- test vectors ----------------------------------------------------
  function automatic int a_val(input int dev, input int i, input int k);
    a_val = ((i * 7 + k * 3 + dev * 5) % 7) - 3;
  endfunction
  function automatic int b_val(input int dev, input int k, input int j);
    b_val = ((k * 5 + j * 2 + dev * 3) % 5) - 2;
  endfunction
  function automatic int c_ref(input int dev, input int K, input int i, input int j);
    int s;
    begin
      s = 0;
      for (int k = 0; k < K; k++) s += a_val(dev, i, k) * b_val(dev, k, j);
      return s;
    end
  endfunction

  // The slab of one P x K x P job in the wire format (zero past the tile).
  function automatic void build_slab(input int dev, input int K,
                                     ref byte unsigned slab[]);
    int P, win, koff, idx;
    logic [31:0] bits;
    begin
      P = geom(dev);
      slab = new[HOST_SLAB_BYTES];
      foreach (slab[i]) slab[i] = 8'h00;
      for (int k = 0; k < K; k++) begin
        win = k / 8; koff = k % 8;
        for (int r = 0; r < P; r++) begin
          idx  = (win << 7) | (r << 3) | koff;
          bits = f32_of_int(a_val(dev, r, k));
          slab[4*idx+0] = bits[7:0];   slab[4*idx+1] = bits[15:8];
          slab[4*idx+2] = bits[23:16]; slab[4*idx+3] = bits[31:24];
        end
        for (int c = 0; c < P; c++) begin
          idx  = (win << 7) | (1 << 6) | (koff << 3) | c;
          bits = f32_of_int(b_val(dev, k, c));
          slab[4*idx+0] = bits[7:0];   slab[4*idx+1] = bits[15:8];
          slab[4*idx+2] = bits[23:16]; slab[4*idx+3] = bits[31:24];
        end
      end
    end
  endfunction

  // ---- host primitives -------------------------------------------------
  task automatic host_push(input byte unsigned b);
    host_tx_q.push_back(b);
  endtask

  task automatic write32(input logic [7:0] addr, input logic [31:0] data);
    host_push(8'h01); host_push(addr);
    host_push(data[7:0]); host_push(data[15:8]); host_push(data[23:16]); host_push(data[31:24]);
  endtask

  task automatic mem_write(input int addr, ref byte unsigned payload[]);
    host_push(8'h02);
    host_push(8'(addr)); host_push(8'(addr >> 8)); host_push(8'(addr >> 16)); host_push(8'(addr >> 24));
    host_push(8'(payload.size())); host_push(8'(payload.size() >> 8));
    host_push(8'(payload.size() >> 16)); host_push(8'(payload.size() >> 24));
    foreach (payload[i]) host_push(payload[i]);
  endtask

  task automatic submit(input int job_id, input int dev, input int K,
                        input int a_base, input int c_base);
    int P;
    P = geom(dev);
    write32(8'h04, 32'(job_id));
    write32(8'h08, 32'(dev));
    write32(8'h0c, 32'(P));
    write32(8'h10, 32'(P));
    write32(8'h14, 32'(K));
    write32(8'h18, 32'd0);           // start_cycle
    write32(8'h1c, 32'd100);         // est_cycles (any positive value)
    write32(8'h20, 32'(a_base)); write32(8'h24, 32'd0);
    write32(8'h28, 32'd0);       write32(8'h2c, 32'd0);
    write32(8'h30, 32'(c_base)); write32(8'h34, 32'd0);
    write32(8'h00, 32'd1);           // CONTROL = submit, last
  endtask

  task automatic stage(input int dev, input int K);
    byte unsigned slab[];
    build_slab(dev, K, slab);
    mem_write(slab_addr(dev), slab);
  endtask

  // Wait until the host has n bytes, or give up after `cycles` DPTI clocks.
  task automatic wait_bytes(input int n, input int cycles, output bit ok);
    int t;
    ok = 1'b0;
    for (t = 0; t < cycles; t++) begin
      @(posedge dpti_clk);
      if (host_rx_q.size() >= n) begin ok = 1'b1; return; end
    end
  endtask

  task automatic pop_bytes(input int n, ref byte unsigned out[]);
    out = new[n];
    for (int i = 0; i < n; i++) out[i] = host_rx_q.pop_front();
  endtask

  // With +completion_header=1 every completion starts with the 16-byte record.
  bit use_header = 1'b0;

  typedef struct {
    bit          ok;
    logic [31:0] job_id;
    int          dev;
    int          status;
    int          payload_bytes;
    logic [31:0] seq;
  } completion_t;

  task automatic read_header(input int cycles, output completion_t c);
    bit ok;
    byte unsigned h[];
    logic [31:0] magic;
    c.ok = 1'b0;
    wait_bytes(HEADER_BYTES, cycles, ok);
    if (!ok) return;
    pop_bytes(HEADER_BYTES, h);
    magic = {h[3], h[2], h[1], h[0]};
    if (magic != HEADER_MAGIC) begin
      $display("  FAIL: completion header magic %08h (want %08h): the host is out of sync", magic, HEADER_MAGIC);
      return;
    end
    c.job_id        = {h[7], h[6], h[5], h[4]};
    c.dev           = int'(h[8]);
    c.status        = int'(h[9]);
    c.payload_bytes = int'({h[11], h[10]});
    c.seq           = {h[15], h[14], h[13], h[12]};
    c.ok            = 1'b1;
  endtask

  // Read one P x P tile and compare it with the reference of (dev, K).
  task automatic check_tile(input int dev, input int K, input int cycles, output bit ok,
                            input string what);
    int P, got, want, bad;
    byte unsigned t[];
    logic [31:0] bits;
    P = geom(dev);
    wait_bytes(P * P * 4, cycles, ok);
    if (!ok) begin
      $display("  FAIL: %s: only %0d of %0d result bytes arrived within %0d DPTI cycles",
               what, host_rx_q.size(), P * P * 4, cycles);
      return;
    end
    pop_bytes(P * P * 4, t);
    bad = 0;
    for (int i = 0; i < P; i++)
      for (int j = 0; j < P; j++) begin
        bits = {t[4*(i*P+j)+3], t[4*(i*P+j)+2], t[4*(i*P+j)+1], t[4*(i*P+j)+0]};
        got  = int_of_f32(bits);
        want = c_ref(dev, K, i, j);
        if (got != want) begin
          if (bad < 6)
            $display("  FAIL: %s: C[%0d][%0d] = %08h (%0d), want %0d", what, i, j, bits, got, want);
          bad++;
        end
      end
    ok = (bad == 0);
    if (ok) $display("  ok: %s: %0dx%0d tile of device %0d (k=%0d) is bit-exact", what, P, P, dev, K);
  endtask

  // The slab of device dev as it sits in DDR, compared with what was sent.
  task automatic check_slab_in_ddr(input int dev, input int K, output bit ok);
    byte unsigned slab[];
    int base, bad;
    logic [31:0] w;
    build_slab(dev, K, slab);
    base = slab_addr(dev);
    bad = 0;
    for (int i = 0; i < HOST_SLAB_BYTES / 4; i++) begin
      w = dut.u_mig_7series_0.mem[base / 4 + i];
      if (w != {slab[4*i+3], slab[4*i+2], slab[4*i+1], slab[4*i+0]}) begin
        if (bad < 4)
          $display("  FAIL: slab of device %0d word %0d in DDR = %08h, sent %02h%02h%02h%02h",
                   dev, i, w, slab[4*i+3], slab[4*i+2], slab[4*i+1], slab[4*i+0]);
        bad++;
      end
    end
    ok = (bad == 0);
    if (ok) $display("  ok: slab of device %0d is intact in DDR (%0d words)", dev, HOST_SLAB_BYTES / 4);
  endtask

  // Wait for the host queue to drain; report a stall when it does not.
  task automatic wait_consumed(input int cycles, output bit ok);
    int t;
    ok = 1'b0;
    for (t = 0; t < cycles; t++) begin
      @(posedge dpti_clk);
      if (host_tx_q.size() == 0) begin ok = 1'b1; return; end
    end
  endtask

  function automatic string hw_flags();
    hw_flags = $sformatf("any_err=%0b err_w_owner=%0b err_job_k=%0b mem_errors=%0d strobe_violations=%0d",
                         dut.any_err, dut.err_w_owner, dut.err_job_k,
                         dut.u_mig_7series_0.errors, dpc_violations);
  endfunction

  // ---- the tests -------------------------------------------------------
  // The DPTI-cycle budgets: a k=16 job is accepted, filled, computed and
  // written back within ~5000 array cycles; readback adds the host's byte
  // rate. 200 000 DPTI cycles (3.3 ms) is far beyond any of it.
  localparam int T_JOB = 200000;

  int errors = 0;

  task automatic t_basic();
    bit ok;
    completion_t c;
    for (int dev = 0; dev < 4; dev++) begin
      int K = kmax_of(dev);
      stage(dev, K);
      submit(100 + dev, dev, K, slab_addr(dev), result_addr(dev));
      if (use_header) begin
        read_header(T_JOB, c);
        if (!c.ok || c.job_id != 32'(100 + dev) || c.dev != dev || c.status != 0 ||
            c.payload_bytes != geom(dev) * geom(dev) * 4) begin
          $display("  FAIL: header for device %0d: ok=%0b job=%0d dev=%0d status=%0d bytes=%0d",
                   dev, c.ok, c.job_id, c.dev, c.status, c.payload_bytes);
          errors++;
        end
      end
      check_tile(dev, K, T_JOB, ok, $sformatf("basic device %0d", dev));
      if (!ok) errors++;
    end
  endtask

  // Four jobs in flight at once, one per device; the completions arrive in
  // the hardware's order (the readback slot grants the lowest-numbered
  // pending context, so it is not the submission order whenever a 4x4
  // finishes before the 8x8).
  task automatic t_multi();
    bit ok;
    completion_t c;
    bit seen[4];
    for (int dev = 0; dev < 4; dev++) begin
      stage(dev, kmax_of(dev));
      submit(200 + dev, dev, kmax_of(dev), slab_addr(dev), result_addr(dev));
    end
    if (!use_header) begin
      $display("  (no completion header: the bench can only read the tiles in submission order)");
      for (int dev = 0; dev < 4; dev++) begin
        check_tile(dev, kmax_of(dev), T_JOB, ok, $sformatf("multi, assumed device %0d", dev));
        if (!ok) errors++;
      end
      return;
    end
    for (int n = 0; n < 4; n++) begin
      read_header(T_JOB, c);
      if (!c.ok) begin errors++; return; end
      if (c.dev < 0 || c.dev > 3 || seen[c.dev] || c.job_id != 32'(200 + c.dev) || c.status != 0) begin
        $display("  FAIL: completion %0d: job=%0d dev=%0d status=%0d (seen=%0b)", n, c.job_id, c.dev, c.status, seen[c.dev]);
        errors++;
        return;
      end
      seen[c.dev] = 1'b1;
      check_tile(c.dev, kmax_of(c.dev), T_JOB, ok, $sformatf("multi, completion %0d is device %0d", n, c.dev));
      if (!ok) errors++;
    end
  endtask

  task automatic t_refused();
    bit ok;
    completion_t c;
    int K0 = kmax_of(0);
    // The refused one first: device 1, k one past its capacity.
    stage(1, kmax_of(1));
    submit(301, 1, kmax_of(1) + 1, slab_addr(1), result_addr(1));
    // Then a valid job on device 0.
    stage(0, K0);
    submit(300, 0, K0, slab_addr(0), result_addr(0));
    if (use_header) begin
      read_header(T_JOB, c);
      if (!c.ok || c.job_id != 32'd301 || c.dev != 1 || c.status != 1 || c.payload_bytes != 0) begin
        $display("  FAIL: expected a REFUSED completion for job 301 on device 1: ok=%0b job=%0d dev=%0d status=%0d bytes=%0d",
                 c.ok, c.job_id, c.dev, c.status, c.payload_bytes);
        errors++;
        return;
      end
      $display("  ok: job 301 refused (status 1, no payload)");
      read_header(T_JOB, c);
      if (!c.ok || c.job_id != 32'd300 || c.dev != 0 || c.status != 0) begin
        $display("  FAIL: expected job 300's completion after the refusal: ok=%0b job=%0d dev=%0d status=%0d",
                 c.ok, c.job_id, c.dev, c.status);
        errors++;
        return;
      end
    end
    check_tile(0, K0, T_JOB, ok, "job 300 after the refused job 301");
    if (!ok) begin
      errors++;
      wait_consumed(1000, ok);
      if (!ok)
        $display("  the link is wedged: %0d host bytes were never consumed (err_job_k=%0b)",
                 host_tx_q.size(), dut.err_job_k);
    end
  endtask

  // A MEM_WRITE for device 1 placed inside device 0's write-back burst.
  task automatic t_collision();
    bit ok;
    completion_t c;
    bit hit = 1'b0;
    int K0 = kmax_of(0);
    int wait_cycles;
    if (dut.u_mig_7series_0.w_stall == 0)
      $display("  (run with +w_stall=8: without it the write-back burst is too short to be hit on purpose)");
    stage(0, K0);
    submit(400, 0, K0, slab_addr(0), result_addr(0));
    // Wait for device 0's write-back AW to be accepted, then put the
    // MEM_WRITE on the link at once.
    wait_cycles = 0;
    while (!(dut.wb_awvalid && dut.wb_awready) && wait_cycles < T_JOB) begin
      @(posedge sys_clk); wait_cycles++;
    end
    if (wait_cycles >= T_JOB) begin
      $display("  FAIL: device 0 never started its write-back");
      errors++;
      return;
    end
    stage(1, kmax_of(1));
    // Was the hazard window actually hit? The host-staging engine asked for
    // the write channel (awvalid) while the memory still had a burst open.
    // Before the burst-boundary grant that request was accepted at once and
    // the two bursts interleaved; now it waits -- either way the window was
    // exercised.
    fork
      begin
        int t = 0;
        while (t < 20000) begin
          @(posedge sys_clk);
          if (dut.hs_awvalid && dut.u_mig_7series_0.w_open) hit = 1'b1;
          t++;
        end
      end
    join_none
    if (use_header) begin
      read_header(T_JOB, c);
      if (!c.ok || c.job_id != 32'd400) begin errors++; return; end
    end
    check_tile(0, K0, T_JOB, ok, "device 0 during the collision");
    if (!ok) errors++;
    wait_consumed(T_JOB, ok);
    if (!ok) begin $display("  FAIL: the MEM_WRITE for device 1 was never consumed"); errors++; end
    repeat (2000) @(posedge sys_clk);   // let the staging engine finish
    check_slab_in_ddr(1, kmax_of(1), ok);
    if (!ok) errors++;
    if (dut.u_mig_7series_0.errors != 0 || dut.err_w_owner) begin
      $display("  FAIL: the memory saw two masters at once (%s)", hw_flags());
      errors++;
    end
    $display("  hazard window %s", hit ? "HIT (the staging write arrived inside the write-back burst)" :
                                        "NOT HIT (this run did not exercise the hazard; inconclusive)");
    if (!hit) errors++;
    // And device 1 computes from the slab that was staged during the burst.
    submit(401, 1, kmax_of(1), slab_addr(1), result_addr(1));
    if (use_header) begin
      read_header(T_JOB, c);
      if (!c.ok || c.job_id != 32'd401) begin errors++; return; end
    end
    check_tile(1, kmax_of(1), T_JOB, ok, "device 1 from the slab staged during the burst");
    if (!ok) errors++;
  endtask

  // A result leaves while a slab is still coming in.
  task automatic t_direction();
    bit ok;
    completion_t c;
    int K0 = kmax_of(0);
    stage(0, K0);
    submit(500, 0, K0, slab_addr(0), result_addr(0));
    // 1033 bytes take ~17 us at the FT2232H's byte rate; device 0's result
    // (fill + fold + write-back, a few us) will want the bus meanwhile.
    stage(1, kmax_of(1));
    stage(2, kmax_of(2));
    stage(3, kmax_of(3));
    if (use_header) begin
      read_header(T_JOB, c);
      if (!c.ok || c.job_id != 32'd500) begin errors++; return; end
    end
    check_tile(0, K0, T_JOB, ok, "device 0 while slabs stream in");
    if (!ok) errors++;
    wait_consumed(T_JOB, ok);
    if (!ok) begin $display("  FAIL: the slabs were never consumed"); errors++; end
    repeat (2000) @(posedge sys_clk);
    for (int dev = 1; dev < 4; dev++) begin
      check_slab_in_ddr(dev, kmax_of(dev), ok);
      if (!ok) errors++;
    end
    if (dpc_violations != 0) begin
      $display("  FAIL: %0d DPTI strobe-rule violation(s) while a result left during a MEM_WRITE", dpc_violations);
      errors++;
    end
    for (int dev = 1; dev < 4; dev++) begin
      submit(500 + dev, dev, kmax_of(dev), slab_addr(dev), result_addr(dev));
      if (use_header) begin
        read_header(T_JOB, c);
        if (!c.ok || c.job_id != 32'(500 + dev)) begin errors++; return; end
      end
      check_tile(dev, kmax_of(dev), T_JOB, ok, $sformatf("device %0d from a slab staged under traffic", dev));
      if (!ok) errors++;
    end
  endtask

  // More descriptors than the devices can hold, from a host that writes
  // them all before it reads anything: six jobs to every device, 24
  // descriptors in a row. The 5th descriptor (device 0's second) stops the
  // command stream until device 0's first result has been read back into
  // the FT2232H; results keep coming as long as the FT2232H's 1 KiB transmit
  // buffer has room, and 24 completions (6 x 272 + 18 x 80 = 3072 B) do not
  // fit. Once TXE# is high the readback stalls, the busy device never frees,
  // the descriptor behind it never goes in, and the host -- still writing --
  // is stuck: the deadlock of docs/ASYNC.md section 5. Then the host reads,
  // and everything drains.
  //
  // Documented behaviour, not a verdict on the RTL: the host's credit rule
  // (never a second descriptor to a busy device, read before writing more)
  // is what keeps this from happening on the board.
  task automatic t_hol();
    bit ok;
    int left_over, unread_at_stall;
    for (int round = 0; round < 6; round++)
      for (int dev = 0; dev < 4; dev++) begin
        stage(dev, kmax_of(dev));
        submit(600 + 10 * round + dev, dev, kmax_of(dev), slab_addr(dev), result_addr(dev));
      end
    wait_consumed(T_JOB, ok);
    left_over = host_tx_q.size();
    unread_at_stall = host_rx_q.size();
    if (ok) begin
      $display("  all bytes consumed without the host reading (%s)", hw_flags());
      $display("RESULT hol DRAINED unconsumed=0 rx_bytes=%0d txe_high=%0b", host_rx_q.size(), ft_txe_n);
      $finish;
      return;
    end
    $display("  STALL: %0d host bytes unconsumed after %0d DPTI cycles; %0d result bytes unread, TXE#=%0b (%s)",
             left_over, T_JOB, unread_at_stall, ft_txe_n, hw_flags());
    // Now the host drains the IN direction: every stalled job completes.
    while (host_rx_q.size() > 0) void'(host_rx_q.pop_front());
    for (int t = 0; t < T_JOB; t++) begin
      @(posedge dpti_clk);
      while (host_rx_q.size() > 0) void'(host_rx_q.pop_front());
      if (host_tx_q.size() == 0) break;
    end
    repeat (20000) @(posedge sys_clk);
    while (host_rx_q.size() > 0) void'(host_rx_q.pop_front());
    $display("RESULT hol STALL unconsumed_at_stall=%0d unread_at_stall=%0d after_reading_unconsumed=%0d wr_strobes=%0d (24 completions = %0d B)",
             left_over, unread_at_stall, host_tx_q.size(), wr_strobes, 6 * 272 + 18 * 80);
    $finish;
    return;
  endtask

  // ---- main ------------------------------------------------------------
  string test;
  int    v;

  initial begin
    if (!$value$plusargs("test=%s", test)) test = "basic";
    if ($value$plusargs("completion_header=%d", v)) use_header = (v != 0);
    if ($value$plusargs("dpti_check=%d", v)) dpc_enabled = (v != 0);

    repeat (20) @(posedge sys_clk);
    cpu_resetn = 1'b1;
    repeat (2000) @(posedge sys_clk);
    if (led[0] !== 1'b1)
      $fatal(1, "init_calib_complete (led[0]) did not rise");
    $display("== tb_dpti_direct: test=%s K_MAX_8X8=%0d K_MAX_4X4=%0d HOST_SLAB_BYTES=%0d completion_header=%0d ==",
             test, K_MAX_8X8, K_MAX_4X4, HOST_SLAB_BYTES, use_header);

    case (test)
      "basic":     t_basic();
      "multi":     t_multi();
      "refused":   t_refused();
      "collision": t_collision();
      "direction": t_direction();
      "hol":       t_hol();
      default: begin
        $display("unknown +test=%s", test);
        $finish;
      end
    endcase

    repeat (200) @(posedge sys_clk);
    if (dpc_violations != 0 && test != "direction") begin
      $display("  FAIL: %0d DPTI strobe-rule violation(s)", dpc_violations);
      errors++;
    end
    $display("RESULT %s %s errors=%0d %s rd_strobes=%0d wr_strobes=%0d",
             test, (errors == 0) ? "PASS" : "FAIL", errors, hw_flags(), rd_strobes, wr_strobes);
    $finish;
  end

  // Nothing may run forever.
  initial begin
    #50ms;
    $display("RESULT %s FAIL timeout", test);
    $fatal(1, "timeout");
  end

endmodule

`default_nettype wire
