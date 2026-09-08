// -----------------------------------------------------------------------------
// systolic_dma_top.sv -- bring-up step 3b: the array fed from DRAM.
//
//   DDR3 -> MIG -> dma_engine -> dma_operand_writer -> operand buffers
//        -> tile feeder -> systolic array -> C -> checksum
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
// Read the numbers over JTAG with dma_top_build.tcl -tclargs read.
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

  parameter integer BASE_ADDR = 0
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
  output wire [0:0]  ddr3_odt
);

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

  // ---- AXI write channel: the seeder --------------------------------------
  wire [1:0]   awid;   wire [28:0] awaddr;  wire [7:0] awlen;
  wire [2:0]   awsize; wire [1:0]  awburst; wire [0:0] awlock;
  wire [3:0]   awcache; wire [2:0] awprot;  wire [3:0] awqos;
  wire         awvalid, awready;
  wire [127:0] wdata_axi; wire [15:0] wstrb; wire wlast, wvalid, wready;
  wire [1:0]   bid, bresp; wire bvalid, bready;

  // ---- AXI read channel: the engine ---------------------------------------
  wire [1:0]   arid;   wire [28:0] araddr;  wire [7:0] arlen;
  wire [2:0]   arsize; wire [1:0]  arburst; wire [0:0] arlock;
  wire [3:0]   arcache; wire [2:0] arprot;  wire [3:0] arqos;
  wire         arvalid, arready;
  wire [127:0] rdata_axi; wire [1:0] rresp; wire rlast, rvalid, rready;

  // =========================================================================
  // Bring-up sequencer
  //
  //   P_CALIB  wait for DDR3
  //   P_SEED   write the known image
  //   P_READ   pull it back through the DMA into the operand buffers
  //   P_GO     one pulse to start the fold
  //   P_FOLD   wait for the array
  //   P_SCAN   read C back, one entry per cycle
  // =========================================================================
  typedef enum logic [2:0] {
    P_CALIB, P_SEED, P_READ, P_GO, P_FOLD, P_SCAN, P_DONE
  } phase_t;
  phase_t phase;

  logic          seed_start;
  wire           seed_busy, seed_done, seed_err_align, seed_err_resp;

  logic          desc_valid;
  wire           desc_ready, read_done;
  wire           eng_err_align, eng_err_resp, wr_err_range;
  wire [31:0]    words_written;
  logic          desc_started;
  logic          seed_done_sticky, read_done_sticky, fold_done_sticky;

  logic          fold_start;          // the pulse that replaces matrices_ready
  logic          c_done;              // declared here, driven by the copied block

  // P_READ ends when the last word has LANDED, not when the last beat has been
  // received.  dma_operand_writer takes four cycles to unpack a 128-bit beat
  // into the single 32-bit buffer write port, so read_done leads the final
  // write by up to three cycles.  Starting the fold on read_done would race the
  // last three operands into the array -- intermittently, and only at the tail
  // of the payload, which is the worst kind of bug to hunt on a board.
  // words_written is exact.
  wire fill_complete = read_done_sticky && (words_written == 32'(RX_WORDS));

  localparam integer C_N = N * N;
  logic [2*LANE_W:0] scan_c;
  wire scan_c_last = (scan_c == (2*LANE_W+1)'(C_N));

  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n) begin
      phase      <= P_CALIB;
      seed_start <= 1'b0;
      desc_valid <= 1'b0;
      fold_start <= 1'b0;
      scan_c     <= '0;
    end else begin
      seed_start <= 1'b0;
      desc_valid <= 1'b0;
      fold_start <= 1'b0;
      case (phase)
        P_CALIB: if (init_calib_complete) begin
                   seed_start <= 1'b1;
                   phase      <= P_SEED;
                 end
        P_SEED:  if (seed_done) phase <= P_READ;
        P_READ:  begin
                   if (!desc_started) desc_valid <= 1'b1;
                   if (fill_complete) phase <= P_GO;
                 end
        P_GO:    begin
                   fold_start <= 1'b1;
                   phase      <= P_FOLD;
                 end
        P_FOLD:  if (c_done) begin
                   scan_c <= '0;
                   phase  <= P_SCAN;
                 end
        P_SCAN:  if (scan_c_last) phase <= P_DONE;
                 else             scan_c <= scan_c + 1'b1;
        default: ;
      endcase
    end
  end

  // desc_valid is a pulse; remember it was taken so P_READ does not re-issue.
  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n)                     desc_started <= 1'b0;
    else if (phase == P_SEED)          desc_started <= 1'b0;
    else if (desc_valid && desc_ready) desc_started <= 1'b1;
  end

  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n) begin
      seed_done_sticky <= 1'b0;
      read_done_sticky <= 1'b0;
      fold_done_sticky <= 1'b0;
    end else begin
      if (seed_done) seed_done_sticky <= 1'b1;
      if (read_done) read_done_sticky <= 1'b1;
      if (c_done)    fold_done_sticky <= 1'b1;
    end
  end

  // ---- seeder -------------------------------------------------------------
  dma_seed_writer #(
    .AXI_DATA_W (AXI_DATA_W), .AXI_ADDR_W (AXI_ADDR_W), .AXI_ID_W (2),
    .BURST_LEN (16), .TOTAL_BEATS (N_BEATS),
    .SEED_MODE (1), .MODULUS (127)
  ) u_seed (
    .clk (ui_clk), .rst_n (ui_rst_n),
    .start (seed_start), .base_addr (AXI_ADDR_W'(BASE_ADDR)),
    .busy (seed_busy), .done (seed_done),
    .err_align (seed_err_align), .err_resp (seed_err_resp),
    .m_axi_awid (awid), .m_axi_awaddr (awaddr), .m_axi_awlen (awlen),
    .m_axi_awsize (awsize), .m_axi_awburst (awburst), .m_axi_awlock (awlock),
    .m_axi_awcache (awcache), .m_axi_awprot (awprot), .m_axi_awqos (awqos),
    .m_axi_awvalid (awvalid), .m_axi_awready (awready),
    .m_axi_wdata (wdata_axi), .m_axi_wstrb (wstrb), .m_axi_wlast (wlast),
    .m_axi_wvalid (wvalid), .m_axi_wready (wready),
    .m_axi_bid (bid), .m_axi_bresp (bresp), .m_axi_bvalid (bvalid),
    .m_axi_bready (bready)
  );

  // ---- read engine --------------------------------------------------------
  wire          dst_wr_en;
  wire [15:0]   dst_wr_beat;
  wire [127:0]  dst_wr_data;
  wire          dst_full, dst_almost_full;

  dma_engine #(
    .AXI_DATA_W (AXI_DATA_W), .AXI_ADDR_W (AXI_ADDR_W), .AXI_ID_W (2),
    .BEAT_W (16), .BURST_LEN (16), .MAX_OUTSTANDING (8)
  ) u_eng (
    .clk (ui_clk), .rst_n (ui_rst_n), .init_calib_complete (init_calib_complete),
    .desc_valid (desc_valid), .desc_ready (desc_ready),
    .desc_addr (AXI_ADDR_W'(BASE_ADDR)), .desc_beats (16'(N_BEATS)),
    .desc_tag (8'h3B),
    .done_valid (read_done), .done_tag (),
    .m_axi_arid (arid), .m_axi_araddr (araddr), .m_axi_arlen (arlen),
    .m_axi_arsize (arsize), .m_axi_arburst (arburst), .m_axi_arlock (arlock),
    .m_axi_arcache (arcache), .m_axi_arprot (arprot), .m_axi_arqos (arqos),
    .m_axi_arvalid (arvalid), .m_axi_arready (arready),
    .m_axi_rid (2'b0), .m_axi_rdata (rdata_axi), .m_axi_rresp (rresp),
    .m_axi_rlast (rlast), .m_axi_rvalid (rvalid), .m_axi_rready (rready),
    .dst_almost_full (dst_almost_full), .dst_full (dst_full),
    .dst_wr_en (dst_wr_en), .dst_wr_beat (dst_wr_beat),
    .dst_wr_data (dst_wr_data), .dst_wr_tag (),
    .busy_cycles (), .rdy_stall_cycles (), .r_stall_cycles (),
    .err_align (eng_err_align), .err_resp (eng_err_resp), .stat_clear (1'b0)
  );

  // ---- operand writer -----------------------------------------------------
  wire              a_wr, b_wr;
  wire [LANE_W-1:0] wsel;
  wire [K_W-1:0]    waddr;
  wire [31:0]       wdata_buf;

  dma_operand_writer #(
    .N (N), .K_MAX (K_MAX), .AXI_DATA_W (AXI_DATA_W), .BEAT_W (16)
  ) u_wr (
    .clk (ui_clk), .rst_n (ui_rst_n),
    .dst_wr_en (dst_wr_en), .dst_wr_beat (dst_wr_beat), .dst_wr_data (dst_wr_data),
    .dst_full (dst_full), .dst_almost_full (dst_almost_full),
    .a_wr (a_wr), .b_wr (b_wr), .wsel (wsel), .waddr (waddr), .wdata (wdata_buf),
    .words_written (words_written), .err_range (wr_err_range), .clear (1'b0)
  );

  // ---- checksum 1: the write stream ---------------------------------------
  // seed_ref.py's formula, accumulated as the words go past rather than by
  // reading the buffers back.  The read ports belong to the feeder; taking them
  // for a scan would mean muxing the array's own datapath in order to observe
  // it.  This costs two adders and touches nothing.
  //
  // The order differs from seed_ref's -- payload order, not (k, bank) order --
  // and that is fine: 32-bit wrapping addition is commutative and associative,
  // so the total is identical.  It is the same constant 3a compared against.
  logic [31:0] chk_wr;
  wire [15:0] k16   = 16'(waddr);
  wire [7:0]  bank8 = 8'(wsel);
  wire [31:0] wpos  = {8'd0, bank8, k16};

  always_ff @(posedge ui_clk or negedge ui_rst_n) begin
    if (!ui_rst_n)              chk_wr <= '0;
    else if (phase == P_CALIB)  chk_wr <= '0;
    else if (a_wr)              chk_wr <= chk_wr + (wdata_buf ^ wpos);
    else if (b_wr)              chk_wr <= chk_wr + (wdata_buf ^ (32'h8000_0000 | wpos));
  end

  // =========================================================================
  // FROM HERE TO THE MIG INSTANCE, EVERYTHING IS systolic_uart_top's COMPUTE
  // CORE, COPIED.  Do not "clean it up".  Any edit here is an edit to the
  // thing 3b is trying to hold constant, and the 125-cycle check is the only
  // thing standing between a faithful copy and a subtly different one.
  // =========================================================================

  // ---- operand memories ---------------------------------------------------
  // One write port and one synchronous read port each, which is the shape block
  // RAM wants.  The read is SYNCHRONOUS: the address issued on beat t returns
  // data on t+1, and the feeder already delays valid to match.  That one cycle
  // is why the cost is k + 2(N-1) + H rather than one less.
  logic [K_W-1:0] a_raddr [0:N-1];
  logic [K_W-1:0] b_raddr [0:N-1];
  wire  [31:0]    a_rdata [0:N-1];
  wire  [31:0]    b_rdata [0:N-1];

  // A and B are the same hardware; the only difference is which field selects
  // the bank and which forms the address, and that swap is the A/B transpose.
  // Here both come from dma_operand_writer, which computes them with the same
  // bit slices systolic_uart_top's rx_count decode uses -- proved equivalent in
  // tb_dma_operand_writer against a golden model of that decode.
  systolic_operand_buffer #(
    .K_MAX   (K_MAX),
    .K_W     (K_W),
    .N_BANKS (N)
  ) u_a_buf (
    .clk   (ui_clk),
    .wr    (a_wr),
    .wsel  (wsel),
    .waddr (waddr),
    .wdata (wdata_buf),
    .raddr (a_raddr),
    .rdata (a_rdata)
  );

  systolic_operand_buffer #(
    .K_MAX   (K_MAX),
    .K_W     (K_W),
    .N_BANKS (N)
  ) u_b_buf (
    .clk   (ui_clk),
    .wr    (b_wr),
    .wsel  (wsel),
    .waddr (waddr),
    .wdata (wdata_buf),
    .raddr (b_raddr),
    .rdata (b_rdata)
  );

  // ---- array interface ----------------------------------------------------
  logic [31:0] a_in [0:N-1];
  logic [31:0] b_in [0:N-1];
  logic        a_valid_in [0:N-1];
  logic        b_valid_in [0:N-1];
  logic        c_valid_out;
  logic [31:0] c_out [0:N-1][0:N-1];

  // k_dim is a constant here rather than a register written by a frame header:
  // there is no host in this design.  It keeps the width systolic_uart_top gives
  // it so the feeder is parameterised identically.
  wire [FEED_W-1:0] k_dim = FEED_W'(K_DIM);

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

  systolic_array #(
    .N      (N),
    .DATA_W (32)
  ) u_array (
    .clk           (ui_clk),
    .rst           (rst_i),

    .a_in          (a_in),
    .b_in          (b_in),

    .a_valid_in    (a_valid_in),
    .b_valid_in    (b_valid_in),

    .c_valid_out   (c_valid_out),
    .c_out         (c_out)
  );

  // ---- store final results ------------------------------------------------
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

      // The array itself tells us when a reduced C matrix is ready.
      if (c_valid_out) begin
        for (rr = 0; rr < N; rr = rr + 1)
          for (cc = 0; cc < N; cc = cc + 1)
            C[rr][cc] <= c_out[rr][cc];

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
          // One fold per configuration.  Nothing re-arms it.
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
  logic [31:0] cyc_latched;
  logic        cyc_running;

  always_ff @(posedge ui_clk) begin
    if (rst_i) begin
      cyc_count   <= '0;
      cyc_latched <= '0;
      cyc_running <= 1'b0;
    end
    else begin
      if (state == ST_FEED && feed_t == '0 && !cyc_running) begin
        cyc_running <= 1'b1;
        cyc_count   <= 32'd1;
      end
      else if (cyc_running) begin
        cyc_count <= cyc_count + 1'b1;
        if (c_valid_out) begin
          cyc_running <= 1'b0;
          cyc_latched <= cyc_count + 1'b1;
        end
      end
    end
  end

  // =========================================================================
  // End of the copied core.
  // =========================================================================

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

  wire wr_match = read_done_sticky && (chk_wr == EXPECT_WR_CHK);
  wire c_match  = (phase == P_DONE)  && (chk_c  == EXPECT_C_CHK);
  wire any_err  = seed_err_align | seed_err_resp
                | eng_err_align  | eng_err_resp | wr_err_range;

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
  // FIVE probes, not 3a's four: four 32-bit values plus an 8-bit flag word.
  // dma_top_build.tcl configures vio_0 to match -- 3a's IP has probe_in3 at 8
  // bits, and reusing that configuration would silently truncate a 32-bit
  // probe, which still looks like a number.
  //
  // The expected constants are folded parameters with no net behind them, so
  // they are NOT probed; the script prints them from its own variables.  3a
  // shipped a bitstream that printed "expected 0x" for exactly this reason.
  vio_0 u_vio (
    .clk       (ui_clk),
    .probe_in0 (chk_wr),
    .probe_in1 (chk_c),
    .probe_in2 (cyc_latched),
    .probe_in3 (words_written),
    .probe_in4 ({ 1'b0, any_err, c_match, wr_match,
                  fold_done_sticky, read_done_sticky, seed_done_sticky,
                  init_calib_complete })
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
  end
`endif

endmodule

`default_nettype wire