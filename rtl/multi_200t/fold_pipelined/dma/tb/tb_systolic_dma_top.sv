`default_nettype none
`timescale 1ns / 1ps

/*
 * tb_systolic_dma_top -- the integrated design, end to end, in simulation.
 *
 * Every stage below this has its own bench and its own mutants.  What none of
 * them can see is the seam: two write masters on one AXI channel, a phase
 * machine that hands the channel over between them, and a result reader started
 * from that machine and reading the C register the array wrote.  This bench
 * runs the real systolic_dma_top -- unmodified, with only the Xilinx IP
 * replaced -- from power-on to P_DONE, and asks four questions:
 *
 *   1. Does the operand path still work?  chk_wr is accumulated from the
 *      operand write stream and does not depend on the arithmetic, so the
 *      board's constant 0x3f880780 must come back even though fp_mul and
 *      fp_add here are integer pipelines.  If it does not, the write-back work
 *      broke the read path.
 *   2. Does the result tile reach memory intact?  Every word of the image at
 *      WB_BASE_ADDR is compared against the C register itself, in the order
 *      systolic_tx_source defines.
 *   3. Did the two write masters ever overlap?  err_w_owner is the design's own
 *      answer; the memory model's interleaving check is an independent one,
 *      taken from the far side of the bus.
 *   4. Does it finish at all?  A write-back whose last write response never
 *      returns hangs here rather than on the board.
 *
 * chk_c is deliberately NOT checked.  fp_model.sv is not floating point, so C
 * here is not the board's C.  What C contains is settled by the board and by
 * tools/seed_ref.py; what this bench settles is that whatever C contains
 * arrives in memory unchanged.
 *
 *   verilator --binary -Wno-fatal --timing --public-flat-rw \
 *       --top-module tb_systolic_dma_top -o tbrun \
 *       tb/tb_systolic_dma_top.sv tb/xil_stubs.sv tb/mig_axi_model.sv \
 *       ../../tb/fp_model.sv ../systolic_dma_top.sv ... && ./obj_dir/tbrun
 *
 * Icarus cannot run this one: the feeder and the array talk through unpacked
 * array ports, and its always_comb sensitivity inference leaves them at X.
 * That is why the array's own benches in tb/ are Verilator commands.
 */

module tb_systolic_dma_top;

  localparam integer N        = 8;
  localparam integer K_MAX    = 16;
  localparam integer WB_BASE  = 4096;
  localparam integer RX_WORDS = (K_MAX * 8 * N) / 4;
  localparam integer MAX_CYC  = 200_000;
  localparam logic [31:0] EXPECT_WR_CHK = 32'h3F88_0780;

  logic clk  = 1'b0;
  logic rstn = 1'b0;
  always #5 clk = ~clk;                     // 100 MHz, the ui_clk frequency

  wire [7:0]  led;
  wire [14:0] ddr3_addr;  wire [2:0] ddr3_ba;
  wire        ddr3_cas_n, ddr3_ras_n, ddr3_reset_n, ddr3_we_n;
  wire [0:0]  ddr3_ck_n, ddr3_ck_p, ddr3_cke, ddr3_odt;
  wire [1:0]  ddr3_dm;
  wire [15:0] ddr3_dq;    wire [1:0] ddr3_dqs_n, ddr3_dqs_p;

  systolic_dma_top #(
    .N (N), .K_MAX (K_MAX), .K_DIM (K_MAX),
    .BASE_ADDR (0), .WB_BASE_ADDR (WB_BASE)
  ) dut (
    .sys_clk_pin (clk), .cpu_resetn (rstn), .led (led),
    .ddr3_addr (ddr3_addr), .ddr3_ba (ddr3_ba), .ddr3_cas_n (ddr3_cas_n),
    .ddr3_ck_n (ddr3_ck_n), .ddr3_ck_p (ddr3_ck_p), .ddr3_cke (ddr3_cke),
    .ddr3_ras_n (ddr3_ras_n), .ddr3_reset_n (ddr3_reset_n),
    .ddr3_we_n (ddr3_we_n), .ddr3_dq (ddr3_dq),
    .ddr3_dqs_n (ddr3_dqs_n), .ddr3_dqs_p (ddr3_dqs_p),
    .ddr3_dm (ddr3_dm), .ddr3_odt (ddr3_odt)
  );

  // A stall in this design is a phase that never advances, so the phase is the
  // trace.  One line per transition turns a timeout from "it hung" into "it
  // hung in P_READ with 191 of 256 words written".
  logic [3:0] phase_d;
  always_ff @(posedge clk) begin
    phase_d <= dut.phase;
    if (dut.phase !== phase_d)
      $display("  phase %0d -> %0d   words=%0d", phase_d, dut.phase,
               dut.words_written);
  end

  integer errors = 0;
  integer waited = 0;
  integer i, r, c;
  logic [31:0] got, want;

  initial begin
    $display("== tb_systolic_dma_top: seed -> DDR -> fold -> C -> DDR ==");
    repeat (20) @(posedge clk);
    rstn = 1'b1;

    while (dut.wb_done_sticky !== 1'b1 && waited < MAX_CYC) begin
      @(posedge clk);
      waited = waited + 1;
    end

    if (dut.wb_done_sticky !== 1'b1) begin
      $display("  FAIL: no write-back completion after %0d cycles", MAX_CYC);
      $display("        phase=%0d seed=%0b read=%0b fold=%0b words=%0d",
               dut.phase, dut.seed_done_sticky, dut.read_done_sticky,
               dut.fold_done_sticky, dut.words_written);
      errors = errors + 1;
    end

    repeat (20) @(posedge clk);

    // ---- 1. the operand path is untouched ---------------------------------
    if (dut.words_written !== 32'(RX_WORDS)) begin
      $display("  FAIL: words_written = %0d, want %0d",
               dut.words_written, RX_WORDS);
      errors = errors + 1;
    end
    if (dut.chk_wr !== EXPECT_WR_CHK) begin
      $display("  FAIL: chk_wr = %h, want %h (the board's constant)",
               dut.chk_wr, EXPECT_WR_CHK);
      errors = errors + 1;
    end else
      $display("  chk_wr = %h matches the board's constant", dut.chk_wr);

    // ---- 2. the result image ----------------------------------------------
    // Word w of the image is C[w >> log2 N][w & (N-1)]: row major, exactly the
    // order systolic_tx_source walks and tb_dma_result_reader pins down.
    for (i = 0; i < N*N; i = i + 1) begin
      r    = i / N;
      c    = i % N;
      want = dut.C[r][c];
      got  = dut.u_mig_7series_0.mem[WB_BASE/4 + i];
      if (got !== want) begin
        if (errors < 8)
          $display("  FAIL: result word %0d (row %0d col %0d): memory %h, C %h",
                   i, r, c, got, want);
        errors = errors + 1;
      end
    end

    // ---- 3. the two write masters never overlapped ------------------------
    if (dut.err_w_owner !== 1'b0) begin
      $display("  FAIL: err_w_owner latched -- a master drove the write channel it did not own");
      errors = errors + 1;
    end
    if (dut.u_mig_7series_0.errors != 0) begin
      $display("  FAIL: the memory model reported %0d protocol error(s)",
               dut.u_mig_7series_0.errors);
      errors = errors + dut.u_mig_7series_0.errors;
    end
    if (dut.any_err !== 1'b0) begin
      $display("  FAIL: any_err latched (seed %0b%0b eng %0b%0b wr %0b wb %0b%0b own %0b)",
               dut.seed_err_align, dut.seed_err_resp,
               dut.eng_err_align, dut.eng_err_resp, dut.wr_err_range,
               dut.wb_err_align, dut.wb_err_resp, dut.err_w_owner);
      errors = errors + 1;
    end

    if (errors == 0)
      $display("  all %0d result words in memory match C, from one owner at a time",
               N*N);

    // ---- 4. reported, not asserted ----------------------------------------
    // Printed, not checked.  The PE is deliberately agnostic to how many
    // cycles its multiplier and adder take -- fp_model's own header makes that
    // the point of running it at LAT = 3/5 and 9/12 -- so the cycle count in
    // simulation tracks the model's latency, not the IP's, and a difference of
    // a few cycles from the board's 125 is the model being a model.  Asserting
    // it here would be claiming these stubs are the array.  The number that
    // means something is the one the board reports, and dma_top_build.tcl
    // checks that one against EXPECT_CYC.
    $display("  cyc_latched = %0d  (the board reports 125 at k_dim = %0d)",
             dut.cyc_latched, K_MAX);

    $display("== %s: %0d error(s) ==", (errors == 0) ? "PASS" : "FAIL", errors);
    if (errors != 0) $fatal(1);
    $finish;
  end

endmodule

`default_nettype wire
