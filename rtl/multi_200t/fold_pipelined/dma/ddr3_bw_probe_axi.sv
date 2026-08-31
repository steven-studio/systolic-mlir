// -----------------------------------------------------------------------------
// ddr3_bw_probe_axi.sv -- DDR3 read-bandwidth probe, AXI4 front end.
//
// Same instrument as ddr3_bw_probe.sv; the counting, the measurement window and
// the UART report are unchanged (they were mutation-tested against seven
// mutants).  Only the request/response layer is different, because Digilent's
// mig.prj configures MIG with an AXI4 slave interface rather than the native
// user interface.
//
//   native UI                        AXI4
//   ---------------------------      -------------------------------------
//   app_en / app_rdy, one command     arvalid / arready, one BURST
//   per returned beat                 of BURST_LEN beats
//   app_rd_data_valid                 rvalid / rready, rlast on burst end
//
// Bursts are the reason AXI4 is easier here, not harder: one accepted AR gives
// BURST_LEN beats, so the address channel is nowhere near the bottleneck and
// the read data channel can run flat out.
//
// ARITHMETIC -- unchanged in form, watch the width
//   s_axi_rdata is 128 bits (C0_S_AXI_DATA_WIDTH = 128, matched 1:1 to the
//   memory controller) = 16 bytes = 4 fp32 words per beat.
//
//       words_per_ui_clk = 4 * BEATS / CYC
//       beta_ceiling     = words_per_ui_clk * f_ui / f_array
//
//   With ui_clk = f_array = 100 MHz the ratio is 1, so beta_ceiling is just
//   4 * BEATS / CYC.  Compare against the fold-average demand
//   2sK/(K+2(s-1)+H) = 4096/365 = 11.2 words per array cycle at s=8, K=256.
//
// STILL A CEILING, NOT BETA
//   Sequential addresses, so row hits throughout; no refresh contention under
//   mixed load; no writes competing for the bus.  And it measures DRAM -> here,
//   which is main_1 Eq. 2's aggregate beta -- NOT operand_throttle's per-step
//   beta (2N words per reduction step, no buffering).  Name them differently in
//   the paper or the knee at 16 and the ceiling measured here will read as a
//   contradiction.
//
// SELF-DIAGNOSIS -- the report always comes out
//   BEATS=00000000 CYC=<timeout>  AR accepted, no data came back
//   BEATS=00000000 CYC=00000000   arready never asserted; no AR was accepted
//   BEATS < expected              partial; the timeout fired
//   SINK=00000000                 all-zero data.  Legal on DRAM never written,
//                                 but also what a dead bus looks like.
// -----------------------------------------------------------------------------

`default_nettype none

module ddr3_bw_probe_axi #(
  parameter int unsigned AXI_ADDR_W  = 29,
  parameter int unsigned AXI_DATA_W  = 128,
  parameter int unsigned AXI_ID_W    = 2,
  parameter int unsigned BURST_LEN   = 16,          // beats per burst (<= 256)
  parameter int unsigned N_BURSTS    = 32'd4096,    // total beats = this * BURST_LEN
  parameter int unsigned MAX_OUTST   = 8,           // AR issued but not yet completed
  parameter int unsigned BASE_ADDR   = 0,
  parameter int unsigned TIMEOUT     = 32'h1000_0000,
  parameter int unsigned GAP_CYCLES  = 32'd100_000_000,
  parameter int unsigned CLK_HZ      = 100_000_000, // ui_clk
  parameter int unsigned BAUD        = 115_200
) (
  input  wire                     clk,              // ui_clk
  input  wire                     rst_n,            // ~ui_clk_sync_rst
  input  wire                     init_calib_complete,

  // ---- AXI4 read address channel -----------------------------------------
  output wire [AXI_ID_W-1:0]      m_axi_arid,
  output logic [AXI_ADDR_W-1:0]   m_axi_araddr,
  output wire [7:0]               m_axi_arlen,
  output wire [2:0]               m_axi_arsize,
  output wire [1:0]               m_axi_arburst,
  output wire [0:0]               m_axi_arlock,
  output wire [3:0]               m_axi_arcache,
  output wire [2:0]               m_axi_arprot,
  output wire [3:0]               m_axi_arqos,
  output logic                    m_axi_arvalid,
  input  wire                     m_axi_arready,

  // ---- AXI4 read data channel --------------------------------------------
  input  wire [AXI_DATA_W-1:0]    m_axi_rdata,
  input  wire [1:0]               m_axi_rresp,
  input  wire                     m_axi_rlast,
  input  wire                     m_axi_rvalid,
  output wire                     m_axi_rready,

  // ---- report -------------------------------------------------------------
  output wire                     uart_tx_pin,
  output wire                     running,
  output wire                     reported,

  // ---- the same three numbers, for a VIO / ILA to read over JTAG ----------
  // The UART is one way out; this is the other.  Both read the SAME latched
  // registers, so a disagreement between them is a transport bug, not a
  // measurement bug.
  output wire [31:0]              dbg_beats,
  output wire [31:0]              dbg_cyc,
  output wire [31:0]              dbg_sink
);

  assign dbg_beats = beats_r;
  assign dbg_cyc   = cyc_r;
  assign dbg_sink  = sink_r;

  localparam int unsigned TOTAL_BEATS = N_BURSTS * BURST_LEN;
  localparam int unsigned BURST_BYTES = BURST_LEN * (AXI_DATA_W / 8);
  localparam int          MSG_LEN     = 43;

  // AXI constants: full-width INCR bursts, no locking, no caching hints.
  // arsize is log2(bytes per beat) = log2(128/8) = 4.
  localparam logic [2:0] SIZE_FULL = 3'd4;
  localparam logic [1:0] BURST_INCR = 2'b01;

  typedef enum logic [1:0] {S_CALIB, S_RUN, S_SEND, S_GAP} state_t;

  state_t      state;
  logic [31:0] ar_issued, beats, cyc, sink;
  logic [31:0] beats_r, cyc_r, sink_r;
  logic        started;
  logic [$clog2(MAX_OUTST+1)-1:0] outst;
  logic [5:0]  send_idx;
  logic [31:0] gap_cnt;
  logic        send_done;
  wire         u_ready;

  wire ar_fire   = m_axi_arvalid & m_axi_arready;
  wire r_fire    = m_axi_rvalid  & m_axi_rready;
  wire all_done  = (beats == TOTAL_BEATS);
  wire win_over  = all_done | (cyc == TIMEOUT);

  assign m_axi_arid    = '0;
  assign m_axi_arlen   = 8'(BURST_LEN - 1);
  assign m_axi_arsize  = SIZE_FULL;
  assign m_axi_arburst = BURST_INCR;
  assign m_axi_arlock  = 1'b0;
  assign m_axi_arcache = 4'b0011;      // normal non-cacheable bufferable
  assign m_axi_arprot  = 3'b000;
  assign m_axi_arqos   = 4'b0000;
  assign m_axi_rready  = 1'b1;         // never stall the data channel

  assign running  = (state == S_RUN);
  assign reported = (state == S_SEND) | (state == S_GAP);

  // ---- issue, count, close the window ------------------------------------
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      state         <= S_CALIB;
      m_axi_araddr  <= AXI_ADDR_W'(BASE_ADDR);
      m_axi_arvalid <= 1'b0;
      ar_issued     <= '0;
      beats         <= '0;
      cyc           <= '0;
      sink          <= '0;
      started       <= 1'b0;
      outst         <= '0;
      beats_r       <= '0;
      cyc_r         <= '0;
      sink_r        <= '0;
    end else begin
      case (state)
        S_CALIB: begin
          if (init_calib_complete) begin
            state         <= S_RUN;
            m_axi_arvalid <= 1'b1;
          end
        end

        S_RUN: begin
          // address channel: keep AR asserted while there is work and the
          // outstanding budget allows another burst
          if (ar_fire) begin
            ar_issued    <= ar_issued + 1'b1;
            m_axi_araddr <= m_axi_araddr + AXI_ADDR_W'(BURST_BYTES);
            started      <= 1'b1;
          end

          // outstanding accounting: +1 on an accepted AR, -1 on the burst's
          // last beat.  Both in the same cycle cancel out.
          if (ar_fire && !(r_fire && m_axi_rlast))      outst <= outst + 1'b1;
          else if (!ar_fire && (r_fire && m_axi_rlast)) outst <= outst - 1'b1;

          m_axi_arvalid <= ((ar_issued + (ar_fire ? 32'd1 : 32'd0)) < N_BURSTS)
                           && ((outst + (ar_fire ? 1 : 0)
                                      - ((r_fire && m_axi_rlast) ? 1 : 0)) < MAX_OUTST);

          // the measurement window opens on the first ACCEPTED address
          if (started) cyc <= cyc + 1'b1;

          if (r_fire) begin
            beats <= beats + 1'b1;
            sink  <= sink ^ m_axi_rdata[31:0];   // keeps the read path alive
          end

          if (win_over) begin
            beats_r       <= beats;
            cyc_r         <= cyc;
            sink_r        <= sink;
            m_axi_arvalid <= 1'b0;
            state         <= S_SEND;
          end
        end

        S_SEND: if (send_done) state <= S_GAP;
        S_GAP:  if (gap_cnt == GAP_CYCLES) state <= S_SEND;
        default: state <= S_CALIB;
      endcase
    end
  end

  // ---- report formatting (identical to the native-UI probe) --------------
  function automatic logic [7:0] nib(input logic [3:0] n);
    nib = (n < 4'd10) ? (8'h30 + {4'b0, n}) : (8'h37 + {4'b0, n});
  endfunction

  function automatic logic [63:0] hex8(input logic [31:0] v);
    for (int k = 0; k < 8; k++)
      hex8[(7-k)*8 +: 8] = nib(v[(7-k)*4 +: 4]);
  endfunction

  wire [MSG_LEN*8-1:0] msg_vec = {
    "BEATS=", hex8(beats_r),
    " CYC=",  hex8(cyc_r),
    " SINK=", hex8(sink_r),
    8'h0D, 8'h0A
  };

  wire [7:0] u_data  = msg_vec[(MSG_LEN-1-send_idx)*8 +: 8];
  wire       u_valid = (state == S_SEND) & ~send_done;

  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      send_idx  <= '0;
      send_done <= 1'b0;
      gap_cnt   <= '0;
    end else if (state == S_SEND) begin
      gap_cnt <= '0;
      if (u_valid && u_ready) begin
        if (send_idx == 6'(MSG_LEN - 1)) send_done <= 1'b1;
        else                             send_idx  <= send_idx + 1'b1;
      end
    end else if (state == S_GAP) begin
      send_idx  <= '0;
      send_done <= 1'b0;
      gap_cnt   <= gap_cnt + 1'b1;
    end
  end

  uart_tx #(.CLK_HZ(CLK_HZ), .BAUD(BAUD)) u_uart (
    .clk(clk), .rst_n(rst_n),
    .data(u_data), .valid(u_valid), .ready(u_ready),
    .tx(uart_tx_pin)
  );

endmodule

`default_nettype wire
