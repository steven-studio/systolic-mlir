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
  parameter integer BEAT_W     = 16
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
  output wire         job_done,            // one cycle: last tile's wb_done
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
  localparam integer RX_BYTES   = K_MAX * 8 * N_WIRE;    // one physical slab
  localparam integer RX_WORDS   = RX_BYTES / 4;
  localparam int     K_W        = $clog2(K_MAX);
  localparam int     LANE_W     = $clog2(N_ARRAY);
  localparam int     LANE_W_W   = $clog2(N_WIRE);
  localparam int     FEED_LAST  = K_MAX + N_ARRAY - 2;
  localparam int     FEED_W     = $clog2(FEED_LAST + 1);
  localparam int     RX_SHIFT   = $clog2(RX_BYTES);
  localparam int     RXW_SHIFT  = $clog2(RX_WORDS);
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
  assign run_clear = (phase == P_CALIB);

  // ---- job registers --------------------------------------------------------
  logic [63:0] job_a_base_reg;
  logic [63:0] job_b_base_reg;

  // ---- invocation bookkeeping ----------------------------------------------
  wire  [3:0] n_inv_next = (n_inv_probe == 4'd0) ? 4'd1 : n_inv_probe;
  logic [3:0] fi;                     // legacy serial index (seeding, debug)
  logic [3:0] read_fi;
  logic [3:0] compute_fi;
  logic [3:0] wb_fi;
  logic [3:0] filled_fi;

  wire fill_bank    = read_fi[0];
  wire compute_bank = compute_fi[0];

  wire fi_last         = (fi         == n_inv - 4'd1);
  wire read_fi_last    = (read_fi    == n_inv - 4'd1);
  wire wb_fi_last      = (wb_fi      == n_inv - 4'd1);
  wire compute_fi_last = (compute_fi == n_inv - 4'd1);

  // ---- transfer sizes --------------------------------------------------------
  // The payload is the first job_k depths of the K_MAX-capacity image, in the
  // operand writer's N_WIRE-lane layout.  Legacy bring-up has no descriptor
  // (job_k_reg stays 0; a zero-beat descriptor is ignored by dma_engine) and
  // uses K_DIM.
  wire [31:0] job_rx_bytes =
      USE_EXTERNAL_SCHEDULER ? (job_k_reg * N_WIRE * 8) : (K_DIM * N_WIRE * 8);
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
  assign job_done =
      USE_EXTERNAL_SCHEDULER && job_active && wb_done && wb_fi_last;

  always_ff @(posedge clk or negedge ui_rst_n) begin
    if (!ui_rst_n) begin
      job_busy            <= 1'b0;
      job_active          <= 1'b0;
      job_id_reg          <= 32'd0;
      job_device_id_reg   <= DEVICE_ID;
      job_k_reg           <= 32'd0;
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

  // Scheduler-facing completion is a one-cycle JOB event, not the raw c_done
  // level: arm after the real transaction has started, consume one completion.
  assign scheduler_c_done =
      USE_EXTERNAL_SCHEDULER && job_active && c_done_armed && c_done && !c_done_seen;

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

  always_ff @(posedge clk or negedge ui_rst_n) begin
    if (!ui_rst_n)
      tile_req <= 1'b0;
    else if (USE_EXTERNAL_SCHEDULER &&
             (job_fire || (job_active && tile_handoff && !compute_fi_last)))
      tile_req <= 1'b1;
    else if (tile_req && tile_ack)
      tile_req <= 1'b0;
  end

  // The release is meaningful only once the FSM is in P_GO with a landed tile.
  assign accelerator_ready = (phase == P_GO) && compute_ready;

  // =========================================================================
  // Addresses
  //
  // An external descriptor names exactly ONE operand payload (A and B of depth
  // job_k at job_a_base); n_inv repeats that tile transaction, every
  // invocation re-reading the same payload and writing its own result tile.
  // Legacy bring-up seeds n_inv slabs and reads slab fi.
  // =========================================================================
  assign seed_slab_addr =
      AXI_ADDR_W'(BASE_ADDR) + (AXI_ADDR_W'(fi) << RX_SHIFT);

  wire [AXI_ADDR_W-1:0] read_slab_addr =
      USE_EXTERNAL_SCHEDULER
          ? AXI_ADDR_W'(job_a_base_reg)
          : AXI_ADDR_W'(BASE_ADDR) + (AXI_ADDR_W'(read_fi) << RX_SHIFT);

  assign wb_region_base =
      USE_EXTERNAL_SCHEDULER
          ? AXI_ADDR_W'(job_c_base_reg)
          : AXI_ADDR_W'(BASE_ADDR) + (AXI_ADDR_W'(n_inv) << RX_SHIFT) +
            AXI_ADDR_W'(WB_GAP_BYTES);

  // Result tile stride is this array's own tile size.
  assign wb_desc_addr  = wb_region_base + (AXI_ADDR_W'(wb_fi) << WBT_SHIFT);
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
  wire [31:0] words_want = (32'(read_fi) + 32'd1) * job_rx_words;
  wire fill_complete = read_done_fold && (words_written == words_want);

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
    wire [K_W-1:0]      waddr;
    wire [31:0]         wdata_buf;

    dma_operand_writer #(
      .N (N_WIRE), .K_MAX (K_MAX), .AXI_DATA_W (AXI_DATA_W), .BEAT_W (BEAT_W)
    ) u_wr (
      .clk (clk), .rst_n (ui_rst_n),
      .dst_wr_en (dst_wr_en), .dst_wr_beat (dst_wr_beat), .dst_wr_data (dst_wr_data),
      .dst_full (dst_full), .dst_almost_full (dst_almost_full),
      .a_wr (a_wr), .b_wr (b_wr), .wsel (wsel), .waddr (waddr), .wdata (wdata_buf),
      .words_written (words_written), .err_range (wr_err_range), .clear (run_clear)
    );

    wire lane_in_array = (N_ARRAY >= N_WIRE) ? 1'b1 : (wsel < LANE_W_W'(N_ARRAY));
    wire [LANE_W-1:0] wsel_arr = wsel[LANE_W-1:0];

    wire [15:0] k16   = 16'(waddr);
    wire [7:0]  bank8 = 8'(wsel);
    wire [31:0] wpos  = {8'd0, bank8, k16};

    logic [31:0] chk_wr_q;
    always_ff @(posedge clk) begin
      if (phase == P_CALIB) chk_wr_q <= '0;
      else if (a_wr)        chk_wr_q <= chk_wr_q + (wdata_buf ^ wpos);
      else if (b_wr)        chk_wr_q <= chk_wr_q + (wdata_buf ^ (32'h8000_0000 | wpos));
    end
    assign chk_wr = chk_wr_q;

    wire [31:0] a_rdata_0 [0:N_ARRAY-1];
    wire [31:0] a_rdata_1 [0:N_ARRAY-1];
    wire [31:0] b_rdata_0 [0:N_ARRAY-1];
    wire [31:0] b_rdata_1 [0:N_ARRAY-1];

    systolic_operand_buffer #(.K_MAX (K_MAX), .K_W (K_W), .N_BANKS (N_ARRAY)) u_a_buf_0 (
      .clk (clk), .wr (a_wr && lane_in_array && !fill_bank),
      .wsel (wsel_arr), .waddr (waddr), .wdata (wdata_buf), .raddr (a_raddr), .rdata (a_rdata_0));
    systolic_operand_buffer #(.K_MAX (K_MAX), .K_W (K_W), .N_BANKS (N_ARRAY)) u_a_buf_1 (
      .clk (clk), .wr (a_wr && lane_in_array &&  fill_bank),
      .wsel (wsel_arr), .waddr (waddr), .wdata (wdata_buf), .raddr (a_raddr), .rdata (a_rdata_1));
    systolic_operand_buffer #(.K_MAX (K_MAX), .K_W (K_W), .N_BANKS (N_ARRAY)) u_b_buf_0 (
      .clk (clk), .wr (b_wr && lane_in_array && !fill_bank),
      .wsel (wsel_arr), .waddr (waddr), .wdata (wdata_buf), .raddr (b_raddr), .rdata (b_rdata_0));
    systolic_operand_buffer #(.K_MAX (K_MAX), .K_W (K_W), .N_BANKS (N_ARRAY)) u_b_buf_1 (
      .clk (clk), .wr (b_wr && lane_in_array &&  fill_bank),
      .wsel (wsel_arr), .waddr (waddr), .wdata (wdata_buf), .raddr (b_raddr), .rdata (b_rdata_1));

    for (genvar pp = 0; pp < N_ARRAY; pp = pp + 1) begin : PP_READ_MUX
      assign a_rdata[pp] = compute_bank ? a_rdata_1[pp] : a_rdata_0[pp];
      assign b_rdata[pp] = compute_bank ? b_rdata_1[pp] : b_rdata_0[pp];
    end
  end else begin : OP_V2
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
      .words_written (words_written), .err_range (wr_err_range), .clear (run_clear)
    );

    dma_wr_checksum_v2 #(.N (N_WIRE), .K_MAX (K_MAX), .AXI_DATA_W (AXI_DATA_W)) u_chk_wr (
      .clk (clk), .rst_n (ui_rst_n), .clear (phase == P_CALIB),
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
  wire [FEED_W-1:0] k_dim =
      USE_EXTERNAL_SCHEDULER ? FEED_W'(job_k_reg) : FEED_W'(K_DIM);

  typedef enum logic [2:0] {
    ST_IDLE        = 3'd0,
    ST_FEED        = 3'd1,
    ST_WAIT_RESULT = 3'd2,
    ST_DONE        = 3'd3
  } state_t;
  state_t            state;
  logic [FEED_W-1:0] feed_t;

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
  // snapshot: tile i publishes into bank i%2 (compute_fi[0], stable for the
  // fold); the stage latches that index at tile_handoff and reads only its
  // own bank.  Bank i%2 is rewritten by tile i+2 at the earliest, whose fold
  // cannot start before tile i+1 was handed off, i.e. after tile i's
  // write-back completed.
  logic [31:0] result_bank [0:1][0:N_ARRAY-1][0:N_ARRAY-1];

  integer rr, cc;
  always_ff @(posedge clk) begin
    if (rst_i) begin
      c_done <= 1'b0;
    end else begin
      if (fold_start) c_done <= 1'b0;
      if (c_valid_out) begin
        for (rr = 0; rr < N_ARRAY; rr = rr + 1)
          for (cc = 0; cc < N_ARRAY; cc = cc + 1) begin
            C[rr][cc]                         <= c_out[rr][cc];
            result_bank[compute_fi[0]][rr][cc] <= c_out[rr][cc];
          end
        c_done <= 1'b1;
      end
    end
  end

  // ---- main state progression --------------------------------------------------
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
      if (phase == P_CALIB)  chk_c <= '0;
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

  wire [31:0] active_expected_wr_chk =
      USE_EXTERNAL_SCHEDULER ? expected_wr_chk_for_k(job_k_reg) : EXPECT_WR_CHK;

  always_ff @(posedge clk or negedge ui_rst_n) begin
    if (!ui_rst_n) begin
      want_wr <= '0;
      want_c  <= '0;
    end else if (phase == P_CALIB) begin
      want_wr <= '0;
      want_c  <= '0;
    end else begin
      if (fill_complete)                       want_wr <= want_wr + active_expected_wr_chk;
      if ((wb_phase == W_SCAN) && scan_c_last) want_c  <= want_c + EXPECT_C_CHK;
    end
  end

  assign wr_match = read_done_sticky && (want_wr != 32'd0) && (chk_wr == want_wr);
  assign c_match  = (phase == P_DONE) && (chk_c == want_c);

`ifndef SYNTHESIS
  initial begin
    if (K_DIM > K_MAX)
      $fatal(1, "context %0d: K_DIM %0d exceeds K_MAX %0d", DEVICE_ID, K_DIM, K_MAX);
    if ((1 << RX_SHIFT) != RX_BYTES)
      $fatal(1, "context %0d: RX_BYTES %0d is not a power of two", DEVICE_ID, RX_BYTES);
    if ((1 << RXW_SHIFT) != RX_WORDS)
      $fatal(1, "context %0d: RX_WORDS %0d is not a power of two", DEVICE_ID, RX_WORDS);
    if ((1 << WBT_SHIFT) != WB_TILE_BYTES)
      $fatal(1, "context %0d: WB_TILE_BYTES is not a power of two", DEVICE_ID);
    if (N_ARRAY > N_WIRE)
      $fatal(1, "context %0d: array wider than the operand wire format", DEVICE_ID);
  end

  // An accepted job must fit this accelerator's own K_MAX.
  always_ff @(posedge clk) begin
    if (ui_rst_n && job_fire && (in_k > K_MAX || in_k < 1))
      $fatal(1, "context %0d (device %0d): job %0d k=%0d is outside 1..%0d",
             DEVICE_ID, in_device_id, in_job_id, in_k, K_MAX);
  end
`endif

endmodule

`default_nettype wire
