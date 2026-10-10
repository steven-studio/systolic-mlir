// -----------------------------------------------------------------------------
// dma_engine_multi.sv -- descriptor-driven AXI4 read engine with one descriptor
// slot per requester, so several operand streams share one AXI read port.
//
// dma_engine.sv moves one descriptor at a time.  For a fleet of accelerators
// that is the serialization point of the whole machine: while context A's
// payload is landing, context B's operands cannot even be requested, so B's
// array sits idle until A's fill has ended.  With the v1 operand writer that
// is 4 cycles per beat -- a k=16 payload holds the port for ~257 cycles,
// twice the fold it feeds -- and two jobs submitted together compute one
// after the other no matter how many arrays there are.
//
// This engine keeps one descriptor per slot in flight at the same time and
// shares the single AXI read channel between them.
//
// Kept from dma_engine, cycle for cycle while only one slot is active:
//   - burst splitting (BURST_LEN, beats left, the 4 KiB page), credits counted
//     in bursts, in-order returns on a single ID, rready = ~dst_full, the
//     destination write stream (beat index within the descriptor + tag), the
//     rresp check, the alignment refusal, the statistics.
//
// New:
//   - NUM_SLOTS descriptor ports.  Each slot has its own address and beat
//     counters and its own tag; accepting, issuing and completing one slot
//     never touches another.
//   - A return queue of one entry per issued burst.  With a single ID the data
//     comes back in AR order, so the queue head names the owner of every
//     returning beat: dst_wr_tag and dst_wr_beat follow the data instead of a
//     global "current descriptor".
//   - The AR arbiter: round-robin over the slots that still have beats to
//     request.  With one active slot it issues exactly dma_engine's sequence.
//   - Burst length under sharing.  The v1 writer has no queue in front of it
//     and unpacks a beat in four cycles, so a 16-beat burst for one writer
//     occupies the channel for ~64 cycles while every other writer idles.
//     While two or more slots are active, a burst is SHARED_BURST_LEN beats
//     (one, for v1), so consecutive beats on the channel go to different
//     destinations and each writer runs at its own full rate: two v1 writers
//     take two words per cycle from the port instead of one.  A one-beat
//     burst whose writer is still busy stalls the channel for at most the
//     writer's three remaining cycles -- the same stall the single stream
//     always had -- and rready alone backpressures it.
//   - STRICT_GATE[s]: slot s drains into a queue that may stay full for a
//     long time (the result readback FIFO, emptied at the host's byte rate).
//     Such a slot gets a new burst only while its destination can take one
//     and it has nothing outstanding, in both modes, so a slow host never
//     holds the channel against the other slots' operands.
// -----------------------------------------------------------------------------

`default_nettype none

module dma_engine_multi #(
  parameter integer AXI_DATA_W       = 128,    // must match C0_S_AXI_DATA_WIDTH
  parameter integer AXI_ADDR_W       = 29,     // MIG AXI slave address width
  parameter integer AXI_ID_W         = 2,
  parameter integer BEAT_W           = 16,     // beats per descriptor, max 65535
  parameter integer BURST_LEN        = 16,     // beats per burst, one active slot
  parameter integer SHARED_BURST_LEN = 1,      // beats per burst, two or more active slots
  parameter integer MAX_OUTSTANDING  = 8,      // bursts in flight, all slots together (power of two)
  parameter integer NUM_SLOTS        = 2,
  parameter logic [NUM_SLOTS-1:0] STRICT_GATE = '0
) (
  input  wire                    clk,               // ui_clk
  input  wire                    rst_n,             // ~ui_clk_sync_rst
  input  wire                    init_calib_complete,

  // ---- descriptors, one port per slot --------------------------------------
  input  wire [NUM_SLOTS-1:0]    desc_valid,
  output wire [NUM_SLOTS-1:0]    desc_ready,
  input  wire [AXI_ADDR_W-1:0]   desc_addr  [0:NUM_SLOTS-1],  // byte address, beat aligned
  input  wire [BEAT_W-1:0]       desc_beats [0:NUM_SLOTS-1],  // number of beats
  input  wire [7:0]              desc_tag   [0:NUM_SLOTS-1],

  // ---- completion: one slot at most per cycle (one R beat per cycle) -------
  output logic                   done_valid,        // one-cycle pulse
  output logic [7:0]             done_tag,

  // ---- AXI4 read address channel -------------------------------------------
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

  // ---- AXI4 read data channel ----------------------------------------------
  input  wire  [AXI_ID_W-1:0]    m_axi_rid,         // unused: arid is always 0
  input  wire  [AXI_DATA_W-1:0]  m_axi_rdata,
  input  wire  [1:0]             m_axi_rresp,
  input  wire                    m_axi_rlast,
  input  wire                    m_axi_rvalid,
  output wire                    m_axi_rready,

  // ---- destination backpressure -------------------------------------------
  // dst_full belongs to the destination of the beat now at the head of the
  // return queue -- the one dst_wr_tag names -- and stalls rready directly.
  // dst_almost_full[s] stops NEW bursts to slot s: always for a STRICT_GATE
  // slot, and for the others while it is the only active slot (dma_engine's
  // rule); under sharing their bursts are SHARED_BURST_LEN beats and rready
  // alone backpressures them.
  input  wire  [NUM_SLOTS-1:0]   dst_almost_full,
  input  wire                    dst_full,

  // ---- destination write stream -------------------------------------------
  output logic                   dst_wr_en,
  output logic [BEAT_W-1:0]      dst_wr_beat,       // beat index within the owner's descriptor
  output logic [AXI_DATA_W-1:0]  dst_wr_data,
  output logic [7:0]             dst_wr_tag,        // the owner's tag

  // ---- observability -------------------------------------------------------
  output wire  [NUM_SLOTS-1:0]   slot_active,       // descriptor in flight, per slot
  output wire                    shared,            // two or more slots active
  output logic [31:0]            busy_cycles,       // cycles with any slot active
  output logic [31:0]            rdy_stall_cycles,  // arvalid && !arready
  output logic [31:0]            r_stall_cycles,    // rvalid  && !rready
  output logic                   err_align,         // descriptor not beat aligned
  output logic                   err_resp,          // a burst returned != OKAY
  input  wire                    stat_clear
);

  localparam integer BYTES_PER_BEAT = AXI_DATA_W / 8;
  localparam integer LSB            = $clog2(BYTES_PER_BEAT);   // 4 at 128 bit
  localparam integer BEATS_PER_4K   = 4096 / BYTES_PER_BEAT;    // 256 at 128 bit
  localparam integer SLOT_W         = (NUM_SLOTS > 1) ? $clog2(NUM_SLOTS) : 1;
  localparam integer Q_W            = (MAX_OUTSTANDING > 1) ? $clog2(MAX_OUTSTANDING) : 1;
  localparam integer CNT_W          = $clog2(MAX_OUTSTANDING + 1);

  localparam [2:0] SIZE_FULL  = 3'(LSB);
  localparam [1:0] BURST_INCR = 2'b01;
  localparam [1:0] RESP_OKAY  = 2'b00;
  localparam [1:0] RESP_EXOK  = 2'b01;

  // ---- per-slot state ------------------------------------------------------
  logic [NUM_SLOTS-1:0]  s_active;
  logic [7:0]            s_tag        [0:NUM_SLOTS-1];
  logic [AXI_ADDR_W-1:0] s_addr       [0:NUM_SLOTS-1];   // next burst's address
  logic [BEAT_W-1:0]     s_issue_left [0:NUM_SLOTS-1];   // beats still to REQUEST
  logic [BEAT_W-1:0]     s_ret_left   [0:NUM_SLOTS-1];   // beats still to RETURN
  logic [BEAT_W-1:0]     s_ret_idx    [0:NUM_SLOTS-1];   // destination beat index

  // ---- return queue: the owner of each burst in flight, in AR order -------
  logic [SLOT_W-1:0]     q_slot [0:MAX_OUTSTANDING-1];
  logic [Q_W-1:0]        q_head, q_tail;
  logic [CNT_W-1:0]      q_count;                        // bursts in flight

  wire               q_empty   = (q_count == '0);
  wire               q_full    = (q_count == CNT_W'(MAX_OUTSTANDING));
  wire [SLOT_W-1:0]  head_slot = q_slot[q_head];

  assign slot_active = s_active;

  // ---- sharing ---------------------------------------------------------------
  logic [CNT_W:0] n_active;
  always_comb begin
    n_active = '0;
    for (int k = 0; k < NUM_SLOTS; k++) n_active = n_active + (CNT_W+1)'(s_active[k]);
  end
  assign shared = (n_active > 1);

  // ---- AXI constants ----------------------------------------------------------
  assign m_axi_arid    = '0;             // one ID: returns stay in order
  assign m_axi_arsize  = SIZE_FULL;
  assign m_axi_arburst = BURST_INCR;
  assign m_axi_arlock  = 1'b0;
  assign m_axi_arcache = 4'b0011;        // normal non-cacheable bufferable
  assign m_axi_arprot  = 3'b000;
  assign m_axi_arqos   = 4'b0000;

  // Stall the data channel rather than dropping beats (dma_engine's rule).
  assign m_axi_rready  = ~dst_full;

  wire ar_fire = m_axi_arvalid & m_axi_arready;
  wire r_fire  = m_axi_rvalid  & m_axi_rready & ~q_empty;

  // A slot takes a descriptor while it has none in flight.
  generate
    for (genvar s = 0; s < NUM_SLOTS; s++) begin : RDY
      assign desc_ready[s] = !s_active[s] && init_calib_complete;
    end
  endgenerate

  // ---- next burst length -------------------------------------------------
  // Smallest of: the burst limit, beats left, beats left in this 4 KiB page.
  function automatic logic [31:0] next_len(input logic [AXI_ADDR_W-1:0] a,
                                           input logic [31:0]           remaining,
                                           input logic [31:0]           limit);
    logic [31:0] to_page;
    logic [31:0] lim;
    begin
      to_page = BEATS_PER_4K - {{(32-(12-LSB)){1'b0}}, a[11:LSB]};
      lim     = limit;
      if (remaining < lim) lim = remaining;
      if (to_page   < lim) lim = to_page;
      next_len = lim;
    end
  endfunction

  wire [31:0] burst_lim = shared ? 32'(SHARED_BURST_LEN) : 32'(BURST_LEN);

  // ---- issue side: who may request, and whose turn it is ---------------------
  logic [NUM_SLOTS-1:0] eligible;
  always_comb begin
    for (int k = 0; k < NUM_SLOTS; k++) begin
      eligible[k] = s_active[k] && (s_issue_left[k] != '0);
      if (STRICT_GATE[k]) begin
        // a queue destination: one burst at a time, and only when it has room
        if (dst_almost_full[k] || (s_ret_left[k] != s_issue_left[k])) eligible[k] = 1'b0;
      end else if (!shared) begin
        // alone on the channel: dma_engine's rule
        if (dst_almost_full[k]) eligible[k] = 1'b0;
      end
    end
  end

  logic [SLOT_W-1:0] rr_ptr;
  logic              grant_valid;
  logic [SLOT_W-1:0] grant;
  always_comb begin
    grant_valid = 1'b0;
    grant       = '0;
    for (int i = 0; i < NUM_SLOTS; i++) begin
      automatic int idx = (int'(rr_ptr) + i) % NUM_SLOTS;
      if (!grant_valid && eligible[idx]) begin
        grant_valid = 1'b1;
        grant       = SLOT_W'(idx);
      end
    end
  end

  wire [31:0] g_len = next_len(s_addr[grant],
                               {{(32-BEAT_W){1'b0}}, s_issue_left[grant]},
                               burst_lim);

  always_comb begin
    m_axi_arvalid = grant_valid && !q_full;
    m_axi_araddr  = s_addr[grant];
    m_axi_arlen   = 8'(g_len - 32'd1);
  end

  // ---- state ------------------------------------------------------------------
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      s_active   <= '0;
      for (int k = 0; k < NUM_SLOTS; k++) begin
        s_tag[k]        <= '0;
        s_addr[k]       <= '0;
        s_issue_left[k] <= '0;
        s_ret_left[k]   <= '0;
        s_ret_idx[k]    <= '0;
      end
      for (int k = 0; k < MAX_OUTSTANDING; k++) q_slot[k] <= '0;
      q_head     <= '0;
      q_tail     <= '0;
      q_count    <= '0;
      rr_ptr     <= '0;
      done_valid <= 1'b0;
      done_tag   <= '0;
      err_align  <= 1'b0;
      err_resp   <= 1'b0;
    end else begin
      done_valid <= 1'b0;

      // Descriptors: any number of idle slots may load in the same cycle.
      // A misaligned descriptor would make every burst-length clamp wrong and
      // MIG's response undefined: flag it and refuse the work.
      for (int k = 0; k < NUM_SLOTS; k++) begin
        if (desc_valid[k] && desc_ready[k] && (desc_beats[k] != '0)) begin
          if (desc_addr[k][LSB-1:0] != '0) begin
            err_align <= 1'b1;
          end else begin
            s_active[k]     <= 1'b1;
            s_tag[k]        <= desc_tag[k];
            s_addr[k]       <= desc_addr[k];
            s_issue_left[k] <= desc_beats[k];
            s_ret_left[k]   <= desc_beats[k];
            s_ret_idx[k]    <= '0;
          end
        end
      end

      // One burst requested: advance its slot, remember its owner.
      if (ar_fire) begin
        s_addr[grant]       <= s_addr[grant] + AXI_ADDR_W'(g_len << LSB);
        s_issue_left[grant] <= s_issue_left[grant] - BEAT_W'(g_len);
        q_slot[q_tail]      <= grant;
        q_tail              <= q_tail + 1'b1;
        rr_ptr              <= (int'(grant) == NUM_SLOTS - 1) ? '0 : grant + 1'b1;
      end

      // One beat returned: it belongs to the burst at the queue head.
      if (r_fire) begin
        s_ret_idx[head_slot]  <= s_ret_idx[head_slot]  + 1'b1;
        s_ret_left[head_slot] <= s_ret_left[head_slot] - 1'b1;
        if (!((m_axi_rresp == RESP_OKAY) || (m_axi_rresp == RESP_EXOK)))
          err_resp <= 1'b1;
        if (m_axi_rlast) q_head <= q_head + 1'b1;
        if (s_ret_left[head_slot] == BEAT_W'(1)) begin
          done_valid           <= 1'b1;
          done_tag             <= s_tag[head_slot];
          s_active[head_slot]  <= 1'b0;
        end
      end

      // Bursts in flight: one per accepted AR, one back per rlast.
      if (ar_fire && !(r_fire && m_axi_rlast))      q_count <= q_count + 1'b1;
      else if (!ar_fire && (r_fire && m_axi_rlast)) q_count <= q_count - 1'b1;
    end
  end

  // ---- destination write stream ---------------------------------------------
  always_comb begin
    dst_wr_en   = r_fire;
    dst_wr_beat = s_ret_idx[head_slot];
    dst_wr_data = m_axi_rdata;
    dst_wr_tag  = s_tag[head_slot];
  end

  // ---- statistics (dma_engine's definitions) ----------------------------------
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
      if (|s_active)                       busy_cycles      <= busy_cycles + 1'b1;
      if (m_axi_arvalid && !m_axi_arready) rdy_stall_cycles <= rdy_stall_cycles + 1'b1;
      if (m_axi_rvalid  && !m_axi_rready)  r_stall_cycles   <= r_stall_cycles + 1'b1;
    end
  end

  // ---- elaboration checks ------------------------------------------------------
  initial begin
    if ((MAX_OUTSTANDING & (MAX_OUTSTANDING - 1)) != 0) begin
      $display("dma_engine_multi: MAX_OUTSTANDING=%0d must be a power of two", MAX_OUTSTANDING);
      $fatal(1);
    end
    if (SHARED_BURST_LEN < 1 || SHARED_BURST_LEN > BURST_LEN) begin
      $display("dma_engine_multi: SHARED_BURST_LEN=%0d must be in 1..BURST_LEN=%0d",
               SHARED_BURST_LEN, BURST_LEN);
      $fatal(1);
    end
  end

endmodule

`default_nettype wire
