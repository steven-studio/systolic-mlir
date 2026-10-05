`default_nettype none
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

module tb_dpti_mem_write_top #(
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
  // Physical host byte-stream interface
  //
  // The TB models the external device side of the 8-bit synchronous
  // FIFO interface.  No command is injected yet; this patch only makes
  // the physical boundary explicit so the existing regression can prove
  // that the wiring itself is non-invasive.
  // --------------------------------------------------------------------

  logic [7:0] dpti_d_drive;
  logic       dpti_d_drive_en;

  wire [7:0]  dpti_d;

  logic       dpti_rxf_n;
  logic       dpti_txe_n;
  logic       dpti_clkout;
  logic       dpti_siwun;

  wire        dpti_rd_n;
  wire        dpti_wr_n;
  wire        dpti_oe_n;

  // JOB0 result bytes observed at the physical output boundary.
  localparam integer RESULT_BYTES_PER_JOB = 64;
  localparam integer TOTAL_RESULT_BYTES =
      NUM_JOBS * RESULT_BYTES_PER_JOB;

  logic [7:0] result_bytes [0:TOTAL_RESULT_BYTES-1];
  integer result_byte_count;
  integer result_byte_errors;

  assign dpti_d =
      dpti_d_drive_en
          ? dpti_d_drive
          : 8'bz;

  // Use the same 100 MHz period in simulation.  This is an independent
  // clock signal even though its nominal frequency matches clk.
  initial dpti_clkout = 1'b0;
  always #5 dpti_clkout = ~dpti_clkout;

  // Capture bytes written by the DUT at the physical output boundary.
  always @(posedge dpti_clkout) begin
    if (!dpti_wr_n) begin
      if (dpti_d_drive_en) begin
        $display(
          "ERROR RESULT OUTPUT: DUT attempted output while TB owns dpti_d"
        );
        result_byte_errors = result_byte_errors + 1;
      end
      else if (result_byte_count >= TOTAL_RESULT_BYTES) begin
        $display(
          "ERROR RESULT OUTPUT: extra byte %02h",
          dpti_d
        );
        result_byte_errors = result_byte_errors + 1;
      end
      else begin
        result_bytes[result_byte_count] = dpti_d;
        result_byte_count = result_byte_count + 1;
      end
    end
  end

  initial begin
    dpti_d_drive    = 8'h00;
    dpti_d_drive_en = 1'b0;

    // No input bytes available during the existing baseline regression.
    dpti_rxf_n      = 1'b1;

    // Output-side inputs remain inactive.
    dpti_txe_n      = 1'b1;
    dpti_siwun      = 1'b1;
  end

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
   * This reference MUST match dma_operand_writer.sv exactly.
   *
   * For N=4, K_MAX=32:
   *
   *   w = {beat[5:0], cnt}
   *
   * A decode:
   *   is_b   = w[5]
   *   win    = w[7:6]
   *   lane   = w[4:3]
   *   koff   = w[2:0]
   *
   * B decode:
   *   is_b   = w[5]
   *   win    = w[7:6]
   *   koff   = w[4:2]
   *   lane   = w[1:0]
   *
   * The old reference incorrectly put matrix at bit 8.  That bit is
   * outside the 8-bit payload word used by dma_operand_writer for
   * K_MAX=32, so the TB preload and the DUT writer were addressing
   * different physical words.
   *
   * The semantic call convention remains:
   *
   *   A: ref_payload_index(0, win, row,  koff)
   *   B: ref_payload_index(1, win, koff, col)
   *
   * IMPORTANT:
   * This function is the SINGLE source of truth for the TB.
   */
  function automatic integer ref_payload_index(
      input integer matrix,
      input integer win,
      input integer lane,
      input integer koff
  );
    begin
      if (matrix == 0) begin
        /*
         * A -- matches dma_operand_writer.sv:
         *
         *   w[7]   = win
         *   w[6]   = 0
         *   w[5:3] = a_lane
         *   w[2:0] = a_koff
         */
        ref_payload_index =
            (win  << 7) |
            (lane << 3) |
            koff;
      end
      else begin
        /*
         * B -- matches dma_operand_writer.sv:
         *
         *   w[7]   = win
         *   w[6]   = 1
         *   w[5:3] = b_koff
         *   w[2:0] = b_lane
         *
         * Call-site convention:
         *   lane = semantic K offset
         *   koff = semantic output column
         */
        ref_payload_index =
            (win  << 7) |
            (1    << 6) |
            (lane << 3) |
            koff;
      end
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

  /*
   * Expected C for the staged coordinate-coded JOB0:
   *
   *   A = [ I4 | 0 ]
   *   B[k,col] = float(16*k + col)
   *
   * Therefore:
   *
   *   C[row,col] = float(16*row + col)
   */
  function automatic logic [31:0] ref_expected_c_coordinate(
      input integer row,
      input integer col
  );
    begin
      ref_expected_c_coordinate =
          diag_uint_to_fp32(16 * row + col);
    end
  endfunction

  // --------------------------------------------------------------------
  // Job producer state
  // --------------------------------------------------------------------

  integer current_job;

  logic job_inflight;

  /*
   * JOB0 must not become visible until its operand slab has been
   * completely staged into DDR through the physical MEM_WRITE path.
   */
  logic operand_staged;

  integer jobs_accepted;
  integer jobs_started;
  integer jobs_completed;

  integer errors;

  logic diagnostic_failed;

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
        operand_staged &&
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
      current_job    <= 0;
      job_inflight   <= 1'b0;
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
       * Compiler-job completion occurs only after the final write-back
       * belonging to the accepted job has completed.
       *
       * scheduler_c_done is intentionally earlier: it marks accelerator
       * computation completion, before P_SCAN and P_WB.
       *
       * dut.job_done is observed hierarchically here because this is a
       * testbench-only checker; it does not change the production interface.
       */
      if (job_inflight && dut.job_done) begin

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

      for (j = 1; j < NUM_JOBS; j = j + 1) begin

        /*
         * ============================================================
         * A preload
         * ============================================================
         *
         * Each compiler job owns its own A slab.
         */
        pbase_word =
            ref_job_a_base(j) / 4;

        /*
         * Clear complete A slab.
         */
        for (idx = 0; idx < (A_SLAB_BYTES / 4); idx = idx + 1) begin
          dut.u_mig_7series_0.mem[pbase_word + idx] =
              32'h0000_0000;
        end

        /*
         * A = I4.
         *
         * Payload layout:
         *
         *   matrix | window | lane | koff
         *
         * Therefore A[row][k] lives at:
         *
         *   ref_payload_index(0, k/8, row, k%8)
         *
         * Only k=0..3 are non-zero for I4.
         */
        for (k = 0; k < JOB_K; k = k + 1) begin

          win  = k / 8;
          koff = k % 8;

          for (row = 0; row < 4; row = row + 1) begin

            /*
             * A is a 4xK identity-style test matrix:
             *
             *   A[0][0] = 1
             *   A[1][1] = 1
             *   A[2][2] = 1
             *   A[3][3] = 1
             *
             * All remaining A[row][k] are zero.
             *
             * The payload address uses the K-window encoding, so
             * compare the row against the K offset within the
             * window, and only populate the first four K positions.
             */
            if ((k < 4) && (row == koff)) begin

              idx = ref_payload_index(
                  0,       // matrix A
                  win,
                  row,     // lane
                  koff
              );

              dut.u_mig_7series_0.mem[
                  pbase_word + idx
              ] = 32'h3f80_0000;

            end
          end
        end
      end

      /*
       * ============================================================
       * B preload
       * ============================================================
       *
       * The DMA descriptor uses ONE combined compiler payload slab.
       *
       * The DUT does NOT read job_b_base_reg here.  Both A and B are
       * therefore stored inside the slab selected by job_a_base.
       *
       * Payload word encoding:
       *
       *   A:
       *     ref_payload_index(0, win, row,  koff)
       *
       *   B:
       *     ref_payload_index(1, win, koff, col)
       *
       * Note that B swaps lane/k-offset relative to A.  This matches
       * dma_operand_writer.sv:
       *
       *   A -> wsel=a_lane, waddr={win,a_koff}
       *   B -> wsel=b_lane, waddr={win,b_koff}
       *
       * B is populated INSIDE every job's A slab.
       */

      for (j = 1; j < NUM_JOBS; j = j + 1) begin

        pbase_word = ref_job_a_base(j) / 4;

        for (k = 0; k < JOB_K; k = k + 1) begin

          win  = k / 8;
          koff = k % 8;

          for (col = 0; col < 4; col = col + 1) begin

            idx = ref_payload_index(
                1,       // matrix B
                win,
                koff,    // B lane encoding
                col      // B K-offset encoding
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

      /*
       * ------------------------------------------------------------
       * Preload sanity checks.
       *
       * Check representative A/B payload words before the DUT runs.
       * ------------------------------------------------------------
       */

      /*
       * ------------------------------------------------------------
       * Preload sanity checks.
       *
       * Check representative A/B words inside JOB0's combined slab.
       * ------------------------------------------------------------
       */

      $display(
        "REFERENCE CHECK JOB0 A[0,0] payload idx=%0d data=%h",
        ref_payload_index(0, 0, 0, 0),
        dut.u_mig_7series_0.mem[
          integer'(ref_job_a_base(0) / 4) +
          ref_payload_index(0, 0, 0, 0)
        ]
      );

      $display(
        "REFERENCE CHECK JOB0 B[0,0] payload idx=%0d data=%h",
        ref_payload_index(1, 0, 0, 0),
        dut.u_mig_7series_0.mem[
          integer'(ref_job_a_base(0) / 4) +
          ref_payload_index(1, 0, 0, 0)
        ]
      );

      $display(
        "REFERENCE CHECK JOB0 B[0,1] payload idx=%0d data=%h",
        ref_payload_index(1, 0, 0, 1),
        dut.u_mig_7series_0.mem[
          integer'(ref_job_a_base(0) / 4) +
          ref_payload_index(1, 0, 0, 1)
        ]
      );

      $display(
        "REFERENCE CHECK JOB0 B[1,0] payload idx=%0d data=%h",
        ref_payload_index(1, 0, 1, 0),
        dut.u_mig_7series_0.mem[
          integer'(ref_job_a_base(0) / 4) +
          ref_payload_index(1, 0, 1, 0)
        ]
      );

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
    .job_c_base      (job_c_base),

    .dpti_d           (dpti_d),
    .dpti_rxf_n       (dpti_rxf_n),
    .dpti_txe_n       (dpti_txe_n),
    .dpti_clkout      (dpti_clkout),

    .dpti_rd_n        (dpti_rd_n),
    .dpti_wr_n        (dpti_wr_n),
    .dpti_oe_n        (dpti_oe_n),
    .dpti_siwun       (dpti_siwun)
  );


  // --------------------------------------------------------------------
  // JOB1 transition debug
  //
  // Scope:
  //   Only inspect the transition JOB1 -> JOB2.
  //
  // This does NOT modify any DUT control signal.
  // --------------------------------------------------------------------

  always @(posedge dut.ui_clk) begin
    if (dut.ui_rst_n) begin

      if ((dut.job_valid || dut.job_ready ||
           dut.scheduler_c_done ||
           dut.ingress_job_id == 32'd101 ||
           dut.ingress_job_id == 32'd102)) begin

        if (($time >= 8800) && ($time <= 9300)) begin
          $display(
            "JOB1TRACE t=%0t job_valid=%0d job_ready=%0d fire=%0d in_job=%0d in_A=0x%0h in_C=0x%0h c_done=%0d tb_current_job=%0d tb_inflight=%0d",
            $time,
            dut.job_valid,
            dut.job_ready,
            dut.job_valid && dut.job_ready,
            dut.ingress_job_id,
            dut.ingress_job_a_base,
            dut.ingress_job_c_base,
            dut.scheduler_c_done,
            current_job,
            job_inflight
          );
        end
      end
    end
  end

  // --------------------------------------------------------------------
  // Independent transaction monitor
  // --------------------------------------------------------------------

  integer ar_count;
  integer r_count;

  integer wb_aw_count;
  integer wb_w_count;
  integer wb_b_count;

  // --------------------------------------------------------------------
  // Diagnostic transaction capture
  //
  // Keep the actual AXI write addresses so that writeback-address
  // correctness can be checked independently from result-data correctness.
  // --------------------------------------------------------------------

  logic [63:0] observed_wb_aw_addr [0:NUM_JOBS-1];

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

        if (wb_aw_count < NUM_JOBS) begin
          observed_wb_aw_addr[wb_aw_count] =
              {{(64-$bits(dut.wb_awaddr)){1'b0}}, dut.wb_awaddr};
        end

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
  // Physical byte-stream stimulus
  // --------------------------------------------------------------------

  task automatic send_dpti_byte(input logic [7:0] value);
    begin
      // Present one byte to the external side.
      @(negedge dpti_clkout);
      dpti_d_drive    = value;
      dpti_d_drive_en = 1'b1;
      dpti_rxf_n      = 1'b0;

      // Wait until the DUT actually performs the physical read.
      while (dpti_rd_n !== 1'b0)
        @(negedge dpti_clkout);

      // Keep the byte stable through the active read interval.
      @(negedge dpti_clkout);

      dpti_rxf_n      = 1'b1;
      dpti_d_drive_en = 1'b0;
      dpti_d_drive    = 8'h00;

      // Give the receiver one idle half-cycle before the next byte.
      @(negedge dpti_clkout);
    end
  endtask



  /*
   * Convert the small non-negative integers used by the coordinate-coded
   * diagnostic pattern to IEEE-754 binary32 without real/shortreal.
   *
   * Required range here is only 0..243, so every value is represented
   * exactly as a single-precision float.
   */
  function automatic logic [31:0] diag_uint_to_fp32(
      input integer value
  );
    integer msb;
    integer i;
    logic [7:0] exponent;
    logic [23:0] significand;
    begin
      if (value == 0) begin
        diag_uint_to_fp32 = 32'h0000_0000;
      end else begin
        msb = 0;

        for (i = 0; i < 31; i = i + 1) begin
          if ((value >> i) != 0)
            msb = i;
        end

        exponent = 8'(127 + msb);

        if (msb <= 23)
          significand = value << (23 - msb);
        else
          significand = value >> (msb - 23);

        diag_uint_to_fp32 = {
          1'b0,
          exponent,
          significand[22:0]
        };
      end
    end
  endfunction


  task automatic send_mem_write_operand_1k(
      input logic [31:0] base_addr
  );
    integer i;
    integer row;
    integer col;
    integer k;
    integer win;
    integer koff;
    integer idx;

    logic [31:0] payload [0:255];

    begin
      /*
       * Build exactly the same 1 KiB combined A+B operand image
       * previously constructed by PRELOAD_REFERENCE.
       */
      for (i = 0; i < 256; i = i + 1)
        payload[i] = 32'h0000_0000;

      /*
       * A = 4xK identity-style matrix.
       */
      for (k = 0; k < JOB_K; k = k + 1) begin
        win  = k / 8;
        koff = k % 8;

        for (row = 0; row < 4; row = row + 1) begin
          if ((k < 4) && (row == koff)) begin
            idx = ref_payload_index(
                0,
                win,
                row,
                koff
            );

            payload[idx] = 32'h3f80_0000;
          end
        end
      end

      /*
       * Coordinate-coded B:
       *
       *   B[k,col] = float(16*k + col)
       *
       * This makes the K coordinate directly observable at the feeder.
       */
      for (k = 0; k < JOB_K; k = k + 1) begin
        win  = k / 8;
        koff = k % 8;

        for (col = 0; col < 4; col = col + 1) begin
          idx = ref_payload_index(
              1,
              win,
              koff,
              col
          );

          payload[idx] =
              diag_uint_to_fp32(16 * k + col);
        end
      end

      $display(
        "TB OPERAND MEM_WRITE: base=0x%08x bytes=1024",
        base_addr
      );

      /*
       * MEM_WRITE header.
       */
      send_dpti_byte(8'h02);

      send_dpti_byte(base_addr[7:0]);
      send_dpti_byte(base_addr[15:8]);
      send_dpti_byte(base_addr[23:16]);
      send_dpti_byte(base_addr[31:24]);

      send_dpti_byte(8'h00);
      send_dpti_byte(8'h04);
      send_dpti_byte(8'h00);
      send_dpti_byte(8'h00);

      /*
       * Send the 256 32-bit payload words little-endian.
       */
      for (i = 0; i < 256; i = i + 1) begin
        send_dpti_byte(payload[i][7:0]);
        send_dpti_byte(payload[i][15:8]);
        send_dpti_byte(payload[i][23:16]);
        send_dpti_byte(payload[i][31:24]);
      end

      $display(
        "TB OPERAND MEM_WRITE: all physical operand bytes sent"
      );
    end
  endtask


  task automatic check_operand_mem_write_1k(
      input logic [31:0] base_addr
  );
    integer i;
    integer row;
    integer col;
    integer k;
    integer win;
    integer koff;
    integer idx;
    integer word_addr;

    logic [31:0] expected [0:255];
    logic [31:0] got;

    begin
      /*
       * Reconstruct the expected combined A+B operand slab
       * independently from the sender.
       */
      for (i = 0; i < 256; i = i + 1)
        expected[i] = 32'h0000_0000;

      /*
       * A = 4xK identity-style matrix.
       */
      for (k = 0; k < JOB_K; k = k + 1) begin
        win  = k / 8;
        koff = k % 8;

        for (row = 0; row < 4; row = row + 1) begin
          if ((k < 4) && (row == koff)) begin
            idx = ref_payload_index(
                0,
                win,
                row,
                koff
            );

            expected[idx] = 32'h3f80_0000;
          end
        end
      end

      /*
       * Coordinate-coded B:
       *
       *   B[k,col] = float(16*k + col)
       */
      for (k = 0; k < JOB_K; k = k + 1) begin
        win  = k / 8;
        koff = k % 8;

        for (col = 0; col < 4; col = col + 1) begin
          idx = ref_payload_index(
              1,
              win,
              koff,
              col
          );

          expected[idx] =
              diag_uint_to_fp32(16 * k + col);
        end
      end

      /*
       * Compare all 256 words in the actual DDR image.
       */
      for (i = 0; i < 256; i = i + 1) begin
        word_addr = (base_addr / 4) + i;
        got = dut.u_mig_7series_0.mem[word_addr];

        if (got !== expected[i]) begin
          $display(
            "ERROR OPERAND MEM_WRITE word=%0d addr=0x%08x got=%08x want=%08x",
            i,
            base_addr + i*4,
            got,
            expected[i]
          );

          errors = errors + 1;
        end
      end

      if (errors == 0) begin
        $display(
          "DPTI PHYSICAL OPERAND MEM_WRITE -> DDR: PASS (256/256 words)"
        );

        $display(
          "OPERAND CHECK A[0,0]: idx=%0d data=%08x",
          ref_payload_index(0, 0, 0, 0),
          dut.u_mig_7series_0.mem[
            (base_addr / 4) +
            ref_payload_index(0, 0, 0, 0)
          ]
        );

        $display(
          "DDRBDUMP B00=%08x B01=%08x B02=%08x B03=%08x B10=%08x B11=%08x B20=%08x B30=%08x",
          dut.u_mig_7series_0.mem[(base_addr / 4) + ref_payload_index(1, 0, 0, 0)],
          dut.u_mig_7series_0.mem[(base_addr / 4) + ref_payload_index(1, 0, 0, 1)],
          dut.u_mig_7series_0.mem[(base_addr / 4) + ref_payload_index(1, 0, 0, 2)],
          dut.u_mig_7series_0.mem[(base_addr / 4) + ref_payload_index(1, 0, 0, 3)],
          dut.u_mig_7series_0.mem[(base_addr / 4) + ref_payload_index(1, 0, 1, 0)],
          dut.u_mig_7series_0.mem[(base_addr / 4) + ref_payload_index(1, 0, 1, 1)],
          dut.u_mig_7series_0.mem[(base_addr / 4) + ref_payload_index(1, 0, 2, 0)],
          dut.u_mig_7series_0.mem[(base_addr / 4) + ref_payload_index(1, 0, 3, 0)]
        );

        $display(
          "JOB1DDR B00=%08x B01=%08x B02=%08x B03=%08x B10=%08x B11=%08x B20=%08x B30=%08x",
          dut.u_mig_7series_0.mem[
            (ref_job_a_base(1) / 4) + ref_payload_index(1, 0, 0, 0)
          ],
          dut.u_mig_7series_0.mem[
            (ref_job_a_base(1) / 4) + ref_payload_index(1, 0, 0, 1)
          ],
          dut.u_mig_7series_0.mem[
            (ref_job_a_base(1) / 4) + ref_payload_index(1, 0, 0, 2)
          ],
          dut.u_mig_7series_0.mem[
            (ref_job_a_base(1) / 4) + ref_payload_index(1, 0, 0, 3)
          ],
          dut.u_mig_7series_0.mem[
            (ref_job_a_base(1) / 4) + ref_payload_index(1, 0, 1, 0)
          ],
          dut.u_mig_7series_0.mem[
            (ref_job_a_base(1) / 4) + ref_payload_index(1, 0, 1, 1)
          ],
          dut.u_mig_7series_0.mem[
            (ref_job_a_base(1) / 4) + ref_payload_index(1, 0, 2, 0)
          ],
          dut.u_mig_7series_0.mem[
            (ref_job_a_base(1) / 4) + ref_payload_index(1, 0, 3, 0)
          ]
        );

        $display(
          "OPERAND CHECK B[0,0]: idx=%0d data=%08x",
          ref_payload_index(1, 0, 0, 0),
          dut.u_mig_7series_0.mem[
            (base_addr / 4) +
            ref_payload_index(1, 0, 0, 0)
          ]
        );
      end
    end
  endtask


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
            integer'(ref_job_c_base(j) / 4) + i;

        got =
            dut.u_mig_7series_0.mem[word_addr];

        want =
            (j == 0)
                ? ref_expected_c_coordinate(row, col)
                : ref_expected_c(row, col);

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

  task automatic check_physical_result_bytes;
    integer i;
    integer j;
    integer tile_byte;
    integer word_index;
    integer row;
    integer col;
    integer byte_lane;

    logic [31:0] want_word;
    logic [7:0]  want_byte;

    begin
      if (result_byte_count != TOTAL_RESULT_BYTES) begin
        $display(
          "ERROR RESULT OUTPUT: received %0d bytes, expected %0d",
          result_byte_count,
          TOTAL_RESULT_BYTES
        );
        errors = errors + 1;
      end

      for (i = 0;
           i < result_byte_count && i < TOTAL_RESULT_BYTES;
           i = i + 1) begin

        j          = i / RESULT_BYTES_PER_JOB;
        tile_byte  = i % RESULT_BYTES_PER_JOB;
        word_index = tile_byte / 4;
        row        = word_index / 4;
        col        = word_index % 4;
        byte_lane  = tile_byte % 4;

        want_word =
            (j == 0)
                ? ref_expected_c_coordinate(row, col)
                : ref_expected_c(row, col);

        case (byte_lane)
          0: want_byte = want_word[7:0];
          1: want_byte = want_word[15:8];
          2: want_byte = want_word[23:16];
          default: want_byte = want_word[31:24];
        endcase

        if (result_bytes[i] !== want_byte) begin
          $display(
            "ERROR RESULT BYTE job=%0d byte=%0d row=%0d col=%0d lane=%0d got=%02h want=%02h",
            j,
            tile_byte,
            row,
            col,
            byte_lane,
            result_bytes[i],
            want_byte
          );
          errors = errors + 1;
        end
      end

      if (result_byte_errors != 0) begin
        $display(
          "ERROR RESULT OUTPUT: %0d physical-output protocol error(s)",
          result_byte_errors
        );
        errors = errors + result_byte_errors;
      end

      if ((result_byte_count == TOTAL_RESULT_BYTES) &&
          (result_byte_errors == 0)) begin
        $display(
          "PHYSICAL RESULT OUTPUT: %0d/%0d bytes captured",
          result_byte_count,
          TOTAL_RESULT_BYTES
        );
      end
    end
  endtask

  // --------------------------------------------------------------------
  // Final checker
  // --------------------------------------------------------------------

  initial begin : MAIN

    integer waited;

    operand_staged = 1'b0;

    errors = 0;

    result_byte_count  = 0;
    result_byte_errors = 0;

    current_job    = 0;
    job_inflight   = 1'b0;

    jobs_accepted  = 0;
    jobs_started   = 0;
    jobs_completed = 0;

    ar_count       = 0;
    r_count        = 0;

    wb_aw_count    = 0;
    wb_w_count     = 0;
    wb_b_count     = 0;

    $display("");
    $display("============================================================");
    $display(" PHYSICAL MEM_WRITE -> DDR TOP-LEVEL TEST");
    $display("============================================================");
    $display("");

    // ------------------------------------------------------------
    // Reset.
    // ------------------------------------------------------------

    repeat (20) @(posedge clk);
    rstn = 1'b1;

    $display("RESET RELEASED");

    // Wait for the memory controller / DUT ui-domain reset to finish.
    waited = 0;

    while ((!dut.ui_rst_n || !dut.init_calib_complete) &&
           waited < 10000) begin
      @(posedge clk);
      waited = waited + 1;
    end

    if (!dut.ui_rst_n || !dut.init_calib_complete) begin
      $display("FAIL: DUT did not finish reset/calibration.");
      $fatal(1);
    end

    $display("DUT READY");

    // Give the physical-input reset synchronizer time to release.
    repeat (8) @(posedge dpti_clkout);

    if (dut.dpti_rst !== 1'b0) begin
      $display("FAIL: physical-input reset did not release.");
      $fatal(1);
    end

    // ------------------------------------------------------------
    // Send exactly one 1 KiB MEM_WRITE.
    // ------------------------------------------------------------

    send_mem_write_operand_1k(32'h0001_0000);

    // Input staging has released the bidirectional data bus.
    // Allow the DUT to return the completed result.
    dpti_txe_n = 1'b0;

    // ------------------------------------------------------------
    // Byte delivery finishing is NOT DDR completion.
    // Wait for the AXI writer's final completion pulse.
    // ------------------------------------------------------------

    waited = 0;

    while ((dut.hs_done !== 1'b1) &&
           waited < 20000) begin
      @(posedge dut.ui_clk);
      waited = waited + 1;
    end

    if (dut.hs_done !== 1'b1) begin
      $display(
        "FAIL: host DDR writer did not complete. busy=%0b protocol=%0b align=%0b resp=%0b",
        dut.hs_busy,
        dut.hs_err_protocol,
        dut.hs_err_align,
        dut.hs_err_resp
      );
      $fatal(1);
    end

    $display(
      "HOST WRITER DONE after %0d ui_clk cycles",
      waited
    );

    // Completion pulse means the final write response has retired.
    @(posedge dut.ui_clk);

    if (dut.hs_err_protocol ||
        dut.hs_err_align ||
        dut.hs_err_resp) begin

      $display(
        "FAIL: host writer error protocol=%0b align=%0b resp=%0b",
        dut.hs_err_protocol,
        dut.hs_err_align,
        dut.hs_err_resp
      );
      $fatal(1);
    end

    // ------------------------------------------------------------
    // Verify the actual memory image.
    // ------------------------------------------------------------

    check_operand_mem_write_1k(32'h0001_0000);

    if (errors != 0) begin
      $display(
        "FAIL: staged operand image is incorrect; JOB0 will not run."
      );
      $fatal(1);
    end

    /*
     * The operand image is now fully resident and verified.
     * JOB0 may finally become visible to the scheduler.
     */
    operand_staged = 1'b1;

    $display(
      "OPERAND STAGED: releasing JOB0"
    );

    if (dut.u_mig_7series_0.errors != 0) begin
      $display(
        "FAIL: MIG model reported %0d protocol error(s).",
        dut.u_mig_7series_0.errors
      );
      errors = errors + dut.u_mig_7series_0.errors;
    end

    // ------------------------------------------------------------
    // JOB0 must now consume the physically staged operand slab.
    // Wait for the complete compiler-job boundary.
    // ------------------------------------------------------------

    waited = 0;

    while ((jobs_accepted < 1) &&
           waited < 20000) begin
      @(posedge clk);
      waited = waited + 1;
    end

    if (jobs_accepted < 1) begin
      $display(
        "FAIL: JOB0 was never accepted after operand staging."
      );
      $fatal(1);
    end

    $display(
      "STAGED COMPUTE: JOB0 accepted after %0d cycles",
      waited
    );

    waited = 0;

    while ((jobs_started < 1) &&
           waited < 20000) begin
      @(posedge dut.ui_clk);
      waited = waited + 1;
    end

    if (jobs_started < 1) begin
      $display(
        "FAIL: JOB0 was accepted but accelerator never started."
      );
      $fatal(1);
    end

    $display(
      "STAGED COMPUTE: JOB0 accelerator started after %0d ui_clk cycles",
      waited
    );

    waited = 0;

    while ((jobs_completed < NUM_JOBS) &&
           waited < 1000000) begin
      @(posedge clk);
      waited = waited + 1;
    end

    if (jobs_completed < NUM_JOBS) begin
      $display(
        "FAIL: only %0d/%0d jobs completed.",
        jobs_completed,
        NUM_JOBS
      );
      $fatal(1);
    end

    $display(
      "STAGED COMPUTE: %0d/%0d jobs completed after %0d cycles",
      jobs_completed,
      NUM_JOBS,
      waited
    );

    /*
     * JOB0 C_BASE is 0x00003000.
     *
     * This result can only be meaningful if the accelerator consumed
     * the operand slab staged at JOB0 A_BASE = 0x00010000.
     */
    for (integer j = 0; j < NUM_JOBS; j = j + 1)
      check_job_result(j);

    // The DDR results are correct.  Now wait for every result tile to
    // traverse readback -> CDC -> serializer -> physical output.
    waited = 0;

    while ((result_byte_count < TOTAL_RESULT_BYTES) &&
           waited < 200000) begin
      @(posedge dpti_clkout);
      waited = waited + 1;
    end

    if (result_byte_count < TOTAL_RESULT_BYTES) begin
      $display(
        "FAIL: physical result output timed out after %0d/%0d bytes.",
        result_byte_count,
        TOTAL_RESULT_BYTES
      );
      errors = errors + 1;
    end

    // Allow one additional cycle to expose an accidental 65th byte.
    @(posedge dpti_clkout);

    check_physical_result_bytes();

    if (dut.u_mig_7series_0.errors != 0) begin
      $display(
        "FAIL: MIG model reported %0d protocol error(s).",
        dut.u_mig_7series_0.errors
      );
      errors = errors + dut.u_mig_7series_0.errors;
    end

    $display("");
    $display("============================================================");

    if (errors == 0) begin
      $display("STAGED OPERAND -> COMPUTE TOP TEST: PASS");
      $display("physical operand bytes -> DDR @ 0x00010000");
      $display("DDR operands -> 10/10 jobs compute");
      $display("10/10 job results -> DDR");
      $display("160/160 FP32 result words verified");
      $display("DDR results -> physical output");
      $display("640/640 result bytes verified");
    end
    else begin
      $display(
        "STAGED OPERAND -> COMPUTE TOP TEST: FAIL (%0d errors)",
        errors
      );
      $fatal(1);
    end

    $display("============================================================");
    $finish;
  end

endmodule

`default_nettype wire
