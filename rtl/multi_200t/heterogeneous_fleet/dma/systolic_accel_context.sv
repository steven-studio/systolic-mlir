// -----------------------------------------------------------------------------
// systolic_accel_context.sv -- one physical accelerator's execution context.
//
// Everything a job needs from acceptance to its last write response, for ONE
// physical array:
//
//   job registers  -> P_READ (operand fill into this context's own ping-pong
//   buffers, through the shared DMA read engine) -> P_GO (one scheduler
//   release per tile) -> P_FOLD (this array computes) -> hand-off to this
//   context's write-back stage (scan + descriptor to the shared write-back
//   engine) -> P_DONE once the stage has drained.
//
// This is the per-tile control that systolic_dma_top used to hold once, for
// whichever device the accepted job named.  Holding it once meant one job in
// flight and one array computing; holding it per accelerator is what lets the
// 8x8 and the three 4x4s compute at the same time.  The compute / write-back
// overlap inside a context (tile N's fold under tile N-1's write-back) is kept
// exactly as it was.
//
// PIPELINED JOBS (Oct 2026, USE_EXTERNAL_SCHEDULER only)
//   Every PE holds two accumulator sets and two adders (core/systolic_pe.sv)
//   so that tile T+1 can be fed and accumulated while tile T's bank reduction
//   -- 67 of the 95 cycles of H -- still runs; the PE only needs the array
//   input quiet for 22 cycles between the two (tb/tb_pe_overlap.sv).  This
//   context used to throw that away: its compute FSM waited for c_valid_out
//   (after the reduction) before it could take another fold, P_FOLD waited for
//   the same before handing the tile to write-back, and the top accepted a
//   context's next job only once the previous one was written back and read
//   back.  Every invocation of one accelerator therefore ran read -> compute
//   -> reduce -> write strictly after the previous one.
//
//   Now the external-scheduler path runs a queue of JOB_QUEUE_DEPTH jobs
//   through three stages that share nothing but the counters below:
//
//     fill     operand descriptor for job n_filled, as soon as no completely
//              filled tile is waiting (the ping-pong buffer it writes was last
//              read by job n_filled - 2, whose feed ended before job
//              n_filled - 1's fold started)
//     compute  one scheduler request per job; the release is taken when the
//              job's operands have landed, the array input has been idle for
//              the PE quiet gap after the previous job's k steps, and the
//              result bank it will publish into (n_fed % 2) has been written
//              back (job n_fed - 2 done).  The feed of job n_fed+1 therefore
//              starts while job n_fed is still being reduced.
//     write    the array publishes in order; c_valid_out of result n_cv lands
//              in bank n_cv % 2; the stage scans and writes back result n_wb
//              to that job's own c_base and completes the job (job_done).
//
//   Nothing about the datapath changes: the same feeder, buffers, array and
//   result banks; the legacy bring-up (USE_EXTERNAL_SCHEDULER = 0, n_inv tiles
//   of one slab) keeps its phase FSM verbatim.  What a job may NOT do is
//   unchanged too: run past its k, read a buffer being written, publish into
//   a bank being written back, or start before the PE has seen the boundary.
//
//   Status that still describes ONE transaction, not a pipelined run:
//   cyc_latched / cyc_total (first ST_FEED beat to the publish of the same
//   fold; skipped for a fold that starts while the previous one is being
//   measured), t_span (first fill to the first write-back of the run) and
//   wr_match (tabulated for k = 16 and 32 only).  words_written is the last
//   fill's payload.  The per-job evidence is in tb_pipeline_overlap.sv.
//
// What is per context and what is shared:
//
//   per context     feeder (N_ARRAY lanes), operand buffers (N_ARRAY banks x
//                   K_MAX, A/B, ping-pong), operand writer, result snapshot
//                   banks, result reader + FIFO, phase FSM, write-back stage,
//                   every counter and sticky flag, the job registers.
//   shared (top)    the DMA read engine (one AXI read master; descriptors are
//                   arbitrated and tagged per context), the write-back engine
//                   (one AXI write master; one owner at a time), the seeder,
//                   the scheduler, the ingress, MIG.  Shared engines serialize
//                   the MOVING of operands and results, not the computing.
//
// K_MAX is this accelerator's own depth (K_MAX_8X8 for the 8x8 context,
// K_MAX_4X4 for a 4x4 context).  The operand payload keeps the host's N_WIRE-
// lane format -- the writer decodes it as before -- and a context narrower
// than the wire simply does not store the lanes its array does not have.
// words_written and chk_wr still count every word of the payload, so the
// tabulated checksums are unchanged.
//
// K_MAX is any positive integer.  The wire carries the reduction in windows
// of 8 steps, so this context's slab is SLAB_K = ceil(K_MAX / 8) * 8 steps and
// a job of depth k moves ceil(k / 8) whole windows; slab steps >= K_MAX are
// padding and are dropped at the buffer, exactly as lanes >= N_ARRAY are.
// Slab strides are multiplications by the slab size, not shifts, so nothing
// requires a power of two.  (USE_V2's beat-wide path keeps its own contract,
// K_MAX a multiple of 8 -- see the generate below.)
// -----------------------------------------------------------------------------

`default_nettype none

module systolic_accel_context #(
  parameter integer N_ARRAY   = 8,       // this array's edge: lanes, banks, C
  parameter integer N_WIRE    = 8,       // lanes in the operand payload format
  parameter integer K_MAX     = 32,      // this accelerator's operand depth
  parameter integer K_DIM     = 16,      // legacy reduction length (<= K_MAX)
  parameter integer DEVICE_ID = 0,

  parameter bit     USE_V2                 = 1'b0,
  parameter bit     USE_EXTERNAL_SCHEDULER = 1'b0,
  // The legacy bring-up (seed -> read -> fold -> write-back, n_inv from the
  // VIO) runs on exactly one context; the others only ever take external jobs.
  parameter bit     LEGACY_RUN = 1'b1,

  parameter integer BASE_ADDR    = 0,
  parameter integer WB_GAP_BYTES = 4096,
  parameter logic [31:0] EXPECT_WR_CHK = 32'h3F88_0780,
  parameter logic [31:0] EXPECT_C_CHK  = 32'hC74B_2660,

  parameter integer AXI_DATA_W = 128,
  parameter integer AXI_ADDR_W = 29,
  parameter integer BEAT_W     = 16,

  // Jobs one context holds at a time (accepted, not yet written back), a
  // power of two.  3 are ever active -- one filling, one feeding or reducing,
  // one writing back -- the fourth entry lets the host stay a descriptor ahead.
  parameter integer JOB_QUEUE_DEPTH = 4,
  // The PE detects a tile boundary as a quiet gap on its operand inputs of
  // MUL_LATENCY + ADD_LATENCY + 2 idle cycles between the last operand pair of
  // one tile and the first of the next (systolic_pe.sv: acc_handoff needs the
  // multiplier and the accumulate adder drained): 23 with tb/fp_model.sv's
  // 9 / 12, 21 with the IP's 8 / 11 (tb_pe_overlap.sv: GAP = idle - 1 passes
  // at 22 / 20 and merges the two tiles one below).  The margin is cycles on
  // top of the larger of the two.
  parameter integer PE_QUIET_GAP    = 23,
  parameter integer FEED_GAP_MARGIN = 3
) (
  input  wire        clk,                 // ui_clk
  input  wire        ui_rst_n,
  input  wire        init_calib_complete,

  // ---- run control (legacy bring-up) --------------------------------------
  input  wire [3:0]  n_inv_probe,
  input  wire        rerun_pulse,

  // ---- job acceptance (this context's share of the ingress) ---------------
  input  wire        job_fire,
  input  wire [31:0] in_job_id,
  input  wire [31:0] in_device_id,
  input  wire [31:0] in_k,
  input  wire [31:0] in_start_cycle,
  input  wire [31:0] in_est_cycles,
  input  wire [63:0] in_a_base,
  input  wire [63:0] in_b_base,
  input  wire [63:0] in_c_base,

  output logic        job_busy,
  output logic        job_active,
  output wire         job_accept_ready,    // the queue can take a job now
  output wire         job_done,            // one cycle: a job's result tile is in DDR
  output logic [31:0] job_id_reg,
  output logic [31:0] job_device_id_reg,
  output logic [31:0] job_k_reg,
  output logic [31:0] job_start_cycle_reg,
  output logic [31:0] job_est_cycles_reg,
  output logic [63:0] job_c_base_reg,

  // ---- scheduler ----------------------------------------------------------
  output logic        tile_req,            // level: a tile wants a release
  input  wire         tile_ack,            // scheduler took the request
  output wire         accelerator_ready,   // the release may be issued now
  input  wire         fold_start_ext,      // the adapter's one-cycle release
  output wire         scheduler_c_done,

  // ---- the array ----------------------------------------------------------
  output logic [31:0] a_in       [0:N_ARRAY-1],
  output logic [31:0] b_in       [0:N_ARRAY-1],
  output logic        a_valid_in [0:N_ARRAY-1],
  output logic        b_valid_in [0:N_ARRAY-1],
  input  wire         c_valid_out,
  input  wire  [31:0] c_out      [0:N_ARRAY-1][0:N_ARRAY-1],

  // ---- shared DMA read engine (operand descriptors) -----------------------
  output wire                   op_desc_valid,
  output wire [AXI_ADDR_W-1:0]  op_desc_addr,
  output wire [BEAT_W-1:0]      op_desc_beats,
  input  wire                   op_desc_ready,
  input  wire                   dst_wr_en,       // already filtered to this context
  input  wire [BEAT_W-1:0]      dst_wr_beat,
  input  wire [AXI_DATA_W-1:0]  dst_wr_data,
  output wire                   dst_full,
  output wire                   dst_almost_full,
  input  wire                   read_done,       // this context's descriptor completed

  // ---- shared write-back engine -------------------------------------------
  output logic                  wb_desc_valid,
  output wire [AXI_ADDR_W-1:0]  wb_desc_addr,
  output wire [BEAT_W-1:0]      wb_desc_beats,
  input  wire                   wb_desc_ready,
  input  wire                   wb_done,         // this context's descriptor completed
  output wire                   src_valid,
  output wire [AXI_DATA_W-1:0]  src_data,
  input  wire                   src_ready,
  output wire                   wb_active,       // in W_WB: wants the write channel

  // ---- seeder (legacy bring-up) -------------------------------------------
  output logic                  seed_start,
  output wire [AXI_ADDR_W-1:0]  seed_slab_addr,
  input  wire                   seed_done,

  // ---- status / instrumentation -------------------------------------------
  output wire [3:0]   phase_o,
  output wire         run_clear,
  output wire [31:0]  words_written,
  output wire [31:0]  chk_wr,
  output logic [31:0] want_wr,
  output logic [31:0] chk_c,
  output logic [31:0] want_c,
  output logic [7:0]  folds_done,
  output logic [3:0]  n_inv,
  output logic [31:0] cyc_latched,
  output logic [31:0] cyc_total,
  output logic [31:0] fill_cycles,
  output logic [31:0] wb_cycles,
  output logic [31:0] t_span,
  output logic        seed_done_sticky,
  output logic        read_done_sticky,
  output logic        fold_done_sticky,
  output logic        wb_done_sticky,
  output wire         wr_match,
  output wire         c_match,
  output wire         wr_err_range,
  output wire [AXI_ADDR_W-1:0] wb_region_base,
  output logic [31:0] C [0:N_ARRAY-1][0:N_ARRAY-1]
);

  // ---- geometry -------------------------------------------------------------
  // The array input must be idle for FEED_GAP cycles after an invocation's k
  // steps before the next invocation's first step: PE(r, c) sees both tiles
  // r + c cycles later, so the gap is the same at every PE.
  localparam integer FEED_GAP   = PE_QUIET_GAP + FEED_GAP_MARGIN;
  localparam int     QIDX_W     = (JOB_QUEUE_DEPTH > 1) ? $clog2(JOB_QUEUE_DEPTH) : 1;
  localparam integer WIN_K      = 8;                     // steps per wire window
  localparam integer SLAB_K     = ((K_MAX + WIN_K - 1) / WIN_K) * WIN_K;
  localparam integer RX_BYTES   = SLAB_K * 8 * N_WIRE;   // one physical slab
  localparam integer RX_WORDS   = RX_BYTES / 4;
  // Buffer address: enough bits for 0..K_MAX-1, and at least one (K_MAX = 1).
  localparam int     K_W        = (K_MAX > 1) ? $clog2(K_MAX) : 1;
  // The writer's address: a step of the whole slab, 0..SLAB_K-1.
  localparam int     SLAB_W     = $clog2(SLAB_K);
  localparam int     LANE_W     = $clog2(N_ARRAY);
  localparam int     LANE_W_W   = $clog2(N_WIRE);
  localparam int     FEED_LAST  = K_MAX + N_ARRAY - 2;
  localparam int     FEED_W     = $clog2(FEED_LAST + 1);
  // Legacy bring-up reads K_DIM steps: that many whole windows.
  localparam integer K_DIM_SLAB = ((K_DIM + WIN_K - 1) / WIN_K) * WIN_K;
  localparam integer C_N        = N_ARRAY * N_ARRAY;
  localparam integer WB_TILE_BYTES = C_N * 4;
  localparam integer WB_BEATS   = C_N / (AXI_DATA_W / 32);
  localparam int     WBT_SHIFT  = $clog2(WB_TILE_BYTES);

  wire rst_i = ~ui_rst_n;

  // =========================================================================
  // Phases.  The encoding is the one the bench and the board scripts know:
  // P_DONE is 7.  P_SCAN is no longer visited by this FSM (the scan lives in
  // the write-back stage) but keeps its slot so nothing renumbers.
  // =========================================================================
  typedef enum logic [3:0] {
    P_CALIB, P_SEED, P_READ, P_GO, P_FOLD, P_SCAN, P_WB, P_DONE
  } phase_t;
  phase_t phase;
  assign phase_o   = phase;
  // The run boundary.  Legacy: P_CALIB, which the external path also used to
  // pass through between every two jobs (P_DONE -> P_CALIB -> P_READ).  With
  // jobs pipelined there is no such pass; a run is the jobs the context holds
  // without going idle, and the boundary is a job accepted into an empty
  // queue.  What run_clear resets is run-scoped status (checksums, counters,
  // sticky flags), never the state of a job in flight -- the queue is empty.
  wire run_boundary;
  assign run_clear = (phase == P_CALIB) || run_boundary;

  // ---- job registers --------------------------------------------------------
  logic [63:0] job_a_base_reg;
  logic [63:0] job_b_base_reg;

  // ---- the job queue (external scheduler) ------------------------------------
  // Written at job_fire into entry n_accepted; read by the fill stage
  // (n_filled), the scheduler request (n_req), the feed (n_fed) and the
  // write-back (n_wb).  The counters only ever increase and wrap at 256; a
  // stage's index is the counter modulo JOB_QUEUE_DEPTH, and occupancy is the
  // difference n_accepted - n_wb.
  logic [31:0]           q_k          [0:JOB_QUEUE_DEPTH-1];
  logic [31:0]           q_k_slab     [0:JOB_QUEUE_DEPTH-1];
  logic [31:0]           q_start_cycle[0:JOB_QUEUE_DEPTH-1];
  logic [31:0]           q_est_cycles [0:JOB_QUEUE_DEPTH-1];
  logic [31:0]           q_id         [0:JOB_QUEUE_DEPTH-1];
  logic [AXI_ADDR_W-1:0] q_a_base     [0:JOB_QUEUE_DEPTH-1];
  logic [AXI_ADDR_W-1:0] q_c_base     [0:JOB_QUEUE_DEPTH-1];

  logic [7:0] n_accepted;   // jobs taken from the ingress
  logic [7:0] n_filled;     // jobs whose operand tile has completely landed
  logic [7:0] n_req;        // jobs whose scheduler request was acknowledged
  logic [7:0] n_fed;        // jobs whose fold has started
  logic [7:0] n_cv;         // results published by the array (in order)
  logic [7:0] n_wb;         // results written back (= jobs completed)

  wire [QIDX_W-1:0] qi_fill = n_filled[QIDX_W-1:0];
  wire [QIDX_W-1:0] qi_req  = n_req[QIDX_W-1:0];
  wire [QIDX_W-1:0] qi_fed  = n_fed[QIDX_W-1:0];
  wire [QIDX_W-1:0] qi_wb   = n_wb[QIDX_W-1:0];
  wire [QIDX_W-1:0] qi_acc  = n_accepted[QIDX_W-1:0];

  logic [AXI_ADDR_W-1:0] last_c_base;   // the last completed job's c_base
  logic [31:0]           last_id;       // and id (status while idle)

  wire [7:0] q_occupancy  = n_accepted - n_wb;
  assign run_boundary = USE_EXTERNAL_SCHEDULER && job_fire && (q_occupancy == 8'd0);
  wire       q_full       = (q_occupancy == 8'(JOB_QUEUE_DEPTH));
  wire       fill_pending = (n_accepted != n_filled);   // a job waits for its fill
  wire       tile_waiting = (n_filled != n_fed);        // a filled tile waits for the array
  wire       feed_pending = (n_accepted != n_fed);      // a job has not been fed yet
  wire       wb_pending   = (n_cv != n_wb);             // a published result waits for write-back

  // Job n_fed publishes into result bank n_fed % 2, last used by job
  // n_fed - 2: free once that job is written back (n_fed - n_wb < 2).
  wire result_bank_free = ((n_fed - n_wb) < 8'd2);

  // The array holds at most two tiles: every PE has two accumulator sets, and
  // the PE has no back-pressure -- if tile j+1's operands arrive while tile
  // j's set is still waiting for tile j-1's reduction to finish
  // (acc_handoff needs red_state == RED_IDLE), the PE keeps summing them into
  // tile j's set, silently.  So fold n_fed is released only once tile
  // n_fed - 2 has been published (every PE's reduction of it done): n_cv >=
  // n_fed - 1.  The quiet gap below covers the other half of the boundary,
  // the drain of tile n_fed - 1's own products.  (result_bank_free implies
  // this today, because a tile is written back only after it is published;
  // the guard stands on its own so that neither rule depends on the other.)
  wire array_slot_free = ((n_fed - n_cv) < 8'd2);

  // ---- invocation bookkeeping ----------------------------------------------
  wire  [3:0] n_inv_next = (n_inv_probe == 4'd0) ? 4'd1 : n_inv_probe;
  logic [3:0] fi;                     // legacy serial index (seeding, debug)
  logic [3:0] read_fi;
  logic [3:0] compute_fi;
  logic [3:0] wb_fi;
  logic [3:0] filled_fi;

  // The ping-pong operand buffers: the one being written and the one being
  // read.  External: by job counter; legacy: by tile index.
  logic compute_idx_lsb;   // external: the fed job's parity, latched at fold_start
  wire fill_bank    = USE_EXTERNAL_SCHEDULER ? n_filled[0] : read_fi[0];
  wire compute_bank = USE_EXTERNAL_SCHEDULER ? compute_idx_lsb : compute_fi[0];

  wire fi_last         = (fi         == n_inv - 4'd1);
  wire read_fi_last    = (read_fi    == n_inv - 4'd1);
  wire wb_fi_last      = (wb_fi      == n_inv - 4'd1);
  wire compute_fi_last = (compute_fi == n_inv - 4'd1);

  // ---- transfer sizes --------------------------------------------------------
  // The payload is the first job_k depths of the K_MAX-capacity image, in the
  // operand writer's N_WIRE-lane layout, moved as whole windows: ceil(k / 8)
  // of them (job_k_slab_reg, rounded at acceptance).  For k a multiple of 8 --
  // every k the archived bitstream ran -- that is exactly k * N_WIRE * 8 bytes;
  // for any other k the last window's padding steps come along, because the
  // window's B chunk follows its A chunk on the wire.  Legacy bring-up has no
  // descriptor (job_k_reg stays 0; a zero-beat descriptor is ignored by
  // dma_engine) and uses K_DIM.
  logic [31:0] job_k_slab_reg;
  // The fill stage moves the job it is filling (queue entry n_filled).
  wire [31:0] job_rx_bytes =
      USE_EXTERNAL_SCHEDULER ? (q_k_slab[qi_fill] * N_WIRE * 8) : (K_DIM_SLAB * N_WIRE * 8);
  wire [31:0] job_n_beats  = job_rx_bytes / (AXI_DATA_W / 8);
  wire [31:0] job_rx_words = job_rx_bytes / 4;

  // ---- read-side flags -------------------------------------------------------
  logic desc_valid;              // level: this context wants the read engine
  logic desc_started;
  logic read_active;
  logic read_pipeline_started;
  logic reads_complete;
  logic read_done_fold;
  logic compute_ready;

  // ---- compute-side flags ----------------------------------------------------
  logic fsm_fold_start;
  logic c_done;
  logic c_done_fold;
  logic c_done_seen;
  logic c_done_armed;
  logic cyc_running;

  // ---- write-back stage ------------------------------------------------------
  typedef enum logic [1:0] { W_IDLE, W_SCAN, W_WB } wb_phase_t;
  wb_phase_t wb_phase;
  logic      wb_bank;
  logic      wb_desc_started;
  logic [2*LANE_W:0] scan_c;
  wire scan_c_last = (scan_c == (2*LANE_W+1)'(C_N));

  // The accelerator consumes exactly one start.  External: the adapter's
  // pulse for this array.  Legacy: the FSM's own.
  wire fold_start = USE_EXTERNAL_SCHEDULER ? fold_start_ext : fsm_fold_start;

  // =========================================================================
  // Job lifetime
  // =========================================================================
  // External: one job per tile; it is complete when its own write-back
  // returns (the write-back stage runs the jobs in order).  job_c_base_reg is
  // the c_base of the job being written back / completed -- what the top's
  // readback takes at job_done -- and job_start_cycle_reg / job_est_cycles_reg
  // are the next request's schedule (queue entry n_req), what the scheduler
  // latches at tile_ack.  job_id_reg / job_k_reg are the most recently
  // accepted job's, for status.
  generate
  if (USE_EXTERNAL_SCHEDULER) begin : JOB_PIPE
    assign job_done         = job_active && wb_done && wb_pending;
    assign job_accept_ready = !q_full;

    always_ff @(posedge clk or negedge ui_rst_n) begin
      if (!ui_rst_n) begin
        n_accepted        <= 8'd0;
        job_device_id_reg <= DEVICE_ID;
        job_k_reg         <= 32'd0;
        job_k_slab_reg    <= 32'd0;
        job_a_base_reg    <= 64'd0;
        job_b_base_reg    <= 64'd0;
        for (int e = 0; e < JOB_QUEUE_DEPTH; e++) begin
          q_k[e]           <= 32'd0;
          q_k_slab[e]      <= 32'd0;
          q_start_cycle[e] <= 32'd0;
          q_est_cycles[e]  <= 32'd0;
          q_id[e]          <= 32'd0;
          q_a_base[e]      <= '0;
          q_c_base[e]      <= '0;
        end
      end else if (job_fire) begin
        // Acceptance is the job boundary: the top only fires into a queue
        // with room (job_accept_ready).
        n_accepted             <= n_accepted + 8'd1;
        q_k[qi_acc]            <= in_k;
        q_k_slab[qi_acc]       <= (in_k + 32'(WIN_K - 1)) & ~32'(WIN_K - 1);
        q_start_cycle[qi_acc]  <= in_start_cycle;
        q_est_cycles[qi_acc]   <= in_est_cycles;
        q_id[qi_acc]           <= in_job_id;
        q_a_base[qi_acc]       <= AXI_ADDR_W'(in_a_base);
        q_c_base[qi_acc]       <= AXI_ADDR_W'(in_c_base);
        job_device_id_reg      <= in_device_id;
        job_k_reg              <= in_k;
        job_k_slab_reg         <= (in_k + 32'(WIN_K - 1)) & ~32'(WIN_K - 1);
        job_a_base_reg         <= in_a_base;
        job_b_base_reg         <= in_b_base;
      end
    end

    always_comb begin
      job_busy            = (q_occupancy != 8'd0);
      job_active          = (q_occupancy != 8'd0);
      job_start_cycle_reg = q_start_cycle[qi_req];
      job_est_cycles_reg  = q_est_cycles[qi_req];
      // The job being written back / completed: what the top's readback and
      // the benches sample at job_done.  Idle, the last completed job's.
      job_c_base_reg      = 64'(wb_pending ? q_c_base[qi_wb] : last_c_base);
      job_id_reg          = wb_pending ? q_id[qi_wb] : last_id;
    end

    always_ff @(posedge clk or negedge ui_rst_n) begin
      if (!ui_rst_n) begin
        last_c_base <= '0;
        last_id     <= 32'd0;
      end else if (job_done) begin
        last_c_base <= q_c_base[qi_wb];
        last_id     <= q_id[qi_wb];
      end
    end

`ifndef SYNTHESIS
    always_ff @(posedge clk) begin
      if (ui_rst_n && job_fire && q_full)
        $fatal(1, "context %0d: job %0d accepted into a full queue", DEVICE_ID, in_job_id);
    end
`endif
  end else begin : JOB_LEGACY
    assign job_done =
        USE_EXTERNAL_SCHEDULER && job_active && wb_done && wb_fi_last;
    assign job_accept_ready = 1'b0;

    always_ff @(posedge clk or negedge ui_rst_n) begin
      if (!ui_rst_n) begin
        job_busy            <= 1'b0;
        job_active          <= 1'b0;
        job_id_reg          <= 32'd0;
        job_device_id_reg   <= DEVICE_ID;
        job_k_reg           <= 32'd0;
        job_k_slab_reg      <= 32'd0;
        job_start_cycle_reg <= 32'd0;
        job_est_cycles_reg  <= 32'd0;
        job_a_base_reg      <= 64'd0;
        job_b_base_reg      <= 64'd0;
        job_c_base_reg      <= 64'd0;
      end else begin
        if (job_fire) begin
          // Acceptance is the job boundary.
          job_busy            <= 1'b1;
          job_active          <= 1'b1;
          job_id_reg          <= in_job_id;
          job_device_id_reg   <= in_device_id;
          job_k_reg           <= in_k;
          job_k_slab_reg      <= (in_k + 32'(WIN_K - 1)) & ~32'(WIN_K - 1);
          job_start_cycle_reg <= in_start_cycle;
          job_est_cycles_reg  <= in_est_cycles;
          job_a_base_reg      <= in_a_base;
          job_b_base_reg      <= in_b_base;
          job_c_base_reg      <= in_c_base;
        end
        // Completion closes exactly the job that was accepted.
        if (job_done) begin
          job_busy   <= 1'b0;
          job_active <= 1'b0;
        end
      end
    end

    // The queue is not used on this path; keep its counters quiet.
    always_comb begin
      n_accepted = 8'd0;
    end
  end
  endgenerate

  // Scheduler-facing completion is a one-cycle event per invocation: the
  // array's publish pulse while a job is in flight (the array publishes one
  // result per fold, in order).  The legacy arming below is kept for the
  // single-fold path.
  assign scheduler_c_done =
      USE_EXTERNAL_SCHEDULER && job_active && c_valid_out;

  always_ff @(posedge clk or negedge ui_rst_n) begin
    if (!ui_rst_n) begin
      c_done_seen  <= 1'b0;
      c_done_armed <= 1'b0;
    end else if (job_fire) begin
      c_done_seen  <= 1'b0;
      c_done_armed <= 1'b0;
    end else if (fold_start) begin
      c_done_seen  <= 1'b0;
      c_done_armed <= 1'b1;
    end else if (job_active && c_done_armed && c_done && !c_done_seen) begin
      c_done_seen  <= 1'b1;
      c_done_armed <= 1'b0;
    end
  end

  // =========================================================================
  // Scheduler handshake: one request per tile.
  //
  // The job's first tile is requested at acceptance, every later tile when
  // the previous one is handed to the write-back stage.  The request is a
  // level, held until the scheduler's context for this array takes it, so a
  // release can never be dropped.  It is not cleared by run_clear: the first
  // request is raised while the FSM is still passing through P_CALIB.
  // =========================================================================
  wire tile_handoff;

  // External: one request per job, in job order.  The request for job n_req
  // is raised as soon as that job is in the queue and the previous request's
  // release has been consumed (fold_start seen), so the scheduler slot holds
  // at most one request of this context and latches that job's start_cycle /
  // est_cycles at tile_ack.  The release itself waits for accelerator_ready.
  logic req_inflight;
  generate
  if (USE_EXTERNAL_SCHEDULER) begin : REQ_PIPE
    always_ff @(posedge clk or negedge ui_rst_n) begin
      if (!ui_rst_n) begin
        tile_req     <= 1'b0;
        req_inflight <= 1'b0;
        n_req        <= 8'd0;
      end else begin
        if (tile_req && tile_ack) begin
          tile_req     <= 1'b0;
          req_inflight <= 1'b1;
          n_req        <= n_req + 8'd1;
        end else if (!tile_req && !req_inflight && (n_req != n_accepted)) begin
          tile_req <= 1'b1;
        end
        if (fold_start)
          req_inflight <= 1'b0;
      end
    end
  end else begin : REQ_LEGACY
    always_ff @(posedge clk or negedge ui_rst_n) begin
      if (!ui_rst_n)
        tile_req <= 1'b0;
      else if (USE_EXTERNAL_SCHEDULER &&
               (job_fire || (job_active && tile_handoff && !compute_fi_last)))
        tile_req <= 1'b1;
      else if (tile_req && tile_ack)
        tile_req <= 1'b0;
    end
    always_comb begin
      req_inflight = 1'b0;
      n_req        = 8'd0;
    end
  end
  endgenerate

  // The release: legacy, only once the FSM is in P_GO with a landed tile.
  // External, the next job's operands have landed, the array input has been
  // idle long enough for every PE to see the boundary, and the result bank
  // the job will publish into is free (feed_gap_ok / result_bank_free, from
  // the compute core below).
  wire feed_gap_ok;
  wire compute_idle;
  // A landed tile is offered one registered cycle after it lands, where the
  // legacy path offered it (fill_complete -> compute_ready -> P_GO): an
  // isolated job's fold starts exactly where it did, so the calibrated read
  // term (c_read = 11, descriptor + first beat + release) is unchanged.
  logic tile_waiting_d;
  always_ff @(posedge clk or negedge ui_rst_n) begin
    if (!ui_rst_n) tile_waiting_d <= 1'b0;
    else           tile_waiting_d <= tile_waiting;
  end
  assign accelerator_ready =
      USE_EXTERNAL_SCHEDULER
          ? (tile_waiting && tile_waiting_d && compute_idle && feed_gap_ok &&
             result_bank_free && array_slot_free)
          : ((phase == P_GO) && compute_ready);

  // =========================================================================
  // Addresses
  //
  // An external descriptor names exactly ONE operand payload (A and B of depth
  // job_k at job_a_base); n_inv repeats that tile transaction, every
  // invocation re-reading the same payload and writing its own result tile.
  // Legacy bring-up seeds n_inv slabs and reads slab fi.
  // =========================================================================
  // Slab i starts i * RX_BYTES on.  A multiplication by the constant slab size,
  // not a shift: RX_BYTES is a power of two only when SLAB_K is (for those it
  // synthesizes to the same wiring as the shift it replaces).
  assign seed_slab_addr =
      AXI_ADDR_W'(BASE_ADDR) + AXI_ADDR_W'(fi) * AXI_ADDR_W'(RX_BYTES);

  wire [AXI_ADDR_W-1:0] read_slab_addr =
      USE_EXTERNAL_SCHEDULER
          ? q_a_base[qi_fill]
          : AXI_ADDR_W'(BASE_ADDR) + AXI_ADDR_W'(read_fi) * AXI_ADDR_W'(RX_BYTES);

  assign wb_region_base =
      USE_EXTERNAL_SCHEDULER
          ? AXI_ADDR_W'(job_c_base_reg)
          : AXI_ADDR_W'(BASE_ADDR) + AXI_ADDR_W'(n_inv) * AXI_ADDR_W'(RX_BYTES) +
            AXI_ADDR_W'(WB_GAP_BYTES);

  // Result tile stride is this array's own tile size.  External: one tile per
  // job, at that job's own c_base.
  assign wb_desc_addr  = USE_EXTERNAL_SCHEDULER
                             ? wb_region_base
                             : wb_region_base + (AXI_ADDR_W'(wb_fi) << WBT_SHIFT);
  assign wb_desc_beats = BEAT_W'(WB_BEATS);

  // =========================================================================
  // Operand fill
  // =========================================================================
  assign op_desc_valid = desc_valid;
  assign op_desc_addr  = read_slab_addr;
  assign op_desc_beats = BEAT_W'(job_n_beats);

  // P_READ ends when the last word has LANDED, not when the last beat has been
  // received: the v1 writer takes four cycles to unpack a beat.  words_written
  // is cumulative over the run, so the finishing line moves one slab per
  // invocation; a slab is what the descriptor fetched.
  //
  // words_want is a multiply (it maps to DSP48s), so it is registered rather
  // than sitting in series with the 32-bit compare and fill_complete's fan-out.
  // The register lags its inputs by one cycle, and that cycle is never
  // observed: read_fi changes only on fill_complete or run_clear, job_k_reg
  // only on job_fire, and in every one of those cases read_done_fold is 0 in
  // the following cycle (fill_complete and run_clear clear it; at job_fire
  // the previous fill has long completed and no new descriptor has been
  // accepted yet).  read_done_fold can only be set again by a later
  // descriptor's read_done, by which time the register holds the new value.
  //
  // External: every job has its own k, so the writer's count is cleared when
  // a job's descriptor is taken (the previous fill has completely landed by
  // then: fill_complete) and the finishing line is that job's own words.
  logic [31:0] words_want;
  wire         desc_accept = desc_valid && op_desc_ready;
  generate
  if (USE_EXTERNAL_SCHEDULER) begin : WANT_PIPE
    always_ff @(posedge clk or negedge ui_rst_n) begin
      if (!ui_rst_n)        words_want <= 32'd0;
      else if (desc_accept) words_want <= job_rx_words;
    end
  end else begin : WANT_LEGACY
    always_ff @(posedge clk or negedge ui_rst_n) begin
      if (!ui_rst_n) words_want <= 32'd0;
      else           words_want <= (32'(read_fi) + 32'd1) * job_rx_words;
    end
  end
  endgenerate
  wire fill_complete = read_done_fold && (words_written == words_want);
  // What clears the operand writer's word count: the run boundary (legacy)
  // or each descriptor (external).
  wire wr_clear = USE_EXTERNAL_SCHEDULER ? desc_accept : run_clear;

  // =========================================================================
  // Compute / write-back overlap
  //
  // The outer FSM owns the compute side only (P_READ / P_GO / P_FOLD).  The
  // scan and the write-back of a finished tile run in wb_phase, so tile N's
  // fold overlaps tile N-1's scan + write-back.  P_FOLD may leave only when
  // the stage is idle, so it holds at most one tile and two result banks
  // suffice.  For the last tile the FSM waits in P_WB for the stage to drain,
  // so P_DONE still means "every tile of this run is in memory".
  // =========================================================================
  assign tile_handoff = (phase == P_FOLD) && !fold_start && c_done_fold &&
                        (wb_phase == W_IDLE);

  generate
  if (USE_EXTERNAL_SCHEDULER) begin : CTL_PIPE
    // -----------------------------------------------------------------------
    // The pipelined control.  No phase FSM: the three stages run on the
    // counters, and `phase` is derived from them in the old encoding for the
    // benches and the board scripts (P_CALIB until memory is up; P_FOLD while
    // the array is feeding or has a result outstanding; P_GO while a landed
    // tile waits for its release; P_READ while a fill is in flight or queued;
    // P_WB while only a write-back is left; P_DONE when idle).  run_clear
    // (= P_CALIB) therefore happens once, at power-up, and never between
    // jobs: the counters this file keeps per run (chk_wr, want_wr, chk_c,
    // want_c, fill_cycles, ...) now accumulate over every job of the context.
    // -----------------------------------------------------------------------
    wire compute_outstanding = (n_fed != n_cv);
    always_comb begin
      if (!init_calib_complete)                           phase = P_CALIB;
      else if (!compute_idle || compute_outstanding)      phase = P_FOLD;
      else if (tile_waiting)                              phase = P_GO;
      else if (fill_pending || read_active)               phase = P_READ;
      else if (wb_pending || (wb_phase != W_IDLE))        phase = P_WB;
      else                                                phase = P_DONE;
      n_inv          = 4'd1;
      fi             = 4'd0;
      seed_start     = 1'b0;
      fsm_fold_start = 1'b0;
    end

    // The fill stage: one descriptor at a time, for the next unfilled job,
    // once memory is up and no completely filled tile is waiting -- the
    // one-tile-waiting rule the legacy path has, which keeps the ping-pong
    // buffer being written (n_filled % 2) distinct from the one being fed
    // (n_fed % 2 = (n_filled - 1) % 2 while a tile waits).
    //
    // fill_ok is registered once before it raises desc_valid so that an
    // isolated job's descriptor goes out exactly where it did on the legacy
    // path (job_fire -> P_READ -> desc_valid: granted three cycles after
    // acceptance), which is what the cost model's c_read = 11 was calibrated
    // on; the overlap comes from WHEN the rule lets the fill start, not from
    // shaving the latency of the request.
    logic fill_ok_d;
    wire  fill_ok = init_calib_complete && fill_pending && !tile_waiting &&
                    !desc_started && !read_active;
    always_ff @(posedge clk or negedge ui_rst_n) begin
      if (!ui_rst_n) begin
        fill_ok_d  <= 1'b0;
        desc_valid <= 1'b0;
      end else begin
        fill_ok_d  <= fill_ok;
        desc_valid <= fill_ok && fill_ok_d;
      end
    end
  end else begin : CTL_LEGACY
  always_ff @(posedge clk or negedge ui_rst_n) begin
    if (!ui_rst_n) begin
      phase          <= P_CALIB;
      n_inv          <= 4'd1;
      fi             <= 4'd0;
      seed_start     <= 1'b0;
      desc_valid     <= 1'b0;
      fsm_fold_start <= 1'b0;
    end else begin
      seed_start     <= 1'b0;
      desc_valid     <= 1'b0;
      fsm_fold_start <= 1'b0;
      if (wb_done && !wb_fi_last) fi <= fi + 4'd1;
      case (phase)
        // A run begins once memory is alive and -- for an external job -- a
        // descriptor has been accepted into this context.  Only the legacy
        // context ever seeds.
        P_CALIB: if (init_calib_complete &&
                     (USE_EXTERNAL_SCHEDULER ? job_active : LEGACY_RUN)) begin
                   n_inv <= n_inv_next;
                   fi    <= 4'd0;
                   if (USE_EXTERNAL_SCHEDULER) begin
                     phase <= P_READ;
                   end else begin
                     seed_start <= 1'b1;
                     phase      <= P_SEED;
                   end
                 end
        // One seed pass per slab; seed_start is registered so the next pulse
        // lands the cycle after seed_done, when the seeder is idle again.
        P_SEED:  if (seed_done) begin
                   if (fi_last) begin
                     fi    <= 4'd0;
                     phase <= P_READ;
                   end else begin
                     fi         <= fi + 4'd1;
                     seed_start <= 1'b1;
                   end
                 end
        // A completely landed operand tile is waiting for compute.
        P_READ:  if (compute_ready) phase <= P_GO;
        // External: the scheduler releases this array through its adapter;
        // the release arrives only while we sit here (accelerator_ready).
        // Legacy: the FSM releases itself.
        P_GO:    if (USE_EXTERNAL_SCHEDULER) begin
                   if (fold_start) phase <= P_FOLD;
                 end else begin
                   fsm_fold_start <= 1'b1;
                   phase          <= P_FOLD;
                 end
        // fold_start is the transaction boundary and clears completion state,
        // so a stale c_done_fold is never consumed on that same edge.
        P_FOLD:  if (tile_handoff) begin
                   if (compute_fi_last)
                     phase <= P_WB;
                   else if (compute_ready || fill_complete)
                     phase <= P_GO;
                   else
                     phase <= P_READ;
                 end
        P_WB:    if (wb_phase == W_IDLE) phase <= P_DONE;
        // Legacy bring-up may request another run; an external job accepted
        // into this context is also a new run boundary.
        P_DONE:  if ((LEGACY_RUN && !USE_EXTERNAL_SCHEDULER && rerun_pulse) ||
                     (USE_EXTERNAL_SCHEDULER && job_fire)) begin
                   fi    <= 4'd0;
                   phase <= P_CALIB;
                 end
        default: ;
      endcase

      // After the first read phase, operand filling runs independently of the
      // compute phase: keep at most one completely filled tile waiting, and
      // stop for good after the final fill.
      if ((phase == P_READ || read_pipeline_started) &&
          !desc_started && !read_active && !compute_ready && !reads_complete)
        desc_valid <= 1'b1;
    end
  end
  end
  endgenerate

  // ---- phase trace: one line per transition --------------------------------
  phase_t phase_prev;
  always_ff @(posedge clk or negedge ui_rst_n) begin
    if (!ui_rst_n) phase_prev <= P_CALIB;
    else if (phase != phase_prev) begin
      $display("FSMTRANS t=%0t dev=%0d %0d->%0d fi=%0d | fill=%0b c_done_fold=%0b wb_phase=%0d wb_done=%0b",
               $time, DEVICE_ID, phase_prev, phase, fi, fill_complete, c_done_fold,
               wb_phase, wb_done);
      phase_prev <= phase;
    end
  end

  // ---- write-back stage ------------------------------------------------------
  // External: result n_wb (bank n_wb % 2) as soon as the array has published
  // it, in order; wb_done completes job n_wb.  Legacy: the tile handed off
  // from P_FOLD, as before.
  logic wb_pending_d;
  generate
  if (USE_EXTERNAL_SCHEDULER) begin : WB_PIPE
    always_ff @(posedge clk or negedge ui_rst_n) begin
      if (!ui_rst_n) begin
        wb_phase      <= W_IDLE;
        wb_bank       <= 1'b0;
        wb_fi         <= 4'd0;
        scan_c        <= '0;
        wb_desc_valid <= 1'b0;
        wb_pending_d  <= 1'b0;
        n_wb          <= 8'd0;
      end else begin
        wb_desc_valid <= 1'b0;
        wb_pending_d  <= wb_pending && (wb_phase == W_IDLE);
        case (wb_phase)
          // One registered cycle after the result is published, where the
          // legacy hand-off (c_done -> c_done_fold -> tile_handoff) started
          // the scan, so the write term stays the calibrated c_write; and the
          // same registered cycle after a previous write-back's wb_done, so
          // back-to-back write-backs are spaced exactly one write term apart
          // (the cost model's writeEnd_j = max(computeEnd_j, writeEnd_j-1) +
          // T_write).
          W_IDLE: if (wb_pending && wb_pending_d) begin
                    wb_bank  <= n_wb[0];
                    scan_c   <= '0;
                    wb_phase <= W_SCAN;
                  end
          W_SCAN: if (scan_c_last) wb_phase <= W_WB;
                  else             scan_c   <= scan_c + 1'b1;
          W_WB:   begin
                    if (!wb_desc_started) wb_desc_valid <= 1'b1;
                    if (wb_done) begin
                      n_wb     <= n_wb + 8'd1;
                      wb_phase <= W_IDLE;
                    end
                  end
          default: wb_phase <= W_IDLE;
        endcase
      end
    end
  end else begin : WB_LEGACY
  always_ff @(posedge clk or negedge ui_rst_n) begin
    if (!ui_rst_n || run_clear) begin
      wb_phase      <= W_IDLE;
      wb_bank       <= 1'b0;
      wb_fi         <= 4'd0;
      scan_c        <= '0;
      wb_desc_valid <= 1'b0;
    end else begin
      wb_desc_valid <= 1'b0;
      case (wb_phase)
        W_IDLE: if (tile_handoff) begin
                  wb_bank  <= compute_fi[0];
                  scan_c   <= '0;
                  wb_phase <= W_SCAN;
                end
        W_SCAN: if (scan_c_last) wb_phase <= W_WB;
                else             scan_c   <= scan_c + 1'b1;
        W_WB:   begin
                  if (!wb_desc_started) wb_desc_valid <= 1'b1;
                  if (wb_done) begin
                    if (!wb_fi_last) wb_fi <= wb_fi + 4'd1;
                    wb_phase <= W_IDLE;
                  end
                end
        default: wb_phase <= W_IDLE;
      endcase
    end
  end
  always_comb begin
    n_wb         = 8'd0;
    wb_pending_d = 1'b0;
  end
  end
  endgenerate

  assign wb_active = (wb_phase == W_WB);

  always_ff @(posedge clk or negedge ui_rst_n) begin
    if (!ui_rst_n)                           wb_desc_started <= 1'b0;
    else if (wb_phase != W_WB)               wb_desc_started <= 1'b0;
    else if (wb_desc_valid && wb_desc_ready) wb_desc_started <= 1'b1;
  end

  always_ff @(posedge clk) begin
    if (ui_rst_n && wb_desc_valid && wb_desc_ready)
      $display("WB_FIRE_DBG t=%0t dev=%0d job=%0d wb_fi=%0d tile_addr=0x%08h beats=%0d",
               $time, DEVICE_ID, job_id_reg, wb_fi, wb_desc_addr, wb_desc_beats);
  end

  // ---- read pipeline flags ---------------------------------------------------
  generate
  if (USE_EXTERNAL_SCHEDULER) begin : FLAGS_PIPE
    // The stage counters: a landed tile, a started fold.  (desc_started,
    // read_active and read_done_fold below are shared with the legacy path.)
    always_ff @(posedge clk or negedge ui_rst_n) begin
      if (!ui_rst_n)          n_filled <= 8'd0;
      else if (fill_complete) n_filled <= n_filled + 8'd1;
    end
    always_ff @(posedge clk or negedge ui_rst_n) begin
      if (!ui_rst_n)       n_fed <= 8'd0;
      else if (fold_start) n_fed <= n_fed + 8'd1;
    end
    always_comb begin
      read_pipeline_started = 1'b1;
      reads_complete        = 1'b0;
      read_fi               = 4'd0;
      compute_fi            = 4'd0;
      compute_ready         = tile_waiting;
      filled_fi             = 4'd0;
    end
`ifndef SYNTHESIS
    // The one-tile-waiting rule and the result-bank rule, checked as they
    // happen: a fold must find a landed tile and a free bank; a fill must
    // not start into the buffer being fed.
    always_ff @(posedge clk) begin
      if (ui_rst_n && fold_start && (!tile_waiting || !result_bank_free || !array_slot_free))
        $fatal(1, "context %0d: fold %0d released without a landed tile, a free result bank or a free accumulator set",
               DEVICE_ID, n_fed);
      // The fill writes buffer n_filled % 2; the feed in progress reads
      // buffer (n_fed - 1) % 2.
      if (ui_rst_n && desc_accept && (n_filled[0] != n_fed[0]) && !compute_idle)
        $fatal(1, "context %0d: fill %0d into the operand buffer being fed", DEVICE_ID, n_filled);
    end
`endif
  end else begin : FLAGS_LEGACY
  always_ff @(posedge clk or negedge ui_rst_n) begin
    if (!ui_rst_n || run_clear) read_pipeline_started <= 1'b0;
    else if (phase == P_READ)   read_pipeline_started <= 1'b1;
  end

  always_ff @(posedge clk or negedge ui_rst_n) begin
    if (!ui_rst_n || run_clear)              reads_complete <= 1'b0;
    else if (fill_complete && read_fi_last)  reads_complete <= 1'b1;
  end

  always_ff @(posedge clk or negedge ui_rst_n) begin
    if (!ui_rst_n || run_clear)              read_fi <= 4'd0;
    else if (fill_complete && !read_fi_last) read_fi <= read_fi + 4'd1;
  end

  // Compute takes ownership only when the array actually starts.
  always_ff @(posedge clk or negedge ui_rst_n) begin
    if (!ui_rst_n || run_clear) compute_fi <= 4'd0;
    else if (fold_start)        compute_fi <= filled_fi;
  end

  always_ff @(posedge clk or negedge ui_rst_n) begin
    if (!ui_rst_n || run_clear) begin
      compute_ready <= 1'b0;
      filled_fi     <= 4'd0;
    end else if (fill_complete) begin
      compute_ready <= 1'b1;
      filled_fi     <= read_fi;
    end else if (fold_start) begin
      compute_ready <= 1'b0;
    end
  end
  always_comb begin
    n_filled = 8'd0;
    n_fed    = 8'd0;
  end
  end
  endgenerate

  always_ff @(posedge clk or negedge ui_rst_n) begin
    if (!ui_rst_n || run_clear)             desc_started <= 1'b0;
    else if (fill_complete)                 desc_started <= 1'b0;
    else if (desc_valid && op_desc_ready)   desc_started <= 1'b1;
  end

  always_ff @(posedge clk or negedge ui_rst_n) begin
    if (!ui_rst_n || run_clear)             read_active <= 1'b0;
    else if (fill_complete)                 read_active <= 1'b0;
    else if (desc_valid && op_desc_ready)   read_active <= 1'b1;
  end

  always_ff @(posedge clk or negedge ui_rst_n) begin
    if (!ui_rst_n || run_clear)           read_done_fold <= 1'b0;
    else if (fill_complete)               read_done_fold <= 1'b0;
    else if (desc_valid && op_desc_ready) read_done_fold <= 1'b0;
    else if (read_done)                   read_done_fold <= 1'b1;
  end

  // c_done is a LEVEL (high from publish until the next fold_start); this
  // latch holds only the completion that belongs to the invocation in flight.
  always_ff @(posedge clk or negedge ui_rst_n) begin
    if (!ui_rst_n)                          c_done_fold <= 1'b0;
    else if ((phase == P_GO) || fold_start) c_done_fold <= 1'b0;
    else if (c_done)                        c_done_fold <= 1'b1;
  end

  // ---- sticky run flags ------------------------------------------------------
  always_ff @(posedge clk or negedge ui_rst_n) begin
    if (!ui_rst_n || run_clear) begin
      seed_done_sticky <= 1'b0;
      read_done_sticky <= 1'b0;
      fold_done_sticky <= 1'b0;
      wb_done_sticky   <= 1'b0;
    end else begin
      if (seed_done) seed_done_sticky <= 1'b1;
      if (read_done) read_done_sticky <= 1'b1;
      if (c_done)    fold_done_sticky <= 1'b1;
      if (wb_done)   wb_done_sticky   <= 1'b1;
    end
  end

  // ---- fill / write-back / run counters ---------------------------------------
  logic fill_running, wb_running, t_running;

  always_ff @(posedge clk or negedge ui_rst_n) begin
    if (!ui_rst_n || run_clear) begin
      fill_cycles  <= '0;  fill_running <= 1'b0;
      wb_cycles    <= '0;  wb_running   <= 1'b0;
    end else begin
      if (desc_valid && op_desc_ready) begin
        fill_running <= 1'b1;
        fill_cycles  <= fill_cycles + 32'd1;
      end else if (fill_running) begin
        fill_cycles <= fill_cycles + 1'b1;
        if (fill_complete) fill_running <= 1'b0;
      end
      if (wb_desc_valid && wb_desc_ready) begin
        wb_running <= 1'b1;
        wb_cycles  <= wb_cycles + 32'd1;
      end else if (wb_running) begin
        wb_cycles <= wb_cycles + 1'b1;
        if (wb_done) wb_running <= 1'b0;
      end
    end
  end

  always_ff @(posedge clk or negedge ui_rst_n) begin
    if (!ui_rst_n || run_clear) begin
      t_span     <= '0;
      t_running  <= 1'b0;
      folds_done <= '0;
    end else begin
      if (desc_valid && op_desc_ready && !t_running && (t_span == 32'd0)) begin
        t_running <= 1'b1;
        t_span    <= 32'd1;
      end else if (t_running) begin
        t_span <= t_span + 1'b1;
        if (wb_done && wb_fi_last) t_running <= 1'b0;
      end
      if (wb_done) folds_done <= folds_done + 8'd1;
    end
  end

  // =========================================================================
  // Operand writer + checksum 1 + operand memories (this context's own)
  //
  // The writer decodes the N_WIRE-lane payload exactly as before.  An array
  // narrower than the wire keeps only the banks it has: writes to lanes
  // >= N_ARRAY are dropped at the buffer, while words_written and chk_wr still
  // count every word, so the tabulated constants are unchanged.
  // =========================================================================
  logic [K_W-1:0] a_raddr [0:N_ARRAY-1];
  logic [K_W-1:0] b_raddr [0:N_ARRAY-1];
  wire  [31:0]    a_rdata [0:N_ARRAY-1];
  wire  [31:0]    b_rdata [0:N_ARRAY-1];

  generate
  if (!USE_V2) begin : OP_V1
    wire                a_wr, b_wr;
    wire [LANE_W_W-1:0] wsel;
    wire [SLAB_W-1:0]   waddr_slab;   // step within the slab, 0..SLAB_K-1
    wire [31:0]         wdata_buf;

    dma_operand_writer #(
      .N (N_WIRE), .K_MAX (K_MAX), .AXI_DATA_W (AXI_DATA_W), .BEAT_W (BEAT_W)
    ) u_wr (
      .clk (clk), .rst_n (ui_rst_n),
      .dst_wr_en (dst_wr_en), .dst_wr_beat (dst_wr_beat), .dst_wr_data (dst_wr_data),
      .dst_full (dst_full), .dst_almost_full (dst_almost_full),
      .a_wr (a_wr), .b_wr (b_wr), .wsel (wsel), .waddr (waddr_slab), .wdata (wdata_buf),
      .words_written (words_written), .err_range (wr_err_range), .clear (wr_clear)
    );

    wire lane_in_array = (N_ARRAY >= N_WIRE) ? 1'b1 : (wsel < LANE_W_W'(N_ARRAY));
    wire [LANE_W-1:0] wsel_arr = wsel[LANE_W-1:0];

    // Slab steps >= K_MAX are the last window's padding: dropped here, like
    // the lanes this array does not have.  The compare is on the full slab
    // step (truncating first could alias a padding step onto a real one), and
    // it is constant-true whenever K_MAX is a multiple of 8.
    wire depth_in_array = (int'(waddr_slab) < K_MAX);
    wire [K_W-1:0] waddr = K_W'(waddr_slab);
    wire buf_wr_ok = lane_in_array && depth_in_array;

    wire [15:0] k16   = 16'(waddr_slab);
    wire [7:0]  bank8 = 8'(wsel);
    wire [31:0] wpos  = {8'd0, bank8, k16};

    logic [31:0] chk_wr_q;
    always_ff @(posedge clk) begin
      if (run_clear)        chk_wr_q <= '0;
      else if (a_wr)        chk_wr_q <= chk_wr_q + (wdata_buf ^ wpos);
      else if (b_wr)        chk_wr_q <= chk_wr_q + (wdata_buf ^ (32'h8000_0000 | wpos));
    end
    assign chk_wr = chk_wr_q;

    wire [31:0] a_rdata_0 [0:N_ARRAY-1];
    wire [31:0] a_rdata_1 [0:N_ARRAY-1];
    wire [31:0] b_rdata_0 [0:N_ARRAY-1];
    wire [31:0] b_rdata_1 [0:N_ARRAY-1];

    systolic_operand_buffer #(.K_MAX (K_MAX), .K_W (K_W), .N_BANKS (N_ARRAY)) u_a_buf_0 (
      .clk (clk), .wr (a_wr && buf_wr_ok && !fill_bank),
      .wsel (wsel_arr), .waddr (waddr), .wdata (wdata_buf), .raddr (a_raddr), .rdata (a_rdata_0));
    systolic_operand_buffer #(.K_MAX (K_MAX), .K_W (K_W), .N_BANKS (N_ARRAY)) u_a_buf_1 (
      .clk (clk), .wr (a_wr && buf_wr_ok &&  fill_bank),
      .wsel (wsel_arr), .waddr (waddr), .wdata (wdata_buf), .raddr (a_raddr), .rdata (a_rdata_1));
    systolic_operand_buffer #(.K_MAX (K_MAX), .K_W (K_W), .N_BANKS (N_ARRAY)) u_b_buf_0 (
      .clk (clk), .wr (b_wr && buf_wr_ok && !fill_bank),
      .wsel (wsel_arr), .waddr (waddr), .wdata (wdata_buf), .raddr (b_raddr), .rdata (b_rdata_0));
    systolic_operand_buffer #(.K_MAX (K_MAX), .K_W (K_W), .N_BANKS (N_ARRAY)) u_b_buf_1 (
      .clk (clk), .wr (b_wr && buf_wr_ok &&  fill_bank),
      .wsel (wsel_arr), .waddr (waddr), .wdata (wdata_buf), .raddr (b_raddr), .rdata (b_rdata_1));

    for (genvar pp = 0; pp < N_ARRAY; pp = pp + 1) begin : PP_READ_MUX
      assign a_rdata[pp] = compute_bank ? a_rdata_1[pp] : a_rdata_0[pp];
      assign b_rdata[pp] = compute_bank ? b_rdata_1[pp] : b_rdata_0[pp];
    end
  end else begin : OP_V2
    // The beat-wide path writes A four steps per beat into a cyclic buffer of
    // K_MAX / 4 rows and B one step per beat, with no padding-step drop: its
    // contract is whole windows, K_MAX a multiple of 8 -- and at least two
    // of them, because dma_operand_writer_v2 takes the window index from
    // $clog2(K_MAX) - 3 address bits, which is none at K_MAX = 8.  It is not
    // generalized here (the v1 path above takes any K_MAX); refuse anything
    // else at elaboration rather than write past the buffers.
    if ((K_MAX % WIN_K) != 0 || K_MAX < 2 * WIN_K)
      $error("systolic_accel_context: USE_V2 requires K_MAX to be a multiple of 8 and at least 16 (use the v1 operand path for other K_MAX)");
    wire                  a_wr, b_wr;
    wire [LANE_W_W-1:0]   wsel;
    wire [K_W-1:0]        waddr;
    wire [AXI_DATA_W-1:0] wdata_buf;

    dma_operand_writer_v2 #(
      .N (N_WIRE), .K_MAX (K_MAX), .AXI_DATA_W (AXI_DATA_W), .BEAT_W (BEAT_W)
    ) u_wr (
      .clk (clk), .rst_n (ui_rst_n),
      .dst_wr_en (dst_wr_en), .dst_wr_beat (dst_wr_beat), .dst_wr_data (dst_wr_data),
      .dst_full (dst_full), .dst_almost_full (dst_almost_full),
      .a_wr (a_wr), .b_wr (b_wr), .wsel (wsel), .waddr (waddr), .wdata (wdata_buf),
      .words_written (words_written), .err_range (wr_err_range), .clear (wr_clear)
    );

    dma_wr_checksum_v2 #(.N (N_WIRE), .K_MAX (K_MAX), .AXI_DATA_W (AXI_DATA_W)) u_chk_wr (
      .clk (clk), .rst_n (ui_rst_n), .clear (run_clear),
      .a_wr (a_wr), .b_wr (b_wr), .wsel (wsel), .waddr (waddr), .wdata (wdata_buf),
      .chk (chk_wr));

    // The same ping-pong as v1, with the beat-wide buffers.  A B beat carries
    // four lanes of one depth, so for a 4-lane array the beats of lanes 4..7
    // are the ones dropped.
    wire lane_in_array = (N_ARRAY >= N_WIRE) ? 1'b1 : (wsel < LANE_W_W'(N_ARRAY));
    wire [LANE_W-1:0] wsel_arr = wsel[LANE_W-1:0];

    wire [31:0] a_rdata_0 [0:N_ARRAY-1];
    wire [31:0] a_rdata_1 [0:N_ARRAY-1];
    wire [31:0] b_rdata_0 [0:N_ARRAY-1];
    wire [31:0] b_rdata_1 [0:N_ARRAY-1];

    systolic_operand_buffer_v2 #(.K_MAX (K_MAX), .K_W (K_W), .N_BANKS (N_ARRAY), .LAYOUT_CYCLIC (1'b1)) u_a_buf_0 (
      .clk (clk), .wr (a_wr && lane_in_array && !fill_bank),
      .wsel (wsel_arr), .waddr (waddr), .wdata (wdata_buf), .raddr (a_raddr), .rdata (a_rdata_0));
    systolic_operand_buffer_v2 #(.K_MAX (K_MAX), .K_W (K_W), .N_BANKS (N_ARRAY), .LAYOUT_CYCLIC (1'b1)) u_a_buf_1 (
      .clk (clk), .wr (a_wr && lane_in_array &&  fill_bank),
      .wsel (wsel_arr), .waddr (waddr), .wdata (wdata_buf), .raddr (a_raddr), .rdata (a_rdata_1));
    systolic_operand_buffer_v2 #(.K_MAX (K_MAX), .K_W (K_W), .N_BANKS (N_ARRAY), .LAYOUT_CYCLIC (1'b0)) u_b_buf_0 (
      .clk (clk), .wr (b_wr && lane_in_array && !fill_bank),
      .wsel (wsel_arr), .waddr (waddr), .wdata (wdata_buf), .raddr (b_raddr), .rdata (b_rdata_0));
    systolic_operand_buffer_v2 #(.K_MAX (K_MAX), .K_W (K_W), .N_BANKS (N_ARRAY), .LAYOUT_CYCLIC (1'b0)) u_b_buf_1 (
      .clk (clk), .wr (b_wr && lane_in_array &&  fill_bank),
      .wsel (wsel_arr), .waddr (waddr), .wdata (wdata_buf), .raddr (b_raddr), .rdata (b_rdata_1));

    for (genvar pp = 0; pp < N_ARRAY; pp = pp + 1) begin : PP_READ_MUX
      assign a_rdata[pp] = compute_bank ? a_rdata_1[pp] : a_rdata_0[pp];
      assign b_rdata[pp] = compute_bank ? b_rdata_1[pp] : b_rdata_0[pp];
    end
  end
  endgenerate

  // =========================================================================
  // The compute core, as copied from systolic_uart_top: feeder, control FSM,
  // C register, cycle counter.  Unchanged except that the array it feeds is
  // this context's own.
  // =========================================================================

  typedef enum logic [2:0] {
    ST_IDLE        = 3'd0,
    ST_FEED        = 3'd1,
    ST_WAIT_RESULT = 3'd2,
    ST_DONE        = 3'd3
  } state_t;
  state_t            state;
  logic [FEED_W-1:0] feed_t;

  // External: the depth of the job being fed -- the queue entry of the job
  // about to be released while the array is idle (so the depth is already on
  // k_dim in the fold_start cycle, where the benches' probes sample it), and
  // the value latched at that fold_start while the feed runs (the queue
  // entry may be overwritten by a later job meanwhile).
  logic [31:0] feed_k;
  wire [FEED_W-1:0] k_dim =
      USE_EXTERNAL_SCHEDULER
          ? ((state == ST_FEED) ? FEED_W'(feed_k) : FEED_W'(q_k[qi_fed]))
          : FEED_W'(K_DIM);

  systolic_tile_feeder #(
    .N (N_ARRAY), .K_W (K_W), .FEED_W (FEED_W), .KDIM_W (FEED_W)
  ) u_feeder (
    .clk (clk), .rst (rst_i), .enable (state == ST_FEED),
    .feed_t (feed_t), .k_dim (k_dim),
    .a_rdata (a_rdata), .b_rdata (b_rdata),
    .a_raddr (a_raddr), .b_raddr (b_raddr),
    .a_in (a_in), .b_in (b_in),
    .a_valid_in (a_valid_in), .b_valid_in (b_valid_in)
  );

  // ---- store final results ---------------------------------------------------
  // C is the live result image.  The write-back stage consumes an immutable
  // snapshot.  Legacy: tile i publishes into bank i%2 (compute_fi[0], stable
  // for the fold); the stage latches that index at tile_handoff and reads
  // only its own bank.  Bank i%2 is rewritten by tile i+2 at the earliest,
  // whose fold cannot start before tile i+1 was handed off, i.e. after tile
  // i's write-back completed.
  // External: the array publishes in fold order, so result n_cv goes to bank
  // n_cv % 2 whatever fold is in flight by then; the fold of job j is only
  // released once job j-2 -- the previous user of bank j % 2 -- has been
  // written back (result_bank_free), so the bank is never overwritten under
  // the write-back stage.
  logic [31:0] result_bank [0:1][0:N_ARRAY-1][0:N_ARRAY-1];
  wire result_bank_sel = USE_EXTERNAL_SCHEDULER ? n_cv[0] : compute_fi[0];

  integer rr, cc;
  always_ff @(posedge clk) begin
    if (rst_i) begin
      c_done <= 1'b0;
    end else begin
      if (fold_start) c_done <= 1'b0;
      if (c_valid_out) begin
        for (rr = 0; rr < N_ARRAY; rr = rr + 1)
          for (cc = 0; cc < N_ARRAY; cc = cc + 1) begin
            C[rr][cc]                          <= c_out[rr][cc];
            result_bank[result_bank_sel][rr][cc] <= c_out[rr][cc];
          end
        c_done <= 1'b1;
      end
    end
  end

  generate
  if (USE_EXTERNAL_SCHEDULER) begin : CV_PIPE
    always_ff @(posedge clk or negedge ui_rst_n) begin
      if (!ui_rst_n)        n_cv <= 8'd0;
      else if (c_valid_out) n_cv <= n_cv + 8'd1;
    end
`ifndef SYNTHESIS
    always_ff @(posedge clk) begin
      if (ui_rst_n && c_valid_out && (n_cv == n_fed))
        $fatal(1, "context %0d: the array published a result with no fold outstanding", DEVICE_ID);
      if (ui_rst_n && c_valid_out && ((n_cv - n_wb) >= 8'd2))
        $fatal(1, "context %0d: result %0d published into a bank still owned by write-back",
               DEVICE_ID, n_cv);
    end
`endif
  end else begin : CV_LEGACY
    always_comb n_cv = 8'd0;
  end
  endgenerate

  // ---- main state progression --------------------------------------------------
  // Legacy: ST_FEED -> ST_WAIT_RESULT (until the array publishes) -> ST_DONE
  // -> ST_IDLE, one fold at a time.  External: ST_FEED -> ST_IDLE as soon as
  // the last operand has been issued; the result is collected by the
  // write-back stage when the array publishes it, and the next fold is held
  // off by feed_gap_ok -- FEED_GAP idle cycles of the array input after this
  // fold's k steps -- not by the publish.  That is the overlap the PE was
  // built for: tile n_fed+1 accumulates while tile n_fed reduces.
  logic [7:0] since_feed;   // cycles since the array input went idle (saturating)
  generate
  if (USE_EXTERNAL_SCHEDULER) begin : CORE_PIPE
    // ST_FEED ends at feed_t = k + N - 2 (the last skewed boundary step),
    // i.e. N - 1 cycles after the last step of row 0; the release that
    // follows takes three cycles to reach ST_FEED (scheduler start, adapter,
    // this FSM).  So the gap PE(0, 0) sees between the two tiles is
    // (since_feed at release) + (N - 1) + 3 -- hold the release until that
    // reaches FEED_GAP.  Every other PE sees the same gap, r + c later.
    localparam integer GAP_IDLE_RAW = FEED_GAP - (N_ARRAY - 1) - 3;
    localparam integer GAP_IDLE     = (GAP_IDLE_RAW > 0) ? GAP_IDLE_RAW : 0;

    always_ff @(posedge clk) begin
      if (rst_i) begin
        state      <= ST_IDLE;
        feed_t     <= '0;
        feed_k     <= 32'd0;
        since_feed <= 8'hFF;
        compute_idx_lsb <= 1'b0;
      end else begin
        if (state == ST_FEED)       since_feed <= 8'd0;
        else if (since_feed != 8'hFF) since_feed <= since_feed + 8'd1;
        case (state)
          ST_IDLE: begin
            feed_t <= '0;
            if (fold_start) begin
              state           <= ST_FEED;
              feed_t          <= '0;
              feed_k          <= q_k[qi_fed];
              compute_idx_lsb <= n_fed[0];
            end
          end
          ST_FEED: begin
            if (feed_t == k_dim + FEED_W'(N_ARRAY - 2)) state <= ST_IDLE;
            else                                        feed_t <= feed_t + 1'b1;
          end
          default: state <= ST_IDLE;
        endcase
      end
    end
    assign compute_idle = (state == ST_IDLE);
    assign feed_gap_ok  = (since_feed >= 8'(GAP_IDLE));
  end else begin : CORE_LEGACY
  always_ff @(posedge clk) begin
    if (rst_i) begin
      state  <= ST_IDLE;
      feed_t <= '0;
    end else begin
      case (state)
        ST_IDLE: begin
          feed_t <= '0;
          if (fold_start) begin
            state  <= ST_FEED;
            feed_t <= '0;
          end
        end
        ST_FEED: begin
          if (feed_t == k_dim + FEED_W'(N_ARRAY - 2)) state <= ST_WAIT_RESULT;
          else                                        feed_t <= feed_t + 1'b1;
        end
        ST_WAIT_RESULT: if (c_done) state <= ST_DONE;
        ST_DONE:        state <= ST_IDLE;
        default:        state <= ST_IDLE;
      endcase
    end
  end
  always_comb begin
    feed_k          = 32'd0;
    since_feed      = 8'd0;
    compute_idx_lsb = 1'b0;
  end
  assign compute_idle = (state == ST_IDLE);
  assign feed_gap_ok  = 1'b1;
  end
  endgenerate

  // ---- transaction cycle counter -------------------------------------------------
  // First beat of ST_FEED to the publish pulse, both ends inclusive -- the
  // interval systolic_uart_top measures.
  logic [31:0] cyc_count;
  always_ff @(posedge clk) begin
    if (rst_i || run_clear) begin
      cyc_count   <= '0;
      cyc_latched <= '0;
      cyc_total   <= '0;
      cyc_running <= 1'b0;
    end else begin
      if (state == ST_FEED && feed_t == '0 && !cyc_running) begin
        cyc_running <= 1'b1;
        cyc_count   <= 32'd1;
      end else if (cyc_running) begin
        cyc_count <= cyc_count + 1'b1;
        if (c_valid_out) begin
          cyc_running <= 1'b0;
          cyc_latched <= cyc_count + 1'b1;
          cyc_total   <= cyc_total + cyc_count + 32'd1;
        end
      end
    end
  end

  // =========================================================================
  // Write-back source: this context's result reader over its snapshot bank,
  // through a FIFO, to the shared write-back engine.
  // =========================================================================
  wire [31:0] C_wb [0:N_ARRAY-1][0:N_ARRAY-1];
  generate
    for (genvar wr_ = 0; wr_ < N_ARRAY; wr_ = wr_ + 1) begin : GEN_CWB_R
      for (genvar wc_ = 0; wc_ < N_ARRAY; wc_ = wc_ + 1) begin : GEN_CWB_C
        assign C_wb[wr_][wc_] = result_bank[wb_bank][wr_][wc_];
      end
    end
  endgenerate

  wire                  rdr_wr_en;
  wire [AXI_DATA_W-1:0] rdr_wr_data;
  wire                  rdr_wfull;
  wire                  rdr_done;
  wire                  fifo_rempty;

  dma_result_reader #(.N (N_ARRAY), .AXI_DATA_W (AXI_DATA_W)) u_rdr (
    .clk (clk), .rst (rst_i),
    .start (wb_phase == W_WB), .done (rdr_done),
    .C (C_wb),
    .wr_en (rdr_wr_en), .wr_data (rdr_wr_data), .wfull (rdr_wfull)
  );

  dma_cdc_fifo #(.DW (AXI_DATA_W), .AW (5), .AF_MARGIN (8)) u_wb_fifo (
    .wclk (clk), .wrst_n (ui_rst_n),
    .wr_en (rdr_wr_en), .wr_data (rdr_wr_data), .wfull (rdr_wfull), .walmost_full (),
    .rclk (clk), .rrst_n (ui_rst_n),
    .rd_en (src_valid && src_ready), .rd_data (src_data), .rempty (fifo_rempty)
  );

  assign src_valid = !fifo_rempty;

  // =========================================================================
  // Checksum 2: the result matrix, one entry per cycle off the snapshot bank
  // the write-back stage owns.  The read is registered, so the index issued
  // on cycle t is accumulated on t+1.
  // =========================================================================
  wire [LANE_W-1:0] scan_r_i = scan_c[2*LANE_W-1:LANE_W];
  wire [LANE_W-1:0] scan_c_i = scan_c[LANE_W-1:0];

  logic              scan_val_d;
  logic [LANE_W-1:0] scan_r_d, scan_c_d;
  logic [31:0]       c_rd;

  wire [7:0]  c_row8  = 8'(scan_r_d);
  wire [15:0] c_col16 = 16'(scan_c_d);
  wire [31:0] cpos    = {8'd0, c_row8, c_col16};

  always_ff @(posedge clk or negedge ui_rst_n) begin
    if (!ui_rst_n) begin
      scan_val_d <= 1'b0;
      scan_r_d   <= '0;
      scan_c_d   <= '0;
      c_rd       <= '0;
      chk_c      <= '0;
    end else begin
      scan_val_d <= (wb_phase == W_SCAN) && !scan_c_last;
      scan_r_d   <= scan_r_i;
      scan_c_d   <= scan_c_i;
      c_rd       <= result_bank[wb_bank][scan_r_i][scan_c_i];
      if (run_clear)         chk_c <= '0;
      else if (scan_val_d)   chk_c <= chk_c + (c_rd ^ cpos);
    end
  end

  // Run totals: the tabulated per-invocation constant, once per completed
  // invocation.  The operand constant belongs to the job's K.
  function automatic logic [31:0] expected_wr_chk_for_k(input logic [31:0] k);
    case (k)
      32'd16:  expected_wr_chk_for_k = 32'h3F88_0780;
      32'd32:  expected_wr_chk_for_k = 32'h805C_1F00;
      default: expected_wr_chk_for_k = 32'h0;
    endcase
  endfunction

  // External: the constant belongs to the job whose fill just completed
  // (queue entry n_filled, before the counter advances).
  wire [31:0] active_expected_wr_chk =
      USE_EXTERNAL_SCHEDULER ? expected_wr_chk_for_k(q_k[qi_fill]) : EXPECT_WR_CHK;

  always_ff @(posedge clk or negedge ui_rst_n) begin
    if (!ui_rst_n) begin
      want_wr <= '0;
      want_c  <= '0;
    end else if (run_clear) begin
      want_wr <= '0;
      want_c  <= '0;
    end else begin
      if (fill_complete)                       want_wr <= want_wr + active_expected_wr_chk;
      if ((wb_phase == W_SCAN) && scan_c_last) want_c  <= want_c + EXPECT_C_CHK;
    end
  end

  assign wr_match = read_done_sticky && (want_wr != 32'd0) && (chk_wr == want_wr);
  assign c_match  = (phase == P_DONE) && (chk_c == want_c);

  // ---- elaboration-time contract (simulation and synthesis alike) ------------
  // K_MAX: any positive integer whose slab the descriptor's BEAT_W-bit beat
  // count can name.  The array edges are powers of two -- the wire's lane
  // fields are bit slices and the result tile stride is a shift -- which is a
  // property of the geometry (8 and 4), not of K_MAX.
  generate
    if (K_MAX < 1)
      $error("systolic_accel_context: K_MAX must be a positive integer");
    if (JOB_QUEUE_DEPTH < 2 || (JOB_QUEUE_DEPTH & (JOB_QUEUE_DEPTH - 1)) != 0)
      $error("systolic_accel_context: JOB_QUEUE_DEPTH must be a power of two, at least 2");
    if (PE_QUIET_GAP < 0 || FEED_GAP_MARGIN < 0)
      $error("systolic_accel_context: PE_QUIET_GAP and FEED_GAP_MARGIN must be non-negative");
    if (K_DIM < 1 || K_DIM > K_MAX)
      $error("systolic_accel_context: K_DIM must be in 1..K_MAX");
    if (RX_BYTES / (AXI_DATA_W / 8) > (1 << BEAT_W) - 1)
      $error("systolic_accel_context: the K_MAX slab has more beats than the BEAT_W-bit descriptor counts");
    if ((1 << LANE_W) != N_ARRAY || (1 << LANE_W_W) != N_WIRE)
      $error("systolic_accel_context: N_ARRAY and N_WIRE must be powers of two");
    if (N_ARRAY > N_WIRE)
      $error("systolic_accel_context: array wider than the operand wire format");
  endgenerate

`ifndef SYNTHESIS
  initial begin
    if ((1 << WBT_SHIFT) != WB_TILE_BYTES)
      $fatal(1, "context %0d: WB_TILE_BYTES is not a power of two", DEVICE_ID);
  end

  // An accepted job must fit this accelerator's own K_MAX.  systolic_dma_top
  // refuses any other descriptor before it gets here; this checks that it did.
  always_ff @(posedge clk) begin
    if (ui_rst_n && job_fire && (in_k > K_MAX || in_k < 1))
      $fatal(1, "context %0d (device %0d): job %0d k=%0d is outside 1..%0d",
             DEVICE_ID, in_device_id, in_job_id, in_k, K_MAX);
  end
`endif

endmodule

`default_nettype wire
