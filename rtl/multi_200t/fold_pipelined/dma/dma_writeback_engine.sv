`default_nettype none
`timescale 1ns / 1ps

/*
 * dma_writeback_engine -- descriptor-driven AXI4 WRITE master.
 *
 * The mirror of dma_engine.sv.  Transport, descriptor front end, the 4 KiB
 * burst clamp and the incremental-counter issue loop are carried over
 * verbatim; what is genuinely different is the write channel's shape, and
 * that difference is the whole reason this is a separate module rather than
 * a parameter on the read engine:
 *
 *   - On a read, the SLAVE tells us where a burst ends (rlast).  On a write,
 *     WE must assert wlast on the last beat of every burst, so the W side has
 *     to know the length of the burst the AW channel announced.
 *   - AW and W are separate channels, so AW can legally run ahead of W.  It
 *     does not here: one AW, then that burst's W beats, then the next AW.
 *     Running AW ahead would need a queue of burst lengths for wlast, and the
 *     payload this engine exists to move is ONE 16-beat burst per fold (a
 *     N=8 result tile is 64 words = 2048 bits = 16 beats at 128 bit), so the
 *     queue would buy nothing and cost mutants.  Bursts still overlap through
 *     the B channel: MAX_OUTSTANDING counts write responses, not AW.
 *   - The B channel has to be checked for the same reason rresp does on the
 *     read side: a slave error otherwise looks exactly like a successful
 *     write, and the data is silently not there.
 *
 * ---- UNDERRUN ---------------------------------------------------------
 *
 * Once AW is accepted for a burst of L beats we are committed to delivering
 * L beats on W.  If the source runs dry mid-burst this module holds wvalid
 * low and the burst stalls -- legal AXI, but it parks the memory controller.
 * That cannot happen in the intended use: the result tile is complete in the
 * array's output registers before the descriptor is issued, so the whole
 * payload is already in the CDC FIFO.  src_starve_cycles counts it anyway,
 * because "the memory was slow" and "the array had not produced yet" are
 * different failures and the cost model needs them apart -- the same reason
 * dma_engine separates rdy_stall_cycles from r_stall_cycles.
 *
 * ---- wstrb ------------------------------------------------------------
 *
 * Descriptors are beat aligned and lengths are whole beats, so every beat is
 * a full-width write and wstrb is all ones.  Partial beats would need the
 * descriptor to carry a byte length; that is not needed for a result tile,
 * whose size is 4*N*N bytes and therefore a whole number of 128-bit beats
 * for every power-of-two N >= 4.
 */

module dma_writeback_engine #(
  parameter integer AXI_DATA_W  = 128,     // must match C0_S_AXI_DATA_WIDTH
  parameter integer AXI_ADDR_W  = 29,      // MIG AXI slave address width
  parameter integer AXI_ID_W    = 2,
  parameter integer BEAT_W      = 16,      // beats per descriptor, max 65535
  parameter integer BURST_LEN   = 16,      // beats per AXI burst, 1..256
  parameter integer MAX_OUTSTANDING = 4    // write responses allowed in flight
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
  // Pulses only after the LAST write response has returned, not when the last
  // beat leaves.  A descriptor is not done until the memory says it is.
  output logic                   done_valid,        // one-cycle pulse
  output logic [7:0]             done_tag,

  // ---- AXI4 write address channel ----------------------------------------
  output wire  [AXI_ID_W-1:0]    m_axi_awid,
  output logic [AXI_ADDR_W-1:0]  m_axi_awaddr,
  output logic [7:0]             m_axi_awlen,       // beats - 1
  output wire  [2:0]             m_axi_awsize,
  output wire  [1:0]             m_axi_awburst,
  output wire  [0:0]             m_axi_awlock,
  output wire  [3:0]             m_axi_awcache,
  output wire  [2:0]             m_axi_awprot,
  output wire  [3:0]             m_axi_awqos,
  output logic                   m_axi_awvalid,
  input  wire                    m_axi_awready,

  // ---- AXI4 write data channel -------------------------------------------
  output wire  [AXI_DATA_W-1:0]    m_axi_wdata,
  output wire  [AXI_DATA_W/8-1:0]  m_axi_wstrb,
  output logic                     m_axi_wlast,
  output logic                     m_axi_wvalid,
  input  wire                      m_axi_wready,

  // ---- AXI4 write response channel ---------------------------------------
  input  wire  [AXI_ID_W-1:0]    m_axi_bid,         // unused: awid is always 0
  input  wire  [1:0]             m_axi_bresp,
  input  wire                    m_axi_bvalid,
  output wire                    m_axi_bready,

  // ---- source stream (CDC FIFO read side, array -> ui_clk) ---------------
  input  wire                    src_valid,
  input  wire [AXI_DATA_W-1:0]   src_data,
  output wire                    src_ready,

  // ---- observability -----------------------------------------------------
  output logic [31:0]            busy_cycles,       // cycles not IDLE
  output logic [31:0]            aw_stall_cycles,   // awvalid && !awready
  output logic [31:0]            w_stall_cycles,    // wvalid  && !wready
  output logic [31:0]            src_starve_cycles, // in S_W with no source beat
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
                   S_AW    = 2'd1,
                   S_W     = 2'd2,
                   S_DRAIN = 2'd3;

  localparam integer CRED_W = $clog2(MAX_OUTSTANDING + 1);
  localparam integer LEN_W  = $clog2(BURST_LEN + 1);

  logic [1:0]            state;
  logic [7:0]            tag;
  logic [AXI_ADDR_W-1:0] addr_r;      // NEXT burst's address (advanced on aw_fire)
  logic [BEAT_W-1:0]     issue_left;  // beats not yet announced by an AW
  logic [LEN_W-1:0]      cur_len;     // beats in the burst awlen_r describes
  logic [LEN_W-1:0]      w_left;      // beats still to send on W in this burst
  logic [CRED_W-1:0]     credit;      // write responses still available
  logic [7:0]            awlen_r;

  assign m_axi_awid    = '0;             // one ID: responses stay in order
  assign m_axi_awsize  = SIZE_FULL;
  assign m_axi_awburst = BURST_INCR;
  assign m_axi_awlock  = 1'b0;
  assign m_axi_awcache = 4'b0011;        // normal non-cacheable bufferable
  assign m_axi_awprot  = 3'b000;
  assign m_axi_awqos   = 4'b0000;

  assign m_axi_wdata = src_data;
  assign m_axi_wstrb = {BYTES_PER_BEAT{1'b1}};

  // Always able to retire a response: nothing downstream can backpressure it,
  // and a stalled B channel would deadlock against our own credit counter.
  assign m_axi_bready = 1'b1;

  wire aw_fire = m_axi_awvalid & m_axi_awready;
  wire w_fire  = m_axi_wvalid  & m_axi_wready;
  wire b_fire  = m_axi_bvalid  & m_axi_bready;

  // A source beat is consumed exactly when it goes out on W.
  assign src_ready  = (state == S_W) && m_axi_wready;
  assign desc_ready = (state == S_IDLE) && init_calib_complete;

  // ---- next burst length -------------------------------------------------
  // Smallest of: BURST_LEN, beats left, beats left in this 4 KiB page.
  // Identical to dma_engine's, and for the same reason: a burst that crosses a
  // 4 KiB boundary is illegal AXI, and MIG's behaviour if you issue one is not
  // defined.  Called only when loading a descriptor and at a burst boundary,
  // so it is never on the awvalid path.
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


  // addr_r and issue_left are advanced on aw_fire, so by the time we are in
  // S_W they already describe the NEXT burst and this is its length.
  wire [31:0] len_next = next_len(addr_r, {{(32-BEAT_W){1'b0}}, issue_left});

  wire [AXI_ADDR_W-1:0] addr_next = addr_r
                                  + AXI_ADDR_W'({{(32-LEN_W){1'b0}}, cur_len} << LSB);
  wire [BEAT_W-1:0]     issue_after = issue_left - BEAT_W'(cur_len);

  // ---- channel drive -----------------------------------------------------
  always_comb begin
    m_axi_awvalid = (state == S_AW) && (credit != '0);
    m_axi_awaddr  = addr_r;
    m_axi_awlen   = awlen_r;

    m_axi_wvalid  = (state == S_W) && src_valid;
    m_axi_wlast   = (w_left == LEN_W'(1));
  end

  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      state      <= S_IDLE;
      tag        <= '0;
      addr_r     <= '0;
      issue_left <= '0;
      cur_len    <= LEN_W'(1);
      w_left     <= LEN_W'(1);
      credit     <= CRED_W'(MAX_OUTSTANDING);
      awlen_r    <= 8'd0;
      done_valid <= 1'b0;
      done_tag   <= '0;
      err_align  <= 1'b0;
      err_resp   <= 1'b0;
    end else begin
      done_valid <= 1'b0;

      // Credit: one per burst, spent on an accepted AW, returned by that
      // burst's write response.  Both in the same cycle cancel out -- the
      // shared-cycle case tb_dma_engine could only reach at BURST_LEN=2.
      if (aw_fire && !b_fire)      credit <= credit - 1'b1;
      else if (!aw_fire && b_fire) credit <= credit + 1'b1;

      if (b_fire && !((m_axi_bresp == RESP_OKAY) || (m_axi_bresp == RESP_EXOK)))
        err_resp <= 1'b1;

      case (state)
        S_IDLE: begin
          if (desc_valid && desc_ready && (desc_beats != 0)) begin
            // A misaligned descriptor makes every burst-length clamp wrong and
            // MIG's response undefined.  Refuse the work rather than writing
            // plausible-looking garbage into DRAM -- worse than a read, since
            // this one destroys whatever was there.
            if (desc_addr[LSB-1:0] != '0) begin
              err_align <= 1'b1;
            end else begin
              tag        <= desc_tag;
              addr_r     <= desc_addr;
              issue_left <= desc_beats;
              cur_len    <= LEN_W'(first_len);
              awlen_r    <= 8'(first_len - 32'd1);
              credit     <= CRED_W'(MAX_OUTSTANDING);
              state      <= S_AW;
            end
          end
        end

        S_AW: begin
          if (aw_fire) begin
            addr_r     <= addr_next;
            issue_left <= issue_after;
            w_left     <= cur_len;
            state      <= S_W;
          end
        end

        S_W: begin
          if (w_fire) begin
            if (w_left == LEN_W'(1)) begin
              // issue_left was already advanced on aw_fire, so it is the count
              // that remains AFTER this burst.
              if (issue_left == '0) begin
                state <= S_DRAIN;
              end else begin
                cur_len <= LEN_W'(len_next);
                awlen_r <= 8'(len_next - 32'd1);
                state   <= S_AW;
              end
            end else begin
              w_left <= w_left - 1'b1;
            end
          end
        end

        S_DRAIN: begin
          // Full credit again == every burst has been answered.
          if (credit == CRED_W'(MAX_OUTSTANDING)) begin
            done_valid <= 1'b1;
            done_tag   <= tag;
            state      <= S_IDLE;
          end
        end

        default: state <= S_IDLE;
      endcase
    end
  end

  // ---- statistics --------------------------------------------------------
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      busy_cycles       <= '0;
      aw_stall_cycles   <= '0;
      w_stall_cycles    <= '0;
      src_starve_cycles <= '0;
    end else if (stat_clear) begin
      busy_cycles       <= '0;
      aw_stall_cycles   <= '0;
      w_stall_cycles    <= '0;
      src_starve_cycles <= '0;
    end else begin
      if (state != S_IDLE)                 busy_cycles       <= busy_cycles + 1'b1;
      if (m_axi_awvalid && !m_axi_awready) aw_stall_cycles   <= aw_stall_cycles + 1'b1;
      if (m_axi_wvalid  && !m_axi_wready)  w_stall_cycles    <= w_stall_cycles + 1'b1;
      if ((state == S_W) && !src_valid)    src_starve_cycles <= src_starve_cycles + 1'b1;
    end
  end

endmodule

`default_nettype wire
