// -----------------------------------------------------------------------------
// dma_engine.sv -- descriptor-driven read engine against the MIG 7-series
// user interface.
//
// SCOPE.  Read path only: DDR3 -> destination stream.  Writeback is step 4 of
// the bring-up order and is deliberately not here; keeping the read path alone
// means step 3 ("DMA into the operand buffers, run one fold, compare bit-exact
// against the UART-fed result") is reachable with this module plus a buffer
// writer, and nothing else.
//
// UNIT.  One descriptor is one invocation's operand set, so that the DMA's
// unit of work is identical to the cost model's unit and dma_bytes_per_cycle
// in the systolic.device attribute has a direct hardware meaning.
//
// MIG UI CONTRACT (7-series, 4:1 ratio, x16 DDR3 -> APP_DATA_W = 128):
//   * app_en/app_addr/app_cmd must be held until app_rdy is seen high in the
//     same cycle; that cycle is the acceptance.
//   * Reads return in order, some cycles later, one beat per accepted command,
//     flagged by app_rd_data_valid.  Returns may overlap with issue, so the
//     issue counter and the return counter are independent -- this is the one
//     structural thing a naive implementation gets wrong.
//   * app_addr is a byte address; one command moves APP_DATA_W/8 bytes.
//
// The destination side is a simple write stream (beat index, data) so that it
// can drive either a scratch BRAM (bring-up step 2) or the operand buffer
// writer (step 3) without changing this module.
// -----------------------------------------------------------------------------

`default_nettype none

module dma_engine #(
  parameter integer APP_DATA_W = 128,      // MIG UI data width
  parameter integer APP_ADDR_W = 28,       // MIG UI byte-address width
  parameter integer BEAT_W     = 16,       // beats per descriptor, max 65535
  parameter integer MAX_OUTSTANDING = 8    // read commands allowed in flight
) (
  input  wire                    clk,               // ui_clk
  input  wire                    rst_n,             // ~ui_clk_sync_rst
  input  wire                    init_calib_complete,

  // ---- descriptor in (from the UART control plane) -----------------------
  input  wire                    desc_valid,
  output wire                    desc_ready,
  input  wire [APP_ADDR_W-1:0]   desc_addr,         // byte address, beat aligned
  input  wire [BEAT_W-1:0]       desc_beats,        // number of UI beats
  input  wire [7:0]              desc_tag,

  // ---- completion out ----------------------------------------------------
  output logic                   done_valid,        // one-cycle pulse
  output logic [7:0]             done_tag,

  // ---- MIG UI, read path -------------------------------------------------
  output logic [APP_ADDR_W-1:0]  app_addr,
  output logic [2:0]             app_cmd,
  output logic                   app_en,
  input  wire                    app_rdy,
  input  wire [APP_DATA_W-1:0]   app_rd_data,
  input  wire                    app_rd_data_valid,

  // ---- destination backpressure -----------------------------------------
  // The destination (a CDC FIFO) cannot refuse a beat once MIG has accepted
  // the command for it, so issue must stop while there is less than
  // MAX_OUTSTANDING slots of room.  Size the FIFO's AF_MARGIN >= MAX_OUTSTANDING
  // or beats are dropped silently.
  input  wire                    dst_almost_full,

  // ---- destination write stream -----------------------------------------
  output logic                   dst_wr_en,
  output logic [BEAT_W-1:0]      dst_wr_beat,       // beat index in this descriptor
  output logic [APP_DATA_W-1:0]  dst_wr_data,
  output logic [7:0]             dst_wr_tag,

  // ---- observability -----------------------------------------------------
  output logic [31:0]            busy_cycles,       // cycles not IDLE
  output logic [31:0]            rdy_stall_cycles,  // cycles app_en && !app_rdy
  input  wire                    stat_clear
);

  localparam integer BYTES_PER_BEAT = APP_DATA_W / 8;
  localparam [2:0]   CMD_READ       = 3'b001;

  localparam [1:0] S_IDLE  = 2'd0,
                   S_ISSUE = 2'd1,
                   S_DRAIN = 2'd2;

  // Every counter here is incremental.  Recomputing a wide value each cycle
  // inside the issue loop -- outstanding = issue_cnt - ret_cnt, or
  // app_addr = base + issue_cnt*BYTES -- puts an adder and a comparator on the
  // path that decides app_en, which feeds cmd_accepted, which updates the very
  // counters being read.  That loop missed 200 MHz by 65 ps out of context.
  localparam integer CRED_W = $clog2(MAX_OUTSTANDING + 1);

  logic [1:0]            state;
  logic [7:0]            tag;
  logic [APP_ADDR_W-1:0] addr_r;        // next command address, incremental
  logic [BEAT_W-1:0]     issue_left;    // commands still to issue
  logic [BEAT_W-1:0]     ret_left;      // beats still to return
  logic [BEAT_W-1:0]     ret_idx;       // destination beat index
  logic [CRED_W-1:0]     credit;        // issue credits: MAX_OUTSTANDING .. 0

  assign desc_ready = (state == S_IDLE) && init_calib_complete;

  // Command acceptance is app_en && app_rdy in the same cycle.
  wire cmd_accepted = app_en && app_rdy;

  // ---- issue side --------------------------------------------------------
  // credit != 0 is a zero-compare on a small registered counter, and the
  // address is a register, so the app_en loop is one LUT deep.
  wire may_issue = !dst_almost_full && (credit != '0);

  always_comb begin
    app_en   = (state == S_ISSUE) && may_issue;
    app_cmd  = CMD_READ;
    app_addr = addr_r;
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
      done_valid <= 1'b0;
      done_tag   <= '0;
    end else begin
      done_valid <= 1'b0;

      // Credit: spent when a command is accepted, returned when its beat comes
      // back.  Both events can happen in the same cycle, and then it is a wash.
      if (cmd_accepted && !app_rd_data_valid)      credit <= credit - 1'b1;
      else if (!cmd_accepted && app_rd_data_valid) credit <= credit + 1'b1;

      // Return collection runs in every state: beats arrive while later
      // commands are still being issued, and after the last one is accepted.
      if (app_rd_data_valid && (state != S_IDLE)) begin
        ret_idx  <= ret_idx  + 1'b1;
        ret_left <= ret_left - 1'b1;
      end

      case (state)
        S_IDLE: begin
          if (desc_valid && desc_ready && (desc_beats != 0)) begin
            tag        <= desc_tag;
            addr_r     <= desc_addr;
            issue_left <= desc_beats;
            ret_left   <= desc_beats;
            ret_idx    <= '0;
            credit     <= CRED_W'(MAX_OUTSTANDING);
            state      <= S_ISSUE;
          end
        end

        S_ISSUE: begin
          if (cmd_accepted) begin
            addr_r     <= addr_r + APP_ADDR_W'(BYTES_PER_BEAT);
            issue_left <= issue_left - 1'b1;
            if (issue_left == 1) state <= S_DRAIN;
          end
        end

        S_DRAIN: begin
          if (app_rd_data_valid && (ret_left == 1)) begin
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
    dst_wr_en   = app_rd_data_valid && (state != S_IDLE);
    dst_wr_beat = ret_idx;
    dst_wr_data = app_rd_data;
    dst_wr_tag  = tag;
  end

  // ---- statistics --------------------------------------------------------
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      busy_cycles      <= '0;
      rdy_stall_cycles <= '0;
    end else if (stat_clear) begin
      busy_cycles      <= '0;
      rdy_stall_cycles <= '0;
    end else begin
      if (state != S_IDLE)       busy_cycles      <= busy_cycles + 1'b1;
      if (app_en && !app_rdy)    rdy_stall_cycles <= rdy_stall_cycles + 1'b1;
    end
  end

endmodule

`default_nettype wire
