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
//
// PER-ACCELERATOR CONTEXTS (Oct 2026)
//   The per-job / per-tile control that this file used to hold once -- for
//   whichever device the accepted job named -- now lives in
//   systolic_accel_context.sv and is instantiated once per physical
//   accelerator: one 8x8 context (K_MAX_8X8 deep) and NUM_4X4 4x4 contexts
//   (K_MAX_4X4 deep), each with its own feeder, operand ping-pong buffers,
//   result snapshot banks, phase FSM, write-back stage and counters.  The
//   hardware scheduler likewise keeps one execution context per accelerator.
//   So a job on device 0 and a job on device 1 are accepted, filled, computed
//   and written back independently; the 8x8 and a 4x4 COMPUTE at the same
//   time.  What stays single is what is physically single: one AXI read
//   master, one AXI write master, the seeder, the ingress, MIG.  Those move
//   bytes one channel at a time; they do not serialize the computing:
//
//   - The read engine (dma_engine_multi) holds one operand descriptor per
//     context in flight at once and shares the read channel between them
//     beat by beat, so two fills proceed together instead of one after the
//     other.  With the v1 writer (one word per cycle per context) that is what
//     lets the second array's operands land while the first array is still
//     being filled -- a single-descriptor engine made the 4x4 wait ~257
//     cycles for the 8x8's payload before its own fill could even begin, and
//     its fold then started after the 8x8's had ended.
//   - The write-back engine takes one tile at a time (a few beats each); a
//     context whose tile waits keeps computing.
//
//   The compiler-facing acceptance (job_valid / job_ready) is per device:
//   a descriptor is taken when the context it names is free, whatever the
//   other contexts are doing.  job_ready is the registered decision for the
//   descriptor presented the cycle before, so it is a function of the
//   design's own state only, never of the payload the producer is changing
//   in the same cycle.
//
//   Context 0 (the 8x8) carries the legacy bring-up: the seeder, the VIO
//   n_inv / re-run probes, and the counters the bench and the board scripts
//   read, which are exposed under their old names as context 0's.
//
// TWO INDEPENDENT K_MAX (Oct 2026)
//   K_MAX_8X8 is the 8x8 contexts' operand depth and K_MAX_4X4 every 4x4
//   context's.  They are independent, and each is any positive integer the
//   descriptor's 16-bit beat count can carry (K <= 16376; the BRAM budget is
//   the practical bound): nothing below folds them into one value or assumes
//   a power of two.  What they decide here:
//     - the host's MEM_WRITE payload (HOST_SLAB_BYTES): one operand slab of
//       the DEEPEST geometry, ceil(K_MAX / 8) whole 8-step windows, so a slab
//       for any accelerator fits it -- 1024 bytes at 16 / 16, as before;
//     - which descriptors are accepted: a job's k must be 1..K_MAX of the
//       device it names.  Any other k is refused (never accepted, err_job_k
//       latched into any_err / led[6]) instead of being run past the end of
//       that context's buffers.
// -----------------------------------------------------------------------------

`default_nettype none

module systolic_dma_top #(
  parameter integer N     = 8,
  // K_MAX = 16 for the first 3b build: the geometry whose golden was confirmed
  // on hardware, a 1 KiB payload, and a build measured in minutes.  3b is a
  // correctness step; K_MAX = 256 belongs to 3c, where bandwidth is the point.
  parameter integer K_MAX_8X8 = 32,

  parameter integer K_MAX_4X4 = 16,

  parameter integer K_DIM     = 16,             // runtime reduction length, <= K_MAX

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

  // The PE boundary gap the contexts keep between two invocations' operands
  // (systolic_accel_context.sv PE_QUIET_GAP / FEED_GAP_MARGIN; the defaults
  // are the contexts' own).  Exposed so a bench can shrink the gap below the
  // PE's minimum and show that the boundary check catches merged tiles.
  parameter integer PE_QUIET_GAP    = 23,
  parameter integer FEED_GAP_MARGIN = 3,

  // Legacy compiler/testbench descriptor producer.
  //
  // 1: allow the top-level job_* ports to provide descriptors.
  // 0: production board build uses only the internal descriptor bridge.
  //
  // Keep the default enabled so existing simulation testbenches are unchanged.
  parameter bit USE_LEGACY_JOB_PORTS = 1'b1,

  // FT2232H physical layer.
  //
  // 1: dpti_sync245_phy -- registered pads (IOB candidates), registered OE#,
  //    RD#/RXF# qualification at the capture edge, RX/TX FIFOs, bus
  //    arbitration with turnaround, TX acceptance confirmed by TXE#/WR#.
  // 0: the original dpti_byte_rx + dpti_byte_tx and their glue, unchanged.
  // Both drive the same dpti_byte_* (RX) and rb_tx_byte_* (TX) streams.
  parameter bit DPTI_PHY_SYNC245 = 1'b1,

  // dpti_clk_gen's CLKOUT0 phase (degrees, 360 = 16.667 ns).  0 unless the
  // routed I/O slacks call for a shift; see dpti_clk_gen.sv, PHASE.
  parameter real DPTI_CLKOUT0_PHASE = 0.0
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
  //   dpti_clkout = FT2232H CLKO; it drives only dpti_clk_gen's MMCM, whose
  //   output clk_dpti clocks the DPTI domain
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
  localparam integer ACC_ID_W = (NUM_ACCEL <= 1) ? 1 : $clog2(NUM_ACCEL);

  localparam integer AXI_DATA_W = 128;
  localparam integer AXI_ADDR_W = 29;
  // A slab carries whole 8-step windows: a K_MAX-deep context reads
  // ceil(K_MAX / 8) of them (systolic_accel_context, dma_operand_writer).
  localparam integer SLAB_K_8X8 = ((K_MAX_8X8 + 7) / 8) * 8;
  localparam integer SLAB_K_4X4 = ((K_MAX_4X4 + 7) / 8) * 8;
  // The legacy bring-up (seeder, VIO n_inv, the bench's counters) runs on
  // context 0, the 8x8, whose slab is SLAB_K_8X8 deep.
  localparam integer RX_BYTES   = SLAB_K_8X8 * 8 * N;
  localparam integer N_BEATS    = RX_BYTES / (AXI_DATA_W/8);
  // The host stages each invocation's operands with one MEM_WRITE of a fixed
  // length: the deepest slab of any geometry present, so that every
  // accelerator's slab fits in it whichever of K_MAX_8X8 / K_MAX_4X4 is larger.
  localparam integer HOST_SLAB_K =
      (NUM_8X8 == 0)                                  ? SLAB_K_4X4 :
      (NUM_4X4 == 0 || SLAB_K_8X8 >= SLAB_K_4X4)      ? SLAB_K_8X8 :
                                                        SLAB_K_4X4;
  localparam integer HOST_SLAB_BYTES = HOST_SLAB_K * 8 * N;
  localparam integer HOST_SLAB_BEATS = HOST_SLAB_BYTES / (AXI_DATA_W/8);

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

  // ---- AXI write channel: three masters, one at a time, at burst boundaries
  // The host-staging engine (a MEM_WRITE landing in DDR), the write-back
  // engine (a result tile) and the legacy seeder share the one AXI write
  // port of the MIG.  Until the asynchronous host (docs/ASYNC.md) this was a
  // priority mux keyed on hs_busy: the moment a MEM_WRITE started, every
  // write-channel signal switched to the staging engine -- inside a
  // write-back burst if that was where the engine happened to be.  The MIG
  // counts a burst's beats from its AW, so the staging beats landed in the
  // result tile and the tile's beats in the slab.  A synchronous host never
  // sends a MEM_WRITE while a tile is being written back, which is the only
  // reason that mux was ever correct.
  //
  // Now a grant: a master gets the channel when it raises awvalid (or wvalid)
  // while the channel is free, keeps it while it has a burst open (AW
  // accepted, B not yet returned) or a request pending, and releases it at
  // that boundary.  Priority at a free channel: host staging, then write-back,
  // then the seeder.  A master that is not granted sees ready low and holds
  // its valid, as AXI requires of it, so nothing is lost -- only delayed by
  // at most the bursts the current owner has in flight.
  //
  // err_w_owner is the arbiter's self-check: a handshake on the bus that
  // no granted master made.  It feeds any_err like every other fault.
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

  // The grant.
  localparam [1:0] WG_NONE = 2'd0, WG_HS = 2'd1, WG_WB = 2'd2, WG_SD = 2'd3;
  logic [1:0] w_grant;
  logic [1:0] w_last;          // the owner before the last release: loses the next tie
  logic [3:0] w_open;          // bursts the owner has issued whose B is still to come
  logic       err_w_owner;

  wire hs_wants = hs_awvalid || hs_wvalid;
  wire wb_wants = wb_awvalid || wb_wvalid;
  wire sd_wants = sd_awvalid || sd_wvalid;

  wire g_hs = (w_grant == WG_HS);
  wire g_wb = (w_grant == WG_WB);
  wire g_sd = (w_grant == WG_SD);

  // The owner yields at a burst boundary when another master waits, once
  // it has had at least one burst since it was granted (w_served; without
  // that the two would hand the channel back and forth without either
  // issuing anything): its next AW is held back (not presented to the
  // controller, not accepted) for the one cycle the grant takes to move,
  // so a boundary can be seen at all -- the slab writer asks for its next
  // burst in the very cycle the previous one is answered.
  logic w_served;              // the owner has issued a burst since the grant
  wire w_quiet  = (w_open == 4'd0) && !(bvalid && bready);
  wire hs_yield = g_hs && w_served && w_quiet && (wb_wants || sd_wants);
  wire wb_yield = g_wb && w_served && w_quiet && (hs_wants || sd_wants);

  assign awid       = g_hs ? hs_awid    : g_wb ? wb_awid    : sd_awid;
  assign awaddr     = g_hs ? hs_awaddr  : g_wb ? wb_awaddr  : sd_awaddr;
  assign awlen      = g_hs ? hs_awlen   : g_wb ? wb_awlen   : sd_awlen;
  assign awsize     = g_hs ? hs_awsize  : g_wb ? wb_awsize  : sd_awsize;
  assign awburst    = g_hs ? hs_awburst : g_wb ? wb_awburst : sd_awburst;
  assign awlock     = g_hs ? hs_awlock  : g_wb ? wb_awlock  : sd_awlock;
  assign awcache    = g_hs ? hs_awcache : g_wb ? wb_awcache : sd_awcache;
  assign awprot     = g_hs ? hs_awprot  : g_wb ? wb_awprot  : sd_awprot;
  assign awqos      = g_hs ? hs_awqos   : g_wb ? wb_awqos   : sd_awqos;
  assign awvalid    = g_hs ? (hs_awvalid && !hs_yield) : g_wb ? (wb_awvalid && !wb_yield) : g_sd ? sd_awvalid : 1'b0;

  assign hs_awready = g_hs && !hs_yield && awready;
  assign wb_awready = g_wb && !wb_yield && awready;
  assign sd_awready = g_sd && awready;

  assign wdata_axi  = g_hs ? hs_wdata  : g_wb ? wb_wdata  : sd_wdata;
  assign wstrb      = g_hs ? hs_wstrb  : g_wb ? wb_wstrb  : sd_wstrb;
  assign wlast      = g_hs ? hs_wlast  : g_wb ? wb_wlast  : sd_wlast;
  assign wvalid     = g_hs ? hs_wvalid : g_wb ? wb_wvalid : g_sd ? sd_wvalid : 1'b0;

  assign hs_wready  = g_hs && wready;
  assign wb_wready  = g_wb && wready;
  assign sd_wready  = g_sd && wready;

  // The response goes to the owner only.
  assign bready     = g_hs ? hs_bready : g_wb ? wb_bready : g_sd ? sd_bready : 1'b1;
  assign hs_bvalid  = g_hs && bvalid;
  assign wb_bvalid  = g_wb && bvalid;
  assign sd_bvalid  = g_sd && bvalid;

  wire aw_hs = awvalid && awready;
  wire b_hs  = bvalid  && bready;

  // A burst boundary of the owner: nothing open, no handshake in this very
  // cycle.  The grant moves only there, so a burst is never interleaved.
  wire w_boundary = w_quiet && !aw_hs;

  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n) begin
      w_grant     <= WG_NONE;
      w_last      <= WG_NONE;
      w_served    <= 1'b0;
      w_open      <= 4'd0;
      err_w_owner <= 1'b0;
    end
    else begin
      // Bursts in flight for the owner: +1 at its AW, -1 at its B.
      if (aw_hs && !b_hs)      w_open <= w_open + 4'd1;
      else if (b_hs && !aw_hs) w_open <= w_open - 4'd1;
      if (w_grant == WG_NONE)  w_served <= 1'b0;
      else if (aw_hs)          w_served <= 1'b1;

      case (w_grant)
        // Round robin between the host slab writer and the write-back
        // engine: the one that held the channel last loses the tie.  The
        // slab writer asks for its next burst the moment the previous one
        // is answered and dribbles each burst's beats in at the DPTI link's
        // pace, so a fixed priority would hold the channel for a whole slab
        // (thousands of cycles) while the contexts' finished tiles waited to
        // be written back -- and, with jobs pipelined, their result banks
        // and then their folds behind them (systolic_accel_context.sv
        // result_bank_free).  Alternating at burst boundaries costs the slab
        // writer nothing it is not already waiting for.
        WG_NONE: begin
          if (hs_wants && wb_wants) w_grant <= (w_last == WG_HS) ? WG_WB : WG_HS;
          else if (hs_wants)        w_grant <= WG_HS;
          else if (wb_wants)        w_grant <= WG_WB;
          else if (sd_wants)        w_grant <= WG_SD;
        end
        // Release at a burst boundary: when the owner has nothing pending,
        // or when another master is waiting for its turn.
        WG_HS: if (w_boundary && (!hs_wants || hs_yield)) begin w_grant <= WG_NONE; w_last <= WG_HS; end
        WG_WB: if (w_boundary && (!wb_wants || wb_yield)) begin w_grant <= WG_NONE; w_last <= WG_WB; end
        WG_SD: if (w_boundary && !sd_wants)                             begin w_grant <= WG_NONE; w_last <= WG_SD; end
        default: w_grant <= WG_NONE;
      endcase

      // Self-check: a handshake on the bus with no owner.
      if ((w_grant == WG_NONE) && ((awvalid && awready) || (wvalid && wready) || (bvalid && bready)))
        err_w_owner <= 1'b1;
    end
  end

  // ---- AXI read channel: the engine ---------------------------------------
  wire [1:0]   arid;   wire [28:0] araddr;  wire [7:0] arlen;
  wire [2:0]   arsize; wire [1:0]  arburst; wire [0:0] arlock;
  wire [3:0]   arcache; wire [2:0] arprot;  wire [3:0] arqos;
  wire         arvalid, arready;
  wire [127:0] rdata_axi; wire [1:0] rresp; wire rlast, rvalid, rready;

  // =========================================================================
  // Hardware scheduler: one execution context per physical accelerator.
  //
  // Each accelerator context (below) raises its own tile request; the
  // scheduler's context for that accelerator takes it when idle, waits for
  // the compiler's start cycle and for the datapath window
  // (accelerator_ready = that context in P_GO with a landed tile), pulses
  // accelerator_start[i] into adapter i, and waits for accelerator_done[i]
  // from that adapter -- which is driven by that array's own c_valid_out.
  // Nothing in here is shared between accelerators except the schedule clock.
  // =========================================================================
  wire [NUM_ACCEL-1:0] scheduler_accelerator_start;
  wire [NUM_ACCEL-1:0] scheduler_accelerator_done;
  wire [NUM_ACCEL-1:0] scheduler_accelerator_ready;
  wire [NUM_ACCEL-1:0] scheduler_req_valid;
  wire [NUM_ACCEL-1:0] scheduler_req_ready;
  wire [NUM_ACCEL-1:0] scheduler_req_ack;
  wire [31:0]          scheduler_req_start_cycle    [0:NUM_ACCEL-1];
  wire [31:0]          scheduler_req_compute_cycles [0:NUM_ACCEL-1];
  wire [NUM_ACCEL-1:0] scheduler_busy;
  wire [NUM_ACCEL-1:0] scheduler_schedule_done;
  wire [31:0]          scheduler_cycle_counter;
  wire [31:0]          scheduler_active_start_cycle    [0:NUM_ACCEL-1];
  wire [31:0]          scheduler_active_compute_cycles [0:NUM_ACCEL-1];

  // The adapters' releases, one per physical array.
  wire scheduler_fold_start_8x8 [0:NUM_8X8_STORAGE-1];
  wire scheduler_fold_start_4x4 [0:NUM_4X4_STORAGE-1];

  systolic_hw_scheduler #(
    .NUM_ACCEL (NUM_ACCEL),
    .CYCLE_W   (32)
  ) u_hw_scheduler (
    .clk                    (ui_clk),
    .rst                    (!ui_rst_n),
    .req_valid              (scheduler_req_valid),
    .req_ready              (scheduler_req_ready),
    .req_ack                (scheduler_req_ack),
    .req_start_cycle        (scheduler_req_start_cycle),
    .req_compute_cycles     (scheduler_req_compute_cycles),
    .accelerator_ready      (scheduler_accelerator_ready),
    .accelerator_start      (scheduler_accelerator_start),
    .accelerator_done       (scheduler_accelerator_done),
    .busy                   (scheduler_busy),
    .schedule_done          (scheduler_schedule_done),
    .cycle_counter          (scheduler_cycle_counter),
    .active_start_cycle     (scheduler_active_start_cycle),
    .active_compute_cycles  (scheduler_active_compute_cycles)
  );

  // Array-side nets.  These names are the ones the bench observes: the
  // valids are exactly the arrays' a_valid_in / b_valid_in ports, the
  // c_valid_out_* their publish pulses.
  logic [31:0] a_in_8x8        [0:NUM_8X8_STORAGE-1][0:N-1];
  logic [31:0] b_in_8x8        [0:NUM_8X8_STORAGE-1][0:N-1];
  logic        a_valid_to_8x8  [0:NUM_8X8_STORAGE-1][0:N-1];
  logic        b_valid_to_8x8  [0:NUM_8X8_STORAGE-1][0:N-1];
  logic        c_valid_out_8x8 [0:NUM_8X8_STORAGE-1];
  logic [31:0] c_out_8x8       [0:NUM_8X8_STORAGE-1][0:N-1][0:N-1];

  logic [31:0] a_in_4x4        [0:NUM_4X4_STORAGE-1][0:3];
  logic [31:0] b_in_4x4        [0:NUM_4X4_STORAGE-1][0:3];
  logic        a_valid_to_4x4  [0:NUM_4X4_STORAGE-1][0:3];
  logic        b_valid_to_4x4  [0:NUM_4X4_STORAGE-1][0:3];
  logic        c_valid_out_4x4 [0:NUM_4X4_STORAGE-1];
  logic [31:0] c_out_4x4       [0:NUM_4X4_STORAGE-1][0:3][0:3];

  generate
    for (genvar i = 0; i < NUM_8X8; i++) begin : SCHED_8X8
      systolic_hw_scheduler_adapter #(.CYCLE_W(32)) u_adapter (
        .clk(ui_clk), .rst(!ui_rst_n),
        .accelerator_start(scheduler_accelerator_start[i]),
        .accelerator_done(scheduler_accelerator_done[i]),
        .fold_start(scheduler_fold_start_8x8[i]),
        .c_done(c_valid_out_8x8[i])
      );
    end

    for (genvar i = 0; i < NUM_4X4; i++) begin : SCHED_4X4
      systolic_hw_scheduler_adapter #(.CYCLE_W(32)) u_adapter (
        .clk(ui_clk), .rst(!ui_rst_n),
        .accelerator_start(scheduler_accelerator_start[NUM_8X8+i]),
        .accelerator_done(scheduler_accelerator_done[NUM_8X8+i]),
        .fold_start(scheduler_fold_start_4x4[i]),
        .c_done(c_valid_out_4x4[i])
      );
    end
  endgenerate

  // =========================================================================
  // Per-context state, indexed by physical accelerator (device ID):
  //   0 .. NUM_8X8-1            the 8x8 contexts
  //   NUM_8X8 .. NUM_ACCEL-1    the 4x4 contexts
  // =========================================================================
  wire [3:0]  ctx_phase            [0:NUM_ACCEL-1];
  wire        ctx_job_busy         [0:NUM_ACCEL-1];
  wire        ctx_job_active       [0:NUM_ACCEL-1];
  wire        ctx_job_accept_ready [0:NUM_ACCEL-1];
  wire        ctx_job_done         [0:NUM_ACCEL-1];
  wire        ctx_job_fire         [0:NUM_ACCEL-1];
  wire [31:0] ctx_job_id           [0:NUM_ACCEL-1];
  wire [31:0] ctx_job_device_id    [0:NUM_ACCEL-1];
  wire [31:0] ctx_job_k            [0:NUM_ACCEL-1];
  wire [63:0] ctx_job_c_base       [0:NUM_ACCEL-1];
  wire        ctx_scheduler_c_done [0:NUM_ACCEL-1];
  wire        ctx_run_clear        [0:NUM_ACCEL-1];
  wire [31:0] ctx_words_written    [0:NUM_ACCEL-1];
  wire [31:0] ctx_chk_wr           [0:NUM_ACCEL-1];
  wire [31:0] ctx_want_wr          [0:NUM_ACCEL-1];
  wire [31:0] ctx_chk_c            [0:NUM_ACCEL-1];
  wire [31:0] ctx_want_c           [0:NUM_ACCEL-1];
  wire [7:0]  ctx_folds_done       [0:NUM_ACCEL-1];
  wire [3:0]  ctx_n_inv            [0:NUM_ACCEL-1];
  wire [31:0] ctx_cyc_latched      [0:NUM_ACCEL-1];
  wire [31:0] ctx_cyc_total        [0:NUM_ACCEL-1];
  wire [31:0] ctx_fill_cycles      [0:NUM_ACCEL-1];
  wire [31:0] ctx_wb_cycles        [0:NUM_ACCEL-1];
  wire [31:0] ctx_t_span           [0:NUM_ACCEL-1];
  wire        ctx_seed_done_sticky [0:NUM_ACCEL-1];
  wire        ctx_read_done_sticky [0:NUM_ACCEL-1];
  wire        ctx_fold_done_sticky [0:NUM_ACCEL-1];
  wire        ctx_wb_done_sticky   [0:NUM_ACCEL-1];
  wire        ctx_wr_match         [0:NUM_ACCEL-1];
  wire        ctx_c_match          [0:NUM_ACCEL-1];
  wire        ctx_wr_err_range     [0:NUM_ACCEL-1];
  wire [AXI_ADDR_W-1:0] ctx_wb_region_base [0:NUM_ACCEL-1];

  // shared DMA read engine, per requester
  wire                   ctx_op_desc_valid  [0:NUM_ACCEL-1];
  wire [AXI_ADDR_W-1:0]  ctx_op_desc_addr   [0:NUM_ACCEL-1];
  wire [15:0]            ctx_op_desc_beats  [0:NUM_ACCEL-1];
  wire                   ctx_op_desc_ready  [0:NUM_ACCEL-1];
  wire                   ctx_dst_wr_en      [0:NUM_ACCEL-1];
  wire                   ctx_dst_full       [0:NUM_ACCEL-1];
  wire                   ctx_dst_almost_full[0:NUM_ACCEL-1];
  wire                   ctx_read_done      [0:NUM_ACCEL-1];

  // shared write-back engine, per requester
  wire                   ctx_wb_desc_valid  [0:NUM_ACCEL-1];
  wire [AXI_ADDR_W-1:0]  ctx_wb_desc_addr   [0:NUM_ACCEL-1];
  wire [15:0]            ctx_wb_desc_beats  [0:NUM_ACCEL-1];
  wire                   ctx_wb_desc_ready  [0:NUM_ACCEL-1];
  wire                   ctx_wb_done        [0:NUM_ACCEL-1];
  wire                   ctx_src_valid      [0:NUM_ACCEL-1];
  wire [AXI_DATA_W-1:0]  ctx_src_data       [0:NUM_ACCEL-1];
  wire                   ctx_src_ready      [0:NUM_ACCEL-1];
  wire                   ctx_wb_active      [0:NUM_ACCEL-1];

  // seeder (legacy; only context 0 ever pulses it)
  wire                   ctx_seed_start     [0:NUM_ACCEL-1];
  wire [AXI_ADDR_W-1:0]  ctx_seed_slab_addr [0:NUM_ACCEL-1];

  logic [31:0] C_8x8 [0:NUM_8X8_STORAGE-1][0:N-1][0:N-1];
  logic [31:0] C_4x4 [0:NUM_4X4_STORAGE-1][0:3][0:3];

  // ---- the legacy run's knobs (context 0) ---------------------------------
  wire  [3:0] n_inv_probe;
  wire        rerun_probe;
  logic       rerun_d;
  wire        rerun_pulse = rerun_probe & ~rerun_d;

  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n) rerun_d <= 1'b0;
    else           rerun_d <= rerun_probe;
  end

  wire seed_busy, seed_done, seed_err_align, seed_err_resp;
  wire eng_err_align, eng_err_resp;
  wire wb_err_align, wb_err_resp;

  // the shared read engine's destination stream (routed to contexts by tag)
  wire          dst_wr_en;
  wire [15:0]   dst_wr_beat;
  wire [127:0]  dst_wr_data;
  wire [7:0]    dst_wr_tag;
  wire          dst_full;
  // Result-output CDC backpressure.  The CDC itself sits with the DPTI path.
  wire          rb_cdc_src_ready;


  // ==========================================================
  // Job acceptance.
  //
  // A descriptor is accepted when the context of the device it names can
  // queue it (job_accept_ready: its job queue has room -- JOB_QUEUE_DEPTH
  // jobs accepted and not yet written back) and that context's readback
  // queue has room for the result.  Another context's job being in flight
  // does not matter -- that is the point.  Nor does this context's: a job
  // is taken while the previous one is still being filled, fed, reduced or
  // written back, which is what lets the context pipeline them
  // (systolic_accel_context.sv, PIPELINED JOBS).  Until Oct 2026 acceptance
  // waited for the previous job's write-back and readback and for the
  // scheduler slot to be idle -- one job per context, every invocation of an
  // accelerator strictly after the previous one.
  // ==========================================================
  wire [31:0] effective_job_device_id;             // defined with the ingress
  wire        target_valid = (effective_job_device_id < NUM_ACCEL);
  wire [ACC_ID_W-1:0] target_idx =
      target_valid ? effective_job_device_id[ACC_ID_W-1:0] : '0;

  // The job's k must fit the operand buffers of the accelerator it names:
  // 1..K_MAX_8X8 for an 8x8, 1..K_MAX_4X4 for a 4x4.  A descriptor outside
  // that is refused (never accepted, like one naming no accelerator) and
  // latched in err_job_k; accepting it would run the feeder past the end of
  // that context's buffers.
  wire [31:0] effective_job_k;                     // defined with the ingress
  wire        target_is_8x8 = (effective_job_device_id < NUM_8X8);
  wire        job_k_ok =
      (effective_job_k != 32'd0) &&
      (effective_job_k <= (target_is_8x8 ? 32'(K_MAX_8X8) : 32'(K_MAX_4X4)));
  logic       err_job_k;

  // Completed jobs wait in a per-context queue for the shared DMA read engine
  // to read their result tiles back; a context whose readback queue is full
  // takes no new job until one has been read back.
  wire result_readback_busy [0:NUM_ACCEL-1];

  wire core_job_ready =
      USE_EXTERNAL_SCHEDULER &&
      ui_rst_n &&
      !hs_busy &&
      target_valid &&
      job_k_ok &&
      ctx_job_accept_ready[target_idx] &&
      !result_readback_busy[target_idx];

  // The scheduler slot is no longer part of acceptance: the context raises
  // one request per queued job itself (tile_req) and the slot is released at
  // each start, so a job can be accepted and filled while the slot is still
  // holding the previous job's release.
  wire scheduler_desc_ready = 1'b1;

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
  // (DPTI_PHY_SYNC245 = 0; the pads are driven in g_legacy_phy below.)
  wire [7:0] dpti_tx_data;
  wire       dpti_tx_oe;
  wire       dpti_tx_wr_n;

  // What dpti_phy_debug records as "RXF# seen low" / "RD# asserted": the pads
  // themselves for the legacy layer, registered copies for the sync245 PHY so
  // that each pad keeps a single (IOB) register load.
  wire       dpti_dbg_rxf_avail;
  wire       dpti_dbg_rd_active;

  // ------------------------------------------------------------------------
  // DPTI clock and link reset.
  //
  // Every register of the DPTI domain runs on clk_dpti: CLKOUT regenerated by
  // dpti_clk_gen's MMCM so that it is aligned at the IOB registers with
  // CLKOUT at the pin (see that file for the topology and why).  The port
  // dpti_clkout feeds nothing but the MMCM.
  //
  // dpti_rst (clk_dpti) and dpti_ui_link_rst (ui_clk, the ui_clk halves of
  // the three DPTI crossings) come from dpti_link_ctrl, which takes both down
  // together when MMCM LOCKED drops or on ui_rst_n, lets the ui_clk side drain
  // what already crossed, and brings both up together once LOCKED is back.
  // A MEM_WRITE cut off by a lost link latches dpti_link_fatal (any_err).
  // ------------------------------------------------------------------------

  wire       clk_dpti;
  wire       dpti_mmcm_locked;
  wire       dpti_hold_mmcm;
  wire [7:0] dpti_mmcm_reset_count;     // clk_sys_100

  dpti_clk_gen #(
    .CLKOUT0_PHASE (DPTI_CLKOUT0_PHASE)
  ) u_dpti_clk_gen (
    .clk_sys          (clk_sys_100),
    .clkout_pad       (dpti_clkout),
    .hold_rst         (dpti_hold_mmcm),
    .clk_dpti         (clk_dpti),
    .locked           (dpti_mmcm_locked),
    .mmcm_reset_count (dpti_mmcm_reset_count)
  );

  wire       dpti_rst;
  wire       dpti_ui_link_rst;
  wire       dpti_link_up;
  wire       dpti_link_fatal;
  wire [7:0] dpti_link_drop_count;
  wire       dpti_locked_ui;
  wire [2:0] dpti_link_state;

  dpti_link_ctrl u_dpti_link_ctrl (
    .ui_clk            (ui_clk),
    .ui_rst_n          (ui_rst_n),
    .mmcm_locked       (dpti_mmcm_locked),
    .mem_writer_busy   (hs_busy),
    .cmd_fifo_rd_valid (dpti_fifo_rd_valid),
    .cmd_master_idle   (dpti_axi_cmd_ready),
    .clk_dpti          (clk_dpti),
    .dpti_rst          (dpti_rst),
    .ui_link_rst       (dpti_ui_link_rst),
    .hold_mmcm         (dpti_hold_mmcm),
    .link_up           (dpti_link_up),
    .err_link_fatal    (dpti_link_fatal),
    .link_drop_count   (dpti_link_drop_count),
    .locked_ui         (dpti_locked_ui),
    .state_o           (dpti_link_state)
  );

  // Observation only: the MMCM reset count, a clk_sys_100 counter, sampled in
  // ui_clk.  A sample taken while it changes may be wrong by one read; it is
  // read for whether it moves, as dpti_clk_activity is.
  (* ASYNC_REG = "TRUE" *) reg [7:0] dpti_mmcm_resets_meta;
  (* ASYNC_REG = "TRUE" *) reg [7:0] dpti_mmcm_resets_ui;

  always @(posedge ui_clk) begin
    dpti_mmcm_resets_meta <= dpti_mmcm_reset_count;
    dpti_mmcm_resets_ui   <= dpti_mmcm_resets_meta;
  end

  // ------------------------------------------------------------------------
  // Physical-input diagnostic state.
  //
  // This state is generated entirely in clk_dpti.  Do not sample the
  // short physical-interface events directly from ui_clk.
  //
  // bit0: clk_dpti has run after reset release
  // bit1: input data was reported available
  // bit2: the receiver asserted its physical read control
  // bit3: the receiver produced a valid byte
  // bit4: one byte completed the frontend valid/ready handshake
  // ------------------------------------------------------------------------

  reg [31:0] dpti_phy_debug;

  // Free-running physical-clock activity counter.
  //
  // Deliberately NOT reset by dpti_rst or ui_rst_n.  This is observation-only:
  // if CLKOUT is reaching the FPGA and the MMCM is locked to it (clk_dpti
  // runs), this counter must change regardless of the state of the
  // physical-interface reset synchronizer.
  reg [31:0] dpti_clk_activity = 32'd0;

  always @(posedge clk_dpti) begin
    dpti_clk_activity <= dpti_clk_activity + 32'd1;
  end

  always @(posedge clk_dpti) begin
    if (dpti_rst) begin
      dpti_phy_debug <= 32'd0;
    end
    else begin
      dpti_phy_debug[0] <= 1'b1;

      if (dpti_dbg_rxf_avail)
        dpti_phy_debug[1] <= 1'b1;

      if (dpti_dbg_rd_active)
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

  // The completion record (readback section) goes in ahead of the tile's
  // DMA beats; the two never overlap, the descriptor is issued after it.
  wire               rb_hdr_valid;
  wire [AXI_DATA_W-1:0] rb_hdr_data;

  dpti_output_cdc #(
    .DATA_W (AXI_DATA_W),
    .DEPTH  (8)
  ) u_dpti_output_cdc (
    .src_clk   (ui_clk),
    .src_rst   (!ui_rst_n || dpti_ui_link_rst),

    .src_data  (rb_hdr_valid ? rb_hdr_data : dst_wr_data),
    .src_valid (rb_hdr_valid | rb_dst_wr_en),
    .src_ready (rb_cdc_src_ready),

    .dst_clk   (clk_dpti),
    .dst_rst   (dpti_rst),

    .dst_data  (rb_cdc_dst_data),
    .dst_valid (rb_cdc_dst_valid),
    .dst_ready (rb_cdc_dst_ready)
  );

  dpti_beat_to_byte #(
    .DATA_W (AXI_DATA_W)
  ) u_dpti_beat_to_byte (
    .clk        (clk_dpti),
    .rst        (dpti_rst),

    .beat_data  (rb_cdc_dst_data),
    .beat_valid (rb_cdc_dst_valid),
    .beat_ready (rb_cdc_dst_ready),

    .byte_data  (rb_tx_byte_data),
    .byte_valid (rb_tx_byte_valid),
    .byte_ready (rb_tx_byte_ready)
  );

  // The legacy pads path arbitrates the bus below; the registered PHY
  // reports its own mode on the same wires.
  wire [2:0]  dpti_bus_state;
  wire [31:0] dpti_bus_turnarounds;

  generate
  if (DPTI_PHY_SYNC245) begin : g_sync245_phy

    // ----------------------------------------------------------------------
    // dpti_sync245_phy.  Clocked by clk_dpti like the rest of the DPTI
    // domain: CLKOUT through dpti_clk_gen's delay-compensating MMCM, which is
    // the alignment this PHY's IOB registers are designed for.
    //
    // Reset: asserted at once -- with or without a clock -- by the link reset,
    // by ui_rst_n, and by MMCM LOCKED falling, so the pads are safe (RD#, OE#,
    // WR# high, D released) the moment the clock can no longer be trusted,
    // even while dpti_link_ctrl is still draining the ui_clk side.  Released
    // after dpti_rst plus the PHY's own two-flop synchronizer, so the command
    // frontend is out of reset before the PHY can present a byte.
    // ----------------------------------------------------------------------
    wire [7:0] pad_d_o;
    wire [7:0] pad_d_t;      // 1 = high-Z

    for (genvar gi = 0; gi < 8; gi = gi + 1) begin : g_pad_d
      assign dpti_d[gi] = pad_d_t[gi] ? 1'bz : pad_d_o[gi];
    end

    // ----------------------------------------------------------------------
    // RX pacing at the boundary to the command frontend.
    //
    // dpti_command_frontend hands a byte to the MEM_WRITE decoder for one
    // more cycle after that decoder has finished (it releases OWNER_MEM on
    // the cycle after mem_done, while the decoder is already back in
    // S_OPCODE with byte_ready = 1).  A byte presented on that cycle is
    // swallowed.  dpti_byte_rx could never present one there: after each
    // consumed byte its next byte becomes valid two edges later at the
    // earliest.  The sync245 PHY's FIFO has the next byte ready at once, and
    // without this pacing the replay bench (tb_dpti_top_replay) loses the two
    // bytes after every MEM_WRITE.  The pacer restores exactly the legacy
    // minimum spacing -- after a byte is taken, the next one is held back
    // for two cycles -- so the frontend sees no input pattern the legacy
    // layer could not produce.  It does not fix the frontend; see the Stage 3
    // report.
    // ----------------------------------------------------------------------
    wire [7:0] phy_rx_data;
    wire       phy_rx_valid;
    wire       phy_rx_ready;
    reg  [1:0] rx_hold;

    always @(posedge clk_dpti) begin
      if (dpti_rst)
        rx_hold <= 2'd0;
      else if (dpti_byte_valid && dpti_byte_ready)
        rx_hold <= 2'd2;
      else if (rx_hold != 2'd0)
        rx_hold <= rx_hold - 2'd1;
    end

    assign dpti_byte_data  = phy_rx_data;
    assign dpti_byte_valid = phy_rx_valid && (rx_hold == 2'd0);
    assign phy_rx_ready    = dpti_byte_ready && (rx_hold == 2'd0);

    // unused in the top: the PHY's own counters and mode
    wire [1:0]  phy_bus_mode;
    wire        phy_err_rx_overflow;
    wire [31:0] phy_rx_xfer_count;
    wire [31:0] phy_tx_accept_count;
    wire [31:0] phy_tx_reject_count;

    dpti_sync245_phy u_dpti_sync245_phy (
      .clk             (clk_dpti),
      .arst            (dpti_rst | !ui_rst_n | !dpti_mmcm_locked),

      .pad_d_i         (dpti_d),
      .pad_d_o         (pad_d_o),
      .pad_d_t         (pad_d_t),
      .pad_rxf_n       (dpti_rxf_n),
      .pad_txe_n       (dpti_txe_n),
      .pad_rd_n        (dpti_rd_n),
      .pad_oe_n        (dpti_oe_n),
      .pad_wr_n        (dpti_wr_n),

      .rx_data         (phy_rx_data),
      .rx_valid        (phy_rx_valid),
      .rx_ready        (phy_rx_ready),

      .tx_data         (rb_tx_byte_data),
      .tx_valid        (rb_tx_byte_valid),
      .tx_ready        (rb_tx_byte_ready),

      .dbg_rxf_avail   (dpti_dbg_rxf_avail),
      .dbg_rd_active   (dpti_dbg_rd_active),
      .bus_mode        (phy_bus_mode),
      .err_rx_overflow (phy_err_rx_overflow),
      .rx_xfer_count   (phy_rx_xfer_count),
      .tx_accept_count (phy_tx_accept_count),
      .tx_reject_count (phy_tx_reject_count)
    );

    assign dpti_tx_data = pad_d_o;
    assign dpti_tx_oe   = 1'b0;
    assign dpti_tx_wr_n = 1'b1;
    assign dpti_bus_state       = {1'b0, phy_bus_mode};   // the PHY arbitrates the bus itself
    assign dpti_bus_turnarounds = 32'd0;

  end
  else begin : g_legacy_phy

    assign dpti_d =
        dpti_tx_oe
            ? dpti_tx_data
            : 8'bz;

    assign dpti_wr_n = dpti_tx_wr_n;

    assign dpti_dbg_rxf_avail = !dpti_rxf_n;
    assign dpti_dbg_rd_active = !dpti_rd_n;


    // ----------------------------------------------------------------------
    // The DPTI bus, both directions, one owner at a time (dpti_bus_arbiter).
    //
    // dpti_byte_rx and dpti_byte_tx used to sit here side by side, each
    // driving its own strobes: a bus fight as soon as a result left while a
    // slab was still coming in. The arbiter keeps the FT2232H's rules (read
    // or write, never both; OE# a cycle ahead of RD#; an idle cycle at every
    // turnaround) and alternates the directions in chunks, results first.
    // The byte handshakes toward dpti_command_frontend and from
    // dpti_beat_to_byte are the two old modules', unchanged.  (The
    // registered PHY above owns both directions by itself.)
    // ----------------------------------------------------------------------
    dpti_bus_arbiter #(
      .CHUNK_BYTES (64)
    ) u_dpti_bus (
      .clk           (clk_dpti),
      .rst           (dpti_rst),

      .dpti_d_in     (dpti_d),
      .dpti_rxf_n    (dpti_rxf_n),
      .dpti_txe_n    (dpti_txe_n),
      .dpti_rd_n     (dpti_rd_n),
      .dpti_oe_n     (dpti_oe_n),
      .dpti_wr_n     (dpti_tx_wr_n),
      .dpti_d_out    (dpti_tx_data),
      .dpti_d_oe     (dpti_tx_oe),

      .rx_byte_data  (dpti_byte_data),
      .rx_byte_valid (dpti_byte_valid),
      .rx_byte_ready (dpti_byte_ready),

      .tx_byte_data  (rb_tx_byte_data),
      .tx_byte_valid (rb_tx_byte_valid),
      .tx_byte_ready (rb_tx_byte_ready),

      .state_o       (dpti_bus_state),
      .turnarounds   (dpti_bus_turnarounds)
    );

  end
  endgenerate

  dpti_command_frontend #(
    .ADDR_W        (8),
    .MEM_ADDR_W    (AXI_ADDR_W),
    .MEM_DATA_W    (AXI_DATA_W),
    .PAYLOAD_BYTES (HOST_SLAB_BYTES)
  ) u_dpti_command_frontend (
    .clk                (clk_dpti),
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
    .TOTAL_BEATS (HOST_SLAB_BEATS)
  ) u_dpti_mem_write_cdc_engine (
    .src_clk       (clk_dpti),
    .src_rst       (dpti_rst),

    .src_start     (dpti_mem_start),
    .src_base_addr (dpti_mem_base_addr),
    .src_data      (dpti_mem_data),
    .src_valid     (dpti_mem_valid),
    .src_ready     (dpti_mem_ready),

    .ui_clk        (ui_clk),
    .ui_rst_n      (ui_rst_n),
    .ui_cdc_rst    (dpti_ui_link_rst),

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
    .wr_clk   (clk_dpti),
    .wr_rst   (dpti_rst),

    .wr_valid (dpti_host_cmd_valid),
    .wr_ready (dpti_fifo_wr_ready),

    .wr_addr  (dpti_host_cmd_addr),
    .wr_data  (dpti_host_cmd_data),

    .rd_clk   (ui_clk),
    .rd_rst   (!ui_rst_n || dpti_ui_link_rst),

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

  wire [31:0] effective_job_m;
  wire [31:0] effective_job_n;

  wire [31:0] effective_job_start_cycle;
  wire [31:0] effective_job_est_cycles;

  wire [63:0] effective_job_a_base;
  wire [63:0] effective_job_b_base;
  wire [63:0] effective_job_c_base;

  // Downstream acceptance boundary -- see "Acceptance" below.  This is the
  // registered decision for the descriptor on the bus; target_ready_now is
  // the readiness of the context that descriptor names, this cycle.
  wire descriptor_downstream_ready;
  wire target_ready_now;

  assign target_ready_now =
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

  // ---- Acceptance ------------------------------------------------------------
  //
  // External producer acceptance is the job ownership boundary: a descriptor
  // is accepted by the compiler-facing interface only when the context of the
  // device it names, and that context's scheduler slot, can take it.  Do not
  // derive job_fire from the registered ingress output: that output may still
  // contain the descriptor of the job that just completed.
  //
  // The readiness is per device, so it depends on which device the descriptor
  // on the bus names.  It is therefore REGISTERED: the decision for the
  // descriptor presented in cycle n is published as job_ready in cycle n+1,
  // together with the device id it was made for, and the transfer happens at
  // the first edge where the producer still presents that descriptor.  So
  // job_ready is a function of the design's registered state alone, never of
  // the payload the producer may be changing in the same cycle, and the value
  // the producer samples at an edge is the value this design uses at that
  // edge: the edge at which the producer sees job_ready high is exactly the
  // edge at which the design takes the descriptor -- in any simulator and on
  // the board.  (A combinational ready that depended on job_device_id was
  // evaluated by the design from the new device id while the producer, which
  // had just driven it, was still looking at the ready of the old one; the
  // design took the descriptor at an edge the producer did not recognise, the
  // producer held job_valid, and the same descriptor was accepted twice.)
  //
  // accept_q is dropped the cycle after a transfer, so a producer that keeps
  // job_valid high for one more cycle does not transfer again.
  //
  // REFUSAL (Oct 2026).  A descriptor the fleet cannot run -- its k outside
  // the named accelerator's 1..K_MAX, or a device id the fleet does not have
  // -- used to sit in the descriptor bridge for ever: never accepted, so
  // nothing behind it was taken either, and the host learned of it only by
  // the link going dead (err_job_k on led[6] if someone was looking).  Now it
  // is taken at the same boundary as an accepted descriptor, by the refusal
  // path instead of a context, and reported to the host as a completion
  // record with status 1 (k) or 2 (device) and no payload -- see the
  // readback section.  One refusal is held at a time; a second one waits in
  // the bridge until the record of the first is on its way, like any
  // descriptor waits for its device.
  logic        accept_q;
  logic        refuse_q;
  logic [31:0] accept_dev_q;
  logic        accept_dpti_q;

  wire job_fire;
  wire refuse_fire;
  logic ref_pending;                         // a refusal record waits to be sent

  wire refuse_now =
      USE_EXTERNAL_SCHEDULER && effective_job_valid &&
      (!target_valid || !job_k_ok);

  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n) begin
      accept_q      <= 1'b0;
      refuse_q      <= 1'b0;
      accept_dev_q  <= 32'd0;
      accept_dpti_q <= 1'b0;
    end else begin
      accept_q      <= effective_job_valid && target_ready_now && !job_fire;
      refuse_q      <= refuse_now && !ref_pending && !refuse_fire && !job_fire;
      accept_dev_q  <= effective_job_device_id;
      accept_dpti_q <= dpti_job_valid;
    end
  end

  // The published decision applies to the descriptor it was made for: same
  // source (DPTI has priority over the legacy ports) and same device.
  wire decision_matches =
      (accept_dev_q  == effective_job_device_id) &&
      (accept_dpti_q == dpti_job_valid);

  assign descriptor_downstream_ready =
      (accept_q || refuse_q) && decision_matches;

  // Legacy external producer readiness.
  //
  // If a DPTI descriptor is pending, the legacy producer is not
  // allowed to believe that its descriptor was accepted.
  assign job_ready =
      USE_LEGACY_JOB_PORTS &&
      !dpti_job_valid &&
      descriptor_downstream_ready;

  // A descriptor is accepted at the common downstream boundary -- by a
  // context, or by the refusal path.
  assign job_fire =
      effective_job_valid &&
      accept_q && decision_matches;

  assign refuse_fire =
      effective_job_valid &&
      refuse_q && decision_matches;

  // What the refusal record carries.
  logic [31:0] ref_job_id;
  logic [7:0]  ref_dev;
  logic [7:0]  ref_status;
  wire         ref_sent;                     // the readback section took it

  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n) begin
      ref_pending <= 1'b0;
      ref_job_id  <= 32'd0;
      ref_dev     <= 8'd0;
      ref_status  <= 8'd0;
    end else begin
      if (refuse_fire) begin
        ref_pending <= 1'b1;
        ref_job_id  <= effective_job_id;
        ref_dev     <= effective_job_device_id[7:0];
        ref_status  <= target_valid ? 8'd1 : 8'd2;
      end else if (ref_sent) begin
        ref_pending <= 1'b0;
      end
    end
  end

  // The scheduler accepts exactly the same accepted descriptor.

  // Route the accepted descriptor to the context of the device it names.
  generate
    for (genvar i = 0; i < NUM_ACCEL; i++) begin : JOB_ROUTE
      assign ctx_job_fire[i] = job_fire && (effective_job_device_id == i);
    end
  endgenerate

  always_ff @(posedge ui_clk) begin
    if (USE_EXTERNAL_SCHEDULER && job_fire)
      $display("JOBACCEPT t=%0t job=%0d -> device %0d k=%0d a=0x%08h c=0x%08h",
               $time, effective_job_id, effective_job_device_id, effective_job_k,
               effective_job_a_base, effective_job_c_base);
    if (USE_EXTERNAL_SCHEDULER && refuse_fire)
      $display("JOBREFUSE t=%0t job=%0d device %0d k=%0d: %s; completion record status %0d",
               $time, effective_job_id, effective_job_device_id, effective_job_k,
               target_valid ? "k outside 1..K_MAX of that device" : "no such device",
               target_valid ? 1 : 2);
  end

  // =========================================================================
  // The accelerators: one context + one array each.
  //
  // ACC_8X8[i].u_acc / ACC_4X4[i].u_acc keep their names; the context that
  // feeds each array sits next to it as u_ctx.
  // =========================================================================
  generate
    for (genvar i = 0; i < NUM_8X8; i++) begin : ACC_8X8
      systolic_accel_context #(
        .N_ARRAY (N), .N_WIRE (N), .K_MAX (K_MAX_8X8), .K_DIM (K_DIM),
        .DEVICE_ID (i),
        .PE_QUIET_GAP (PE_QUIET_GAP), .FEED_GAP_MARGIN (FEED_GAP_MARGIN),
        .USE_V2 (USE_V2), .USE_EXTERNAL_SCHEDULER (USE_EXTERNAL_SCHEDULER),
        .LEGACY_RUN (i == 0),
        .BASE_ADDR (BASE_ADDR), .WB_GAP_BYTES (WB_GAP_BYTES),
        .EXPECT_WR_CHK (EXPECT_WR_CHK), .EXPECT_C_CHK (EXPECT_C_CHK),
        .AXI_DATA_W (AXI_DATA_W), .AXI_ADDR_W (AXI_ADDR_W), .BEAT_W (16)
      ) u_ctx (
        .clk (ui_clk), .ui_rst_n (ui_rst_n), .init_calib_complete (init_calib_complete),
        .n_inv_probe (n_inv_probe), .rerun_pulse (rerun_pulse),
        .job_fire (ctx_job_fire[i]),
        .in_job_id (effective_job_id), .in_device_id (effective_job_device_id),
        .in_k (effective_job_k), .in_start_cycle (effective_job_start_cycle),
        .in_est_cycles (effective_job_est_cycles),
        .in_a_base (effective_job_a_base), .in_b_base (effective_job_b_base),
        .in_c_base (effective_job_c_base),
        .job_busy (ctx_job_busy[i]), .job_active (ctx_job_active[i]),
        .job_accept_ready (ctx_job_accept_ready[i]),
        .job_done (ctx_job_done[i]),
        .job_id_reg (ctx_job_id[i]), .job_device_id_reg (ctx_job_device_id[i]),
        .job_k_reg (ctx_job_k[i]),
        .job_start_cycle_reg (scheduler_req_start_cycle[i]),
        .job_est_cycles_reg (scheduler_req_compute_cycles[i]),
        .job_c_base_reg (ctx_job_c_base[i]),
        .tile_req (scheduler_req_valid[i]), .tile_ack (scheduler_req_ack[i]),
        .accelerator_ready (scheduler_accelerator_ready[i]),
        .fold_start_ext (scheduler_fold_start_8x8[i]),
        .scheduler_c_done (ctx_scheduler_c_done[i]),
        .a_in (a_in_8x8[i]), .b_in (b_in_8x8[i]),
        .a_valid_in (a_valid_to_8x8[i]), .b_valid_in (b_valid_to_8x8[i]),
        .c_valid_out (c_valid_out_8x8[i]), .c_out (c_out_8x8[i]),
        .op_desc_valid (ctx_op_desc_valid[i]), .op_desc_addr (ctx_op_desc_addr[i]),
        .op_desc_beats (ctx_op_desc_beats[i]), .op_desc_ready (ctx_op_desc_ready[i]),
        .dst_wr_en (ctx_dst_wr_en[i]), .dst_wr_beat (dst_wr_beat), .dst_wr_data (dst_wr_data),
        .dst_full (ctx_dst_full[i]), .dst_almost_full (ctx_dst_almost_full[i]),
        .read_done (ctx_read_done[i]),
        .wb_desc_valid (ctx_wb_desc_valid[i]), .wb_desc_addr (ctx_wb_desc_addr[i]),
        .wb_desc_beats (ctx_wb_desc_beats[i]), .wb_desc_ready (ctx_wb_desc_ready[i]),
        .wb_done (ctx_wb_done[i]),
        .src_valid (ctx_src_valid[i]), .src_data (ctx_src_data[i]), .src_ready (ctx_src_ready[i]),
        .wb_active (ctx_wb_active[i]),
        .seed_start (ctx_seed_start[i]), .seed_slab_addr (ctx_seed_slab_addr[i]),
        .seed_done (seed_done),
        .phase_o (ctx_phase[i]), .run_clear (ctx_run_clear[i]),
        .words_written (ctx_words_written[i]), .chk_wr (ctx_chk_wr[i]),
        .want_wr (ctx_want_wr[i]), .chk_c (ctx_chk_c[i]), .want_c (ctx_want_c[i]),
        .folds_done (ctx_folds_done[i]), .n_inv (ctx_n_inv[i]),
        .cyc_latched (ctx_cyc_latched[i]), .cyc_total (ctx_cyc_total[i]),
        .fill_cycles (ctx_fill_cycles[i]), .wb_cycles (ctx_wb_cycles[i]),
        .t_span (ctx_t_span[i]),
        .seed_done_sticky (ctx_seed_done_sticky[i]), .read_done_sticky (ctx_read_done_sticky[i]),
        .fold_done_sticky (ctx_fold_done_sticky[i]), .wb_done_sticky (ctx_wb_done_sticky[i]),
        .wr_match (ctx_wr_match[i]), .c_match (ctx_c_match[i]),
        .wr_err_range (ctx_wr_err_range[i]), .wb_region_base (ctx_wb_region_base[i]),
        .C (C_8x8[i])
      );

      (* keep_hierarchy = "yes" *)
      systolic_array #(
        .N      (N),
        .DATA_W (32)
      ) u_acc (
        .clk         (ui_clk),
        .rst         (rst_i),
        .a_in        (a_in_8x8[i]),
        .b_in        (b_in_8x8[i]),
        .a_valid_in  (a_valid_to_8x8[i]),
        .b_valid_in  (b_valid_to_8x8[i]),
        .c_valid_out (c_valid_out_8x8[i]),
        .c_out       (c_out_8x8[i])
      );
    end
  endgenerate

  generate
    for (genvar i = 0; i < NUM_4X4; i++) begin : ACC_4X4
      localparam integer CTX = NUM_8X8 + i;
      systolic_accel_context #(
        .N_ARRAY (4), .N_WIRE (N), .K_MAX (K_MAX_4X4),
        .K_DIM ((K_DIM < K_MAX_4X4) ? K_DIM : K_MAX_4X4),
        .DEVICE_ID (CTX),
        .PE_QUIET_GAP (PE_QUIET_GAP), .FEED_GAP_MARGIN (FEED_GAP_MARGIN),
        .USE_V2 (USE_V2), .USE_EXTERNAL_SCHEDULER (USE_EXTERNAL_SCHEDULER),
        .LEGACY_RUN (1'b0),
        .BASE_ADDR (BASE_ADDR), .WB_GAP_BYTES (WB_GAP_BYTES),
        .EXPECT_WR_CHK (EXPECT_WR_CHK), .EXPECT_C_CHK (32'h0),
        .AXI_DATA_W (AXI_DATA_W), .AXI_ADDR_W (AXI_ADDR_W), .BEAT_W (16)
      ) u_ctx (
        .clk (ui_clk), .ui_rst_n (ui_rst_n), .init_calib_complete (init_calib_complete),
        .n_inv_probe (n_inv_probe), .rerun_pulse (1'b0),
        .job_fire (ctx_job_fire[CTX]),
        .in_job_id (effective_job_id), .in_device_id (effective_job_device_id),
        .in_k (effective_job_k), .in_start_cycle (effective_job_start_cycle),
        .in_est_cycles (effective_job_est_cycles),
        .in_a_base (effective_job_a_base), .in_b_base (effective_job_b_base),
        .in_c_base (effective_job_c_base),
        .job_busy (ctx_job_busy[CTX]), .job_active (ctx_job_active[CTX]),
        .job_accept_ready (ctx_job_accept_ready[CTX]),
        .job_done (ctx_job_done[CTX]),
        .job_id_reg (ctx_job_id[CTX]), .job_device_id_reg (ctx_job_device_id[CTX]),
        .job_k_reg (ctx_job_k[CTX]),
        .job_start_cycle_reg (scheduler_req_start_cycle[CTX]),
        .job_est_cycles_reg (scheduler_req_compute_cycles[CTX]),
        .job_c_base_reg (ctx_job_c_base[CTX]),
        .tile_req (scheduler_req_valid[CTX]), .tile_ack (scheduler_req_ack[CTX]),
        .accelerator_ready (scheduler_accelerator_ready[CTX]),
        .fold_start_ext (scheduler_fold_start_4x4[i]),
        .scheduler_c_done (ctx_scheduler_c_done[CTX]),
        .a_in (a_in_4x4[i]), .b_in (b_in_4x4[i]),
        .a_valid_in (a_valid_to_4x4[i]), .b_valid_in (b_valid_to_4x4[i]),
        .c_valid_out (c_valid_out_4x4[i]), .c_out (c_out_4x4[i]),
        .op_desc_valid (ctx_op_desc_valid[CTX]), .op_desc_addr (ctx_op_desc_addr[CTX]),
        .op_desc_beats (ctx_op_desc_beats[CTX]), .op_desc_ready (ctx_op_desc_ready[CTX]),
        .dst_wr_en (ctx_dst_wr_en[CTX]), .dst_wr_beat (dst_wr_beat), .dst_wr_data (dst_wr_data),
        .dst_full (ctx_dst_full[CTX]), .dst_almost_full (ctx_dst_almost_full[CTX]),
        .read_done (ctx_read_done[CTX]),
        .wb_desc_valid (ctx_wb_desc_valid[CTX]), .wb_desc_addr (ctx_wb_desc_addr[CTX]),
        .wb_desc_beats (ctx_wb_desc_beats[CTX]), .wb_desc_ready (ctx_wb_desc_ready[CTX]),
        .wb_done (ctx_wb_done[CTX]),
        .src_valid (ctx_src_valid[CTX]), .src_data (ctx_src_data[CTX]), .src_ready (ctx_src_ready[CTX]),
        .wb_active (ctx_wb_active[CTX]),
        .seed_start (ctx_seed_start[CTX]), .seed_slab_addr (ctx_seed_slab_addr[CTX]),
        .seed_done (1'b0),
        .phase_o (ctx_phase[CTX]), .run_clear (ctx_run_clear[CTX]),
        .words_written (ctx_words_written[CTX]), .chk_wr (ctx_chk_wr[CTX]),
        .want_wr (ctx_want_wr[CTX]), .chk_c (ctx_chk_c[CTX]), .want_c (ctx_want_c[CTX]),
        .folds_done (ctx_folds_done[CTX]), .n_inv (ctx_n_inv[CTX]),
        .cyc_latched (ctx_cyc_latched[CTX]), .cyc_total (ctx_cyc_total[CTX]),
        .fill_cycles (ctx_fill_cycles[CTX]), .wb_cycles (ctx_wb_cycles[CTX]),
        .t_span (ctx_t_span[CTX]),
        .seed_done_sticky (ctx_seed_done_sticky[CTX]), .read_done_sticky (ctx_read_done_sticky[CTX]),
        .fold_done_sticky (ctx_fold_done_sticky[CTX]), .wb_done_sticky (ctx_wb_done_sticky[CTX]),
        .wr_match (ctx_wr_match[CTX]), .c_match (ctx_c_match[CTX]),
        .wr_err_range (ctx_wr_err_range[CTX]), .wb_region_base (ctx_wb_region_base[CTX]),
        .C (C_4x4[i])
      );

      (* keep_hierarchy = "yes" *)
      systolic_array #(
        .N        (4),
        .DATA_W   (32),
        .ACC_BANKS(16)
      ) u_acc (
        .clk         (ui_clk),
        .rst         (rst_i),
        .a_in        (a_in_4x4[i]),
        .b_in        (b_in_4x4[i]),
        .a_valid_in  (a_valid_to_4x4[i]),
        .b_valid_in  (b_valid_to_4x4[i]),
        .c_valid_out (c_valid_out_4x4[i]),
        .c_out       (c_out_4x4[i])
      );
    end
  endgenerate

  // =========================================================================
  // Context 0's view under the names the bench, the board scripts and the
  // LEDs have always read.  Every regression job is a device-0 job, so these
  // mean exactly what they meant.
  // =========================================================================
  wire [3:0]  phase            = ctx_phase[0];
  wire [31:0] words_written    = ctx_words_written[0];
  wire [31:0] chk_wr           = ctx_chk_wr[0];
  wire [31:0] want_wr          = ctx_want_wr[0];
  wire [31:0] chk_c            = ctx_chk_c[0];
  wire [31:0] want_c           = ctx_want_c[0];
  wire [7:0]  folds_done       = ctx_folds_done[0];
  wire [3:0]  n_inv            = ctx_n_inv[0];
  wire [31:0] cyc_latched      = ctx_cyc_latched[0];
  wire [31:0] cyc_total        = ctx_cyc_total[0];
  wire [31:0] fill_cycles      = ctx_fill_cycles[0];
  wire [31:0] wb_cycles        = ctx_wb_cycles[0];
  wire [31:0] t_span           = ctx_t_span[0];
  wire        seed_done_sticky = ctx_seed_done_sticky[0];
  wire        read_done_sticky = ctx_read_done_sticky[0];
  wire        fold_done_sticky = ctx_fold_done_sticky[0];
  wire        wb_done_sticky   = ctx_wb_done_sticky[0];
  wire        wr_match         = ctx_wr_match[0];
  wire        c_match          = ctx_c_match[0];
  wire        run_clear        = ctx_run_clear[0];
  wire [31:0] job_id_reg        = ctx_job_id[0];
  wire [31:0] job_device_id_reg = ctx_job_device_id[0];
  wire [31:0] job_k_reg         = ctx_job_k[0];
  wire [AXI_ADDR_W-1:0] wb_region_base = ctx_wb_region_base[0];
  wire [31:0] C [0:N-1][0:N-1];
  generate
    for (genvar r_ = 0; r_ < N; r_++) begin : C_ALIAS_R
      for (genvar c_ = 0; c_ < N; c_++) begin : C_ALIAS_C
        assign C[r_][c_] = C_8x8[0][r_][c_];
      end
    end
  endgenerate

  // the legacy seeder is context 0's
  wire                  seed_start     = ctx_seed_start[0];
  wire [AXI_ADDR_W-1:0] seed_slab_addr = ctx_seed_slab_addr[0];

  // fleet-wide events
  logic job_done, job_busy, job_active, wr_err_range, scheduler_c_done_any;
  logic any_wb_active;
  always_comb begin
    job_done = 1'b0; job_busy = 1'b0; job_active = 1'b0;
    wr_err_range = 1'b0; scheduler_c_done_any = 1'b0; any_wb_active = 1'b0;
    for (int k = 0; k < NUM_ACCEL; k++) begin
      job_done             = job_done | ctx_job_done[k];
      job_busy             = job_busy | ctx_job_busy[k];
      job_active           = job_active | ctx_job_active[k];
      wr_err_range         = wr_err_range | ctx_wr_err_range[k];
      scheduler_c_done_any = scheduler_c_done_any | ctx_scheduler_c_done[k];
      any_wb_active        = any_wb_active | ctx_wb_active[k];
    end
  end
  assign scheduler_c_done = scheduler_c_done_any;

  // ---- AXI write-channel ownership ------------------------------------------
  // Granted per burst above (w_grant); any_wb_active is kept for the LEDs
  // and the bench, which read it as "a tile is on its way to memory".

  // The engines' own counters count from reset (stat_clear follows context
  // 0's run boundary, as before) across every descriptor of every context.
  wire [31:0] eng_busy_cycles, eng_rdy_stall_cycles, eng_r_stall_cycles;
  wire [31:0] wb_busy_cycles, wb_aw_stall_cycles, wb_w_stall_cycles, wb_src_starve_cycles;

  // ---- seeder -------------------------------------------------------------
  dma_seed_writer #(
    .AXI_DATA_W (AXI_DATA_W), .AXI_ADDR_W (AXI_ADDR_W), .AXI_ID_W (2),
    .BURST_LEN (16), .TOTAL_BEATS (N_BEATS),
    .SEED_MODE (1), .MODULUS (127)
  ) u_seed (
    .clk (ui_clk), .rst_n (ui_rst_n),
    .start (seed_start), .base_addr (seed_slab_addr),
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
  // One AXI read master serves every context's operand descriptors and the
  // result readbacks, through dma_engine_multi: one descriptor slot per
  // context plus one for the readback, all in flight at the same time, the
  // read channel shared between them.  The slot's tag names the owner, so the
  // returned beats are routed, and the completion credited, to the context
  // that asked -- no owner register, the tag travels with the data.
  //
  //   8'h30 + i : operand fetch for context i   (slot i)
  //   8'h3C     : result readback               (slot NUM_ACCEL)
  //
  // Why a multi-slot engine and not an arbiter in front of the old one: the
  // v1 writer takes a beat every four cycles and nothing queues in front of
  // it, so a descriptor's bursts hold the channel for the whole fill.  Two
  // contexts filling through a single-descriptor engine filled one after the
  // other (~257 cycles each at k=16) and their arrays computed one after the
  // other.  With a slot per context the engine interleaves the two streams
  // beat by beat under contention (SHARED_BURST_LEN = 1 for v1; the beat-wide
  // v2 writer takes a beat per cycle, so its bursts stay long), and the two
  // payloads land together at the writers' combined rate.  Alone on the
  // channel a slot behaves exactly as dma_engine did.
  //
  // The readback slot is STRICT_GATE: its destination is the DPTI output
  // FIFO, drained at the host's byte rate, so it is given a burst only when
  // the FIFO has room for it -- a slow host does not hold the channel against
  // the other contexts' operand streams.
  localparam logic [7:0] DMA_TAG_OPERAND_BASE = 8'h30;
  localparam logic [7:0] DMA_TAG_READBACK     = 8'h3C;

  localparam integer RD_SLOTS   = NUM_ACCEL + 1;
  localparam integer RD_SLOT_RB = NUM_ACCEL;

  wire [RD_SLOTS-1:0]   rd_desc_valid;
  wire [RD_SLOTS-1:0]   rd_desc_ready;
  wire [AXI_ADDR_W-1:0] rd_desc_addr  [0:RD_SLOTS-1];
  wire [15:0]           rd_desc_beats [0:RD_SLOTS-1];
  wire [7:0]            rd_desc_tag   [0:RD_SLOTS-1];
  wire [RD_SLOTS-1:0]   rd_dst_almost_full;
  wire [RD_SLOTS-1:0]   rd_slot_active;
  wire                  rd_shared;

  wire                  eng_done_valid;
  wire [7:0]            eng_done_tag;

  // ---- result readback: a queue per context, one slot, a completion record ----
  // ---- ahead of every result -------------------------------------------------
  // A context's job completes (its write response returns) possibly before
  // the previous job's result has been read back -- the 8x8's readback takes
  // ~213 cycles through the DPTI output path while a pipelined invocation
  // can complete every ~270 -- so each context keeps a FIFO of the result
  // tiles waiting to be read back ({job_id, c_base}, in completion = job
  // order), RB_DEPTH deep: at least as deep as the context's job queue, so
  // every accepted job has a place -- twice as deep, so that a host draining
  // the DPTI output path at its byte rate (an 8x8 tile takes ~850 array
  // cycles of it in simulation) does not throttle the context's intake
  // before the queue does.  A context with RB_DEPTH jobs accepted and not yet
  // read back takes no new job (result_readback_busy).  Round robin from the
  // context after the last one served grants the slot when it is free, one
  // tile at a time (fixed priority would let a context fed a long 8x8 chain
  // hold the slot for good); the result is in DDR before the read descriptor
  // can be accepted.
  //
  // Every result the host reads back starts with one 16-byte completion
  // record, one AXI beat pushed into the output FIFO ahead of the tile's DMA
  // beats:
  //
  //   bytes  0..3   magic 0x5953_4A43 ("CJSY" on the wire)   bits  31:0
  //   bytes  4..7   job_id of the completed descriptor         bits  63:32
  //   byte   8      device_id                                  bits  71:64
  //   byte   9      status: 0 ok, 1 k refused, 2 device refused bits 79:72
  //   bytes 10..11  payload bytes that follow: 256, 64 or 0    bits  95:80
  //   bytes 12..15  seq, this bitstream's completion counter   bits 127:96
  //
  // Without it the host saw bare tiles and could attribute them only by
  // order, which with four contexts completing on their own clocks is no
  // order at all.  The readback descriptor goes to the read engine only once
  // the record is in the FIFO, so the bytes cannot overtake each other.
  // The job_id is the FIFO entry's: captured at the job's own job_done
  // pulse, the one cycle in which the context's job_id_reg names the job
  // being completed (the context takes its next queued job at once, so
  // job_id_reg read any later may name another job).  A refused descriptor
  // (acceptance section: k outside 1..K_MAX, or no such device) is reported
  // the same way, as a record with status 1 or 2 and no payload, ahead of
  // any tile.
  localparam integer RB_DEPTH  = 8;
  localparam integer RB_IDX_W  = 3;
  logic [AXI_ADDR_W-1:0] rb_q      [0:NUM_ACCEL-1][0:RB_DEPTH-1];   // c_base of the result
  logic [31:0]           rb_q_id   [0:NUM_ACCEL-1][0:RB_DEPTH-1];   // job_id of the result
  logic [RB_IDX_W-1:0]   rb_q_wr   [0:NUM_ACCEL-1];
  logic [RB_IDX_W-1:0]   rb_q_rd   [0:NUM_ACCEL-1];
  logic [RB_IDX_W:0]     rb_q_cnt  [0:NUM_ACCEL-1];
  logic [NUM_ACCEL-1:0]  rb_pending;
  logic                  rb_active;
  logic [ACC_ID_W-1:0]   rb_owner;
  logic [AXI_ADDR_W-1:0] rb_addr [0:NUM_ACCEL-1];   // head of each queue
  logic [31:0]           rb_id   [0:NUM_ACCEL-1];   // head of each queue

  // Jobs of a context accepted and not yet read back: every one of them will
  // need a FIFO entry, so acceptance stops at RB_DEPTH of them -- then no
  // completion can ever find the FIFO full, however slowly the host drains
  // the DPTI output path.
  logic [RB_IDX_W:0] rb_inflight [0:NUM_ACCEL-1];
  generate
    for (genvar i = 0; i < NUM_ACCEL; i++) begin : RB_BUSY
      assign rb_pending[i]           = (rb_q_cnt[i] != 0);
      assign rb_addr[i]              = rb_q[i][rb_q_rd[i]];
      assign rb_id[i]                = rb_q_id[i][rb_q_rd[i]];
      assign result_readback_busy[i] = (rb_inflight[i] == (RB_IDX_W+1)'(RB_DEPTH));
    end
  endgenerate

  function automatic logic [15:0] rb_beats_of(input logic [ACC_ID_W-1:0] ctx);
    rb_beats_of = (integer'(ctx) < NUM_8X8) ? 16'd16 : 16'd4;   // 8x8: 64 words; 4x4: 16 words
  endfunction

  logic                rb_any;
  logic [ACC_ID_W-1:0] rb_grant;
  // Round robin from the context after the last one served: with jobs
  // pipelined, a context that completes a tile every readback period
  // (the 8x8 fed a long chain) would otherwise hold a fixed-priority slot
  // for good and the other contexts' finished tiles would never reach the
  // host.
  logic [ACC_ID_W-1:0] rb_last;

  always_comb begin
    rb_any   = 1'b0;
    rb_grant = '0;
    for (int n = NUM_ACCEL; n >= 1; n--) begin
      automatic int k = (int'(rb_last) + n) % NUM_ACCEL;
      if (rb_pending[k]) begin
        rb_any   = 1'b1;
        rb_grant = ACC_ID_W'(k);
      end
    end
  end

  // ---- the completion record -----------------------------------------------
  localparam logic [31:0] COMPLETION_MAGIC = 32'h5953_4A43;
  typedef enum logic [1:0] { RB_IDLE, RB_HDR, RB_DATA } rb_state_t;
  rb_state_t             rb_state;
  logic                  rb_kind_data;                // RB_HDR: a tile follows (else a refusal)
  logic [127:0]          rb_hdr_beat;
  logic [31:0]           rb_seq;
  logic                  rb_desc_sent;
  logic [AXI_ADDR_W-1:0] rb_owner_addr;               // the granted entry's c_base

  wire rb_hdr_fire = (rb_state == RB_HDR) && rb_cdc_src_ready;
  assign ref_sent  = rb_hdr_fire && !rb_kind_data;

  // The grant, in RB_IDLE: a refusal record first (it holds the descriptor
  // path), else the round-robin context with a result in DDR.  The FIFO entry
  // is popped at the grant, its job_id into the record and its c_base into
  // the read descriptor that follows the record.
  wire rb_take = (rb_state == RB_IDLE) && !ref_pending && rb_any;

  assign rd_desc_valid[RD_SLOT_RB]      = (rb_state == RB_DATA) && !rb_desc_sent;
  assign rd_desc_addr[RD_SLOT_RB]       = rb_owner_addr;
  assign rd_desc_beats[RD_SLOT_RB]      = rb_beats_of(rb_owner);
  assign rd_desc_tag[RD_SLOT_RB]        = DMA_TAG_READBACK;
  assign rd_dst_almost_full[RD_SLOT_RB] = !rb_cdc_src_ready;

  wire rb_accept = rd_desc_valid[RD_SLOT_RB] && rd_desc_ready[RD_SLOT_RB];
  wire rb_done   = eng_done_valid && (eng_done_tag == DMA_TAG_READBACK);

  // ---- operand slots: context i's descriptor port is slot i -------------------
  // The owner of the beats now returning: the tag the engine attached.
  wire                 dst_tag_is_rb  = (dst_wr_tag == DMA_TAG_READBACK);
  wire [ACC_ID_W-1:0]  dst_tag_ctx    = dst_wr_tag[ACC_ID_W-1:0];

  generate
    for (genvar i = 0; i < NUM_ACCEL; i++) begin : ENG_ROUTE
      assign rd_desc_valid[i]      = ctx_op_desc_valid[i];
      assign rd_desc_addr[i]       = ctx_op_desc_addr[i];
      assign rd_desc_beats[i]      = ctx_op_desc_beats[i];
      assign rd_desc_tag[i]        = DMA_TAG_OPERAND_BASE + 8'(i);
      assign rd_dst_almost_full[i] = ctx_dst_almost_full[i];
      assign ctx_op_desc_ready[i]  = rd_desc_ready[i];
      assign ctx_dst_wr_en[i] =
          dst_wr_en && (dst_wr_tag == DMA_TAG_OPERAND_BASE + 8'(i));
      assign ctx_read_done[i] =
          eng_done_valid && (eng_done_tag == DMA_TAG_OPERAND_BASE + 8'(i));
    end
  endgenerate

  // The data channel stalls on the destination of the beat being returned.
  assign dst_full = dst_tag_is_rb ? !rb_cdc_src_ready : ctx_dst_full[dst_tag_ctx];

  wire rb_dst_wr_en = dst_wr_en && dst_tag_is_rb;

  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n) begin
      rb_active     <= 1'b0;
      rb_owner      <= '0;
      rb_last       <= ACC_ID_W'(NUM_ACCEL - 1);   // so context 0 is first after reset
      rb_state      <= RB_IDLE;
      rb_kind_data  <= 1'b0;
      rb_hdr_beat   <= '0;
      rb_seq        <= 32'd0;
      rb_desc_sent  <= 1'b0;
      rb_owner_addr <= '0;
      for (int k = 0; k < NUM_ACCEL; k++) begin
        rb_q_wr[k]     <= '0;
        rb_q_rd[k]     <= '0;
        rb_q_cnt[k]    <= '0;
        rb_inflight[k] <= '0;
        for (int d = 0; d < RB_DEPTH; d++) begin
          rb_q[k][d]    <= '0;
          rb_q_id[k][d] <= 32'd0;
        end
      end
    end
    else begin
      for (int k = 0; k < NUM_ACCEL; k++) begin
        // A job completes (push) and a readback of the same context is
        // granted (pop) possibly in one cycle; a job is accepted (in) and a
        // readback of the same context completes (out) possibly in one cycle.
        logic push, pop, in_, out_;
        push = USE_EXTERNAL_SCHEDULER && ctx_job_done[k];
        pop  = rb_take && (rb_grant == ACC_ID_W'(k));
        in_  = USE_EXTERNAL_SCHEDULER && ctx_job_fire[k];
        out_ = rb_done && rb_active && (rb_owner == ACC_ID_W'(k));
        if (push) begin
          rb_q[k][rb_q_wr[k]]    <= ctx_job_c_base[k][AXI_ADDR_W-1:0];
          rb_q_id[k][rb_q_wr[k]] <= ctx_job_id[k];
          rb_q_wr[k]             <= rb_q_wr[k] + 1'b1;
        end
        if (pop)
          rb_q_rd[k] <= rb_q_rd[k] + 1'b1;
        if (push && !pop)      rb_q_cnt[k] <= rb_q_cnt[k] + 1'b1;
        else if (!push && pop) rb_q_cnt[k] <= rb_q_cnt[k] - 1'b1;
        if (in_ && !out_)      rb_inflight[k] <= rb_inflight[k] + 1'b1;
        else if (!in_ && out_) rb_inflight[k] <= rb_inflight[k] - 1'b1;
      end

      case (rb_state)
        RB_IDLE: begin
          if (ref_pending) begin
            rb_kind_data <= 1'b0;
            rb_hdr_beat  <= {rb_seq, 16'd0, ref_status, ref_dev, ref_job_id, COMPLETION_MAGIC};
            rb_seq       <= rb_seq + 32'd1;
            rb_state     <= RB_HDR;
          end
          else if (rb_any) begin
            rb_kind_data  <= 1'b1;
            rb_hdr_beat   <= {rb_seq,
                              16'(rb_beats_of(rb_grant)) * 16'd16,
                              8'd0, 8'(rb_grant),
                              rb_id[rb_grant], COMPLETION_MAGIC};
            rb_seq        <= rb_seq + 32'd1;
            rb_active     <= 1'b1;
            rb_owner      <= rb_grant;
            rb_last       <= rb_grant;
            rb_owner_addr <= rb_addr[rb_grant];
            rb_desc_sent  <= 1'b0;
            rb_state      <= RB_HDR;
          end
        end
        // The record enters the output FIFO; only then may the tile's
        // beats be requested.
        RB_HDR: begin
          if (rb_cdc_src_ready)
            rb_state <= rb_kind_data ? RB_DATA : RB_IDLE;
        end
        RB_DATA: begin
          if (rb_accept) rb_desc_sent <= 1'b1;
          if (rb_done) begin
            rb_active <= 1'b0;
            rb_state  <= RB_IDLE;
          end
        end
        default: rb_state <= RB_IDLE;
      endcase
    end
  end

  assign rb_hdr_valid = (rb_state == RB_HDR);
  assign rb_hdr_data  = rb_hdr_beat;

`ifndef SYNTHESIS
  always_ff @(posedge ui_clk) begin
    for (int k = 0; k < NUM_ACCEL; k++)
      if (ui_rst_n && USE_EXTERNAL_SCHEDULER && ctx_job_done[k] &&
          rb_q_cnt[k] == (RB_IDX_W+1)'(RB_DEPTH) && !(rb_take && rb_grant == ACC_ID_W'(k)))
        $fatal(1, "context %0d completed a job with its readback queue full", k);
  end
`endif
  dma_engine_multi #(
    .AXI_DATA_W (AXI_DATA_W), .AXI_ADDR_W (AXI_ADDR_W), .AXI_ID_W (2),
    .BEAT_W (16), .BURST_LEN (16),
    .SHARED_BURST_LEN (USE_V2 ? 16 : 1),
    .MAX_OUTSTANDING (8),
    .NUM_SLOTS (RD_SLOTS),
    .STRICT_GATE (RD_SLOTS'(1) << RD_SLOT_RB)
  ) u_eng (
    .clk (ui_clk), .rst_n (ui_rst_n), .init_calib_complete (init_calib_complete),
    .desc_valid (rd_desc_valid), .desc_ready (rd_desc_ready),
    .desc_addr (rd_desc_addr), .desc_beats (rd_desc_beats), .desc_tag (rd_desc_tag),
    .done_valid (eng_done_valid), .done_tag (eng_done_tag),
    .m_axi_arid (arid), .m_axi_araddr (araddr), .m_axi_arlen (arlen),
    .m_axi_arsize (arsize), .m_axi_arburst (arburst), .m_axi_arlock (arlock),
    .m_axi_arcache (arcache), .m_axi_arprot (arprot), .m_axi_arqos (arqos),
    .m_axi_arvalid (arvalid), .m_axi_arready (arready),
    .m_axi_rid (2'b0), .m_axi_rdata (rdata_axi), .m_axi_rresp (rresp),
    .m_axi_rlast (rlast), .m_axi_rvalid (rvalid), .m_axi_rready (rready),
    .dst_almost_full (rd_dst_almost_full), .dst_full (dst_full),
    .dst_wr_en (dst_wr_en), .dst_wr_beat (dst_wr_beat),
    .dst_wr_data (dst_wr_data), .dst_wr_tag (dst_wr_tag),
    .slot_active (rd_slot_active), .shared (rd_shared),
    .busy_cycles (eng_busy_cycles), .rdy_stall_cycles (eng_rdy_stall_cycles),
    .r_stall_cycles (eng_r_stall_cycles),
    .err_align (eng_err_align), .err_resp (eng_err_resp), .stat_clear (run_clear)
  );

  always_ff @(posedge ui_clk) begin
    for (int k = 0; k < RD_SLOTS; k++) begin
      if (ui_rst_n && rd_desc_valid[k] && rd_desc_ready[k])
        $display("DMATRACE t=%0t grant=%s ctx=%0d addr=0x%08h beats=%0d tag=%02h",
                 $time, (k == RD_SLOT_RB) ? "readback" : "operand",
                 (k == RD_SLOT_RB) ? integer'(rb_owner) : k,
                 rd_desc_addr[k], rd_desc_beats[k], rd_desc_tag[k]);
    end
    if (ui_rst_n && rb_dst_wr_en)
      $display("RB_BEAT beat=%0d tag=%02h data=%032h", dst_wr_beat, dst_wr_tag, dst_wr_data);
    if (ui_rst_n && rb_done)
      $display("RB_DONE tag=%02h", eng_done_tag);
  end

  // -----------------------------------------------------------------------
  // External-job / result-readback hardware debug (sticky, read over JTAG).
  //
  //   bit 0 : external job accepted
  //   bit 1 : result readback armed after a job's final writeback
  //   bit 2 : readback descriptor accepted
  //   bit 3 : at least one readback data beat returned
  //   bit 4 : JOB0 readback data mismatch observed
  //   bit 5 : a write-back beat differed from the JOB0 signature
  //   bits 6..19 : host-command ingress checkpoints (unchanged)
  //   bit20..22  : context 0 observed in P_CALIB / P_READ / P_DONE
  //   bit23 : host staging writer observed busy
  //   bit24 : any context observed busy
  //   bit25 : any context observed active
  //   bit26 : core_job_ready observed high
  //   bit27 : context 0 reached P_GO
  //   bit28 : (unused since the per-context scheduler; was: replayed start)
  //   bit29 : any scheduler start pulse
  //   bit30 : any 4x4 adapter emitted fold_start
  //   bit31 : any 4x4 array published a result
  // -----------------------------------------------------------------------
  logic [31:0] external_debug_sticky;

  logic any_4x4_fold_start, any_4x4_c_valid;
  always_comb begin
    any_4x4_fold_start = 1'b0;
    any_4x4_c_valid    = 1'b0;
    for (int k = 0; k < NUM_4X4; k++) begin
      any_4x4_fold_start = any_4x4_fold_start | scheduler_fold_start_4x4[k];
      any_4x4_c_valid    = any_4x4_c_valid    | c_valid_out_4x4[k];
    end
  end

  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n) begin
      external_debug_sticky <= 32'd0;
    end
    else begin
      if (job_fire)                              external_debug_sticky[0]  <= 1'b1;
      if (USE_EXTERNAL_SCHEDULER && job_done)    external_debug_sticky[1]  <= 1'b1;
      if (rb_accept)                             external_debug_sticky[2]  <= 1'b1;
      if (rb_dst_wr_en)                          external_debug_sticky[3]  <= 1'b1;
      if (rb_dst_wr_en &&
          (dst_wr_data != 128'h4100000040800000400000003f800000))
                                                 external_debug_sticky[4]  <= 1'b1;
      if (wb_wvalid && wb_wready &&
          (wb_wdata != 128'h4100000040800000400000003f800000))
                                                 external_debug_sticky[5]  <= 1'b1;
      if (dpti_fifo_rd_valid && dpti_fifo_rd_ready)  external_debug_sticky[6]  <= 1'b1;
      if (dpti_axi_cmd_valid && dpti_axi_cmd_ready)  external_debug_sticky[7]  <= 1'b1;
      if (axi_dpti_wr_valid && axi_dpti_wr_ready)    external_debug_sticky[8]  <= 1'b1;
      if (axi_dpti_wr_valid && axi_dpti_wr_ready &&
          (axi_dpti_wr_addr == 8'h00) && axi_dpti_wr_data[0])
                                                 external_debug_sticky[9]  <= 1'b1;
      if (dpti_job_valid)                        external_debug_sticky[10] <= 1'b1;
      if (core_job_ready)                        external_debug_sticky[11] <= 1'b1;
      if (scheduler_desc_ready)                  external_debug_sticky[12] <= 1'b1;
      if (dpti_byte_valid && dpti_byte_ready)    external_debug_sticky[13] <= 1'b1;
      if (dpti_byte_valid && dpti_byte_ready && (dpti_byte_data == 8'h02))
                                                 external_debug_sticky[14] <= 1'b1;
      if (dpti_mem_start)                        external_debug_sticky[15] <= 1'b1;
      if (dpti_mem_valid && dpti_mem_ready)      external_debug_sticky[16] <= 1'b1;
      if (dpti_mem_done)                         external_debug_sticky[17] <= 1'b1;
      if (dpti_frontend_err_opcode || dpti_frontend_err_write32_opcode ||
          dpti_frontend_err_mem_opcode || dpti_frontend_err_mem_length)
                                                 external_debug_sticky[18] <= 1'b1;
      if (dpti_byte_valid && dpti_byte_ready && (dpti_byte_data == 8'h01))
                                                 external_debug_sticky[19] <= 1'b1;
      if (phase == 4'd0)                         external_debug_sticky[20] <= 1'b1;
      if (phase == 4'd2)                         external_debug_sticky[21] <= 1'b1;
      if (phase == 4'd7)                         external_debug_sticky[22] <= 1'b1;
      if (hs_busy)                               external_debug_sticky[23] <= 1'b1;
      if (job_busy)                              external_debug_sticky[24] <= 1'b1;
      if (job_active)                            external_debug_sticky[25] <= 1'b1;
      if (core_job_ready)                        external_debug_sticky[26] <= 1'b1;
      if (phase == 4'd3)                         external_debug_sticky[27] <= 1'b1;
      if (|scheduler_accelerator_start)          external_debug_sticky[29] <= 1'b1;
      if (any_4x4_fold_start)                    external_debug_sticky[30] <= 1'b1;
      if (any_4x4_c_valid)                       external_debug_sticky[31] <= 1'b1;
    end
  end

  // =========================================================================
  // Write-back engine: one AXI write master, one owner at a time.
  //
  // The next round-robin context with a committed tile is granted when the
  // engine is idle and nothing is in flight; it owns the engine -- the
  // descriptor, the result stream and the completion -- until its last write
  // response.  Another context's tile waits its turn here (bytes are moved
  // one tile at a time) while its array keeps computing.
  // =========================================================================
  logic                wb_inflight;
  logic [ACC_ID_W-1:0] wb_owner;
  logic [ACC_ID_W-1:0] wb_last;
  // No external per-job DDR failure status exists. A failed context is
  // quarantined until reset, without job_done/readback, while others proceed.
  logic [NUM_ACCEL-1:0] wb_fault;
  logic                wb_any_req;
  logic [ACC_ID_W-1:0] wb_grant_idx;

  always_comb begin
    wb_any_req   = 1'b0;
    wb_grant_idx = '0;
    for (int n = NUM_ACCEL; n >= 1; n--) begin
      automatic int k = (int'(wb_last) + n) % NUM_ACCEL;
      if (ctx_wb_desc_valid[k] && !wb_fault[k]) begin
        wb_any_req   = 1'b1;
        wb_grant_idx = ACC_ID_W'(k);
      end
    end
  end

  wire                  wb_eng_desc_ready;
  wire                  wb_eng_done;
  wire                  wb_eng_failed;
  wire                  wb_eng_desc_valid = wb_any_req && !wb_inflight;
  wire                  wb_eng_accept     = wb_eng_desc_valid && wb_eng_desc_ready;
  wire [AXI_ADDR_W-1:0] wb_tile_addr      = ctx_wb_desc_addr[wb_grant_idx];
  wire [15:0]           wb_tile_beats     = ctx_wb_desc_beats[wb_grant_idx];

  wire                  wb_src_valid;
  wire [AXI_DATA_W-1:0] wb_src_data;
  wire                  wb_src_ready;

  assign wb_src_valid = wb_inflight ? ctx_src_valid[wb_owner] : 1'b0;
  assign wb_src_data  = ctx_src_data[wb_owner];

  generate
    for (genvar i = 0; i < NUM_ACCEL; i++) begin : WB_ROUTE
      assign ctx_wb_desc_ready[i] =
          wb_eng_desc_ready && !wb_inflight && wb_any_req &&
          (wb_grant_idx == ACC_ID_W'(i));
      assign ctx_wb_done[i]  = wb_eng_done && wb_inflight && (wb_owner == ACC_ID_W'(i));
      assign ctx_src_ready[i] = wb_inflight && (wb_owner == ACC_ID_W'(i)) && wb_src_ready;
    end
  endgenerate

  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n) begin
      wb_inflight <= 1'b0;
      wb_owner    <= '0;
      wb_last     <= ACC_ID_W'(NUM_ACCEL - 1);
      wb_fault    <= '0;
    end
    else begin
      if (wb_eng_accept) begin
        wb_inflight <= 1'b1;
        wb_owner    <= wb_grant_idx;
        wb_last     <= wb_grant_idx;
      end
      else if (wb_eng_done || wb_eng_failed) begin
        wb_inflight <= 1'b0;
        if (wb_eng_failed) wb_fault[wb_owner] <= 1'b1;
      end
    end
  end

  dma_writeback_engine #(
    .AXI_DATA_W (AXI_DATA_W), .AXI_ADDR_W (AXI_ADDR_W), .AXI_ID_W (2),
    .BEAT_W (16), .BURST_LEN (16), .MAX_OUTSTANDING (4)
  ) u_wb (
    .clk (ui_clk), .rst_n (ui_rst_n), .init_calib_complete (init_calib_complete),
    .desc_valid (wb_eng_desc_valid), .desc_ready (wb_eng_desc_ready),
    .desc_addr (wb_tile_addr),
    .desc_beats (wb_tile_beats),
    .desc_tag (8'h40 + 8'(wb_grant_idx)),
    .done_valid (wb_eng_done), .done_tag (), .failed_valid (wb_eng_failed),
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

  // The bench's and the LEDs' view of the write-back descriptor handshake.
  wire wb_desc_valid = wb_eng_desc_valid;
  wire wb_desc_ready = wb_eng_desc_ready;
  wire wb_done       = wb_eng_done;

  // A refused descriptor (its k outside the named accelerator's 1..K_MAX, or
  // a device the fleet does not have) is a host error the board shows on
  // led[6] as well as reporting it in the completion record. It no longer
  // holds up the descriptor path.
  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n)
      err_job_k <= 1'b0;
    else if (refuse_fire)
      err_job_k <= 1'b1;
  end

  // ---- concurrency evidence (Oct 2026) ---------------------------------------
  // What the host cannot see from its clock: how often two or four arrays
  // were in their fold at the same time, and how long each context sat idle
  // with no job queued after its first one (the gap the host's submission
  // latency leaves once the context's job queue has run dry; with jobs
  // pipelined the next job is accepted while the previous one still runs,
  // so a done -> fire window would not measure it). Counted from reset, read
  // over JTAG.
  logic [31:0] overlap2_cycles, overlap4_cycles;
  logic [31:0] idle_gap_cycles [0:NUM_ACCEL-1];
  logic        gap_open        [0:NUM_ACCEL-1];
  logic [3:0]  folds_now;
  always_comb begin
    folds_now = 4'd0;
    for (int k = 0; k < NUM_ACCEL; k++)
      folds_now = folds_now + ((ctx_phase[k] == 4'd4) ? 4'd1 : 4'd0);   // P_FOLD
  end
  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n) begin
      overlap2_cycles <= 32'd0;
      overlap4_cycles <= 32'd0;
      for (int k = 0; k < NUM_ACCEL; k++) begin
        idle_gap_cycles[k] <= 32'd0;
        gap_open[k]        <= 1'b0;
      end
    end else begin
      if (folds_now >= 4'd2) overlap2_cycles <= overlap2_cycles + 32'd1;
      if (folds_now >= 4'd4) overlap4_cycles <= overlap4_cycles + 32'd1;
      for (int k = 0; k < NUM_ACCEL; k++) begin
        if (ctx_job_fire[k]) gap_open[k] <= 1'b1;      // the context has had a job
        if (gap_open[k] && !ctx_job_busy[k] && !ctx_job_fire[k])
          idle_gap_cycles[k] <= idle_gap_cycles[k] + 32'd1;
      end
    end
  end

  wire any_err  = seed_err_align | seed_err_resp
                | eng_err_align  | eng_err_resp | wr_err_range
                | wb_err_align   | wb_err_resp  | err_w_owner
                | err_job_k
                | dpti_link_fatal;   // a MEM_WRITE cut off by a lost DPTI link

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

  // DPTI status for probe_in18, one register so the probe keeps one net name
  // (dma_top_build.tcl 'read' looks it up by that name):
  //   [4:0]   dpti_phy_debug (clk_dpti, sticky)   [7:5]  0
  //   [8]     MMCM LOCKED (ui_clk)                [9]    link up
  //   [10]    link fatal (sticky)                 [13:11] dpti_link_ctrl state
  //   [15:14] 0                                   [23:16] MMCM resets issued
  //   [31:24] link drops
  reg [31:0] dpti_status_word;

  always @(posedge ui_clk)
    dpti_status_word <= { dpti_link_drop_count, dpti_mmcm_resets_ui, 2'd0,
                          dpti_link_state, dpti_link_fatal, dpti_link_up,
                          dpti_locked_ui, 3'd0, dpti_phy_debug_sync[4:0] };

  // ---- JTAG readout -------------------------------------------------------
  // (Oct 2026: 25 probes in -- the five below are the asynchronous host's
  // evidence, appended; dma_top_build.tcl's vio_0 configuration and
  // tb/xil_stubs.sv's stub carry the same count.)
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
    .probe_in18 (dpti_status_word),
    .probe_in19 (dpti_clk_activity_sync),
    // the asynchronous host's evidence (Oct 2026): overlap, idle gaps,
    // completions issued
    .probe_in20 (overlap2_cycles),
    .probe_in21 (overlap4_cycles),
    .probe_in22 (idle_gap_cycles[0]),
    .probe_in23 (idle_gap_cycles[(NUM_ACCEL > 1) ? 1 : 0]),
    .probe_in24 (rb_seq),
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
  // Each context checks its own geometry (K_DIM <= K_MAX, the slab's beats
  // within the descriptor's count, the accepted job's K within its K_MAX).
  // The fleet-level check is that a descriptor names a physical accelerator
  // at all: the acceptance logic never takes an out-of-range device, so the
  // producer would hang instead of being told.
  logic bad_device_reported = 1'b0;
  always_ff @(posedge ui_clk) begin
    if (ui_rst_n && USE_EXTERNAL_SCHEDULER && effective_job_valid && !target_valid &&
        !bad_device_reported) begin
      bad_device_reported <= 1'b1;
      $display("JOBREFUSED t=%0t descriptor names device %0d; the fleet has %0d accelerators (0..%0d); refused with status 2",
               $time, effective_job_device_id, NUM_ACCEL, NUM_ACCEL-1);
    end
  end
  // A k outside the named accelerator's capacity is refused by the acceptance
  // logic and latched in err_job_k (any_err); say so once, without stopping --
  // a bench may present it on purpose to check the refusal.
  logic err_job_k_reported = 1'b0;
  always_ff @(posedge ui_clk) begin
    if (ui_rst_n && err_job_k && !err_job_k_reported) begin
      err_job_k_reported <= 1'b1;
      $display("JOBREFUSED t=%0t device %0d k=%0d: outside 1..%0d (K_MAX_8X8=%0d, K_MAX_4X4=%0d); descriptor refused, err_job_k set",
               $time, effective_job_device_id, effective_job_k,
               target_is_8x8 ? K_MAX_8X8 : K_MAX_4X4, K_MAX_8X8, K_MAX_4X4);
    end
  end
  // Both write engines move whole 16-beat bursts.
  initial begin
    if ((N_BEATS % 16) != 0)
      $fatal(1, "N_BEATS %0d is not a multiple of the seeder's burst", N_BEATS);
    if ((HOST_SLAB_BEATS % 16) != 0)
      $fatal(1, "HOST_SLAB_BEATS %0d is not a multiple of the MEM_WRITE burst", HOST_SLAB_BEATS);
  end
`endif

endmodule

`default_nettype wire
