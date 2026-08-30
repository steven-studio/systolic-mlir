// -----------------------------------------------------------------------------
// mig_ui_stub.sv -- behavioural model of the MIG 7-series user interface,
// read path.  Simulation only; never synthesised.
//
// Models the three things that break naive DMA engines:
//   * app_rdy deasserts unpredictably, so a command must be held until it is
//     accepted;
//   * read data comes back an arbitrary number of cycles later, so returns
//     overlap with issue;
//   * returns are IN ORDER, which is what lets the engine use a plain counter
//     as the destination address.
//
// RDY_PATTERN and LAT_MIN/LAT_MAX let a test make the interface as hostile as
// it likes without changing the engine.
// -----------------------------------------------------------------------------

`timescale 1ns/1ps
`default_nettype none

module mig_ui_stub #(
  parameter integer APP_DATA_W = 128,
  parameter integer APP_ADDR_W = 28,
  parameter integer MEM_BEATS  = 4096,     // modelled memory size, in beats
  parameter integer LAT_MIN    = 6,
  parameter integer LAT_MAX    = 23,
  parameter [31:0]  RDY_PATTERN = 32'b1101_1111_0111_1110_1111_1011_1111_0111
) (
  input  wire                   clk,
  input  wire                   rst_n,
  output wire                   init_calib_complete,

  input  wire [APP_ADDR_W-1:0]  app_addr,
  input  wire [2:0]             app_cmd,
  input  wire                   app_en,
  output wire                   app_rdy,
  output reg  [APP_DATA_W-1:0]  app_rd_data,
  output reg                    app_rd_data_valid
);

  localparam integer BYTES_PER_BEAT = APP_DATA_W / 8;

  // ---- modelled memory ---------------------------------------------------
  reg [APP_DATA_W-1:0] mem [0:MEM_BEATS-1];

  // ---- calibration -------------------------------------------------------
  reg calib = 0;
  assign init_calib_complete = calib;
  initial begin
    repeat (20) @(posedge clk);
    calib = 1;
  end

  // ---- app_rdy backpressure ---------------------------------------------
  reg [4:0] rdy_phase = 0;
  always @(posedge clk) rdy_phase <= rdy_phase + 1'b1;
  assign app_rdy = calib & RDY_PATTERN[rdy_phase];

  // ---- in-order return queue --------------------------------------------
  // Each accepted command becomes an entry with a due time; entries retire in
  // order, so a later command with a shorter latency waits for its turn.
  integer q_head = 0, q_tail = 0;
  reg [APP_DATA_W-1:0] q_data [0:255];
  integer              q_due  [0:255];
  integer              now = 0;
  integer              lat_seed = 32'h1234_5678;
  integer              lat;
  integer              beat_idx;

  always @(posedge clk) now <= now + 1;

  always @(posedge clk) begin
    if (!rst_n) begin
      q_head <= 0; q_tail <= 0;
    end else begin
      // accept a command
      if (app_en && app_rdy && (app_cmd == 3'b001)) begin
        beat_idx = app_addr / BYTES_PER_BEAT;
        lat_seed = (lat_seed * 1103515245 + 12345);
        lat      = LAT_MIN + ((lat_seed >>> 16) % (LAT_MAX - LAT_MIN + 1));
        if (lat < LAT_MIN) lat = LAT_MIN;
        q_data[q_tail % 256] <= mem[beat_idx];
        q_due [q_tail % 256] <= now + lat;
        q_tail <= q_tail + 1;
      end
    end
  end

  // retire in order, at most one beat per cycle
  always @(posedge clk) begin
    if (!rst_n) begin
      app_rd_data_valid <= 1'b0;
      app_rd_data       <= '0;
    end else begin
      app_rd_data_valid <= 1'b0;
      if ((q_head < q_tail) && (now >= q_due[q_head % 256])) begin
        app_rd_data       <= q_data[q_head % 256];
        app_rd_data_valid <= 1'b1;
        q_head            <= q_head + 1;
      end
    end
  end

endmodule

`default_nettype wire
