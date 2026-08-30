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

  logic [1:0]            state;
  logic [APP_ADDR_W-1:0] base_addr;
  logic [BEAT_W-1:0]     beats;
  logic [7:0]            tag;
  logic [BEAT_W-1:0]     issue_cnt;     // commands accepted by the controller
  logic [BEAT_W-1:0]     ret_cnt;       // beats returned

  assign desc_ready = (state == S_IDLE) && init_calib_complete;

  // Command acceptance is app_en && app_rdy in the same cycle.
  wire cmd_accepted = app_en && app_rdy;

  // ---- issue side --------------------------------------------------------
  // Outstanding = accepted commands whose data has not yet come back.
  wire [BEAT_W:0] outstanding = {1'b0, issue_cnt} - {1'b0, ret_cnt};
  wire            may_issue   = !dst_almost_full &&
                                (outstanding < MAX_OUTSTANDING);

  always_comb begin
    app_en   = (state == S_ISSUE) && may_issue;
    app_cmd  = CMD_READ;
    app_addr = base_addr + (APP_ADDR_W'(issue_cnt) * BYTES_PER_BEAT);
  end

  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      state      <= S_IDLE;
      base_addr  <= '0;
      beats      <= '0;
      tag        <= '0;
      issue_cnt  <= '0;
      ret_cnt    <= '0;
      done_valid <= 1'b0;
      done_tag   <= '0;
    end else begin
      done_valid <= 1'b0;

      // Return collection runs in every state: beats can arrive while later
      // commands are still being issued, and after the last one is accepted.
      if (app_rd_data_valid && (state != S_IDLE))
        ret_cnt <= ret_cnt + 1'b1;

      case (state)
        S_IDLE: begin
          if (desc_valid && desc_ready && (desc_beats != 0)) begin
            base_addr <= desc_addr;
            beats     <= desc_beats;
            tag       <= desc_tag;
            issue_cnt <= '0;
            ret_cnt   <= '0;
            state     <= S_ISSUE;
          end
        end

        S_ISSUE: begin
          if (cmd_accepted) begin
            issue_cnt <= issue_cnt + 1'b1;
            if (issue_cnt + 1'b1 == beats) state <= S_DRAIN;
          end
        end

        S_DRAIN: begin
          // ret_cnt is incremented above; compare against the incremented value
          // so that the final beat completes the descriptor in its own cycle.
          if (app_rd_data_valid && (ret_cnt + 1'b1 == beats)) begin
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
    dst_wr_beat = ret_cnt;
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
