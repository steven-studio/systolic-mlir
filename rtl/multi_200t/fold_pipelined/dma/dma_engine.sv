// -----------------------------------------------------------------------------
// dma_engine.sv -- descriptor-driven read engine, AXI4 master.
//
// Replaces the native-UI version (kept as dma_engine_native.sv).  The MIG in
// this design is generated from nexys_video_mig_axi128.prj, which gives an AXI4
// slave with C0_S_AXI_DATA_WIDTH = 128 -- there is no app_en / app_rdy to talk
// to.  The descriptor front end, the credit accounting and the statistics are
// carried over unchanged; only the transport is different.
//
// The AR-issue and outstanding-tracking logic below is the same shape that ran
// on the board in ddr3_bw_probe_axi.sv (65536 beats in 72270 cycles, reproduced
// bit-identically across two builds), so this is a port, not a new design.
//
// SCOPE.  Read path only, as before.  Writeback stays out until step 4.
//
// UNIT.  One descriptor is still one invocation's operand set, so the DMA's
// unit of work equals the cost model's unit.
//
// ---- WHAT CHANGES WHEN THE TRANSPORT IS AXI4 --------------------------------
//
//  native UI                        AXI4
//  -----------------------------    ------------------------------------------
//  one command  = one beat          one AR = up to BURST_LEN beats
//  app_en held until app_rdy        arvalid held until arready (same rule)
//  app_rd_data_valid, no handshake  rvalid / rready -- the data channel CAN be
//                                   stalled, and rlast marks the burst end
//  in-order returns                 in-order per ID; arid is tied to 0, so all
//                                   returns are in order
//
// Three consequences, each of which is a silent-corruption bug if missed:
//
//  1. CREDIT IS NOW IN BURSTS, NOT BEATS.  One credit buys BURST_LEN beats, so
//     the destination must have room for MAX_OUTSTANDING * BURST_LEN beats, not
//     MAX_OUTSTANDING.  Either size AF_MARGIN accordingly, or wire dst_full and
//     let rready do the work (this module does both).
//
//  2. A BURST MUST NOT CROSS A 4 KiB BOUNDARY.  AXI4 forbids it and MIG will
//     not fix it for you.  With BURST_LEN = 16 x 16 B = 256 B, a burst crosses
//     4 KiB whenever the start address is not 256 B aligned -- which a ragged
//     tile easily is.  Each burst's length is therefore clamped to whichever is
//     smallest: BURST_LEN, the beats left in the descriptor, or the beats left
//     in the current 4 KiB page.
//
//  3. rresp MUST BE CHECKED.  A SLVERR return still asserts rvalid and still
//     carries data; without the check, a decode error reads as plausible
//     numbers.  err_resp latches it.
//
// ---- TIMING -----------------------------------------------------------------
// Same discipline as the native version: nothing wide sits on the path that
// decides arvalid.  It depends on three registered values compared against zero
// (bursts_left, credit, dst_almost_full).  The next burst's length and address
// are computed in the ar_fire branch -- register to register, a full cycle of
// slack -- not in the arvalid decision.  Re-run tools/ooc_timing.tcl after any
// change here; the native version missed 200 MHz by 65 ps for exactly this
// reason before the counters were made incremental.
//
// ---- DESTINATION -------------------------------------------------------------
// Unchanged: a write stream (beat index, data, tag) so it can drive a scratch
// BRAM in bring-up step 2 and the operand buffer writer in step 3 without
// touching this module.  dst_wr_beat counts beats within the descriptor, so it
// is independent of how the bursts were split.
// -----------------------------------------------------------------------------

`default_nettype none

module dma_engine #(
  parameter integer AXI_DATA_W  = 128,     // must match C0_S_AXI_DATA_WIDTH
  parameter integer AXI_ADDR_W  = 29,      // MIG AXI slave address width
  parameter integer AXI_ID_W    = 2,
  parameter integer BEAT_W      = 16,      // beats per descriptor, max 65535
  parameter integer BURST_LEN   = 16,      // beats per AXI burst, 1..256
  parameter integer MAX_OUTSTANDING = 8    // AXI bursts allowed in flight
) (
  input  wire                    clk,               // ui_clk
  input  wire                    rst_n,             // ~ui_clk_sync_rst
  input  wire                    init_calib_complete,

  // ---- descriptor in (from the UART control plane) -----------------------
  input  wire                    desc_valid,
  output wire                    desc_ready,
  input  wire [AXI_ADDR_W-1:0]   desc_addr,         // byte address, beat aligned
  input  wire [BEAT_W-1:0]       desc_beats,        // number of beats
  input  wire [7:0]              desc_tag,

  // ---- completion out ----------------------------------------------------
  output logic                   done_valid,        // one-cycle pulse
  output logic [7:0]             done_tag,

  // ---- AXI4 read address channel -----------------------------------------
  output wire  [AXI_ID_W-1:0]    m_axi_arid,
  output logic [AXI_ADDR_W-1:0]  m_axi_araddr,
  output logic [7:0]             m_axi_arlen,       // beats - 1
  output wire  [2:0]             m_axi_arsize,
  output wire  [1:0]             m_axi_arburst,
  output wire  [0:0]             m_axi_arlock,
  output wire  [3:0]             m_axi_arcache,
  output wire  [2:0]             m_axi_arprot,
  output wire  [3:0]             m_axi_arqos,
  output logic                   m_axi_arvalid,
  input  wire                    m_axi_arready,

  // ---- AXI4 read data channel --------------------------------------------
  input  wire  [AXI_ID_W-1:0]    m_axi_rid,         // unused: arid is always 0
  input  wire  [AXI_DATA_W-1:0]  m_axi_rdata,
  input  wire  [1:0]             m_axi_rresp,
  input  wire                    m_axi_rlast,
  input  wire                    m_axi_rvalid,
  output wire                    m_axi_rready,

  // ---- destination backpressure -----------------------------------------
  // dst_full stalls the data channel directly (rready), which is the safe
  // option and the one to use.  dst_almost_full additionally stops NEW bursts
  // being issued; if you leave dst_full tied low you must size the FIFO's
  // AF_MARGIN >= MAX_OUTSTANDING * BURST_LEN or beats are dropped silently.
  input  wire                    dst_almost_full,
  input  wire                    dst_full,

  // ---- destination write stream -----------------------------------------
  output logic                   dst_wr_en,
  output logic [BEAT_W-1:0]      dst_wr_beat,       // beat index in this descriptor
  output logic [AXI_DATA_W-1:0]  dst_wr_data,
  output logic [7:0]             dst_wr_tag,

  // ---- observability -----------------------------------------------------
  output logic [31:0]            busy_cycles,       // cycles not IDLE
  output logic [31:0]            rdy_stall_cycles,  // arvalid && !arready
  output logic [31:0]            r_stall_cycles,    // rvalid  && !rready
  output logic                   err_align,         // descriptor not beat aligned
  output logic                   err_resp,          // a burst returned != OKAY
  input  wire                    stat_clear
);

  localparam integer BYTES_PER_BEAT = AXI_DATA_W / 8;
  localparam integer LSB            = $clog2(BYTES_PER_BEAT);   // 4 at 128 bit
  localparam integer BEATS_PER_4K   = 4096 / BYTES_PER_BEAT;    // 256 at 128 bit

  localparam [2:0] SIZE_FULL  = 3'(LSB);
  localparam [1:0] BURST_INCR = 2'b01;
  localparam [1:0] RESP_OKAY  = 2'b00;
  localparam [1:0] RESP_EXOK  = 2'b01;

  localparam [1:0] S_IDLE  = 2'd0,
                   S_ISSUE = 2'd1,
                   S_DRAIN = 2'd2;

  localparam integer CRED_W = $clog2(MAX_OUTSTANDING + 1);
  localparam integer LEN_W  = $clog2(BURST_LEN + 1);

  logic [1:0]            state;
  logic [7:0]            tag;
  logic [AXI_ADDR_W-1:0] addr_r;        // next burst's address, incremental
  logic [BEAT_W-1:0]     issue_left;    // beats still to REQUEST
  logic [BEAT_W-1:0]     ret_left;      // beats still to RETURN
  logic [BEAT_W-1:0]     ret_idx;       // destination beat index
  logic [CRED_W-1:0]     credit;        // outstanding-burst credits
  logic [LEN_W-1:0]      cur_len;       // beats in the burst arlen_r describes
  logic [7:0]            arlen_r;

  assign m_axi_arid    = '0;             // one ID: returns stay in order
  assign m_axi_arsize  = SIZE_FULL;
  assign m_axi_arburst = BURST_INCR;
  assign m_axi_arlock  = 1'b0;
  assign m_axi_arcache = 4'b0011;        // normal non-cacheable bufferable
  assign m_axi_arprot  = 3'b000;
  assign m_axi_arqos   = 4'b0000;

  // Stall the data channel rather than dropping beats.  This backpressures MIG,
  // which is correct for a DMA (it is NOT correct for the bandwidth probe, and
  // that is why ddr3_bw_probe_axi ties rready high instead).
  assign m_axi_rready  = ~dst_full;

  wire ar_fire = m_axi_arvalid & m_axi_arready;
  wire r_fire  = m_axi_rvalid  & m_axi_rready;

  assign desc_ready = (state == S_IDLE) && init_calib_complete;

  // ---- next burst length -------------------------------------------------
  // Smallest of: BURST_LEN, beats left, beats left in this 4 KiB page.
  // Called only when loading a descriptor and in the ar_fire branch, so it is
  // never on the arvalid path.
  function automatic logic [31:0] next_len(input logic [AXI_ADDR_W-1:0] a,
                                           input logic [31:0]           remaining);
    logic [31:0] to_page;
    logic [31:0] lim;
    begin
      to_page = BEATS_PER_4K - {{(32-(12-LSB)){1'b0}}, a[11:LSB]};
      lim     = BURST_LEN;
      if (remaining < lim) lim = remaining;
      if (to_page   < lim) lim = to_page;
      next_len = lim;
    end
  endfunction

  wire [31:0] first_len = next_len(desc_addr, {{(32-BEAT_W){1'b0}}, desc_beats});

  wire [AXI_ADDR_W-1:0] addr_next = addr_r
                                  + AXI_ADDR_W'({{(32-LEN_W){1'b0}}, cur_len} << LSB);
  wire [BEAT_W-1:0]     issue_after = issue_left - BEAT_W'(cur_len);
  wire [31:0]           len_next  = next_len(addr_next,
                                             {{(32-BEAT_W){1'b0}}, issue_after});

  // ---- issue side --------------------------------------------------------
  // Three registered, zero-compared terms.  Nothing wide here, by design.
  wire may_issue = !dst_almost_full && (credit != '0);

  always_comb begin
    m_axi_arvalid = (state == S_ISSUE) && may_issue;
    m_axi_araddr  = addr_r;
    m_axi_arlen   = arlen_r;
  end

  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      state      <= S_IDLE;
      tag        <= '0;
      addr_r     <= '0;
      issue_left <= '0;
      ret_left   <= '0;
      ret_idx    <= '0;
      credit     <= CRED_W'(MAX_OUTSTANDING);
      cur_len    <= LEN_W'(1);
      arlen_r    <= 8'd0;
      done_valid <= 1'b0;
      done_tag   <= '0;
      err_align  <= 1'b0;
      err_resp   <= 1'b0;
    end else begin
      done_valid <= 1'b0;

      // Credit: one per burst, spent on an accepted AR, returned on that
      // burst's last beat.  Both in the same cycle cancel out.
      if (ar_fire && !(r_fire && m_axi_rlast))      credit <= credit - 1'b1;
      else if (!ar_fire && (r_fire && m_axi_rlast)) credit <= credit + 1'b1;

      // Return collection runs in ISSUE and DRAIN alike: beats come back while
      // later bursts are still being issued.
      if (r_fire && (state != S_IDLE)) begin
        ret_idx  <= ret_idx  + 1'b1;
        ret_left <= ret_left - 1'b1;
        if (!((m_axi_rresp == RESP_OKAY) || (m_axi_rresp == RESP_EXOK)))
          err_resp <= 1'b1;
      end

      case (state)
        S_IDLE: begin
          if (desc_valid && desc_ready && (desc_beats != 0)) begin
            // A misaligned descriptor would make every burst-length clamp wrong
            // and MIG's response undefined.  Flag it and refuse the work rather
            // than moving plausible-looking garbage.
            if (desc_addr[LSB-1:0] != '0) begin
              err_align <= 1'b1;
            end else begin
              tag        <= desc_tag;
              addr_r     <= desc_addr;
              issue_left <= desc_beats;
              ret_left   <= desc_beats;
              ret_idx    <= '0;
              credit     <= CRED_W'(MAX_OUTSTANDING);
              cur_len    <= LEN_W'(first_len);
              arlen_r    <= 8'(first_len - 32'd1);
              state      <= S_ISSUE;
            end
          end
        end

        S_ISSUE: begin
          if (ar_fire) begin
            addr_r     <= addr_next;
            issue_left <= issue_after;
            if (issue_after == '0) begin
              state <= S_DRAIN;
            end else begin
              cur_len <= LEN_W'(len_next);
              arlen_r <= 8'(len_next - 32'd1);
            end
          end
        end

        S_DRAIN: begin
          if (r_fire && (ret_left == 1)) begin
            done_valid <= 1'b1;
            done_tag   <= tag;
            state      <= S_IDLE;
          end
        end

        default: state <= S_IDLE;
      endcase
    end
  end

  // ---- destination write stream -----------------------------------------
  always_comb begin
    dst_wr_en   = r_fire && (state != S_IDLE);
    dst_wr_beat = ret_idx;
    dst_wr_data = m_axi_rdata;
    dst_wr_tag  = tag;
  end

  // ---- statistics --------------------------------------------------------
  // rdy_stall_cycles keeps its name: arvalid && !arready is the direct
  // analogue of the native app_en && !app_rdy, so existing top-level wiring
  // and the VIO probe map stay valid.  r_stall_cycles is new and worth having:
  // it separates "the memory was slow" from "we could not take the data",
  // which is exactly the distinction the cost model needs.
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      busy_cycles      <= '0;
      rdy_stall_cycles <= '0;
      r_stall_cycles   <= '0;
    end else if (stat_clear) begin
      busy_cycles      <= '0;
      rdy_stall_cycles <= '0;
      r_stall_cycles   <= '0;
    end else begin
      if (state != S_IDLE)                busy_cycles      <= busy_cycles + 1'b1;
      if (m_axi_arvalid && !m_axi_arready) rdy_stall_cycles <= rdy_stall_cycles + 1'b1;
      if (m_axi_rvalid  && !m_axi_rready)  r_stall_cycles   <= r_stall_cycles + 1'b1;
    end
  end

endmodule

`default_nettype wire
