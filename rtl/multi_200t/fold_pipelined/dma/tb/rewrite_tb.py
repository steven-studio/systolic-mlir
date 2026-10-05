from pathlib import Path
from datetime import datetime
import shutil

TB = Path("tb_systolic_dma_top.sv")

if not TB.exists():
    raise SystemExit(f"ERROR: {TB} does not exist")

# ----------------------------------------------------------------------
# Backup the old testbench before replacing it.
# ----------------------------------------------------------------------

stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
backup = TB.with_name(f"tb_systolic_dma_top.sv.bak_before_clean_tb_{stamp}")

shutil.copy2(TB, backup)
print(f"BACKUP: {backup}")

# ----------------------------------------------------------------------
# Clean top-level testbench.
#
# Design philosophy:
#
#   1. The TB owns the scheduler input.
#   2. The TB owns the reference address map.
#   3. The TB owns the operand preload.
#   4. DUT internal scheduler signals are OBSERVATION ONLY.
#   5. Job completion is defined by scheduler_c_done, not by an
#      arbitrary AXI B handshake.
#   6. Every compiler job has an explicit reference record.
#   7. No magic checksum is used as the primary correctness criterion.
#
# This is intentionally a much smaller integration testbench.
# ----------------------------------------------------------------------

NEW_TB = r'''`default_nettype none
`timescale 1ns / 1ps

/*
 * Clean end-to-end integration testbench for systolic_dma_top.
 *
 * This testbench deliberately does NOT use the DUT's internal scheduler
 * output to generate its own scheduler input.
 *
 * The external control flow is:
 *
 *        TB scheduler
 *             |
 *             | job_valid + descriptor
 *             v
 *          DUT job ingress
 *             |
 *             | job_ready
 *             v
 *          DUT execution
 *             |
 *             | scheduler_fold_start
 *             v
 *          accelerator
 *             |
 *             | scheduler_c_done
 *             v
 *        TB scheduler
 *
 * The TB therefore tests the scheduler/accelerator seam instead of
 * accidentally feeding the DUT's own scheduler output back into itself.
 *
 * The TB also contains exactly one reference implementation for:
 *
 *   - compiler job C placement
 *   - operand payload index
 *   - expected 4x4 C tile
 *
 * No result address is hard-coded separately in debug code.
 */

module tb_systolic_dma_top #(
  parameter bit     USE_V2 = 1'b0,
  parameter integer K_MAX  = 16,
  parameter integer JOB_K  = K_MAX,
  parameter bit     USE_EXTERNAL_SCHEDULER = 1'b1
);

  // --------------------------------------------------------------------
  // Hardware geometry
  // --------------------------------------------------------------------

  localparam integer N = 8;

  localparam integer RX_BYTES = K_MAX * 8 * N;
  localparam integer RX_WORDS = RX_BYTES / 4;

  localparam integer MAX_CYCLES = 2_000_000;

  // --------------------------------------------------------------------
  // Compiler test stream
  //
  // Keep this deliberately small.
  //
  // Every job is a 4x4xK compiler tile.
  // Each result is therefore:
  //
  //     4 * 4 * sizeof(float)
  //       = 64 bytes
  //
  // This is a compiler-stream integration test, not a fake 32x32 GEMM.
  // --------------------------------------------------------------------

  localparam integer NUM_JOBS = 10;

  localparam [63:0] A_BASE_START = 64'h0000_0000_0001_0000;
  localparam [63:0] B_BASE       = 64'h0000_0000_0000_2000;
  localparam [63:0] C_BASE_START = 64'h0000_0000_0000_3000;

  localparam integer A_SLAB_BYTES = 32'h800;
  localparam integer C_TILE_BYTES = 4 * 4 * 4;

  // --------------------------------------------------------------------
  // Clock / reset
  // --------------------------------------------------------------------

  logic clk  = 1'b0;
  logic rstn = 1'b0;

  always #5 clk = ~clk;

  // --------------------------------------------------------------------
  // DDR3 pins
  // --------------------------------------------------------------------

  wire [7:0]  led;

  wire [14:0] ddr3_addr;
  wire [2:0]  ddr3_ba;

  wire ddr3_cas_n;
  wire ddr3_ras_n;
  wire ddr3_reset_n;
  wire ddr3_we_n;

  wire [0:0] ddr3_ck_n;
  wire [0:0] ddr3_ck_p;
  wire [0:0] ddr3_cke;
  wire [0:0] ddr3_odt;

  wire [1:0] ddr3_dm;

  wire [15:0] ddr3_dq;
  wire [1:0]  ddr3_dqs_n;
  wire [1:0]  ddr3_dqs_p;

  // --------------------------------------------------------------------
  // Scheduler interface
  // --------------------------------------------------------------------

  logic scheduler_fold_start;
  wire  scheduler_c_done;

  logic        job_valid;
  wire         job_ready;

  logic [31:0] job_id;
  logic [31:0] job_device_id;
  logic [31:0] job_m;
  logic [31:0] job_n;
  logic [31:0] job_k;
  logic [31:0] job_start_cycle;
  logic [31:0] job_est_cycles;

  logic [63:0] job_a_base;
  logic [63:0] job_b_base;
  logic [63:0] job_c_base;

  // --------------------------------------------------------------------
  // Reference model
  // --------------------------------------------------------------------

  function automatic integer job_c_offset_bytes(input integer idx);
    begin
      job_c_offset_bytes = idx * C_TILE_BYTES;
    end
  endfunction

  function automatic [63:0] ref_job_c_base(input integer idx);
    begin
      ref_job_c_base =
          C_BASE_START +
          64'(job_c_offset_bytes(idx));
    end
  endfunction

  function automatic [63:0] ref_job_a_base(input integer idx);
    begin
      ref_job_a_base =
          A_BASE_START +
          64'(idx * A_SLAB_BYTES);
    end
  endfunction

  /*
   * Payload encoding reference.
   *
   * The intended compiler payload layout is:
   *
   *   bit 7     : matrix
   *   bit 6     : K window
   *   bits 5:3  : lane
   *   bits 2:0  : K offset
   *
   * Therefore:
   *
   *   A = matrix 0
   *   B = matrix 1
   *
   * IMPORTANT:
   * This function is the SINGLE source of truth for the TB.
   * Do not duplicate this encoding elsewhere.
   */
  function automatic integer ref_payload_index(
      input integer matrix,
      input integer win,
      input integer lane,
      input integer koff
  );
    begin
      ref_payload_index =
          (matrix << 7) |
          (win    << 6) |
          (lane   << 3) |
          koff;
    end
  endfunction

  /*
   * Expected 4x4 result.
   *
   * A = I4
   *
   * B column pattern:
   *
   *   [1, 2, 4, 8]
   *
   * Therefore:
   *
   *   C =
   *
   *   1 2 4 8
   *   1 2 4 8
   *   1 2 4 8
   *   1 2 4 8
   */
  function automatic [31:0] ref_expected_c(
      input integer row,
      input integer col
  );
    begin
      case (col)
        0: ref_expected_c = 32'h3f80_0000;
        1: ref_expected_c = 32'h4000_0000;
        2: ref_expected_c = 32'h4080_0000;
        3: ref_expected_c = 32'h4100_0000;
        default: ref_expected_c = 32'hxxxx_xxxx;
      endcase
    end
  endfunction

  // --------------------------------------------------------------------
  // Job producer state
  // --------------------------------------------------------------------

  integer current_job;

  logic job_inflight;

  integer jobs_accepted;
  integer jobs_started;
  integer jobs_completed;

  integer errors;

  logic [31:0] accepted_id;
  logic [63:0] accepted_a_base;
  logic [63:0] accepted_c_base;

  // --------------------------------------------------------------------
  // Descriptor generation
  //
  // The TB presents exactly ONE descriptor at a time.
  // No next descriptor is exposed until scheduler_c_done.
  // --------------------------------------------------------------------

  always_comb begin
    job_valid = 1'b0;

    job_id          = 32'd0;
    job_device_id   = 32'd1;

    job_m           = 32'd4;
    job_n           = 32'd4;
    job_k           = 32'(JOB_K);

    job_start_cycle = 32'd0;
    job_est_cycles  = 32'd16;

    job_a_base      = A_BASE_START;
    job_b_base      = B_BASE;
    job_c_base      = C_BASE_START;

    if (USE_EXTERNAL_SCHEDULER &&
        dut.ui_rst_n &&
        !job_inflight &&
        current_job < NUM_JOBS) begin

      job_valid = 1'b1;

      job_id = 32'(100 + current_job);

      job_device_id = 32'd1;

      job_m = 32'd4;
      job_n = 32'd4;
      job_k = 32'(JOB_K);

      /*
       * These values are metadata only for the current baseline.
       * The TB does not use them as the definition of completion.
       */
      job_start_cycle = 32'(current_job * 16);
      job_est_cycles  = 32'd16;

      job_a_base = ref_job_a_base(current_job);
      job_b_base = B_BASE;
      job_c_base = ref_job_c_base(current_job);
    end
  end

  // --------------------------------------------------------------------
  // Job bookkeeping
  // --------------------------------------------------------------------

  always_ff @(posedge clk or negedge rstn) begin
    if (!rstn) begin
      current_job  <= 0;
      job_inflight <= 1'b0;

      jobs_accepted  <= 0;
      jobs_completed <= 0;

      accepted_id     <= 32'd0;
      accepted_a_base <= 64'd0;
      accepted_c_base <= 64'd0;
    end
    else if (USE_EXTERNAL_SCHEDULER) begin

      /*
       * Descriptor acceptance.
       */
      if (job_valid && job_ready) begin
        job_inflight <= 1'b1;

        jobs_accepted <= jobs_accepted + 1;

        accepted_id     <= job_id;
        accepted_a_base <= job_a_base;
        accepted_c_base <= job_c_base;

        $display(
          "JOB_ACCEPT job=%0d id=%0d A=0x%0h C=0x%0h",
          current_job,
          job_id,
          job_a_base,
          job_c_base
        );
      end

      /*
       * Completion comes from the scheduler interface itself.
       *
       * This is deliberately NOT:
       *
       *   wb_bvalid && wb_bready
       *
       * because an AXI write response is a bus transaction boundary,
       * not automatically a compiler-job boundary.
       */
      if (job_inflight && scheduler_c_done) begin

        jobs_completed <= jobs_completed + 1;

        job_inflight <= 1'b0;

        $display(
          "JOB_DONE job=%0d id=%0d C=0x%0h",
          current_job,
          accepted_id,
          accepted_c_base
        );

        current_job <= current_job + 1;
      end
    end
  end

  // --------------------------------------------------------------------
  // External scheduler -> accelerator start
  //
  // The TB really drives this signal.
  //
  // We wait until DUT reaches P_FOLD and then emit ONE pulse.
  //
  // There is intentionally NO connection to:
  //
  //   dut.scheduler_fold_start_selected
  //
  // --------------------------------------------------------------------

  logic start_armed;

  always_ff @(posedge dut.ui_clk or negedge rstn) begin
    if (!rstn) begin
      scheduler_fold_start <= 1'b0;
      start_armed          <= 1'b0;
      jobs_started         <= 0;
    end
    else begin

      scheduler_fold_start <= 1'b0;

      if (!USE_EXTERNAL_SCHEDULER) begin
        start_armed <= 1'b0;
      end
      else if (!job_inflight) begin
        start_armed <= 1'b0;
      end
      else if (dut.phase != 4'd4) begin
        /*
         * P_FOLD = 4.
         *
         * Re-arm whenever we leave P_FOLD.
         */
        start_armed <= 1'b0;
      end
      else if (!start_armed) begin
        /*
         * Exactly one external start pulse for this job.
         */
        scheduler_fold_start <= 1'b1;
        start_armed          <= 1'b1;
        jobs_started         <= jobs_started + 1;

        $display(
          "JOB_START job=%0d id=%0d phase=%0d",
          current_job,
          accepted_id,
          dut.phase
        );
      end
    end
  end

  // --------------------------------------------------------------------
  // MIG operand preload
  //
  // The TB owns the reference image.
  //
  // Each compiler job receives one 0x800-byte physical slab.
  // --------------------------------------------------------------------

  initial begin : PRELOAD_REFERENCE

    integer j;
    integer row;
    integer col;
    integer k;
    integer win;
    integer koff;
    integer idx;
    integer pbase_word;

    if (USE_EXTERNAL_SCHEDULER) begin

      /*
       * Let the MIG model initialize its backing memory first.
       */
      #1;

      for (j = 0; j < NUM_JOBS; j = j + 1) begin

        pbase_word =
            ref_job_a_base(j) / 4;

        /*
         * Clear complete slab.
         */
        for (idx = 0; idx < (A_SLAB_BYTES / 4); idx = idx + 1) begin
          dut.u_mig_7series_0.mem[pbase_word + idx] =
              32'h0000_0000;
        end

        /*
         * Populate A and B using ONLY ref_payload_index().
         *
         * A = I4
         */
        for (k = 0; k < 32; k = k + 1) begin

          win  = k / 8;
          koff = k % 8;

          for (row = 0; row < 4; row = row + 1) begin

            idx = ref_payload_index(
                0,       // matrix A
                win,
                row,
                koff
            );

            if (row == k) begin
              dut.u_mig_7series_0.mem[
                  pbase_word + idx
              ] = 32'h3f80_0000;
            end
          end

          /*
           * B.
           */
          for (col = 0; col < 4; col = col + 1) begin

            idx = ref_payload_index(
                1,       // matrix B
                win,
                0,
                koff
            );

            /*
             * B payload is duplicated across the relevant lane structure.
             * Keep the actual physical placement explicit rather than
             * hiding it behind a second address formula.
             */
            idx = ref_payload_index(
                1,
                win,
                koff,
                col
            );

            case (col)
              0: dut.u_mig_7series_0.mem[
                   pbase_word + idx
                 ] = 32'h3f80_0000;

              1: dut.u_mig_7series_0.mem[
                   pbase_word + idx
                 ] = 32'h4000_0000;

              2: dut.u_mig_7series_0.mem[
                   pbase_word + idx
                 ] = 32'h4080_0000;

              3: dut.u_mig_7series_0.mem[
                   pbase_word + idx
                 ] = 32'h4100_0000;
            endcase
          end
        end
      end

      $display(
        "REFERENCE: preloaded %0d compiler operand slabs",
        NUM_JOBS
      );

      for (j = 0; j < NUM_JOBS; j = j + 1) begin
        $display(
          "REFERENCE JOB%0d: A=0x%0h C=0x%0h",
          j,
          ref_job_a_base(j),
          ref_job_c_base(j)
        );
      end
    end
  end

  // --------------------------------------------------------------------
  // DUT
  // --------------------------------------------------------------------

  systolic_dma_top #(
    .N (N),
    .K_MAX (K_MAX),
    .K_DIM (K_MAX),
    .BASE_ADDR (0),
    .WB_GAP_BYTES (4096),
    .USE_V2 (USE_V2),
    .USE_EXTERNAL_SCHEDULER (USE_EXTERNAL_SCHEDULER)
  ) dut (
    .sys_clk_pin (clk),
    .cpu_resetn (rstn),
    .led (led),

    .ddr3_addr (ddr3_addr),
    .ddr3_ba (ddr3_ba),
    .ddr3_cas_n (ddr3_cas_n),
    .ddr3_ck_n (ddr3_ck_n),
    .ddr3_ck_p (ddr3_ck_p),
    .ddr3_cke (ddr3_cke),
    .ddr3_ras_n (ddr3_ras_n),
    .ddr3_reset_n (ddr3_reset_n),
    .ddr3_we_n (ddr3_we_n),
    .ddr3_dq (ddr3_dq),
    .ddr3_dqs_n (ddr3_dqs_n),
    .ddr3_dqs_p (ddr3_dqs_p),
    .ddr3_dm (ddr3_dm),
    .ddr3_odt (ddr3_odt),

    .scheduler_fold_start (scheduler_fold_start),
    .scheduler_c_done     (scheduler_c_done),

    .job_valid       (job_valid),
    .job_ready       (job_ready),
    .job_id          (job_id),
    .job_device_id   (job_device_id),
    .job_m            (job_m),
    .job_n            (job_n),
    .job_k            (JOB_K),
    .job_start_cycle (job_start_cycle),
    .job_est_cycles  (job_est_cycles),
    .job_a_base      (job_a_base),
    .job_b_base      (job_b_base),
    .job_c_base      (job_c_base)
  );

  // --------------------------------------------------------------------
  // Independent transaction monitor
  // --------------------------------------------------------------------

  integer ar_count;
  integer r_count;

  integer wb_aw_count;
  integer wb_w_count;
  integer wb_b_count;

  always @(posedge dut.ui_clk) begin

    if (dut.ui_rst_n) begin

      if (dut.arvalid && dut.arready) begin
        ar_count = ar_count + 1;

        if (ar_count <= 16) begin
          $display(
            "BUS AR[%0d] addr=0x%0h len=%0d",
            ar_count - 1,
            dut.araddr,
            dut.arlen + 1
          );
        end
      end

      if (dut.rvalid && dut.rready) begin
        r_count = r_count + 1;
      end

      if (dut.wb_awvalid && dut.wb_awready) begin
        wb_aw_count = wb_aw_count + 1;

        $display(
          "BUS WB_AW[%0d] addr=0x%0h len=%0d",
          wb_aw_count - 1,
          dut.wb_awaddr,
          dut.wb_awlen + 1
        );
      end

      if (dut.wb_wvalid && dut.wb_wready) begin
        wb_w_count = wb_w_count + 1;
      end

      if (dut.wb_bvalid && dut.wb_bready) begin
        wb_b_count = wb_b_count + 1;
      end
    end
  end

  // --------------------------------------------------------------------
  // Reference result checker
  // --------------------------------------------------------------------

  task automatic check_job_result(input integer j);
    integer i;
    integer row;
    integer col;

    integer word_addr;

    logic [31:0] got;
    logic [31:0] want;

    begin

      $display(
        "CHECK JOB%0d C_BASE=0x%0h",
        j,
        ref_job_c_base(j)
      );

      for (i = 0; i < 16; i = i + 1) begin

        row = i / 4;
        col = i % 4;

        word_addr =
            (ref_job_c_base(j) / 4) + i;

        got =
            dut.u_mig_7series_0.mem[word_addr];

        want =
            ref_expected_c(row, col);

        if (got !== want) begin

          $display(
            "ERROR RESULT job=%0d word=%0d row=%0d col=%0d addr=0x%0h got=%h want=%h",
            j,
            i,
            row,
            col,
            ref_job_c_base(j) + 64'(i * 4),
            got,
            want
          );

          errors = errors + 1;
        end
      end
    end
  endtask

  // --------------------------------------------------------------------
  // Final checker
  // --------------------------------------------------------------------

  initial begin : MAIN

    integer waited;
    integer j;

    errors = 0;

    current_job   = 0;
    job_inflight  = 1'b0;

    jobs_accepted  = 0;
    jobs_started   = 0;
    jobs_completed = 0;

    ar_count    = 0;
    r_count     = 0;

    wb_aw_count = 0;
    wb_w_count  = 0;
    wb_b_count  = 0;

    $display("");
    $display("============================================================");
    $display(" CLEAN SYSTOLIC DMA TOP TESTBENCH");
    $display("============================================================");
    $display(
      "USE_V2=%0d K_MAX=%0d JOB_K=%0d NUM_JOBS=%0d",
      USE_V2,
      K_MAX,
      JOB_K,
      NUM_JOBS
    );
    $display("");

    if (!USE_EXTERNAL_SCHEDULER) begin
      $display(
        "ERROR: this clean compiler-stream TB requires USE_EXTERNAL_SCHEDULER=1"
      );
      $fatal(1);
    end

    if (JOB_K <= 0 || JOB_K > K_MAX) begin
      $display(
        "ERROR: JOB_K=%0d outside K_MAX=%0d",
        JOB_K,
        K_MAX
      );
      $fatal(1);
    end

    /*
     * Reset.
     */
    repeat (20) @(posedge clk);

    rstn = 1'b1;

    $display("RESET RELEASED");

    /*
     * Wait until every compiler job completes.
     */
    waited = 0;

    while (jobs_completed < NUM_JOBS &&
           waited < MAX_CYCLES) begin

      @(posedge clk);

      waited = waited + 1;
    end

    $display("");
    $display("JOB SUMMARY");
    $display(
      "accepted=%0d started=%0d completed=%0d",
      jobs_accepted,
      jobs_started,
      jobs_completed
    );

    /*
     * Job protocol checks.
     */
    if (jobs_accepted != NUM_JOBS) begin
      $display(
        "ERROR: accepted=%0d want=%0d",
        jobs_accepted,
        NUM_JOBS
      );
      errors = errors + 1;
    end

    if (jobs_started != NUM_JOBS) begin
      $display(
        "ERROR: started=%0d want=%0d",
        jobs_started,
        NUM_JOBS
      );
      errors = errors + 1;
    end

    if (jobs_completed != NUM_JOBS) begin
      $display(
        "ERROR: completed=%0d want=%0d",
        jobs_completed,
        NUM_JOBS
      );
      errors = errors + 1;
    end

    if (dut.phase !== 4'd7) begin
      $display(
        "ERROR: DUT did not reach P_DONE. phase=%0d",
        dut.phase
      );
      errors = errors + 1;
    end

    /*
     * Result image checks.
     *
     * The ONLY address formula used here is ref_job_c_base().
     */
    if (jobs_completed == NUM_JOBS) begin

      for (j = 0; j < NUM_JOBS; j = j + 1) begin
        check_job_result(j);
      end

    end

    /*
     * Print physical result map.
     */
    $display("");
    $display("REFERENCE RESULT MAP");

    for (j = 0; j < NUM_JOBS; j = j + 1) begin

      $display(
        "JOB%0d: C_BASE=0x%0h .. 0x%0h",
        j,
        ref_job_c_base(j),
        ref_job_c_base(j) + C_TILE_BYTES - 1
      );
    end

    /*
     * Print bus summary.
     */
    $display("");
    $display("BUS SUMMARY");
    $display("AR handshakes   = %0d", ar_count);
    $display("R handshakes    = %0d", r_count);
    $display("WB AW handshakes= %0d", wb_aw_count);
    $display("WB W handshakes = %0d", wb_w_count);
    $display("WB B handshakes = %0d", wb_b_count);

    /*
     * Do not treat AXI counts as job completion.
     * They are diagnostics only.
     */

    /*
     * Final status.
     */
    $display("");
    $display("============================================================");

    if (errors == 0) begin
      $display("CLEAN TB: PASS");
      $display(
        "All %0d compiler jobs accepted, started, completed, and",
        NUM_JOBS
      );
      $display(
        "all %0d reference result tiles matched memory.",
        NUM_JOBS
      );
    end
    else begin
      $display(
        "CLEAN TB: FAIL (%0d errors)",
        errors
      );
      $fatal(1);
    end

    $display("============================================================");

    repeat (10) @(posedge clk);

    $finish;
  end

endmodule

`default_nettype wire
'''

TB.write_text(NEW_TB)
print(f"REWRITTEN: {TB}")
print("")
print("Next:")
print("  1. verilator compile the new TB")
print("  2. do NOT interpret failures yet")
print("  3. first check whether the reference payload encoding matches")
print("     dma_operand_writer.sv")
print("  4. then check the actual first AR address")
