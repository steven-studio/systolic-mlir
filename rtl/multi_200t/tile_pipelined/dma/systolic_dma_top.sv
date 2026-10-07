// -----------------------------------------------------------------------------
// systolic_dma_top.sv -- bring-up step 3b: the array fed from DRAM.
//
//   DDR3 -> MIG -> dma_engine -> dma_operand_writer -> operand buffers
//        -> tile feeder -> systolic array -> C -> checksum
//        -> dma_result_reader -> dma_writeback_engine -> MIG -> DDR3
//
// systolic_uart_top.sv IS NOT TOUCHED BY THIS.  It is not instantiated, not
// patched, not parameterised -- it stays byte-identical, so the 48-configuration
// validation behind it and the board measurements taken from it remain exactly
// what they were.  That is the constraint this file is built to respect, and it
// is a reasonable one: the array is the thing 3b assumes is already correct, and
// editing the file that establishes that is a poor way to preserve it.
//
// THE COST, STATED PLAINLY
//   The compute core is therefore duplicated rather than shared: the operand
//   buffers, the feeder, the array, the C register and the four-state control
//   FSM appear here as well as there.  Two copies of anything can drift.  What
//   keeps them honest:
//
//     - Every block below is copied VERBATIM from systolic_uart_top, including
//       the synchronous reset style and the exact ST_FEED exit condition.  The
//       only deliberate differences are what starts a fold (a phase pulse
//       instead of matrices_ready) and what happens after one (a checksum scan
//       instead of a UART send).
//
//     - cyc_latched is the equivalence test.  systolic_uart_top measured 125
//       cycles for k_dim = 16, N = 8 on this board.  This design counts the same
//       interval with the same code.  If the copy has drifted by so much as one
//       cycle of control, 125 will not come back -- so the cycle count is not
//       decoration here, it is the check that the duplicate is faithful.
//
// WHY THE SEED IS NOT 3a's
//   3a's pattern (word = its own index) is denormal read as fp32: every product
//   underflows to zero, every accumulation is zero, and a golden model built
//   from the same pattern agrees perfectly with a path that moved nothing.  The
//   test would pass and distinguish nothing.  SEED_MODE = 1 seeds the fp32
//   values of the integers 1..127 -- see dma_seed_writer's header for why 127,
//   and why exactness matters more than realism.
//
// TWO CHECKSUMS
//   chk_wr   over dma_operand_writer's WRITE STREAM, one term per word, in the
//            formula tools/seed_ref.py uses.  32-bit wrapping addition is
//            commutative, so payload order gives the same total as seed_ref's
//            (k, bank) order.
//   chk_c    over the RESULT matrix, one entry per cycle.
//
//   Either alone is weak in a specific way: chk_wr would not notice an array
//   that ignored its inputs, and chk_c would not say where a failure happened.
//   Together they cut the path at the buffers -- chk_wr wrong means the DMA
//   side, chk_wr right and chk_c wrong means the array side.
//
// THE GOLDEN IS NOT MINE
//   Both constants come from tools/seed_ref.py, whose model was confirmed
//   against this array over UART before any of this existed: all 64 entries of
//   C matched and the cycle count came back 125.  A disagreement here is a DMA
//   failure, not an argument about what the array should do.  That question was
//   settled first, deliberately -- settling it afterwards costs a bitstream per
//   attempt.
//
//   The UART path remains the independent reference, in its own bitstream:
//     python3 tools/uart_check.py --port /dev/ttyUSB2 --kmax 16
//   run against a build_kmax build, before or after this one.
//
// LEDS, right to left
//   led[0] init_calib_complete    memory alive
//   led[1] seed written
//   led[2] descriptor complete
//   led[3] fold complete
//   led[4] chk_wr MATCHES         operands arrived correctly
//   led[5] chk_c  MATCHES         <- the gate: the array computed from them
//   led[6] any error latched
//   led[7] heartbeat, ui_clk
//
//   The write-back status does not get an LED -- all eight are spoken for and
//   renumbering them would invalidate every photograph of a previous run.  It
//   is probe_in4[7] over JTAG, and a write-back fault raises led[6] like any
//   other fault.
//
// Read the numbers over JTAG with dma_top_build.tcl -tclargs read.
//
// TWO OPERAND PATHS, ONE TOP (USE_V2, Sep 2026)
//   The FPGA'27 draft measures the operand path stage by stage and then widens
//   the buffer write port.  Both versions are built from this file: USE_V2=0
//   is the v1 path (four-cycle writer, one 32-bit port -- the bottleneck),
//   USE_V2=1 the beat-wide one.  Nine counters were added for it and probed:
//   fill_cycles (descriptor accepted -> last word landed), the read engine's
//   busy / rdy_stall / r_stall, wb_cycles (descriptor -> last B), and the
//   write-back engine's busy / aw_stall / w_stall / src_starve.  Nothing in
//   the copied core changed; cyc_latched is still the equivalence check, on
//   both versions.
// -----------------------------------------------------------------------------

`default_nettype none

module systolic_dma_top #(
  parameter integer N     = 8,
  // K_MAX = 16 for the first 3b build: the geometry whose golden was confirmed
  // on hardware, a 1 KiB payload, and a build measured in minutes.  3b is a
  // correctness step; K_MAX = 256 belongs to 3c, where bandwidth is the point.
  parameter integer K_MAX = 16,
  parameter integer K_DIM = 16,             // runtime reduction length, <= K_MAX

  // Both printed by:  python3 tools/seed_ref.py --mode 1 --kmax 16
  parameter logic [31:0] EXPECT_WR_CHK = 32'h3F88_0780,   // "EXPECT_CHK"
  parameter logic [31:0] EXPECT_C_CHK  = 32'hC74B_2660,   // "C checksum"

  parameter integer BASE_ADDR = 0,

  // The gap between the operand image and the result image.  The operand
  // image is n_inv * RX_BYTES long (RX_BYTES = K_MAX*8*N, 1 KiB at K_MAX = 16)
  // and n_inv is a RUN-TIME quantity, so the write-back base is computed from
  // the latched n_inv rather than folded here: far enough that an
  // address-arithmetic error in either direction shows up as a wrong checksum
  // rather than as one image quietly overwriting the other.  At n_inv = 1 the
  // computed base is BASE_ADDR + RX_BYTES + WB_GAP_BYTES, which is the
  // constant the old WB_BASE_ADDR parameter carried -- the single-invocation
  // address map is unchanged, byte for byte.
  parameter integer WB_GAP_BYTES = 4096,

  // OPERAND PATH VERSION.  0 = v1: dma_operand_writer unpacks each 128-bit
  // beat into four serial writes to systolic_operand_buffer's single 32-bit
  // port (1.00 word/cycle by construction -- the bottleneck the paper
  // measures).  1 = v2: dma_operand_writer_v2 hands the beat whole to
  // systolic_operand_buffer_v2, cyclic layout on A and block layout on B, and
  // chk_wr is accumulated four terms per beat by dma_wr_checksum_v2 -- same
  // constant.  Everything downstream of the buffers' read ports -- feeder,
  // array, FSM, cyc_latched, chk_c, EXPECT_* -- is identical in both, which is
  // what makes the two bitstreams comparable: same operands, same array, same
  // golden, only the write side of the buffers differs.
  parameter bit USE_V2 = 1'b0,

  // Scheduler integration mode.
  //
  // 0: existing bring-up FSM owns fold_start.
  // 1: external scheduler owns the accelerator transaction start.
  parameter bit USE_EXTERNAL_SCHEDULER = 1'b0,

  // Physical accelerator fleet configuration.
  //
  // Default configuration matches the verified hardware:
  //   device 0     : 8x8
  //   devices 1..3: 4x4
  parameter integer NUM_8X8 = 1,
  parameter integer NUM_4X4 = 3,

  // Legacy compiler/testbench descriptor producer.
  //
  // 1: allow the top-level job_* ports to provide descriptors.
  // 0: production board build uses only the internal descriptor bridge.
  //
  // Keep the default enabled so existing simulation testbenches are unchanged.
  parameter bit USE_LEGACY_JOB_PORTS = 1'b1
) (
  input  wire        sys_clk_pin,     // R4, 100 MHz
  input  wire        cpu_resetn,      // G4, active low
  output wire [7:0]  led,

  output wire [14:0] ddr3_addr,
  output wire [2:0]  ddr3_ba,
  output wire        ddr3_cas_n,
  output wire [0:0]  ddr3_ck_n,
  output wire [0:0]  ddr3_ck_p,
  output wire [0:0]  ddr3_cke,
  output wire        ddr3_ras_n,
  output wire        ddr3_reset_n,
  output wire        ddr3_we_n,
  inout  wire [15:0] ddr3_dq,
  inout  wire [1:0]  ddr3_dqs_n,
  inout  wire [1:0]  ddr3_dqs_p,
  output wire [1:0]  ddr3_dm,
  output wire [0:0]  ddr3_odt,

  // Scheduler integration interface.
  //
  // These expose the existing accelerator transaction boundary:
  //   scheduler_fold_start -> start one systolic computation
  //   scheduler_c_done     <- result matrix is ready
  //
  // They are intentionally added without changing the existing
  // bring-up FSM yet.  The next integration step can select whether
  // the internal FSM or the external scheduler owns fold_start.
  input  wire            scheduler_fold_start,
  output wire            scheduler_c_done,

  // ----------------------------------------------------------
  // Scheduler job interface.
  //
  // A job is accepted when:
  //
  //     job_valid && job_ready
  //
  // The descriptor is latched at that boundary.  The existing
  // accelerator datapath is intentionally left unchanged in
  // this step; the next integration step will consume the
  // latched address/shape fields in the DMA path.
  // ----------------------------------------------------------
  input  wire            job_valid,
  output wire            job_ready,

  input  wire [31:0]     job_id,

  // Physical accelerator instance selected by the scheduler.
  // This is an opaque device ID, not a geometry encoding.
  input wire [31:0] job_device_id,

  input  wire [31:0]     job_m,
  input  wire [31:0]     job_n,
  input  wire [31:0]     job_k,

  // Compiler-generated temporal schedule.
  //
  // start_cycle : absolute release cycle generated by the compiler.
  // est_cycles  : analytical compute duration generated by the compiler.
  //
  // start_cycle is consumed by the hardware scheduler.
  // est_cycles is carried through the ABI for observability and
  // future schedule validation. Actual completion still comes
  // from the physical accelerator.
  input  wire [31:0]     job_start_cycle,
  input  wire [31:0]     job_est_cycles,
  input  wire [63:0]     job_a_base,
  input  wire [63:0]     job_b_base,
  input  wire [63:0]     job_c_base,

  // ----------------------------------------------------------
  // DPTI descriptor transport.
  //
  // Generic register-write interface. A physical host/MMIO
  // transport can be connected above this boundary.
  // ----------------------------------------------------------
  input  wire            dpti_wr_valid,
  output wire            dpti_wr_ready,
  input  wire [7:0]      dpti_wr_addr,
  input  wire [31:0]     dpti_wr_data,

  // ----------------------------------------------------------
  // Physical Nexys Video FT2232H DPTI interface.
  //
  // DPTI is an 8-bit bidirectional synchronous FIFO interface.
  // The FT2232H drives/receives the data bus depending on direction.
  //
  // DPTI clock:
  //   dpti_clkout = FT2232H CLKO
  //
  // All *_n signals are active-low.
  // ----------------------------------------------------------
  inout  wire [7:0]      dpti_d,
  input  wire            dpti_rxf_n,
  input  wire            dpti_txe_n,
  input  wire            dpti_clkout,

  output wire            dpti_rd_n,
  output wire            dpti_wr_n,
  output wire            dpti_oe_n,
  input  wire             dpti_siwun
);

  localparam integer NUM_ACCEL = NUM_8X8 + NUM_4X4;
  localparam integer NUM_8X8_STORAGE = (NUM_8X8 > 0) ? NUM_8X8 : 1;
  localparam integer NUM_4X4_STORAGE = (NUM_4X4 > 0) ? NUM_4X4 : 1;

  localparam integer AXI_DATA_W = 128;
  localparam integer AXI_ADDR_W = 29;
  localparam integer RX_BYTES   = K_MAX * 8 * N;
  localparam integer RX_WORDS   = RX_BYTES / 4;
  localparam integer N_BEATS    = RX_BYTES / (AXI_DATA_W/8);

  // ---- geometry, derived exactly as systolic_uart_top derives it ----------
  localparam int K_W       = $clog2(K_MAX);
  localparam int LANE_W    = $clog2(N);
  localparam int FEED_LAST = K_MAX + N - 2;
  localparam int FEED_W    = $clog2(FEED_LAST + 1);

  // ---- clocking: identical to dma_bringup_top / ddr3_bw_top ---------------
  // The array runs on ui_clk.  MIG's 4:1 PHY ratio against 800 Mbps DDR3 makes
  // ui_clk exactly 100 MHz -- the frequency the array was characterised at --
  // so there is no clock-domain crossing anywhere in this design.  A CDC here
  // would be a second thing that could be wrong in a step whose entire job is
  // to find out whether the first thing is.
  wire clk100_ibuf, clk200_raw, clkfb_raw, clkfb;
  wire clk_sys_100, clk_ref_200;
  wire mmcm_locked_user;

  IBUF u_ibuf (.I(sys_clk_pin), .O(clk100_ibuf));

  MMCME2_BASE #(
    .BANDWIDTH ("OPTIMIZED"), .CLKIN1_PERIOD (10.000),
    .DIVCLK_DIVIDE (1), .CLKFBOUT_MULT_F (10.000), .CLKFBOUT_PHASE (0.000),
    .CLKOUT0_DIVIDE_F (5.000), .CLKOUT0_DUTY_CYCLE (0.500), .CLKOUT0_PHASE (0.000),
    .REF_JITTER1 (0.010), .STARTUP_WAIT ("FALSE")
  ) u_mmcm (
    .CLKIN1 (clk100_ibuf), .CLKFBIN (clkfb),
    .CLKFBOUT (clkfb_raw), .CLKFBOUTB (),
    .CLKOUT0 (clk200_raw), .CLKOUT0B (),
    .CLKOUT1 (), .CLKOUT1B (), .CLKOUT2 (), .CLKOUT2B (),
    .CLKOUT3 (), .CLKOUT3B (), .CLKOUT4 (), .CLKOUT5 (), .CLKOUT6 (),
    .LOCKED (mmcm_locked_user), .PWRDWN (1'b0), .RST (1'b0)
  );

  BUFG u_bufg_fb  (.I(clkfb_raw),  .O(clkfb));
  BUFG u_bufg_200 (.I(clk200_raw), .O(clk_ref_200));
  BUFG u_bufg_100 (.I(clk100_ibuf),.O(clk_sys_100));

  wire sys_rst_n = cpu_resetn & mmcm_locked_user;

  wire ui_clk, ui_clk_sync_rst, mmcm_locked_mig, init_calib_complete;
  wire ui_rst_n = ~ui_clk_sync_rst;

  // systolic_uart_top resets its array-side logic SYNCHRONOUSLY, from a plain
  // active-high rst.  The copied blocks below keep that style rather than the
  // asynchronous form the DMA modules use: reset style changes the netlist, and
  // "the same logic" has to mean the same logic if the 125-cycle equivalence
  // check is going to mean anything.
  wire rst_i = ~ui_rst_n;

  // ---- AXI write channel: two masters, one at a time ----------------------
  // The seeder writes the operand image in P_SEED; the write-back engine writes
  // the result tile in P_WB.  They are never alive at the same time -- the
  // seeder's done pulses only after its last write response, and P_WB is four
  // phases later -- so this is an ownership switch at a phase boundary, not an
  // arbiter.  There is no round-robin, no priority, and nothing to starve.
  //
  // The premise is checked rather than asserted.  err_w_owner latches if the
  // master that does not own the channel ever raises awvalid or wvalid, and it
  // feeds any_err like every other fault.  An ownership scheme whose premise
  // lives only in a comment is one more silent mode of the kind this design
  // keeps running into.
  wire [1:0]   awid;   wire [28:0] awaddr;  wire [7:0] awlen;
  wire [2:0]   awsize; wire [1:0]  awburst; wire [0:0] awlock;
  wire [3:0]   awcache; wire [2:0] awprot;  wire [3:0] awqos;
  wire         awvalid, awready;
  wire [127:0] wdata_axi; wire [15:0] wstrb; wire wlast, wvalid, wready;
  wire [1:0]   bid, bresp; wire bvalid, bready;

  // seeder side
  wire [1:0]   sd_awid;    wire [28:0] sd_awaddr;  wire [7:0] sd_awlen;
  wire [2:0]   sd_awsize;  wire [1:0]  sd_awburst; wire [0:0] sd_awlock;
  wire [3:0]   sd_awcache; wire [2:0]  sd_awprot;  wire [3:0] sd_awqos;
  wire         sd_awvalid; wire        sd_awready;
  wire [127:0] sd_wdata;   wire [15:0] sd_wstrb;
  wire         sd_wlast, sd_wvalid, sd_wready;
  wire         sd_bvalid, sd_bready;

  // write-back side
  wire [1:0]   wb_awid;    wire [28:0] wb_awaddr;  wire [7:0] wb_awlen;
  wire [2:0]   wb_awsize;  wire [1:0]  wb_awburst; wire [0:0] wb_awlock;
  wire [3:0]   wb_awcache; wire [2:0]  wb_awprot;  wire [3:0] wb_awqos;
  wire         wb_awvalid; wire        wb_awready;
  wire [127:0] wb_wdata;   wire [15:0] wb_wstrb;
  wire         wb_wlast, wb_wvalid, wb_wready;
  wire         wb_bvalid, wb_bready;

  // Host-staging write side.
  wire [1:0]   hs_awid;    wire [28:0] hs_awaddr;  wire [7:0] hs_awlen;
  wire [2:0]   hs_awsize;  wire [1:0]  hs_awburst; wire [0:0] hs_awlock;
  wire [3:0]   hs_awcache; wire [2:0]  hs_awprot;  wire [3:0] hs_awqos;
  wire         hs_awvalid; wire        hs_awready;
  wire [127:0] hs_wdata;   wire [15:0] hs_wstrb;
  wire         hs_wlast, hs_wvalid, hs_wready;
  wire         hs_bvalid, hs_bready;

  wire         hs_busy;
  wire         hs_done;
  wire         hs_err_protocol;
  wire         hs_err_align;
  wire         hs_err_resp;

  logic        wb_owns_w;

  // Host staging has ownership only while its writer is active.
  // Otherwise preserve the original writeback-versus-seeder selection.
  assign awid       = hs_busy ? hs_awid
                              : (wb_owns_w ? wb_awid    : sd_awid);
  assign awaddr     = hs_busy ? hs_awaddr
                              : (wb_owns_w ? wb_awaddr  : sd_awaddr);
  assign awlen      = hs_busy ? hs_awlen
                              : (wb_owns_w ? wb_awlen   : sd_awlen);
  assign awsize     = hs_busy ? hs_awsize
                              : (wb_owns_w ? wb_awsize  : sd_awsize);
  assign awburst    = hs_busy ? hs_awburst
                              : (wb_owns_w ? wb_awburst : sd_awburst);
  assign awlock     = hs_busy ? hs_awlock
                              : (wb_owns_w ? wb_awlock  : sd_awlock);
  assign awcache    = hs_busy ? hs_awcache
                              : (wb_owns_w ? wb_awcache : sd_awcache);
  assign awprot     = hs_busy ? hs_awprot
                              : (wb_owns_w ? wb_awprot  : sd_awprot);
  assign awqos      = hs_busy ? hs_awqos
                              : (wb_owns_w ? wb_awqos   : sd_awqos);
  assign awvalid    = hs_busy ? hs_awvalid
                              : (wb_owns_w ? wb_awvalid : sd_awvalid);

  assign hs_awready = hs_busy && awready;
  assign sd_awready = !hs_busy && !wb_owns_w && awready;
  assign wb_awready = !hs_busy &&  wb_owns_w && awready;

  assign wdata_axi  = hs_busy ? hs_wdata
                              : (wb_owns_w ? wb_wdata  : sd_wdata);
  assign wstrb      = hs_busy ? hs_wstrb
                              : (wb_owns_w ? wb_wstrb  : sd_wstrb);
  assign wlast      = hs_busy ? hs_wlast
                              : (wb_owns_w ? wb_wlast  : sd_wlast);
  assign wvalid     = hs_busy ? hs_wvalid
                              : (wb_owns_w ? wb_wvalid : sd_wvalid);

  assign hs_wready  = hs_busy && wready;
  assign sd_wready  = !hs_busy && !wb_owns_w && wready;
  assign wb_wready  = !hs_busy &&  wb_owns_w && wready;

  // Response is visible only to the selected write master.
  assign bready     = hs_busy ? hs_bready
                              : (wb_owns_w ? wb_bready : sd_bready);

  assign hs_bvalid  = hs_busy && bvalid;
  assign sd_bvalid  = !hs_busy && !wb_owns_w && bvalid;
  assign wb_bvalid  = !hs_busy &&  wb_owns_w && bvalid;

  // ---- AXI read channel: the engine ---------------------------------------
  wire [1:0]   arid;   wire [28:0] araddr;  wire [7:0] arlen;
  wire [2:0]   arsize; wire [1:0]  arburst; wire [0:0] arlock;
  wire [3:0]   arcache; wire [2:0] arprot;  wire [3:0] arqos;
  wire         arvalid, arready;
  wire [127:0] rdata_axi; wire [1:0] rresp; wire rlast, rvalid, rready;

  // =========================================================================
  // Bring-up sequencer
  //
  //   P_CALIB  wait for DDR3, latch n_inv
  //   P_SEED   write the known image -- once per slab, all n_inv of them
  //   P_READ   pull slab fi back through the DMA into the operand buffers
  //   P_GO     one pulse to start the fold
  //   P_FOLD   wait for the array
  //   P_SCAN   read C back, one entry per cycle
  //   P_WB     push the same C out to DRAM through the write-back engine,
  //            then back to P_READ for slab fi+1 until fi = n_inv-1
  //
  // The loop is the whole point of the n_inv > 1 build: a GEMM of depth
  // K = n_inv * K_MAX run as n_inv invocations of depth K_MAX, each paying
  // 2(N-1) + H and its own write-back, which is the split the cost model
  // prices against one invocation of depth K.  All seeding happens before the
  // first read so the AXI write channel changes hands exactly once.
  // =========================================================================
  typedef enum logic [3:0] {
    P_CALIB, P_SEED, P_READ, P_GO, P_FOLD, P_SCAN, P_WB, P_DONE
  } phase_t;
  phase_t phase;

  // -------------------------------------------------------------------------
  // DEBUG: top-level transaction FSM transition trace.
  //
  // -------------------------------------------------------------------------
  phase_t phase_prev;

  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n) begin
      phase_prev <= P_CALIB;
    end
    else if (phase != phase_prev) begin
      $display(
        "FSMTRANS t=%0t %0d->%0d fi=%0d | fill=%0b c_done_fold=%0b scan_last=%0b | wb_desc_valid=%0b wb_desc_ready=%0b wb_done=%0b | select8=%0b select4=%0b",
        $time,
        phase_prev,
        phase,
        fi,
        fill_complete,
        c_done_fold,
        scan_c_last,
        wb_desc_valid,
        wb_desc_ready,
        wb_done,
        select_8x8,
        select_4x4
      );
      phase_prev <= phase;
    end
  end

  logic          seed_start;
  wire           seed_busy, seed_done, seed_err_align, seed_err_resp;

  logic          desc_valid;
  wire           desc_ready, read_done;
  wire           eng_err_align, eng_err_resp, wr_err_range;
  wire [31:0]    words_written;
  logic          desc_started;
  logic          seed_done_sticky, read_done_sticky, fold_done_sticky;

  logic          fsm_fold_start;     // fold_start generated by the bring-up FSM
  logic          c_done;              // declared here, driven by the copied block

  // -------------------------------------------------------------------------
  // Hardware scheduler integration.
  //
  // The existing job ABI supplies the physical device ID and memory region.
  // The scheduler additionally owns the temporal release of the selected
  // accelerator.  The current ABI does not yet carry start_cycle or
  // compute_cycles, so the first integration stage uses:
  //
  //   start_cycle   = 0
  //   compute_cycles = 0
  //
  // This stage validates the control/dataflow boundary only.  Analytical
  // scheduling cost remains a compiler-side responsibility.
  // -------------------------------------------------------------------------

  wire [NUM_ACCEL-1:0] scheduler_accelerator_start;
  wire [NUM_ACCEL-1:0] scheduler_accelerator_done;

  // -------------------------------------------------------------------------
  // Scheduler start pulse capture
  //
  // The hardware scheduler emits accelerator_start as a one-cycle pulse.
  // The existing accelerator FSM accepts fold_start only while phase == P_GO.
  //
  // Therefore an early accelerator_start is remembered here and replayed
  // through the normal scheduler adapter once P_GO is reached.
  // -------------------------------------------------------------------------

  wire scheduler_fold_start_8x8 [0:NUM_8X8_STORAGE-1];
  wire scheduler_fold_start_4x4 [0:NUM_4X4_STORAGE-1];

  localparam integer ACC_ID_W =
      (NUM_ACCEL <= 1) ? 1 : $clog2(NUM_ACCEL);

  logic                scheduler_start_pending;
  logic [ACC_ID_W-1:0] scheduler_start_pending_id;

  // Registered one-cycle replay pulse.
  //
  // This is intentionally separate from the pending bit:
  //
  //   pending = remembered scheduler release
  //   replay  = actual one-cycle protocol event sent to the adapter
  //
  // Keeping replay registered avoids relying on a combinational
  // phase/P_GO transition at the same clock edge.
  logic scheduler_start_replay;

  wire scheduler_start_request =
      |scheduler_accelerator_start;

  // -------------------------------------------------------------------------
  // Single scheduler-release capture.
  //
  // The hardware scheduler emits accelerator_start as a one-cycle pulse.
  // DMA may still be in P_CALIB/P_READ at that time, so the pulse cannot be
  // connected directly to the existing accelerator adapter.
  //
  // Remember the release until P_GO.  The pending bit is the single source
  // of truth for both:
  //
  //   1. deciding that P_GO may advance to P_FOLD
  //   2. replaying accelerator_start into the adapter
  //
  // Do NOT maintain a second independent pending bit.
  // -------------------------------------------------------------------------
  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n) begin
      scheduler_start_pending    <= 1'b0;
      scheduler_start_pending_id <= '0;
      scheduler_start_replay     <= 1'b0;
    end
    else begin
      // replay is always a one-cycle pulse.
      scheduler_start_replay <= 1'b0;

      // Capture the scheduler's one-cycle release.
      if (scheduler_start_request) begin
        scheduler_start_pending <= 1'b1;

        for (integer start_i = 0; start_i < NUM_ACCEL; start_i = start_i + 1) begin
          if (scheduler_accelerator_start[start_i])
            scheduler_start_pending_id <= ACC_ID_W'(start_i);
        end
      end

      // When P_GO is reached, convert the remembered scheduler event
      // into an explicit registered pulse for the adapter.
      //
      // The replay pulse becomes visible AFTER this clock edge and
      // therefore remains stable for the following adapter sampling edge.
      else if (phase == P_GO && scheduler_start_pending) begin
        scheduler_start_replay  <= 1'b1;
        scheduler_start_pending <= 1'b0;
      end
    end
  end

  // The adapter consumes the explicit replay pulse.
  //
  // A live scheduler pulse is also allowed while P_GO is active, although
  // the normal external-scheduler path should normally use the replay pulse.
  wire [NUM_ACCEL-1:0] scheduler_accelerator_start_delayed =
      scheduler_start_replay
        ? ({{(NUM_ACCEL-1){1'b0}}, 1'b1} << scheduler_start_pending_id)
        : ((phase == P_GO)
            ? scheduler_accelerator_start
            : {NUM_ACCEL{1'b0}});

  wire scheduler_busy;
  wire scheduler_schedule_done;
  wire scheduler_desc_ready;

  wire [31:0] scheduler_cycle_counter;
  wire [31:0] scheduler_active_start_cycle;
  wire [31:0] scheduler_active_compute_cycles;
  wire [1:0]  scheduler_active_accelerator_id;

  systolic_hw_scheduler #(
    .NUM_ACCEL (NUM_ACCEL),
    .CYCLE_W   (32),
    .ACC_W     (ACC_ID_W)
  ) u_hw_scheduler (
    .clk                    (ui_clk),
    .rst                    (!ui_rst_n),

    .desc_valid             (job_fire),
    .desc_ready             (scheduler_desc_ready),

    .desc_accelerator_id    (effective_job_device_id[1:0]),
    .desc_start_cycle       (effective_job_start_cycle),
    .desc_compute_cycles    (effective_job_est_cycles),

    .accelerator_start      (scheduler_accelerator_start),
    .accelerator_done       (scheduler_accelerator_done),

    .busy                   (scheduler_busy),
    .schedule_done          (scheduler_schedule_done),

    .cycle_counter          (scheduler_cycle_counter),

    .active_start_cycle     (scheduler_active_start_cycle),
    .active_compute_cycles  (scheduler_active_compute_cycles),
    .active_accelerator_id  (scheduler_active_accelerator_id)
  );

  generate
    for (genvar i = 0; i < NUM_8X8; i++) begin : SCHED_8X8
      systolic_hw_scheduler_adapter #(.CYCLE_W(32)) u_adapter (
        .clk(ui_clk), .rst(!ui_rst_n),
        .accelerator_start(scheduler_accelerator_start_delayed[i]),
        .accelerator_done(scheduler_accelerator_done[i]),
        .fold_start(scheduler_fold_start_8x8[i]),
        .c_done(c_valid_out_8x8[i])
      );
    end

    for (genvar i = 0; i < NUM_4X4; i++) begin : SCHED_4X4
      systolic_hw_scheduler_adapter #(.CYCLE_W(32)) u_adapter (
        .clk(ui_clk), .rst(!ui_rst_n),
        .accelerator_start(scheduler_accelerator_start_delayed[NUM_8X8+i]),
        .accelerator_done(scheduler_accelerator_done[NUM_8X8+i]),
        .fold_start(scheduler_fold_start_4x4[i]),
        .c_done(c_valid_out_4x4[i])
      );
    end
  endgenerate

  wire scheduler_fold_start_selected =
      select_8x8 ? scheduler_fold_start_8x8[selected_8x8_idx] :
      select_4x4 ? scheduler_fold_start_4x4[selected_4x4_idx] :
                   1'b0;

  // Selected accelerator start signal.
  //
  // Normal bring-up:
  //   FSM -> fsm_fold_start -> fold_start
  //
  // Scheduler mode:
  //   scheduler adapter -> fold_start
  //
  // The existing accelerator consumes only fold_start.
  wire fold_start =
      USE_EXTERNAL_SCHEDULER
          ? scheduler_fold_start_selected
          : fsm_fold_start;

  // ==========================================================
  // Scheduler job boundary.
  //
  // For now a job is accepted only when the accelerator is idle
  // with respect to an externally scheduled transaction.
  //
  // The descriptor is latched exactly once per accepted job.
  // The legacy scheduler_fold_start path remains available for
  // compatibility while the job interface is being integrated.
  // ==========================================================

  logic        job_busy;

  // ----------------------------------------------------------
  // Scheduler job lifetime.
  //
  // job_fire  : job boundary / acceptance
  // job_active: exactly one accepted job is in flight
  // job_done  : completion belonging to that accepted job
  //
  // scheduler_fold_start is NOT the job boundary.  It starts
  // computation inside the already accepted job.
  // ----------------------------------------------------------
  logic        job_active;

  logic [31:0] job_id_reg;
  logic [31:0] job_device_id_reg;
  logic [31:0] job_m_reg;
  logic [31:0] job_n_reg;
  logic [31:0] job_k_reg;
  logic [31:0] job_start_cycle_reg;
  logic [31:0] job_est_cycles_reg;

  // ----------------------------------------------------------
  // Physical device selection.
  //
  // device_id is an opaque physical-instance identifier.
  // Geometry is NOT encoded into the scheduler protocol.
  //
  // These IDs describe the 1x8x8 + 3x4x4 physical fleet:
  //   0 -> 8x8 instance 0
  //   1 -> 4x4 instance 0
  //   2 -> 4x4 instance 1
  //   3 -> 4x4 instance 2
  //
  // Geometry and physical-instance identity are deliberately separate.
  // All three 4x4 instances share the same geometry but retain distinct
  // scheduler-visible device IDs.
  // ----------------------------------------------------------
  wire select_8x8 =
      (job_device_id_reg < NUM_8X8);

  wire select_4x4 =
      (job_device_id_reg >= NUM_8X8) &&
      (job_device_id_reg < NUM_ACCEL);

  wire [31:0] selected_8x8_idx = job_device_id_reg;
  wire [31:0] selected_4x4_idx = job_device_id_reg - NUM_8X8;

  // Runtime transfer size for an externally scheduled job.
  // One invocation contains A[K,N] + B[K,N], both FP32:
  //   K * N * 4 + K * N * 4 = K * N * 8 bytes.
  //
  // K_MAX is the physical buffer capacity; JOB_K is the
  // reduction length of this particular job.
  // The operand image is physically laid out at K_MAX capacity.
  // JOB_K controls how many reduction entries the array consumes;
  // it does not change the DMA payload layout or operand-buffer decode.
  wire [31:0] job_rx_bytes = RX_BYTES;

  wire [31:0] job_n_beats =
      job_rx_bytes / (AXI_DATA_W / 8);

  wire [31:0] job_rx_words =
      job_rx_bytes / 4;
  logic [63:0] job_a_base_reg;
  logic [63:0] job_b_base_reg;
  logic [63:0] job_c_base_reg;

  // A job can be accepted when no external job is currently in flight.
  // P_DONE is the natural idle point for the externally driven path.
  // External jobs must be accepted before P_CALIB launches the seed.
  // Otherwise slab_addr still comes from the reset value of job_a_base_reg
  // and the seed writer would write the operand image to address 0.
  wire core_job_ready;

  // A completed compute/writeback job may still own the shared DMA read
  // engine while its result tile is being read back.  Do not accept the next
  // external descriptor until that readback has completely drained.
  wire result_readback_busy;

  assign core_job_ready =
      USE_EXTERNAL_SCHEDULER &&
      ui_rst_n &&
      !job_busy &&
      !hs_busy &&
      !result_readback_busy &&
      (phase == P_CALIB || phase == P_READ || phase == P_DONE);

  // ----------------------------------------------------------
  // DPTI descriptor bridge.
  //
  // Converts host register writes into the existing DPTI job
  // descriptor protocol.
  // ----------------------------------------------------------

  wire        dpti_job_valid;
  wire        dpti_job_ready;

  wire [31:0] dpti_job_id;
  wire [31:0] dpti_job_device_id;

  wire [31:0] dpti_job_m;
  wire [31:0] dpti_job_n;
  wire [31:0] dpti_job_k;

  wire [31:0] dpti_job_start_cycle;
  wire [31:0] dpti_job_est_cycles;

  wire [63:0] dpti_job_a_base;
  wire [63:0] dpti_job_b_base;
  wire [63:0] dpti_job_c_base;

  wire [31:0] dpti_status;

  // ----------------------------------------------------------
  // Physical FT2232H DPTI host command path.
  //
  // Ubuntu -> USB -> FT2232H -> DPTI -> RX parser
  //       -> async CDC FIFO -> ui_clk AXI4-Lite master.
  //
  // This is the real host transport.  UART is not involved.
  // ----------------------------------------------------------

  wire        dpti_host_cmd_valid;
  wire        dpti_host_cmd_ready;
  wire [7:0]  dpti_host_cmd_addr;
  wire [31:0] dpti_host_cmd_data;

  // Shared physical byte stream.
  wire [7:0]  dpti_byte_data;
  wire        dpti_byte_valid;
  wire        dpti_byte_ready;

  // MEM_WRITE branch.  During this first integration step the decoded
  // payload is intentionally consumed locally.  The CDC/DDR writer is
  // connected only after the existing WRITE32 path regresses cleanly.
  wire                    dpti_mem_start;
  wire [AXI_ADDR_W-1:0]   dpti_mem_base_addr;
  wire [AXI_DATA_W-1:0]   dpti_mem_data;
  wire                    dpti_mem_valid;
  wire                    dpti_mem_ready;
  wire                    dpti_mem_done;

  wire dpti_frontend_err_opcode;
  wire dpti_frontend_err_write32_opcode;
  wire dpti_frontend_err_mem_opcode;
  wire dpti_frontend_err_mem_length;

  wire        dpti_fifo_wr_ready;
  wire        dpti_fifo_rd_valid;
  wire        dpti_fifo_rd_ready;
  wire [7:0]  dpti_fifo_rd_addr;
  wire [31:0] dpti_fifo_rd_data;

  wire        dpti_axi_cmd_valid;
  wire        dpti_axi_cmd_ready;

  wire        dpti_axi_rsp_valid;
  wire        dpti_axi_rsp_ready;
  wire [1:0]  dpti_axi_rsp_resp;

  wire [7:0]  dpti_axi_awaddr;
  wire        dpti_axi_awvalid;
  wire        dpti_axi_awready;

  wire [31:0] dpti_axi_wdata;
  wire [3:0]  dpti_axi_wstrb;
  wire        dpti_axi_wvalid;
  wire        dpti_axi_wready;

  wire [1:0]  dpti_axi_bresp;
  wire        dpti_axi_bvalid;
  wire        dpti_axi_bready;

  // ------------------------------------------------------------------------
  // FT2232H DPTI RX parser.
  //
  // The DPTI data bus is only driven by the FT2232H during RX.
  // Therefore the FPGA side remains high-Z here.
  // ------------------------------------------------------------------------

  // Physical data bus ownership is selected by the output path.
  // Otherwise the FPGA releases the bidirectional bus for input traffic.
  wire [7:0] dpti_tx_data;
  wire       dpti_tx_oe;
  wire       dpti_tx_wr_n;

  assign dpti_d =
      dpti_tx_oe
          ? dpti_tx_data
          : 8'bz;

  assign dpti_wr_n = dpti_tx_wr_n;

  // ------------------------------------------------------------------------
  // DPTI clock-domain reset.
  //
  // dpti_host_rx is clocked by dpti_clkout, so synchronize the
  // system reset into the DPTI clock domain before using it there.
  // ------------------------------------------------------------------------

  reg dpti_rst_meta;
  reg dpti_rst;

  always @(posedge dpti_clkout) begin
    if (!ui_rst_n) begin
      dpti_rst_meta <= 1'b1;
      dpti_rst      <= 1'b1;
    end
    else begin
      dpti_rst_meta <= 1'b0;
      dpti_rst      <= dpti_rst_meta;
    end
  end

  // ------------------------------------------------------------------------
  // Physical-input diagnostic state.
  //
  // This state is generated entirely in dpti_clkout.  Do not sample the
  // short physical-interface events directly from ui_clk.
  //
  // bit0: dpti_clkout has run after reset release
  // bit1: input data was reported available
  // bit2: the receiver asserted its physical read control
  // bit3: the receiver produced a valid byte
  // bit4: one byte completed the frontend valid/ready handshake
  // ------------------------------------------------------------------------

  reg [31:0] dpti_phy_debug;

  // Free-running physical-clock activity counter.
  //
  // Deliberately NOT reset by dpti_rst or ui_rst_n.  This is observation-only:
  // if dpti_clkout is reaching the FPGA, this counter must change regardless of
  // the state of the physical-interface reset synchronizer.
  reg [31:0] dpti_clk_activity = 32'd0;

  always @(posedge dpti_clkout) begin
    dpti_clk_activity <= dpti_clk_activity + 32'd1;
  end

  always @(posedge dpti_clkout) begin
    if (dpti_rst) begin
      dpti_phy_debug <= 32'd0;
    end
    else begin
      dpti_phy_debug[0] <= 1'b1;

      if (!dpti_rxf_n)
        dpti_phy_debug[1] <= 1'b1;

      if (!dpti_rd_n)
        dpti_phy_debug[2] <= 1'b1;

      if (dpti_byte_valid)
        dpti_phy_debug[3] <= 1'b1;

      if (dpti_byte_valid && dpti_byte_ready)
        dpti_phy_debug[4] <= 1'b1;
    end
  end

  // Observation-only CDC into ui_clk.
  //
  // dpti_phy_debug is sticky: bits only transition 0 -> 1 after reset.
  // The synchronized copy is used only for diagnostics, never for control.
  (* ASYNC_REG = "TRUE" *) reg [31:0] dpti_phy_debug_meta;
  (* ASYNC_REG = "TRUE" *) reg [31:0] dpti_phy_debug_sync;

  // Observation-only sampling of the free-running physical-clock counter.
  // Exact numeric coherence is not required here; we only compare successive
  // observations to determine whether the source clock is advancing.
  (* ASYNC_REG = "TRUE" *) reg [31:0] dpti_clk_activity_meta;
  (* ASYNC_REG = "TRUE" *) reg [31:0] dpti_clk_activity_sync;

  always @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n) begin
      dpti_phy_debug_meta     <= 32'd0;
      dpti_phy_debug_sync     <= 32'd0;
      dpti_clk_activity_meta  <= 32'd0;
      dpti_clk_activity_sync  <= 32'd0;
    end
    else begin
      dpti_phy_debug_meta     <= dpti_phy_debug;
      dpti_phy_debug_sync     <= dpti_phy_debug_meta;
      dpti_clk_activity_meta  <= dpti_clk_activity;
      dpti_clk_activity_sync  <= dpti_clk_activity_meta;
    end
  end

  // ------------------------------------------------------------------------
  // Result output path.
  //
  // DDR readback beats originate in ui_clk.  Cross them through an
  // asynchronous FIFO before serializing them in the physical-interface
  // clock domain.
  // ------------------------------------------------------------------------

  wire [AXI_DATA_W-1:0] rb_cdc_dst_data;
  wire                  rb_cdc_dst_valid;
  wire                  rb_cdc_dst_ready;

  wire [7:0]            rb_tx_byte_data;
  wire                  rb_tx_byte_valid;
  wire                  rb_tx_byte_ready;

  dpti_output_cdc #(
    .DATA_W (AXI_DATA_W),
    .DEPTH  (8)
  ) u_dpti_output_cdc (
    .src_clk   (ui_clk),
    .src_rst   (!ui_rst_n),

    .src_data  (dst_wr_data),
    .src_valid (rb_dst_wr_en),
    .src_ready (rb_cdc_src_ready),

    .dst_clk   (dpti_clkout),
    .dst_rst   (dpti_rst),

    .dst_data  (rb_cdc_dst_data),
    .dst_valid (rb_cdc_dst_valid),
    .dst_ready (rb_cdc_dst_ready)
  );

  dpti_beat_to_byte #(
    .DATA_W (AXI_DATA_W)
  ) u_dpti_beat_to_byte (
    .clk        (dpti_clkout),
    .rst        (dpti_rst),

    .beat_data  (rb_cdc_dst_data),
    .beat_valid (rb_cdc_dst_valid),
    .beat_ready (rb_cdc_dst_ready),

    .byte_data  (rb_tx_byte_data),
    .byte_valid (rb_tx_byte_valid),
    .byte_ready (rb_tx_byte_ready)
  );

  dpti_byte_tx u_dpti_byte_tx (
    .clk         (dpti_clkout),
    .rst         (dpti_rst),

    .byte_data   (rb_tx_byte_data),
    .byte_valid  (rb_tx_byte_valid),
    .byte_ready  (rb_tx_byte_ready),

    .dpti_txe_n  (dpti_txe_n),

    .dpti_d_out  (dpti_tx_data),
    .dpti_d_oe   (dpti_tx_oe),
    .dpti_wr_n   (dpti_tx_wr_n)
  );

  dpti_byte_rx u_dpti_byte_rx (
    .dpti_clkout (dpti_clkout),
    .rst         (dpti_rst),

    .dpti_d_in   (dpti_d),
    .dpti_rxf_n  (dpti_rxf_n),

    .dpti_rd_n   (dpti_rd_n),
    .dpti_oe_n   (dpti_oe_n),

    .byte_data   (dpti_byte_data),
    .byte_valid  (dpti_byte_valid),
    .byte_ready  (dpti_byte_ready)
  );

  dpti_command_frontend #(
    .ADDR_W        (8),
    .MEM_ADDR_W    (AXI_ADDR_W),
    .MEM_DATA_W    (AXI_DATA_W),
    .PAYLOAD_BYTES (RX_BYTES)
  ) u_dpti_command_frontend (
    .clk                (dpti_clkout),
    .rst                (dpti_rst),

    .byte_data          (dpti_byte_data),
    .byte_valid         (dpti_byte_valid),
    .byte_ready         (dpti_byte_ready),

    .cmd_valid          (dpti_host_cmd_valid),
    .cmd_ready          (dpti_host_cmd_ready),
    .cmd_addr           (dpti_host_cmd_addr),
    .cmd_data           (dpti_host_cmd_data),

    .mem_start          (dpti_mem_start),
    .mem_base_addr      (dpti_mem_base_addr),
    .mem_data           (dpti_mem_data),
    .mem_valid          (dpti_mem_valid),
    .mem_ready          (dpti_mem_ready),
    .mem_done           (dpti_mem_done),

    .err_opcode         (dpti_frontend_err_opcode),
    .err_write32_opcode (dpti_frontend_err_write32_opcode),
    .err_mem_opcode     (dpti_frontend_err_mem_opcode),
    .err_mem_length     (dpti_frontend_err_mem_length)
  );

  // MEM_WRITE payload crosses into ui_clk and is staged into DDR.
  dpti_mem_write_cdc_engine #(
    .AXI_DATA_W  (AXI_DATA_W),
    .AXI_ADDR_W  (AXI_ADDR_W),
    .AXI_ID_W    (2),
    .FIFO_DEPTH  (8),
    .BURST_LEN   (16),
    .TOTAL_BEATS (N_BEATS)
  ) u_dpti_mem_write_cdc_engine (
    .src_clk       (dpti_clkout),
    .src_rst       (dpti_rst),

    .src_start     (dpti_mem_start),
    .src_base_addr (dpti_mem_base_addr),
    .src_data      (dpti_mem_data),
    .src_valid     (dpti_mem_valid),
    .src_ready     (dpti_mem_ready),

    .ui_clk        (ui_clk),
    .ui_rst_n      (ui_rst_n),

    .busy          (hs_busy),
    .done          (hs_done),
    .err_protocol  (hs_err_protocol),
    .err_align     (hs_err_align),
    .err_resp      (hs_err_resp),

    .m_axi_awid    (hs_awid),
    .m_axi_awaddr  (hs_awaddr),
    .m_axi_awlen   (hs_awlen),
    .m_axi_awsize  (hs_awsize),
    .m_axi_awburst (hs_awburst),
    .m_axi_awlock  (hs_awlock),
    .m_axi_awcache (hs_awcache),
    .m_axi_awprot  (hs_awprot),
    .m_axi_awqos   (hs_awqos),
    .m_axi_awvalid (hs_awvalid),
    .m_axi_awready (hs_awready),

    .m_axi_wdata   (hs_wdata),
    .m_axi_wstrb   (hs_wstrb),
    .m_axi_wlast   (hs_wlast),
    .m_axi_wvalid  (hs_wvalid),
    .m_axi_wready  (hs_wready),

    .m_axi_bid     (bid),
    .m_axi_bresp   (bresp),
    .m_axi_bvalid  (hs_bvalid),
    .m_axi_bready  (hs_bready)
  );

  // ------------------------------------------------------------------------
  // DPTI -> ui_clk asynchronous command FIFO.
  // ------------------------------------------------------------------------

  dpti_cmd_async_fifo #(
    .ADDR_W(8),
    .DEPTH (4)
  ) u_dpti_cmd_async_fifo (
    .wr_clk   (dpti_clkout),
    .wr_rst   (dpti_rst),

    .wr_valid (dpti_host_cmd_valid),
    .wr_ready (dpti_fifo_wr_ready),

    .wr_addr  (dpti_host_cmd_addr),
    .wr_data  (dpti_host_cmd_data),

    .rd_clk   (ui_clk),
    .rd_rst   (!ui_rst_n),

    .rd_valid (dpti_fifo_rd_valid),
    .rd_ready (dpti_fifo_rd_ready),

    .rd_addr  (dpti_fifo_rd_addr),
    .rd_data  (dpti_fifo_rd_data)
  );

  assign dpti_host_cmd_ready = dpti_fifo_wr_ready;

  // ------------------------------------------------------------------------
  // ui_clk-domain AXI4-Lite master.
  // ------------------------------------------------------------------------

  assign dpti_axi_cmd_valid = dpti_fifo_rd_valid;
  assign dpti_fifo_rd_ready = dpti_axi_cmd_ready;

  dpti_axi4lite_master #(
    .ADDR_W(8)
  ) u_dpti_axi4lite_master (
    .clk       (ui_clk),
    .rst       (!ui_rst_n),

    .cmd_valid (dpti_axi_cmd_valid),
    .cmd_ready (dpti_axi_cmd_ready),

    .cmd_addr  (dpti_fifo_rd_addr),
    .cmd_data  (dpti_fifo_rd_data),

    .rsp_valid (dpti_axi_rsp_valid),
    .rsp_ready (dpti_axi_rsp_ready),
    .rsp_resp  (dpti_axi_rsp_resp),

    .m_axi_awaddr  (dpti_axi_awaddr),
    .m_axi_awvalid (dpti_axi_awvalid),
    .m_axi_awready (dpti_axi_awready),

    .m_axi_wdata   (dpti_axi_wdata),
    .m_axi_wstrb   (dpti_axi_wstrb),
    .m_axi_wvalid  (dpti_axi_wvalid),
    .m_axi_wready  (dpti_axi_wready),

    .m_axi_bresp   (dpti_axi_bresp),
    .m_axi_bvalid  (dpti_axi_bvalid),
    .m_axi_bready  (dpti_axi_bready)
  );

  // The current DPTI AXI master only performs writes.
  // Completion is consumed locally for now.  A later TX path can expose
  // the response to Ubuntu.
  assign dpti_axi_rsp_ready = 1'b1;

  // ------------------------------------------------------------------------
  // Physical DPTI AXI4-Lite master drives the existing AXI4-Lite slave
  // bridge.
  // ------------------------------------------------------------------------
  // ----------------------------------------------------------
  // AXI4-Lite -> generic DPTI register-write transport.
  //
  // AXI4-Lite is terminated here.  The descriptor bridge below
  // remains independent of the physical host transport.
  // ----------------------------------------------------------

  // ------------------------------------------------------------------------
  // AXI4-Lite slave termination.
  //
  // The physical DPTI AXI4-Lite master is the AXI source.
  // This bridge converts AXI4-Lite into the generic register-write
  // transaction consumed by dpti_descriptor_bridge.
  // ------------------------------------------------------------------------

  wire        axi_dpti_wr_valid;
  wire        axi_dpti_wr_ready;
  wire [7:0]  axi_dpti_wr_addr;
  wire [31:0] axi_dpti_wr_data;

  // ----------------------------------------------------------
  // Internal AXI4-Lite bus driven by the DPTI host master.
  //
  // DPTI host bytes are converted into register-write commands,
  // then this master generates a genuine AXI4-Lite transaction.
  //
  // No AXI4-Lite signal leaves the FPGA top-level.
  // ----------------------------------------------------------

  // AW/W/B signals are declared above at the physical DPTI
  // AXI4-Lite master.  The bridge below consumes that same bus.

  wire [7:0]  dpti_axi_araddr;
  wire        dpti_axi_arvalid;
  wire        dpti_axi_arready;

  wire [31:0] dpti_axi_rdata;
  wire [1:0]  dpti_axi_rresp;
  wire        dpti_axi_rvalid;
  wire        dpti_axi_rready;

  // ----------------------------------------------------------
  // DPTI command source -> AXI4-Lite master.
  //
  // The command source will be connected to the physical DPTI
  // RX/FIFO path.  Until then these command signals remain an
  // internal protocol boundary.
  // ----------------------------------------------------------

  // ----------------------------------------------------------
  // Internal AXI4-Lite master -> DPTI descriptor register bank.
  // ----------------------------------------------------------

  axi4lite_dpti_bridge #(
    .ADDR_W(8)
  ) u_axi4lite_dpti_bridge (
    .aclk          (ui_clk),
    .aresetn       (ui_rst_n),

    .s_axi_awaddr  (dpti_axi_awaddr),
    .s_axi_awvalid (dpti_axi_awvalid),
    .s_axi_awready (dpti_axi_awready),

    .s_axi_wdata   (dpti_axi_wdata),
    .s_axi_wstrb   (dpti_axi_wstrb),
    .s_axi_wvalid  (dpti_axi_wvalid),
    .s_axi_wready  (dpti_axi_wready),

    .s_axi_bresp   (dpti_axi_bresp),
    .s_axi_bvalid  (dpti_axi_bvalid),
    .s_axi_bready  (dpti_axi_bready),

    .s_axi_araddr  (dpti_axi_araddr),
    .s_axi_arvalid (dpti_axi_arvalid),
    .s_axi_arready (dpti_axi_arready),

    .s_axi_rdata   (dpti_axi_rdata),
    .s_axi_rresp   (dpti_axi_rresp),
    .s_axi_rvalid  (dpti_axi_rvalid),
    .s_axi_rready  (dpti_axi_rready),

    .wr_valid      (axi_dpti_wr_valid),
    .wr_ready      (axi_dpti_wr_ready),
    .wr_addr       (axi_dpti_wr_addr),
    .wr_data       (axi_dpti_wr_data),

    .status        (dpti_status)
  );

  dpti_descriptor_bridge u_dpti_descriptor_bridge (
    .clk             (ui_clk),
    .rst             (!ui_rst_n),

    .wr_valid        (axi_dpti_wr_valid),
    .wr_ready        (axi_dpti_wr_ready),
    .wr_addr         (axi_dpti_wr_addr),
    .wr_data         (axi_dpti_wr_data),

    .job_valid       (dpti_job_valid),
    .job_ready       (dpti_job_ready),

    .job_id          (dpti_job_id),
    .job_device_id   (dpti_job_device_id),

    .job_m           (dpti_job_m),
    .job_n           (dpti_job_n),
    .job_k           (dpti_job_k),

    .job_start_cycle (dpti_job_start_cycle),
    .job_est_cycles  (dpti_job_est_cycles),

    .job_a_base      (dpti_job_a_base),
    .job_b_base      (dpti_job_b_base),
    .job_c_base      (dpti_job_c_base),

    .status          (dpti_status)
  );

  // ----------------------------------------------------------
  // Descriptor source arbitration.
  //
  // DPTI has priority when a descriptor is pending.  Otherwise
  // preserve the existing external job producer unchanged.
  //
  // There is intentionally only ONE descriptor entering the
  // existing ingress path.
  // ----------------------------------------------------------

  wire        effective_job_valid;
  wire [31:0] effective_job_id;
  wire [31:0] effective_job_device_id;

  wire [31:0] effective_job_m;
  wire [31:0] effective_job_n;
  wire [31:0] effective_job_k;

  wire [31:0] effective_job_start_cycle;
  wire [31:0] effective_job_est_cycles;

  wire [63:0] effective_job_a_base;
  wire [63:0] effective_job_b_base;
  wire [63:0] effective_job_c_base;

  // Downstream acceptance boundary.
  wire descriptor_downstream_ready;

  assign descriptor_downstream_ready =
      ingress_job_ready &&
      core_job_ready &&
      scheduler_desc_ready;

  // DPTI descriptor gets priority over the legacy external
  // descriptor producer.
  assign effective_job_valid =
      dpti_job_valid ||
      (USE_LEGACY_JOB_PORTS && job_valid);

  assign effective_job_id =
      dpti_job_valid ? dpti_job_id :
      (USE_LEGACY_JOB_PORTS ? job_id : 32'd0);

  assign effective_job_device_id =
      dpti_job_valid ? dpti_job_device_id :
      (USE_LEGACY_JOB_PORTS ? job_device_id : 32'd0);

  assign effective_job_m =
      dpti_job_valid ? dpti_job_m :
      (USE_LEGACY_JOB_PORTS ? job_m : 32'd0);

  assign effective_job_n =
      dpti_job_valid ? dpti_job_n :
      (USE_LEGACY_JOB_PORTS ? job_n : 32'd0);

  assign effective_job_k =
      dpti_job_valid ? dpti_job_k :
      (USE_LEGACY_JOB_PORTS ? job_k : 32'd0);

  assign effective_job_start_cycle =
      dpti_job_valid ? dpti_job_start_cycle :
      (USE_LEGACY_JOB_PORTS ? job_start_cycle : 32'd0);

  assign effective_job_est_cycles =
      dpti_job_valid ? dpti_job_est_cycles :
      (USE_LEGACY_JOB_PORTS ? job_est_cycles : 32'd0);

  assign effective_job_a_base =
      dpti_job_valid ? dpti_job_a_base :
      (USE_LEGACY_JOB_PORTS ? job_a_base : 64'd0);

  assign effective_job_b_base =
      dpti_job_valid ? dpti_job_b_base :
      (USE_LEGACY_JOB_PORTS ? job_b_base : 64'd0);

  assign effective_job_c_base =
      dpti_job_valid ? dpti_job_c_base :
      (USE_LEGACY_JOB_PORTS ? job_c_base : 64'd0);

  // DPTI is accepted only when its descriptor is actually selected
  // and the complete downstream path is ready.
  assign dpti_job_ready =
      dpti_job_valid &&
      descriptor_downstream_ready;

  // ----------------------------------------------------------
  // Job ingress.
  //
  // The external descriptor producer sees job_valid/job_ready.
  // The DMA/scheduler core consumes the one-entry elastic buffer.
  // ----------------------------------------------------------

  wire        ingress_job_valid;
  wire        ingress_job_ready;

  wire [31:0] ingress_job_id;
  wire [31:0] ingress_job_device_id;

  wire [31:0] ingress_job_m;
  wire [31:0] ingress_job_n;
  wire [31:0] ingress_job_k;

  wire [31:0] ingress_job_start_cycle;
  wire [31:0] ingress_job_est_cycles;

  wire [63:0] ingress_job_a_base;
  wire [63:0] ingress_job_b_base;
  wire [63:0] ingress_job_c_base;

  systolic_job_ingress u_job_ingress (
    .clk              (ui_clk),
    .rst_n            (ui_rst_n),

    .in_valid         (effective_job_valid),
    .in_ready         (ingress_job_ready),

    .in_job_id        (effective_job_id),
    .in_device_id     (effective_job_device_id),

    .in_m             (effective_job_m),
    .in_n             (effective_job_n),
    .in_k             (effective_job_k),

    .in_start_cycle   (effective_job_start_cycle),
    .in_est_cycles    (effective_job_est_cycles),

    .in_a_base        (effective_job_a_base),
    .in_b_base        (effective_job_b_base),
    .in_c_base        (effective_job_c_base),

    .consumer_ready   (core_job_ready),
    .job_valid        (ingress_job_valid),

    .job_id           (ingress_job_id),
    .job_device_id    (ingress_job_device_id),

    .job_m            (ingress_job_m),
    .job_n            (ingress_job_n),
    .job_k            (ingress_job_k),

    .job_start_cycle  (ingress_job_start_cycle),
    .job_est_cycles   (ingress_job_est_cycles),

    .job_a_base       (ingress_job_a_base),
    .job_b_base       (ingress_job_b_base),
    .job_c_base       (ingress_job_c_base)
  );

  // External producer handshake must reflect the complete acceptance
  // boundary. A descriptor is not accepted by the compiler-facing
  // interface unless both the DMA core and hardware scheduler can
  // consume it.
  // External producer acceptance is the job ownership boundary.
  //
  // The compiler-facing descriptor is accepted only when the core and
  // hardware scheduler are both ready.  Do not derive job_fire from the
  // registered ingress output: that output may still contain the descriptor
  // of the job that just completed.
  // Legacy external producer readiness.
  //
  // If a DPTI descriptor is pending, the legacy producer is not
  // allowed to believe that its descriptor was accepted.
  assign job_ready =
      USE_LEGACY_JOB_PORTS &&
      !dpti_job_valid &&
      descriptor_downstream_ready;

  // A descriptor is accepted at the common downstream boundary.
  wire job_fire =
      effective_job_valid &&
      descriptor_downstream_ready;

  // The scheduler accepts exactly the same accepted descriptor.
  wire scheduler_desc_fire =
      job_fire;


  // --------------------------------------------------------------------------
  // --------------------------------------------------------------------------
  // DEBUG: trace every actual job_fire together with the complete readiness
  // seam. Diagnostic only; no functional signal is modified.
  //
  // This captures the descriptor handshake boundary so we can determine
  // whether the ingress descriptor is duplicated, advanced too late, or
  // accepted at an unexpected job boundary.
  // --------------------------------------------------------------------------
  always_ff @(posedge ui_clk) begin
    if (USE_EXTERNAL_SCHEDULER &&
        (job_fire ||
         ingress_job_valid ||
         ingress_job_ready ||
         job_ready)) begin

      $display(
        "JOBFIREDBG t=%0t phase=%0d active=%0b busy=%0b job_reg=%0d ingress_id=%0d in_valid=%0b in_ready=%0b job_ready=%0b core_ready=%0b sched_ready=%0b fire=%0b same_id=%0b",
        $time,
        phase,
        job_active,
        job_busy,
        job_id_reg,
        ingress_job_id,
        ingress_job_valid,
        ingress_job_ready,
        job_ready,
        core_job_ready,
        scheduler_desc_ready,
        job_fire,
        (ingress_job_id == job_id_reg)
      );
    end
  end

  // --------------------------------------------------------------------------
  // DEBUG: observe compiler -> scheduler descriptor handshake.
  // --------------------------------------------------------------------------
  always_ff @(posedge ui_clk) begin
    if (USE_EXTERNAL_SCHEDULER &&
        (job_fire || scheduler_desc_fire || scheduler_desc_ready || scheduler_busy)) begin
      $display(
        "SCHEDHS t=%0t phase=%0d job_fire=%0b sched_fire=%0b desc_valid=%0b desc_ready=%0b state=%0d busy=%0b ing_id=%0d ing_dev=%0d ing_start=%0d ing_est=%0d ing_C=0x%08h",
        $time,
        phase,
        job_fire,
        scheduler_desc_fire,
        ingress_job_valid,
        scheduler_desc_ready,
        u_hw_scheduler.state,
        scheduler_busy,
        ingress_job_id,
        ingress_job_device_id,
        ingress_job_start_cycle,
        ingress_job_est_cycles,
        ingress_job_c_base
      );
    end
  end

  // --------------------------------------------------------------------------
  // DEBUG: print the memory bases of every externally accepted compiler job.
  // This is diagnostic only; it does not affect RTL behavior.
  // --------------------------------------------------------------------------
  always_ff @(posedge ui_clk) begin
    if (USE_EXTERNAL_SCHEDULER && job_fire) begin
      $display(
        "JOBADDRDBG accept: ingress_job_id=%0d a_base=0x%08h b_base=0x%08h c_base=0x%08h",
        ingress_job_id,
        ingress_job_a_base,
        ingress_job_b_base,
        ingress_job_c_base
      );
    end
  end

  // --------------------------------------------------------------------------
  // DEBUG: scheduler protocol trace.
  //
  // This trace follows one descriptor from compiler acceptance through
  // scheduler start and accelerator completion.  It is diagnostic only.
  // --------------------------------------------------------------------------
  always_ff @(posedge ui_clk) begin
    if (USE_EXTERNAL_SCHEDULER &&
        (job_fire ||
         scheduler_desc_ready ||
         scheduler_busy ||
         scheduler_accelerator_start != 0 ||
         scheduler_c_done ||
         scheduler_schedule_done ||
         scheduler_start_pending ||
         scheduler_accelerator_start_delayed != 0)) begin

      $display(
        "SCHTRACE t=%0t phase=%0d fire=%0b desc_ready=%0b sched_state=%0d sched_busy=%0b sched_cyc=%0d active_start=%0d active_est=%0d active_dev=%0d start=%b pending=%0b pending_id=%0d delayed=%b fold_start=%0b c_done=%0b c_armed=%0b c_seen=%0b sched_c_done=%0b sched_done=%0b",
        $time,
        phase,
        job_fire,
        scheduler_desc_ready,
        u_hw_scheduler.state,
        scheduler_busy,
        scheduler_cycle_counter,
        scheduler_active_start_cycle,
        scheduler_active_compute_cycles,
        scheduler_active_accelerator_id,
        scheduler_accelerator_start,
        scheduler_start_pending,
        scheduler_start_pending_id,
        scheduler_accelerator_start_delayed,
        fold_start,
        c_done,
        c_done_armed,
        c_done_seen,
        scheduler_c_done,
        scheduler_schedule_done
      );
    end
  end


  // --------------------------------------------------------------------------
  // DEBUG: first-failure scheduler -> datapath ownership seam.
  //
  // Goal:
  //   Determine whether an accelerator/fold transaction for a newly
  //   presented ingress job starts before job_fire latches that job's
  //   descriptor into the job_*_reg registers.
  //
  // Diagnostic only. No functional signal is modified.
  // --------------------------------------------------------------------------
  always_ff @(posedge ui_clk) begin
    if (USE_EXTERNAL_SCHEDULER &&
        (job_fire ||
         fold_start ||
         scheduler_accelerator_start != 0 ||
         wb_desc_valid)) begin

      $display(
        "FIRSTFAIL_SEAM t=%0t phase=%0d job_fire=%0b job_active=%0b job_id_reg=%0d ingress_job_id=%0d ingress_c_base=0x%08h job_c_base_reg=0x%08h fold_start=%0b sched_start=%b sched_busy=%0b sched_state=%0d wb_valid=%0b wb_ready=%0b wb_tile_addr=0x%08h",
        $time,
        phase,
        job_fire,
        job_active,
        job_id_reg,
        ingress_job_id,
        ingress_job_c_base,
        job_c_base_reg,
        fold_start,
        scheduler_accelerator_start,
        scheduler_busy,
        u_hw_scheduler.state,
        wb_desc_valid,
        wb_desc_ready,
        wb_tile_addr
      );
    end
  end

  // --------------------------------------------------------------------------
  // DEBUG: observe the complete external job handshake.
  //
  // Diagnostic only. No functional signal is modified.
  // --------------------------------------------------------------------------
  always_ff @(posedge ui_clk) begin
    if (USE_EXTERNAL_SCHEDULER &&
        (job_valid || ingress_job_valid || job_fire)) begin
      $display("JOBHS t=%0t phase=%0d busy=%0b in_valid=%0b in_ready=%0b ing_valid=%0b ing_ready=%0b core_ready=%0b sched_ready=%0b fire=%0b in_id=%0d ing_id=%0d dev=%0d",
        $time,
        phase,
        job_busy,
        job_valid,
        ingress_job_ready,
        ingress_job_valid,
        ingress_job_ready,
        core_job_ready,
        scheduler_desc_ready,
        job_fire,
        job_id,
        ingress_job_id,
        ingress_job_device_id
      );
    end
  end

  // Job-level completion is defined later, after the write-back
  // completion signal and final-invocation predicate are available.
  wire job_done;

  // DEBUG: observe the exact job completion condition.
  always_ff @(posedge ui_clk) begin
    if (job_active && scheduler_c_done) begin
      $display(
        "JOBDONE_DBG t=%0t job_active=%0b scheduler_c_done=%0b job_done=%0b job_id=%0d",
        $time,
        job_active,
        scheduler_c_done,
        job_done,
        job_id_reg
      );
    end
  end

  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n) begin
      job_busy       <= 1'b0;
      job_active     <= 1'b0;
      job_id_reg     <= 32'd0;
      job_device_id_reg <= 32'd0;
      job_m_reg      <= 32'd0;
      job_n_reg      <= 32'd0;
      job_k_reg      <= 32'd0;
      job_start_cycle_reg <= 32'd0;
      job_est_cycles_reg  <= 32'd0;
      job_a_base_reg <= 64'd0;
      job_b_base_reg <= 64'd0;
      job_c_base_reg <= 64'd0;
    end
    else begin
      if (job_fire || job_done) begin
        $display(
          "JOBEDGE_DBG t=%0t job_fire=%0b job_done=%0b job_active=%0b job_busy=%0b job_id_reg=%0d ingress_job_id=%0d",
          $time,
          job_fire,
          job_done,
          job_active,
          job_busy,
          job_id_reg,
          ingress_job_id
        );
      end

      if (job_fire) begin
        // Acceptance is the job boundary.
        job_busy       <= 1'b1;
        job_active     <= 1'b1;

        job_id_reg     <= effective_job_id;
        job_device_id_reg <= effective_job_device_id;
        job_m_reg      <= effective_job_m;
        job_n_reg      <= effective_job_n;
        job_k_reg      <= effective_job_k;
        job_start_cycle_reg <= effective_job_start_cycle;
        job_est_cycles_reg  <= effective_job_est_cycles;
        job_a_base_reg <= effective_job_a_base;
        job_b_base_reg <= effective_job_b_base;
        if (ingress_job_id == 32'd102) begin
          $display(
            "JOB2_CBASECAP t=%0t job_fire=%0b ingress_job_id=%0d ingress_c_base=0x%08h c_base_reg_before=0x%08h",
            $time,
            job_fire,
            ingress_job_id,
            ingress_job_c_base,
            job_c_base_reg
          );
        end
        job_c_base_reg <= effective_job_c_base;
      end

      // Completion closes exactly the job that was accepted.
      if (job_done) begin
        $display(
          "JOBCLOSE_DBG t=%0t job_id=%0d job_done=%0b job_active=%0b job_busy=%0b",
          $time,
          job_id_reg,
          job_done,
          job_active,
          job_busy
        );

        job_busy   <= 1'b0;
        job_active <= 1'b0;
      end
    end
  end

  // Scheduler-facing completion is a JOB event, not the raw c_done level.
  //
  // c_done is sticky until the next fold_start.  Therefore exposing c_done
  // directly would allow a stale completion from the previous job to look
  // like completion of the next job.
  //
  // c_done_seen is cleared when a new job is accepted and set after the first
  // completion belonging to that job.
  logic        c_done_seen;
  logic        c_done_armed;

  // Scheduler-facing completion is a one-cycle event.
  //
  // c_done itself is sticky until the next fold_start, so do not expose the
  // raw level to the scheduler.  Arm completion only after the real
  // accelerator transaction has started, then consume the first c_done.
  assign scheduler_c_done =
      USE_EXTERNAL_SCHEDULER &&
      job_active &&
      c_done_armed &&
      c_done &&
      !c_done_seen;

  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n) begin
      c_done_seen  <= 1'b0;
      c_done_armed <= 1'b0;
    end
    else begin
      // A new job invalidates any completion state from the previous job.
      if (job_fire) begin
        c_done_seen  <= 1'b0;
        c_done_armed <= 1'b0;
      end

      // fold_start is the actual accelerator transaction boundary.
      else if (fold_start) begin
        c_done_seen  <= 1'b0;
        c_done_armed <= 1'b1;
      end

      // Consume exactly one completion event.
      else if (job_active &&
               c_done_armed &&
               c_done &&
               !c_done_seen) begin
        c_done_seen  <= 1'b1;
        c_done_armed <= 1'b0;
      end
    end
  end

  // ---- write-back control -------------------------------------------------
  logic          wb_desc_valid;
  wire           wb_desc_ready, wb_done;
  wire           wb_err_align, wb_err_resp;
  logic          wb_desc_started, wb_done_sticky;
  logic          err_w_owner;
  // ----------------------------------------------------------
  // Device-selected result geometry.
  //
  // IMPORTANT:
  //   ingress_job_device_id remains an opaque physical-instance ID.
  //   Geometry is a property of the selected device, not of the
  //   scheduler ABI.
  //
  // Current prototype device table:
  //
  //   DEVICE_ID_8X8   -> 64 result words -> 256 bytes -> 16 AXI beats
  //   DEVICE_ID_4X4_0 -> 16 result words ->  64 bytes ->  4 AXI beats
  //   DEVICE_ID_4X4_1 -> 16 result words ->  64 bytes ->  4 AXI beats
  //   DEVICE_ID_4X4_2 -> 16 result words ->  64 bytes ->  4 AXI beats
  //
  // These are device properties.  The scheduler only supplies
  // device_id; it does not supply "use_4x4".
  // ----------------------------------------------------------

  localparam integer WB_BEATS_8X8      = (8 * 8) / (AXI_DATA_W / 32);
  localparam integer WB_TILE_BYTES_8X8 = 8 * 8 * 4;

  localparam integer WB_BEATS_4X4      = (4 * 4) / (AXI_DATA_W / 32);
  localparam integer WB_TILE_BYTES_4X4 = 4 * 4 * 4;

  wire [31:0] selected_result_words =
      select_8x8 ? 32'd64 :
      select_4x4 ? 32'd16 :
                   32'd0;

  wire [31:0] selected_result_bytes =
      select_8x8 ? 32'd256 :
      select_4x4 ? 32'd64 :
                   32'd0;

  wire [31:0] selected_wb_beats =
      select_8x8 ? 32'd16 :
      select_4x4 ? 32'd4 :
                   32'd0;

  // Both supported geometries have power-of-two tile sizes.
  // Keep separate shifts so the address calculation remains
  // compile-time simple rather than introducing a runtime shift.
  localparam int WBT_SHIFT_8X8 = $clog2(WB_TILE_BYTES_8X8);
  localparam int WBT_SHIFT_4X4 = $clog2(WB_TILE_BYTES_4X4);

  localparam int RX_SHIFT  = $clog2(RX_BYTES);
  localparam int RXW_SHIFT = $clog2(RX_WORDS);

  // P_READ ends when the last word has LANDED, not when the last beat has been
  // received.  dma_operand_writer takes four cycles to unpack a 128-bit beat
  // into the single 32-bit buffer write port, so read_done leads the final
  // write by up to three cycles.  Starting the fold on read_done would race the
  // last three operands into the array -- intermittently, and only at the tail
  // of the payload, which is the worst kind of bug to hunt on a board.
  // words_written is exact.
  // words_written is cumulative (the writer's clear is tied low), so the
  // finishing line moves one slab per invocation: invocation fi is full when
  // the (fi+1)-th slab has landed.  At n_inv = 1 this is the old comparison.
  wire [31:0] words_want =
      (32'(fi) + 32'd1) * job_rx_words;
  wire fill_complete = read_done_fold && (words_written == words_want);

  localparam integer C_N = N * N;
  logic [2*LANE_W:0] scan_c;
  wire scan_c_last = (scan_c == (2*LANE_W+1)'(C_N));

  // ---- how many invocations this run makes --------------------------------
  // K > K_MAX is a scheduling quantity, not a hardware capacity: the operand
  // buffers are indexed by absolute k and no fold count appears in them.
  // uart/build_kmax.tcl says why a top-level NFOLD generic was the wrong
  // abstraction and asks that it not come back under another name, so the count
  // arrives from OUTSIDE the RTL at run time -- vio_0's one output probe -- and
  // ONE bitstream measures the whole sweep n_inv = 1..8.  What the sweep buys
  // over a single point is the slope: the per-descriptor start-up inside the
  // fill is measured rather than inferred from a single fill's excess.
  //
  // Latched on the way out of P_CALIB.  A probe written mid-run would move the
  // finishing line under a measurement, and every counter below is defined over
  // "this run".  0 reads as 1: an unset probe must not produce a run that does
  // nothing and reports it as a total.
  wire  [3:0] n_inv_probe;
  wire  [3:0] n_inv_next = (n_inv_probe == 4'd0) ? 4'd1 : n_inv_probe;
  // The second output probe re-runs the design from P_CALIB without a reset, so
  // the whole n_inv sweep is one script instead of eight presses of BTNC with a
  // project re-opened between them.  A rising edge, not a level: a level would
  // re-run for as long as JTAG left it high.  It is honoured only in P_DONE --
  // there is no way to interrupt a run in flight, which is the point.
  //
  // Everything a run measures is therefore cleared in P_CALIB rather than by
  // ui_rst_n alone: the counters below, both engines' own counters (stat_clear),
  // the writer's word count, the checksums and their expectations, and the four
  // done flags.  The rule is "P_CALIB is the start of a run"; a counter that
  // does not obey it would report the sum of every run since power-on and look
  // like a slow run.  err flags are the deliberate exception -- an alignment or
  // response fault is a property of the bitstream, not of one run, and stays
  // latched until reset.
  wire        rerun_probe;
  logic       rerun_d;
  wire        rerun_pulse = rerun_probe & ~rerun_d;
  logic [3:0] n_inv;
  logic [3:0] fi;                     // legacy serial invocation index
  wire        run_clear;              // = (phase == P_CALIB), assigned below
  wire        fi_last = (fi == n_inv - 4'd1);

  // A scheduler job is complete only after the final invocation has made
  // its result memory-visible.  Accelerator completion alone is too early:
  // P_SCAN and P_WB still have to execute after the array finishes.
  assign job_done =
      USE_EXTERNAL_SCHEDULER &&
      job_active &&
      wb_done &&
      fi_last;

  // ----------------------------------------------------------
  // Job-aware DMA address generation.
  //
  // Legacy mode keeps the original BASE_ADDR-based addressing.
  //
  // External scheduler mode makes the accepted job descriptor
  // the owner of the memory region:
  //
  //   job_a_base_reg -> operand/read region
  //   job_c_base_reg -> result/write-back region
  //
  // fi is still the invocation index within this accepted job.
  //
  // job_b_base_reg is latched but intentionally unused here.
  // The current operand path consumes one combined slab; separating
  // independent A/B streams is a later datapath change.
  // ----------------------------------------------------------

  wire [AXI_ADDR_W-1:0] slab_addr =
      USE_EXTERNAL_SCHEDULER
          ? AXI_ADDR_W'(job_a_base_reg) +
            (AXI_ADDR_W'(fi) << RX_SHIFT)
          : AXI_ADDR_W'(BASE_ADDR) +
            (AXI_ADDR_W'(fi) << RX_SHIFT);

  // --------------------------------------------------------------------------
  // DEBUG: observe the actual AXI read addresses issued by dma_engine.
  // Diagnostic only; no datapath/state modification.
  // --------------------------------------------------------------------------
  always_ff @(posedge ui_clk) begin
    if (USE_EXTERNAL_SCHEDULER && arvalid && arready) begin
      $display(
        "ARDBG: ingress_job_id=%0d araddr=0x%08h arlen=%0d",
        job_id_reg,
        araddr,
        arlen + 1
      );
    end
  end

  wire [AXI_ADDR_W-1:0] wb_region_base =
      USE_EXTERNAL_SCHEDULER
          ? AXI_ADDR_W'(job_c_base_reg)
          : AXI_ADDR_W'(BASE_ADDR) +
            (AXI_ADDR_W'(n_inv) << RX_SHIFT) +
            AXI_ADDR_W'(WB_GAP_BYTES);

  // Result tile stride belongs to the selected physical device.
  //
  // 8x8 device: one result tile = 256 bytes.
  // 4x4 device: one result tile = 64 bytes.
  //
  // The scheduler still supplies only the opaque device ID.
  wire [AXI_ADDR_W-1:0] wb_tile_addr =
      select_8x8
          ? wb_region_base + (AXI_ADDR_W'(fi) << WBT_SHIFT_8X8)
          : select_4x4
              ? wb_region_base + (AXI_ADDR_W'(fi) << WBT_SHIFT_4X4)
              : wb_region_base;

  // --------------------------------------------------------------------------
  // DEBUG: first-failure writeback address diagnostic.
  //
  // Only print when the WB descriptor is actually accepted.
  // This captures the exact state used for the transaction:
  //
  //   job_id_reg
  //   ingress_job_id
  //   job_c_base_reg
  //   wb_region_base
  //   fi
  //   wb_tile_addr
  //
  // Diagnostic only; no functional signal is modified.
  // --------------------------------------------------------------------------
  always_ff @(posedge ui_clk) begin
    if (USE_EXTERNAL_SCHEDULER &&
        wb_desc_valid && wb_desc_ready) begin
      $display(
        "WB_FIRE_DBG t=%0t job_reg=%0d ingress_job=%0d job_c_base_reg=0x%08h wb_region_base=0x%08h fi=%0d select8=%0b select4=%0b wb_tile_addr=0x%08h wb_beats=%0d",
        $time,
        job_id_reg,
        ingress_job_id,
        job_c_base_reg,
        wb_region_base,
        fi,
        select_8x8,
        select_4x4,
        wb_tile_addr,
        selected_wb_beats
      );
    end
  end


  // --------------------------------------------------------------------------
  // DEBUG: JOB1 writeback descriptor lifecycle.
  //
  // This isolates duplicate descriptor issuance for ingress job id 101.
  // We intentionally trace only the descriptor/control seam:
  //
  //   phase
  //   job_id_reg
  //   fi
  //   job_active
  //   job_done
  //   wb_desc_valid
  //   wb_desc_ready
  //   wb_desc_started
  //   wb_done
  //
  // Diagnostic only; no functional signal is modified.
  // --------------------------------------------------------------------------
  always_ff @(posedge ui_clk) begin
    if (USE_EXTERNAL_SCHEDULER &&
        (job_id_reg == 32'd101) &&
        (phase == P_WB ||
         wb_desc_valid ||
         wb_desc_ready ||
         wb_done ||
         job_done)) begin

      $display(
        "JOB1WBTRACE t=%0t phase=%0d fi=%0d job_id=%0d active=%0b done=%0b wb_valid=%0b wb_ready=%0b wb_fire=%0b wb_started=%0b wb_done=%0b c_base=0x%08h tile_addr=0x%08h",
        $time,
        phase,
        fi,
        job_id_reg,
        job_active,
        job_done,
        wb_desc_valid,
        wb_desc_ready,
        wb_desc_valid && wb_desc_ready,
        wb_desc_started,
        wb_done,
        job_c_base_reg,
        wb_tile_addr
      );
    end
  end

  // --------------------------------------------------------------------------
  // DEBUG: trace WB descriptor generation for the external-job path.
  //
  // The current failure shows a WB transaction for job_reg=101 occurring
  // before job 102 is accepted.  This diagnostic distinguishes:
  //
  //   1. phase entering P_WB
  //   2. wb_desc_valid being asserted
  //   3. wb_desc_started state
  //   4. actual descriptor handshake
  //
  // Diagnostic only; no functional signal is modified.
  // --------------------------------------------------------------------------
  always_ff @(posedge ui_clk) begin
    if (USE_EXTERNAL_SCHEDULER &&
        (phase == P_WB ||
         wb_desc_valid ||
         wb_desc_started ||
         (wb_desc_valid && wb_desc_ready))) begin
      $display(
        "WBSEAM_DBG t=%0t phase=%0d fi=%0d job_active=%0b job_id_reg=%0d ingress_job_id=%0d job_fire=%0b wb_valid=%0b wb_ready=%0b wb_started=%0b wb_done=%0b c_base_reg=0x%08h tile_addr=0x%08h",
        $time,
        phase,
        fi,
        job_active,
        job_id_reg,
        ingress_job_id,
        job_fire,
        wb_desc_valid,
        wb_desc_ready,
        wb_desc_started,
        wb_done,
        job_c_base_reg,
        wb_tile_addr
      );
    end
  end

  // The engine's completion, re-armed per invocation.  read_done_sticky stays
  // what it was -- a run-level flag for led[2] and probe_in4.
  logic read_done_fold;
  // And the array's.  c_done is a LEVEL: it goes high when the array publishes
  // C and stays high until the next fold_start clears it, which was sound when
  // there was only ever one fold.  On invocation 2 and after it is still high
  // from the previous invocation at the moment P_FOLD is entered, so the FSM
  // would leave P_FOLD at once and scan the previous C -- a run that looks
  // right (the operands are identical, so the tile is identical) and is
  // 147 cycles too fast.  This latch holds only the completion that belongs to
  // the invocation in flight.  The clear covers two cycles because fold_start
  // is registered: P_GO sets it, and it does not reach c_done until the first
  // cycle of P_FOLD, so that cycle is cleared too.
  logic c_done_fold;

  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n) begin
      phase         <= P_CALIB;
      n_inv         <= 4'd1;
      fi            <= 4'd0;
      seed_start    <= 1'b0;
      desc_valid    <= 1'b0;
      fsm_fold_start    <= 1'b0;
      wb_desc_valid <= 1'b0;
      scan_c        <= '0;
    end else begin
      seed_start    <= 1'b0;
      desc_valid    <= 1'b0;
      fsm_fold_start    <= 1'b0;
      wb_desc_valid <= 1'b0;
      case (phase)
        P_CALIB: if (init_calib_complete &&
                       (!USE_EXTERNAL_SCHEDULER || job_active)) begin
                   n_inv <= n_inv_next;
                   fi    <= 4'd0;

                   // Legacy bring-up owns synthetic operand generation.
                   // External/compiler jobs consume operand slabs already
                   // resident in DDR, so they must bypass the seed writer.
                   if (USE_EXTERNAL_SCHEDULER) begin
                     phase <= P_READ;
                   end else begin
                     seed_start <= 1'b1;
                     phase      <= P_SEED;
                   end
                 end

        // One seed pass per slab.  seed_start is registered, so the next pulse
        // lands the cycle AFTER seed_done, by which time dma_seed_writer is
        // back in S_IDLE and will take it.  Every slab carries the same pattern
        // (the writer restarts beat_idx and vbase on each start), which is what
        // makes the expected checksums n_inv times the tabulated ones.
        P_SEED:  if (seed_done) begin
                   if (fi_last) begin
                     fi    <= 4'd0;
                     phase <= P_READ;
                   end else begin
                     fi         <= fi + 4'd1;
                     seed_start <= 1'b1;
                   end
                 end
        P_READ:  begin
                   if (!desc_started) desc_valid <= 1'b1;
                   if (fill_complete) phase <= P_GO;
                 end
        P_GO:    begin
                   if (USE_EXTERNAL_SCHEDULER) begin
                     // External scheduler owns the accelerator release.
                     //
                     // The scheduler release may have happened before DMA
                     // reached P_GO.  Accept either:
                     //   1. the live one-cycle release pulse, or
                     //   2. an early release captured in the pending bit.
                     if (scheduler_start_pending ||
                         scheduler_fold_start_selected)
                       phase <= P_FOLD;
                   end
                   else begin
                     fsm_fold_start <= 1'b1;
                     phase <= P_FOLD;
                   end
                 end
        P_FOLD:  if (!fold_start && c_done_fold) begin
                   // fold_start is the transaction boundary and clears
                   // completion state.  Do not consume a stale c_done_fold
                   // from the previous transaction on that same clock edge.
                   $display("CDBG C00=%h C01=%h C10=%h C77=%h",
                            C[0][0], C[0][1], C[1][0], C[N-1][N-1]);
                   scan_c <= '0;
                   phase  <= P_SCAN;
                 end
        P_SCAN:  if (scan_c_last) phase <= P_WB;
                 else             scan_c <= scan_c + 1'b1;
        // The tile goes to memory only after chk_c has been taken off the
        // register file, so a write-back fault can never be mistaken for a
        // compute fault: by the time anything is written, led[5] has settled.
        P_WB:    begin
                   if (!wb_desc_started) wb_desc_valid <= 1'b1;
                   if (wb_done) begin
                     if (fi_last) phase <= P_DONE;
                     else begin
                       fi    <= fi + 4'd1;
                       phase <= P_READ;    // refill, fold and store slab fi+1
                     end
                   end
                 end
        P_DONE:  begin
                   // Legacy bring-up can explicitly request another run.
                   //
                   // In external-scheduler mode, accepting a new job is also
                   // a new run boundary.  The descriptor has already been
                   // latched by job_fire, so restart the datapath from
                   // P_CALIB and let the normal seed/read/compute/writeback
                   // sequence execute for the newly accepted job.
                   if (rerun_pulse ||
                       (USE_EXTERNAL_SCHEDULER && job_fire)) begin
                     fi    <= 4'd0;
                     phase <= P_CALIB;
                   end
                 end
        default: ;
      endcase
    end
  end

  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n) rerun_d <= 1'b0;
    else           rerun_d <= rerun_probe;
  end

  assign run_clear = (phase == P_CALIB);

  // desc_valid is a pulse; remember it was taken so P_READ does not re-issue.
  // Cleared OUTSIDE the phase rather than in one named predecessor, so the
  // n_inv-th pass through P_READ arms exactly as the first one did.
  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n)                     desc_started <= 1'b0;
    else if (phase != P_READ)          desc_started <= 1'b0;
    else if (desc_valid && desc_ready) desc_started <= 1'b1;
  end

  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n)                           wb_desc_started <= 1'b0;
    else if (phase != P_WB)                  wb_desc_started <= 1'b0;
    else if (wb_desc_valid && wb_desc_ready) wb_desc_started <= 1'b1;
  end

  // The read engine's own done pulse, held only for the invocation it belongs
  // to: fill_complete must not be satisfied by the previous invocation's.
  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n)            read_done_fold <= 1'b0;
    else if (phase != P_READ) read_done_fold <= 1'b0;
    else if (read_done)       read_done_fold <= 1'b1;
  end

  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n)                             c_done_fold <= 1'b0;
    else if ((phase == P_GO) || fold_start)    c_done_fold <= 1'b0;
    else if (c_done)                           c_done_fold <= 1'b1;
  end

  // Ownership is registered from the phase, so it is already settled one cycle
  // before the engine can raise awvalid: the descriptor it needs is itself a
  // registered pulse, and the engine issues no address in the cycle it accepts
  // one.  Ownership is never handed back, and the loop does not change that:
  // all n_inv slabs are seeded in P_SEED, before the first descriptor, so the
  // seeder has finished for good by the time the first P_WB takes the channel.
  // Handed back at the start of a run, not never: a re-run seeds again, and a
  // seeder writing into a channel the write-back engine still owns is a hang
  // (its AW never reaches the controller) plus an err_w_owner that blames the
  // wrong master.  Within a run the hand-over is still one-way.
  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n || run_clear) wb_owns_w <= 1'b0;
    else if (phase == P_WB)     wb_owns_w <= 1'b1;
  end

  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n) begin
      err_w_owner <= 1'b0;
    end
    else if (
        (hs_busy &&
         (sd_awvalid || sd_wvalid || wb_awvalid || wb_wvalid)) ||
        (!hs_busy && wb_owns_w &&
         (sd_awvalid || sd_wvalid || hs_awvalid || hs_wvalid)) ||
        (!hs_busy && !wb_owns_w &&
         (wb_awvalid || wb_wvalid || hs_awvalid || hs_wvalid))
    ) begin
      err_w_owner <= 1'b1;
    end
  end

  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n) begin
      seed_done_sticky <= 1'b0;
      read_done_sticky <= 1'b0;
      fold_done_sticky <= 1'b0;
      wb_done_sticky   <= 1'b0;
    end else if (run_clear) begin
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

  // ---- the operand path, counted ------------------------------------------
  // The paper's Table 4 row "DMA v1/v2 [measured]" is three numbers: fill,
  // compute (cyc_latched, below, unchanged), write-back.  Both new counters are
  // inclusive: they start at 1 on the cycle the descriptor is accepted and stop
  // on the cycle the phase's completion condition is first true.
  //
  //   fill_cycles   desc accepted -> fill_complete (last word LANDED, not the
  //                 last beat received -- the same distinction P_READ makes).
  //                 v1 at N=8, k=256: ~4096 + startup, the port-bound number;
  //                 v2: ~1129, the controller-bound one.  The engine's own
  //                 r_stall_cycles (rvalid && !rready) says where the difference
  //                 went: ~3/4 of fill on v1, ~0 on v2.
  //   wb_cycles     wb descriptor accepted -> wb_done (the last B response).
  logic [31:0] fill_cycles, wb_cycles;
  logic        fill_running, wb_running;

  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n || run_clear) begin
      fill_cycles  <= '0;
      fill_running <= 1'b0;
      wb_cycles    <= '0;
      wb_running   <= 1'b0;
    end else begin
      if (desc_valid && desc_ready) begin
        fill_running <= 1'b1;
        fill_cycles  <= fill_cycles + 32'd1;   // summed over invocations
      end else if (fill_running) begin
        fill_cycles <= fill_cycles + 1'b1;
        if (fill_complete) fill_running <= 1'b0;
      end

      if (wb_desc_valid && wb_desc_ready) begin
        wb_running <= 1'b1;
        wb_cycles  <= wb_cycles + 32'd1;       // summed over invocations
      end else if (wb_running) begin
        wb_cycles <= wb_cycles + 1'b1;
        if (wb_done) wb_running <= 1'b0;
      end
    end
  end

  // ---- the run, end to end ------------------------------------------------
  // fill + compute + write-back is what the paper's per-invocation table adds
  // up, and it is a sum of three phases, not a wall clock: P_GO, the hand-offs
  // and the whole of P_SCAN sit between them.  t_span is the wall clock --
  // first descriptor accepted to the last write response -- so the difference
  // between the two says exactly how much of the run is instrumentation rather
  // than leaving it to be argued.  Neither number is the other's estimate.
  logic [31:0] t_span;
  logic [7:0]  folds_done;
  logic        t_running;

  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n || run_clear) begin
      t_span     <= '0;
      t_running  <= 1'b0;
      folds_done <= '0;
    end else begin
      if (desc_valid && desc_ready && !t_running && (t_span == 32'd0)) begin
        t_running <= 1'b1;
        t_span    <= 32'd1;
      end else if (t_running) begin
        t_span <= t_span + 1'b1;
        if (wb_done && fi_last) t_running <= 1'b0;
      end
      if (wb_done) folds_done <= folds_done + 8'd1;
    end
  end

  // The engines' own counters, previously left unconnected.  They count from
  // reset (stat_clear is tied low) across every descriptor the run issues, so
  // at n_inv = 1 they are per-descriptor totals and above it they are run
  // totals -- the same quantity fill_cycles and wb_cycles report.
  wire [31:0] eng_busy_cycles, eng_rdy_stall_cycles, eng_r_stall_cycles;
  wire [31:0] wb_busy_cycles, wb_aw_stall_cycles, wb_w_stall_cycles, wb_src_starve_cycles;

  // ---- seeder -------------------------------------------------------------
  dma_seed_writer #(
    .AXI_DATA_W (AXI_DATA_W), .AXI_ADDR_W (AXI_ADDR_W), .AXI_ID_W (2),
    .BURST_LEN (16), .TOTAL_BEATS (N_BEATS),
    .SEED_MODE (1), .MODULUS (127)
  ) u_seed (
    .clk (ui_clk), .rst_n (ui_rst_n),
    .start (seed_start), .base_addr (slab_addr),
    .busy (seed_busy), .done (seed_done),
    .err_align (seed_err_align), .err_resp (seed_err_resp),
    .m_axi_awid (sd_awid), .m_axi_awaddr (sd_awaddr), .m_axi_awlen (sd_awlen),
    .m_axi_awsize (sd_awsize), .m_axi_awburst (sd_awburst),
    .m_axi_awlock (sd_awlock), .m_axi_awcache (sd_awcache),
    .m_axi_awprot (sd_awprot), .m_axi_awqos (sd_awqos),
    .m_axi_awvalid (sd_awvalid), .m_axi_awready (sd_awready),
    .m_axi_wdata (sd_wdata), .m_axi_wstrb (sd_wstrb), .m_axi_wlast (sd_wlast),
    .m_axi_wvalid (sd_wvalid), .m_axi_wready (sd_wready),
    .m_axi_bid (bid), .m_axi_bresp (bresp), .m_axi_bvalid (sd_bvalid),
    .m_axi_bready (sd_bready)
  );

  // ---- read engine --------------------------------------------------------
  //
  // One AXI read engine serves two descriptor classes:
  //
  //   8'h3B : normal operand fetch
  //   8'h3C : result readback
  //
  // Result readback is not enabled yet; rb_req_valid is held low below.
  // Introducing the mux first lets the existing operand path regress
  // unchanged before the readback producer and output CDC are connected.
  //
  localparam logic [7:0] DMA_TAG_OPERAND  = 8'h3B;
  localparam logic [7:0] DMA_TAG_READBACK = 8'h3C;

  wire          dst_wr_en;
  wire [15:0]   dst_wr_beat;
  wire [127:0]  dst_wr_data;
  wire [7:0]    dst_wr_tag;
  // Destination backpressure is selected according to the descriptor
  // currently using the shared read engine.
  wire          dst_full, dst_almost_full;
  wire          op_dst_full, op_dst_almost_full;

  // Result-output CDC backpressure.  The CDC itself is connected below.
  wire          rb_cdc_src_ready;

  // Existing operand descriptor source.
  wire                    op_desc_valid = desc_valid;
  wire [AXI_ADDR_W-1:0]   op_desc_addr  = slab_addr;
  wire [15:0]             op_desc_beats = 16'(job_n_beats);

  // Result-readback descriptor source.
  //
  // For the first integration checkpoint, automatically read back the
  // completed external 4x4 result tile after its final DDR write response.
  //
  // A 4x4 FP32 tile is 16 words = 64 bytes = four 128-bit beats.
  logic                   rb_pending;
  logic                   rb_active;
  logic [AXI_ADDR_W-1:0]  rb_addr_reg;

  assign result_readback_busy = rb_pending || rb_active;

  // Backpressure presented to the shared read engine.
  //
  // Operand descriptors retain the existing operand-writer behavior.
  // During result readback, the output CDC FIFO is the destination.
  //
  // rb_active remains asserted for the lifetime of an accepted readback
  // descriptor, so it is the correct selector after rb_pending is cleared.
  assign dst_full =
      (rb_pending || rb_active)
          ? !rb_cdc_src_ready
          : op_dst_full;

  // The output FIFO exposes only ready/full, not an almost-full threshold.
  // Full is sufficient for this four-beat result stream.
  assign dst_almost_full =
      (rb_pending || rb_active)
          ? !rb_cdc_src_ready
          : op_dst_almost_full;

  wire                    rb_req_valid = rb_pending;
  wire [AXI_ADDR_W-1:0]   rb_req_addr  = rb_addr_reg;
  wire [15:0]             rb_req_beats = selected_wb_beats[15:0];

  // Readback owns the descriptor input while pending.
  wire                    eng_desc_is_readback = rb_req_valid;

  wire                    eng_desc_valid =
      eng_desc_is_readback ? rb_req_valid : op_desc_valid;

  wire [AXI_ADDR_W-1:0]   eng_desc_addr =
      eng_desc_is_readback ? rb_req_addr : op_desc_addr;

  wire [15:0]             eng_desc_beats =
      eng_desc_is_readback ? rb_req_beats : op_desc_beats;

  wire [7:0]              eng_desc_tag =
      eng_desc_is_readback ? DMA_TAG_READBACK : DMA_TAG_OPERAND;

  wire                    eng_desc_ready;
  wire                    eng_done_valid;
  wire [7:0]              eng_done_tag;

  // Preserve the existing operand-side handshake semantics.
  assign desc_ready = !eng_desc_is_readback && eng_desc_ready;

  dma_engine #(
    .AXI_DATA_W (AXI_DATA_W), .AXI_ADDR_W (AXI_ADDR_W), .AXI_ID_W (2),
    .BEAT_W (16), .BURST_LEN (16), .MAX_OUTSTANDING (8)
  ) u_eng (
    .clk (ui_clk), .rst_n (ui_rst_n), .init_calib_complete (init_calib_complete),
    .desc_valid (eng_desc_valid), .desc_ready (eng_desc_ready),
    .desc_addr (eng_desc_addr), .desc_beats (eng_desc_beats),
    .desc_tag (eng_desc_tag),
    .done_valid (eng_done_valid), .done_tag (eng_done_tag),
    .m_axi_arid (arid), .m_axi_araddr (araddr), .m_axi_arlen (arlen),
    .m_axi_arsize (arsize), .m_axi_arburst (arburst), .m_axi_arlock (arlock),
    .m_axi_arcache (arcache), .m_axi_arprot (arprot), .m_axi_arqos (arqos),
    .m_axi_arvalid (arvalid), .m_axi_arready (arready),
    .m_axi_rid (2'b0), .m_axi_rdata (rdata_axi), .m_axi_rresp (rresp),
    .m_axi_rlast (rlast), .m_axi_rvalid (rvalid), .m_axi_rready (rready),
    .dst_almost_full (dst_almost_full), .dst_full (dst_full),
    .dst_wr_en (dst_wr_en), .dst_wr_beat (dst_wr_beat),
    .dst_wr_data (dst_wr_data), .dst_wr_tag (dst_wr_tag),
    .busy_cycles (eng_busy_cycles), .rdy_stall_cycles (eng_rdy_stall_cycles),
    .r_stall_cycles (eng_r_stall_cycles),
    .err_align (eng_err_align), .err_resp (eng_err_resp), .stat_clear (run_clear)
  );

  // Completion visible to the existing P_READ FSM only for an operand
  // descriptor.  A future readback completion must not satisfy fill_complete
  // or advance the compute FSM.
  assign read_done =
      eng_done_valid &&
      (eng_done_tag == DMA_TAG_OPERAND);

  // -----------------------------------------------------------------------
  // First result-readback integration checkpoint.
  //
  // Arm exactly once when the final result writeback receives its final
  // response.  The result is therefore already resident in DDR before the
  // read descriptor can be accepted.
  //
  // rb_active remains asserted for the lifetime of the readback descriptor
  // and clears only on the matching completion tag.
  // -----------------------------------------------------------------------
  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n) begin
      rb_pending  <= 1'b0;
      rb_active   <= 1'b0;
      rb_addr_reg <= '0;
    end
    else begin
      // Final result writeback completed.
      if (USE_EXTERNAL_SCHEDULER &&
          job_active &&
          wb_done &&
          fi_last &&
          !rb_pending &&
          !rb_active) begin
        rb_pending  <= 1'b1;
        rb_addr_reg <= job_c_base_reg[AXI_ADDR_W-1:0];
      end

      // Readback descriptor accepted by the shared read engine.
      if (rb_pending && eng_desc_ready) begin
        rb_pending <= 1'b0;
        rb_active  <= 1'b1;
      end

      // Matching readback descriptor completely returned.
      if (eng_done_valid &&
          (eng_done_tag == DMA_TAG_READBACK)) begin
        rb_active <= 1'b0;
      end
    end
  end

  wire rb_dst_wr_en =
      dst_wr_en &&
      (dst_wr_tag == DMA_TAG_READBACK);

  // -----------------------------------------------------------------------
  // External-job / result-readback hardware debug.
  //
  // Sticky event bits survive after the short handshakes themselves have
  // disappeared, allowing the completed path to be inspected later.
  //
  //   bit 0 : external job accepted
  //   bit 1 : result readback armed after final writeback
  //   bit 2 : readback descriptor accepted
  //   bit 3 : at least one readback data beat returned
  //   bit 4 : JOB0 readback data mismatch observed
  // -----------------------------------------------------------------------
  logic [31:0] external_debug_sticky;

  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n) begin
      external_debug_sticky <= 32'd0;
    end
    else begin
      if (job_fire)
        external_debug_sticky[0] <= 1'b1;

      if (USE_EXTERNAL_SCHEDULER &&
          job_active &&
          wb_done &&
          fi_last &&
          !rb_pending &&
          !rb_active)
        external_debug_sticky[1] <= 1'b1;

      if (rb_pending && eng_desc_ready)
        external_debug_sticky[2] <= 1'b1;

      if (rb_dst_wr_en)
        external_debug_sticky[3] <= 1'b1;

      // JOB0 numerical readback signature check.
      // For the current smoke test, every 128-bit C beat must contain
      // four FP32 values {8.0, 4.0, 2.0, 1.0}.
      if (rb_dst_wr_en &&
          (dst_wr_data != 128'h4100000040800000400000003f800000))
        external_debug_sticky[4] <= 1'b1;

      // Host-command ingress checkpoints.
      //
      // bit 5: frontend emitted and downstream accepted a WRITE32 command
      // bit 6: command crossed the async FIFO into ui_clk
      // bit 7: AXI4-Lite master accepted the command
      // bit 8: register write reached and was accepted by descriptor bridge
      // bit 9: submit write (CONTROL 0x00, bit 0 = 1) reached the bridge
      // bit10: descriptor bridge asserted a pending job
      // bit11: DMA core was ready for an external job
      // bit12: hardware scheduler was ready for a descriptor
      // JOB0 numerical writeback signature check.
      //
      // bit5 is temporarily repurposed from the command-ingress checkpoint.
      // A writeback beat is checked only when it is actually accepted by the
      // shared DDR write interface.
      if (wb_wvalid &&
          wb_wready &&
          (wb_wdata !=
           128'h4100000040800000400000003f800000))
        external_debug_sticky[5] <= 1'b1;

      if (dpti_fifo_rd_valid && dpti_fifo_rd_ready)
        external_debug_sticky[6] <= 1'b1;

      if (dpti_axi_cmd_valid && dpti_axi_cmd_ready)
        external_debug_sticky[7] <= 1'b1;

      if (axi_dpti_wr_valid && axi_dpti_wr_ready)
        external_debug_sticky[8] <= 1'b1;

      if (axi_dpti_wr_valid &&
          axi_dpti_wr_ready &&
          (axi_dpti_wr_addr == 8'h00) &&
          axi_dpti_wr_data[0])
        external_debug_sticky[9] <= 1'b1;

      if (dpti_job_valid)
        external_debug_sticky[10] <= 1'b1;

      if (core_job_ready)
        external_debug_sticky[11] <= 1'b1;

      if (scheduler_desc_ready)
        external_debug_sticky[12] <= 1'b1;

      // Physical-input / MEM_WRITE checkpoints.
      //
      // bit13: at least one input byte accepted by the frontend
      // bit14: MEM_WRITE opcode 0x02 accepted while starting a command
      // bit15: MEM_WRITE header accepted and staging started
      // bit16: at least one complete payload beat accepted downstream
      // bit17: complete MEM_WRITE payload finished
      // bit18: frontend/decoder error observed
      // bit19: WRITE32 opcode 0x01 accepted while starting a command
      if (dpti_byte_valid && dpti_byte_ready)
        external_debug_sticky[13] <= 1'b1;

      if (dpti_byte_valid &&
          dpti_byte_ready &&
          (dpti_byte_data == 8'h02))
        external_debug_sticky[14] <= 1'b1;

      if (dpti_mem_start)
        external_debug_sticky[15] <= 1'b1;

      if (dpti_mem_valid && dpti_mem_ready)
        external_debug_sticky[16] <= 1'b1;

      if (dpti_mem_done)
        external_debug_sticky[17] <= 1'b1;

      if (dpti_frontend_err_opcode ||
          dpti_frontend_err_write32_opcode ||
          dpti_frontend_err_mem_opcode ||
          dpti_frontend_err_mem_length)
        external_debug_sticky[18] <= 1'b1;

      if (dpti_byte_valid &&
          dpti_byte_ready &&
          (dpti_byte_data == 8'h01))
        external_debug_sticky[19] <= 1'b1;

      // Core-readiness diagnosis.
      //
      // bit20: transaction FSM observed in P_CALIB
      // bit21: transaction FSM observed in P_READ
      // bit22: transaction FSM observed in P_DONE
      // bit23: host staging writer observed busy
      // bit24: external job lifetime observed busy
      // bit25: external job observed active
      // bit26: core_job_ready observed high
      if (phase == P_CALIB)
        external_debug_sticky[20] <= 1'b1;

      if (phase == P_READ)
        external_debug_sticky[21] <= 1'b1;

      if (phase == P_DONE)
        external_debug_sticky[22] <= 1'b1;

      if (hs_busy)
        external_debug_sticky[23] <= 1'b1;

      if (job_busy)
        external_debug_sticky[24] <= 1'b1;

      if (job_active)
        external_debug_sticky[25] <= 1'b1;

      if (core_job_ready)
        external_debug_sticky[26] <= 1'b1;

      // External scheduler -> feeder diagnosis.
      //
      // bit27: transaction FSM reached P_GO
      // bit28: remembered scheduler release was replayed
      // bit29: any delayed scheduler start reached an adapter
      // bit30: 4x4 adapter emitted fold_start
      // bit31: common feeder entered ST_FEED
      if (phase == P_GO)
        external_debug_sticky[27] <= 1'b1;

      if (scheduler_start_replay)
        external_debug_sticky[28] <= 1'b1;

      if (|scheduler_accelerator_start_delayed)
        external_debug_sticky[29] <= 1'b1;

      // bit30: any physical 4x4 instance received its fold-start event
      if (select_4x4 && scheduler_fold_start_selected)
        external_debug_sticky[30] <= 1'b1;

      if (state == ST_FEED)
        external_debug_sticky[31] <= 1'b1;
    end
  end

  // Observation-only readback trace.
  always_ff @(posedge ui_clk) begin
    if (ui_rst_n && rb_dst_wr_en) begin
      $display(
        "RB_BEAT beat=%0d tag=%02h data=%032h",
        dst_wr_beat,
        dst_wr_tag,
        dst_wr_data
      );
    end

    if (ui_rst_n &&
        eng_done_valid &&
        (eng_done_tag == DMA_TAG_READBACK)) begin
      $display("RB_DONE tag=%02h", eng_done_tag);
    end
  end

  wire op_dst_wr_en =
      dst_wr_en &&
      (dst_wr_tag == DMA_TAG_OPERAND);

  // ---- operand writer + checksum 1 + operand memories ---------------------
  // The one place the two operand-path versions differ.  Selected by USE_V2 at
  // elaboration; see the parameter's comment.  The operand memories' READ
  // side -- a_raddr/b_raddr in, a_rdata/b_rdata out one cycle later -- is the
  // same in both versions and is what the copied core below is wired to.
  //
  // checksum 1, the write stream: seed_ref.py's formula, accumulated as the
  // words go past rather than by reading the buffers back.  The read ports
  // belong to the feeder; taking them for a scan would mean muxing the array's
  // own datapath in order to observe it.  The order differs from seed_ref's --
  // payload order, not (k, bank) order -- and that is fine: 32-bit wrapping
  // addition is commutative and associative, so the total is identical.  It is
  // the same constant 3a compared against, and it stays the same constant on
  // v2, where four terms are added per cycle instead of one.
  logic [31:0]    chk_wr;
  logic [K_W-1:0] a_raddr [0:N-1];
  logic [K_W-1:0] b_raddr [0:N-1];
  wire  [31:0]    a_rdata [0:N-1];
  wire  [31:0]    b_rdata [0:N-1];

  generate
  if (!USE_V2) begin : OP_V1
    wire              a_wr, b_wr;
    wire [LANE_W-1:0] wsel;
    wire [K_W-1:0]    waddr;
    wire [31:0]       wdata_buf;

    dma_operand_writer #(
      .N (N), .K_MAX (K_MAX), .AXI_DATA_W (AXI_DATA_W), .BEAT_W (16)
    ) u_wr (
      .clk (ui_clk), .rst_n (ui_rst_n),
      .dst_wr_en (op_dst_wr_en), .dst_wr_beat (dst_wr_beat), .dst_wr_data (dst_wr_data),
      .dst_full (op_dst_full), .dst_almost_full (op_dst_almost_full),
      .a_wr (a_wr), .b_wr (b_wr), .wsel (wsel), .waddr (waddr), .wdata (wdata_buf),
      .words_written (words_written), .err_range (wr_err_range), .clear (run_clear)
    );

    wire [15:0] k16   = 16'(waddr);
    wire [7:0]  bank8 = 8'(wsel);
    wire [31:0] wpos  = {8'd0, bank8, k16};

    // Diagnostic checksum only.
    //
    // chk_wr is synchronously cleared during P_CALIB, so it does not need
    // an asynchronous reset event here.  Keeping this block on ui_clk only
    // also avoids Vivado treating this debug-only event control as an
    // ambiguous clock/reset structure during synthesis.
    always_ff @(posedge ui_clk) begin
      if (phase == P_CALIB)      chk_wr <= '0;
      else if (a_wr)             chk_wr <= chk_wr + (wdata_buf ^ wpos);
      else if (b_wr)             chk_wr <= chk_wr + (wdata_buf ^ (32'h8000_0000 | wpos));

      if ((a_wr || b_wr) && (words_written < 8))
        $display("CHKDBG %s wsel=%0d waddr=%0d data=%h wpos=%h",
                 a_wr ? "A" : "B", wsel, waddr, wdata_buf, wpos);
    end

    // -------------------------------------------------------------------------
    // DEBUG: observe B-side operand writes.
    // Observation only; no functional signal is modified.
    // This avoids hierarchical probing of the operand-buffer memory.
    // -------------------------------------------------------------------------
    integer b_dbg_count;

    initial begin
      b_dbg_count = 0;
    end

    always_ff @(posedge ui_clk or negedge ui_rst_n) begin
      if (!ui_rst_n) begin
        b_dbg_count <= 0;
      end
      else if (b_wr && b_dbg_count < 16) begin
        $display(
          "BWRDBG #%0d beat=%0d dst=%h wsel=%0d waddr=%0d data=%h",
          b_dbg_count,
          dst_wr_beat,
          dst_wr_data,
          wsel,
          waddr,
          wdata_buf
        );
        b_dbg_count <= b_dbg_count + 1;
      end
    end

    // One write port and one synchronous read port each, which is the shape
    // block RAM wants.  The read is SYNCHRONOUS: the address issued on beat t
    // returns data on t+1, and the feeder already delays valid to match.  That
    // one cycle is why the cost is k + 2(N-1) + H rather than one less.
    //
    // A and B are the same hardware; the only difference is which field
    // selects the bank and which forms the address, and that swap is the A/B
    // transpose.  Both come from dma_operand_writer, which computes them with
    // the same bit slices systolic_uart_top's rx_count decode uses -- proved
    // equivalent in tb_dma_operand_writer against a golden model of that decode.
    systolic_operand_buffer #(
      .K_MAX (K_MAX), .K_W (K_W), .N_BANKS (N)
    ) u_a_buf (
      .clk (ui_clk), .wr (a_wr), .wsel (wsel), .waddr (waddr), .wdata (wdata_buf),
      .raddr (a_raddr), .rdata (a_rdata)
    );

    systolic_operand_buffer #(
      .K_MAX (K_MAX), .K_W (K_W), .N_BANKS (N)
    ) u_b_buf (
      .clk (ui_clk), .wr (b_wr), .wsel (wsel), .waddr (waddr), .wdata (wdata_buf),
      .raddr (b_raddr), .rdata (b_rdata)
    );
  end else begin : OP_V2
    wire                  a_wr, b_wr;
    wire [LANE_W-1:0]     wsel;
    wire [K_W-1:0]        waddr;
    wire [AXI_DATA_W-1:0] wdata_buf;       // the whole beat, word j at [32j +: 32]

    dma_operand_writer_v2 #(
      .N (N), .K_MAX (K_MAX), .AXI_DATA_W (AXI_DATA_W), .BEAT_W (16)
    ) u_wr (
      .clk (ui_clk), .rst_n (ui_rst_n),
      .dst_wr_en (op_dst_wr_en), .dst_wr_beat (dst_wr_beat), .dst_wr_data (dst_wr_data),
      .dst_full (op_dst_full), .dst_almost_full (op_dst_almost_full),
      .a_wr (a_wr), .b_wr (b_wr), .wsel (wsel), .waddr (waddr), .wdata (wdata_buf),
      .words_written (words_written), .err_range (wr_err_range), .clear (run_clear)
    );

    dma_wr_checksum_v2 #(
      .N (N), .K_MAX (K_MAX), .AXI_DATA_W (AXI_DATA_W)
    ) u_chk_wr (
      .clk (ui_clk), .rst_n (ui_rst_n), .clear (phase == P_CALIB),
      .a_wr (a_wr), .b_wr (b_wr), .wsel (wsel), .waddr (waddr), .wdata (wdata_buf),
      .chk (chk_wr)
    );

    // Same read side as v1; the write side takes a beat.  A is the cyclic
    // layout (a beat is four depths of one bank), B the block layout (a beat
    // is four banks at one depth) -- the wire format decides which is which.
    // Proven against v1's image at N = 4/8/16 in tb_dma_path_v2.
    systolic_operand_buffer_v2 #(
      .K_MAX (K_MAX), .K_W (K_W), .N_BANKS (N), .LAYOUT_CYCLIC (1'b1)
    ) u_a_buf (
      .clk (ui_clk), .wr (a_wr), .wsel (wsel), .waddr (waddr), .wdata (wdata_buf),
      .raddr (a_raddr), .rdata (a_rdata)
    );

    systolic_operand_buffer_v2 #(
      .K_MAX (K_MAX), .K_W (K_W), .N_BANKS (N), .LAYOUT_CYCLIC (1'b0)
    ) u_b_buf (
      .clk (ui_clk), .wr (b_wr), .wsel (wsel), .waddr (waddr), .wdata (wdata_buf),
      .raddr (b_raddr), .rdata (b_rdata)
    );
  end
  endgenerate

  // =========================================================================
  // FROM HERE TO THE MIG INSTANCE, EVERYTHING IS systolic_uart_top's COMPUTE
  // CORE, COPIED.  Do not "clean it up".  Any edit here is an edit to the
  // thing 3b is trying to hold constant, and the 125-cycle check is the only
  // thing standing between a faithful copy and a subtly different one.
  // =========================================================================

  // ---- operand memories ---------------------------------------------------
  // Instantiated above, inside the USE_V2 generate, because the write side is
  // what the two versions differ in.  What the copied core sees is unchanged:
  // a_raddr/b_raddr in, a_rdata/b_rdata out one cycle later, from block RAM
  // with one synchronous read port per bank.  That one cycle is why the cost
  // is k + 2(N-1) + H rather than one less, and cyc_latched below is the check
  // that neither version has moved it.

  // ---- array interface ----------------------------------------------------
  logic [31:0] a_in [0:N-1];
  logic [31:0] b_in [0:N-1];
  logic        a_valid_in [0:N-1];
  logic        b_valid_in [0:N-1];
  logic        c_valid_out_8x8 [0:NUM_8X8_STORAGE-1];
  logic [31:0] c_out_8x8 [0:NUM_8X8_STORAGE-1][0:N-1][0:N-1];

  // k_dim is a constant here rather than a register written by a frame header:
  // there is no host in this design.  It keeps the width systolic_uart_top gives
  // it so the feeder is parameterised identically.
  // Runtime reduction length.
  //
  // Legacy mode keeps the original compile-time K_DIM.
  //
  // External-scheduler mode takes K from the accepted job descriptor.
  // job_k_reg is latched at job_valid && job_ready, so the feeder sees
  // a stable K for the lifetime of the transaction.
  wire [FEED_W-1:0] k_dim =
      USE_EXTERNAL_SCHEDULER
          ? FEED_W'(job_k_reg)
          : FEED_W'(K_DIM);

  // ---- control FSM --------------------------------------------------------
  // systolic_uart_top's four states with ST_SEND replaced by ST_DONE: there is
  // nothing to transmit.  The encoding is written out explicitly there because
  // systolic_status keeps a copy; no status block here, but the values are kept
  // the same anyway so a waveform from either design reads alike.
  typedef enum logic [2:0] {
    ST_IDLE        = 3'd0,
    ST_FEED        = 3'd1,
    ST_WAIT_RESULT = 3'd2,
    ST_DONE        = 3'd3
  } state_t;

  state_t state;
  logic [FEED_W-1:0] feed_t;

  // systolic_uart_top writes these two as $bits(feed_t) and $bits(k_dim).  Both
  // of those signals are [FEED_W-1:0] there and here, so FEED_W is the same
  // number -- written out because $bits() in a parameter override is a place
  // simulators disagree, and the widths of the feeder's ports are not something
  // to leave to a tool's mood.
  systolic_tile_feeder #(
    .N      (N),
    .K_W    (K_W),
    .FEED_W (FEED_W),
    .KDIM_W (FEED_W)
  ) u_feeder (
    .clk            (ui_clk),
    .rst            (rst_i),
    .enable         (state == ST_FEED),

    .feed_t         (feed_t),
    .k_dim          (k_dim),

    .a_rdata        (a_rdata),
    .b_rdata        (b_rdata),

    .a_raddr        (a_raddr),
    .b_raddr        (b_raddr),

    .a_in           (a_in),
    .b_in           (b_in),

    .a_valid_in     (a_valid_in),
    .b_valid_in     (b_valid_in)
  );

  // -------------------------------------------------------------------------
  // DEBUG: first few feeder cycles of every fold
  // This is observation-only; no functional signal is modified.
  // -------------------------------------------------------------------------
  always_ff @(posedge ui_clk) begin
    if (!rst_i && state == ST_FEED && feed_t <= FEED_W'(3)) begin
      $display(
        "FEEDDBG t=%0t feed_t=%0d aV0=%b bV0=%b a0=%h b0=%h ar0=%0d br0=%0d",
        $time,
        feed_t,
        a_valid_in[0],
        b_valid_in[0],
        a_in[0],
        b_in[0],
        a_raddr[0],
        b_raddr[0]
      );
    end
  end



  // -------------------------------------------------------------------------
  // DEBUG: first four common-feeder lanes.
  // Observation only; no functional signal is modified.
  //
  // The physical 4x4 device consumes exactly lanes 0..3 of this N=8 feeder.
  // Trace addresses, returned data, and delayed valids together so that
  // per-lane K alignment can be checked directly.
  // -------------------------------------------------------------------------
  always_ff @(posedge ui_clk) begin
    if (!rst_i && state == ST_FEED && feed_t <= FEED_W'(8)) begin
      $display(
        "FEED4DBG t=%0t ft=%0d | L0 ar=%0d av=%b a=%h br=%0d bv=%b b=%h | L1 ar=%0d av=%b a=%h br=%0d bv=%b b=%h | L2 ar=%0d av=%b a=%h br=%0d bv=%b b=%h | L3 ar=%0d av=%b a=%h br=%0d bv=%b b=%h",
        $time, feed_t,
        a_raddr[0], a_valid_in[0], a_in[0],
        b_raddr[0], b_valid_in[0], b_in[0],
        a_raddr[1], a_valid_in[1], a_in[1],
        b_raddr[1], b_valid_in[1], b_in[1],
        a_raddr[2], a_valid_in[2], a_in[2],
        b_raddr[2], b_valid_in[2], b_in[2],
        a_raddr[3], a_valid_in[3], a_in[3],
        b_raddr[3], b_valid_in[3], b_in[3]
      );
    end
  end


  // -------------------------------------------------------------------------
  // ----------------------------------------------------------
  // Three independent physical 4x4 accelerator instances.
  //
  // All three instances share the same geometry and input data buses,
  // but device selection gates their valid signals independently.
  // Physical-instance identity therefore remains distinct from geometry.
  // ----------------------------------------------------------
  logic [31:0] a_in_4x4 [0:3];
  logic [31:0] b_in_4x4 [0:3];

  generate
    for (genvar d = 0; d < 4; d = d + 1) begin : CONNECT_4X4_DEVICE
      assign a_in_4x4[d] = a_in[d];
      assign b_in_4x4[d] = b_in[d];
    end
  endgenerate

  logic a_valid_to_8x8 [0:NUM_8X8_STORAGE-1][0:N-1];
  logic b_valid_to_8x8 [0:NUM_8X8_STORAGE-1][0:N-1];

  logic a_valid_to_4x4 [0:NUM_4X4_STORAGE-1][0:3];
  logic b_valid_to_4x4 [0:NUM_4X4_STORAGE-1][0:3];

  logic        c_valid_out_4x4 [0:NUM_4X4_STORAGE-1];
  logic [31:0] c_out_4x4 [0:NUM_4X4_STORAGE-1][0:3][0:3];

  generate
    for (genvar ai = 0; ai < NUM_8X8; ai++) begin : DISPATCH_8X8_INSTANCE
      for (genvar d = 0; d < N; d++) begin : DISPATCH_8X8_LANE
        assign a_valid_to_8x8[ai][d] =
            a_valid_in[d] && select_8x8 &&
            (selected_8x8_idx == ai);
        assign b_valid_to_8x8[ai][d] =
            b_valid_in[d] && select_8x8 &&
            (selected_8x8_idx == ai);
      end
    end
  endgenerate

  generate
    for (genvar ai = 0; ai < NUM_4X4; ai++) begin : DISPATCH_4X4_INSTANCE
      for (genvar d = 0; d < 4; d++) begin : DISPATCH_4X4_LANE
        assign a_valid_to_4x4[ai][d] =
            a_valid_in[d] && select_4x4 &&
            (selected_4x4_idx == ai);
        assign b_valid_to_4x4[ai][d] =
            b_valid_in[d] && select_4x4 &&
            (selected_4x4_idx == ai);
      end
    end
  endgenerate

  generate
    for (genvar i = 0; i < NUM_4X4; i++) begin : ACC_4X4
      (* keep_hierarchy = "yes" *)
      systolic_array #(
        .N        (4),
        .DATA_W   (32),
        .ACC_BANKS(16)
      ) u_acc (
        .clk         (ui_clk),
        .rst         (rst_i),
        .a_in        (a_in_4x4),
        .b_in        (b_in_4x4),
        .a_valid_in  (a_valid_to_4x4[i]),
        .b_valid_in  (b_valid_to_4x4[i]),
        .c_valid_out (c_valid_out_4x4[i]),
        .c_out       (c_out_4x4[i])
      );
    end
  endgenerate

  generate
    for (genvar i = 0; i < NUM_8X8; i++) begin : ACC_8X8
      (* keep_hierarchy = "yes" *)
      systolic_array #(
        .N      (N),
        .DATA_W (32)
      ) u_acc (
        .clk         (ui_clk),
        .rst         (rst_i),
        .a_in        (a_in),
        .b_in        (b_in),
        .a_valid_in  (a_valid_to_8x8[i]),
        .b_valid_in  (b_valid_to_8x8[i]),
        .c_valid_out (c_valid_out_8x8[i]),
        .c_out       (c_out_8x8[i])
      );
    end
  endgenerate

  // -------------------------------------------------------------------------
  // Device-selected accelerator completion.
  //
  // The scheduler selects a physical accelerator by opaque device ID.
  // Do NOT encode geometry into the scheduler protocol.
  //
  // 8x8 device:
  //   c_valid_selected = c_valid_out
  //   selected geometry = 8x8
  //
  // 4x4 devices:
  //   c_valid_selected = completion of the selected 4x4 instance
  //   selected geometry = 4x4
  //
  // The C data itself is intentionally NOT flattened into the 8x8 result
  // buffer here.  The writeback path must become device-aware because a
  // 4x4 invocation produces 16 words, while an 8x8 invocation produces
  // 64 words.
  // -------------------------------------------------------------------------

  logic c_valid_selected;

  wire c_valid_out_4x4_selected =
      select_4x4 ? c_valid_out_4x4[selected_4x4_idx] : 1'b0;

  wire c_valid_out_8x8_selected =
      select_8x8 ? c_valid_out_8x8[selected_8x8_idx] : 1'b0;

  assign c_valid_selected =
      select_8x8 ? c_valid_out_8x8_selected :
      select_4x4 ? c_valid_out_4x4_selected :
                   1'b0;


  // -------------------------------------------------------------------------
  // DEBUG: observe array-side transaction boundary.
  // Observation only: no functional signal is modified.
  // -------------------------------------------------------------------------
  always_ff @(posedge ui_clk) begin
    if (!rst_i && state == ST_FEED && feed_t <= FEED_W'(3)) begin
      $display(
        "ARRAYINDBG t=%0t feed_t=%0d rst=%b aV0=%b bV0=%b a0=%h b0=%h",
        $time,
        feed_t,
        rst_i,
        a_valid_in[0],
        b_valid_in[0],
        a_in[0],
        b_in[0]
      );
    end

    if (!rst_i && select_8x8 && c_valid_out_8x8_selected) begin
      $display(
        "ARRAYOUTDBG t=%0t c_valid=1 c00=%h c01=%h c10=%h c77=%h",
        $time,
        c_out_8x8[selected_8x8_idx][0][0],
        c_out_8x8[selected_8x8_idx][0][1],
        c_out_8x8[selected_8x8_idx][1][0],
        c_out_8x8[selected_8x8_idx][N-1][N-1]
      );
    end
  end


  // ---- store final results ------------------------------------------------
  //
  // C is kept as an N x N backing store because the existing writeback path
  // is still being migrated to device-aware result sizes.
  //
  // The producer, however, is selected by ingress_job_device_id:
  //
  //   DEVICE_ID_8X8   -> u_acc_8x8   -> N x N results
  //   DEVICE_ID_4X4_0 -> u_acc_4x4_0 -> 4 x 4 results
  //   DEVICE_ID_4X4_1 -> u_acc_4x4_1 -> 4 x 4 results
  //   DEVICE_ID_4X4_2 -> u_acc_4x4_2 -> 4 x 4 results
  //
  // Do not let the 8x8 storage shape imply the physical device shape.
  // The next writeback step will use the selected device geometry to decide
  // how many words are emitted to DDR.
  //
  logic [31:0] C [0:N-1][0:N-1];

  integer rr;
  integer cc;

  always_ff @(posedge ui_clk) begin
    if (rst_i) begin
      c_done <= 1'b0;
    end
    else begin
      // New transaction starts.
      if (fold_start)
        c_done <= 1'b0;

      // ----------------------------------------------------------
      // 8x8 physical device result.
      // ----------------------------------------------------------
      if (select_8x8 && c_valid_out_8x8_selected) begin
        for (rr = 0; rr < N; rr = rr + 1)
          for (cc = 0; cc < N; cc = cc + 1)
            C[rr][cc] <= c_out_8x8[selected_8x8_idx][rr][cc];

        $display(
          "CDBG_RAW device=8x8 C00=%h C01=%h C10=%h C77=%h",
          c_out_8x8[selected_8x8_idx][0][0],
          c_out_8x8[selected_8x8_idx][0][1],
          c_out_8x8[selected_8x8_idx][1][0],
          c_out_8x8[selected_8x8_idx][N-1][N-1]
        );

        c_done <= 1'b1;
      end

      // ----------------------------------------------------------
      // 4x4 physical device result.
      //
      // Store the 4x4 result in the upper-left portion of the
      // common C backing store.  Physical result size / placement
      // in DDR is handled by the device-aware writeback stage.
      // ----------------------------------------------------------
      else if (select_4x4 && c_valid_out_4x4_selected) begin
        for (rr = 0; rr < 4; rr = rr + 1)
          for (cc = 0; cc < 4; cc = cc + 1)
            C[rr][cc] <= c_out_4x4[selected_4x4_idx][rr][cc];

        c_done <= 1'b1;
      end
    end
  end

  // ---- main state progression ---------------------------------------------
  always_ff @(posedge ui_clk) begin
    if (rst_i) begin
      state  <= ST_IDLE;
      feed_t <= '0;
    end
    else begin
      case (state)

        ST_IDLE: begin
          feed_t <= '0;
          if (fold_start) begin
            state  <= ST_FEED;
            feed_t <= '0;
          end
        end

        ST_FEED: begin
          if (feed_t == k_dim + FEED_W'(N - 2)) begin
            state <= ST_WAIT_RESULT;
          end
          else begin
            feed_t <= feed_t + 1'b1;
          end
        end

        ST_WAIT_RESULT: begin
          // No hard-coded drain count: wait for the accelerator itself to
          // report that the reduced result matrix exists.
          if (c_done) state <= ST_DONE;
        end

        ST_DONE: begin
          // Back to idle, so invocation fi+1's fold_start finds the machine
          // invocation fi found.  The accumulators need no help: systolic_pe
          // clears the reduced set's bank valids at RED_DONE, so nothing of
          // invocation fi survives into fi+1 and each one is an independent
          // k-deep GEMM whose C is written back on its own.
          state <= ST_IDLE;
        end

        default: state <= ST_IDLE;

      endcase
    end
  end

  // ---- transaction cycle counter ------------------------------------------
  // Start: the first beat of ST_FEED (feed_t == 0).  End: the result is
  // published (c_valid_out).  Both ends inclusive -- the same interval
  // systolic_uart_top measures, which is what makes 125 comparable.
  logic [31:0] cyc_count;
  logic [31:0] cyc_latched;    // the LAST invocation's, so the golden still checks
  logic [31:0] cyc_total;      // summed over the run's invocations
  logic        cyc_running;

  always_ff @(posedge ui_clk) begin
    if (rst_i || run_clear) begin
      cyc_count   <= '0;
      cyc_latched <= '0;
      cyc_total   <= '0;
      cyc_running <= 1'b0;
    end
    else begin
      if (state == ST_FEED && feed_t == '0 && !cyc_running) begin
        cyc_running <= 1'b1;
        cyc_count   <= 32'd1;
      end
      else if (cyc_running) begin
        cyc_count <= cyc_count + 1'b1;
        if (c_valid_selected) begin
          cyc_running <= 1'b0;
          cyc_latched <= cyc_count + 1'b1;
          cyc_total   <= cyc_total + cyc_count + 32'd1;
        end
      end
    end
  end

  // =========================================================================
  // End of the copied core.
  // =========================================================================

  // ---- write-back: C -> DRAM ----------------------------------------------
  //
  // Device-aware result path.
  //
  // Both parameterized readers exist physically:
  //
  //   u_rdr_8x8 : N=8 -> 64 result words -> 16 AXI beats
  //   u_rdr_4x4 : N=4 -> 16 result words ->  4 AXI beats
  //
  // ingress_job_device_id is the runtime selector.  It does NOT become a
  // runtime parameter N.
  //
  // The selected reader is the only reader started for the current
  // P_WB phase, and its FIFO is the only FIFO consumed by u_wb.
  // -------------------------------------------------------------------------

  // 4x4 reader view of the common C backing store.
  //
  // The 4x4 accelerator writes its result into C[0:3][0:3].
  // Keep this as an explicit 4x4 array because dma_result_reader's
  // N parameter determines the unpacked array port shape.
  wire [31:0] C_4x4 [0:3][0:3];

  generate
    for (genvar c4_r = 0; c4_r < 4; c4_r = c4_r + 1) begin : C4_VIEW_R
      for (genvar c4_c = 0; c4_c < 4; c4_c = c4_c + 1) begin : C4_VIEW_C
        assign C_4x4[c4_r][c4_c] = C[c4_r][c4_c];
      end
    end
  endgenerate

  wire                  wb8_rempty;
  wire                  wb4_rempty;
  wire [AXI_DATA_W-1:0] wb8_rd_data;
  wire [AXI_DATA_W-1:0] wb4_rd_data;
  wire                  wb8_rd_en;
  wire                  wb4_rd_en;

  // ----------------------------------------------------------
  // 8x8 result reader
  // ----------------------------------------------------------
  wire                     rdr8_wr_en;
  wire [AXI_DATA_W-1:0]    rdr8_wr_data;
  wire                     rdr8_wfull;
  wire                     rdr8_done;

  wire wb_rd_start_8x8 =
      (phase == P_WB) && select_8x8;

  dma_result_reader #(
    .N          (8),
    .AXI_DATA_W (AXI_DATA_W)
  ) u_rdr_8x8 (
    .clk     (ui_clk),
    .rst     (rst_i),
    .start   (wb_rd_start_8x8),
    .done    (rdr8_done),
    .C       (C),
    .wr_en   (rdr8_wr_en),
    .wr_data (rdr8_wr_data),
    .wfull   (rdr8_wfull)
  );

  dma_cdc_fifo #(
    .DW        (AXI_DATA_W),
    .AW        (5),
    .AF_MARGIN (8)
  ) u_wb_fifo_8x8 (
    .wclk        (ui_clk),
    .wrst_n      (ui_rst_n),
    .wr_en       (rdr8_wr_en),
    .wr_data     (rdr8_wr_data),
    .wfull       (rdr8_wfull),
    .walmost_full(),

    .rclk        (ui_clk),
    .rrst_n      (ui_rst_n),
    .rd_en       (wb8_rd_en),
    .rd_data     (wb8_rd_data),
    .rempty      (wb8_rempty)
  );

  // ----------------------------------------------------------
  // 4x4 result reader
  // ----------------------------------------------------------
  wire                     rdr4_wr_en;
  wire [AXI_DATA_W-1:0]    rdr4_wr_data;
  wire                     rdr4_wfull;
  wire                     rdr4_done;

  wire wb_rd_start_4x4 =
      (phase == P_WB) && select_4x4;

  dma_result_reader #(
    .N          (4),
    .AXI_DATA_W (AXI_DATA_W)
  ) u_rdr_4x4 (
    .clk     (ui_clk),
    .rst     (rst_i),
    .start   (wb_rd_start_4x4),
    .done    (rdr4_done),
    .C       (C_4x4),
    .wr_en   (rdr4_wr_en),
    .wr_data (rdr4_wr_data),
    .wfull   (rdr4_wfull)
  );

  dma_cdc_fifo #(
    .DW        (AXI_DATA_W),
    .AW        (5),
    .AF_MARGIN (8)
  ) u_wb_fifo_4x4 (
    .wclk        (ui_clk),
    .wrst_n      (ui_rst_n),
    .wr_en       (rdr4_wr_en),
    .wr_data     (rdr4_wr_data),
    .wfull       (rdr4_wfull),
    .walmost_full(),

    .rclk        (ui_clk),
    .rrst_n      (ui_rst_n),
    .rd_en       (wb4_rd_en),
    .rd_data     (wb4_rd_data),
    .rempty      (wb4_rempty)
  );

  // ----------------------------------------------------------
  // Runtime device selection at the FIFO boundary.
  //
  // Only the selected physical result stream is exposed to the
  // common writeback engine.
  // ----------------------------------------------------------
  wire                  wb_src_valid;
  wire [AXI_DATA_W-1:0] wb_src_data;
  wire                  wb_src_ready;

  assign wb_src_valid =
      select_8x8 ? !wb8_rempty :
      select_4x4 ? !wb4_rempty :
                   1'b0;

  assign wb_src_data =
      select_8x8 ? wb8_rd_data :
      select_4x4 ? wb4_rd_data :
                   '0;

  assign wb8_rd_en =
      select_8x8 && wb_src_valid && wb_src_ready;

  assign wb4_rd_en =
      select_4x4 && wb_src_valid && wb_src_ready;

  dma_writeback_engine #(
    .AXI_DATA_W (AXI_DATA_W), .AXI_ADDR_W (AXI_ADDR_W), .AXI_ID_W (2),
    .BEAT_W (16), .BURST_LEN (16), .MAX_OUTSTANDING (4)
  ) u_wb (
    .clk (ui_clk), .rst_n (ui_rst_n), .init_calib_complete (init_calib_complete),
    .desc_valid (wb_desc_valid), .desc_ready (wb_desc_ready),
    .desc_addr (wb_tile_addr),
    .desc_beats (16'(selected_wb_beats)),
    .desc_tag (8'h3C),
    .done_valid (wb_done), .done_tag (),
    .m_axi_awid (wb_awid), .m_axi_awaddr (wb_awaddr), .m_axi_awlen (wb_awlen),
    .m_axi_awsize (wb_awsize), .m_axi_awburst (wb_awburst),
    .m_axi_awlock (wb_awlock), .m_axi_awcache (wb_awcache),
    .m_axi_awprot (wb_awprot), .m_axi_awqos (wb_awqos),
    .m_axi_awvalid (wb_awvalid), .m_axi_awready (wb_awready),
    .m_axi_wdata (wb_wdata), .m_axi_wstrb (wb_wstrb), .m_axi_wlast (wb_wlast),
    .m_axi_wvalid (wb_wvalid), .m_axi_wready (wb_wready),
    .m_axi_bid (bid), .m_axi_bresp (bresp), .m_axi_bvalid (wb_bvalid),
    .m_axi_bready (wb_bready),
    .src_valid (wb_src_valid), .src_data (wb_src_data), .src_ready (wb_src_ready),
    .busy_cycles (wb_busy_cycles), .aw_stall_cycles (wb_aw_stall_cycles),
    .w_stall_cycles (wb_w_stall_cycles), .src_starve_cycles (wb_src_starve_cycles),
    .err_align (wb_err_align), .err_resp (wb_err_resp), .stat_clear (run_clear)
  );

  // ---- checksum 2: the result matrix --------------------------------------
  // One entry per cycle, not sixty-four in one.  3a's first bitstream missed
  // timing by 1.011 ns doing sixteen 32-bit adds in a cycle, and every failing
  // path was in the measurement rather than in anything being measured.  An
  // instrument that cannot meet timing casts doubt on every number it reports,
  // even when the number turns out to be right.
  //
  // The read is registered, so the index issued on cycle t is accumulated on
  // t+1 -- the same one-cycle skew the operand buffers have, handled the same
  // way.
  wire [LANE_W-1:0] scan_r_i = scan_c[2*LANE_W-1:LANE_W];
  wire [LANE_W-1:0] scan_c_i = scan_c[LANE_W-1:0];

  logic              scan_val_d;
  logic [LANE_W-1:0] scan_r_d, scan_c_d;
  logic [31:0]       c_rd;
  logic [31:0]       chk_c;

  wire [7:0]  c_row8  = 8'(scan_r_d);
  wire [15:0] c_col16 = 16'(scan_c_d);
  wire [31:0] cpos    = {8'd0, c_row8, c_col16};

  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n) begin
      scan_val_d <= 1'b0;
      scan_r_d   <= '0;
      scan_c_d   <= '0;
      c_rd       <= '0;
      chk_c      <= '0;
    end else begin
      scan_val_d <= (phase == P_SCAN) && !scan_c_last;
      scan_r_d   <= scan_r_i;
      scan_c_d   <= scan_c_i;
      c_rd       <= C[scan_r_i][scan_c_i];
      if (phase == P_CALIB)  chk_c <= '0;
      else if (scan_val_d)   chk_c <= chk_c + (c_rd ^ cpos);
    end
  end

  // chk_wr and chk_c are RUN totals: both are cleared in P_CALIB and both
  // accumulate across the run's invocations, so the constant they are compared
  // against has to grow with the run or led[4] and led[5] would go dark on a
  // correct 8-invocation run.  Every slab carries the same pattern and every
  // invocation therefore produces the same C, so the expected total is the
  // tabulated per-invocation constant added once per completed invocation --
  // 32-bit wrapping addition, the same arithmetic the hardware does.  At
  // n_inv = 1 this is the old comparison against the tabulated constant.
  logic [31:0] want_wr, want_c;
  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n) begin
      want_wr <= '0;
      want_c  <= '0;
    end else if (phase == P_CALIB) begin
      want_wr <= '0;
      want_c  <= '0;
    end else begin
      if ((phase == P_READ) && fill_complete) want_wr <= want_wr + EXPECT_WR_CHK;
      if ((phase == P_SCAN) && scan_c_last)   want_c  <= want_c  + EXPECT_C_CHK;
    end
  end

  wire wr_match = read_done_sticky && (want_wr != 32'd0) && (chk_wr == want_wr);
  wire c_match  = (phase == P_DONE)  && (chk_c  == want_c);
  wire any_err  = seed_err_align | seed_err_resp
                | eng_err_align  | eng_err_resp | wr_err_range
                | wb_err_align   | wb_err_resp  | err_w_owner;

  // ---- MIG ----------------------------------------------------------------
  mig_7series_0 u_mig_7series_0 (
    .ddr3_addr (ddr3_addr), .ddr3_ba (ddr3_ba), .ddr3_cas_n (ddr3_cas_n),
    .ddr3_ck_n (ddr3_ck_n), .ddr3_ck_p (ddr3_ck_p), .ddr3_cke (ddr3_cke),
    .ddr3_ras_n (ddr3_ras_n), .ddr3_reset_n (ddr3_reset_n), .ddr3_we_n (ddr3_we_n),
    .ddr3_dq (ddr3_dq), .ddr3_dqs_n (ddr3_dqs_n), .ddr3_dqs_p (ddr3_dqs_p),
    .init_calib_complete (init_calib_complete),
    .ddr3_dm (ddr3_dm), .ddr3_odt (ddr3_odt),

    .ui_clk (ui_clk), .ui_clk_sync_rst (ui_clk_sync_rst),
    .ui_addn_clk_0 (), .ui_addn_clk_1 (), .ui_addn_clk_2 (),
    .ui_addn_clk_3 (), .ui_addn_clk_4 (),
    .mmcm_locked (mmcm_locked_mig), .aresetn (ui_rst_n),
    .app_sr_req (1'b0), .app_ref_req (1'b0), .app_zq_req (1'b0),
    .app_sr_active (), .app_ref_ack (), .app_zq_ack (),

    .s_axi_awid (awid), .s_axi_awaddr (awaddr), .s_axi_awlen (awlen),
    .s_axi_awsize (awsize), .s_axi_awburst (awburst), .s_axi_awlock (awlock),
    .s_axi_awcache (awcache), .s_axi_awprot (awprot), .s_axi_awqos (awqos),
    .s_axi_awvalid (awvalid), .s_axi_awready (awready),
    .s_axi_wdata (wdata_axi), .s_axi_wstrb (wstrb), .s_axi_wlast (wlast),
    .s_axi_wvalid (wvalid), .s_axi_wready (wready),
    .s_axi_bid (bid), .s_axi_bresp (bresp), .s_axi_bvalid (bvalid),
    .s_axi_bready (bready),

    .s_axi_arid (arid), .s_axi_araddr (araddr), .s_axi_arlen (arlen),
    .s_axi_arsize (arsize), .s_axi_arburst (arburst), .s_axi_arlock (arlock),
    .s_axi_arcache (arcache), .s_axi_arprot (arprot), .s_axi_arqos (arqos),
    .s_axi_arvalid (arvalid), .s_axi_arready (arready),
    .s_axi_rid (), .s_axi_rdata (rdata_axi), .s_axi_rresp (rresp),
    .s_axi_rlast (rlast), .s_axi_rvalid (rvalid), .s_axi_rready (rready),

    .sys_clk_i (clk_sys_100), .clk_ref_i (clk_ref_200), .sys_rst (sys_rst_n)
  );

  // ---- JTAG readout -------------------------------------------------------
  // SEVENTEEN probes in, ONE out: 3b's five (four 32-bit values plus an 8-bit
  // flag word), the nine operand-path counters, and the three the invocation
  // loop adds (t_span, cyc_total, and the n_inv/folds_done status word).  The
  // two output probes are n_inv itself -- see its declaration for why the count
  // is driven from outside the RTL rather than built in -- and the re-run
  // request that makes a sweep over n_inv one script.  dma_top_build.tcl configures vio_0
  // to match -- 3a's IP had probe_in3 at 8 bits, and reusing a stale
  // configuration silently truncates a 32-bit probe, which still looks like a
  // number.  Probe order is append-only: 0..4 keep their numbers so the
  // 'read' script's fallbacks and every earlier run's printout stay valid.
  //
  // The expected constants are folded parameters with no net behind them, so
  // they are NOT probed; the script prints them from its own variables.  3a
  // shipped a bitstream that printed "expected 0x" for exactly this reason.
  vio_0 u_vio (
    .clk        (ui_clk),
    .probe_in0  (chk_wr),
    .probe_in1  (chk_c),
    .probe_in2  (cyc_latched),
    .probe_in3  (words_written),
    .probe_in4  ({ wb_done_sticky, any_err, c_match, wr_match,
                   fold_done_sticky, read_done_sticky, seed_done_sticky,
                   init_calib_complete }),
    .probe_in5  (fill_cycles),
    .probe_in6  (eng_busy_cycles),
    .probe_in7  (eng_rdy_stall_cycles),
    .probe_in8  (eng_r_stall_cycles),
    .probe_in9  (wb_cycles),
    .probe_in10 (wb_busy_cycles),
    .probe_in11 (wb_aw_stall_cycles),
    .probe_in12 (wb_w_stall_cycles),
    .probe_in13 (wb_src_starve_cycles),
    .probe_in14 (t_span),
    .probe_in15 (cyc_total),
    .probe_in16 ({16'd0, folds_done, 4'd0, n_inv}),
    .probe_in17 (external_debug_sticky),
    .probe_in18 (dpti_phy_debug_sync),
    .probe_in19 (dpti_clk_activity_sync),
    .probe_out0 (n_inv_probe),
    .probe_out1 (rerun_probe)
  );

  // ---- LEDs ---------------------------------------------------------------
  (* keep = "true" *) reg [25:0] hb_ui = 26'd0;
  always_ff @(posedge ui_clk) hb_ui <= hb_ui + 1'b1;

  assign led = { hb_ui[25], any_err, c_match, wr_match,
                 fold_done_sticky, read_done_sticky, seed_done_sticky,
                 init_calib_complete };

`ifndef SYNTHESIS
  initial begin
    if (K_DIM > K_MAX)
      $fatal(1, "K_DIM %0d exceeds K_MAX %0d -- the golden would not match", K_DIM, K_MAX);

    // External jobs must stay within the physical operand-buffer capacity.
    // job_k_reg is zero before the first accepted job, so only validate it
    // after the scheduler has actually accepted a descriptor.
    if (USE_EXTERNAL_SCHEDULER && job_k_reg != 0 &&
        (job_k_reg > K_MAX || job_k_reg < 1))
      $fatal(1, "scheduler ingress_job_k %0d is outside 1..%0d", job_k_reg, K_MAX);
    // The slab and tile strides are shifted, not multiplied.  If either stride
    // stopped being a power of two the shift would silently address the wrong
    // slab, so it is checked here rather than assumed in a comment.
    if ((1 << RX_SHIFT) != RX_BYTES)
      $fatal(1, "RX_BYTES %0d is not a power of two -- slab_addr shifts", RX_BYTES);
    if ((1 << RXW_SHIFT) != RX_WORDS)
      $fatal(1, "RX_WORDS %0d is not a power of two -- words_want shifts", RX_WORDS);
    if ((1 << WBT_SHIFT_8X8) != WB_TILE_BYTES_8X8)
      $fatal(1, "WB_TILE_BYTES_8X8 is not a power of two");

    if ((1 << WBT_SHIFT_4X4) != WB_TILE_BYTES_4X4)
      $fatal(1, "WB_TILE_BYTES_4X4 is not a power of two");
  end
`endif

endmodule

`default_nettype wire
