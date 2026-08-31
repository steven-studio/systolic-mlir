// -----------------------------------------------------------------------------
// ddr3_bw_probe.sv -- measures the DDR3 read-bandwidth CEILING on this board.
//
// WHAT IT MEASURES
//   Issues N_READS back-to-back sequential read commands at the MIG user
//   interface and counts, in ui_clk cycles, how long the returned beats take.
//   Nothing else is in the design -- no array, no operand buffers, no DMA
//   engine -- so the number is the transport ceiling, not the system's beta.
//
//   Sequential addresses mean row hits.  The real operand pattern interleaves
//   an A row-major stream with a B column-major stream and WILL page-thrash;
//   refresh also steals cycles under sustained load.  So:
//
//       report this as "beta_ceiling", never as "beta".
//
//   It is still worth having: it bounds the axis the fleet model sweeps, and
//   it tells you which side of the array's demand this board lives on.
//
// ARITHMETIC (do it on the host, not here)
//   One accepted command returns one app_rd_data beat.  For DDR3 x16 at a 4:1
//   UI ratio, app_data_width = 2 * 4 * 16 = 128 bits = 16 bytes = 4 fp32 words.
//
//       bytes            = BEATS * (APP_DATA_W/8)
//       words_per_ui_clk = 4 * BEATS / CYC
//       beta_ceiling     = words_per_ui_clk * f_ui / f_array   [words/array cycle]
//
//   Compare against the fold-average operand demand, 2*s*K / (K + 2(s-1) + H).
//   At s=8, K=256, H=95 that is 4096/365 = 11.2 words per array cycle.
//
//   CYC carries a +1 cycle bias (the window closes one cycle after the last
//   beat is registered).  On 65536 commands that is 6 ppm; ignore it, but do
//   not ignore it if you ever run this with N_READS in the tens.
//
// SELF-DIAGNOSIS
//   The report always comes out, even when the memory is dead, and the numbers
//   say where it died:
//     BEATS=00000000, CYC=10000000  -> commands accepted, nothing came back
//     BEATS=00000000, CYC=00000000  -> app_rdy never went high; no command was
//                                      ever accepted (check ui_clk and reset)
//     BEATS < N_READS               -> partial returns; the timeout fired
//     SINK=00000000                 -> data came back all-zero.  On DRAM that
//                                      was never written that is legal, but it
//                                      is also what a dead DQ bus looks like --
//                                      write a pattern first if you want this
//                                      column to mean anything.
//   The report repeats forever with a gap, so opening the terminal late is fine.
//
// NOTE ON ADDR_STRIDE
//   MIG 7-series app_addr granularity depends on the configured ratio and burst
//   length; for BL8 the column address advances by 8 per command.  Take the
//   value from the generated example design rather than trusting this default.
//   For a BANDWIDTH measurement a wrong stride is harmless -- too small just
//   re-reads the same row, which still measures the return rate -- but fix it
//   before this address path ever carries real operands.
// -----------------------------------------------------------------------------

`default_nettype none

module ddr3_bw_probe #(
  parameter int unsigned APP_ADDR_W  = 29,
  parameter int unsigned APP_DATA_W  = 128,
  parameter int unsigned N_READS     = 32'd65536,
  parameter int unsigned ADDR_STRIDE = 8,
  parameter int unsigned BASE_ADDR   = 0,
  parameter int unsigned TIMEOUT     = 32'h1000_0000,   // ~1.3 s at 200 MHz
  parameter int unsigned GAP_CYCLES  = 32'd200_000_000, // ~1 s between reports
  parameter int unsigned CLK_HZ      = 200_000_000,
  parameter int unsigned BAUD        = 115_200
) (
  input  wire                    clk,                   // ui_clk
  input  wire                    rst_n,
  input  wire                    init_calib_complete,

  // ---- MIG 7-series user interface (read path only) ----------------------
  output logic [APP_ADDR_W-1:0]  app_addr,
  output wire  [2:0]             app_cmd,
  output logic                   app_en,
  input  wire                    app_rdy,
  input  wire [APP_DATA_W-1:0]   app_rd_data,
  input  wire                    app_rd_data_valid,

  // ---- report ------------------------------------------------------------
  output wire                    uart_tx_pin,
  output wire                    running,               // drive an LED
  output wire                    reported                // drive an LED
);

  localparam logic [2:0] CMD_READ = 3'b001;
  localparam int         MSG_LEN  = 43;  // 6 + 8 + 5 + 8 + 6 + 8 + 2

  typedef enum logic [1:0] {S_CALIB, S_RUN, S_SEND, S_GAP} state_t;

  // ---- declarations (all before first use) -------------------------------
  state_t      state;
  logic [31:0] issued, beats, cyc, sink;
  logic [31:0] beats_r, cyc_r, sink_r;
  logic        started;
  logic [5:0]  send_idx;
  logic [31:0] gap_cnt;
  logic        send_done;
  wire         u_ready;

  wire cmd_accepted = app_en & app_rdy;
  wire all_returned = (beats == N_READS);
  wire window_over  = all_returned | (cyc == TIMEOUT);

  assign app_cmd  = CMD_READ;
  assign running  = (state == S_RUN);
  assign reported = (state == S_SEND) | (state == S_GAP);

  // ---- issue, count, and close the window --------------------------------
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      state    <= S_CALIB;
      app_addr <= APP_ADDR_W'(BASE_ADDR);
      app_en   <= 1'b0;
      issued   <= '0;
      beats    <= '0;
      cyc      <= '0;
      sink     <= '0;
      started  <= 1'b0;
      beats_r  <= '0;
      cyc_r    <= '0;
      sink_r   <= '0;
    end else begin
      case (state)
        S_CALIB: begin
          if (init_calib_complete) begin
            state  <= S_RUN;
            app_en <= 1'b1;
          end
        end

        S_RUN: begin
          // command issue: app_en is held until app_rdy takes the command
          if (cmd_accepted) begin
            issued   <= issued + 1'b1;
            app_addr <= app_addr + APP_ADDR_W'(ADDR_STRIDE);
            started  <= 1'b1;
            if (issued + 32'd1 == N_READS) app_en <= 1'b0;
          end

          // the measurement window opens on the first accepted command
          if (started) cyc <= cyc + 1'b1;

          // returns
          if (app_rd_data_valid) begin
            beats <= beats + 1'b1;
            sink  <= sink ^ app_rd_data[31:0];   // keeps the read path alive
          end

          if (window_over) begin
            beats_r <= beats;
            cyc_r   <= cyc;
            sink_r  <= sink;
            app_en  <= 1'b0;
            state   <= S_SEND;
          end
        end

        S_SEND: if (send_done) state <= S_GAP;

        S_GAP:  if (gap_cnt == GAP_CYCLES) state <= S_SEND;   // repeat forever

        default: state <= S_CALIB;
      endcase
    end
  end

  // ---- report formatting -------------------------------------------------
  function automatic logic [7:0] nib(input logic [3:0] n);
    nib = (n < 4'd10) ? (8'h30 + {4'b0, n}) : (8'h37 + {4'b0, n});  // 0-9, A-F
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
